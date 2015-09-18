#define KEYBD_EXTERN

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>     /* for _delay_ms() */

#include "timer.h"
#include "keysta.h"
#include "print.h"

#include "keymap.h"
#include "led.h"
#include "matrix.h"
#include "ps2main.h"

static uint8_t QUEUE[QUEUE_SIZE];
static int rear=0, front=0;

static uint8_t lastMAKE_keyidx;
static uint8_t lastMAKE[10];
static uint8_t lastMAKE_SIZE=0;
static uint8_t lastMAKE_IDX=0;
static long loopCnt;

unsigned char txScanCode = 0; // scancode being urrently transmitted
unsigned char m_state;
unsigned char lastSent;
unsigned char lastState;
static uint8_t TYPEMATIC_DELAY=2;
static long TYPEMATIC_REPEAT=10;

uint8_t checkExtended(uint8_t keyidx)
{
   uint8_t i;

	for(i=0; i< 46; i++)
	{
      if(keyidx == pgm_read_byte((uint16_t)(&keycode_set2_extend[i])))
         return 1;
	}
   return 0;
}

// Queue operation -> push, pop
void push(uint8_t item) {
	static uint8_t record=0;

	if(item==START_MAKE) {
		lastMAKE_SIZE=0;
		record=1;
		return;
	}
	if(item==END_MAKE) {
		record=0;
		return;
	}
	if(item==NO_REPEAT) {
		lastMAKE_SIZE=0;
		record=0;
		return;
	}

	if(record)
		lastMAKE[lastMAKE_SIZE++] = item;

    rear = (rear+1)%QUEUE_SIZE;
    if(front==rear) {
        rear = (rear!=0) ? (rear-1):(QUEUE_SIZE-1);
        return;
    }
    QUEUE[rear] = item;
}

uint8_t pop(void) {
    if(front==rear) {
        return 0;
    }
    front = (front+1)%QUEUE_SIZE;

    return QUEUE[front];
}

uint8_t isEmpty(void) {
	if(front==rear)
        return 1;
	else
		return 0;
}

void clear(void) {
	int i;
	rear = front = 0;
	lastMAKE_SIZE=0;
	lastMAKE_IDX=0;
	loopCnt=0;

	for(i=0;i<MATRIX_MAX_COL;i++)
		oldMATRIX[i] = 0x00;
}

void tx_state(unsigned char x, unsigned char newstate)
{
	if(x != 0xFE)
		lastSent=x;
	kbd_set_tx(x);
	m_state = newstate;
}



// push the keycodes into the queue by its key index, and isDown
void putKey(uint8_t keyidx, uint8_t isPushed)
{
	// if prev and current state are different,
	uint8_t keyVal = pgm_read_byte(&keycode_set2[keyidx]);

	/*if(isDown)
		DEBUG_PRINT(("[%02x] PUSHED\n", keyVal));
	else
		DEBUG_PRINT(("[%02x] RELEASED\n", keyVal));*/

	if(isPushed) {		// make code

		lastMAKE_keyidx = keyidx;
		loopCnt=0;
		m_state = STA_NORMAL;

		switch(keyidx)
      {
   		case K_PRNSCR:
   			push(START_MAKE);
   			push(0xE0);
   			push(0x12);
   			push(0xE0);
   			push(0x7C);
   			push(END_MAKE);
   			push(SPLIT); // SPLIT is for make sure all key codes are transmitted before disturbed by RX
   			break;
   		case K_PAUSE:
   			push(NO_REPEAT);
   			push(0xE1);
   			push(0x14);
   			push(0x77);
   			push(0xE1);
   			push(0xF0);
   			push(0x14);
   			push(0xF0);
   			push(0x77);
   			push(SPLIT);
   			break;
         default:
            push(START_MAKE);
            if(checkExtended(keyidx) == 1)
				   push(0xE0);
            push(keyVal);
            push(END_MAKE);
            push(SPLIT);
            break;
         
         }
      
	}else			// break code - key realeased
	{
		if(lastMAKE_keyidx == keyidx)		// repeat is resetted only if last make key is released
			lastMAKE_SIZE=0;

			switch(keyidx) 
         {
				case K_PRNSCR:
					push(0xE0);
					push(0xF0);
					push(0x7C);
					push(0xE0);
					push(0xF0);
					push(0x12);
					push(SPLIT);
					break;
       
            default:
               if(checkExtended(keyidx) == 1)
      			   push(0xE0);
      			push(0xF0);
      			push(keyVal);
      			push(SPLIT);
               break;
		   }
	}
}


