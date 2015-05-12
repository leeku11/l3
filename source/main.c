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
#include <string.h>

#include "hwport.h"
#include "usbdrv.h"
#include "hwaddress.h"
#include "matrix.h"
#include "macro.h"


#ifdef SUPPORT_I2C
#include "i2c.h"        // include i2c support
#include "tinycmdapi.h"

#define TINY_DETECT_RETRY        10

// local data buffer
uint8_t tinyExist = 1;
unsigned char localBuffer[0x4B];
unsigned char localBufferLength;
#endif // SUPPORT_I2C

extern uint8_t usbmain(void);
extern uint8_t ps2main(void);

kbd_configuration_t kbdConf;


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
    PORTC   = 0xFF; // LED5,6 off, COL[0-5] pull-up

    DDRA	= 0x00; // col
    DDRB    = 0x00; // LED_VESEL, LED_PIN_PRT, LED_PIN_PAD, LED_PIN_WASD OUT        (11110000)
    DDRC	= 0x00; // row 2, 3, 4, 5, 6, 7, 8, 9

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
#endif // SUPPORT_I2C

int8_t setPS2USB(void)
{
    uint8_t cur_usbmode = kbdConf.ps2usb_mode;
    DDRC  |= BV(4);        //  col2
    PORTC &= ~BV(4);       //

    _delay_us(10);

    if (CHECK_U)   
    {
        cur_usbmode = 1;
    }else if (CHECK_P)
    {
        cur_usbmode = 0;
    }else
    {
        if (cur_usbmode > 1)
            cur_usbmode = 1;    //default USB
    }

    /* control zener diode for level shift signal line
        1 : TR on - 3v level
        0 : TR off - 5v level
        */
    
    if (cur_usbmode)
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
    
    kbdConf.ps2usb_mode = cur_usbmode;    
    return cur_usbmode;
}        

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
        ret = tinycmd_ver(TRUE);
        if(ret)
        { 
            tinycmd_config(kbdConf.rgb_chain + 1, kbdConf.rgb_limit, TRUE);
            break;
        }
    }
#endif // SUPPORT_TINY_CMD

    return ret;    
}  

uint8_t tiny_init(void)
{
    uint8_t ret = 0;
    if(tinyExist)
    {
        // Init RGB Effect
        tinycmd_rgb_buffer(MAX_RGB_CHAIN, 0, (uint8_t *)kbdConf.rgb_preset, TRUE);
        tinycmd_rgb_set_preset(kbdConf.rgb_effect_index, &kbdConf.rgb_effect_param[kbdConf.rgb_effect_index], TRUE); // RGB_EFFECT_BOOTHID
        // now kbdConf.rgb_effect_index should be 0.
        tinycmd_rgb_set_effect(kbdConf.rgb_effect_index, TRUE);
        tinycmd_rgb_effect_speed(kbdConf.rgb_speed, TRUE);      // fast

        // Init LED Effect
        tinycmd_led_set_effect(kbdConf.led_preset_index, TRUE);
        tinycmd_led_config_preset((uint8_t*)kbdConf.led_preset, TRUE);


        ret = 1;
    }
    return ret;
}

void updateConf(void)
{
    eeprom_update_block(&kbdConf, EEPADDR_KBD_CONF, sizeof(kbdConf));
}


#if 1

///////////////////SHOULD BE REMOVED ////////////////

rgb_effect_param_type kbdRgbEffectParam[RGB_EFFECT_MAX] = 
{
    { RGB_EFFECT_BOOTHID, 0, 0, 0 },    // RGB_EFFECT_BOOTHID
    { RGB_EFFECT_FADE_BUF, FADE_HIGH_HOLD, FADE_LOW_HOLD, FADE_IN_ACCEL },    // RGB_EFFECT_FADE_BUF
    { RGB_EFFECT_FADE_LOOP, FADE_HIGH_HOLD, FADE_LOW_HOLD, FADE_IN_ACCEL },    // RGB_EFFECT_FADE_LOOP
    { RGB_EFFECT_HEARTBEAT_BUF, HEARTBEAT_HIGH_HOLD, HEARTBEAT_LOW_HOLD, HEARTBEAT_IN_ACCEL },    // RGB_EFFECT_HEARTBEAT_BUF
    { RGB_EFFECT_HEARTBEAT_LOOP, HEARTBEAT_HIGH_HOLD, HEARTBEAT_LOW_HOLD, HEARTBEAT_IN_ACCEL },    // RGB_EFFECT_HEARTBEAT_LOOP
    { RGB_EFFECT_BASIC, HEARTBEAT_HIGH_HOLD, HEARTBEAT_LOW_HOLD, HEARTBEAT_IN_ACCEL },    // RGB_EFFECT_HEARTBEAT_LOOP
};

