#include <stdio.h>
#include <string.h>

#include "global.h"
#include "tinycmd.h"
#include "tinycmdpkt.h"
#include "i2c.h"
#include "led.h"
#include "hwaddress.h"

extern uint8_t tinyExist;           // 1 : attiny85 is exist, 0 : not
extern unsigned char localBuffer[0x4B]; // I2C_WRSIZE
extern unsigned char localBufferLength;

#define TARGET_ADDR     0xB0

#if 1//def SUPPORT_TINY_CMD
static uint8_t waitResponse(uint8_t cmd)
{
    tinycmd_rsp_type *p_rsp = 0;
    uint8_t i, ret = 0;
    
    for(i = 0; i < 255; i++)
    {
        // then read n byte(s) from the selected MegaIO register
        i2cMasterReceiveNI(TARGET_ADDR, sizeof(tinycmd_rsp_type), (uint8_t*)localBuffer);

        p_rsp = (tinycmd_rsp_type *)localBuffer;
        if(p_rsp->cmd_code == cmd)
        {
            ret = 1;
            break;
        }
    }

    return ret;
}

uint8_t tinycmd_config(uint8_t rsp)
{
    tinycmd_config_req_type *p_cfg_req = (tinycmd_config_req_type *)localBuffer;
    uint8_t ret = 0;
    
    p_cfg_req->cmd_code = TINY_CMD_CONFIG_F;
    if(rsp)
    {
        p_cfg_req->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_cfg_req->pkt_len = sizeof(tinycmd_ver_req_type);

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_cfg_req->pkt_len, (uint8_t *)p_cfg_req);
        ret = waitResponse(TINY_CMD_CONFIG_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_cfg_req->pkt_len, (uint8_t *)p_cfg_req);
        ret = 1;
    }

    return ret;
}

uint8_t tinycmd_ver(uint8_t rsp)
{
    tinycmd_ver_req_type *p_ver_req = (tinycmd_ver_req_type *)localBuffer;
    uint8_t ret = 0;
    
    p_ver_req->cmd_code = TINY_CMD_VER_F;
    if(rsp)
    {
        p_ver_req->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_ver_req->pkt_len = sizeof(tinycmd_ver_req_type);

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_ver_req->pkt_len, (uint8_t *)p_ver_req);
        ret = waitResponse(TINY_CMD_VER_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_ver_req->pkt_len, (uint8_t *)p_ver_req);
        ret = 1;
    }

    return ret;
}

uint8_t tinycmd_reset(uint8_t type, uint8_t rsp)
{
    tinycmd_reset_req_type *p_reset_req = (tinycmd_reset_req_type *)localBuffer;
    uint8_t ret = 0;
    
    p_reset_req->cmd_code = TINY_CMD_RESET_F;
    if(rsp)
    {
        p_reset_req->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_reset_req->pkt_len = sizeof(tinycmd_reset_req_type);
    p_reset_req->type = 0;

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_reset_req->pkt_len, (uint8_t *)p_reset_req);
        ret = waitResponse(TINY_CMD_RESET_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_reset_req->pkt_len, (uint8_t *)p_reset_req);
        ret = 1;
    }

    return rsp;
}

uint8_t tinycmd_three_lock(uint8_t num, uint8_t caps, uint8_t scroll, uint8_t rsp)
{
    uint8_t lock = 0;
    tinycmd_three_lock_req_type *p_three_lock_req = (tinycmd_three_lock_req_type *)localBuffer;
    uint8_t ret = 0;
    
    p_three_lock_req->cmd_code = TINY_CMD_THREE_LOCK_F;
    if(rsp)
    {
        p_three_lock_req->cmd_code |= TINY_CMD_RSP_MASK;
    }
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

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_three_lock_req->pkt_len, (uint8_t *)p_three_lock_req);
        ret = waitResponse(TINY_CMD_THREE_LOCK_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_three_lock_req->pkt_len, (uint8_t *)p_three_lock_req);
        ret = 1;
    }

    return ret;
}

uint8_t tinycmd_dirty(uint8_t down)
{
    tinycmd_dirty_req_type *p_dirty_req = (tinycmd_dirty_req_type *)localBuffer;

    if(down)
    {
        p_dirty_req->cmd_code = TINY_CMD_DIRTY | TINY_CMD_KEY_MASK;
    }
    else
    {
        p_dirty_req->cmd_code = TINY_CMD_DIRTY;
    }

    i2cMasterSend(TARGET_ADDR, sizeof(tinycmd_dirty_req_type), (uint8_t *)p_dirty_req);

    return 1;
}

