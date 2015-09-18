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
#include <string.h>

#include "hwport.h"
#include "usbdrv.h"
#include "hwaddress.h"
#include "matrix.h"
#include "macro.h"


#ifdef SUPPORT_I2C
#include "i2c.h"        // include i2c support
#include "tinycmdapi.h"

#define TINY_DETECT_RETRY        50
#define RGB_CHAIN_NUM   20
#define DEFAULT_LAYER   0

// local data buffer
uint8_t tinyExist = 1;
uint8_t localBuffer[0x4B];
uint8_t localBufferLength;
#endif // SUPPORT_I2C

extern uint8_t usbmain(void);
extern uint8_t ps2main(void);
extern uint32_t scankeycntms;

//global configuration strored in E2P
kbd_configuration_t kbdConf = 
{
    1,                  // ps2usb_mode
    DEFAULT_LAYER,      // keymapLayerIndex
    0,                  // swapCtrlCaps
    0,                  // swapAltGui
    0,                  // led_preset_index
    {{LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS}, //led_preset
     {LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS},
     {LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS}},
    3,                  // rgb_effect_index
    RGB_CHAIN_NUM,      // rgb_chain
    {{0, 250, 250}, {0, 250, 250},{0, 250, 100}, {0, 250, 250}, {0, 50, 250},  {0, 0, 250}, {250, 0, 0}, {250, 250, 0}, {100, 250,0},  {0, 250, 0}, // rgb_preset
     {0, 250, 0}, {0, 250, 0},   {100, 250,0},  {250, 250, 0}, {250, 0, 0}, {0, 0, 250}, {0, 50, 250},  {0, 250, 250}, {0, 250, 100}, {0, 250, 100}}, 
    {{ RGB_EFFECT_BOOTHID, 0, 0, 0 },                                                           // RGB_EFFECT_BOOTHID
     { RGB_EFFECT_FADE_BUF, FADE_HIGH_HOLD, FADE_LOW_HOLD, FADE_IN_ACCEL },                     // RGB_EFFECT_FADE_BUF
     { RGB_EFFECT_FADE_LOOP, FADE_HIGH_HOLD, FADE_LOW_HOLD, FADE_IN_ACCEL },                    // RGB_EFFECT_FADE_LOOP
     { RGB_EFFECT_HEARTBEAT_BUF, HEARTBEAT_HIGH_HOLD, HEARTBEAT_LOW_HOLD, HEARTBEAT_IN_ACCEL }, // RGB_EFFECT_HEARTBEAT_BUF
     { RGB_EFFECT_HEARTBEAT_LOOP, HEARTBEAT_HIGH_HOLD, HEARTBEAT_LOW_HOLD, HEARTBEAT_IN_ACCEL },// RGB_EFFECT_HEARTBEAT_LOOP
     { RGB_EFFECT_BASIC, HEARTBEAT_HIGH_HOLD, HEARTBEAT_LOW_HOLD, HEARTBEAT_IN_ACCEL }},        // RGB_EFFECT_HEARTBEAT_LOOP
    260,                // rgb_limit
    500,                // rgb_speed
    4,                  // matrix_debounce
    30,                 // sleepTimerMin
};

int portInit(void)
{
/*
PA  [0:7]   col[0:7]
PB  [0:7]   col[8:15]
PC  [0]     SCL
    [1]     SDA
    [2:6]   row[0:6]
PD  [0]     USB D+- level shifter(high 3.3v, low 5v)
    [1]     PS2 pull-up for CLK
    [2]     CLK[D+]
    [3]     DATA[D-]
    [4:7]   col[16:19]
*/
    
// signal direction : row -> col
    PORTA   = 0xFF; // pull up
    DDRA    = 0x00; // input
       
    PORTB   = 0xFF; // pull up
    DDRB    = 0x00; // input

    PORTC   = 0xFF; // pull up
    DDRC	   = 0x00; // input

    PORTD   = 0xF1; // col(pull up) D-(pull up) D+(pull up) PS2PU(low) USBSHIFT(high)
    DDRD    = 0x03; // 

    return 0;
}


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
#endif // SUPPORT_I2C


