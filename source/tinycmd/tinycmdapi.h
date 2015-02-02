#ifndef TINYCMDAPI_H
#define TINYCMDAPI_H

#include "tinycmd.h"
#include "tinycmdpkt.h"

void tinycmd_ver(uint8_t rsp);
void tinycmd_reset(uint8_t type);
void tinycmd_three_lock(uint8_t num, uint8_t caps, uint8_t scroll);
void tinycmd_bl_led_all(uint8_t on, uint8_t r, uint8_t g, uint8_t b);
void tinycmd_bl_led_pos(uint8_t pos, uint8_t r, uint8_t g, uint8_t b);
void tinycmd_bl_led_range(uint8_t num, uint8_t offset, uint8_t r, uint8_t g, uint8_t b);
void tinycmd_key_led_ch_mode(uint8_t mode);
void tinycmd_key_led_level(uint8_t channel, uint8_t level);

void tinycmd_api_set_bl_color(int8_t index, uint8_t level);

void tinycmd_set_led_mode(uint8_t storage, uint8_t block, uint8_t mode);
void tinycmd_config_led_mode(uint8_t *p_led_mode_array);

#endif // TINYCMDAPI_H

