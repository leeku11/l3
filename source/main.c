/* Copyright Jamie Honan, 2001.  Distributed under the GPL.
   This program comes with ABSOLUTELY NO WARRANTY.
   See the file COPYING for license details.
   */
#define KEYBD_EXTERN
#include "global.h"
#include "timer.h"
#include "keymap.h"
#include "print.h"
#include "led.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>     /* for _delay_ms() */

#include "hwport.h"
#include "usbdrv.h"
#include "hwaddress.h"
#include "matrix.h"
#include "macro.h"

#ifdef SUPPORT_TINY_CMD
#include "tinycmd.h"
#include "tinycmdpkt.h"
#endif // SUPPORT_TINY_CMD


#ifdef SUPPORT_I2C
#include "i2c.h"        // include i2c support

#define LOCAL_ADDR      0xA0
#define TARGET_ADDR     0xB0

// local data buffer
unsigned char localBuffer[0x60];
unsigned char localBufferLength;
#endif // SUPPORT_I2C

int8_t usbmode;
extern uint8_t usbmain(void);
extern uint8_t ps2main(void);


#ifdef DEBUG
void enable_printf(void)
{
	stdout = &mystdout;
    DDRD |= 0x01;

	UBRRH = (UBRR>>8);
	UBRRL = UBRR;
    
	UCSRA = 0x00;									   // asynchronous normal mode
	UCSRB = 0x08;									   // Tx enable, 8 data
	UCSRC = 0x86;									   // no parity, 1 stop, 8 data
}
#else
void enable_printf(void)
{
}
#endif





/* control zener diode for level shift signal line
    1 : TR on - 3v level
    0 : TR off - 5v level
*/
void DPpullEn(uint8_t enable)
{
    if (enable)
    {
        sbi(USB_LEVEL_SHIFT_PORT, USB_LEVEL_SHIFT_PIN);     // pullup
        cbi(USB_LEVEL_SHIFT_DDR, USB_LEVEL_SHIFT_PIN);      // INPUT

        sbi(PS2_CLK_PULLUP_DDR, PS2_CLK_PULLUP_PIN);      // OUT
        cbi(PS2_CLK_PULLUP_PORT, PS2_CLK_PULLUP_PIN);     // drive 0
    }
    else
    {
        sbi(USB_LEVEL_SHIFT_DDR, USB_LEVEL_SHIFT_PIN);      // OUTPUT
        cbi(USB_LEVEL_SHIFT_PORT, USB_LEVEL_SHIFT_PIN);     // drive 0
        
        sbi(PS2_CLK_PULLUP_PORT, PS2_CLK_PULLUP_PIN);     // pullup
        cbi(PS2_CLK_PULLUP_DDR, PS2_CLK_PULLUP_PIN);      // INPUT
    }
}



int portInit(void)
{
//  initialize matrix ports - cols, rows
//    PA       0:7      col      (6, 7 reserved)
//    PG       0:1      row 0, 1
//               2         N/A
//               3         N/A
//    PC       0:7      row 2:9
//    PF        0:7      row 10:17
    
//    PB
//    0      INDI0
//    1      SCK (ISP)
//    2      INDI1
//    3      INDI2
//    4(OC0)       LED_PIN_WASD
//    5(OC1A)     LED_PIN_PAD
//    6(OC1B)     LED_PIN_PRT
//    7(OC1C)     LED_VESEL
    
//    PD
//    0(INT0)      D+
//    1(INT1)      D-
//    2               N/A
//    3               LED_NUM
//    4               LED_CAPS
//    5               LED_SCR
//    6               Zener Diode
//    7               D+ pull-up register
    
//    PE
//    0(PDI)         MOSI
//    1(PDO)        MISO
//    2                N/A
//    3(OC3A)     LED_PIN_Fx
//    4(OC3B)     LED_PIN_BASE
//    5(OC3C)     LED_PIN_ESC
//    6                N/A
//    7                N/A

    
    // signal direction : col -> row

//  Matrix
    PORTA   = 0xFF; //  row
    PORTB   = 0xFF; //  row
    PORTC   = 0xFC; // LED5,6 off, COL[0-5] pull-up

    DDRA	= 0x00; // col
    DDRB    = 0x00; // LED_VESEL, LED_PIN_PRT, LED_PIN_PAD, LED_PIN_WASD OUT        (11110000)
    DDRC	= 0x03; // row 2, 3, 4, 5, 6, 7, 8, 9

    PORTD   = 0xF1; // DPpull-up(Low), Zener(pull-up), LED_SCR, LED_CAPS, LED_NUM (0ff), D-(0), D+(0)
    DDRD    = 0x03; // DPpull-up(OUT), Zener(OUT), LED_SCR, LED_CAPS, LED_NUM (OUT), D-(INPUT), D+(INPUT)

    return 0;
}

