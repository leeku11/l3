/*
*/

#include <avr/io.h>     // include I/O definitions (port names, pin names, etc)
#include <stdbool.h>
#include <util/delay.h>

#include "i2c-slave.h"
#include "../Light_WS2812/light_ws2812.h"
#include "timeratt.h"
#include "led.h"
#include "keymap.h"
#include "matrix.h"

#define SUPPORT_TINY_CMD
#ifdef SUPPORT_TINY_CMD
#include "tinycmd.h"
#include "tinycmdpkt.h"
#include "tinycmdhandler.h"
#endif // SUPPORT_TINY_CMD

#define SLAVE_ADDR  0xB0
//#define TARGET_ADDR 0xA0
#define I2C_SEND_DATA_BUFFER_SIZE		I2C_RDSIZE	//0x5A
#define I2C_RECEIVE_DATA_BUFFER_SIZE	I2C_WRSIZE	//0x5A	//30 led

#define CLED_NUM           21
#define CLED_ELEMENT       3
#define CLED_ARRAY_SIZE    (CLED_NUM*CLED_ELEMENT)

#define ws2812_pin2  1
#define ws2812_pin3  4	// PB4 -> OC1B
//#define ws2812_pin4  5 //reset... do not use!!

//#define TINY_LEDMODE_STORAGE_MAX        3
//#define TINY_LED_BLOCK_MAX              5
//#define LEDMODE_ARRAY_SIZE              (TINY_LEDMODE_STORAGE_MAX*TINY_LED_BLOCK_MAX)

#define TINYCMD_CMD_MASK                0x7F
#define TINYCMD_RSP_MASK                0x80

#define DEBUG_LED_ON(p, o, r, g, b)     led_pos_on(p, o, r, g, b)

typedef struct {
    uint8_t led_max;
    uint8_t level_max;
    uint8_t comm_init;
} sys_config_type;

// local data buffer
uint8_t localBuffer[I2C_RECEIVE_DATA_BUFFER_SIZE];
uint8_t localBufferLength;

uint8_t cmdBuffer[I2C_RECEIVE_DATA_BUFFER_SIZE];

uint8_t threeLock[3] = { 0, 0, 0 }; // initial level

uint8_t ledmodeIndex = 0;
uint8_t ledmode[LEDMODE_INDEX_MAX][LED_BLOCK_MAX];

static uint8_t speed[LED_BLOCK_MAX] = {0, 0, 0, 5, 5};
static uint8_t brigspeed[LED_BLOCK_MAX] = {0, 0, 0, 3, 3};
static uint8_t pwmDir[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};
static uint16_t pwmCounter[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};

static uint16_t pushedLevelStay[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};
static uint8_t pushedLevel[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};
static uint16_t pushedLevelDuty[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};
uint8_t LEDstate;     ///< current state of the LEDs

sys_config_type sysConfig;

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
uint8_t handlecmd_set_led_mode_all(tinycmd_pkt_req_type *p_req);
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
    handlecmd_config,
    handlecmd_test,
    handlecmd_set_led_mode,
    handlecmd_set_led_mode_all,
    0
};
#define CMD_HANDLER_TABLE_SIZE            (sizeof(handle_cmd_func)/sizeof(tinycmd_handler_func))

void three_lock_state(uint8_t num, uint8_t caps, uint8_t scroll)
{
    threeLock[0] = num;
    threeLock[1] = caps;
    threeLock[2] = scroll;
}

void three_lock_on(void)
{
    uint8_t *pTmp = localBuffer;
    // Three lock
    *(pTmp++) = threeLock[0];
    *(pTmp++) = threeLock[1];
    *(pTmp++) = threeLock[2];
    localBufferLength = 3;
    ws2812_sendarray(localBuffer, localBufferLength);
}

void led_array_clear(uint8_t *p_buf)
{
    memset(p_buf, 0, CLED_ARRAY_SIZE);
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
    
    for(i = 1; i < CLED_NUM; i++)
    {
        *(pTmp++) = g;
        *(pTmp++) = r;
        *(pTmp++) = b;
    }
    localBufferLength = CLED_ARRAY_SIZE;
    ws2812_sendarray(localBuffer, localBufferLength);
}

