#ifndef PRINT
#define PRINT

//#define DEBUG

#ifdef DEBUG

#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>

#define BAUD 57600UL
#define UBRR (((F_CPU / (BAUD * 16UL))) - 1) 

#define DEBUG_PRINT(arg) printf arg
//#define DEBUG_PRINT printf

static int
uart_putchar(char c, FILE *stream)
{
    if (c == '\n')
        uart_putchar('\r', stream);
    loop_until_bit_is_set(UCSRA, UDRE);
    UDR = c;
    return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
#else
#define DEBUG_PRINT(arg)
#endif

#endif
