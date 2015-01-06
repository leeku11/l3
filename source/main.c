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
	PORTA	= 0xFF;	//  row
	PORTB   = 0xFF; //  row
    PORTC   = 0xFC; // LED5,6 off, COL[0-5] pull-up
	
	DDRA	= 0x00;	// col
	DDRB 	= 0x00;	// LED_VESEL, LED_PIN_PRT, LED_PIN_PAD, LED_PIN_WASD OUT        (11110000)
    DDRC	= 0x03; // row 2, 3, 4, 5, 6, 7, 8, 9
	
    PORTD   = 0xF1; // DPpull-up(Low), Zener(pull-up), LED_SCR, LED_CAPS, LED_NUM (0ff), D-(0), D+(0)
    DDRD    = 0x03; // DPpull-up(OUT), Zener(OUT), LED_SCR, LED_CAPS, LED_NUM (OUT), D-(INPUT), D+(INPUT)

    return 0;
}

#define CHECK_U (~PINA & 0x80)  // col2-row7 => U
#define CHECK_P (~PINB & 0x04)  // col2-row10 => P



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

