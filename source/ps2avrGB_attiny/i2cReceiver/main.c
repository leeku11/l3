/*
*/

#include <avr/io.h>     // include I/O definitions (port names, pin names, etc)
#include <stdbool.h>
#include <util/delay.h>

#include "i2c-slave.h"
#include "../Light_WS2812/light_ws2812.h"

#define SUPPORT_TINY_CMD
#ifdef SUPPORT_TINY_CMD
#include "tinycmd.h"
#include "tinycmdpkt.h"
#include "tinycmdhandler.h"
#endif // SUPPORT_TINY_CMD

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

uint8_t cmdBuffer[I2C_RECEIVE_DATA_BUFFER_SIZE];

void i2cSlaveReceiveService(uint8_t receiveDataLength, uint8_t* receiveData);


#ifdef SUPPORT_TINY_CMD
//tinycmd_pkt_req_type tinycmd_req;

uint8_t handle_ver(tinycmd_pkt_req_type *p_req);
uint8_t handle_reset(tinycmd_pkt_req_type *p_req);
uint8_t handle_three_lock(tinycmd_pkt_req_type *p_req);
uint8_t handle_led(tinycmd_pkt_req_type *p_req);
uint8_t handle_test(tinycmd_pkt_req_type *p_req);

const tinycmd_handler_func handle_cmd_func[] = {
    handle_ver,
    handle_reset,
    handle_three_lock,
    handle_led,
    handle_test,
    0
};
#define HANDLER_SIZE            (sizeof(handle_cmd_func)/sizeof(tinycmd_handler_func))

uint8_t handle_ver(tinycmd_pkt_req_type *p_req)
{
    uint8_t *pTmp;
    uint8_t i;

    // Off
    pTmp = (uint8_t *)localBuffer;
    for(i = 0; i < 15; i++)
    {
        *(pTmp++) = 0;
        *(pTmp++) = 0;
        *(pTmp++) = 0;
    }

    pTmp = (uint8_t *)localBuffer;
    // Three lock
    *(pTmp++) = 0;
    *(pTmp++) = 0;
    *(pTmp++) = 0;

    for(i = 0; i < 2; i++)
    {
        *(pTmp++) = 70;
        *(pTmp++) = 70;
        *(pTmp++) = 70;
    }
    localBufferLength = 15*3;

    ws2812_sendarray(localBuffer,localBufferLength);        // output message data to port D

    return 0;
}

uint8_t handle_reset(tinycmd_pkt_req_type *p_req)
{
    uint8_t *pTmp;
    uint8_t i;

    // Off
    pTmp = (uint8_t *)localBuffer;
    for(i = 0; i < 15; i++)
    {
        *(pTmp++) = 0;
        *(pTmp++) = 0;
        *(pTmp++) = 0;
    }

    pTmp = (uint8_t *)localBuffer;
    // Three lock
    *(pTmp++) = 0;
    *(pTmp++) = 0;
    *(pTmp++) = 0;

    for(i = 0; i < 4; i++)
    {
        *(pTmp++) = 0;
        *(pTmp++) = 200;
        *(pTmp++) = 0;
    }
    localBufferLength = 15*3;

    ws2812_sendarray(localBuffer,localBufferLength);        // output message data to port D

    return 0;
}

uint8_t handle_three_lock(tinycmd_pkt_req_type *p_req)
{
    tinycmd_three_lock_req_type *p_three_lock = (tinycmd_three_lock_req_type *)p_req;
    uint8_t *pTmp;
    uint8_t i;

    // Off
    pTmp = (uint8_t *)localBuffer;
    for(i = 0; i < 15; i++)
    {
        *(pTmp++) = 0;
        *(pTmp++) = 0;
        *(pTmp++) = 0;
    }

    pTmp = (uint8_t *)localBuffer;
    // Three lock
    *(pTmp++) = 0;
    *(pTmp++) = 0;
    *(pTmp++) = 0;

    for(i = 0; i < 4; i++)
    {
        *(pTmp++) = 0;
        *(pTmp++) = 0;
        *(pTmp++) = 0;
    }
    
    if(p_three_lock->lock & 0x4)
    {
        *(pTmp++) = 200;
        *(pTmp++) = 0;
        *(pTmp++) = 0;
    }
    
    if(p_three_lock->lock & 0x2)
    {
        *(pTmp++) = 0;
        *(pTmp++) = 200;
        *(pTmp++) = 0;
    }
    
    if(p_three_lock->lock & 0x1)
    {
        *(pTmp++) = 0;
        *(pTmp++) = 0;
        *(pTmp++) = 200;
    }
    
    localBufferLength = 15*3;

    ws2812_sendarray(localBuffer,localBufferLength);        // output message data to port D

    return 0;
}