int processRX(void)
{
    
    unsigned char rxed;
    int temp_a, temp_b;
    int i, j;
    
    rxed = kbd_get_rx_char();       

    switch(m_state) {
 
        case STA_RXCHAR:
            if (rxed == 0xF5)
                tx_state(0xFA, STA_NORMAL);
            else {
                tx_state(0xFA, STA_RXCHAR);
            }
            break;
    
        case STA_WAIT_SCAN_SET:
            clear();
            tx_state(0xFA, rxed == 0 ? STA_WAIT_SCAN_REPLY : STA_NORMAL);
            break;
        case STA_WAIT_AUTOREP:
            TYPEMATIC_DELAY = (rxed&0b01100000)/0b00100000;
    
            temp_a = (rxed&0b00000111);
            temp_b = (rxed&0b00011000)/(0b000001000);
    
            j=1;
            for(i=0;i<temp_b;i++) {
                j = j*2;
            }
    
            TYPEMATIC_REPEAT = temp_a*j;
    
            tx_state(0xFA, STA_NORMAL);
            break;
        case STA_WAIT_LEDS:
            // Reflect LED states to PD0~2
            // scroll lock
            if(rxed & 0x01){
                gLEDstate |= LED_SCROLL;
            }else{          
                gLEDstate &= ~LED_SCROLL;
            }
    
            // num lock
            if(rxed & 0x02){
                gLEDstate |= LED_NUM;
            }else{
                gLEDstate &= ~LED_NUM;
            }
            
            // capslock
            if(rxed & 0x04)
            {
                gLEDstate |= LED_CAPS;
            }
            else
            {
                gLEDstate &= ~LED_CAPS;
            }
    
            led_3lockupdate(gLEDstate);
            tx_state(0xFA, STA_NORMAL);
            break;

        default:
            switch(rxed) {
                case 0xEE: /* echo */
                    tx_state(0xEE, m_state);
                    break;
                case 0xF2: /* read id */
                    tx_state(0xFA, STA_WAIT_ID);
                    break;
                case 0xFF: /* reset */
                    tx_state(0xFA, STA_WAIT_RESET);
                    break;
                case 0xFE: /* resend */
                    tx_state(lastSent, m_state);
                    break;
                case 0xF0: /* scan code set */
                    tx_state(0xFA, STA_WAIT_SCAN_SET);
                    break;
                case 0xED: /* led indicators */     
                    tx_state(0xFA, STA_WAIT_LEDS);
                    break;
                case 0xF3:
                    tx_state(0xFA, STA_WAIT_AUTOREP);
                    break;
                case 0xF4:      // enable
                    tx_state(0xFA, STA_NORMAL);//STA_RXCHAR);
                    break;
                case 0xF5:      // disable
                    //clear();  // clear buffers
                    //tx_state(0xFA, STA_DISABLED);
                    tx_state(0xFA, STA_NORMAL);
                    break;
                case 0xF6:      // Set Default
                    TYPEMATIC_DELAY=1;
                    TYPEMATIC_REPEAT=5;
                    clear();
                default:
                    tx_state(0xFA, STA_NORMAL);
                    break;
            }
        break;
            
    }
    
    return 0;
}

int processTX(void)
{
    uint8_t keyval = 0;
    switch(m_state) {
    case STA_NORMAL:
    	// if error during send
    	if(isEmpty())
    		scankey();

    	keyval=pop();
    	if(keyval==SPLIT)
    		break;

    	if(keyval) {
    		tx_state(keyval, STA_NORMAL);

    		loopCnt=0;
    	}
    	else if(lastMAKE_SIZE>0) {		// means key is still pressed
    		loopCnt++;

    		// if key is pressed until typmatic_delay, goes to repeat the last key
    		if(loopCnt >= TYPEMATIC_DELAY*50+100) {
    			loopCnt=0;
    			lastMAKE_IDX=0;
    			m_state = STA_REPEAT;
    		}
    	}

    	break;
    // typematic : repeat last key
    case STA_REPEAT:
    	
    	if(lastMAKE_IDX==0)	{	// key state can be escaped only if whole key scancode is transmitted
    		scankey();
    	}

    	if(lastMAKE_SIZE==0 || !isEmpty()) {	// key is released. go to normal
    		m_state=STA_NORMAL;
    		loopCnt=0;
    		break;
    	}

    	// if release key is pushed, send them.
    	if(loopCnt==1 || lastMAKE_IDX!=0) {
    		tx_state(lastMAKE[lastMAKE_IDX++], STA_REPEAT);
    		lastMAKE_IDX %= lastMAKE_SIZE;
    	}
    	
    	loopCnt++;
    	loopCnt %= (3+TYPEMATIC_REPEAT*10);
    	
    	break;
    case STA_WAIT_SCAN_REPLY:
    	tx_state(0x02, STA_NORMAL);
    	break;
    case STA_WAIT_ID:
    	tx_state(0xAB, STA_WAIT_ID1);
    	break;
    case STA_WAIT_ID1:
    	tx_state(0x83, STA_NORMAL);
    	break;

    	_delay_ms(300);
    case STA_WAIT_RESET:
        clear();
//        led_check(0);
        led_mode_init();
        tx_state(0xAA, STA_NORMAL);
        break;
    }
    return keyval;
}



uint8_t ps2main(void)
{
   int keyval=0;
   m_state = STA_WAIT_RESET;
   cli();


   kbd_init_ps2();

   wdt_enable(WDTO_2S);
   sei();

   while(1)
   {
      wdt_reset();
      // check that every key code for single keys are transmitted
      if ((kbd_flags & FLA_RX_BYTE) && (keyval==SPLIT || isEmpty())) 
      {     // pokud nastaveny flag prijmu bytu, vezmi ho a zanalyzuj
         // pokud law, the flag setting apart, take it and zanalyzuj
         processRX();
      }
      if (kbd_flags & FLA_TX_OK)
      {   // pokud flag odesilani ok -> if the flag sent ok
         keyval = processTX();
      }
   }
   return 0;
}
