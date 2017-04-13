#define KEYBD_EXTERN

#include "global.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include <util/delay.h>     /* for _delay_ms() */

#include <stdio.h>
#include <avr/wdt.h>

#include "keysta.h"
#include "keymap.h"
#include "usbdrv.h"
#include "usbmain.h"
#include "ps2main.h"
#include "matrix.h"
#include "macro.h"
#include "led.h"
#include "hwaddress.h"




uint8_t macrobuffer[256] = {};

const char PROGMEM macrostart[] = "MACRO REC@";
const char PROGMEM macroend[] = "@DONE@";
const char PROGMEM mresetstart[] = "RESET";
const char PROGMEM macroresetdone[]  = "@DONE@";
uint8_t specialKey[28][3] = 
    {{K_C,K_A,K_P},
    {K_F,K_1,K_NONE},
    {K_F,K_2,K_NONE},
    {K_F,K_3,K_NONE},
    {K_F,K_4,K_NONE},
    {K_F,K_5,K_NONE},
    {K_F,K_6,K_NONE},
    {K_F,K_7,K_NONE},
    {K_F,K_8,K_NONE},
    {K_F,K_9,K_NONE},
    {K_F,K_1,K_0},
    {K_F,K_1,K_1},
    {K_F,K_1,K_2},
    {K_P,K_R,K_N},
    {K_S,K_C,K_L},
    {K_P,K_A,K_U},
    {K_I,K_N,K_S},
    {K_H,K_O,K_M},
    {K_P,K_G,K_U},
    {K_D,K_E,K_L},
    {K_E,K_N,K_D},
    {K_P,K_G,K_D},
    {K_R,K_I,K_T},
    {K_L,K_F,K_T},
    {K_D,K_N,K_NONE},
    {K_U,K_P,K_NONE},
    {K_N,K_U,K_M},
    {K_E,K_S,K_C}
};


extern MODIFIERS modifierBitmap[];


/**
 * Send a single report to the computer. This function is not used during
 * normal typing, it is only used to send non-pressed keys to simulate input.
 * \param mode modifier-byte
 * \param key key-code
 */
void usbSendReport(uint8_t mode, uint8_t key) {
    // buffer for HID reports. we use a private one, so nobody gets disturbed
    uint8_t repBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    repBuffer[0] = mode;
    repBuffer[2] = key;
    while (!usbInterruptIsReady())
    {
        usbPoll();
        _delay_ms(1);
    }; // wait
    usbSetInterrupt(repBuffer, sizeof(repBuffer)); // send
}


/**
 * Convert an ASCII-character to the corresponding key-code and modifier-code
 * combination.
 * \parm character ASCII-character to convert
 * \return structure containing the combination
 */
Key charToKey(char character) {
    Key key;
    // initialize with reserved values
    key.mode = MOD_NONE;
    key.key = K_NONE;
    if ((character >= 'a') && (character <= 'z')) {
        // a..z
        key.key = (character - 'a') + 0x04;
    } else if ((character >= 'A') && (character <= 'Z')) {
        // A..Z
        key.mode = MOD_SHIFT_LEFT;
        key.key = (character - 'A') + 0x04;
    } else if ((character >= '1') && (character <= '9')) {
        // 1..9
        key.key = (character - '1') + 0x1E;
    }
    // we can't map the other characters directly, so we switch...
    switch (character) {
        case '0':
            key.key = K_0; break;
        case '!':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_1; break;
        /*
        case '@':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_2; break;
        case '#':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_3; break;
        */
        case '$':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_4; break;
        case '%':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_5; break;
        case '^':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_6; break;
        case '&':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_7; break;
        case '*':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_8; break;
        case '(':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_9; break;
        case ')':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_0; break;
        case ' ':
            key.key = K_SPACE; break;
        case '-':
            key.key = K_MINUS; break;
        case '_':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_MINUS; break;
        case '=':
            key.key = K_EQUAL; break;
        case '+':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_EQUAL; break;
        case '[':
            key.key = K_LBR; break;
        case '{':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_LBR; break;
        case ']':
            key.key = K_RBR; break;
        case '}':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_RBR; break;
        case '\\':
            key.key = K_BKSLASH; break;
        case '|':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_BKSLASH; break;
        case '#':
            key.key = K_HASH; break;
        case '@':
            //key.mode = MOD_SHIFT_LEFT;
            key.key = K_ENTER; break;
        case ';':
            key.key = K_COLON; break;
        case ':':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_COLON; break;
#if 0
        case '\'':
            key.key = K_apostroph; break;
        case '"':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_apostroph; break;
#endif            
        case '`':
            key.key = K_HASH; break;
        case '~':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_HASH; break;
        case ',':
            key.key = K_COMMA; break;
        case '<':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_COMMA; break;
        case '.':
            key.key = K_DOT; break;
        case '>':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_DOT; break;
        case '/':
            key.key = K_SLASH; break;
        case '?':
            key.mode = MOD_SHIFT_LEFT;
            key.key = K_SLASH; break;
    }
    if (key.key == K_NONE) {
        // still reserved? WTF? return question mark...
        key.mode = MOD_SHIFT_LEFT;
        key.key = K_SLASH;
    }
    return key;
} 

