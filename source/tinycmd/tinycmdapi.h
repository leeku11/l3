#ifndef TINYCMDAPI_H
#define TINYCMDAPI_H

#include "tinycmd.h"
#include "tinycmdpkt.h"

void tinycmd_ver(uint8_t rsp);
void tinycmd_reset(uint8_t type);
void tinycmd_three_lock(uint8_t num, uint8_t caps, uint8_t scroll);
void tinycmd_rgb_all(uint8_t on, uint8_t r, uint8_t g, uint8_t b);
void tinycmd_rgb_pos(uint8_t pos, uint8_t r, uint8_t g, uint8_t b);
void tinycmd_rgb_range(uint8_t num, uint8_t offset, uint8_t r, uint8_t g, uint8_t b);
void tinycmd_rgb_buffer(uint8_t num, uint8_t offset, tinycmd_led_type *led);

void tinycmd_rgb_set_effect(uint8_t index);
void tinycmd_rgb_set_preset(uint8_t preset, uint8_t mode);
void tinycmd_led_level(uint8_t channel, uint8_t level);
void tinycmd_led_set_effect(uint8_t index);
void tinycmd_led_set_preset(uint8_t preset, uint8_t block, uint8_t mode);
void tinycmd_led_config_preset(uint8_t *p_led_mode_array);


#endif // TINYCMDAPI_H

