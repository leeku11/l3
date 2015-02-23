/*
*/

#include <avr/io.h>     // include I/O definitions (port names, pin names, etc)
#include <avr/wdt.h>    // include watch dog feature

#include <stdbool.h>
#include <util/delay.h>
#include <string.h>

#include "i2c-slave.h"
#include "../Light_WS2812/light_ws2812.h"
#include "timeratt.h"
#include "led.h"
#include "keymap.h"
#include "matrix.h"
#include "hwaddress.h" // for rgb effect. temporary

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

#define CLED_NUM                        21           // (NCS)x1 + (RGB5050)x20
#define CLED_ELEMENT                    3
#define CLED_ARRAY_SIZE                 (CLED_NUM*CLED_ELEMENT)
#define CLED_GET_ARRAY_SIZE(num)        (num*CLED_ELEMENT)

#define CHANNEL_MODE_SYNC               0
#define CHANNEL_MODE_ASYNC              1

#define KEY_LED_CHANNEL_A               0
#define KEY_LED_CHANNEL_B               1
#define KEY_LED_CHANNEL_ALL             0xFF

#define OFF                             0
#define ON                              1

#define ws2812_pin2                     1
#define ws2812_pin3                     4       // PB4 -> OC1B
//#define ws2812_pin4                                      5           //reset... do not use!!


#define TINY_LED_EFFECT_COUNT           131
#define TINY_RGB_EFFECT_COUNT           400

#define DEBUG_LED_ON(p, o, r, g, b)    // rgb_pos_on(p, o, r, g, b)

typedef struct {
    uint8_t comm_init;
    uint8_t rgb_num;
    uint16_t rgb_effect_speed;
    uint8_t rgb_effect_on;
    uint8_t led_effect_on;
    uint8_t is_sleep;
    uint16_t rgb_limit;
} tiny_config_type;

typedef struct {
    uint8_t index;
    uint16_t maxRGBsum;
    uint8_t high_hold;
    uint8_t low_hold;
    uint8_t accel_mode;
    uint8_t dir;
    uint8_t level;
    uint8_t cnt;
    uint8_t hcnt;
    uint8_t lcnt;
} tiny_rgb_effect_type;

typedef uint8_t (*tiny_effector_func)(tiny_rgb_effect_type *); 

typedef enum
{
    TINY_LED_PIN_BASE = 0,
    TINY_LED_PIN_WASD,
    TINY_LED_PIN_MAX
}TINY_LED_BLOCK;    
#define TINY_LED_BLOCK_MAX              TINY_LED_PIN_MAX

// local data buffer
uint8_t rgbBuffer[CLED_NUM][CLED_ELEMENT] = 
            {{0, 0, 0}, {0, 250, 0}, {100, 250,0}, {250, 250, 0}, {250, 0, 0}, {0, 0, 250}, {0, 50, 250}, {0, 250, 250},
        {0, 250, 250}, {0, 50, 250}, {0, 0, 250},  {250, 0, 0},{250, 250, 0},{100, 250,0}, {0, 250, 0},
        {200,0,200}, {200,0,0},{200,50,0},{200,100,0},{200,150,0}};

uint8_t tmprgbBuffer[CLED_NUM][CLED_ELEMENT];

uint8_t cmdBuffer[I2C_RECEIVE_DATA_BUFFER_SIZE];

uint8_t tinyLedmodeIndex = 0;
uint8_t tinyLedmode[LEDMODE_INDEX_MAX][TINY_LED_BLOCK_MAX];

static uint8_t tinySpeed[TINY_LED_BLOCK_MAX] = {5, 5};
static uint8_t tinyBrigspeed[TINY_LED_BLOCK_MAX] = {3, 3};
static uint8_t tinyPwmDir[TINY_LED_BLOCK_MAX] = {0, 0};
static uint16_t tinyPwmCounter[TINY_LED_BLOCK_MAX] = {0, 0};

static uint16_t tinyPushedLevelStay[TINY_LED_BLOCK_MAX] = {0, 0};
static uint8_t tinyPushedLevel[TINY_LED_BLOCK_MAX] = {0, 0};
static uint16_t tinyPushedLevelDuty[TINY_LED_BLOCK_MAX] = {0, 0};

uint8_t tinyRgbmodeIndex = 0; // effect preset index
tiny_rgb_effect_type tinyRgbEffect[RGBMODE_INDEX_MAX]; // rgb effect preset buffer

uint8_t scanDirty;
uint32_t gcounter = 0;

tiny_config_type tinyConfig;

void key_led_control(uint8_t ch, uint8_t on);
void key_led_pwm_on(uint8_t channel, uint8_t on);
void key_led_pwm_duty(uint8_t channel, uint8_t duty);
void tiny_led_mode_change (TINY_LED_BLOCK ledblock, int mode);
void tiny_led_pushed_level_cal(void);
void tiny_led_off(TINY_LED_BLOCK block);
void tiny_led_on(TINY_LED_BLOCK block);
void tiny_led_wave_on(TINY_LED_BLOCK block);
void tiny_led_wave_off(TINY_LED_BLOCK block);
void tiny_led_blink(int matrixState);