void clearKey(void)
{
    if(kbdConf.ps2usb_mode)
    {
        usbSendReport(0, 0);
    }
}


/**
 * Send a key to the computer, followed by the release of all keys. This can be
 * used repetitively to send a string.
 * \param keytosend key structure to send
 */
void sendKey(Key keytosend) 
{
    uint8_t keyval = 0;
    if(kbdConf.ps2usb_mode)
    {
        usbSendReport(keytosend.mode, keytosend.key);
    }else
    {
        putKey(keytosend.key,1);
        while((keyval = pop()) !=SPLIT)
        {
            while(!(kbd_flags & FLA_TX_OK));
            _delay_us(10);
            tx_state(keyval, STA_NORMAL);
        }
        putKey(keytosend.key,0);
        while((keyval = pop()) !=SPLIT)
        {
            while(!(kbd_flags & FLA_TX_OK));
            _delay_us(10);
            tx_state(keyval, STA_NORMAL);
        }
    }
}


void printModifier(uint8_t keytosend, uint8_t open)
{
   Key prtKey;
   
   prtKey.mode = 0;
   if(open)
   {
      prtKey.key = K_LBR;
   }else
   {
      
      prtKey.mode = MOD_SHIFT_LEFT;
      prtKey.key = K_BKSLASH;
   }
   sendKey(prtKey);

   wdt_reset();
   
   prtKey.mode = MOD_SHIFT_LEFT;
   switch(keytosend)
   {
      case K_LCTRL:
      case K_RCTRL:
      {
         prtKey.key = K_C;
         break;
      }
      case K_LSHIFT:
      case K_RSHIFT:
      {
         prtKey.key = K_S;
         break;

      }
      case K_LALT:
      case K_RALT:
      {
            
         prtKey.key = K_A;
         break;
      }
      case K_LGUI:
      case K_RGUI:
      {
         prtKey.key = K_W;
         break;
      }
   }
   
   sendKey(prtKey);

   wdt_reset();
   if(!open)
   {
      prtKey.key = K_RBR;
   }else
   {
      prtKey.mode = MOD_SHIFT_LEFT;
      prtKey.key = K_BKSLASH;
   }
   sendKey(prtKey);
   clearKey();
}

void printSpecialKey(uint8_t keytosend)
{
    Key prtKey;
    uint8_t index;
    uint8_t i;
    index = keytosend - K_CAPS;
    if(keytosend == K_ESC)
        index = 27;             // for "ESC" abnormal case.
        
    prtKey.mode = 0;
    prtKey.key = K_LBR;
    sendKey(prtKey);

    for (i = 0; i < 3; i++)
    {
        wdt_reset();
        prtKey.key = specialKey[index][i];
        sendKey(prtKey);
    }

    prtKey.mode = 0;
    prtKey.key = K_RBR;
    sendKey(prtKey);
    wdt_reset();
    clearKey();
}


/**
 * Send a string to the computer. This function converts each character of an
 * ASCII-string to a key-structure and uses sendKey() to send it.
 * \param string string to send
 */
void sendString(uint16_t pString) {
    uint8_t i = 0;
    uint8_t keychar, oldkeychar = 0;
    Key key;
    while((keychar = pgm_read_byte(pString))!= 0 && i < 32) // limit to 64 charater to send at once.
    {
        key = charToKey(keychar);

        if(keychar == oldkeychar)
        {
            clearKey();
        }
        oldkeychar = keychar;
        sendKey(key);
        pString++;
        i++;
    }
    
    clearKey();
}

void sendMatrix(char row, char col)
{
    Key a;

	 a.mode = 0;
    a.key = K_LBR;
    sendKey(a);
	
    a.key = K_A + row;
    sendKey(a);

    a.key = K_MINUS;
    sendKey(a);

    a.key = K_A + col;
    sendKey(a);

    a.key = K_RBR;
    sendKey(a);

    clearKey();
}


