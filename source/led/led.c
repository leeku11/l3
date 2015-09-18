#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>     /* for _delay_ms() */


#include "led.h"
#include "global.h"
#include "timer.h"
#include "keymap.h"
#include "matrix.h"
#include "hwaddress.h"
#include "macro.h"
#include "Tinycmdapi.h"

#undef LED_CONTROL_MASTER
#define LED_CONTROL_SLAVE

#define PWM_OFF                 0
#define PWM_ON                  1

#define PWM_CHANNEL_0           0
#define PWM_CHANNEL_1           1

static uint8_t *const ledport[] = {LED_NUM_PORT, LED_CAP_PORT,LED_SCR_PORT};
    
static uint8_t const ledpin[] = {LED_NUM_PIN, LED_CAP_PIN, LED_SCR_PIN};


#ifdef LED_CONTROL_MASTER    

uint8_t ledmode[LEDMODE_INDEX_MAX][LED_BLOCK_MAX] = {
    { 0, 0, 0, LED_EFFECT_FADING, LED_EFFECT_FADING },
    { 0, 0, 0, LED_EFFECT_PUSH_ON, LED_EFFECT_PUSH_ON },
    { 0, 0, 0, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS },
};



static uint8_t speed[LED_BLOCK_MAX] = {0, 0, 0, 5, 5};
static uint8_t brigspeed[LED_BLOCK_MAX] = {0, 0, 0, 3, 3};
static uint8_t pwmDir[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};
static uint16_t pwmCounter[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};

static uint16_t pushedLevelStay[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};
static uint8_t pushedLevel[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};
static uint16_t pushedLevelDuty[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};

#endif


void led_off(LED_BLOCK block)
{
    switch(block)
    {
        case LED_PIN_NUMLOCK:
        case LED_PIN_CAPSLOCK:
        case LED_PIN_SCROLLOCK:
            *(ledport[block]) |= BV(ledpin[block]);
        break;
        case LED_PIN_BASE:
            //tinycmd_led_level(PWM_CHANNEL_0, PWM_DUTY_MIN);
            break;
        case LED_PIN_WASD:
            //tinycmd_led_level(PWM_CHANNEL_1, PWM_DUTY_MIN);
            break;                    
        default:
            return;
    }
}

void led_on(LED_BLOCK block)
{
    switch(block)
    {
        case LED_PIN_NUMLOCK:
        case LED_PIN_CAPSLOCK:
        case LED_PIN_SCROLLOCK: 
            *(ledport[block]) &= ~BV(ledpin[block]);
            break;
        case LED_PIN_BASE:
            break;
        case LED_PIN_WASD:
            break;
        default:
            return;
    }
}


void led_wave_on(LED_BLOCK block)
{
}

void led_wave_off(LED_BLOCK block)
{
}




void led_wave_set(LED_BLOCK block, uint16_t duty)
{
}




void led_blink(int matrixState)
{
    if(tinyExist)
    {
        static int matrixStateOld = 0;

        if(matrixStateOld != matrixState)
        {
            matrixStateOld = matrixState;
            tinycmd_dirty(matrixState & SCAN_DIRTY);
        }
    }
}

void led_fader(void)
{
}

void led_check(uint8_t forward)
{
    led_on(LED_PIN_NUMLOCK);
    _delay_ms(100);
    led_on(LED_PIN_SCROLLOCK);
    _delay_ms(100);
    led_on(LED_PIN_NUMLOCK);
    _delay_ms(100);
}

uint8_t tmpToggle;
void led_3lockupdate(uint8_t LEDstate)
{
    if (tinyExist)
    {
        tinycmd_three_lock((LEDstate & LED_NUM), (LEDstate & LED_CAPS), (LEDstate & LED_SCROLL), FALSE);
    }
    else
    {
        if (LEDstate & LED_NUM)
        { // light up caps lock
            led_on(LED_PIN_NUMLOCK);
        }
        else
        {
            led_off(LED_PIN_NUMLOCK);
        }
        if (LEDstate & LED_CAPS)
        { // light up caps lock
            led_on(LED_PIN_CAPSLOCK);
        }
        else
        {
            led_off(LED_PIN_CAPSLOCK);
            if (LEDstate & LED_SCROLL)
            { // light up caps lock
                led_on(LED_PIN_SCROLLOCK);
            }
            else
            {
                led_off(LED_PIN_SCROLLOCK);
            }
        }
    }
}


void led_mode_init(void)
{
    led_3lockupdate(gLEDstate);
}

uint8_t led_sleep_preset[3][5] ={{LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF}, 
                                 {LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF}, 
                                 {LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF}};
void led_sleep(void)
{
    led_3lockupdate(0);
    _delay_ms(1);
    tinycmd_led_config_preset((uint8_t*)led_sleep_preset, TRUE);
    _delay_ms(1);
    // RGB Effect
    tinycmd_rgb_all(0, 0, 0, 0, TRUE);
}

void led_restore(void)
{
    led_3lockupdate(gLEDstate);
    _delay_ms(1);
    tinycmd_rgb_buffer(MAX_RGB_CHAIN, 0, (uint8_t *)kbdConf.rgb_preset, TRUE);
    _delay_ms(1);
    tinycmd_rgb_effect_speed(kbdConf.rgb_speed, TRUE);
    _delay_ms(1);
    tinycmd_rgb_set_preset(kbdConf.rgb_effect_index, (rgb_effect_param_type *)&kbdConf.rgb_effect_param[kbdConf.rgb_effect_index], TRUE);
    _delay_ms(1);
    tinycmd_rgb_set_effect(kbdConf.rgb_effect_index, TRUE);
    // RGB Effect
    tinycmd_config(kbdConf.rgb_chain + 1, kbdConf.rgb_limit, TRUE);
    _delay_ms(1);
    // LED Effect
    tinycmd_led_set_effect(kbdConf.led_preset_index, TRUE);
    _delay_ms(1);
    tinycmd_led_config_preset((uint8_t*)kbdConf.led_preset, TRUE);
    _delay_ms(1);
}
