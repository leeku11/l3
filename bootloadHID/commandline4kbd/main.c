/* Name: main.c
 * Project: AVR bootloader HID
 * Author: Christian Starkjohann
 * Creation Date: 2007-03-19
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: Proprietary, free under certain conditions. See Documentation.
 * This Revision: $Id$
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "usbcalls.h"

#define IDENT_VENDOR_NUM        0x20a0
#define IDENT_VENDOR_STRING     "LeeKu"
#define IDENT_PRODUCT_NUM       16967
#define IDENT_PRODUCT_STRING    "L3"

#define DBG_PRINTF printf
#define HIDDEBUG

typedef unsigned char uint8_t;
/* ------------------------------------------------------------------------- */

static char dataBuffer[131072 + 256];    /* buffer for file data */
static int  startAddress[1024], endAddress[1024];
static int  addressIndex = 0;
static char leaveBootLoader = 0;

/* ------------------------------------------------------------------------- */

static int  parseUntilColon(FILE *fp)
{
int c;

    do{
        c = getc(fp);
    }while(c != ':' && c != EOF);
    return c;
}

static int  parseHex(FILE *fp, int numDigits)
{
int     i;
char    temp[9];

    for(i = 0; i < numDigits; i++)
        temp[i] = getc(fp);
    temp[i] = 0;
    return strtol(temp, NULL, 16);
}

/* ------------------------------------------------------------------------- */
static int start = 1;
static int  parseIntelHex(char *hexfile, char buffer[131072 + 256])
{
int     address, base, d, segment, i, lineLen, sum, extSegAddr;
FILE    *input;
   extSegAddr = 0;
    input = fopen(hexfile, "r");
    if(input == NULL){
        fprintf(stderr, "error opening %s: %s\n", hexfile, strerror(errno));
        return 1;
    }
    while(parseUntilColon(input) == ':'){
        sum = 0;
        sum += lineLen = parseHex(input, 2);
        base = address = parseHex(input, 4);
        sum += address >> 8;
        sum += address;
        
        address += extSegAddr;
        base += extSegAddr;

        if(address >= 0x7000)
            break;
        
        sum += segment = parseHex(input, 2);  /* segment value? */
        if(segment == 0x02)    /* Extended Segment Address Records (HEX86)*/
        {
            extSegAddr = parseHex(input, 4);
            extSegAddr = (extSegAddr << 4) & 0xFFFF0; 
            continue;
        }else if(segment != 0)    /* ignore lines where this byte is not 0 */
        {
           continue;
        }
        for(i = 0; i < lineLen ; i++){
            d = parseHex(input, 2);
            buffer[address++] = d;
            sum += d;
        }
        sum += parseHex(input, 2);
        if((sum & 0xff) != 0){
            fprintf(stderr, "Warning: Checksum error between address 0x%x and 0x%x\n", base, address);
        }

        if(start)
        {
            startAddress[addressIndex] = base;
            endAddress[addressIndex] = address;
            start = 0;
//            fprintf(stderr, "s[%d]=%x, e[%d]=%x \n", addressIndex, startAddress[addressIndex], addressIndex, endAddress[addressIndex]);
        }
            
        if ((base - endAddress[addressIndex]) >= 32 || (endAddress[addressIndex] - base) >= 32)
        {
            addressIndex++;
            startAddress[addressIndex] = base;
//            fprintf(stderr, "s[%d]=%x, e[%d]=%x \n", addressIndex, startAddress[addressIndex], addressIndex, endAddress[addressIndex]);
        }

        endAddress[addressIndex] = address;
        
//        fprintf(stderr, "s[%d]=%x, e[%d]=%x \n", addressIndex, startAddress[addressIndex], addressIndex, endAddress[addressIndex]);
    }

#ifdef DEBUG
    for(i = 0; i <= addressIndex; i++)
    {
       fprintf(stderr, "s[%d]=%x, e[%d]=%x \n", i, startAddress[i], i, endAddress[i]);
    }
#endif
    fclose(input);
    return 0;
}

/* ------------------------------------------------------------------------- */