static uint8_t tmpled_preset[3][5] = {{LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_FADING, LED_EFFECT_FADING_PUSH_ON},
                    {LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_PUSHED_LEVEL, LED_EFFECT_PUSH_ON},
                    {LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_PUSH_OFF, LED_EFFECT_BASECAPS}};

#ifdef L3_ALPhas
static uint8_t tmprgp_preset[MAX_RGB_CHAIN][3] =         
        {{0, 250, 250}, {0, 250, 250},
         {0, 250, 100}, {0, 250, 250}, {0, 50, 250},  {0, 0, 250}, {250, 0, 0}, {250, 250, 0}, {100, 250,0},  {0, 250, 0}, {0, 250, 0},
         {0, 250, 0},   {100, 250,0},  {250, 250, 0}, {250, 0, 0}, {0, 0, 250}, {0, 50, 250},  {0, 250, 250}, {0, 250, 100}, {0, 250, 100}
        };


#define RGB_CHAIN_NUM   20
#define DEFAULT_LAYER   2

#else
static uint8_t tmprgp_preset[MAX_RGB_CHAIN][3] =         
        {{0, 250, 0},  {100, 250,0},  {250, 250, 0}, {250, 0, 0},   {0, 0, 250},   {0, 50, 250}, {0, 250, 250},
        {0, 250, 250}, {0, 50, 250},  {0, 0, 250},   {250, 0, 0},   {250, 250, 0}, {100, 250,0}, {0, 250, 0},
        {0, 250, 100}, {0, 250, 100}, {0, 250, 100}, {0, 250, 100}, {0, 250, 100}, {0, 250, 100}};

#define RGB_CHAIN_NUM   14         
#define DEFAULT_LAYER   0
#endif    


void kbdActivation(void)
{
    uint8_t i;
    


    // TODO : LOAD to E2P

    
    if(eeprom_read_byte(KBD_ACTIVATION) != KBD_ACTIVATION_BIT)
    {
        kbdConf.ps2usb_mode = 1;
        kbdConf.keymapLayerIndex = DEFAULT_LAYER;
        kbdConf.swapCtrlCaps = 0;
        kbdConf.swapAltGui = 0;
        kbdConf.led_preset_index = 1;
        memcpy(kbdConf.led_preset, tmpled_preset, sizeof(kbdConf.led_preset));
        kbdConf.rgb_effect_index = 3;
        

        kbdConf.rgb_chain = RGB_CHAIN_NUM;
        kbdConf.rgb_limit = 500;
        kbdConf.rgb_speed = 500;
        kbdConf.matrix_debounce = 4;
        memcpy(kbdConf.rgb_preset, tmprgp_preset, sizeof(kbdConf.rgb_preset));
        memcpy(kbdConf.rgb_effect_param, kbdRgbEffectParam, sizeof(kbdRgbEffectParam));
        
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
    
    ///////////////////SHOULD BE REMOVED ////////////////
}

#endif

void kbd_init(void)
{

    kbdActivation();
        
    portInit();
    initI2C();
    enable_printf();
    eeprom_read_block(&kbdConf, EEPADDR_KBD_CONF, sizeof(kbdConf));

    if(kbdConf.swapAltGui != 1)
        kbdConf.swapAltGui = 0;    
    if(kbdConf.swapCtrlCaps != 1)
        kbdConf.swapCtrlCaps = 0; 
    if(kbdConf.keymapLayerIndex >= MAX_LAYER)
        kbdConf.keymapLayerIndex = 1;
    
    setPS2USB();

    //   led_off(LED_PIN_Fx);
    //   timerInit();
    //   timer1PWMInit(8);
    //   timer2PWMInit(8);

    timer0Init();
    timer0SetPrescaler(TIMER_CLK_DIV8);

    keymap_init();
    tinyExist = establishSlaveComm();
    if (tinyExist != 1)
    {
        DDRD |= 0xE0;           // DDRD [7:5] -> NCR
    }
    tiny_init();
    
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

