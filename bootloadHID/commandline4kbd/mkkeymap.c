#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "keymap.h"

#define MAX_ROW     6
#define MAX_COL     19

#define MAX_LAYER   8
#define MAXHEXLINE  32	/* the maximum number of bytes to put in one line */


#define DBG_PRINTF(X, s...) printf(X, ##s )
//#define DBG_PRINTF(X, s...)

const char *keycode[256] = {
        "K_NONE",
        "ErrorRollOver",
        "POSTFail",
        "ErrorUndefined",
        "K_A",              // 0x04
        "K_B",
        "K_C",
        "K_D",
        "K_E",
        "K_F",
        "K_G",
        "K_H",
        "K_I",
        "K_J",
        "K_K",
        "K_L",
    
        "K_M",              // 0x10
        "K_N",
        "K_O",
        "K_P",
        "K_Q",
        "K_R",
        "K_S",
        "K_T",
        "K_U",
        "K_V",
        "K_W",
        "K_X",
        "K_Y",
        "K_Z",
        "K_1",              //       1 and !
        "K_2",              //       2 and @
    
        "K_3",              // 0x20  3 and #
        "K_4",              //       4 and $
        "K_5",              //       5 and %
        "K_6",              //       6 and ^
        "K_7",              //       7 and &
        "K_8",              //       8 and *
        "K_9",              //       9 and (
        "K_0",              // 0x27  0 and )
        "K_ENTER",          // 0x28  enter
        "K_ESC",            // 0x29
        "K_BKSP",           // 0x2A  backspace
        "K_TAB",            // 0x2B
        "K_SPACE",          // 0x2C
        "K_MINUS",          // 0x2D  - and _
        "K_EQUAL",          // 0x2E  = and +
        "K_LBR",            // 0x2F  [ and {
    
        "K_RBR",            // 0x30  ] and }
        "K_BKSLASH",        // 0x31  \ and |
        "K_Europe1",        // 0x32  non-US # and ~
        "K_COLON",          // 0x33  ; and :
        "K_QUOTE",          // 0x34  ' and "
        "K_HASH",           // 0x35  grave accent and tilde
        "K_COMMA",          // 0x36  ", and <
        "K_DOT",            // 0x37  . and >
        "K_SLASH",          // 0x38  / and ?
        "K_CAPS",           // 0x39
        "K_F1",
        "K_F2",
        "K_F3",
        "K_F4",
        "K_F5",
        "K_F6",
    
        "K_F7",             // 0x40
        "K_F8",
        "K_F9",
        "K_F10",
        "K_F11",
        "K_F12",
        "K_PRNSCR",
        "K_SCRLCK",
        "K_PAUSE",          //Break
        "K_INSERT",
        "K_HOME",
        "K_PGUP",
        "K_DEL",
        "K_END",
        "K_PGDN",
        "K_RIGHT",
        
        "K_LEFT",           // 0x50
        "K_DOWN",
        "K_UP",
        "K_NUMLOCK",        // Clear
        "K_KP_SLASH",
        "K_KP_AST",
        "K_KP_MINUS",
        "K_KP_PLUS",
        "K_KP_ENTER",
        "K_KP_1",           // End
        "K_KP_2",           // Down Arrow
        "K_KP_3",           // Page Down
        "K_KP_4",           // Left Arrow
        "K_KP_5",
        "K_KP_6",           // Right Arrow
        "K_KP_7",           // Home
        
        "K_KP_8",           // 0x60  Up Arrow
        "K_KP_9",           // Page Up
        "K_KP_0",           // Insert
        "K_KP_DOT",         // Delete
        "K_Europe2",        // non-US \ and |
        "K_APPS",           // 102
        "K_POWER_HID",
        "K_KP_EQUAL",
        "K_LED0",           //
        "K_LED1",           //
        "K_LED2",           //
        "K_LED3",           //
        "K_LFX",       //
        "K_LPAD",      //
        "K_LFULL",     //
        "K_LASD",     //
    
        "K_LARR",    // 0x70
        "K_LVESEL",   
        "K_MRESET", 
        "K_RESET",          //254
        "K_FN",             //255
        "K_HELP",
        "K_MENU",
        "K_SEL",
        "K_STOP_HID",
        "K_AGAIN",
        "K_UNDO",
        "K_CUT",
        "K_COPY",
        "K_PASTE",
        "K_FIND",
        "K_MUTE_HID",
        
        "K_VOLUP",          // 0x80
        "K_VOLDN",
        "K_KL_CAP",
        "K_KL_NUM",
        "K_KL_SCL",
        "K_KP_COMMA",
        "K_EQUAL_SIGN",
        "K_L0",             //
        "K_L1",             //
        "K_L2",             //
        "K_L3",             //
        "K_L4",             //
        "K_L5",             //
        "K_L6",             //
        "K_INTL8",
        "K_INTL9",
        
        "K_HANJA",          // 0x90
        "K_HANGLE",
        "K_KATA",
        "K_HIRA",
        "K_System",         //K_LANG5,
        "K_POWER",          //K_LANG6,
        "K_SLEEP",          //K_LANG7,
        "K_WAKE",           //K_LANG8,
        "K_KEYLOCK",        //K_LANG9,
        "K_WINKEYLOCK",
        "K_SYSREQ",
        "K_CANCEL",
        "K_CLEAR",
        "K_PRIOR",
        "K_RETURN",
        "K_SPERATOR",
        
        "K_OUT",            // 0xA0
        "K_OPER",
        "K_CLR_AGIN",
        "K_CRSEL",
        "K_EXCEL",
        
    /* These are NOT standard USB HID - handled specially in decoding", 
         so they will be mapped to the modifier byte in the USB report */
        "K_Modifiers",
        "K_LCTRL",          // 0x01
        "K_LSHIFT",         // 0x02
        "K_LALT",           // 0x04
        "K_LGUI",           // 0x08
        "K_RCTRL",          // 0x10
        "K_RSHIFT",         // 0x20
        "K_RALT",           // 0x40
        "K_RGUI",           // 0x80
        "K_Modifiers_end",
    
       "K_NEXT_TRK",        
       "K_PREV_TRK",        // 0xB0
       "K_STOP",
       "K_PLAY",
       "K_MUTE",
       "K_BASS_BST",
       "K_LOUDNESS",
       "K_VOL_UP",
       "K_VOL_DOWN",
       "K_BASS_UP",
       "K_BASS_DN",
       "K_TRE_UP",
       "K_TRE_DN",
    
       "K_MEDIA_SEL",      // 0xc0
       "K_MAIL",
       "K_CALC",
       "K_MYCOM",
       "K_WWW_SEARCH",
       "K_WWW_HOME",
       "K_WWW_BACK",
       "K_WWW_FORWARD",
       "K_WWW_STOP",
       "K_WWW_REFRESH",
       "K_WWW_FAVORITE",
       "K_EJECT",
       "K_SCREENSAVE",
       "K_REC",
        "K_REWIND",
        "K_MINIMIZE",
    
       "K_M01",            // 0xd0 //K_F13,
       "K_M02",            //K_F14,
       "K_M03",            //K_F15,
       "K_M04",            //K_F16,
       "K_M05",            //K_F17,
       "K_M06",            //K_F18,
       "K_M07",            //K_F19,
       "K_M08",            //K_F20,
       "K_M09",            //K_F21,        // 0x70
       "K_M10",            //K_F22,
       "K_M11",            //K_F23,
       "K_M12",            //K_F24,
       "K_M13",            //K_INTL1,
       "K_M14",            //K_INTL2,
       "K_M15",            //K_INTL3,
       "K_M16",            //K_INTL4,
    
       "K_M17",            // 0xe0 //K_INTL5,
       "K_M18",            //K_INTL6,
       "K_M19",            //K_INTL7,
       "K_M20",        
       "K_M21",        
       "K_M22",        
       "K_M23",       
       "K_M24",   
       "K_M25",            //213
       "K_M26",            //214
       "K_M27",            //215
       "K_M28",            //216
       "K_M29",            //217
       "K_M30",            //218
       "K_M31",            //219
       "K_M32",            //220
    
       "K_M33",            // 0xf0//221
       "K_M34",            //222
       "K_M35",            //223
       "K_M36",            //224
       "K_M37",            //225
       "K_M38",            //226
       "K_M39",            //227
       "K_M40",            //228
       "K_M41",            //229
       "K_M42",            //230
       "K_M43",            //231
       "K_M44",            //232
       "K_M45",            //233
       "K_M46",            //234
       "K_M47",            //235
       "K_M48",            //236
       "K_M49",
       "K_M50",
       "K_M51",
       "K_M52"
};
    