char    *usbErrorMessage(int errCode)
{
static char buffer[80];

    switch(errCode){
        case USB_ERROR_ACCESS:      return "Access to device denied";
        case USB_ERROR_NOTFOUND:    return "The specified device was not found";
        case USB_ERROR_BUSY:        return "The device is used by another application";
        case USB_ERROR_IO:          return "Communication error with device";
        default:
            sprintf(buffer, "Unknown USB error %d", errCode);
            return buffer;
    }
    return NULL;    /* not reached */
}

static int  getUsbInt(char *buffer, int numBytes)
{
int shift = 0, value = 0, i;

    for(i = 0; i < numBytes; i++){
        value |= ((int)*buffer & 0xff) << shift;
        shift += 8;
        buffer++;
    }
    return value;
}

static void setUsbInt(char *buffer, int value, int numBytes)
{
int i;

    for(i = 0; i < numBytes; i++){
        *buffer++ = value;
        value >>= 8;
    }
}

/* ------------------------------------------------------------------------- */

typedef struct deviceInfo{
    char    reportId;
    char    pageSize[2];
    char    flashSize[4];
}deviceInfo_t;

typedef struct deviceData{
    char    reportId;
    char    address[3];
    char    data[128];
}deviceData_t;


char    command[3]={1,2,3};


typedef struct bootCmd{
    char reportId;
    char cmd;
    char index;
    char length;   
    unsigned char data[128];
}bootCmd_t;


bootCmd_t cmdData;


static int sendDBGCmd(char *buffer)
{
usbDevice_t *dev = NULL;
int         err = 0, len, mask, pageSize, deviceSize, i,j, offset;

    if((err = usbOpenDevice(&dev, IDENT_VENDOR_NUM, IDENT_VENDOR_STRING, IDENT_PRODUCT_NUM, IDENT_PRODUCT_STRING, 1)) != 0){
        fprintf(stderr, "Error opening HIDBoot device: %s\n", usbErrorMessage(err));
        goto errorOccurred;
    }
    len = 8;

    buffer[0] = 1;      // Report ID
    buffer[1] = 1;      // Command
    
    if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, buffer, 8)) != 0){
        //fprintf(stderr, "Error uploading data block: %s\n", usbErrorMessage(err));
        goto errorOccurred;
        }
    printf("cmd : [%d][%d][%d][%d][%d][%d][%d][%d] :length %d Sent \n", buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7], len);
    
        fflush(stdout);
    errorOccurred:
        if(dev != NULL)
            usbCloseDevice(dev);
        return err;
}

    

static int receiveCmd(bootCmd_t *cmdBuffer)
{
usbDevice_t *dev = NULL;
int         err = 0, len, mask, pageSize, deviceSize, i,j, offset;

    if((err = usbOpenDevice(&dev, IDENT_VENDOR_NUM, IDENT_VENDOR_STRING, IDENT_PRODUCT_NUM, IDENT_PRODUCT_STRING, 1)) != 0){
        fprintf(stderr, "Error opening HIDBoot device: %s\n", usbErrorMessage(err));
        goto errorOccurred;
    }
    len = sizeof(bootCmd_t);

    cmdBuffer->reportId = 1;

    printf("report ID   : %d \n", cmdBuffer->reportId);
    printf("cmd         : %d \n", cmdBuffer->cmd);
    printf("index       : %d \n", cmdBuffer->index);
    printf("length      : %d \n", cmdBuffer->length);
    
    if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, cmdBuffer, 8)) != 0){
        fprintf(stderr, "Error uploading data block: %s\n", usbErrorMessage(err));
        goto errorOccurred;
        }
    
    cmdBuffer->reportId = 2;
    if((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 2, cmdBuffer, &len)) != 0){
        fprintf(stderr, "Error reading page size: %s\n", usbErrorMessage(err));
        goto errorOccurred;
        }
    if(len < sizeof(cmdBuffer)){
        fprintf(stderr, "Not enough bytes in device info report (%d instead of %d)\n", len, (int)sizeof(cmdBuffer->cmd));
        err = -1;
        goto errorOccurred;
        }
    fprintf(stderr, "received length: %d\n", len);

    offset = 0;

    printf("report ID   : %d \n", cmdBuffer->reportId);
    printf("cmd         : %d \n", cmdBuffer->cmd);
    printf("index       : %d \n", cmdBuffer->index);
    printf("length      : %d \n", cmdBuffer->length);

    for (i = 0; i < 6; i++)
    {
        printf("\n");
        for(j = 0; j <20; j++)
            {
            printf("0x%2x|", cmdBuffer->data[offset++]);
        }
    }
    printf("Receive Succeed\n");


    fflush(stdout);