void led_pos_on(uint8_t pos, uint8_t on, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t i;
    uint8_t *pTmp = localBuffer;

    led_array_clear(pTmp);
    
    if(on == 0)
    {
        r = g = b = 0;
    }

    // Three lock
    *(pTmp++) = threeLock[0];
    *(pTmp++) = threeLock[1];
    *(pTmp++) = threeLock[2];

    pTmp += (pos * CLED_ELEMENT);
    
    *(pTmp++) = g;
    *(pTmp++) = r;
    *(pTmp++) = b;

    localBufferLength = CLED_ARRAY_SIZE;
    ws2812_sendarray(localBuffer, localBufferLength);
}

uint8_t handlecmd_ver(tinycmd_pkt_req_type *p_req)
{
    tinycmd_ver_req_type *p_ver_req = (tinycmd_ver_req_type *)p_req;

    if((p_ver_req->cmd_code & TINY_CMD_RSP_MASK) != 0)
    {
         /*
         * To set response data, wait until i2c_reply_ready() returns nonzero,
         * then fill i2c_rdbuf with the data, finally call i2c_reply_done(n).
         * Interrupts are disabled while updating.
         */
        while(i2c_reply_ready() == 0);
        
        tinycmd_ver_rsp_type *p_ver_rsp = (tinycmd_ver_rsp_type *)i2c_rdbuf;

        p_ver_rsp->cmd_code = TINY_CMD_VER_F;
        p_ver_rsp->pkt_len = sizeof(tinycmd_ver_rsp_type);
        p_ver_rsp->version = 0xA5;
        i2c_reply_done(p_ver_rsp->pkt_len);

        // debug
        led_array_on(TRUE, 100, 0, 0);
        //sysConfig.comm_init = 1;
    }

    return 0;
}

uint8_t handlecmd_reset(tinycmd_pkt_req_type *p_req)
{
    return 0;
}

uint8_t handlecmd_three_lock(tinycmd_pkt_req_type *p_req)
{
    tinycmd_three_lock_req_type *p_three_lock = (tinycmd_three_lock_req_type *)p_req;
    uint8_t *pTmp;

    threeLock[0] = 0;
    threeLock[1] = 0;
    threeLock[2] = 0;
    
    if(p_three_lock->lock & (1<<2))
    {
        threeLock[0] = 150;
    }
    if(p_three_lock->lock & (1<<1))
    {
        threeLock[1] = 150;
    }
    if(p_three_lock->lock & (1<<0))
    {
        threeLock[2] = 150;
    }

    pTmp = (uint8_t *)localBuffer;
    *(pTmp++) = threeLock[0];
    *(pTmp++) = threeLock[1];
    *(pTmp++) = threeLock[2];

    localBufferLength = CLED_ELEMENT;
    ws2812_sendarray(localBuffer,localBufferLength);        // output message data to port D

    return 0;
}

uint8_t handlecmd_bl_led_all(tinycmd_pkt_req_type *p_req)
{
    tinycmd_bl_led_all_req_type *p_bl_led_all = (tinycmd_bl_led_all_req_type *)p_req;

    led_array_on(p_bl_led_all->on, p_bl_led_all->led.r, p_bl_led_all->led.g, p_bl_led_all->led.b);
    
    return 0;
}

uint8_t handlecmd_bl_led_pos(tinycmd_pkt_req_type *p_req)
{
    tinycmd_bl_led_pos_req_type *p_bl_led_pos = (tinycmd_bl_led_pos_req_type *)p_req;
    uint8_t *pTmp;

    // clear buffer
    led_array_clear(localBuffer);

    pTmp = (uint8_t *)localBuffer;

    // Three lock
    *(pTmp++) = threeLock[0];
    *(pTmp++) = threeLock[1];
    *(pTmp++) = threeLock[2];

    pTmp += CLED_ELEMENT*p_bl_led_pos->pos;
    
    *(pTmp++) = p_bl_led_pos->led.b;
    *(pTmp++) = p_bl_led_pos->led.r;
    *(pTmp++) = p_bl_led_pos->led.g;
    
    localBufferLength = CLED_ARRAY_SIZE;

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

    pLed = (uint8_t *)&p_bl_led_range->led;
    for(i = 1; i < CLED_NUM; i++)
    {
        *(pTmp++) = *(pLed++);
        *(pTmp++) = *(pLed++);
        *(pTmp++) = *(pLed++);
    }
    localBufferLength = CLED_ARRAY_SIZE;

    ws2812_sendarray(localBuffer,localBufferLength);        // output message data to port D
    
    return 0;
}

