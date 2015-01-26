#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>     /* for _delay_ms() */


#include "led.h"
#include "global.h"
#include "timer128.h"
#include "keymap.h"
#include "matrix.h"
#include "hwaddress.h"
#include "macro.h"

#define PUSHED_LEVEL_MAX        20


static uint8_t *const ledport[] = {LED_NUM_PORT, LED_CAP_PORT,LED_SCR_PORT, LED_PRT_PORT, 
                                    LED_ESC_PORT,LED_Fx_PORT,LED_PAD_PORT, LED_BASE_PORT, 
                                    LED_WASD_PORT,LED_ARROW18_PORT, LED_VESEL_PORT};
    
static uint8_t const ledpin[] = {LED_NUM_PIN, LED_CAP_PIN, LED_SCR_PIN, LED_PRT_PIN, 
                                    LED_ESC_PIN,LED_Fx_PIN,LED_PAD_PIN,LED_BASE_PIN, 
                                    LED_WASD_PIN,LED_ARROW18_PIN, LED_VESEL_PIN};
uint8_t ledmodeIndex;

#define LEDMODE_ARRAY_SIZE 5*11
uint8_t ledmode[5][11] ={ 
        {LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, 
        LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, 
        LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS},
        
        {LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, 
        LED_EFFECT_ALWAYS, LED_EFFECT_FADING, LED_EFFECT_FADING, LED_EFFECT_FADING, 
        LED_EFFECT_FADING, LED_EFFECT_FADING, LED_EFFECT_FADING},
        
        {LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, 
        LED_EFFECT_ALWAYS, LED_EFFECT_FADING_PUSH_ON, LED_EFFECT_FADING_PUSH_ON, LED_EFFECT_FADING_PUSH_ON, 
        LED_EFFECT_FADING_PUSH_ON, LED_EFFECT_FADING_PUSH_ON, LED_EFFECT_FADING_PUSH_ON},
        
        {LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS, 
        LED_EFFECT_ALWAYS, LED_EFFECT_PUSHED_LEVEL, LED_EFFECT_PUSHED_LEVEL, LED_EFFECT_PUSHED_LEVEL, 
        LED_EFFECT_PUSHED_LEVEL, LED_EFFECT_PUSHED_LEVEL, LED_EFFECT_PUSHED_LEVEL},
        
        {LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, 
        LED_EFFECT_FADING, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, 
        LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF}
};