uint8_t tinycmd_rgb_all(uint8_t on, uint8_t r, uint8_t g, uint8_t b, uint8_t rsp)
{
    tinycmd_rgb_all_req_type *p_rgb_all_req = (tinycmd_rgb_all_req_type *)localBuffer;
    uint8_t ret = 0;
    
    p_rgb_all_req->cmd_code = TINY_CMD_RGB_ALL_F;
    if(rsp)
    {
        p_rgb_all_req->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_rgb_all_req->pkt_len = sizeof(tinycmd_rgb_all_req_type);

    p_rgb_all_req->on = on;
    
    p_rgb_all_req->led.g = g;
    p_rgb_all_req->led.r = r;
    p_rgb_all_req->led.b = b;

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_rgb_all_req->pkt_len, (uint8_t *)p_rgb_all_req);
        ret = waitResponse(TINY_CMD_RGB_ALL_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_rgb_all_req->pkt_len, (uint8_t *)p_rgb_all_req);
        ret = 1;
    }

    return ret;
}

uint8_t tinycmd_rgb_pos(uint8_t pos, uint8_t r, uint8_t g, uint8_t b, uint8_t rsp)
{
    tinycmd_rgb_pos_req_type *p_rgb_pos_req = (tinycmd_rgb_pos_req_type *)localBuffer;
    uint8_t ret = 0;
    
    p_rgb_pos_req->cmd_code = TINY_CMD_RGB_POS_F;
    if(rsp)
    {
        p_rgb_pos_req->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_rgb_pos_req->pkt_len = sizeof(tinycmd_rgb_pos_req_type);

    p_rgb_pos_req->pos = pos;
    
    p_rgb_pos_req->led.g = g;
    p_rgb_pos_req->led.r = r;
    p_rgb_pos_req->led.b = b;

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_rgb_pos_req->pkt_len, (uint8_t *)p_rgb_pos_req);
        ret = waitResponse(TINY_CMD_RGB_POS_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_rgb_pos_req->pkt_len, (uint8_t *)p_rgb_pos_req);
        ret = 1;
    }

    return ret;
}

uint8_t tinycmd_rgb_range(uint8_t num, uint8_t offset, uint8_t r, uint8_t g, uint8_t b, uint8_t rsp)
{
    tinycmd_led_type led;
    tinycmd_rgb_range_req_type *p_rgb_range_req = (tinycmd_rgb_range_req_type *)localBuffer;
    uint8_t i, ret = 0;

    //overflow
    if((offset + num) > TINYCMD_LED_MAX)
        return ret;
    
    p_rgb_range_req->cmd_code = TINY_CMD_RGB_RANGE_F;
    if(rsp)
    {
        p_rgb_range_req->cmd_code |= TINY_CMD_RSP_MASK;
    }
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

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_rgb_range_req->pkt_len, (uint8_t *)p_rgb_range_req);
        ret = waitResponse(TINY_CMD_RGB_RANGE_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_rgb_range_req->pkt_len, (uint8_t *)p_rgb_range_req);
        ret = 1;
    }

    return ret;
}

uint8_t tinycmd_rgb_buffer(uint8_t num, uint8_t offset, uint8_t *data, uint8_t rsp)
{
    tinycmd_rgb_buffer_req_type *p_rgb_buffer_req = (tinycmd_rgb_buffer_req_type *)localBuffer;
    uint8_t ret = 0;

    //overflow
    if((offset + num) > TINYCMD_LED_MAX)
        return ret;

    p_rgb_buffer_req->cmd_code = TINY_CMD_RGB_BUFFER_F;
    if(rsp)
    {
        p_rgb_buffer_req->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_rgb_buffer_req->pkt_len = sizeof(tinycmd_rgb_buffer_req_type);

    p_rgb_buffer_req->num = num;
    p_rgb_buffer_req->offset = offset;

    memcpy(p_rgb_buffer_req->data, data, num*3);

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_rgb_buffer_req->pkt_len, (uint8_t *)p_rgb_buffer_req);
        ret = waitResponse(TINY_CMD_RGB_BUFFER_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_rgb_buffer_req->pkt_len, (uint8_t *)p_rgb_buffer_req);
        ret = 1;
    }

    return ret;
}


uint8_t tinycmd_rgb_set_effect(uint8_t index, rgb_effect_param_type *p_param, uint8_t rsp)
{
    tinycmd_rgb_set_effect_req_type *p_rgb_set_effect = (tinycmd_rgb_set_effect_req_type *)localBuffer;
    uint8_t ret = 0;

    p_rgb_set_effect->cmd_code = TINY_CMD_RGB_SET_EFFECT_F;
    if(rsp)
    {
        p_rgb_set_effect->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_rgb_set_effect->pkt_len = sizeof(tinycmd_rgb_set_effect_req_type);
    p_rgb_set_effect->index = index;
    //p_rgb_set_effect->effect_param = *p_param;
    memcpy(&p_rgb_set_effect->effect_param, p_param, sizeof(rgb_effect_param_type));

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_rgb_set_effect->pkt_len, (uint8_t *)p_rgb_set_effect);
        ret = waitResponse(TINY_CMD_RGB_SET_EFFECT_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_rgb_set_effect->pkt_len, (uint8_t *)p_rgb_set_effect);
        ret = 1;
    }

    return ret;
}