uint8_t handlecmd_ver(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_reset(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_three_lock(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_all(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_pos(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_range(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_buffer(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_set_effect(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_set_preset(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_effect_speed(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_effect_on(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_led_level(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_led_set_effect(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_led_set_preset(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_led_config_preset(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_led_effect_on(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_config(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_dirty(tinycmd_pkt_req_type *p_req);

const tinycmd_handler_array_type cmdhandler[] = {
    {TINY_CMD_CONFIG_F, handlecmd_config},
    {TINY_CMD_VER_F, handlecmd_ver},
    {TINY_CMD_RESET_F,handlecmd_reset},
    {TINY_CMD_THREE_LOCK_F,handlecmd_three_lock},
    {TINY_CMD_DIRTY_F,handlecmd_dirty},

    {TINY_CMD_RGB_ALL_F,handlecmd_rgb_all},
    {TINY_CMD_RGB_POS_F,handlecmd_rgb_pos},
    {TINY_CMD_RGB_RANGE_F,handlecmd_rgb_range},
    {TINY_CMD_RGB_BUFFER_F,handlecmd_rgb_buffer},
    {TINY_CMD_RGB_SET_EFFECT_F,handlecmd_rgb_set_effect},
    {TINY_CMD_RGB_SET_PRESET_F,handlecmd_rgb_set_preset},
    {TINY_CMD_RGB_EFFECT_SPEED_F,handlecmd_rgb_effect_speed},
    {TINY_CMD_RGB_EFFECT_ON_F,handlecmd_rgb_effect_on},

    {TINY_CMD_LED_LEVEL_F,handlecmd_led_level},
    {TINY_CMD_LED_SET_EFFECT_F,handlecmd_led_set_effect},
    {TINY_CMD_LED_SET_PRESET_F,handlecmd_led_set_preset},
    {TINY_CMD_LED_CONFIG_PRESET_F,handlecmd_led_config_preset},
    {TINY_CMD_LED_EFFECT_ON_F,handlecmd_led_effect_on}
    // TINY_CMD_MAX
};
#define CMD_HANDLER_TABLE_SIZE            (sizeof(handle_cmd_func)/sizeof(tinycmd_handler_func))

uint8_t rgb_effect_snake(tiny_rgb_effect_type *p_effect);
uint8_t rgb_effect_crazy(tiny_rgb_effect_type *p_effect);
uint8_t rgb_effect_null(tiny_rgb_effect_type *p_effect);
uint8_t rgb_effect_basic(tiny_rgb_effect_type *p_effect);
uint8_t rgb_effect_basic_loop(tiny_rgb_effect_type *p_effect);
uint8_t rgb_effect_fade_inout_buf(tiny_rgb_effect_type *p_effect);
uint8_t rgb_effect_fade_inout_loop(tiny_rgb_effect_type *p_effect);

const tiny_effector_func effectHandler[] = {
    rgb_effect_snake,              // RGB_EFFECT_BOOTHID
    rgb_effect_fade_inout_buf,     // RGB_EFFECT_FADE_BUF
    rgb_effect_fade_inout_loop,    // RGB_EFFECT_FADE_LOOP
    rgb_effect_fade_inout_buf,     // RGB_EFFECT_HEARTBEAT_BUF
    rgb_effect_fade_inout_loop,     // RGB_EFFECT_HEARTBEAT_LOOP
    rgb_effect_basic
};
#define EFFECT_HANDLER_TABLE_SIZE            (sizeof(effectHandler)/sizeof(tiny_effector_func))


void rgb_weighted_mask(uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint32_t gRGBlimit, weight;

    gRGBlimit = tinyConfig.rgb_limit;
    weight = *r + *g + *b;

    if(gRGBlimit < weight)     // do not amplify
    {
        *r = (uint8_t)((uint32_t )*r * gRGBlimit / weight);
        *g = (uint8_t)((uint32_t )*g * gRGBlimit / weight);
        *b = (uint8_t)((uint32_t )*b * gRGBlimit / weight);
    }
}


void rgb_array_clear(void)
{
    memset(rgbBuffer[1], 0, (CLED_ARRAY_SIZE - CLED_ELEMENT));
}

void rgb_array_on(uint8_t on, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t i;
    
    if(on == 0)
    {
        r = g = b = 0;
    }
    rgb_weighted_mask(&r, &g, &b);
    
    for(i = 1; i < tinyConfig.rgb_num; i++)
    {
        rgbBuffer[i][0] = g;
        rgbBuffer[i][1] = r;
        rgbBuffer[i][2] = b;
    }
    ws2812_sendarray((uint8_t *)rgbBuffer, CLED_GET_ARRAY_SIZE(tinyConfig.rgb_num));
}

void rgb_pos_on(uint8_t pos, uint8_t on, uint8_t r, uint8_t g, uint8_t b)
{
    rgb_array_clear();
    
    if(on == 0)
    {
        r = g = b = 0;
    }

    pos++;          // skip NCR indicator

    rgb_weighted_mask(&r, &g, &b);

    rgbBuffer[pos][0] = g;
    rgbBuffer[pos][1] = r;
    rgbBuffer[pos][2] = b;

    ws2812_sendarray((uint8_t *)rgbBuffer, CLED_GET_ARRAY_SIZE(tinyConfig.rgb_num));
}

uint8_t rgb_effect_null(tiny_rgb_effect_type *p_effect)
{
    return 0;
}

uint8_t rgb_effect_snake(tiny_rgb_effect_type *p_effect)
{
    static uint8_t pos, state;
    uint8_t i;

    memset(&tmprgbBuffer[1][0], 0x00, CLED_ARRAY_SIZE);
    if(state == 0)
    {
        for(i = 0; i < pos; i++)            // turn on from 0 to pos
        {
            tmprgbBuffer[i][0] = rgbBuffer[i][0];
            tmprgbBuffer[i][1] = rgbBuffer[i][1];
            tmprgbBuffer[i][2] = rgbBuffer[i][2];
        }
        if(pos++ > tinyConfig.rgb_num)
        {
            pos = 0;
            state = 1;
        }
    }else
    {
        for(i = pos; i < tinyConfig.rgb_num; i++)     // turn on from pos to end
        {
            tmprgbBuffer[i][0] = rgbBuffer[i][0];
            tmprgbBuffer[i][1] = rgbBuffer[i][1];
            tmprgbBuffer[i][2] = rgbBuffer[i][2];
        }
        if(pos++ > tinyConfig.rgb_num)
        {
            pos = 0;
            state = 0;
        }
    }
  
    return 1;
}



uint8_t rgb_effect_crazy(tiny_rgb_effect_type *p_effect)
{
    uint8_t i;
    for(i = 0; i < tinyConfig.rgb_num; i++)            
    {
        tmprgbBuffer[i][0] = TCNT0;
        tmprgbBuffer[i][1] = TCNT1;
        tmprgbBuffer[i][2] = TCNT0^TCNT1;
    }
    return 1;
}


uint8_t rgb_effect_basic(tiny_rgb_effect_type *p_effect)
{
    uint8_t i = 0;
    uint32_t gRGBlimit, weight;

    gRGBlimit = tinyConfig.rgb_limit;
    
    tmprgbBuffer[i][0] = rgbBuffer[0][0];
    tmprgbBuffer[i][1] = rgbBuffer[0][1];
    tmprgbBuffer[i][2] = rgbBuffer[0][2];
    i++;

    for(; i < tinyConfig.rgb_num; i++)
    {
        weight = (rgbBuffer[i][0] + rgbBuffer[i][1] +rgbBuffer[i][2]);
        
        if(gRGBlimit >= weight)     // do not amplify
        {
            tmprgbBuffer[i][0] = rgbBuffer[i][0];
            tmprgbBuffer[i][1] = rgbBuffer[i][1];
            tmprgbBuffer[i][2] = rgbBuffer[i][2];

        }else
        {
            tmprgbBuffer[i][0] = (uint8_t)((uint32_t)rgbBuffer[i][0] * gRGBlimit / weight);
            tmprgbBuffer[i][1] = (uint8_t)((uint32_t)rgbBuffer[i][1] * gRGBlimit / weight);
            tmprgbBuffer[i][2] = (uint8_t)((uint32_t)rgbBuffer[i][2] * gRGBlimit / weight);
        }
    }

    for(i = tinyConfig.rgb_num; i < CLED_NUM; i++)
    {
        tmprgbBuffer[i][0] = 0;
        tmprgbBuffer[i][1] = 0;
        tmprgbBuffer[i][2] = 0;
    }
    
    return 1;
}

uint8_t rgb_effect_basic_loop(tiny_rgb_effect_type *p_effect)
{
    uint8_t i = 0;
    uint8_t rgb[3];
    uint32_t gRGBlimit, weight;

    gRGBlimit = tinyConfig.rgb_limit;


    tmprgbBuffer[i][0] = rgbBuffer[0][0];
    tmprgbBuffer[i][1] = rgbBuffer[0][1];
    tmprgbBuffer[i][2] = rgbBuffer[0][2];
    i++;

    rgb[0] = rgbBuffer[p_effect->cnt + i][0];
    rgb[1] = rgbBuffer[p_effect->cnt + i][1];
    rgb[2] = rgbBuffer[p_effect->cnt + i][2];

    for(; i < tinyConfig.rgb_num; i++)
    {
        weight = rgb[0] + rgb[1] + rgb[2];
        if(gRGBlimit >= weight)     // do not amplify
        {
            tmprgbBuffer[i][0] = rgb[0];
            tmprgbBuffer[i][1] = rgb[1];
            tmprgbBuffer[i][2] = rgb[2];

        }else
        {
            tmprgbBuffer[i][0] = (uint8_t)((uint32_t)rgb[0] * gRGBlimit / weight);
            tmprgbBuffer[i][1] = (uint8_t)((uint32_t)rgb[1] * gRGBlimit / weight);
            tmprgbBuffer[i][2] = (uint8_t)((uint32_t)rgb[2] * gRGBlimit / weight);
        }
    }
    if(++p_effect->hcnt > p_effect->high_hold)
    {
        p_effect->hcnt = 0;
        if(++p_effect->cnt == (tinyConfig.rgb_num - 1))
        {
            p_effect->cnt = 0;
        }
    }

    return 1;
}

static uint16_t rgbstep = 0;

uint8_t rgb_effect_fade_inout_buf(tiny_rgb_effect_type *p_effect)
{
    uint8_t i = 0, j;
    uint8_t max, limit;
    tmprgbBuffer[i][0] = rgbBuffer[0][0];
    tmprgbBuffer[i][1] = rgbBuffer[0][1];
    tmprgbBuffer[i][2] = rgbBuffer[0][2];
    i++;
    
    max = 0;
    if(p_effect->dir == 0) // ASCENDING
    {
        for(; i < tinyConfig.rgb_num; i++)
        {
            for(j = 0; j < 3; j++)
            {
                // read pixel level
         
                limit = rgbBuffer[i][j];  
                                
                // increase level according to mode
                tmprgbBuffer[i][j] = (uint8_t)((uint16_t)limit * rgbstep / (uint16_t)255);

                if(tmprgbBuffer[i][j] > limit)
                {
                    tmprgbBuffer[i][j] = limit;
                }

                if(p_effect->accel_mode == 0)
                {
                    rgbstep = p_effect->level;                                            // liner
                }
                else if(p_effect->accel_mode == 1)
                {
                    rgbstep = p_effect->level * p_effect->level + 1;    //exponential
                }
                else if(p_effect->accel_mode == 2)
                {
                    rgbstep = p_effect->level * 4 + 1;                  // x4
                }
            }
        }

        // If no update go to the sustain state
        if(rgbstep >= 255)
        {
            rgbstep = 255;
            if(p_effect->hcnt++ > p_effect->high_hold)
            {
                //p_effect->level = 0;
                p_effect->dir = (!p_effect->dir);
                p_effect->hcnt = 0;
            }
        }
        else
        {
            p_effect->level++;
        }
    }
    else // DESCENDING
    {
        for(; i < tinyConfig.rgb_num; i++)
        {
            for(j = 0; j < 3; j++)
            {
                tmprgbBuffer[i][j] = (uint8_t)((uint16_t)rgbBuffer[i][j] * rgbstep / (uint16_t)255);
            }
        }

        if( rgbstep-- <= 0)
        {
            rgbstep = 0;
            if(p_effect->lcnt++ > p_effect->low_hold)
            {
                p_effect->level = 0;
                p_effect->dir = (!p_effect->dir);
                p_effect->lcnt = 0;
            }
        }
    }

    return 1;
}

uint8_t rgb_effect_fade_inout_loop(tiny_rgb_effect_type *p_effect)
{
    uint8_t i = 0, j;
    uint8_t max , limit;
    uint8_t rgb[3];

    tmprgbBuffer[i][0] = rgbBuffer[0][0];
    tmprgbBuffer[i][1] = rgbBuffer[0][1];
    tmprgbBuffer[i][2] = rgbBuffer[0][2];
    i++;

    rgb[0] = rgbBuffer[p_effect->cnt + i][0];
    rgb[1] = rgbBuffer[p_effect->cnt + i][1];
    rgb[2] = rgbBuffer[p_effect->cnt + i][2];

    max = 0;
    if(p_effect->dir == 0) // ASCENDING
    {
        for(; i < tinyConfig.rgb_num; i++)
        {
            for(j = 0; j < 3; j++)
            {
                // read pixel level
                limit = rgb[j];  
              
                // increase level according to mode
                tmprgbBuffer[i][j] = (uint8_t)((uint16_t)limit * rgbstep / (uint16_t)255);

                if(tmprgbBuffer[i][j] > limit)
                {
                    tmprgbBuffer[i][j] = limit;
                }

                if(p_effect->accel_mode == 0)
                {
                    rgbstep = p_effect->level;                                            // liner
                }
                else if(p_effect->accel_mode == 1)
                {
                    rgbstep = p_effect->level * p_effect->level + 1;    //exponential
                }
                else if(p_effect->accel_mode == 2)
                {
                    rgbstep = p_effect->level * 4 + 1;                  // x4
                }
            }
        }

        // If no update go to the sustain state
        if(rgbstep >= 255)
        {
            rgbstep = 255;
            if(p_effect->hcnt++ > p_effect->high_hold)
            {
                //p_effect->level = 0;
                p_effect->dir = (!p_effect->dir);
                p_effect->hcnt = 0;
            }
        }
        else
        {
            p_effect->level++;
        }
    }
    else // DESCENDING
    {
        for(; i < tinyConfig.rgb_num; i++)
        {
            for(j = 0; j < 3; j++)
            {
                tmprgbBuffer[i][j] = (uint8_t)((uint16_t)rgb[j] * rgbstep / (uint16_t)255);
            }
        }

        if(rgbstep-- <= 0)
        {
            rgbstep = 0;
            if(p_effect->lcnt++ > p_effect->low_hold)
            {
                p_effect->level = 0;
                p_effect->dir = (!p_effect->dir);
                p_effect->lcnt = 0;

                if(++p_effect->cnt == (tinyConfig.rgb_num - 1))
                {
                    p_effect->cnt = 0;
                }
            }
        }
    }

    return 1;
}

static uint8_t sendResponse(uint8_t cmd)
{
    tinycmd_rsp_type *p_rsp = (tinycmd_rsp_type *)i2c_rdbuf;
    volatile uint16_t i2cTimeout;
     /*
     * To set response data, wait until i2c_reply_ready() returns nonzero,
     * then fill i2c_rdbuf with the data, finally call i2c_reply_done(n).
     * Interrupts are disabled while updating.
     */
    i2cTimeout = 0xFFFF;
    while((i2c_reply_ready() == 0) && i2cTimeout--);
    
    p_rsp->cmd_code = cmd;
    i2c_reply_done(sizeof(tinycmd_rsp_type));

    return 0;
}

uint8_t handlecmd_config(tinycmd_pkt_req_type *p_req)
{
    uint8_t i;
    tinycmd_config_req_type *p_config_req = (tinycmd_config_req_type *)p_req;

    tinyConfig.rgb_num = p_config_req->rgb_num;
    tinyConfig.rgb_limit = p_config_req->rgb_limit;

    for(i = 1; i < tinyConfig.rgb_num; i++)
    {
        rgb_weighted_mask(&rgbBuffer[i][0], &rgbBuffer[i][1], &rgbBuffer[i][2]);
    }
    
    return 0;
}

uint8_t handlecmd_ver(tinycmd_pkt_req_type *p_req)
{
    return 0;
}

uint8_t handlecmd_reset(tinycmd_pkt_req_type *p_req)
{
    wdt_enable(WDTO_250MS);
    while(1); // wait for watchdog reset
    return 0;
}

uint8_t handlecmd_three_lock(tinycmd_pkt_req_type *p_req)
{
    uint8_t i;
    tinycmd_three_lock_req_type *p_three_lock_req = (tinycmd_three_lock_req_type *)p_req;

    rgbBuffer[0][0] = 0;     // Num
    rgbBuffer[0][1] = 0;     // Caps
    rgbBuffer[0][2] = 0;     // Scrl
    tmprgbBuffer[0][0] = 0;     // Num
    tmprgbBuffer[0][1] = 0;     // Caps
    tmprgbBuffer[0][2] = 0;     // Scrl

  
    if(p_three_lock_req->lock & (1<<2))
    {
        rgbBuffer[0][0] = 250;
        tmprgbBuffer[0][0] = 250;
    }
    if(p_three_lock_req->lock & (1<<1))
    {
        rgbBuffer[0][1] = 250;
        tmprgbBuffer[0][1] = 250;
    }
    if(p_three_lock_req->lock & (1<<0))
    {
        rgbBuffer[0][2] = 250;
        tmprgbBuffer[0][2] = 250;

    }
  

    for (i=TINY_LED_PIN_BASE; i<=TINY_LED_PIN_WASD; i++)
    {
        if(tinyLedmode[tinyLedmodeIndex][i] == LED_EFFECT_BASECAPS)
        {
            if(rgbBuffer[0][1] != 0)
            {
                tiny_led_on(i);
            }else
            {
                tiny_led_off(i);
            }
        }
    }

//    ws2812_sendarray(&rgbBuffer[0][0],CLED_ELEMENT);        // output message data to port D

    return 1;
}

uint8_t handlecmd_rgb_all(tinycmd_pkt_req_type *p_req)
{
    tinycmd_rgb_all_req_type *p_rgb_all_req = (tinycmd_rgb_all_req_type *)p_req;

    rgb_array_on(p_rgb_all_req->on, p_rgb_all_req->led.r, p_rgb_all_req->led.g, p_rgb_all_req->led.b);

    return 0;
}

uint8_t handlecmd_rgb_pos(tinycmd_pkt_req_type *p_req)
{
    tinycmd_rgb_pos_req_type *p_rgb_pos_req = (tinycmd_rgb_pos_req_type *)p_req;
    uint8_t tmpPos;
    // clear buffer
    rgb_array_clear();

    tmpPos = p_rgb_pos_req->pos + 1;          // skip NCR indicator

    rgb_weighted_mask(&p_rgb_pos_req->led.r, &p_rgb_pos_req->led.g, &p_rgb_pos_req->led.b);
          
    rgbBuffer[tmpPos][0] = p_rgb_pos_req->led.b;
    rgbBuffer[tmpPos][1] = p_rgb_pos_req->led.r;
    rgbBuffer[tmpPos][2] = p_rgb_pos_req->led.g;
        
    ws2812_sendarray((uint8_t *)rgbBuffer, CLED_GET_ARRAY_SIZE(tinyConfig.rgb_num));        // output message data to port D

    return 0;
}

uint8_t handlecmd_rgb_range(tinycmd_pkt_req_type *p_req)
{
    tinycmd_rgb_range_req_type *p_rgb_range_req = (tinycmd_rgb_range_req_type *)p_req;
    tinycmd_led_type *pLed; 
    uint8_t i, tmpPos;

    // clear buffer
    rgb_array_clear();

    tmpPos = p_rgb_range_req->offset + 1;
    pLed = p_rgb_range_req->led;

    for(i = 0; i < p_rgb_range_req->num; i++)
    {
        rgb_weighted_mask(&pLed[i].r, &pLed[i].g, &pLed[i].b);
        rgbBuffer[tmpPos+i][0] = pLed[i].b;
        rgbBuffer[tmpPos+i][1] = pLed[i].r;
        rgbBuffer[tmpPos+i][2] = pLed[i].g;
    }

    ws2812_sendarray((uint8_t *)rgbBuffer, CLED_GET_ARRAY_SIZE(tinyConfig.rgb_num));        // output message data to port D

    return 0;
}

uint8_t handlecmd_rgb_buffer(tinycmd_pkt_req_type *p_req)
{
    tinycmd_rgb_buffer_req_type *p_rgb_buffer_req = (tinycmd_rgb_buffer_req_type *)p_req;
    uint8_t tmpPos, i;

    // clear buffer
    rgb_array_clear();

    tmpPos = p_rgb_buffer_req->offset + 1;

    memcpy(&rgbBuffer[tmpPos][0], p_rgb_buffer_req->data, p_rgb_buffer_req->num * CLED_ELEMENT);

    for(i = 1; i < tinyConfig.rgb_num; i++)
    {
        rgb_weighted_mask(&rgbBuffer[i][0], &rgbBuffer[i][1], &rgbBuffer[i][2]);
    }
//    ws2812_sendarray((uint8_t *)rgbBuffer, CLED_GET_ARRAY_SIZE(tinyConfig.rgb_num));        // output message data to port D

    return 0;
}

uint8_t handlecmd_rgb_set_effect(tinycmd_pkt_req_type *p_req)
{
    tinycmd_rgb_set_effect_req_type *p_set_effect_req = (tinycmd_rgb_set_effect_req_type *)p_req;
    tinyRgbmodeIndex = p_set_effect_req->index;
    if(tinyRgbmodeIndex >= RGBMODE_INDEX_MAX)
    {
        tinyRgbmodeIndex = 0;
    }

    return 0;
}

uint8_t handlecmd_rgb_set_preset(tinycmd_pkt_req_type *p_req)
{
    uint8_t index;
    tinycmd_rgb_set_preset_req_type *p_set_preset_req = (tinycmd_rgb_set_preset_req_type *)p_req;
    tinyRgbmodeIndex = index = p_set_preset_req->index;
    if(index >= RGBMODE_INDEX_MAX)
    {
        index = 0;
    }
    tinyRgbmodeIndex = index;
    // Zero init.
    memset(&tinyRgbEffect[index], 0, sizeof(tiny_rgb_effect_type));
    // Fill parameter from command
    tinyRgbEffect[index].index = p_set_preset_req->effect_param.index;
    tinyRgbEffect[index].maxRGBsum = tinyConfig.rgb_limit;
    tinyRgbEffect[index].high_hold = p_set_preset_req->effect_param.high_hold;
    tinyRgbEffect[index].low_hold = p_set_preset_req->effect_param.low_hold;
    tinyRgbEffect[index].accel_mode = p_set_preset_req->effect_param.accel_mode;

    return 0;
}

uint8_t handlecmd_rgb_effect_speed(tinycmd_pkt_req_type *p_req)
{
    tinycmd_rgb_effect_speed_req_type *p_effect_speed_req = (tinycmd_rgb_effect_speed_req_type *)p_req;
    tinyConfig.rgb_effect_speed = p_effect_speed_req->speed;

    return 0;
}

uint8_t handlecmd_rgb_effect_on(tinycmd_pkt_req_type *p_req)
{
    tinycmd_rgb_effect_on_req_type *p_effect_on_req = (tinycmd_rgb_effect_on_req_type *)p_req;

    tinyConfig.rgb_effect_on = p_effect_on_req->on;

    return 0;
}

uint8_t handlecmd_led_level(tinycmd_pkt_req_type *p_req)
{
    tinycmd_led_level_req_type *p_led_level_req = (tinycmd_led_level_req_type *)p_req;
    
    key_led_control(p_led_level_req->channel, p_led_level_req->level);

    return 0;
}

uint8_t handlecmd_led_set_effect(tinycmd_pkt_req_type *p_req)
{
    tinycmd_led_set_effect_req_type *p_set_effect_req = (tinycmd_led_set_effect_req_type *)p_req;

    tinyLedmodeIndex = p_set_effect_req->preset;

    tiny_led_mode_change(TINY_LED_PIN_BASE, tinyLedmodeIndex);
    tiny_led_mode_change(TINY_LED_PIN_WASD, tinyLedmodeIndex);
    
    return 0;
}

uint8_t handlecmd_led_set_preset(tinycmd_pkt_req_type *p_req)
{
    tinycmd_led_set_preset_req_type *p_set_preset_req = (tinycmd_led_set_preset_req_type *)p_req;
    uint8_t block;

    block = p_set_preset_req->block - 3;
    tinyLedmode[p_set_preset_req->preset][block] = p_set_preset_req->effect;

    return 0;
}

uint8_t handlecmd_led_config_preset(tinycmd_pkt_req_type *p_req)
{
    tinycmd_led_config_preset_req_type *p_cfg_preset_req = (tinycmd_led_config_preset_req_type *)p_req;
    uint8_t i, j;
    uint8_t *pTmp = p_cfg_preset_req->data;

    //memcpy(tinyLedmode, p_cfg_preset_req->data, sizeof(tinyLedmode));
    for(i = 0; i < LEDMODE_INDEX_MAX; i++)
    {
        pTmp+=3;
        for(j = 0; j < TINY_LED_BLOCK_MAX; j++)
        {
            tinyLedmode[i][j] = *pTmp++;
        }
    }    

    for (i = 0; i < 2; i++)
    {
        tinyPwmDir[i] = 0;
        tinyPwmCounter[i] = 0;
        tiny_led_mode_change(i, tinyLedmode[tinyLedmodeIndex][i]);
    }
    tinyConfig.comm_init = 1;

    return 0;
}

uint8_t handlecmd_led_effect_on(tinycmd_pkt_req_type *p_req)
{
    tinycmd_led_effect_on_req_type *p_effect_on_req = (tinycmd_led_effect_on_req_type *)p_req;

    tinyConfig.led_effect_on = p_effect_on_req->on;

    return 0;
}

uint8_t handlecmd_dirty(tinycmd_pkt_req_type *p_req)
{
    if(p_req->cmd_code & TINY_CMD_KEY_MASK) // down
    {
        scanDirty |= SCAN_DIRTY;
        gcounter = 0;
        tiny_led_pushed_level_cal();
    }
    else
    {
        scanDirty = 0;
    }
    tiny_led_blink(scanDirty);
    return 0;
}

uint8_t handlecmd(tinycmd_pkt_req_type *p_req)
{
    uint8_t ret = 0;
    uint8_t cmd;
    uint8_t i;
    cmd = p_req->cmd_code & TINY_CMD_CMD_MASK;

    for(i = 0; i < sizeof(cmdhandler)/sizeof(tinycmd_handler_array_type); i++)    // scan command func array
    {
        // handle command
        if(cmdhandler[i].cmd == cmd)
        {
            ret = cmdhandler[i].p_func(p_req);
            if((p_req->cmd_code & TINY_CMD_RSP_MASK) != 0)
            {
                sendResponse(cmd);
            }
            break;
        }
    }

    return ret;
}

void i2cSlaveSend(uint8_t *pData, uint8_t len)
{
    memcpy((uint8_t *)i2c_rdbuf, pData, len);
    volatile uint16_t i2cTimeout;

    /*
    * To set response data, wait until i2c_reply_ready() returns nonzero,
    * then fill i2c_rdbuf with the data, finally call i2c_reply_done(n).
    * Interrupts are disabled while updating.
    */
    i2cTimeout = 0xFFFF;
    while((i2c_reply_ready() == 0) && i2cTimeout--);
    i2c_reply_done(len);
}

void key_led_control(uint8_t ch, uint8_t on)
{
    if(ch == KEY_LED_CHANNEL_ALL) // all
    {
        if(on)
        {
            PORTB |= ((1<<PB4) | (1<<PB1));
        }
        else
        {
            PORTB &= ~((1<<PB4) | (1<<PB1));
        }
    }
    else
    {
        if(ch == KEY_LED_CHANNEL_A)
        {
            PORTB |= (1<<PB1);
        }
        else if(ch == KEY_LED_CHANNEL_B)
        {
            PORTB |= (1<<PB4);
        }
    }
}

void key_led_pwm_on(uint8_t channel, uint8_t on)
{
    if(channel == KEY_LED_CHANNEL_A)
    {
        if(on)
        {
            timer1PWMAOn();
        }
        else
        {
            timer1PWMAOff();
        }
    }
    else if(channel == KEY_LED_CHANNEL_B)
    {
        if(on)
        {
            timer1PWMBOn();
        }
        else
        {
            timer1PWMBOff();
        }
    }
    else if(channel == KEY_LED_CHANNEL_ALL)
    {
        if(on)
        {
            timer1PWMAOn();
            timer1PWMBOn();
        }
        else
        {
            timer1PWMAOff();
            timer1PWMBOff();
        }
    }
}

void key_led_pwm_duty(uint8_t channel, uint8_t duty)
{
    if(channel == KEY_LED_CHANNEL_A)
    {
        timer1PWMASet(duty);
    }
    else if(channel == KEY_LED_CHANNEL_B)
    {
        timer1PWMBSet(duty);
    }
    else if(channel == KEY_LED_CHANNEL_ALL)
    {
        timer1PWMASet(duty);
        timer1PWMBSet(duty);
    }
}

void tiny_led_off(TINY_LED_BLOCK block)
{
    tiny_led_wave_off(block);
    switch(block)
    {
        case TINY_LED_PIN_BASE:
            PORTB &= ~(1<<PB1);
            break;
        case TINY_LED_PIN_WASD:
            PORTB &= ~(1<<PB4);
            break;                    
        default:
            return;
    }
}

void tiny_led_on(TINY_LED_BLOCK block)
{
    tiny_led_wave_off(block);
    switch(block)
    {
        case TINY_LED_PIN_BASE:
            PORTB |= (1<<PB1);
            break;
        case TINY_LED_PIN_WASD:
            PORTB |= (1<<PB4);
            break;
        default:
            return;
    }
    
}

void tiny_led_wave_on(TINY_LED_BLOCK block)
{
    switch(block)
    {
        case TINY_LED_PIN_BASE:
            timer1PWMAOn();
            PORTB |= (1<<PB1);
            break;
        case TINY_LED_PIN_WASD:
            timer1PWMBOn();
            PORTB |= (1<<PB4);
            break;
        default:
            break;
    }
}

void tiny_led_wave_off(TINY_LED_BLOCK block)
{
    switch(block)
    {
        case TINY_LED_PIN_BASE:
            timer1PWMASet(PWM_DUTY_MIN);
            timer1PWMAOff();
            break;
        case TINY_LED_PIN_WASD:
            timer1PWMBSet(PWM_DUTY_MIN);
            timer1PWMBOff();
            break;
        default:
            break;
    }
}




void tiny_led_wave_set(TINY_LED_BLOCK block, uint16_t duty)
{
    switch(block)
    {
        case TINY_LED_PIN_BASE:
            timer1PWMASet(duty);
            break;
        case TINY_LED_PIN_WASD:
            timer1PWMBSet(duty);
            break;
       default:
            break;
    }
}

void tiny_led_blink(int matrixState)
{
    TINY_LED_BLOCK ledblock;

    for (ledblock = TINY_LED_PIN_BASE; ledblock <= TINY_LED_PIN_WASD; ledblock++)
    {
        if(matrixState & SCAN_DIRTY)      // 1 or more key is pushed
        {
            switch(tinyLedmode[tinyLedmodeIndex][ledblock])
            {

                case LED_EFFECT_FADING_PUSH_ON:
                case LED_EFFECT_PUSH_ON:
                    tiny_led_on(ledblock);
                    break;
                case LED_EFFECT_PUSH_OFF:
                    tiny_led_off(ledblock);
                    break;
                default :
                    break;
            }             
        }
        else
        {          // none of keys is pushed
            switch(tinyLedmode[tinyLedmodeIndex][ledblock])
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

void tiny_led_fader(void)
{
    uint8_t ledblock;
    for (ledblock = TINY_LED_PIN_BASE; ledblock <= TINY_LED_PIN_WASD; ledblock++)
    {
        if((tinyLedmode[tinyLedmodeIndex][ledblock] == LED_EFFECT_FADING)
           || ((tinyLedmode[tinyLedmodeIndex][ledblock] == LED_EFFECT_FADING_PUSH_ON) && (gcounter > 0x10000)))
        {
            if(tinyPwmDir[ledblock]==0)
            {
                tiny_led_wave_set(ledblock, ((uint16_t)(tinyPwmCounter[ledblock]/tinyBrigspeed[ledblock])));        // brighter
                if(tinyPwmCounter[ledblock]>=255*tinyBrigspeed[ledblock])
                {
                    tinyPwmDir[ledblock] = 1;
                    DEBUG_LED_ON(0, 1, 50, 50, 0);
                }
            }
            else if(tinyPwmDir[ledblock]==2)
            {
                tiny_led_wave_set(ledblock, ((uint16_t)(255-tinyPwmCounter[ledblock]/tinySpeed[ledblock])));    // darker
                if(tinyPwmCounter[ledblock]>=255*tinySpeed[ledblock])
                {
                    tinyPwmDir[ledblock] = 3;
                    DEBUG_LED_ON(1, 1, 50, 50, 0);
                }
            }
            else if(tinyPwmDir[ledblock]==1)
            {
                if(tinyPwmCounter[ledblock]>=255*tinySpeed[ledblock])
                {
                    tinyPwmCounter[ledblock] = 0;
                    tinyPwmDir[ledblock] = 2;
                    DEBUG_LED_ON(2, 1, 50, 50, 0);
                }
            }
            else if(tinyPwmDir[ledblock]==3)
            {
                if(tinyPwmCounter[ledblock]>=255*tinyBrigspeed[ledblock])
                {
                    tinyPwmCounter[ledblock] = 0;
                    tinyPwmDir[ledblock] = 0;
                    DEBUG_LED_ON(3, 1, 50, 50, 0);
                }
            }

            tiny_led_wave_on(ledblock);

            // tinyPwmDir 0~3 : idle
       
            tinyPwmCounter[ledblock]++;

        }
        else if (tinyLedmode[tinyLedmodeIndex][ledblock] == LED_EFFECT_PUSHED_LEVEL)
        {
    		// 일정시간 유지
    		if(tinyPushedLevelStay[ledblock] > 0)
    		{
    			tinyPushedLevelStay[ledblock]--;
    		}
    		else
    		{
    			// 시간이 흐르면 레벨을 감소 시킨다.
    			if(tinyPushedLevelDuty[ledblock] > 0)
    			{
    					tinyPushedLevelDuty[ledblock]--;
    			}
    		}
    		tiny_led_wave_set(ledblock, tinyPushedLevelDuty[ledblock]);
            tiny_led_wave_on(ledblock);
    	}
    }
}

void tiny_rgb_effector(void)
{
    tiny_rgb_effect_type *pEffect;
    uint8_t update, index;

    if(!tinyConfig.rgb_effect_on)
    {
        return;
    }

    pEffect = &tinyRgbEffect[tinyRgbmodeIndex];
    update = 0;
    index = pEffect->index;

    if(index < RGB_EFFECT_MAX)
    {
        if(effectHandler[index] != 0)
        {
            update = effectHandler[index](pEffect);
        }
    }

    if(update)
    {
        ws2812_sendarray((uint8_t *)tmprgbBuffer, CLED_ARRAY_SIZE);
    }
}

void tiny_led_mode_change (TINY_LED_BLOCK ledblock, int mode)
{
    switch (mode)
    {
        case LED_EFFECT_FADING :
        case LED_EFFECT_FADING_PUSH_ON :
            tiny_led_wave_set(ledblock,0);
            tiny_led_wave_on(ledblock);
            break;
        case LED_EFFECT_PUSH_OFF :
        case LED_EFFECT_ALWAYS :
            tiny_led_wave_set(ledblock,0);
            tiny_led_wave_off(ledblock);
            tiny_led_on(ledblock);
            break;
        case LED_EFFECT_PUSH_ON :
        case LED_EFFECT_OFF :
            tiny_led_wave_set(ledblock,0);
            tiny_led_wave_off(ledblock);
            tiny_led_off(ledblock);
            break;

        case LED_EFFECT_PUSHED_LEVEL :
            tiny_led_wave_set(ledblock,0);
            tiny_led_wave_on(ledblock);
            break;
            
        case LED_EFFECT_BASECAPS :
            if(rgbBuffer[0][1] != 0)
            {
                tiny_led_on(ledblock);
            }else
            {
                tiny_led_off(ledblock);
            }
            break;
        default :
            tinyLedmode[tinyLedmodeIndex][ledblock] = LED_EFFECT_FADING;
            break;
     }
}

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
    timer0Init();
    timer1Init();
    timer1SetPrescaler(TIMER_CLK_DIV8);
    timer1PWMInit(0);

    // Enable PWM
    timer1PWMAOn();
    timer1PWMBOn();
}

void TinyInitCfg(void)
{
    memset(&tinyConfig, 0, sizeof(tiny_config_type));
    tinyConfig.rgb_num = CLED_NUM;
    tinyConfig.led_effect_on = 1; // default on
    tinyConfig.rgb_effect_on = 1; // default on
    tinyConfig.rgb_effect_speed = TINY_RGB_EFFECT_COUNT;
    tinyConfig.rgb_limit = 500;
}

void TinyInitEffect(void)
{
    uint8_t i;
    // init effect. firstly go to boot hid mode
    tinyRgbmodeIndex = 0;
    for(i = 0; i < RGBMODE_INDEX_MAX; i++)
    {
        memset(&tinyRgbEffect[i], 0, sizeof(tiny_rgb_effect_type));
    }
}

void tiny_led_pushed_level_cal(void)
{
    LED_BLOCK ledblock;
    // update pushed level

    for (ledblock = TINY_LED_PIN_BASE; ledblock <= TINY_LED_PIN_WASD; ledblock++)
    { 
        tinyPushedLevelStay[ledblock] = 180;
        if((tinyPushedLevelDuty[ledblock] += 15) >= 255)
        {
            tinyPushedLevelDuty[ledblock] = 255;
        }
    }
}


int main(void)
{
    uint8_t i = 0;
    uint8_t *pTmp;
    uint8_t rcvlen;
    uint16_t count = 0;
    uint16_t effect_count = 0;

    TinyInitHW();
    TinyInitTimer();

    TinyInitCfg();
    TinyInitEffect();

    wdt_enable(WDTO_2S);
    
    sei();

    for(;;)
    {
        wdt_reset();
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
            count++;
            effect_count++;
            if(gcounter++ > 0xfffff)
            {
                gcounter = 0xfffff;
            }

            if((count%TINY_LED_EFFECT_COUNT == 0) && (tinyConfig.comm_init))
            {
                tiny_led_fader();
            }

            if(effect_count % tinyConfig.rgb_effect_speed == 0)
            {
                tiny_rgb_effector();
            }
           
        }

    }

    return 0;

}
