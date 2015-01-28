#include <stdio.h>

#include "global.h"
#include "tinycmd.h"
#include "tinycmdpkt.h"
#include "i2c.h"

extern uint8_t tinyExist;           // 1 : attiny85 is exist, 0 : not
extern unsigned char localBuffer[0x60];
extern unsigned char localBufferLength;

#define LOCAL_ADDR      0xA0
#define TARGET_ADDR     0xB0

#if 1//def SUPPORT_TINY_CMD
void tinycmd_ver(void)
{
    tinycmd_ver_req_type *p_ver_req = (tinycmd_ver_req_type *)localBuffer;
    
    p_ver_req->cmd_code = TINY_CMD_VER_F;
    p_ver_req->pkt_len = sizeof(tinycmd_ver_req_type);

    i2cMasterSend(TARGET_ADDR, p_ver_req->pkt_len, (uint8_t *)p_ver_req);
}

void tinycmd_reset(uint8_t type)
{
    tinycmd_reset_req_type *p_reset_req = (tinycmd_reset_req_type *)localBuffer;
    
    p_reset_req->cmd_code = TINY_CMD_RESET_F;
    p_reset_req->pkt_len = sizeof(tinycmd_reset_req_type);
    p_reset_req->type = TINY_RESET_HARD;

    i2cMasterSend(TARGET_ADDR, p_reset_req->pkt_len, (uint8_t *)p_reset_req);
}

void tinycmd_three_lock(uint8_t num, uint8_t caps, uint8_t scroll)
{
    uint8_t lock = 0;
    tinycmd_three_lock_req_type *p_three_lock_req = (tinycmd_three_lock_req_type *)localBuffer;
    
    p_three_lock_req->cmd_code = TINY_CMD_THREE_LOCK_F;
    p_three_lock_req->pkt_len = sizeof(tinycmd_three_lock_req_type);
    if(num)
    {
        lock |= (1<<2);
    }
    if(caps)
    {
        lock |= (1<<1);
    }
    if(scroll)
    {
        lock |= (1<<0);
    }
    p_three_lock_req->lock = lock;

    i2cMasterSend(TARGET_ADDR, p_three_lock_req->pkt_len, (uint8_t *)p_three_lock_req);
}

void tinycmd_bl_led_all(uint8_t on, uint8_t r, uint8_t g, uint8_t b)
{
#define MAX_LEVEL_MASK(a)               (a & 0x5F) // below 95
    uint8_t i;
    tinycmd_led_type led;
    tinycmd_bl_led_all_req_type *p_bl_led_all_req = (tinycmd_bl_led_all_req_type *)localBuffer;
    
    p_bl_led_all_req->cmd_code = TINY_CMD_BL_LED_ALL_F;
    p_bl_led_all_req->pkt_len = sizeof(tinycmd_bl_led_all_req_type);

    p_bl_led_all_req->on = on;
    
    p_bl_led_all_req->led.g = MAX_LEVEL_MASK(g);
    p_bl_led_all_req->led.r = MAX_LEVEL_MASK(r);
    p_bl_led_all_req->led.b = MAX_LEVEL_MASK(b);

    i2cMasterSend(TARGET_ADDR, p_bl_led_all_req->pkt_len, (uint8_t *)p_bl_led_all_req);
}

void tinycmd_bl_led_pos(uint8_t pos, uint8_t r, uint8_t g, uint8_t b)
{
#define MAX_LEVEL_MASK(a)               (a & 0x5F) // below 95
    uint8_t i;
    tinycmd_led_type led;
    tinycmd_bl_led_pos_req_type *p_bl_led_pos_req = (tinycmd_bl_led_pos_req_type *)localBuffer;
    
    p_bl_led_pos_req->cmd_code = TINY_CMD_BL_LED_POS_F;
    p_bl_led_pos_req->pkt_len = sizeof(tinycmd_bl_led_all_req_type);

    p_bl_led_pos_req->pos = pos;
    
    p_bl_led_pos_req->led.g = MAX_LEVEL_MASK(g);
    p_bl_led_pos_req->led.r = MAX_LEVEL_MASK(r);
    p_bl_led_pos_req->led.b = MAX_LEVEL_MASK(b);

    i2cMasterSend(TARGET_ADDR, p_bl_led_pos_req->pkt_len, (uint8_t *)p_bl_led_pos_req);
}