uint8_t handlecmd_bl_led_effect(tinycmd_pkt_req_type *p_req)
{
    return 0;
}

uint8_t handlecmd_pwm(tinycmd_pkt_req_type *p_req)
{
    return 0;
}

uint8_t handlecmd_set_led_mode(tinycmd_pkt_req_type *p_req)
{
    tinycmd_set_led_mode_req_type *p_ledmode = (tinycmd_set_led_mode_req_type *)p_req;

    ledmode[p_ledmode->storage][p_ledmode->block] = p_ledmode->mode;
    
    return 0;
}

uint8_t handlecmd_set_led_mode_all(tinycmd_pkt_req_type *p_req)
{
    tinycmd_set_led_mode_all_req_type *p_ledmode_all_req = (tinycmd_set_led_mode_all_req_type *)p_req;
    uint16_t i = 0;
 
    memcpy(ledmode, p_ledmode_all_req->data, sizeof(ledmode));

    if((p_ledmode_all_req->cmd_code & TINY_CMD_RSP_MASK) != 0)
    {
         /*
         * To set response data, wait until i2c_reply_ready() returns nonzero,
         * then fill i2c_rdbuf with the data, finally call i2c_reply_done(n).
         * Interrupts are disabled while updating.
         */
        while(i2c_reply_ready() == 0);
        
        tinycmd_rsp_type *p_gen_rsp = (tinycmd_ver_rsp_type *)i2c_rdbuf;

        p_gen_rsp->cmd_code = TINY_CMD_VER_F;
        p_gen_rsp->pkt_len = sizeof(tinycmd_ver_rsp_type);
        i2c_reply_done(p_gen_rsp->pkt_len);

        // debug
        //led_array_on(TRUE, 0, 100, 0);
        sysConfig.comm_init = 1;

#if 0
{
    uint8_t i, j;
    uint8_t *pTmp = localBuffer;
    
    // Three lock
    *(pTmp++) = threeLock[0];
    *(pTmp++) = threeLock[1];
    *(pTmp++) = threeLock[2];

    for(j = 0; j < 2; j++)
    {
        for(i = 0; i < 5; i++)
        {
            switch(ledmode[j][i])
            {
                case 0:
                    *(pTmp++) = 0;
                    *(pTmp++) = 0;
                    *(pTmp++) = 0;
                    break;
                case 1:
                    *(pTmp++) = 50;
                    *(pTmp++) = 0;
                    *(pTmp++) = 0;
                    break;
                case 2:
                    *(pTmp++) = 0;
                    *(pTmp++) = 50;
                    *(pTmp++) = 0;
                    break;
                case 3:
                    *(pTmp++) = 0;
                    *(pTmp++) = 0;
                    *(pTmp++) = 50;
                    break;
                case 4:
                    *(pTmp++) = 50;
                    *(pTmp++) = 50;
                    *(pTmp++) = 0;
                    break;
                case 5:
                    *(pTmp++) = 0;
                    *(pTmp++) = 50;
                    *(pTmp++) = 50;
                    break;
                case 6:
                    *(pTmp++) = 50;
                    *(pTmp++) = 0;
                    *(pTmp++) = 50;
                    break;
                default:
                    *(pTmp++) = 50;
                    *(pTmp++) = 50;
                    *(pTmp++) = 50;
                    break;
            }
        }
    }

    localBufferLength = CLED_ARRAY_SIZE;
    ws2812_sendarray(localBuffer, localBufferLength);
}
#endif

    }

    return 0;
}


uint8_t handlecmd_config(tinycmd_pkt_req_type *p_req)
{
    tinycmd_config_req_type *p_config = (tinycmd_config_req_type *)p_req;
    //uint8_t *pTmp = (uint8_t *)localBuffer;

    sysConfig.led_max = p_config->value.led_max;
    sysConfig.level_max = p_config->value.level_max;

    return 0;
}

uint8_t handlecmd_test(tinycmd_pkt_req_type *p_req)
{
    //tinycmd_test_data_type *p_data = (tinycmd_test_data_type *)&p_req->test.data;

    return 0;
}

