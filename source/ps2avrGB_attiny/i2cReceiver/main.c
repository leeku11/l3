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

#define CLED_NUM           21           // (NCS)x1 + (RGB5050)x20
#define CLED_ELEMENT       3
#define CLED_ARRAY_SIZE    ((CLED_NUM)*CLED_ELEMENT)

#define CHANNEL_MODE_SYNC               0
#define CHANNEL_MODE_ASYNC              1

#define KEY_LED_CHANNEL_A               0
#define KEY_LED_CHANNEL_B               1
#define KEY_LED_CHANNEL_ALL             0xFF

#define OFF                             0
#define ON                              1

#define ws2812_pin2  1
#define ws2812_pin3  4	// PB4 -> OC1B
//#define ws2812_pin4  5 //reset... do not use!!

#define DEBUG_LED_ON(p, o, r, g, b)    // rgb_pos_on(p, o, r, g, b)

typedef struct {
    uint8_t comm_init;
    uint8_t rgb_num;
    uint8_t led_preset_num;
} tiny_config_type;

typedef uint8_t (*tiny_effector_func)(rgb_effect_param_type *); 

enum
{
    RGB_EFFECT_BOOTHID = 0,
    RGB_EFFECT_BASIC,
    RGB_EFFECT_BASIC_LOOP,
    RGB_EFFECT_FADE,
    RGB_EFFECT_FADE_BUF,
    RGB_EFFECT_FADE_LOOP,
    RGB_EFFECT_HEARTBEAT,
    RGB_EFFECT_HEARTBEAT_BUF,
    RGB_EFFECT_HEARTBEAT_LOOP,
    RGB_EFFECT_SWIPE,
    RGB_EFFECT_SWIPE_BUF,
    RGB_EFFECT_SWIPE_LOOP,
    RGB_EFFECT_MAX
};

// local data buffer
uint8_t rgbBuffer[CLED_NUM][CLED_ELEMENT];
uint8_t tmprgbBuffer[CLED_NUM][CLED_ELEMENT];
uint8_t rgbBufferLength; // = CLED_ARRAY_SIZE;

uint8_t cmdBuffer[I2C_RECEIVE_DATA_BUFFER_SIZE];

uint8_t *NCSLock; // initial level

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

uint8_t rgbmodeIndex = 0; // effect preset index
rgb_effect_param_type rgbEffect; // current rgb effect
//rgb_effect_param_type rgbEffectPreset[2];

uint8_t scanDirty;

tiny_config_type tinyConfig;

void key_led_control(uint8_t ch, uint8_t on);
void key_led_pwm_on(uint8_t channel, uint8_t on);
void key_led_pwm_duty(uint8_t channel, uint8_t duty);
void tiny_led_mode_change (LED_BLOCK ledblock, int mode);