errorOccurred:
    if(dev != NULL)
        usbCloseDevice(dev);
    return err;
}


static int sendCmd(bootCmd_t *cmdBuffer)
{
usbDevice_t *dev = NULL;
int         err = 0, len, mask, pageSize, deviceSize, i,j, offset;

    if((err = usbOpenDevice(&dev, IDENT_VENDOR_NUM, IDENT_VENDOR_STRING, IDENT_PRODUCT_NUM, IDENT_PRODUCT_STRING, 1)) != 0){
        fprintf(stderr, "Error opening HIDBoot device: %s\n", usbErrorMessage(err));
        goto errorOccurred;
    }
    



    cmdBuffer->reportId = 1;
    if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, cmdBuffer, 8)) != 0){
    fprintf(stderr, "Error uploading data block: %s\n", usbErrorMessage(err));
    goto errorOccurred;
    }

    cmdBuffer->reportId = 2;
    printf("Send Command ==============\n");
    printf("report ID   : %d \n", cmdBuffer->reportId);
    printf("cmd         : %d \n", cmdBuffer->cmd);
    printf("index       : %d \n", cmdBuffer->index);
    printf("length      : %d \n", cmdBuffer->length);
    printf("data[0]     : %d \n", cmdBuffer->data[0]);
    for (i = 0; i < 6; i++)
    {
        printf("\n");
        for(j = 0; j <20; j++)
            {
            printf("0x%2x|", cmdBuffer->data[offset++]);
        }
    }
    if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, cmdBuffer, sizeof(bootCmd_t))) != 0){
        fprintf(stderr, "Error uploading data block: %s\n", usbErrorMessage(err));
        goto errorOccurred;
        }

    printf("\nSend Succeed !\n");

errorOccurred:
    if(dev != NULL)
        usbCloseDevice(dev);
    return err;
}

/* ------------------------------------------------------------------------- */