void playMacroUSB(uint8_t macrokey)
{
    uint8_t i;
    uint8_t keyidx, oldKeyidx = 0;

    uint8_t mIndex = 0;
    uint8_t esctoggle = 0;
    uint8_t macroSET;
    uint16_t address;


    Key key;
    
    key.mode = 0;
    key.key = 0;

    mIndex = macrokey - K_M01;
    address = MACRO_ADDR_START + (0x100 * mIndex);
    if(address >= (0x7000 - 0x100))
    {
        return;
    }

    macroSET = eeprom_read_byte(EEPADDR_MACRO_SET+mIndex);
    if (macroSET != EEPVAL_MACRO_BIT)      // MACRO not recorded
    {
        return;
    }
    keyidx = pgm_read_byte(address++);
    for (i = 0; i < MAX_MACRO_LEN; i++)
    {
        if((K_Modifiers < keyidx) && (keyidx < K_Modifiers_end))
        {
            key.mode ^= modifierBitmap[keyidx -K_Modifiers];
            keyidx = pgm_read_byte(address++);
        }
        while(((keyidx < K_Modifiers) || (K_Modifiers_end < keyidx)) && keyidx != K_NONE && keyidx < K_M01)
        {
            if(esctoggle++ == 4)
            {
//                led_on(LED_PIN_ESC);
                esctoggle = 0;
            }else{
//                led_off(LED_PIN_ESC);
            }
        
            wdt_reset();
            if(keyidx == oldKeyidx)
            {
                key.key = 0;
                sendKey(key);
            }
            oldKeyidx = keyidx;
            
            key.key = keyidx;
            sendKey(key);
            wdt_reset();
            keyidx = pgm_read_byte(address++);
        }
        if(keyidx == K_NONE)
            break;

    }
    
    key.mode = 0;
    key.key = 0;
    sendKey(key);

//    led_mode_init();
}



void playMacroPS2(uint8_t macrokey)
{
    uint8_t i;
    uint8_t keyidx;
    uint8_t keyval;
    long address;
    uint8_t macroSET;

    Key key;
    key.mode = 0;
    key.key = 0;
    
    uint8_t mIndex = 0;
    uint8_t esctoggle =0;

    mIndex = macrokey - K_M01;
    address = MACRO_ADDR_START + (0x100 * mIndex);
    

    macroSET = eeprom_read_byte(EEPADDR_MACRO_SET+mIndex);
     if (macroSET != EEPVAL_MACRO_BIT)      // MACRO not recorded
     {
         return;
     }


    for (i = 0; i < MAX_MACRO_LEN; i++)
    {
        if(esctoggle++ == 4)
        {
//            led_on(LED_PIN_ESC);
            esctoggle = 0;
        }else{
//            led_off(LED_PIN_ESC);
        }
        
        keyidx = pgm_read_byte(address++);

        if(keyidx == K_NONE)
            return;
        if((K_Modifiers < keyidx) && (keyidx < K_Modifiers_end))
        {
            key.mode ^= modifierBitmap[keyidx -K_Modifiers];
            if(key.mode & modifierBitmap[keyidx -K_Modifiers])
            {
                putKey(keyidx,1);
            }else
            {
                putKey(keyidx,0);
            }
            while((keyval = pop()) !=SPLIT)
            {
                while(!(kbd_flags & FLA_TX_OK));
                _delay_us(10);
                tx_state(keyval, STA_NORMAL);
            }
        }else
        {
            putKey(keyidx,1);
            while((keyval = pop()) !=SPLIT)
            {
                while(!(kbd_flags & FLA_TX_OK));
                _delay_us(10);
                tx_state(keyval, STA_NORMAL);
            }
        
            putKey(keyidx,0);
            while((keyval = pop()) !=SPLIT)
            {
                while(!(kbd_flags & FLA_TX_OK));
                _delay_us(10);
                tx_state(keyval, STA_NORMAL);
            }
        }
       
    }
}

#if 1

#if (FLASHEND) > 0xffff /* we need long addressing */
#   define addr_t           unsigned long
#else
#   define addr_t           unsigned int
#endif


typedef union ADDRESS_U{
    addr_t  l;
    unsigned int    s[sizeof(long)/2];
    uchar   c[sizeof(long)];
}ADDRESS;


void writepage(uint8_t *data, addr_t addr) 
    __attribute__ ((section (".appinboot")));

void writepage(uint8_t *data, addr_t addr)
{
    uchar len= SPM_PAGESIZE;
#if 1
    ADDRESS address;

    do{
        long prevAddr;
#if SPM_PAGESIZE > 256
        uint pageAddr;
#else
        uchar pageAddr;
#endif
        pageAddr = address.s[0] & (SPM_PAGESIZE - 1);
        if(pageAddr == 0){              /* if page start: erase */
            cli();
//            boot_page_erase(address.l); /* erase page */
            sei();
//            boot_spm_busy_wait();       /* wait until page is erased */
        }
        cli();
//        boot_page_fill(address.l, *(short *)data);
        sei();
        prevAddr = address.l;
        address.l += 2;
        data += 2;
        /* write page when we cross page boundary */
        pageAddr = address.s[0] & (SPM_PAGESIZE - 1);
        if(pageAddr == 0){
            cli();
//            boot_page_write(prevAddr);
            sei();
//            boot_spm_busy_wait();
        }
        len -= 2;
    }while(len);
#endif
    return;
}