#define CHECK_U (~PINA & 0x80)  // col2-row7 => U
#define CHECK_P (~PINB & 0x04)  // col2-row10 => P

#ifdef SUPPORT_I2C
// slave operations
void i2cSlaveReceiveService(u08 receiveDataLength, u08* receiveData)
{
    u08 i;

    // this function will run when a master somewhere else on the bus
    // addresses us and wishes to write data to us

    //showByte(*receiveData);
    cbi(PORTB, PB6);

    // copy the received data to a local buffer
    for(i=0; i<receiveDataLength; i++)
    {
        localBuffer[i] = *receiveData++;
    }
    localBufferLength = receiveDataLength;
}

u08 i2cSlaveTransmitService(u08 transmitDataLengthMax, u08* transmitData)
{
    u08 i;

    // this function will run when a master somewhere else on the bus
    // addresses us and wishes to read data from us

    //showByte(*transmitData);
    cbi(PORTB, PB7);

    // copy the local buffer to the transmit buffer
    for(i=0; i<localBufferLength; i++)
    {
        *transmitData++ = localBuffer[i];
    }

    localBuffer[0]++;

    return localBufferLength;
}

void initI2C(void)
{
    // Initialze I2C
    i2cInit();
    // set local device address and allow response to general call
    i2cSetLocalDeviceAddr(LOCAL_ADDR, TRUE);
    // set the Slave Receive Handler function
    // (this function will run whenever a master somewhere else on the bus
    //	writes data to us as a slave)
    i2cSetSlaveReceiveHandler( i2cSlaveReceiveService );
    // set the Slave Transmit Handler function
    // (this function will run whenever a master somewhere else on the bus
    //	attempts to read data from us as a slave)
    i2cSetSlaveTransmitHandler( i2cSlaveTransmitService );
}

void tinycmd_ver(void)
{
    tinycmd_ver_req_type *p_ver_req = (tinycmd_ver_req_type *)localBuffer;
    
    p_ver_req->cmd_code = TINY_CMD_VER_F;
    p_ver_req->pkt_len = sizeof(tinycmd_ver_req_type);

    i2cMasterSend(TARGET_ADDR, p_ver_req->pkt_len, p_ver_req);
}

void tinycmd_reset(uint8_t type)
{
    tinycmd_reset_req_type *p_reset_req = (tinycmd_reset_req_type *)localBuffer;
    
    p_reset_req->cmd_code = TINY_CMD_RESET_F;
    p_reset_req->pkt_len = sizeof(tinycmd_reset_req_type);
    p_reset_req->type = TINY_RESET_HARD;

    i2cMasterSend(TARGET_ADDR, p_reset_req->pkt_len, p_reset_req);
}

void tinycmd_three_lock(uint8_t num, uint8_t caps, uint8_t scroll)
{
    tinycmd_three_lock_req_type *p_three_lock_req = (tinycmd_three_lock_req_type *)localBuffer;
    
    p_three_lock_req->cmd_code = TINY_CMD_THREE_LOCK_F;
    p_three_lock_req->pkt_len = sizeof(tinycmd_three_lock_req_type);
    p_three_lock_req->lock = num | caps | scroll;

    i2cMasterSend(TARGET_ADDR, p_three_lock_req->pkt_len, p_three_lock_req);
}

