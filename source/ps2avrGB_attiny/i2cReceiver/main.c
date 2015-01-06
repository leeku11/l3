/*
*/

#include <avr/io.h>     // include I/O definitions (port names, pin names, etc)
#include <stdbool.h>
#include <util/delay.h>

#include "i2c-slave.h"
#include "../Light_WS2812/light_ws2812.h"

#define LOCAL_ADDR  0xB0
//#define TARGET_ADDR 0xA0
#define I2C_SEND_DATA_BUFFER_SIZE		I2C_RDSIZE	//0x5A
#define I2C_RECEIVE_DATA_BUFFER_SIZE	I2C_WRSIZE	//0x5A	//30 led

#define ws2812_pin2  1
#define ws2812_pin3  4	// PB4 -> OC1B
//#define ws2812_pin4  5 //reset... do not use!!

// local data buffer
uint8_t localBuffer[I2C_RECEIVE_DATA_BUFFER_SIZE];
uint8_t localBufferLength = I2C_RECEIVE_DATA_BUFFER_SIZE;
void i2cSlaveReceiveService(uint8_t receiveDataLength, uint8_t* receiveData);

// slave operations
void i2cSlaveReceiveService(uint8_t receiveDataLength, uint8_t* receiveData)
{
	uint8_t i;

	/*
	 * data[15] = rgb 2,3
	 * data[0] : index;
	 * data[1~5] : reserved
	 * data[6~14] : grb data * 3
	 *
	 * data[1] = on/off
	 * data[0] : 0bit = ws2812_pin2 on/off, 1bit = ws2812_pin3 on/off
	 *
	 */
	/*if(receiveDataLength == 1){
		PORTB &= ~((1<<ws2812_pin2)|(1<<ws2812_pin3));	// masking
		PORTB |= ((*receiveData & 0x01)<<ws2812_pin2)|(((*receiveData>>1) & 0x01)<<ws2812_pin3);

		return;
	}else
	if(receiveDataLength == 15){
		// led 2, 3
		if(*receiveData == 1){
			initWs2812(ws2812_pin2);
		}else if(*receiveData == 2){
			initWs2812(ws2812_pin3);
		}
		receiveData += 6;
		receiveDataLength -= 6;
	}else{
		// led1
	    initWs2812(ws2812_pin);

	}*/

    // copy the received data to a local buffer
    for(i=0; i<receiveDataLength; i++)
    {
        localBuffer[i] = *receiveData++;
    }
    localBufferLength = receiveDataLength;

    ws2812_sendarray(localBuffer,localBufferLength);
}


#define END_MARKER 255 // Signals the end of transmission
int count;
void blink(void){
    // blink;
    if(--count == 0){
        PORTB ^= (1<<PB4);
        PORTB ^= (1<<PB1);
    }
}
 
int main(void)    // main 함수를 호출합니다.
{
    uint8_t i;
    uint8_t *pTmp;
    CLKPR=_BV(CLKPCE);
    CLKPR=0;			// set clock prescaler to 1 (attiny 25/45/85/24/44/84/13/13A)

    DDRB |= (1<<PB4) | (1<<PB1);
    PORTB |= (1<<PB4) | (1<<PB1);

//    count = 0;

    // port output
	DDRB |= (1<<ws2812_pin)|(1<<ws2812_pin2)|(1<<ws2812_pin3);

	i2c_initialize( LOCAL_ADDR );

	initWs2812(ws2812_pin);
    sei();

    pTmp = i2c_wrbuf;
    for(i = 0; i < 20; i++)
    {
        *(pTmp++) = 0;
        *(pTmp++) = 0;
        *(pTmp++) = 250;
    }
    i2c_wrlen = 15*3;
    
	//for(;;)
	{
#if 1
        if(1){ //i2c_message_ready()) {

			i2cSlaveReceiveService(i2c_wrlen,  (uint8_t*)i2c_wrbuf);		// output message data to port D

			i2c_message_done();

		}
#endif
//		PORTB |= (1<<PB4);
		blink();
    }

	return 0;

}





