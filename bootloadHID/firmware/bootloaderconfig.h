/* Name: bootloaderconfig.h
 * Project: AVR bootloader HID
 * Author: Christian Starkjohann
 * Creation Date: 2007-03-19
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt)
 * This Revision: $Id$
 */

#ifndef __bootloaderconfig_h_included__
#define __bootloaderconfig_h_included__



/*
General Description:
This file (together with some settings in Makefile) configures the boot loader
according to the hardware.

This file contains (besides the hardware configuration normally found in
usbconfig.h) two functions or macros: bootLoaderInit() and
bootLoaderCondition(). Whether you implement them as macros or as static
inline functions is up to you, decide based on code size and convenience.

bootLoaderInit() is called as one of the first actions after reset. It should
be a minimum initialization of the hardware so that the boot loader condition
can be read. This will usually consist of activating a pull-up resistor for an
external jumper which selects boot loader mode. You may call leaveBootloader()
from this function if you know that the main code should run.

bootLoaderCondition() is called immediately after initialization and in each
main loop iteration. If it returns TRUE, the boot loader will be active. If it
returns FALSE, the boot loader jumps to address 0 (the loaded application)
immediately.

For compatibility with Thomas Fischl's avrusbboot, we also support the macro
names BOOTLOADER_INIT and BOOTLOADER_CONDITION for this functionality. If
these macros are defined, the boot loader usees them.
*/

/* ---------------------------- Hardware Config ---------------------------- */

#define USB_CFG_IOPORTNAME      D
/* This is the port where the USB bus is connected. When you configure it to
 * "B", the registers PORTB, PINB and DDRB will be used.
 */
 
/* This is the bit number in USB_CFG_IOPORT where the USB D- line is connected.
 * This may be any bit in the port.
*/
/* This is the bit number in USB_CFG_IOPORT where the USB D+ line is connected.
* This may be any bit in the port. Please note that D+ must also be connected
* to interrupt pin INT0! [You can also use other interrupts, see section
* "Optional MCU Description" below, or you can connect D- to the interrupt, as
* it is required if you use the USB_COUNT_SOF feature. If you use D- for the
* interrupt, the USB interrupt will also be triggered at Start-Of-Frame
* markers every millisecond.]
*/
#ifdef KBDMOD_M3
#define USB_CFG_DMINUS_BIT      3           // for KBDMOD keyboards
#define USB_CFG_DPLUS_BIT       2
#else
#define USB_CFG_DMINUS_BIT      1           // for KBDMOD keyboards
#define USB_CFG_DPLUS_BIT       0
#endif

#define USB_CFG_CLOCK_KHZ       (F_CPU/1000)
/* Clock rate of the AVR in MHz. Legal values are 12000, 12800, 15000, 16000,
 * 16500 and 20000. The 12.8 MHz and 16.5 MHz versions of the code require no
 * crystal, they tolerate +/- 1% deviation from the nominal frequency. All
 * other rates require a precision of 2000 ppm and thus a crystal!
 * Default if not specified: 12 MHz
 */

/* ----------------------- Optional Hardware Config ------------------------ */

/* #define USB_CFG_PULLUP_IOPORTNAME   D */
/* If you connect the 1.5k pullup resistor from D- to a port pin instead of
 * V+, you can connect and disconnect the device from firmware by calling
 * the macros usbDeviceConnect() and usbDeviceDisconnect() (see usbdrv.h).
 * This constant defines the port on which the pullup resistor is connected.
 */
/* #define USB_CFG_PULLUP_BIT          4 */
/* This constant defines the bit number in USB_CFG_PULLUP_IOPORT (defined
 * above) where the 1.5k pullup resistor is connected. See description
 * above for details.
 */

/* --------------------------- Functional Range ---------------------------- */

#define BOOTLOADER_CAN_EXIT     1
/* If this macro is defined to 1, the boot loader command line utility can
 * initiate a reboot after uploading the FLASH when the "-r" command line
 * option is given. If you define it to 0 or leave it undefined, the "-r"
 * option won't work and you save a couple of bytes in the boot loader. This
 * may be of advantage if you compile with gcc 4 instead of gcc 3 because it
 * generates slightly larger code.
 * If you need to save even more memory, consider using your own vector table.
 * Since only the reset vector and INT0 (the first two vectors) are used,
 * this saves quite a bit of flash. See Alexander Neumann's boot loader for
 * an example: http://git.lochraster.org:2080/?p=fd0/usbload;a=tree
 */

/* ------------------------------------------------------------------------- */