uint8_t handlecmd(tinycmd_pkt_req_type *p_req)
{
    uint8_t ret = 0;
    uint8_t cmd;

    cmd = p_req->cmd_code & TINYCMD_CMD_MASK;

    // handle command
    if(handle_cmd_func[cmd] != 0)
    {
        ret = handle_cmd_func[cmd](p_req);
    }

    return ret;
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

void i2cSlaveSend(uint8_t *pData, uint8_t len)
{
    memcpy((uint8_t *)i2c_rdbuf, pData, len);

    /*
    * To set response data, wait until i2c_reply_ready() returns nonzero,
    * then fill i2c_rdbuf with the data, finally call i2c_reply_done(n).
    * Interrupts are disabled while updating.
    */
    while(i2c_reply_ready() != 0);
    i2c_reply_done(len);
}

#define END_MARKER 255 // Signals the end of transmission

#if 1
void tiny_led_off(LED_BLOCK block)
{
    switch(block)
    {
        case LED_PIN_NUMLOCK:
        case LED_PIN_CAPSLOCK:
        case LED_PIN_SCROLLOCK:
//            *(ledport[block]) |= BV(ledpin[block]);
            break;
        case LED_PIN_BASE:
            PORTB &= ~(1<<PB1);
            break;
        case LED_PIN_WASD:
            PORTB &= ~(1<<PB4);
            break;                    
        default:
            return;
    }
}

void tiny_led_on(LED_BLOCK block)
{
    switch(block)
    {
        case LED_PIN_NUMLOCK:
        case LED_PIN_CAPSLOCK:
        case LED_PIN_SCROLLOCK:
//            *(ledport[block]) |= BV(ledpin[block]);
            break;
        case LED_PIN_BASE:
            PORTB |= (1<<PB1);
            break;
        case LED_PIN_WASD:
            PORTB |= (1<<PB4);
            break;
        default:
            return;
    }
    
}

void tiny_led_wave_on(LED_BLOCK block)
{
    switch(block)
    {
        case LED_PIN_BASE:
            timer1PWMAOn();
            PORTB |= (1<<PB1);
            break;
        case LED_PIN_WASD:
            timer1PWMBOn();
            PORTB |= (1<<PB4);
            break;
        default:
            break;
    }
}

void tiny_led_wave_off(LED_BLOCK block)
{
    switch(block)
    {
        case LED_PIN_BASE:
            timer1PWMASet(PWM_DUTY_MIN);
            timer1PWMAOff();
            break;
        case LED_PIN_WASD:
            timer1PWMBSet(PWM_DUTY_MIN);
            timer1PWMBOff();
            break;
        default:
            break;
    }
}




void tiny_led_wave_set(LED_BLOCK block, uint16_t duty)
{
    switch(block)
    {
        case LED_PIN_BASE:
            timer1PWMASet(duty);
            break;
        case LED_PIN_WASD:
            timer1PWMBSet(duty);
            break;
       default:
            break;
    }
}

void tiny_blink(int matrixState)
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
                    tiny_led_on(ledblock);
                    break;
                case LED_EFFECT_PUSH_OFF:
                    tiny_led_wave_off(ledblock);
                    tiny_led_wave_set(ledblock, 0);
                    tiny_led_off(ledblock);
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
                    tiny_led_off(ledblock);
                    break;
                case LED_EFFECT_PUSH_OFF:
                    tiny_led_on(ledblock);
                    break;
                default :
                    break;
            }
        }
    }
}

