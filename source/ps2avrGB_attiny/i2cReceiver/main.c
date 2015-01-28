/*
*/

#include <avr/io.h>     // include I/O definitions (port names, pin names, etc)
#include <stdbool.h>
#include <util/delay.h>

#include "i2c-slave.h"
#include "../Light_WS2812/light_ws2812.h"
#include "timeratt.h"

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

#define LED_NUM           21
#define LED_ELEMENT       3
#define LED_ARRAY_SIZE    (LED_NUM*LED_ELEMENT)

#define ws2812_pin2  1
#define ws2812_pin3  4	// PB4 -> OC1B
//#define ws2812_pin4  5 //reset... do not use!!

#define TINY_LEDMODE_STORAGE_MAX        3
#define TINY_LED_BLOCK_MAX              5
#define LEDMODE_ARRAY_SIZE              (TINY_LEDMODE_STORAGE_MAX*TINY_LED_BLOCK_MAX)


typedef struct {
    uint8_t led_max;
    uint8_t level_max;
} sys_config_type;

// local data buffer
uint8_t localBuffer[I2C_RECEIVE_DATA_BUFFER_SIZE];
//uint8_t localBufferLength = I2C_RECEIVE_DATA_BUFFER_SIZE;
uint8_t localBufferLength;

uint8_t cmdBuffer[I2C_RECEIVE_DATA_BUFFER_SIZE];

uint8_t threeLock[3] = { 100, 100, 100 }; // initial level

uint8_t tiny_ledmodeIndex;
uint8_t tiny_ledmode[TINY_LEDMODE_STORAGE_MAX][TINY_LED_BLOCK_MAX];

sys_config_type sysConfig;
uint8_t pwmDutyMax;
uint8_t pwmDuty;
uint8_t pwmStep;
uint8_t pwmState;

void i2cSlaveReceiveService(uint8_t receiveDataLength, uint8_t* receiveData);