uint8_t establishSlaveComm(void)
{
    uint8_t ret = 0;
#ifdef SUPPORT_TINY_CMD
    uint16_t retry = 0;

    //initI2C();
    // Establish a communication with tiny slave
    while(ret == 0 && (retry++ < TINY_DETECT_RETRY))
    {
        // proving
        _delay_ms(50);
        ret = tinycmd_ver(TRUE);
        _delay_ms(50);
        if(ret)
        { 
            tinycmd_config(kbdConf.rgb_chain + 1, kbdConf.rgb_limit, TRUE);
            break;
        }
    }
#endif // SUPPORT_TINY_CMD

    return ret;    
}  

int8_t setPS2USB(void)
{
    DDRC  |= BV(4);        //  row2
    PORTC &= ~BV(4);       //

    _delay_us(10);

    if (CHECK_U)   
    {
        kbdConf.ps2usb_mode = 1;
    }else if (CHECK_P)
    {
        kbdConf.ps2usb_mode = 0;
    }else
    {
        if (kbdConf.ps2usb_mode > 1)
            kbdConf.ps2usb_mode = 1;    //default USB
    }
    /* control zener diode for level shift signal line
        1 : TR on - 3v level
        0 : TR off - 5v level
    */
    if (kbdConf.ps2usb_mode)
    {
        sbi(USB_LEVEL_SHIFT_PORT, USB_LEVEL_SHIFT_PIN);     // pullup
        cbi(USB_LEVEL_SHIFT_DDR, USB_LEVEL_SHIFT_PIN);      // INPUT

        sbi(PS2_CLK_PULLUP_DDR, PS2_CLK_PULLUP_PIN);        // OUT
        cbi(PS2_CLK_PULLUP_PORT, PS2_CLK_PULLUP_PIN);       // drive 0
    }
    else
    {
        sbi(USB_LEVEL_SHIFT_DDR, USB_LEVEL_SHIFT_PIN);      // OUTPUT
        cbi(USB_LEVEL_SHIFT_PORT, USB_LEVEL_SHIFT_PIN);     // drive 0
        
        sbi(PS2_CLK_PULLUP_PORT, PS2_CLK_PULLUP_PIN);       // pullup
        cbi(PS2_CLK_PULLUP_DDR, PS2_CLK_PULLUP_PIN);        // INPUT
    }
    return kbdConf.ps2usb_mode;
}


void updateConf(void)
{
    eeprom_update_block(&kbdConf, EEPADDR_KBD_CONF, sizeof(kbdConf));
}


#if 1
///////////////////SHOULD BE REMOVED ////////////////
void kbdActivation(void)
{
    if(eeprom_read_byte(KBD_ACTIVATION) != KBD_ACTIVATION_BIT)
    {
        updateConf();       // should be removed
#if 0
        for(i = 0; i < 120; i++)
        {
        eeprom_write_byte(EEPADDR_KEYMAP_LAYER0+i, pgm_read_byte(0x6300+i));
        eeprom_write_byte(EEPADDR_KEYMAP_LAYER1+i, pgm_read_byte(0x6400+i));
        eeprom_write_byte(EEPADDR_KEYMAP_LAYER2+i, pgm_read_byte(0x6500+i));
        eeprom_write_byte(EEPADDR_KEYMAP_LAYER3+i, pgm_read_byte(0x6600+i));
        }
#endif        
        eeprom_update_byte(KBD_ACTIVATION, KBD_ACTIVATION_BIT);
    }
}
#endif

void kbd_init(void)
{
    kbdActivation();
        
    portInit();
    initI2C();

    timer0Init();
    timer0SetPrescaler(TIMER_CLK_DIV8);

    eeprom_read_block(&kbdConf, EEPADDR_KBD_CONF, sizeof(kbdConf));
    if(kbdConf.swapAltGui & 0x01)
        kbdConf.swapAltGui = 1;    
    if(kbdConf.swapCtrlCaps & 0x01)
        kbdConf.swapCtrlCaps = 1; 
    if(kbdConf.keymapLayerIndex >= MAX_LAYER)
        kbdConf.keymapLayerIndex = 1;

    setPS2USB();
    keymap_init();
    scankeycntms = (uint32_t)SCAN_COUNT_IN_MIN * (uint32_t)kbdConf.sleepTimerMin;

    tinyExist = establishSlaveComm();
    if (tinyExist != 1)
    {
        DDRD |= 0xE0;           // DDRD [7:5] -> NCR
    }else
    {
        led_restore();
    }
    updateConf();
}

int main(void)
{
   kbd_init();   
    
   if(kbdConf.ps2usb_mode)
   {
      led_mode_init();
      usbmain();
   }
   else
   {
    
      led_mode_init();
      ps2main();
   }
   return 0;
}