uint8_t handlecmd_ver(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_reset(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_three_lock(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_all(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_pos(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_range(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_buffer(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_set_effect(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_rgb_set_preset(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_led_level(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_led_set_effect(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_led_set_preset(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_led_config_preset(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_config(tinycmd_pkt_req_type *p_req);
uint8_t handlecmd_dirty(tinycmd_pkt_req_type *p_req);

const tinycmd_handler_array_type cmdhandler[] = {
    {TINY_CMD_CONFIG, handlecmd_config},                 // TINY_CMD_CONFIG
    {TINY_CMD_VER_F, handlecmd_ver},                    // TINY_CMD_VER_F
    {TINY_CMD_RESET_F,handlecmd_reset},                  // TINY_CMD_RESET_F
    {TINY_CMD_THREE_LOCK_F,handlecmd_three_lock},             // TINY_CMD_THREE_LOCK_F
    {TINY_CMD_DIRTY,handlecmd_dirty},                  // TINY_CMD_DIRTY_F

    {TINY_CMD_RGB_ALL_F,handlecmd_rgb_all},                // TINY_CMD_RGB_ALL_F
    {TINY_CMD_RGB_POS_F,handlecmd_rgb_pos},                // TINY_CMD_RGB_POS_F
    {TINY_CMD_RGB_RANGE_F,handlecmd_rgb_range},              // TINY_CMD_RGB_RANGE_F
    {TINY_CMD_RGB_BUFFER_F,handlecmd_rgb_buffer},              // TINY_CMD_RGB_BUFFER_F
    {TINY_CMD_RGB_SET_EFFECT_F,handlecmd_rgb_set_effect},         // TINY_CMD_RGB_SET_EFFECT_F
    {TINY_CMD_RGB_SET_PRESET_F,handlecmd_rgb_set_preset},         // TINY_CMD_RGB_SET_PRESET_F

    {TINY_CMD_LED_LEVEL_F,handlecmd_led_level},              // TINY_CMD_LED_LEVEL_F
    {TINY_CMD_LED_SET_EFFECT_F,handlecmd_led_set_effect},         // TINY_CMD_LED_SET_EFFECT_F
    {TINY_CMD_LED_SET_PRESET_F,handlecmd_led_set_preset},         // TINY_CMD_LED_SET_PRESET_F
    {TINY_CMD_LED_CONFIG_PRESET_F,handlecmd_led_config_preset}      // TINY_CMD_LED_CONFIG_PRESET_F
                                // TINY_CMD_MAX
};
#define CMD_HANDLER_TABLE_SIZE            (sizeof(handle_cmd_func)/sizeof(tinycmd_handler_func))

uint8_t rgb_effect_null(rgb_effect_param_type *p_effect);
uint8_t rgb_effect_basic(rgb_effect_param_type *p_effect);
uint8_t rgb_effect_basic_loop(rgb_effect_param_type *p_effect);
uint8_t rgb_effect_fade_inout(rgb_effect_param_type *p_effect);
uint8_t rgb_effect_fade_inout_buf(rgb_effect_param_type *p_effect);
uint8_t rgb_effect_fade_inout_loop(rgb_effect_param_type *p_effect);

const tiny_effector_func effectHandler[] = {
    rgb_effect_fade_inout,         // RGB_EFFECT_BOOTHID
    rgb_effect_basic,              // RGB_EFFECT_BASIC
    rgb_effect_basic_loop,         // RGB_EFFECT_BASIC_LOOP
    rgb_effect_fade_inout,         // RGB_EFFECT_FADE
    rgb_effect_fade_inout_buf,     // RGB_EFFECT_FADE_BUF
    rgb_effect_fade_inout_loop,    // RGB_EFFECT_FADE_LOOP
    rgb_effect_fade_inout,         // RGB_EFFECT_HEARTBEAT
    rgb_effect_fade_inout_buf,     // RGB_EFFECT_HEARTBEAT_BUF
    rgb_effect_fade_inout_loop,    // RGB_EFFECT_HEARTBEAT_LOOP
    rgb_effect_null,               // RGB_EFFECT_SWIPE
    rgb_effect_null,               // RGB_EFFECT_SWIPE_BUF
    rgb_effect_null,               // RGB_EFFECT_SWIPE_LOOP
};
#define EFFECT_HANDLER_TABLE_SIZE            (sizeof(effectHandler)/sizeof(tiny_effector_func))

void three_lock_state(uint8_t num, uint8_t caps, uint8_t scroll)
{
    NCSLock[0] = num;
    NCSLock[1] = caps;
    NCSLock[2] = scroll;
}

void three_lock_update(void)
{
    ws2812_sendarray(NCSLock , CLED_ELEMENT);
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

    for(i = 1; i < CLED_NUM; i++)
    {
        rgbBuffer[i][0] = g;
        rgbBuffer[i][1] = r;
        rgbBuffer[i][2] = b;
    }
    ws2812_sendarray((uint8_t *)rgbBuffer, rgbBufferLength);
}

void rgb_pos_on(uint8_t pos, uint8_t on, uint8_t r, uint8_t g, uint8_t b)
{
    rgb_array_clear();
    
    if(on == 0)
    {
        r = g = b = 0;
    }

    pos++;          // skip NCR indicator
    
    rgbBuffer[pos][0] = g;
    rgbBuffer[pos][1] = r;
    rgbBuffer[pos][2] = b;

    ws2812_sendarray((uint8_t *)rgbBuffer, rgbBufferLength);
}

uint8_t rgb_effect_null(rgb_effect_param_type *p_effect)
{
    return 0;
}

uint8_t rgb_effect_basic(rgb_effect_param_type *p_effect)
{
    uint8_t i = 0, j;

    tmprgbBuffer[i][0] = NCSLock[0];
    tmprgbBuffer[i][1] = NCSLock[1];
    tmprgbBuffer[i][2] = NCSLock[2];
    i++;

    for(i = 1; i < CLED_NUM; i++)
    {
        for(j = 0; j < 3; j++)
        {
            p_effect->level = rgbBuffer[i][j];
            if(p_effect->level < p_effect->max_rgb[j])
            {
                tmprgbBuffer[i][j] = p_effect->level;
            }
            else
            {
                tmprgbBuffer[i][j] = p_effect->max_rgb[j];
            }
        }
    }

    return 1;
}

uint8_t rgb_effect_basic_loop(rgb_effect_param_type *p_effect)
{
    uint8_t i = 0, j;
    uint8_t rgb[3];

    tmprgbBuffer[i][0] = NCSLock[0];
    tmprgbBuffer[i][1] = NCSLock[1];
    tmprgbBuffer[i][2] = NCSLock[2];
    i++;

    rgb[0] = rgbBuffer[p_effect->cnt + i][0];
    rgb[1] = rgbBuffer[p_effect->cnt + i][1];
    rgb[2] = rgbBuffer[p_effect->cnt + i][2];

    for(i = 1; i < CLED_NUM; i++)
    {
        for(j = 0; j < 3; j++)
        {
            p_effect->level = rgb[j];
            if(p_effect->level < p_effect->max_rgb[j])
            {
                tmprgbBuffer[i][j] = p_effect->level;
            }
            else
            {
                tmprgbBuffer[i][j] = p_effect->max_rgb[j];
            }
        }
    }

    if(++p_effect->hcnt == 0)
    {
        if(++p_effect->cnt == (CLED_NUM - 1))
        {
            p_effect->cnt = 0;
        }
    }

    return 1;
}

uint8_t rgb_effect_fade_inout(rgb_effect_param_type *p_effect)
{
    uint8_t i = 0, j;
    uint8_t max, limit;
    uint8_t rgb[3];

    tmprgbBuffer[i][0] = NCSLock[0];
    tmprgbBuffer[i][1] = NCSLock[1];
    tmprgbBuffer[i][2] = NCSLock[2];
    i++;

    rgb[0] = p_effect->max_rgb[0];
    rgb[1] = p_effect->max_rgb[1];
    rgb[2] = p_effect->max_rgb[2];

    max = 0;
    if(p_effect->dir == 0) // ASCENDING
    {
        for(i = 1; i < CLED_NUM; i++)
        {
            for(j = 0; j < 3; j++)
            {
                // read pixel level
                limit = rgb[j];
                // limit max level
                if(limit > p_effect->max_rgb[j])
                {
                    limit = p_effect->max_rgb[j];
                }

                // increase level according to mode
                if(p_effect->accel_mode == 0)
                {
                    if(tmprgbBuffer[i][j] < limit)
                    {
                        tmprgbBuffer[i][j]++;
                        max++;
                    }
                }
                else if(p_effect->accel_mode == 1)
                {
                    if(p_effect->level < 16)
                    {
                        tmprgbBuffer[i][j] = p_effect->level * p_effect->level + 1;
                        max++;
                    }
                    else
                    {
                        tmprgbBuffer[i][j] = 255;
                    }
                }
                else if(p_effect->accel_mode == 2)
                {
                    if(p_effect->level < 60)
                    {
                        tmprgbBuffer[i][j] = 4 * p_effect->level + 1;
                        max++;
                    }
                    else
                    {
                        tmprgbBuffer[i][j] = 255;
                    }
                }

                if(tmprgbBuffer[i][j] > limit)
                {
                    tmprgbBuffer[i][j] = limit;
                }
            }
        }

        // If no update go to the sustain state
        if(max == 0)
        {
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
        for(; i < CLED_NUM; i++)
        {
            for(j = 0; j < 3; j++)
            {
                if(tmprgbBuffer[i][j] > 0)
                {
                    tmprgbBuffer[i][j]--;
                    max++;
                }
                else
                {
                    tmprgbBuffer[i][j] = 0;
                }
            }
        }

        if(max == 0)
        {
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

uint8_t rgb_effect_fade_inout_buf(rgb_effect_param_type *p_effect)
{
    uint8_t i = 0, j;
    uint8_t max, limit;

    tmprgbBuffer[i][0] = NCSLock[0];
    tmprgbBuffer[i][1] = NCSLock[1];
    tmprgbBuffer[i][2] = NCSLock[2];
    i++;

    max = 0;
    if(p_effect->dir == 0) // ASCENDING
    {
        for(i = 1; i < CLED_NUM; i++)
        {
            for(j = 0; j < 3; j++)
            {
                // read pixel level
                limit = rgbBuffer[i][j];
                // limit max level
                if(limit > p_effect->max_rgb[j])
                {
                    limit = p_effect->max_rgb[j];
                }

                // increase level according to mode
                if(p_effect->accel_mode == 0)
                {
                    if(tmprgbBuffer[i][j] < limit)
                    {
                        tmprgbBuffer[i][j]++;
                        max++;
                    }
                }
                else if(p_effect->accel_mode == 1)
                {
                    if(p_effect->level < 16)
                    {
                        tmprgbBuffer[i][j] = p_effect->level * p_effect->level + 1;
                        max++;
                    }
                    else
                    {
                        tmprgbBuffer[i][j] = 255;
                    }
                }
                else if(p_effect->accel_mode == 2)
                {
                    if(p_effect->level < 60)
                    {
                        tmprgbBuffer[i][j] = 4 * p_effect->level + 1;
                        max++;
                    }
                    else
                    {
                        tmprgbBuffer[i][j] = 255;
                    }
                }

                if(tmprgbBuffer[i][j] > limit)
                {
                    tmprgbBuffer[i][j] = limit;
                }
            }
        }

        // If no update go to the sustain state
        if(max == 0)
        {
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
        for(; i < CLED_NUM; i++)
        {
            for(j = 0; j < 3; j++)
            {
                if(tmprgbBuffer[i][j] > 0)
                {
                    tmprgbBuffer[i][j]--;
                    max++;
                }
                else
                {
                    tmprgbBuffer[i][j] = 0;
                }
            }
        }

        if(max == 0)
        {
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

uint8_t rgb_effect_fade_inout_loop(rgb_effect_param_type *p_effect)
{
    uint8_t i = 0, j;
    uint8_t max, limit;
    uint8_t rgb[3];

    tmprgbBuffer[i][0] = NCSLock[0];
    tmprgbBuffer[i][1] = NCSLock[1];
    tmprgbBuffer[i][2] = NCSLock[2];
    i++;

    rgb[0] = rgbBuffer[p_effect->cnt + i][0];
    rgb[1] = rgbBuffer[p_effect->cnt + i][1];
    rgb[2] = rgbBuffer[p_effect->cnt + i][2];

    max = 0;
    if(p_effect->dir == 0) // ASCENDING
    {
        for(i = 1; i < CLED_NUM; i++)
        {
            for(j = 0; j < 3; j++)
            {
                // read pixel level
                limit = rgb[j];
                // limit max level
                if(limit > p_effect->max_rgb[j])
                {
                    limit = p_effect->max_rgb[j];
                }

                // increase level according to mode
                if(p_effect->accel_mode == 0)
                {
                    if(tmprgbBuffer[i][j] < limit)
                    {
                        tmprgbBuffer[i][j]++;
                        max++;
                    }
                }
                else if(p_effect->accel_mode == 1)
                {
                    if(p_effect->level < 16)
                    {
                        tmprgbBuffer[i][j] = p_effect->level * p_effect->level + 1;
                        max++;
                    }
                    else
                    {
                        tmprgbBuffer[i][j] = 255;
                    }
                }
                else if(p_effect->accel_mode == 2)
                {
                    if(p_effect->level < 60)
                    {
                        tmprgbBuffer[i][j] = 4 * p_effect->level + 1;
                        max++;
                    }
                    else
                    {
                        tmprgbBuffer[i][j] = 255;
                    }
                }

                if(tmprgbBuffer[i][j] > limit)
                {
                    tmprgbBuffer[i][j] = limit;
                }
            }
        }

        // If no update go to the sustain state
        if(max == 0)
        {
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
        for(; i < CLED_NUM; i++)
        {
            for(j = 0; j < 3; j++)
            {
                if(tmprgbBuffer[i][j] > 0)
                {
                    tmprgbBuffer[i][j]--;
                    max++;
                }
                else
                {
                    tmprgbBuffer[i][j] = 0;
                }
            }
        }

        if(max == 0)
        {
            if(p_effect->lcnt++ > p_effect->low_hold)
            {
                p_effect->level = 0;
                p_effect->dir = (!p_effect->dir);
                p_effect->lcnt = 0;

                if(++p_effect->cnt == (CLED_NUM - 1))
                {
                    p_effect->cnt = 0;
                }
            }
        }
    }

    return 1;
}
uint8_t handlecmd_config(tinycmd_pkt_req_type *p_req)
{
    tinycmd_config_req_type *p_config_req = (tinycmd_config_req_type *)p_req;

    tinyConfig.rgb_num = p_config_req->rgb_num;
    tinyConfig.led_preset_num = p_config_req->led_preset_num;

    ledmodeIndex = p_config_req->led_preset_index;
    rgbmodeIndex = p_config_req->rgb_preset_index;
    
    return 0;
}

uint8_t handlecmd_ver(tinycmd_pkt_req_type *p_req)
{
    volatile uint16_t i2cTimeout;
    tinycmd_ver_req_type *p_ver_req = (tinycmd_ver_req_type *)p_req;

    if((p_ver_req->cmd_code & TINY_CMD_RSP_MASK) != 0)
    {
         /*
         * To set response data, wait until i2c_reply_ready() returns nonzero,
         * then fill i2c_rdbuf with the data, finally call i2c_reply_done(n).
         * Interrupts are disabled while updating.
         */
        i2cTimeout = 0xFFFF;
        while((i2c_reply_ready() == 0) && i2cTimeout--);
        
        tinycmd_ver_rsp_type *p_ver_rsp = (tinycmd_ver_rsp_type *)i2c_rdbuf;

        p_ver_rsp->cmd_code = TINY_CMD_VER_F;
        p_ver_rsp->pkt_len = sizeof(tinycmd_ver_rsp_type);
        p_ver_rsp->version = 0xA5;
        i2c_reply_done(p_ver_rsp->pkt_len);

        // debug
        //rgb_array_on(TRUE, 100, 0, 100);
        
        //tinyConfig.comm_init = 1;
    }

    return 0;
}

uint8_t handlecmd_reset(tinycmd_pkt_req_type *p_req)
{
    return 0;
}

uint8_t handlecmd_three_lock(tinycmd_pkt_req_type *p_req)
{
    tinycmd_three_lock_req_type *p_three_lock_req = (tinycmd_three_lock_req_type *)p_req;

    tmprgbBuffer[0][0] = NCSLock[0] = 0;
    tmprgbBuffer[0][1] = NCSLock[1] = 0;
    tmprgbBuffer[0][2] = NCSLock[2] = 0;
    
    if(p_three_lock_req->lock & (1<<2))
    {
        tmprgbBuffer[0][0] = NCSLock[0] = 240;

    }
    if(p_three_lock_req->lock & (1<<1))
    {
        tmprgbBuffer[0][1] = NCSLock[2] = 240;
    }
    if(p_three_lock_req->lock & (1<<0))
    {
        tmprgbBuffer[0][2] = NCSLock[2] = 240;
    }

    ws2812_sendarray(NCSLock,CLED_ELEMENT);        // output message data to port D
    return 0;
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
    
    rgbBuffer[tmpPos][0] = p_rgb_pos_req->led.b;
    rgbBuffer[tmpPos][1] = p_rgb_pos_req->led.r;
    rgbBuffer[tmpPos][2] = p_rgb_pos_req->led.g;
        
    ws2812_sendarray((uint8_t *)rgbBuffer,rgbBufferLength);        // output message data to port D

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
        rgbBuffer[tmpPos+i][0] = pLed[i].b;
        rgbBuffer[tmpPos+i][1] = pLed[i].r;
        rgbBuffer[tmpPos+i][2] = pLed[i].g;
    }

    ws2812_sendarray((uint8_t *)rgbBuffer,rgbBufferLength);        // output message data to port D
    
    return 0;
}

uint8_t handlecmd_rgb_buffer(tinycmd_pkt_req_type *p_req)
{
    tinycmd_rgb_buffer_req_type *p_rgb_buffer_req = (tinycmd_rgb_buffer_req_type *)p_req;
    uint8_t tmpPos;

    // clear buffer
    rgb_array_clear();

    tmpPos = p_rgb_buffer_req->offset + 1;

    memcpy(rgbBuffer[tmpPos], p_rgb_buffer_req->data, p_rgb_buffer_req->num * CLED_ELEMENT);

    ws2812_sendarray((uint8_t *)rgbBuffer,rgbBufferLength);        // output message data to port D
    
    return 0;
}

uint8_t handlecmd_rgb_set_effect(tinycmd_pkt_req_type *p_req)
{
    tinycmd_rgb_set_effect_req_type *p_set_effect_req = (tinycmd_rgb_set_effect_req_type *)p_req;
    rgbmodeIndex = p_set_effect_req->index;
    rgbEffect = p_set_effect_req->effect_param;

    memset(&tmprgbBuffer, 0, CLED_ARRAY_SIZE);
    
    return 0;
}

uint8_t handlecmd_rgb_set_preset(tinycmd_pkt_req_type *p_req)
{
    tinycmd_rgb_set_preset_req_type *pset_preset_req = (tinycmd_rgb_set_preset_req_type *)p_req;

    memcpy(rgbBuffer[1], pset_preset_req->data, tinyConfig.rgb_num);

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

    ledmodeIndex = p_set_effect_req->preset;
    
    return 0;
}

uint8_t handlecmd_led_set_preset(tinycmd_pkt_req_type *p_req)
{
    tinycmd_led_set_preset_req_type *p_set_preset_req = (tinycmd_led_set_preset_req_type *)p_req;

    ledmode[p_set_preset_req->preset][p_set_preset_req->block] = p_set_preset_req->effect;
    
    return 0;
}

uint8_t handlecmd_led_config_preset(tinycmd_pkt_req_type *p_req)
{
    tinycmd_led_config_preset_req_type *p_cfg_preset_req = (tinycmd_led_config_preset_req_type *)p_req;
    volatile uint16_t i2cTimeout;
    LED_BLOCK ledblock;

    memcpy(ledmode, p_cfg_preset_req->data, sizeof(ledmode));

    if((p_cfg_preset_req->cmd_code & TINY_CMD_RSP_MASK) != 0)
    {
         /*
         * To set response data, wait until i2c_reply_ready() returns nonzero,
         * then fill i2c_rdbuf with the data, finally call i2c_reply_done(n).
         * Interrupts are disabled while updating.
         */
        i2cTimeout = 0xFFFF;
        while((i2c_reply_ready() == 0) && i2cTimeout--);
        
        tinycmd_rsp_type *p_gen_rsp = (tinycmd_rsp_type *)i2c_rdbuf;

        p_gen_rsp->cmd_code = TINY_CMD_VER_F;
        p_gen_rsp->pkt_len = sizeof(tinycmd_rsp_type);
        i2c_reply_done(p_gen_rsp->pkt_len);

        for (ledblock = LED_PIN_BASE; ledblock <= LED_PIN_WASD; ledblock++)
        {
            pwmDir[ledblock ] = 0;
            pwmCounter[ledblock] = 0;
            tiny_led_mode_change(ledblock, ledmode[ledmodeIndex][ledblock]);
        }

        tinyConfig.comm_init = 1;
    }

    return 0;
}

uint8_t handlecmd_dirty(tinycmd_pkt_req_type *p_req)
{
    if(p_req->cmd_code & TINY_CMD_KEY_MASK) // down
    {
        scanDirty |= SCAN_DIRTY;
    }
    else
    {
        scanDirty = 0;
    }
    
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

void tiny_led_blink(int matrixState)
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

void tiny_led_fader(void)
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

void tiny_rgb_effector(void)
{
    uint8_t update = 0;
    uint8_t index = rgbEffect.index;

    if(index < RGB_EFFECT_MAX)
    {
        if(effectHandler[index] != 0)
        {
            update = effectHandler[index](&rgbEffect);
        }
    }

    if(update)
    {
        ws2812_sendarray((uint8_t *)tmprgbBuffer, CLED_ARRAY_SIZE);
    }
}

void tiny_led_mode_change (LED_BLOCK ledblock, int mode)
{
    switch (mode)
    {
        case LED_EFFECT_FADING :
        case LED_EFFECT_FADING_PUSH_ON :
            break;
        case LED_EFFECT_PUSH_OFF :
        case LED_EFFECT_ALWAYS :
            tiny_led_on(ledblock);
            break;
        case LED_EFFECT_PUSH_ON :
        case LED_EFFECT_OFF :
        case LED_EFFECT_PUSHED_LEVEL :
        case LED_EFFECT_BASECAPS :
            tiny_led_off(ledblock);
            tiny_led_wave_set(ledblock,0);
            tiny_led_wave_on(ledblock);
            break;
        default :
            ledmode[ledmodeIndex][ledblock] = LED_EFFECT_FADING;
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
    timer1Init();
    timer1SetPrescaler(TIMER_CLK_DIV8);
    timer1PWMInit(0);

    // Enable PWM
    timer1PWMAOn();
    timer1PWMBOn();
}

void TinyInitCfg(void)
{
    tinyConfig.comm_init = 0;
    memset(rgbBuffer, 0, sizeof(CLED_NUM * CLED_ELEMENT));
    NCSLock = rgbBuffer[0];           // 0'st RGB is NCR indicator 
}

void TinyInitEffect(void)
{
    // init effect. firstly go to boot hid mode
    rgbmodeIndex = 0;
    memset(&rgbEffect, 0, sizeof(rgb_effect_param_type));
}

int main(void)
{
    uint8_t i = 0;
    volatile uint8_t *pTmp;
    uint8_t rcvlen;

    uint16_t count = 0;
    uint16_t effect_count = 0;

    TinyInitHW();
    TinyInitTimer();

    TinyInitCfg();
    TinyInitEffect();

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
            count++;
            effect_count++;
            if(tinyConfig.comm_init)
            {
                if(count%150 == 0)
                {
                    tiny_led_blink(scanDirty);
                    tiny_led_fader();
                }

                if(effect_count%777 == 0)
                {
                    tiny_rgb_effector();
                }
            }
            else
            {

                if(effect_count%1500 == 0)
                {
                    tiny_rgb_effector();
                }
            }
        }

    }

    return 0;

}