void tinycmd_bl_led_range(uint8_t num, uint8_t offset, uint8_t r, uint8_t g, uint8_t b)
{
#define MAX_LEVEL_MASK(a)               (a & 0x5F) // below 95
    uint8_t i;
    tinycmd_led_type led;
    tinycmd_bl_led_range_req_type *p_bl_led_range_req = (tinycmd_bl_led_range_req_type *)localBuffer;
    
    p_bl_led_range_req->cmd_code = TINY_CMD_BL_LED_RANGE_F;
    p_bl_led_range_req->pkt_len = sizeof(tinycmd_bl_led_range_req_type);

    p_bl_led_range_req->num = num;
    p_bl_led_range_req->offset = offset;
    
    led.g = MAX_LEVEL_MASK(g);
    led.r = MAX_LEVEL_MASK(r);
    led.b = MAX_LEVEL_MASK(b);

    for(i = 0; i < p_bl_led_range_req->num; i++)
    {
       p_bl_led_range_req->led[i] = led;
    }

    i2cMasterSend(TARGET_ADDR, p_bl_led_range_req->pkt_len, (uint8_t *)p_bl_led_range_req);
}

void tinycmd_pwm(uint8_t channel, uint8_t on, uint8_t duty)
{
    tinycmd_pwm_req_type *p_pwm_req = (tinycmd_pwm_req_type *)localBuffer;

    p_pwm_req->cmd_code = TINY_CMD_PWM_F;
    p_pwm_req->pkt_len = sizeof(tinycmd_pwm_req_type);
    p_pwm_req->channel = channel;
    p_pwm_req->enable = on;
    p_pwm_req->duty = duty;

    i2cMasterSend(TARGET_ADDR, p_pwm_req->pkt_len, (uint8_t *)p_pwm_req);
}

void tinycmd_api_set_bl_color(uint8_t index, uint8_t level)
{
    tinycmd_led_type led;
    uint8_t r = 0, g = 0, b = 0;
    if(index & 0x1)
    {
        r = level;
    }
    if(index & 0x2)
    {
        g = level;
    }
    if(index & 0x4)
    {
        b = level;
    }

    tinycmd_bl_led_all(1, r, g, b);
}

void tinycmd_set_led_mode(uint8_t storage, uint8_t block, uint8_t mode)
{
    tinycmd_set_led_mode_req_type *p_set_led_mode_req = (tinycmd_set_led_mode_req_type *)localBuffer;

    p_set_led_mode_req->cmd_code = TINY_CMD_SET_LED_MODE_F;
    p_set_led_mode_req->pkt_len = sizeof(tinycmd_set_led_mode_req_type);
    p_set_led_mode_req->storage = storage;
    p_set_led_mode_req->block = block;
    p_set_led_mode_req->mode = mode;

    i2cMasterSend(TARGET_ADDR, p_set_led_mode_req->pkt_len, (uint8_t *)p_set_led_mode_req);
}

#if 0
void testI2C(uint8_t count, uint8_t duty)
{
    switch(count % 8)
    {
    case 0:
        tinycmd_ver();
        break;
    case 1:
        tinycmd_pwm(TRUE, duty);
        break;
    case 2:
        //tinycmd_reset(TINY_RESET_SOFT);
        break;
    case 3:
        //tinycmd_three_lock(1, 1, 1);
        break;
    case 4:
        break;
    case 5:
        //tinycmd_led_on(0, 255, 255);
        break;
    case 6:
        tinycmd_pwm(FALSE, duty);
        break;
    case 7:
        break;
    case 8:
        //tinycmd_led_on(255, 255, 0);
        break;
    default:
        break;
    }
}
#endif

#endif // SUPPORT_TINY_CMD