void tiny_fader(void)
{
    uint8_t ledblock;
    for (ledblock = LED_PIN_BASE; ledblock <= LED_PIN_WASD; ledblock++)
    {
        if((ledmode[ledmodeIndex][ledblock] == LED_EFFECT_FADING)
           || ((ledmode[ledmodeIndex][ledblock] == LED_EFFECT_FADING_PUSH_ON)))
        {
            if(pwmDir[ledblock]==0)
            {
                tiny_led_wave_set(ledblock, ((uint16_t)(pwmCounter[ledblock]/brigspeed[ledblock])));        // brighter
                if(pwmCounter[ledblock]>=255*brigspeed[ledblock])
                {
                    pwmDir[ledblock] = 1;
                    DEBUG_LED_ON(0, 1, 50, 50, 0);
                }
            }
            else if(pwmDir[ledblock]==2)
            {
                tiny_led_wave_set(ledblock, ((uint16_t)(255-pwmCounter[ledblock]/speed[ledblock])));    // darker
                if(pwmCounter[ledblock]>=255*speed[ledblock])
                {
                    pwmDir[ledblock] = 3;
                    DEBUG_LED_ON(1, 1, 50, 50, 0);
                }
            }
            else if(pwmDir[ledblock]==1)
            {
                if(pwmCounter[ledblock]>=255*speed[ledblock])
                {
                    pwmCounter[ledblock] = 0;
                    pwmDir[ledblock] = 2;
                    DEBUG_LED_ON(2, 1, 50, 50, 0);
                }
            }
            else if(pwmDir[ledblock]==3)
            {
                if(pwmCounter[ledblock]>=255*brigspeed[ledblock])
                {
                    pwmCounter[ledblock] = 0;
                    pwmDir[ledblock] = 0;
                    DEBUG_LED_ON(3, 1, 50, 50, 0);
                }
            }

            tiny_led_wave_on(ledblock);

            // pwmDir 0~3 : idle
       
            pwmCounter[ledblock]++;

        }
        else if (ledmode[ledmodeIndex][ledblock] == LED_EFFECT_PUSHED_LEVEL)
        {
    		// 일정시간 유지
    		if(pushedLevelStay[ledblock] > 0)
    		{
    			pushedLevelStay[ledblock]--;
    		}
    		else
    		{
    			// 시간이 흐르면 레벨을 감소 시킨다.
    			if(pushedLevelDuty[ledblock] > 0)
    			{
    				pwmCounter[ledblock]++;
    				if(pwmCounter[ledblock] >= speed[ledblock])
    				{
    					pwmCounter[ledblock] = 0;			
    					pushedLevelDuty[ledblock]--;
    					pushedLevel[ledblock] = PUSHED_LEVEL_MAX - (255-pushedLevelDuty[ledblock]) / (255/PUSHED_LEVEL_MAX);
    					/*if(pushedLevel_prev != pushedLevel){
    						DEBUG_PRINT(("---------------------------------decrease pushedLevel : %d, life : %d\n", pushedLevel, pushedLevelDuty));
    						pushedLevel_prev = pushedLevel;
    					}*/
    				}
    			}
    			else
    			{
    				pushedLevel[ledblock] = 0;
    				pwmCounter[ledblock] = 0;
    			}
    		}
    		tiny_led_wave_set(ledblock, pushedLevelDuty[ledblock]);

    	}
    	else
        {
            tiny_led_wave_set(ledblock, 0);
            tiny_led_wave_off(ledblock);
            pwmCounter[ledblock]=0;
            pwmDir[ledblock]=0;

            DEBUG_LED_ON(7, 1, 50, 50, 50);
        }
    }
}
#endif

void TinyInitHW(void)
{
    CLKPR=_BV(CLKPCE);  // Clock Prescaler Change Enable
    CLKPR=0;			// set clock prescaler to 1 (attiny 25/45/85/24/44/84/13/13A)

    // Port B Data Direction Register
    DDRB |= (1<<PB4) | (1<<PB3) | (1<<PB1);    // PB1 and PB4 is LED0 and LED1 driver.
    //DDRB |= (1<<PB4) | (1<<PB1);    // PB1 and PB4 is LED0 and LED1 driver.
    PORTB |= (1<<PB4) | (1<<PB1);


    // port output
    //DDRB |= (1<<ws2812_pin)|(1<<ws2812_pin2)|(1<<ws2812_pin3);

    i2c_initialize( SLAVE_ADDR );

    initWs2812(ws2812_pin);
}

void TinyInitTimer(void)
{
    timer1Init();
    timer1SetPrescaler(TIMER_CLK_DIV8);
    timer1PWMInit(0);

    // Enable PWM
    timer1PWMAOn();
    timer1PWMBOn();
}

int main(void)
{
    uint8_t i = 0;
    uint8_t *pTmp;
    uint8_t rcvlen;

    uint16_t count = 0;

    sysConfig.comm_init = 0;
    
    TinyInitHW();
    TinyInitTimer();

    sei();

    for(;;)
    {
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
            
            handlecmd((tinycmd_pkt_req_type *)cmdBuffer);
        }
        else
        {
            if(sysConfig.comm_init)
            {
                if(++count%250 == 0)
                {
                    tiny_blink(0);
                    tiny_fader();
                }
            }
        }
        //PORTB |= (1<<PB4);
        //blink();

    }

    return 0;

}