// Total 132 keys + one none

char temp;
unsigned char keymap_code[MAX_LAYER * MAX_ROW * MAX_COL];


const char *sLedmode[9] = {
    "FADING",
    "FADING_PUSH_ON",
    "PUSHED_LEVEL",
    "PUSH_ON",
    "PUSH_OFF",
    "ALWAYS",
    "BASECAPS",
    "OFF",
    "NONE"
};
    
#define MAX_TOKEN_LEN   256

typedef enum
{
    LED_EFFECT_FADING          = 0,
    LED_EFFECT_FADING_PUSH_ON  = 1,
    LED_EFFECT_PUSHED_LEVEL    = 2,
    LED_EFFECT_PUSH_ON         = 3,
    LED_EFFECT_PUSH_OFF        = 4,
    LED_EFFECT_ALWAYS          = 5,
    LED_EFFECT_BASECAPS        = 6,
    LED_EFFECT_OFF             = 7,
    LED_EFFECT_NONE
}LED_MODE;

#define LEDMODE_ADDRESS 0x9800
unsigned char  ledmode[8][11];


int keymAddress;
int ledmAddress;

typedef enum
{
    PARSING_KEYMAP,
    PARSING_LEDEFFECT
}PARSING;

int addressExtended = 0;


short getKeyIdx(const char *keystring)
{
    unsigned int i;
    DBG_PRINTF("%s\n", keystring);
    //scanf("%c", &temp);
    for (i = 0; i < 256; i++)
    {
        if(strcmp(keystring, keycode[i]) == 0)
        {
            DBG_PRINTF(": found!!\n");
            return (char)i;
        }
    }
    DBG_PRINTF("ERROR: matrix is invalied ! \n");
    //scanf("%c", &temp);
    return -1;
}




