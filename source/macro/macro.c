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
#include "keymap.h"
#include "matrix.h"
#include "macro.h"
#include "led.h"
#include "hwaddress.h"




uint8_t macrobuffer[256] = {};

const char PROGMEM macrostart[20] = "MACRO record mode@";
const char PROGMEM macroend[20] = "@record done@";
const char PROGMEM mresetstart[20] = "MACRO erase";
const char PROGMEM macroresetdone[20]  = "done@";


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
    while (!usbInterruptIsReady()); // wait
    usbSetInterrupt(repBuffer, sizeof(repBuffer)); // send
}

/**
 * This structure can be used as a container for a single 'key'. It consists of
 * the key-code and the modifier-code.
 */
typedef struct {
    uint8_t mode;
    uint8_t key;
} Key;


extern MODIFIERS modifierBitmap[];
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
   uint8_t keyval = 0;
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
   
   prtKey.mode = 0;
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
         prtKey.key = K_G;
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

void clearKey(void)
{
    if(kbdConf.ps2usb_mode)
    {
        usbSendReport(0, 0);
    }
}
/**
 * Send a string to the computer. This function converts each character of an
 * ASCII-string to a key-structure and uses sendKey() to send it.
 * \param string string to send
 */
void sendString(char* string) {
    uint8_t i = 0;
    uint8_t keychar, oldkeychar;
    Key key;
    while((keychar = pgm_read_byte(string))!= (uint8_t)NULL && i < 32) // limit to 64 charater to send at once.
    {
        key = charToKey(keychar);

        if(keychar == oldkeychar)
        {
            clearKey();
        }
        oldkeychar = keychar;
        sendKey(key);
        string++;
        i++;
    }
    
    clearKey();
}


uint8_t getkey(uint8_t key, uint16_t index)
{

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

    macroSET = eeprom_read_byte(EEPADDR_MACRO_SET+mIndex);
    if (macroSET != 1)      // MACRO not recorded
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

    led_mode_init();
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
     if (macroSET != 1)      // MACRO not recorded
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
        
#ifdef KBDMOD_M3
        keyidx = pgm_read_byte(address++);
#else            
        keyidx = pgm_read_byte_far(address++);
#endif

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

typedef union ADDRESS_U{
    long  l;
    unsigned int    s[sizeof(long)/2];
    uchar   c[sizeof(long)];
}ADDRESS;

int8_t flash_writeinpage (uint8_t *data, unsigned long addr)
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

}


void writepage(uint8_t *data, unsigned long addr) 
    __attribute__ ((section (".appinboot")));

void writepage(uint8_t *data, unsigned long addr)
{
    uchar   isLast;
    uchar len;
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

void resetMacro(void)
{
    uint8_t mIndex;
    long address;
    
    sendString(mresetstart);
    for (mIndex = 0; mIndex < MAX_MACRO_INDEX; mIndex++)
    {
      wdt_reset();
      eeprom_write_byte(EEPADDR_MACRO_SET+mIndex, 0);
      sendString("-");
    }
    
    eeprom_write_byte(EEPADDR_SWAPCTRLCAPS, 0x80);
    eeprom_write_byte(EEPADDR_SWAPALTGUI, 0x80);

    sendString(macroresetdone);
}


void recordMacro(uint8_t macrokey)
{
   int8_t col, row;
   uint32_t prev, cur;
   uint8_t prevBit, curBit;
   uint8_t keyidx;
   uint8_t matrixState = 0;
   uint8_t retVal = 0;
   int16_t i;
   int16_t index;
   uint8_t mIndex;
   long page;
   uint8_t t_layer;
   Key key;
   uint16_t address;
   mIndex = macrokey - K_M01;
   address = MACRO_ADDR_START + (0x100 * mIndex);
    
   index = 0;
   page = 0;

   key.mode = 0;
      
   wdt_reset();

   
   cntKey(K_FN, 0x0000);

//   for (i = 0; i <= 255; i++)
//      macrobuffer[i] = 0x00;
   for(col = 0; col < MAX_COL; col++)
   {
      for(row = 0; row < MAX_ROW; row++)
      {
         debounceMATRIX[col][row] = -1;
      }
   }

   sendString(macrostart);



   while(1)
   {
      
      wdt_reset();
      matrixState = scanmatrix();
      
      t_layer = kbdConf.keymapLayerIndex;

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

            row = i;

            keyidx = pgm_read_byte(keylayer(t_layer)+(col*MAX_ROW)+row);

         if ((keyidx <= ErrorUndefined) || (K_Modifiers_end <= keyidx))
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
                  if ((keyidx == K_FN) || ((page == 1) && (index == 0x7F)))
                  {
                     macrobuffer[index] = K_NONE;

                     wdt_reset();
                     flash_writeinpage(macrobuffer, address+(page*128));
                     wdt_reset();
                     eeprom_write_byte(EEPADDR_MACRO_SET+mIndex, 1);
                     wdt_reset();
                     sendString(macroend);
                     return;
                  }
                  else
                  {
                     macrobuffer[index] = keyidx;
                     if ((K_Modifiers < keyidx)  && (keyidx < K_Modifiers_end))
                     {
                        printModifier(keyidx, 1);
                     }else
                     {
                        key.key = macrobuffer[index];
                        sendKey(key);
                        clearKey();
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

               debounceMATRIX[col][row] = -1;
            }
         }
      }
      }
   }
}
#endif