void tinycmd_led_on(uint8_t r, uint8_t g, uint8_t b)
{
#define MAX_LEVEL_MASK(a)               (a & 0x5F) // below 95
    uint8_t i;
    led_type led;
    tinycmd_led_req_type *p_led_req = (tinycmd_led_req_type *)localBuffer;
    
    p_led_req->cmd_code = TINY_CMD_LED_F;
    p_led_req->pkt_len = sizeof(tinycmd_led_req_type);

    p_led_req->num = 5;
    p_led_req->offset = 6;
    
    led.g = MAX_LEVEL_MASK(g);
    led.r = MAX_LEVEL_MASK(r);
    led.b = MAX_LEVEL_MASK(b);

    for(i = 0; i < p_led_req->num; i++)
    {
       p_led_req->led[i] = led;
    }

    i2cMasterSend(TARGET_ADDR, p_led_req->pkt_len, p_led_req);
}

void tinycmd_test(void)
{
    uint8_t i;
    tinycmd_test_req_type *p_test_req = (tinycmd_test_req_type *)localBuffer;

    p_test_req->cmd_code = TINY_CMD_TEST_F;
    p_test_req->pkt_len = sizeof(tinycmd_test_req_type);

    p_test_req->data_len = 14*3;
    for(i = 0; i < p_test_req->data_len; i++)
    {
        p_test_req->data[i] = 60+i;
    }

    i2cMasterSend(TARGET_ADDR, p_test_req->pkt_len, p_test_req);
}

void testI2C(uint8_t count)
{
    switch(count % 7)
    {
    case 0:
        tinycmd_ver();
        break;
    case 1:
        tinycmd_reset(TINY_RESET_SOFT);
        break;
    case 2:
        tinycmd_three_lock(4, 2, 1);
        break;
    case 3:
        tinycmd_test();
        break;
    case 4:
        tinycmd_led_on(0, 255, 255);
        break;
    case 5:
        tinycmd_led_on(255, 0, 255);
        break;
    case 6:
        tinycmd_led_on(255, 255, 0);
        break;
    default:
        break;
    }
}

#endif // SUPPORT_I2C

int8_t checkInterface(void)
{
    uint8_t cur_usbmode = 0;
    DDRC  |= BV(4);        //  col2
    PORTC &= ~BV(4);       //

    _delay_us(10);

    if (CHECK_U)   
    {
        cur_usbmode = 1;
        eeprom_write_byte(EEPADDR_USBPS2_MODE, cur_usbmode);
    }else if (CHECK_P)
    {
        cur_usbmode = 0;
        eeprom_write_byte(EEPADDR_USBPS2_MODE, cur_usbmode);
    }else
    {
        cur_usbmode = eeprom_read_byte(EEPADDR_USBPS2_MODE);
        if (cur_usbmode > 1)
            cur_usbmode = 1;    //default USB
    }
    return cur_usbmode;
}        
    

int main(void)
{
    
   enable_printf();
   portInit();
   usbmode = checkInterface();
   DPpullEn(usbmode);
//   led_off(LED_PIN_Fx);
   
//   timerInit();
//   timer1PWMInit(8);
//   timer2PWMInit(8);
   keymap_init();

#ifdef SUPPORT_I2C
   initI2C();
#endif // SUPPORT_I2C

   if(usbmode)
   {
      led_check(1);

    led_off(LED_PIN_Fx);
    led_off(LED_PIN_BASE);
    led_off(LED_PIN_WASD);
    
      led_mode_init();
      usbmain();
   }
   else
   {
    
    led_on(LED_PIN_Fx);
      led_check(0);
      led_mode_init();
      ps2main();
   }
   return 0;
}