static uint8_t speed[] = {1, 1, 1, 1, 5, 5, 5, 5, 5, 5, 5};
static uint8_t brigspeed[] = {1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3};
static uint8_t pwmDir[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint16_t pwmCounter[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static int16_t pushedLevelStay[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t pushedLevel[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint16_t pushedLevelDuty[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t LEDstate;     ///< current state of the LEDs

extern uint16_t scankeycntms;



void led_off(LED_BLOCK block)
{
    uint8_t i;
    switch(block)
    {
        case LED_PIN_NUMLOCK:
        case LED_PIN_CAPSLOCK:
        case LED_PIN_SCROLLOCK:
        case LED_PIN_PRT:

        case LED_PIN_ESC:
        case LED_PIN_Fx:
        case LED_PIN_BASE:
        case LED_PIN_WASD:
        case LED_PIN_VESEL:
            break;                    

        case LED_PIN_PAD:
#ifdef KBDMOD_M5            
            if(!isLED3000)
                *(ledport[LED_PIN_VESEL]) &= ~(BV(ledpin[LED_PIN_VESEL]));
#endif
            break;
        case LED_PIN_ARROW18:
#ifdef KBDMOD_M5
            if(isLED3000)
                *(ledport[LED_PIN_VESEL]) &= ~(BV(ledpin[LED_PIN_VESEL]));
#endif
            *(ledport[block]) &= ~(BV(ledpin[block]));
            break;
            
        case LED_PIN_ALL:
            for (i = 0; i < LED_PIN_ALL; i++)
            {
                *(ledport[i]) &= ~(BV(ledpin[i]));
            }
            return;
        default:
            return;
    }
    
    *(ledport[block]) &= ~(BV(ledpin[block]));
}

void led_on(LED_BLOCK block)
{
    
    uint8_t i;
    switch(block)
    {
        case LED_PIN_NUMLOCK:
        case LED_PIN_CAPSLOCK:
        case LED_PIN_SCROLLOCK:
        case LED_PIN_PRT:

        case LED_PIN_ESC:
        case LED_PIN_Fx:
        case LED_PIN_BASE:
        case LED_PIN_WASD:
        case LED_PIN_VESEL:
            break;                    

        case LED_PIN_PAD:
#ifdef KBDMOD_M5            
            if(!isLED3000)
                *(ledport[LED_PIN_VESEL]) |= BV(ledpin[LED_PIN_VESEL]);
#endif            
            break;
        case LED_PIN_ARROW18:
#ifdef KBDMOD_M5            
            if(isLED3000)
                *(ledport[LED_PIN_VESEL]) |= BV(ledpin[LED_PIN_VESEL]);
#endif
            break;
        case LED_PIN_ALL:
            for (i = 0; i < LED_PIN_ALL; i++)
            {
                *(ledport[i]) |= BV(ledpin[i]);
            }

            return;
        default:
            return;
    }
    led_wave_off(block);
    *(ledport[block]) |= BV(ledpin[block]);
}


void led_wave_on(LED_BLOCK block)
{
    switch(block)
    {
        case LED_PIN_ESC:
            timer3PWMCOn();
            break;
        case LED_PIN_Fx:
            timer3PWMAOn();
            break;
        case LED_PIN_PAD:
            timer0PWMOn();
#ifdef KBDMOD_M5            
            if(!isLED3000)
                timer1PWMAOn();
#endif
            break;
        case LED_PIN_BASE:
            timer3PWMBOn();
            break;
        case LED_PIN_WASD:
            timer1PWMCOn();
            break;
        case LED_PIN_ARROW18:
            timer1PWMBOn();
#ifdef KBDMOD_M5            
            if(isLED3000)
                timer1PWMAOn();
#endif
            break;
        case LED_PIN_VESEL:
            timer1PWMAOn();
            break;
            
        case LED_PIN_ALL:
            timer3PWMCOn();
            timer3PWMAOn();
            timer3PWMBOn();
            timer1PWMCOn();
            timer1PWMBOn();
            timer1PWMAOn();
            timer0PWMOn();
            break;
        default:
            break;
    }
}

void led_wave_off(LED_BLOCK block)
{
    switch(block)
    {
        case LED_PIN_ESC:
            timer3PWMCOff();
            break;
        case LED_PIN_Fx:
            timer3PWMAOff();
            break;
        case LED_PIN_PAD:
            timer0PWMOff();
            
#ifdef KBDMOD_M5            
            if(!isLED3000)
                timer1PWMAOff();
#endif
            break;
        case LED_PIN_BASE:
            timer3PWMBOff();
            break;
        case LED_PIN_WASD:
            timer1PWMCOff();
            break;
        case LED_PIN_ARROW18:
            timer1PWMBOff();
#ifdef KBDMOD_M5            
            if(isLED3000)
                timer1PWMAOff();
#endif
            break;

        case LED_PIN_VESEL:
            timer1PWMAOff();
            break;

        case LED_PIN_ALL:
            timer3PWMCOff();
            timer3PWMAOff();
            timer3PWMBOff();
            timer1PWMCOff();
            timer1PWMBOff();
            timer1PWMAOff();
            timer0PWMOff();
            break;
        default:
            break;
    }
}




void led_wave_set(LED_BLOCK block, uint16_t duty)
{
    switch(block)
    {
        case LED_PIN_ESC:
            timer3PWMCSet(duty);
            break;
        case LED_PIN_Fx:
            timer3PWMASet(duty);
            break;
        case LED_PIN_PAD:
            timer0PWMSet(duty);
#ifdef KBDMOD_M5            
            if(!isLED3000)
                timer1PWMASet(duty);
#endif
            break;
        case LED_PIN_BASE:
            timer3PWMBSet(duty);
            break;
        case LED_PIN_WASD:
            timer1PWMCSet(duty);
            break;
        case LED_PIN_ARROW18:
            timer1PWMBSet(duty);
#ifdef KBDMOD_M5            
            if(isLED3000)
                timer1PWMASet(duty);
#endif
            break;
        case LED_PIN_VESEL:
            timer1PWMASet(duty);
            break;
            
        case LED_PIN_ALL:
            timer3PWMCSet(duty);
            timer3PWMASet(duty);
            timer3PWMBSet(duty);
            timer1PWMCSet(duty);
            timer1PWMBSet(duty);
            timer1PWMASet(duty);
            timer0PWMSet(duty);
            break;
        default:
            break;
    }



}




void led_blink(int matrixState)
{
    LED_BLOCK ledblock;

    for (ledblock = LED_PIN_Fx; ledblock<LED_PIN_VESEL; ledblock++)
    {
        
        if(matrixState & SCAN_DIRTY)      // 1 or more key is pushed
        {
            switch(ledmode[ledmodeIndex][ledblock])
            {

                case LED_EFFECT_FADING_PUSH_ON:
                case LED_EFFECT_PUSH_ON:
                    led_on(ledblock);
                    break;
                case LED_EFFECT_PUSH_OFF:
                    led_wave_off(ledblock);
                    led_wave_set(ledblock, 0);
                    led_off(ledblock);
                    break;
                default :
                    break;
            }             
        }else{          // none of keys is pushed
            switch(ledmode[ledmodeIndex][ledblock])
                 {
                     case LED_EFFECT_FADING_PUSH_ON:
                     case LED_EFFECT_PUSH_ON:
                        led_off(ledblock);
                        break;
                     case LED_EFFECT_PUSH_OFF:
                        led_on(ledblock);
                        break;
                     default :
                         break;
                 }
        }
    }
}

void led_fader(void)
{
    
    uint8_t ledblock;
    for (ledblock = LED_PIN_ESC; ledblock < LED_PIN_VESEL; ledblock++)
    {
        if((ledmode[ledmodeIndex][ledblock] == LED_EFFECT_FADING) || ((ledmode[ledmodeIndex][ledblock] == LED_EFFECT_FADING_PUSH_ON) && (scankeycntms > 1000)))
        {
            if(pwmDir[ledblock]==0)
            {
                led_wave_set(ledblock, ((uint16_t)(pwmCounter[ledblock]/brigspeed[ledblock])));        // brighter
                if(pwmCounter[ledblock]>=255*brigspeed[ledblock])
                    pwmDir[ledblock] = 1;
                    
            }
            else if(pwmDir[ledblock]==2)
            {
                led_wave_set(ledblock, ((uint16_t)(255-pwmCounter[ledblock]/speed[ledblock])));    // darker
                if(pwmCounter[ledblock]>=255*speed[ledblock])
                    pwmDir[ledblock] = 3;

            }
            else if(pwmDir[ledblock]==1)
            {
                if(pwmCounter[ledblock]>=255*speed[ledblock])
                   {
                       pwmCounter[ledblock] = 0;
                       pwmDir[ledblock] = 2;
                   }
            }else if(pwmDir[ledblock]==3)
            {
                if(pwmCounter[ledblock]>=255*brigspeed[ledblock])
                   {
                       pwmCounter[ledblock] = 0;
                       pwmDir[ledblock] = 0;
                   }
            }


            led_wave_on(ledblock);

            // pwmDir 0~3 : idle
       
            pwmCounter[ledblock]++;

        }else if (ledmode[ledmodeIndex][ledblock] == LED_EFFECT_PUSHED_LEVEL)
        {
    		// 일정시간 유지

    		if(pushedLevelStay[ledblock] > 0){
    			pushedLevelStay[ledblock]--;
    		}else{
    			// 시간이 흐르면 레벨을 감소 시킨다.
    			if(pushedLevelDuty[ledblock] > 0){
    				pwmCounter[ledblock]++;
    				if(pwmCounter[ledblock] >= speed[ledblock]){
    					pwmCounter[ledblock] = 0;			
    					pushedLevelDuty[ledblock]--;
    					pushedLevel[ledblock] = PUSHED_LEVEL_MAX - (255-pushedLevelDuty[ledblock]) / (255/PUSHED_LEVEL_MAX);
    					/*if(pushedLevel_prev != pushedLevel){
    						DEBUG_PRINT(("---------------------------------decrease pushedLevel : %d, life : %d\n", pushedLevel, pushedLevelDuty));
    						pushedLevel_prev = pushedLevel;
    					}*/
    				}
    			}else{
    				pushedLevel[ledblock] = 0;
    				pwmCounter[ledblock] = 0;
    			}
    		}
    		led_wave_set(ledblock, pushedLevelDuty[ledblock]);

    	}else
        {
            led_wave_set(ledblock, 0);
            led_wave_off(ledblock);
            pwmCounter[ledblock]=0;
            pwmDir[ledblock]=0;
        }
    }
}

void led_check(uint8_t forward)
{
    led_off(LED_PIN_ALL);
    if(forward){
        led_on(LED_PIN_PRT);
    }else{
        led_on(LED_PIN_NUMLOCK);
    }
    _delay_ms(100);
    led_off(LED_PIN_ALL);
    led_on(LED_PIN_SCROLLOCK);

    _delay_ms(100);
    led_off(LED_PIN_ALL);
    if(forward){
        led_on(LED_PIN_NUMLOCK);
    }else{
        led_on(LED_PIN_PRT);
    }
    _delay_ms(100);
    led_off(LED_PIN_ALL);
}


void led_3lockupdate(uint8_t LEDstate)
{
    uint8_t ledblock;
        if (LEDstate & LED_NUM) { // light up caps lock
            led_on(LED_PIN_NUMLOCK);
        } else {
            led_off(LED_PIN_NUMLOCK);
        }
        if (LEDstate & LED_CAPS) { // light up caps lock
            led_on(LED_PIN_CAPSLOCK);
            for(ledblock = LED_PIN_Fx; ledblock <= LED_PIN_ARROW18; ledblock++)
            {
                if (ledmode[ledmodeIndex][ledblock] == LED_EFFECT_BASECAPS)
                    led_on(ledblock);
            }
        } else {
            led_off(LED_PIN_CAPSLOCK);
            for(ledblock = LED_PIN_Fx; ledblock <= LED_PIN_ARROW18; ledblock++)
            {
                if (ledmode[ledmodeIndex][ledblock] == LED_EFFECT_BASECAPS)
                    led_off(ledblock);
            }
        }
        if (LEDstate & LED_SCROLL) { // light up caps lock
            led_on(LED_PIN_SCROLLOCK);
        } else {
            led_off(LED_PIN_SCROLLOCK);
        }
}

#define LED_INDICATOR_MAXTIME 90
#define LED_INDICATOR_MAXINDEX 16

uint8_t ledESCIndicator[6][LED_INDICATOR_MAXINDEX] = {
   {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}
};

uint8_t ledPRTIndicator[6][LED_INDICATOR_MAXINDEX] = {
   {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
   {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
   {1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};


uint8_t ledESCIndicatorTimer;
uint8_t ledESCIndicatorIndex;

uint8_t ledPRTIndicatorTimer;
uint8_t ledPRTIndicatorIndex;


void led_ESCIndicater(uint8_t layer)
{
    if(ledESCIndicatorTimer++ >= LED_INDICATOR_MAXTIME)
    {
        if(ledESCIndicator[layer][ledESCIndicatorIndex] == 1)
        {
            led_on(LED_PIN_ESC);
        }else
        {
            led_off(LED_PIN_ESC);
        }
        if (++ledESCIndicatorIndex >= LED_INDICATOR_MAXINDEX)
        {
            ledESCIndicatorIndex = 0;
        }
        ledESCIndicatorTimer = 0;
    }
}

void led_PRTIndicater(uint8_t index)
{
    if(ledPRTIndicatorTimer++ >= LED_INDICATOR_MAXTIME)
    {
        if(ledPRTIndicator[index][ledPRTIndicatorIndex] == 1)
        {
            led_on(LED_PIN_PRT);
        }else
        {
            led_off(LED_PIN_PRT);
        }
        if (++ledPRTIndicatorIndex >= LED_INDICATOR_MAXINDEX)
        {
            ledPRTIndicatorIndex = 0;
        }
        ledPRTIndicatorTimer = 0;
    }
}



void led_mode_init(void)
{
    LED_BLOCK ledblock;
    int16_t i;
    uint8_t *buf;
    ledmodeIndex = eeprom_read_byte(EEPADDR_LED_STATUS); 
    if (ledmodeIndex > 4)
        ledmodeIndex = 0;
    buf = ledmode;
    for (i = 0; i < LEDMODE_ARRAY_SIZE; i++)
    {
//        *buf++ = pgm_read_byte_far(LEDMODE_ADDRESS+i);
        *buf++ = eeprom_read_byte(EEPADDR_LED_MODE+i);
    }
    for (ledblock = LED_PIN_ESC; ledblock < LED_PIN_VESEL; ledblock++)
    {
      pwmDir[ledblock ] = 0;
      pwmCounter[ledblock] = 0;
        led_mode_change(ledblock, ledmode[ledmodeIndex][ledblock]);
    }
}

void led_mode_change (LED_BLOCK ledblock, int mode)
{
    switch (mode)
    {
        case LED_EFFECT_FADING :
        case LED_EFFECT_FADING_PUSH_ON :
            break;
        case LED_EFFECT_PUSH_OFF :
        case LED_EFFECT_ALWAYS :
            led_on(ledblock);
            break;
        case LED_EFFECT_PUSH_ON :
        case LED_EFFECT_OFF :
        case LED_EFFECT_PUSHED_LEVEL :
        case LED_EFFECT_BASECAPS :
            led_off(ledblock);
            led_wave_set(ledblock,0);
            led_wave_on(ledblock);
            break;
        default :
            ledmode[ledmodeIndex][ledblock] = LED_EFFECT_FADING;
            break;
     }
}

void led_mode_save(void)
{
    eeprom_write_byte(EEPADDR_LED_STATUS, ledmodeIndex);
}

void led_pushed_level_cal(void)
{
    LED_BLOCK ledblock;
	// update pushed level
	
    for (ledblock = LED_PIN_ESC; ledblock < LED_PIN_ALL; ledblock++)
    { 
        if(pushedLevel[ledblock] < PUSHED_LEVEL_MAX)
        {
            pushedLevelStay[ledblock] = 511;
            pushedLevel[ledblock]++;
            pushedLevelDuty[ledblock] = (255 * pushedLevel[ledblock]) / PUSHED_LEVEL_MAX;
        }
	}
}

uint8_t ledstart[] = "LED record mode - push any key@";
uint8_t ledend[] = "LED record done@";
uint8_t sledmode[8][15] = {"fading@", "fading-pushon@", "pushed-weight@","pushon@", "pushoff@", "always@", "caps@", "off@"}; 
uint8_t sledblk[5][7] = {"Fx----", "Pad---", "Base--", "WASD--", "Arrow-"};

void recordLED(uint8_t ledkey)
{
    ledmodeIndex = ledkey - K_LED0;
    int8_t col, row;
    uint32_t prev, cur;
    uint8_t prevBit, curBit;
    uint8_t keyidx;
    uint8_t matrixState = 0;
    uint8_t retVal = 0;
    int8_t i;
    int16_t index;
    long page;
    uint8_t ledblk;
    long address;
    index = 0;
    page = 0;


    wdt_reset();



    sendString(ledstart);

    for(col = 0; col < MAX_COL; col++)
    {
        for(row = 0; row < MAX_ROW; row++)
        {
            debounceMATRIX[col][row] = -1;
        }
    }

    while(1)
    {

    wdt_reset();
    matrixState = scanmatrix();

    /* LED Blinker */
    led_blink(matrixState);

    /* LED Fader */
    led_fader();
    


    // debounce cleared => compare last matrix and current matrix
    for(col = 0; col < MAX_COL; col++)
    {
     prev = MATRIX[col];
     cur  = curMATRIX[col];
     MATRIX[col] = curMATRIX[col];
     for(i = 0; i < MAX_ROW; i++)
     {
        prevBit = (uint8_t)prev & 0x01;
        curBit = (uint8_t)cur & 0x01;
        prev >>= 1;
        cur >>= 1;

#ifdef KBDMOD_M5
        if (i < 8)
        {
            row = 10+i;
        }else if (i < 16)
        {
            row = -6+i;
        }else
        {
            row = -16+i;
        }
        
#else //KBDMOD_M7
         row = i;
#endif

        keyidx = pgm_read_byte(keymap[6]+(col*MAX_ROW)+row);

         if (keyidx == K_NONE)
            continue;

         if (!prevBit && curBit)   //pushed
         {
            debounceMATRIX[col][row] = 0;    //triger

         }else if (prevBit && !curBit)  //released
         {
            debounceMATRIX[col][row] = 0;    //triger
         }

         if(debounceMATRIX[col][row] >= 0)
         {                
            if(debounceMATRIX[col][row]++ >= DEBOUNCE_MAX)
            {
               if(curBit)
               {
                    if (keyidx == K_FN)
                    {
                        flash_writeinpage(ledmode, LEDMODE_ADDRESS);
                        led_mode_save();

                        wdt_reset();
                        sendString("===========================@");

                        for (ledblk = LED_PIN_Fx; ledblk < LED_PIN_VESEL; ledblk++)
                        {
                           wdt_reset();
                           sendString(sledblk[ledblk-5]);
                           sendString(sledmode[ledmode[ledmodeIndex][ledblk]]);
                        }
                        sendString("===========================@");
                        wdt_reset();

                        sendString(ledend);
                        return;
                    }else
                    {
                        ledblk = keyidx - K_LFX + 5;
                        
                        if(++ledmode[ledmodeIndex][ledblk] >= LED_EFFECT_NONE)
                        {
                            ledmode[ledmodeIndex][ledblk] = LED_EFFECT_FADING;
                        }
                        
                        wdt_reset();
                        sendString(sledblk[ledblk-5]);
                        
                        wdt_reset();
                        sendString(sledmode[ledmode[ledmodeIndex][ledblk]]);

                         for (ledblk = LED_PIN_Fx; ledblk < LED_PIN_VESEL; ledblk++)
                         {
                              pwmDir[ledblk ] = 0;
                              pwmCounter[ledblk] = 0;
                             led_mode_change(ledblk, ledmode[ledmodeIndex][ledblk]);
                         }                    
                     }
               }else
               {
                
               }

               debounceMATRIX[col][row] = -1;
            }
        }
    }
    }
    }
}