static void printUsage(char *pname)
{
    fprintf(stderr, "usage: %s [-r] [<intel-hexfile>]\n", pname);
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

#define MAX_RGB_CHAIN       20


enum
{
    RGB_EFFECT_BOOTHID = 0,
    RGB_EFFECT_FADE_BUF,
    RGB_EFFECT_FADE_LOOP,
    RGB_EFFECT_HEARTBEAT_BUF,
    RGB_EFFECT_HEARTBEAT_LOOP,
    RGB_EFFECT_BASIC,
    RGB_EFFECT_MAX
};

#define FADE_HIGH_HOLD             5
#define FADE_LOW_HOLD              0
#define FADE_IN_ACCEL              0  // normal ascending

#define HEARTBEAT_HIGH_HOLD        1
#define HEARTBEAT_LOW_HOLD         5
#define HEARTBEAT_IN_ACCEL         1  // 1: weighted ascending. 2: quadratic ascending 



typedef struct {
    unsigned char index;
    unsigned char high_hold;
    unsigned char low_hold;
    unsigned char accel_mode;
} rgb_effect_param_type;


typedef struct kbd_conf
{
    unsigned char ps2usb_mode;                    // 0: PS2, 1: USB
    unsigned char keymapLayerIndex;                     // KEYMAP layer index;
    unsigned char swapCtrlCaps;                   // 1: Swap Capslock <-> Left Ctrl
    unsigned char swapAltGui;                     // 1: Swap Alt <-> GUI(WIN) 
    unsigned char led_preset_index;               // LED effect  preset index
    unsigned char led_preset[3][5];               // Block configuration for LED effect  preset
    unsigned char rgb_effect_index;               // RGB effect preset
    unsigned char rgb_chain;                      // RGB5050 numbers (H/W dependent)
    unsigned char rgb_preset[MAX_RGB_CHAIN][3];   // Chain color
    rgb_effect_param_type rgb_effect_param[RGB_EFFECT_MAX]; // RGB effect parameter
    unsigned short rgb_limit;
    unsigned short rgb_speed;
    unsigned char matrix_debounce;
    unsigned char sleeptimer;    
    uint8_t padding[16];
}kbd_configuration_t;



kbd_configuration_t kbdConf;
kbd_configuration_t *pkbdConf;


typedef struct HIDCMD_debug{
    uint8_t reportID;
    uint8_t cmd;
    uint8_t subcmd;
    uint8_t arg2;   
    uint8_t arg3;
    uint8_t arg4;
    uint8_t arg5;
    uint8_t arg6;
}HIDCMD_debug_t;

typedef enum
{
    CMD_DEBUG = 1,
    CMD_CONFIG = 2,
    CMD_KEYMAP = 3,
    CMD_MACRO  = 4
}BOOT_HID_CMD;


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


rgb_effect_param_type kbdRgbEffectParam[RGB_EFFECT_MAX] = 
{
    { RGB_EFFECT_BOOTHID, 0, 0, 0 },    // RGB_EFFECT_BOOTHID
    { RGB_EFFECT_FADE_BUF, FADE_HIGH_HOLD, FADE_LOW_HOLD, FADE_IN_ACCEL },    // RGB_EFFECT_FADE_BUF
    { RGB_EFFECT_FADE_LOOP, FADE_HIGH_HOLD, FADE_LOW_HOLD, FADE_IN_ACCEL },    // RGB_EFFECT_FADE_LOOP
    { RGB_EFFECT_HEARTBEAT_BUF, HEARTBEAT_HIGH_HOLD, HEARTBEAT_LOW_HOLD, HEARTBEAT_IN_ACCEL },    // RGB_EFFECT_HEARTBEAT_BUF
    { RGB_EFFECT_HEARTBEAT_LOOP, HEARTBEAT_HIGH_HOLD, HEARTBEAT_LOW_HOLD, HEARTBEAT_IN_ACCEL },    // RGB_EFFECT_HEARTBEAT_LOOP
    { RGB_EFFECT_BASIC, HEARTBEAT_HIGH_HOLD, HEARTBEAT_LOW_HOLD, HEARTBEAT_IN_ACCEL },    // RGB_EFFECT_HEARTBEAT_LOOP
};

static uint8_t tmpled_preset[3][5] = {{LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_FADING, LED_EFFECT_FADING_PUSH_ON},
                    {LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_PUSHED_LEVEL, LED_EFFECT_PUSH_ON},
                    {LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_PUSH_OFF, LED_EFFECT_BASECAPS}};

#if 0  //L3_ALPhas
static uint8_t tmprgp_preset[MAX_RGB_CHAIN][3] =         
        {{0, 250, 250}, {0, 250, 250},
         {0, 250, 0},   {100, 250,0},  {250, 250, 0}, {250, 0, 0}, {0, 0, 250}, {0, 50, 250},  {0, 250, 250}, {0, 250, 100}, {0, 250, 100},
         {0, 250, 100}, {0, 250, 250}, {0, 50, 250},  {0, 0, 250}, {250, 0, 0}, {250, 250, 0}, {100, 250,0},  {0, 250, 0}, {0, 250, 0}};

#define RGB_CHAIN_NUM   21
#define DEFAULT_LAYER   2

#else
static uint8_t tmprgp_preset[MAX_RGB_CHAIN][3] =         
        {{0, 250, 0},  {100, 250,0},  {250, 250, 0}, {250, 0, 0},   {0, 0, 250},   {0, 50, 250}, {0, 250, 250},
        {0, 250, 250}, {0, 50, 250},  {0, 0, 250},   {250, 0, 0},   {250, 250, 0}, {100, 250,0}, {0, 250, 0},
        {0, 250, 100}, {0, 250, 100}, {0, 250, 100}, {0, 250, 100}, {0, 250, 100}, {0, 250, 100}};

#define RGB_CHAIN_NUM   14         
#define DEFAULT_LAYER   1
#endif    

int readConfg(char *pbuf)
{
   int status;

   cmdData.cmd = CMD_CONFIG;

   status = receiveCmd(&cmdData);
   if(status != 0)
   {
      return status;
   }
   memcpy(pbuf,cmdData.data, sizeof(kbdConf));

   return status;
}

int writeConfig(char *pbuf)
{
   int status;

   memcpy(cmdData.data, pbuf, sizeof(kbdConf)); 

   cmdData.cmd = CMD_CONFIG;
   status = sendCmd(&cmdData);

   return status;
}


int readKeymap(char *pbuf)
{
   int i;
   int status;
   
   cmdData.cmd = CMD_KEYMAP;

   for (i = 0; i < 4; i++)
   {
       cmdData.index = i;

       status = receiveCmd(&cmdData);

       memcpy(pbuf, cmdData.data, 120);
       pbuf += 120;
       if(status != 0)
         break;
   }
   return status;
}

int writeKeymap(char *pbuf)
{
   int i;
   int status;
   
   cmdData.cmd = CMD_KEYMAP;

   for (i = 0; i < 4; i++)
   {
       cmdData.index = i;
       memcpy(cmdData.data,pbuf, 120);
       pbuf += 120;
       
       status = sendCmd(&cmdData);
       if(status != 0)
         break;

   }
   return status;
}

int setRGB(unsigned char g, unsigned char r, unsigned char b)
{
    int i;
    for (i = 0; i < MAX_RGB_CHAIN; i++)
    {
        kbdConf.rgb_preset[i][0] = g;
        kbdConf.rgb_preset[i][1] = r;
        kbdConf.rgb_preset[i][2] = b;
    }
}



#define ERR_NONE             0
#define ERR_INVALID_ARG    -100
#define ERR_INVALID_FILE   -101  


int write2File(char *filename, void* pbuf, int length)
{
   FILE *fp;
   int writeLen;

   fp = fopen(filename, "wb");
   if(fp == NULL)
   {
      return ERR_INVALID_FILE;
   }

   writeLen = fwrite(pbuf, 1, length, fp);
  
   fclose(fp);

   if(length != writeLen)
   {
      return ERR_INVALID_FILE;
   }
   return ERR_NONE;
}

int readFromFile(char *filename, void* pbuf, int length)
{
   FILE *fp;
   int readLen;
   int fsize;
   fp = fopen(filename, "rb");
   if(fp == NULL)
   {
      return ERR_INVALID_FILE;
   }
   fseek(fp, 0, SEEK_END);
   fsize = ftell(fp);
   fseek(fp, 0, SEEK_SET);
   
   printf("fsize = %d \n", fsize);
   if(fsize != length)
   {
      return ERR_INVALID_FILE;
   }
   readLen = fread(pbuf, 1, fsize, fp);
   fclose(fp);

   if(readLen != length)
   {
      return ERR_INVALID_FILE;
   }
    return ERR_NONE;
}


typedef enum KBD_CONF_TYPE{
   KBD_CONF_RGB_EFFET_INDEX = 0,
   KBD_CONF_RGB_LIMIT,
   KBD_CONF_RGB_SPEED,
   KBD_CONF_LED_PRESET,
   KBD_CONF_SLEEP_TIMER
}KBD_CONF_TYPE_E;


unsigned char buffer[512];

unsigned char reportMatrix[] = {1, 1, 2, 2, 0, 0, 0, 0}; 
unsigned char reportKeyCode[] = {1, 1, 2, 1, 0, 0, 0, 0}; 
unsigned char jumpBootloader[] = {1, 1, 3, 0, 0, 0, 0, 0}; 

int setConfg(char *param, char *value)
{
   int status;
   int a;
   
   a = atoi(value);

   status = readConfg(buffer);
   if (status != ERR_NONE)
      return status;

   pkbdConf = (kbd_configuration_t *)buffer;

  if(strcasecmp(param, "rgbeffect") == 0)
   {
      pkbdConf->rgb_effect_index = a;
   }else if(strcasecmp(param, "rgblimit") == 0)
   {
      pkbdConf->rgb_limit = a;
   }else if(strcasecmp(param, "rgbspeed") == 0)
   {
      pkbdConf->rgb_speed = a;
   }else if(strcasecmp(param, "ledpreset") == 0)
   {
      pkbdConf->led_preset_index = a;
   }else if(strcasecmp(param, "sleep") == 0)
   {
      pkbdConf->sleeptimer = a;
   }else
   {
      status = ERR_INVALID_ARG;
   }
   if(status == ERR_NONE)
   {
      status = writeConfig(buffer);
   }

   return status;

}

int printcfg(char *pBuf, int valid)
{
   pkbdConf = (kbd_configuration_t *)pBuf;

   printf("ps2usb_mode : %d \n", pkbdConf->ps2usb_mode);
   printf("keymapLayerIndex : %d \n", pkbdConf->keymapLayerIndex);
   printf("swapCtrlCaps : %d \n", pkbdConf->swapCtrlCaps);
   printf("swapAltGui : %d \n", pkbdConf->swapAltGui);
   printf("led_preset_index : %d \n", pkbdConf->led_preset_index);
   printf("rgb_effect_index : %d \n", pkbdConf->rgb_effect_index);
   printf("rgb_chain : %d \n", pkbdConf->rgb_chain);
   printf("rgb_limit : %d \n", pkbdConf->rgb_limit);
   printf("rgb_speed : %d \n", pkbdConf->rgb_speed);
   printf("matrix_debounce : %d \n", pkbdConf->matrix_debounce);
   printf("sleeptimer : %d \n", pkbdConf->sleeptimer);

   printf("sizeof kdbConf = %d\n", sizeof(kbdConf));
   return ERR_NONE;
}



int main(int argc, char **argv)
{

   char *pbuf;
   int status;
   pbuf = buffer;
     
   if(strcasecmp((char *)argv[1], "cmd") == 0)
   {
      if(strcasecmp((char *)argv[2], "boot") == 0)
      {
         status = sendDBGCmd(jumpBootloader);
      }else if(strcasecmp((char *)argv[2], "matrix") == 0)
      {
         status = sendDBGCmd(reportMatrix);
      }else if(strcasecmp((char *)argv[2], "keycode") == 0)
      {
         status = sendDBGCmd(reportKeyCode);
      }else
      {
         status = ERR_INVALID_ARG;
      }

   }else if(strcasecmp((char *)argv[1], "printcfg") == 0)
   {
      if (argv[2] != NULL)
      {
         status = readFromFile(argv[2], pbuf,128);
      }else
      {
         status = readConfg(pbuf);
      }
      if(status == ERR_NONE)
      {
         status = printcfg(pbuf, 1);
      }else
      {
         return ERR_INVALID_ARG;
      }
   }else if(strcasecmp((char *)argv[1], "readcfg") == 0)
   {
      if (argv[2] == NULL)
            return ERR_INVALID_ARG;
      status = readConfg(pbuf);
      if(status == ERR_NONE)
      {
         status = write2File(argv[2], pbuf,128);
      }
   }else if(strcasecmp((char *)argv[1], "writecfg") == 0)
   {
      if (argv[2] == NULL)
            return ERR_INVALID_ARG;
      
      status = readFromFile(argv[2], pbuf,128);
      if(status == ERR_NONE)
      {
         status = writeConfig(pbuf);
      }
   }else if(strcasecmp((char *)argv[1], "readkey") == 0)
   {
      if (argv[2] == NULL)
            return ERR_INVALID_ARG;
      status = readKeymap(pbuf);
      if(status == ERR_NONE)
      {
         status = write2File(argv[2], pbuf,480);
      }
   }else if(strcasecmp((char *)argv[1], "writekey") == 0)
   {
      if (argv[2] == NULL)
            return ERR_INVALID_ARG;
      
      status = readFromFile(argv[2], pbuf,480);
      if(status == ERR_NONE)
      {
         status = writeKeymap(pbuf);
      }

   }else if(strcasecmp((char *)argv[1], "set") == 0)
   {
      if (argv[2] == NULL)
              return ERR_INVALID_ARG;
        
        status = setConfg(argv[2], argv[3]);

   }else
   {
      printf("USAGE : l3cmd [cmd] [filename]\n");

   }
   printf("status = %d\n", status);
}