uint8_t handle_led(tinycmd_pkt_req_type *p_req)
{
    tinycmd_led_req_type *p_led = (tinycmd_led_req_type *)p_req;
    uint8_t *pTmp, *pLed;
    uint8_t i;

    // Off
    pTmp = (uint8_t *)localBuffer;
    for(i = 0; i < 15; i++)
    {
        *(pTmp++) = 0;
        *(pTmp++) = 0;
        *(pTmp++) = 0;
    }

    pTmp = (uint8_t *)localBuffer;

    // Three lock
    *(pTmp++) = 0;
    *(pTmp++) = 0;
    *(pTmp++) = 0;
    
    for(i = 0; i < p_led->offset; i++)
    {
        *(pTmp++) = 0;
        *(pTmp++) = 0;
        *(pTmp++) = 0;
    }
    
    pLed = (uint8_t *)p_req->led.led;
    for(i = 0; i < p_led->num; i++)
    {
        *(pTmp++) = *(pLed++);
        *(pTmp++) = *(pLed++);
        *(pTmp++) = *(pLed++);
    }
    localBufferLength = 15*3;

    ws2812_sendarray(localBuffer,localBufferLength);        // output message data to port D
    
    return 0;
}

uint8_t handle_test(tinycmd_pkt_req_type *p_req)
{
    tinycmd_test_req_type *p_test = (tinycmd_test_req_type *)p_req;
    uint8_t *pTmp;
    uint8_t i;

    pTmp = (uint8_t *)localBuffer;
    for(i = 0; i < 15; i++)
    {
        *(pTmp++) = 0;
        *(pTmp++) = 0;
        *(pTmp++) = 0;
    }

    pTmp = (uint8_t *)localBuffer;
    
    // Three lock
    *(pTmp++) = 0;
    *(pTmp++) = 0;
    *(pTmp++) = 0;
    
    for(i = 0; i < p_test->data_len; i++)
    {
        *(pTmp++) = p_test->data[i];
    }

    localBufferLength = 15*3;

    ws2812_sendarray(localBuffer,localBufferLength);        // output message data to port D

    return 0;
}
#endif // SUPPORT_TINY_CMD

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
 
int main(void)
{
    uint8_t i;
    uint8_t *pTmp;
    uint8_t rcvlen;
    uint8_t rgb;
    CLKPR=_BV(CLKPCE);
    CLKPR=0;			// set clock prescaler to 1 (attiny 25/45/85/24/44/84/13/13A)

    DDRB |= (1<<PB4) | (1<<PB1);
    PORTB |= (1<<PB4) | (1<<PB1);

    count = 0;

    // port output
    DDRB |= (1<<ws2812_pin)|(1<<ws2812_pin2)|(1<<ws2812_pin3);

    i2c_initialize( LOCAL_ADDR );

    initWs2812(ws2812_pin);
    sei();

    for(;;)
    {
        count++;
        rcvlen = i2c_message_ready();

        if(rcvlen != 0)
        {
            tinycmd_pkt_req_type *pReq;
            pTmp = i2c_wrbuf;
            // copy the received data to a local buffer
            for(i=0; i<rcvlen; i++)
            {
                cmdBuffer[i] = *pTmp++;
            }
            //localBufferLength = rcvlen;

            i2c_message_done();

            pReq = (tinycmd_pkt_req_type *)cmdBuffer;

            // handle command
            if(pReq->cmd_code < HANDLER_SIZE)
            {
                if(handle_cmd_func[pReq->cmd_code] != 0)
                {
                    handle_cmd_func[pReq->cmd_code](pReq);
                }
            }
        }
#if 0
        else
        {
            if(count%50000 == 0)
            {
                //if(rgb != 1)
                {
                    //rgb = 1;
                    pTmp = localBuffer;
                    for(i = 0; i < 20; i++)
                    {
                        *(pTmp++) = 50;
                        *(pTmp++) = 50;
                        *(pTmp++) = 50;
                    }
                    
                    ws2812_sendarray(localBuffer, 15*3);
                }
            }
        }
        //PORTB |= (1<<PB4);
        //blink();
#endif
    }

    return 0;

}