#ifdef SUPPORT_TINY_CMD
uint8_t handlecmd_ver(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_reset(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_three_lock(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_bl_led_all(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_bl_led_pos(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_bl_led_range(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_bl_led_effect(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_pwm(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_set_led_mode(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_config(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_test(tinycmd_pkt_req_type *p_req);

const tinycmd_handler_func handle_cmd_func[] = {
    handlecmd_ver,
    handlecmd_reset,
    handlecmd_three_lock,
    handlecmd_bl_led_all,
    handlecmd_bl_led_pos,
    handlecmd_bl_led_range,
    handlecmd_bl_led_effect,
    handlecmd_pwm,
    handlecmd_set_led_mode,
    handlecmd_config,
    handlecmd_test,
    0
};
#define CMD_HANDLER_TABLE_SIZE            (sizeof(handle_cmd_func)/sizeof(tinycmd_handler_func))

void three_lock_state(uint8_t num, uint8_t caps, uint8_t scroll)
{
    threeLock[0] = num;
    threeLock[1] = caps;
    threeLock[2] = scroll;
}

void led_array_clear(uint8_t *p_buf)
{
    memset(p_buf, 0, LED_ARRAY_SIZE);
}

void led_array_on(uint8_t on, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t i;
    uint8_t *pTmp = localBuffer;
    
    if(on == 0)
    {
        r = g = b = 0;
    }

    // Three lock
    *(pTmp++) = threeLock[0];
    *(pTmp++) = threeLock[1];
    *(pTmp++) = threeLock[2];
    
    for(i = 1; i < LED_NUM; i++)
    {
        *(pTmp++) = b;
        *(pTmp++) = r;
        *(pTmp++) = g;
    }
    localBufferLength = LED_ARRAY_SIZE;
    ws2812_sendarray(localBuffer, localBufferLength);
}

uint8_t handlecmd_ver(tinycmd_pkt_req_type *p_req)
{
    // clear buffer
    led_array_clear(localBuffer);
    led_array_on(FALSE, 50, 50, 50);

    return 0;
}

uint8_t handlecmd_reset(tinycmd_pkt_req_type *p_req)
{
    // clear buffer
    led_array_clear(localBuffer);
    led_array_on(TRUE, 0, 100, 0);

    return 0;
}

uint8_t handlecmd_three_lock(tinycmd_pkt_req_type *p_req)
{
    tinycmd_three_lock_req_type *p_three_lock = (tinycmd_three_lock_req_type *)p_req;
    uint8_t *pTmp;
    uint8_t i;

    // clear buffer
    led_array_clear(localBuffer);


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
    *(pTmp++) = threeLock[0];
    *(pTmp++) = threeLock[1];
    *(pTmp++) = threeLock[2];

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

uint8_t handlecmd_bl_led_all(tinycmd_pkt_req_type *p_req)
{
    tinycmd_bl_led_all_req_type *p_bl_led_all = (tinycmd_bl_led_all_req_type *)p_req;
    // clear buffer
    led_array_clear(localBuffer);
    led_array_on(p_bl_led_all->on, p_bl_led_all->led.r, p_bl_led_all->led.g, p_bl_led_all->led.b);
    
    return 0;
}

uint8_t handlecmd_bl_led_pos(tinycmd_pkt_req_type *p_req)
{
    tinycmd_bl_led_pos_req_type *p_bl_led_pos = (tinycmd_bl_led_pos_req_type *)p_req;
    uint8_t *pTmp;
    uint8_t i;

    // clear buffer
    led_array_clear(localBuffer);

    pTmp = (uint8_t *)localBuffer;

    // Three lock
    *(pTmp++) = threeLock[0];
    *(pTmp++) = threeLock[1];
    *(pTmp++) = threeLock[2];

    pTmp += 3*p_bl_led_pos->pos;
    
    *(pTmp++) = p_bl_led_pos->led.b;
    *(pTmp++) = p_bl_led_pos->led.r;
    *(pTmp++) = p_bl_led_pos->led.g;
    
    localBufferLength = 15*3;

    ws2812_sendarray(localBuffer,localBufferLength);        // output message data to port D

    return 0;
}

uint8_t handlecmd_bl_led_range(tinycmd_pkt_req_type *p_req)
{
    tinycmd_bl_led_range_req_type *p_bl_led_range = (tinycmd_bl_led_range_req_type *)p_req;
    uint8_t *pTmp, *pLed;
    uint8_t i;

    // clear buffer
    led_array_clear(localBuffer);

    pTmp = (uint8_t *)localBuffer;

    // Three lock
    *(pTmp++) = threeLock[0];
    *(pTmp++) = threeLock[1];
    *(pTmp++) = threeLock[2];

    //if(p_bl_led_range->on)
    {
        pLed = (uint8_t *)&p_bl_led_range->led;
        for(i = 0; i < 14; i++)
        {
            *(pTmp++) = *(pLed++);
            *(pTmp++) = *(pLed++);
            *(pTmp++) = *(pLed++);
        }
    }
    localBufferLength = 15*3;

    ws2812_sendarray(localBuffer,localBufferLength);        // output message data to port D
    
    return 0;
}

uint8_t handlecmd_bl_led_effect(tinycmd_pkt_req_type *p_req)
{
    return 0;
}

// test
void pwm_on(uint8_t duty)
{
    pwmState = 1;
    pwmStep = 8;
    
    pwmDuty = duty;
    timer1PWMASet(pwmDuty);
    timer1PWMBSet(pwmDuty);
    timer1PWMAOn();
    timer1PWMBOn();
}

uint8_t handlecmd_pwm(tinycmd_pkt_req_type *p_req)
{
    tinycmd_pwm_req_type *p_pwm = (tinycmd_pwm_req_type *)p_req;

    if(p_pwm->enable)
    {
        pwmState = 1;
        led_array_clear(localBuffer);
        led_array_on(TRUE, 100, 100, 0);
        led_array_on(FALSE, 0, 0, 0);

        pwmDutyMax = p_pwm->duty;
        pwmDuty = 1;
        pwmStep = 5;
        
        timer1PWMASet(pwmDuty);
        timer1PWMBSet(pwmDuty);
        timer1PWMAOn();
        timer1PWMBOn();
    }
    else
    {
        pwmState = 0;
        led_array_clear(localBuffer);
        led_array_on(TRUE, 100, 0, 100);
        led_array_on(FALSE, 0, 0, 0);
        
        timer1PWMOff();
    }

    return 0;
}

uint8_t handlecmd_set_led_mode(tinycmd_pkt_req_type *p_req)
{
    tinycmd_set_led_mode_req_type *p_ledmode = (tinycmd_pwm_req_type *)p_req;

    tiny_ledmode[p_ledmode->storage][p_ledmode->block] = p_ledmode->mode;
    
    return 0;
}


uint8_t handlecmd_config(tinycmd_pkt_req_type *p_req)
{
    tinycmd_config_req_type *p_config = (tinycmd_config_req_type *)p_req;
    uint8_t *pTmp = (uint8_t *)localBuffer;

    sysConfig.led_max = p_config->value.led_max;
    sysConfig.level_max = p_config->value.level_max;

    return 0;
}

uint8_t handlecmd_test(tinycmd_pkt_req_type *p_req)
{
    tinycmd_test_data_type *p_data = (tinycmd_test_data_type *)&p_req->test.data;

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

#if 0
void blink(int matrixState)
{
    LED_BLOCK ledblock;

    for (ledblock = LED_PIN_BASE; ledblock <= LED_PIN_WASD; ledblock++)
    {
        
        if(matrixState & SCAN_DIRTY)      // 1 or more key is pushed
        {
            switch(ledmode[ledmodeIndex][ledblock])
            {

                case LED_EFFECT_FADING_PUSH_ON:
                case LED_EFFECT_PUSH_ON:
                    led_on(ledblock);
                    break;
                case LED_EFFECT_PUSH_OFF:
                    led_wave_off(ledblock);
                    led_wave_set(ledblock, 0);
                    led_off(ledblock);
                    break;
                default :
                    break;
            }             
        }
        else
        {          // none of keys is pushed
            switch(ledmode[ledmodeIndex][ledblock])
                 {
                     case LED_EFFECT_FADING_PUSH_ON:
                     case LED_EFFECT_PUSH_ON:
                        led_off(ledblock);
                        break;
                     case LED_EFFECT_PUSH_OFF:
                        led_on(ledblock);
                        break;
                     default :
                         break;
                 }
        }
    }
}

void fader(void)
{
    uint8_t ledblock;
    for (ledblock = LED_PIN_BASE; ledblock <= LED_PIN_WASD; ledblock++)
    {
        if((scankeycntms > 1000)
            && (ledmode[ledmodeIndex][ledblock] == LED_EFFECT_FADING)
                || ((ledmode[ledmodeIndex][ledblock] == LED_EFFECT_FADING_PUSH_ON)))
        {
            if(pwmDir[ledblock]==0)
            {
                led_wave_set(ledblock, ((uint16_t)(pwmCounter[ledblock]/brigspeed[ledblock])));        // brighter
                if(pwmCounter[ledblock]>=255*brigspeed[ledblock])
                    pwmDir[ledblock] = 1;
                    
            }
            else if(pwmDir[ledblock]==2)
            {
                led_wave_set(ledblock, ((uint16_t)(255-pwmCounter[ledblock]/speed[ledblock])));    // darker
                if(pwmCounter[ledblock]>=255*speed[ledblock])
                    pwmDir[ledblock] = 3;

            }
            else if(pwmDir[ledblock]==1)
            {
                if(pwmCounter[ledblock]>=255*speed[ledblock])
                   {
                       pwmCounter[ledblock] = 0;
                       pwmDir[ledblock] = 2;
                   }
            }else if(pwmDir[ledblock]==3)
            {
                if(pwmCounter[ledblock]>=255*brigspeed[ledblock])
                   {
                       pwmCounter[ledblock] = 0;
                       pwmDir[ledblock] = 0;
                   }
            }


            led_wave_on(ledblock);

            // pwmDir 0~3 : idle
       
            pwmCounter[ledblock]++;

        }
        else if (ledmode[ledmodeIndex][ledblock] == LED_EFFECT_PUSHED_LEVEL)
        {
    		// 일정시간 유지

    		if(pushedLevelStay[ledblock] > 0){
    			pushedLevelStay[ledblock]--;
    		}else{
    			// 시간이 흐르면 레벨을 감소 시킨다.
    			if(pushedLevelDuty[ledblock] > 0){
    				pwmCounter[ledblock]++;
    				if(pwmCounter[ledblock] >= speed[ledblock]){
    					pwmCounter[ledblock] = 0;			
    					pushedLevelDuty[ledblock]--;
    					pushedLevel[ledblock] = PUSHED_LEVEL_MAX - (255-pushedLevelDuty[ledblock]) / (255/PUSHED_LEVEL_MAX);
    					/*if(pushedLevel_prev != pushedLevel){
    						DEBUG_PRINT(("---------------------------------decrease pushedLevel : %d, life : %d\n", pushedLevel, pushedLevelDuty));
    						pushedLevel_prev = pushedLevel;
    					}*/
    				}
    			}else{
    				pushedLevel[ledblock] = 0;
    				pwmCounter[ledblock] = 0;
    			}
    		}
    		led_wave_set(ledblock, pushedLevelDuty[ledblock]);

    	}
    	else
        {
            led_wave_set(ledblock, 0);
            led_wave_off(ledblock);
            pwmCounter[ledblock]=0;
            pwmDir[ledblock]=0;
        }
    }
}
#endif

int main(void)
{
    uint8_t i;
    uint8_t *pTmp;
    uint8_t rcvlen;
    
    CLKPR=_BV(CLKPCE);  // Clock Prescaler Change Enable
    CLKPR=0;			// set clock prescaler to 1 (attiny 25/45/85/24/44/84/13/13A)

    // Port B Data Direction Register
    DDRB |= (1<<PB4) | (1<<PB3) | (1<<PB1);    // PB1 and PB4 is LED0 and LED1 driver.
    //DDRB |= (1<<PB4) | (1<<PB1);    // PB1 and PB4 is LED0 and LED1 driver.
    PORTB |= (1<<PB4) | (1<<PB1);

    count = 0;

    // port output
    //DDRB |= (1<<ws2812_pin)|(1<<ws2812_pin2)|(1<<ws2812_pin3);

    i2c_initialize( LOCAL_ADDR );

    initWs2812(ws2812_pin);

    timer1Init();
    timer1SetPrescaler(TIMER_CLK_DIV8);
    timer1PWMInit(0);

    led_array_on(TRUE, 0, 150, 0);
    
    pwmState = 0;

    //_lkh test
    pwm_on(0);
    
    sei();

    for(;;)
    {
        count++;
        rcvlen = i2c_message_ready();

        if(rcvlen != 0)
        {
            pTmp = i2c_wrbuf;
            // copy the received data to a local buffer
            for(i=0; i<rcvlen; i++)
            {
                cmdBuffer[i] = *pTmp++;
            }
            i2c_message_done();
            
            tinycmd_pkt_req_type *p_req = (tinycmd_pkt_req_type *)cmdBuffer;
            // handle command
            if(handle_cmd_func[p_req->cmd_code] != 0)
            {
                handle_cmd_func[p_req->cmd_code](p_req);
            }
        }
        else
        {
            if(count%5000 == 0)
            {
                if(pwmState)
                {
                    pwmDuty += pwmStep;
                    timer1PWMASet(pwmDuty);
                    timer1PWMBSet(pwmDuty);
                }
            }
#if 0
            if(count%50000 == 0)
            {
                pTmp = localBuffer;
                for(i = 0; i < 20; i++)
                {
                    *(pTmp++) = 50;
                    *(pTmp++) = 50;
                    *(pTmp++) = 50;
                }
                
                ws2812_sendarray(localBuffer, 15*3);
            }
#endif
        }
        //PORTB |= (1<<PB4);
        //blink();

    }

    return 0;

}





