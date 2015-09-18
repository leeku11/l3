#ifndef _HWPORT_H_
#define _HWPORT_H_

#include <avr/io.h>
#include <avr/wdt.h>

#define USB_LEVEL_SHIFT_PORT    PORTD
#define USB_LEVEL_SHIFT_DDR     DDRD
#define USB_LEVEL_SHIFT_PIN     0

#define PS2_CLK_PULLUP_PORT     PORTD
#define PS2_CLK_PULLUP_DDR      DDRD
#define PS2_CLK_PULLUP_PIN      1


#define LED_NUM_PORT        (uint8_t *const)&PORTD
#define LED_NUM_PIN         5

#define LED_CAP_PORT        (uint8_t *const)&PORTD
#define LED_CAP_PIN         6

#define LED_SCR_PORT        (uint8_t *const)&PORTD
#define LED_SCR_PIN         7


#define BOOTLOADER_ADDRESS 0x7000

#define CHECK_U (~PINA & 0x80)  // row2-col7 => U
#define CHECK_P (~PINB & 0x04)  // row2-col10 => P

#define Reset_AVR() wdt_enable(WDTO_250MS); while(1) {}
typedef void (*AppPtr_t) (void); 
//#define Reset_AVR Bootloader 

#endif