uint8_t tinycmd_rgb_set_preset(uint8_t preset, uint8_t effect, uint8_t rsp)
{
    tinycmd_rgb_set_preset_req_type *p_rgb_set_preset = (tinycmd_rgb_set_preset_req_type *)localBuffer;
    uint8_t ret = 0;

    p_rgb_set_preset->cmd_code = TINY_CMD_RGB_SET_PRESET_F;
    if(rsp)
    {
        p_rgb_set_preset->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_rgb_set_preset->pkt_len = sizeof(tinycmd_rgb_set_preset_req_type);
    p_rgb_set_preset->preset = preset;
    
    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_rgb_set_preset->pkt_len, (uint8_t *)p_rgb_set_preset);
        ret = waitResponse(TINY_CMD_RGB_SET_PRESET_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_rgb_set_preset->pkt_len, (uint8_t *)p_rgb_set_preset);
        ret = 1;
    }

    return ret;
}

uint8_t tinycmd_led_level(uint8_t channel, uint8_t level, uint8_t rsp)
{
    tinycmd_led_level_req_type *p_led_level_req = (tinycmd_led_level_req_type *)localBuffer;
    uint8_t ret = 0;

    p_led_level_req->cmd_code = TINY_CMD_LED_LEVEL_F;
    if(rsp)
    {
        p_led_level_req->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_led_level_req->pkt_len = sizeof(tinycmd_led_level_req_type);

    p_led_level_req->channel = channel;
    p_led_level_req->level = level;

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_led_level_req->pkt_len, (uint8_t *)p_led_level_req);
        ret = waitResponse(TINY_CMD_LED_LEVEL_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_led_level_req->pkt_len, (uint8_t *)p_led_level_req);
        ret = 1;
    }

    return ret;
}

uint8_t tinycmd_led_set_effect(uint8_t index, uint8_t rsp)
{
    tinycmd_led_set_effect_req_type *p_rgb_set_effect = (tinycmd_led_set_effect_req_type *)localBuffer;
    uint8_t ret = 0;

    p_rgb_set_effect->cmd_code = TINY_CMD_LED_SET_EFFECT_F;
    if(rsp)
    {
        p_rgb_set_effect->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_rgb_set_effect->pkt_len = sizeof(tinycmd_led_set_effect_req_type);
    p_rgb_set_effect->preset = index;

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_rgb_set_effect->pkt_len, (uint8_t *)p_rgb_set_effect);
        ret = waitResponse(TINY_CMD_LED_SET_EFFECT_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_rgb_set_effect->pkt_len, (uint8_t *)p_rgb_set_effect);
        ret = 1;
    }

    return ret;
}

uint8_t tinycmd_led_set_preset(uint8_t preset, uint8_t block, uint8_t effect, uint8_t rsp)
{
    tinycmd_led_set_preset_req_type *p_led_set_preset_mode = (tinycmd_led_set_preset_req_type *)localBuffer;
    uint8_t ret = 0;

    p_led_set_preset_mode->cmd_code = TINY_CMD_LED_SET_PRESET_F;
    if(rsp)
    {
        p_led_set_preset_mode->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_led_set_preset_mode->pkt_len = sizeof(tinycmd_led_set_preset_req_type);
    p_led_set_preset_mode->preset = preset;
    p_led_set_preset_mode->block = block;
    p_led_set_preset_mode->effect = effect;

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_led_set_preset_mode->pkt_len, (uint8_t *)p_led_set_preset_mode);
        ret = waitResponse(TINY_CMD_LED_SET_PRESET_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_led_set_preset_mode->pkt_len, (uint8_t *)p_led_set_preset_mode);
        ret = 1;
    }

    return ret;
}

uint8_t tinycmd_led_preset_config(uint8_t *p_led_mode_array, uint8_t rsp)
{
    tinycmd_led_config_preset_req_type *p_led_cfg_preset = (tinycmd_led_config_preset_req_type *)localBuffer;
    uint8_t i, ret = 0;

    p_led_cfg_preset->cmd_code = TINY_CMD_LED_CONFIG_PRESET_F;
    if(rsp)
    {
        p_led_cfg_preset->cmd_code |= TINY_CMD_RSP_MASK;
    }
    p_led_cfg_preset->pkt_len = sizeof(tinycmd_led_config_preset_req_type);

    for(i = 0; i < LEDMODE_ARRAY_SIZE; i++)
    {
        p_led_cfg_preset->data[i] = p_led_mode_array[i];
    }

    // If need to wait response from the slave
    if(rsp)
    {
        i2cMasterSendNI(TARGET_ADDR, p_led_cfg_preset->pkt_len, (uint8_t *)p_led_cfg_preset);
        ret = waitResponse(TINY_CMD_LED_CONFIG_PRESET_F);
    }
    else
    {
        i2cMasterSend(TARGET_ADDR, p_led_cfg_preset->pkt_len, (uint8_t *)p_led_cfg_preset);
        ret = 1;
    }

    return ret;
}

#endif // SUPPORT_TINY_CMD