short getLEDIdx(const char *str)
{
    unsigned int i;
    DBG_PRINTF("%s", str);
    //scanf("%c", &temp);
    for (i = 0; i < 8; i++)
    {
        if(strcmp(str, sLedmode[i]) == 0)
        {
            DBG_PRINTF(": found!!\n");
            return (char)i;
        }
    }
    DBG_PRINTF("ERROR: LED is invalied ! \n");
    //scanf("%c", &temp);
    return -1;
}


int getToken(FILE *fp, char *pbuf)
{
    char ch;
    size_t len;
    int i;
   
    i = 0;
    memset((void *)pbuf, 0, 256);
    while (1){
        len = fread(&ch, 1, 1, fp);
        if((len != 1) || (i >= MAX_TOKEN_LEN))
        {
            DBG_PRINTF("read len = %d \n", len);
            break;
        }
        if(ch == ',' || ch == ' '|| ch == '{' ||ch == '}'|| ch == '\n'|| ch == '\t'|| ch == '\r')           // seperator
        {
            if(i == 0)
            {
                continue;   // ignore the beginning
            }else
            {
                break;      // end of token

            }
        }else
        {
            *pbuf++ = ch;   // make token
            i++;
        }
    }
    return i;
}

int strtoi(char *str, int nsystem)
{
    int i = 0;
    int digit = 1;
    int number;
    int result = 0;
    while (*(str+1) != 0)        // rewind to 1st digit
    {
        str++;
        i++;
    }
    
    while(*str != 'x' && *str != 'X' && i-- >= 0)
    {
        //DBG_PRINTF("%c", *str);
        if('0' <= *str && *str <= '9')
        {
            number = *str -'0';
        }else if ('a' <= *str && *str <= 'f')
        {
            number = 10 + *str -'a';
        }else if ('A' <= *str && *str <= 'F')
        {
            number = 10 + *str -'A';
        }else
        {
            DBG_PRINTF("Error : [%c] is not a Number \n", *str);
            result = -1;
            break;
        }
            
        result += number * digit;
        digit *= nsystem;                     // hexadecimal
        str--;
    }
    return result;
}