/* Example configuration: Port D bit 3 is connected to a jumper which ties
 * this pin to GND if the boot loader is requested. Initialization allows
 * several clock cycles for the input voltage to stabilize before
 * bootLoaderCondition() samples the value.
 * We use a function for bootLoaderInit() for convenience and a macro for
 * bootLoaderCondition() for efficiency.
 */

#ifndef __ASSEMBLER__   /* assembler cannot parse function definitions */
#include <util/delay.h>


#define DDR_ROW0    DDRA
#define PORT_ROW0   PORTA
#define PIN_ROW0    PINA
#define DDR_COL0    DDRC
#define PORT_COL0   PORTC

#define DDR_LED0    DDRD
#define DDR_LED1    DDRD
#define DDR_LED2    DDRD
#define DDR_LED3    DDRD

#define PORT_LED0   PORTD
#define PORT_LED1   PORTD
#define PORT_LED2   PORTD
#define PORT_LED3   PORTD

#define PIN_LED0    1 << 4
#define PIN_LED1    1 << 5
#define PIN_LED2    1 << 7
#define PIN_LED3    1 << 5



uint8_t ledcounter = 0; ///< counter used to set the speed of the running light
uint8_t ledstate = 0;   ///< state of the running light

static uint8_t isBootloader = 0;
static int counter=0;
static int prevState;

static void ledOff()
{
   PORT_LED0 &= ~(PIN_LED0);
   PORT_LED1 &= ~(PIN_LED1);
   PORT_LED2 &= ~(PIN_LED2);
   PORT_LED3 &= ~(PIN_LED3);            
}

static void ledInit()
{
   DDR_LED0 |= (PIN_LED0);
   DDR_LED1 |= (PIN_LED1);
   DDR_LED2 |= (PIN_LED2);
   DDR_LED3 |= (PIN_LED3); 

}

static void ledOn(uint8_t pin)
{
   switch (pin)
   {
      case 0:
         PORT_LED0 |= PIN_LED0;
         break;
      
      case 1:
         PORT_LED1 |= PIN_LED1;
         break;
      
      case 2:
         PORT_LED2 |= PIN_LED2;
         break;
      
      case 3:
         PORT_LED3 |= PIN_LED3;
         break;

      default:
         break;   
   }

}

static inline void  bootLoaderInit(void)
{

	// 5v -> 3.3v for USB	
	 DDRD |= (1 << PIND0);	
    PORTD |= (1 << PIND0);	
    // PS2 pullup	
    DDRD |= (1 << PIND1);	
    PORTD &= ~(1 << PIND1);
  
    // switch on leds
    ledInit();
    ledOff();

    // choose matrix position for hotkey. we use KEY_KPminus, so we set row 13
    // and later look for pin 7

    DDR_ROW0  &= ~(1 << PINA0);     // PINA0(row0) input
    PORT_ROW0 |= (1 << PINA0);      // PINA0(pullUP)
    DDR_COL0  |= (1 << PINC3);      // PINB0(column0) output
    PORT_COL0 &= ~(1 << PINC3);     // drive low
}

//#define bootLoaderCondition()   ((PIND & (1 << 3)) == 0)   /* True if jumper is set */
static inline uint8_t bootLoaderCondition() {
	prevState = PIN_ROW0 & (1 << PINA0);
	while(counter<=10) {                            // PINA0 should be 0 for 10 cycles
		if(prevState != (PIN_ROW0 & (1 << PINA0)))
			counter=0;
		else
			counter++;

		prevState = (PIN_ROW0 & (1 << PINA0));
	}
	if(!prevState) isBootloader = 1;

    if (isBootloader) {
        // boot loader active, blink leds
        _delay_ms(1);
        ledcounter++;
        if (ledcounter == 127) {
            switch (ledstate) {
                case 0:
                  ledOff();
                  ledOn(0);
                  ledstate = 1;
                    break;
                case 1:
                   ledOff();
                   ledOn(1);
                    ledstate = 2;
                    break;
                case 2:
                   ledOff();
                   ledOn(2);
                    ledstate = 3;
                    break;


                case 3:
                   ledOff();
                   ledOn(3);
                    ledstate = 4;
                    break;

                case 4:
                   ledOff();
                   ledOn(2);
                    ledstate = 5;
                    break;

                case 5:
                   ledOff();
                   ledOn(1);
                    ledstate = 0;
                    break;

                default:
                   ledOff();

                    ledstate = 0;
            }
            ledcounter = 0;
        }
        return 1;
    } else {
        // no boot loader
        return 0;
    }
}


#endif

/* ------------------------------------------------------------------------- */

#endif /* __bootloader_h_included__ */
