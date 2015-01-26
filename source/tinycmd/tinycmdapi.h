#ifndef TINYCMDAPI_H
#define TINYCMDAPI_H

#include "tinycmd.h"
#include "tinycmdpkt.h"

void tinycmd_ver(void);
void tinycmd_reset(uint8_t type);
void tinycmd_three_lock(uint8_t num, uint8_t caps, uint8_t scroll);
void tinycmd_bl_led_all(uint8_t on, uint8_t r, uint8_t g, uint8_t b);
void tinycmd_bl_led_pos(uint8_t pos, uint8_t r, uint8_t g, uint8_t b);
void tinycmd_bl_led_range(uint8_t num, uint8_t offset, uint8_t r, uint8_t g, uint8_t b);
void tinycmd_pwm(uint8_t channel, uint8_t on, uint8_t duty);

void tinycmd_api_set_bl_color(int8_t index, uint8_t level);

#endif // TINYCMDAPI_H