int  maxrow, maxcol, maxlayer;

    
int interprete(const char *filename, unsigned char *keymap, unsigned char *ledmode)
{
    FILE *fp = fopen(filename, "r");
    FILE *fptmp = fopen("tmpKeymap.bin", "wb");

    char str[256];
    char index;
    
    unsigned char temp;
    size_t len;
    int mode;
    mode = PARSING_KEYMAP;
    unsigned char *pTmp;
    pTmp = keymap;
    size_t lenKeymap = 0;
    
    maxrow = MAX_ROW;
    maxcol = MAX_COL;
    maxlayer = MAX_LAYER;
    
    if (fp == NULL)
    {
        DBG_PRINTF("Error : Invalid file name!\n");
//        scanf("%c", &temp);
        return -1;
    }
    
    while(1)
    {
        // 1 get token
        len = getToken(fp,str);
        if(len == 0)
        {
            break;
        }
        DBG_PRINTF("Token:%s , Len = %d\n", str, len);

        if(strcmp(str, "KEYMAP") == 0)
        {
            mode = PARSING_KEYMAP;
            if(getToken(fp,str) == 0)
                break;
            
            DBG_PRINTF("keymAddress = %s \n", str);
            keymAddress = strtoi(str,16);
            
            DBG_PRINTF("keymAddress = %x \n", keymAddress);
            //scanf("%c", &temp);
            
        }else if(strcmp(str, "LEDEFFECT") == 0)
        {
            mode = PARSING_LEDEFFECT;

            if(getToken(fp,str) == 0)
                break;

            DBG_PRINTF("ledmAddress = %s \n", str);
            ledmAddress = strtoi(str,16);

            DBG_PRINTF("ledmAddress = %x \n", ledmAddress);
            //scanf("%c", &temp);
        }else if(strcmp(str, "ROW") == 0)
        {
            if(getToken(fp,str) == 0)
                break;
            DBG_PRINTF("maxrow = %s \n", str);
            maxrow = strtoi(str, 10);
            DBG_PRINTF("maxrow = %d \n", maxrow);
        }else if(strcmp(str, "COLUMN") == 0)
        {
            if(getToken(fp,str) == 0)
                break;
            DBG_PRINTF("maxcolumn = %s \n", str);
            maxcol = strtoi(str, 10);
            DBG_PRINTF("maxcolumn = %d \n", maxcol);
        }else if(strcmp(str, "LAYER") == 0)
        {
            if(getToken(fp,str) == 0)
                break;
            DBG_PRINTF("STRING maxlayer = %s \n", str);
            maxlayer = strtoi(str, 10);
            DBG_PRINTF("DEC maxlayer = %d \n", maxlayer);
        }

        if(mode == PARSING_KEYMAP)
        {
            // 2. get key
            index = getKeyIdx(str);
    		if(index == -1)
    			continue;
            // 3. put key to matrix
            *keymap++ = (char)index;
            lenKeymap++;
        }else if(mode == PARSING_LEDEFFECT)
        {
            index = getLEDIdx(str);
            if(index == -1)
                continue;
                // 3. put key to matrix
            *ledmode++ = (char)index;
        }
		
		DBG_PRINTF("DEC maxlayer = %d \n", maxlayer);
		if(maxlayer == -1)
			break;
	}        
    fwrite( pTmp, 1, lenKeymap, fptmp);
    fclose(fp);
    fclose(fptmp);
    return 0;
}


void insertExtSegAddr(FILE *fp)
{
   fprintf(fp,":020000021000EC\n");
}      

void revertExtSegAddr(FILE *fp)
{
   fprintf(fp,":020000020000FC\n");
}

int buffer2Hex(FILE *fp, int address, int length, unsigned char *buffer)
{
   unsigned char checksum = 0;
   int j;
   char cnt;   

   cnt = length;

   while (length > 0)
   {
      if (!addressExtended && (address >= 0x10000))
      {
         insertExtSegAddr(fp);
         addressExtended = 1;
      }else if (addressExtended && (address < 0x10000))
      {
         revertExtSegAddr(fp);
         addressExtended = 0;
      }

      length = length - 0x10;

      cnt = length >= 0 ? 0x10 : length + 0x10;

      fprintf(fp,":%02X", cnt);
      checksum += cnt;
      fprintf(fp,"%04X", (unsigned short)address);
      checksum += ((address >> 8) & 0xFF);
      checksum += address & 0xFF;
      fprintf(fp,"00");
      checksum += 0x00;

      for(j = 0; j < cnt; j++)
      {
         fprintf(fp,"%02X", *buffer);
         checksum += *buffer;
         buffer++;
      }
      checksum = ~(checksum & 0xFF) + 1;
      fprintf(fp,"%02X\n", checksum & 0xFF);
      address += 0x10;
      checksum = 0;
   }
   return 0;
}

int main(int argc, char *argv[])
{
    int layer;
    int address;
    unsigned char *keymap;
    int fnamelen;
    char *fname;
    
    if(argc != 2)
    {
        DBG_PRINTF("USAGE : mkkeymap.exe keymap.txt \n");
        return -1;
    }
    fnamelen = strlen(argv[1]);
    fname = malloc(fnamelen+1);
    memset(fname, '\0', fnamelen);
    memcpy(fname, argv[1], fnamelen-4);
    strcat(fname, ".hex");

    DBG_PRINTF("%s \n", fname);
      
    FILE *fp = fopen(fname, "w");

    
   interprete(argv[1], keymap_code, &(ledmode[0][0]));
       DBG_PRINTF("DEC maxlayer = %d \n", maxlayer);

   return 0;

   address = keymAddress;
	
   DBG_PRINTF("maxlayer = %d \n", maxlayer);
   for (layer = 0; layer < maxlayer ; layer++)
   {
      keymap = &(keymap_code[layer * maxrow * maxcol]);
      buffer2Hex(fp, address, maxrow * maxcol, keymap);
      address += 0x100;
   }

   keymap = &(ledmode[0][0]);

   buffer2Hex(fp, ledmAddress, 256, keymap);
   
   fclose(fp);
   return 0;
}

