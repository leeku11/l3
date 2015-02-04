#include <stdio.h>

#include "global.h"
#include "tinycmd.h"
#include "tinycmdpkt.h"
#include "i2c.h"
#include "led.h"

extern uint8_t tinyExist;           // 1 : attiny85 is exist, 0 : not
extern unsigned char localBuffer[0x50];
extern unsigned char localBufferLength;

#define TARGET_ADDR     0xB0

#if 1//def SUPPORT_TINY_CMD
void tinycmd_ver(uint8_t rsp)
{
    tinycmd_ver_req_type *p_ver_req = (tinycmd_ver_req_type *)localBuffer;
    
    p_ver_req->cmd_code = TINY_CMD_VER_F;
    if(rsp)
    {
        p_ver_req->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_ver_req->pkt_len = sizeof(tinycmd_ver_req_type);


    i2cMasterSendNI(TARGET_ADDR, p_ver_req->pkt_len, (uint8_t *)p_ver_req);

    // If need to wait response from the slave
    if(rsp)
    {
    	// then read n byte(s) from the selected MegaIO register
    	i2cMasterReceiveNI(TARGET_ADDR, sizeof(tinycmd_ver_rsp_type), (uint8_t*)localBuffer);
    }
}

void tinycmd_reset(uint8_t type)
{
    tinycmd_reset_req_type *p_reset_req = (tinycmd_reset_req_type *)localBuffer;
    
    p_reset_req->cmd_code = TINY_CMD_RESET_F;
    p_reset_req->pkt_len = sizeof(tinycmd_reset_req_type);
    p_reset_req->type = TINY_RESET_HARD;

    i2cMasterSendNI(TARGET_ADDR, p_reset_req->pkt_len, (uint8_t *)p_reset_req);
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

    i2cMasterSendNI(TARGET_ADDR, p_three_lock_req->pkt_len, (uint8_t *)p_three_lock_req);
}

void tinycmd_rgb_all(uint8_t on, uint8_t r, uint8_t g, uint8_t b)
{
    tinycmd_rgb_all_req_type *p_rgb_all_req = (tinycmd_rgb_all_req_type *)localBuffer;
    
    p_rgb_all_req->cmd_code = TINY_CMD_RGB_ALL_F;
    p_rgb_all_req->pkt_len = sizeof(tinycmd_rgb_all_req_type);

    p_rgb_all_req->on = on;
    
    p_rgb_all_req->led.g = g;
    p_rgb_all_req->led.r = r;
    p_rgb_all_req->led.b = b;

    i2cMasterSendNI(TARGET_ADDR, p_rgb_all_req->pkt_len, (uint8_t *)p_rgb_all_req);
}

void tinycmd_rgb_pos(uint8_t pos, uint8_t r, uint8_t g, uint8_t b)
{
    tinycmd_rgb_pos_req_type *p_rgb_pos_req = (tinycmd_rgb_pos_req_type *)localBuffer;
    
    p_rgb_pos_req->cmd_code = TINY_CMD_RGB_POS_F;
    p_rgb_pos_req->pkt_len = sizeof(tinycmd_rgb_pos_req_type);

    p_rgb_pos_req->pos = pos;
    
    p_rgb_pos_req->led.g = g;
    p_rgb_pos_req->led.r = r;
    p_rgb_pos_req->led.b = b;

    i2cMasterSendNI(TARGET_ADDR, p_rgb_pos_req->pkt_len, (uint8_t *)p_rgb_pos_req);
}

void tinycmd_rgb_range(uint8_t num, uint8_t offset, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t i;
    tinycmd_led_type led;
    tinycmd_rgb_range_req_type *p_rgb_range_req = (tinycmd_rgb_range_req_type *)localBuffer;
    
    p_rgb_range_req->cmd_code = TINY_CMD_RGB_RANGE_F;
    p_rgb_range_req->pkt_len = sizeof(tinycmd_rgb_range_req_type);

    p_rgb_range_req->num = num;
    p_rgb_range_req->offset = offset;
    
    led.g = g;
    led.r = r;
    led.b = b;

    for(i = 0; i < p_rgb_range_req->num; i++)
    {
       p_rgb_range_req->led[i] = led;
    }

    i2cMasterSendNI(TARGET_ADDR, p_rgb_range_req->pkt_len, (uint8_t *)p_rgb_range_req);
}

void tinycmd_rgb_set_effect(uint8_t index)
{
    tinycmd_rgb_set_effect_req_type *p_rgb_set_effect = (tinycmd_rgb_set_effect_req_type *)localBuffer;
    p_rgb_set_effect->cmd_code = TINY_CMD_RGB_SET_EFFECT_F;
    p_rgb_set_effect->pkt_len = sizeof(tinycmd_rgb_set_effect_req_type);
    p_rgb_set_effect->preset = index;

    i2cMasterSendNI(TARGET_ADDR, p_rgb_set_effect->pkt_len, (uint8_t *)p_rgb_set_effect);
}

void tinycmd_rgb_set_preset(uint8_t preset, uint8_t effect)
{
    tinycmd_rgb_set_preset_req_type *p_rgb_set_preset = (tinycmd_rgb_set_preset_req_type *)localBuffer;
    p_rgb_set_preset->cmd_code = TINY_CMD_RGB_SET_PRESET_F;
    p_rgb_set_preset->pkt_len = sizeof(tinycmd_rgb_set_preset_req_type);
    p_rgb_set_preset->preset = preset;
    
}

void tinycmd_led_level(uint8_t channel, uint8_t level)
{
    tinycmd_led_level_req_type *p_led_level_req = (tinycmd_led_level_req_type *)localBuffer;

    p_led_level_req->cmd_code = TINY_CMD_LED_LEVEL_F;
    p_led_level_req->pkt_len = sizeof(tinycmd_led_level_req_type);

    p_led_level_req->channel = channel;
    p_led_level_req->level = level;

    i2cMasterSendNI(TARGET_ADDR, p_led_level_req->pkt_len, (uint8_t *)p_led_level_req);
}

void tinycmd_led_set_effect(uint8_t index)
{
    tinycmd_led_set_effect_req_type *p_rgb_set_effect = (tinycmd_led_set_effect_req_type *)localBuffer;
    p_rgb_set_effect->cmd_code = TINY_CMD_LED_SET_EFFECT_F;
    p_rgb_set_effect->pkt_len = sizeof(tinycmd_led_set_effect_req_type);
    p_rgb_set_effect->preset = index;

    i2cMasterSendNI(TARGET_ADDR, p_rgb_set_effect->pkt_len, (uint8_t *)p_rgb_set_effect);
}

void tinycmd_led_set_preset(uint8_t preset, uint8_t block, uint8_t effect)
{
    tinycmd_led_set_preset_req_type *p_led_set_preset_mode = (tinycmd_led_set_preset_req_type *)localBuffer;

    p_led_set_preset_mode->cmd_code = TINY_CMD_LED_SET_PRESET_F;
    p_led_set_preset_mode->pkt_len = sizeof(tinycmd_led_set_preset_req_type);
    p_led_set_preset_mode->preset = preset;
    p_led_set_preset_mode->block = block;
    p_led_set_preset_mode->effect = effect;

    i2cMasterSendNI(TARGET_ADDR, p_led_set_preset_mode->pkt_len, (uint8_t *)p_led_set_preset_mode);
}

void tinycmd_led_preset_config(uint8_t *p_led_mode_array)
{
    uint8_t i;
    tinycmd_led_config_preset_req_type *p_led_cfg_preset = (tinycmd_led_config_preset_req_type *)localBuffer;

    p_led_cfg_preset->cmd_code = TINY_CMD_LED_CONFIG_PRESET_F | TINY_CMD_RSP_MASK;
    p_led_cfg_preset->pkt_len = sizeof(tinycmd_led_config_preset_req_type);

    for(i = 0; i < LEDMODE_ARRAY_SIZE; i++)
    {
        p_led_cfg_preset->data[i] = p_led_mode_array[i];
    }

    i2cMasterSendNI(TARGET_ADDR, p_led_cfg_preset->pkt_len, (uint8_t *)p_led_cfg_preset);

    i2cMasterReceiveNI(TARGET_ADDR, sizeof(tinycmd_ver_rsp_type), (uint8_t*)localBuffer);
}

#endif // SUPPORT_TINY_CMD