uint8_t flash_writeinpage (uint8_t *data, uint16_t addr)
{
   if (addr < 0x4400)      // FW code area
   {
      
      wdt_disable();
      while(1)
      {
         _delay_ms(1);
      }
      return -1;
   }else{

      writepage(data, addr);
   }
   return 0;
}
#endif



void resetMacro(void)
{
    uint8_t mIndex;
    
    sendString((uint16_t) &mresetstart);
    for (mIndex = 0; mIndex < MAX_MACRO_INDEX; mIndex++)
    {
      wdt_reset();
      eeprom_write_byte(EEPADDR_MACRO_SET+mIndex, (uint8_t)~(EEPVAL_MACRO_BIT));
      sendString((uint16_t) "-");
    }
    sendString((uint16_t) macroend);
}


void recordMacro(uint8_t macrokey)
{
   int8_t col, row;
   uint32_t prev, cur;
   uint8_t prevBit, curBit;
   uint8_t keyidx;
   uint8_t matrixState = 0;
   int16_t index;
   uint8_t mIndex;
   long page;
   Key key;
   uint16_t address;
   mIndex = macrokey - K_M01;
   address = MACRO_ADDR_START + (0x100 * mIndex);
   if(address >= 0x7000)
   {
      sendString((uint16_t) macroend);
      return;
   }
   index = 0;
   page = 0;

   key.mode = 0;
      
   wdt_reset();
   
   cntKey(K_FN, 0x0000);


   for(row = 0; row < MATRIX_MAX_ROW; row++)
   {
      for(col = 0; col < MATRIX_MAX_COL; col++)
      {
         debounceMATRIX[row][col] = -1;
      }
   }
   sendString((uint16_t)macrostart);

   while(1)
   {
      
      wdt_reset();
      matrixState = scanmatrix();
      
      // debounce cleared => compare last matrix and current matrix
      for(row = 0; row < MATRIX_MAX_ROW; row++)
      {
         prev = oldMATRIX[row];
         cur  = curMATRIX[row];
         oldMATRIX[row] = curMATRIX[row];
         for(col = 0; col < MATRIX_MAX_COL; col++)
         {
            prevBit = (uint8_t)prev & 0x01;
            curBit = (uint8_t)cur & 0x01;
            prev >>= 1;
            cur >>= 1;

              keyidx = currentLayer[row][col];

         if ((keyidx <= ErrorUndefined) || (K_Modifiers_end <= keyidx))
            continue;

         if (!prevBit && curBit)                //pushed
         {
            debounceMATRIX[row][col] = 0;       //triger

         }else if (prevBit && !curBit)          //released
         {
            debounceMATRIX[row][col] = 0;    //triger
         }

         if(debounceMATRIX[row][col] >= 0)
         {                
            if(debounceMATRIX[row][col]++ >= kbdConf.matrix_debounce)
            {
               if(curBit)
               {
                  if ((keyidx == K_FN) || ((page == 1) && (index == 0x7F)))
                  {
                     macrobuffer[index] = K_NONE;

                     wdt_reset();
                     flash_writeinpage(macrobuffer, address+(page*128));
                     wdt_reset();
                     eeprom_write_byte(EEPADDR_MACRO_SET+mIndex, EEPVAL_MACRO_BIT);
                     wdt_reset();
                     sendString((uint16_t) macroend);
                     return;
                  }
                  else
                  {
                     macrobuffer[index] = keyidx;
                     if ((K_Modifiers < keyidx)  && (keyidx < K_Modifiers_end))
                     {
                        printModifier(keyidx, 1);
                     }else if (((K_CAPS <= keyidx) && (keyidx <= K_NUMLOCK)) || (keyidx == K_ESC))
                     {
                        printSpecialKey(keyidx);
                     }else if (keyidx < K_KP_EQUAL) 
                     {
                        key.key = macrobuffer[index];
                        sendKey(key);
                        clearKey();
                     }else
                     {
                        index --;       // ignore other keys
                     }

                     if(index == 0x7F)
                     {
                         flash_writeinpage(macrobuffer, address+(page*128));
                         page++;
                         index = 0;
                     }else
                     {
                        index++;
                     }
                  }
               }else
               {
                  if ((K_Modifiers < keyidx)  && (keyidx < K_Modifiers_end))
                  {
                     macrobuffer[index++] = keyidx;
                     printModifier(keyidx, 0);
                     
                  }
               }

               debounceMATRIX[row][col] = -1;
            }
         }
      }
      }
   }
}
