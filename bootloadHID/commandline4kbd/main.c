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
        fprintf(stderr, "Error uploading data block: %s\n", usbErrorMessage(err));
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

    printf("Send Succeed !\n");

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
    unsigned char rgb_preset_index;               // RGB effect preset
    unsigned char rgb_chain;                      // RGB5050 numbers (H/W dependent)
    unsigned char rgb_preset[MAX_RGB_CHAIN][3];   // Chain color
    rgb_effect_param_type rgb_effect_param[RGB_EFFECT_MAX]; // RGB effect parameter
    unsigned char  rgb_limit;
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


int setConfig(void)
{
    static unsigned char tmpled_preset[3][5] = {{LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_FADING, LED_EFFECT_FADING},
                    {LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_FADING_PUSH_ON, LED_EFFECT_FADING_PUSH_ON},
                    {LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_NONE, LED_EFFECT_PUSH_OFF, LED_EFFECT_PUSH_OFF}};


    static unsigned char tmprgp_preset[MAX_RGB_CHAIN][3] = {{200,0,0},{200,0,50},{200,0,100},{200,0,150},{200,0,200},
                    {0,200,0},{0,200,50},{0,200,100},{0,200,150},{0,200,200},
                    {0,0,200},{50,0,200},{100,0,200},{150,0,200},{200,0,200},
                    {200,0,0},{200,50,0},{200,100,0},{200,150,0},{200,200,0}};


    kbdConf.ps2usb_mode = 1;
    kbdConf.keymapLayerIndex = 0;
    kbdConf.swapCtrlCaps = 0;
    kbdConf.swapAltGui = 0;
    kbdConf.led_preset_index = 2;
    memcpy(kbdConf.led_preset, tmpled_preset, sizeof(kbdConf.led_preset));
    kbdConf.rgb_preset_index = 1;
    kbdConf.rgb_chain = MAX_RGB_CHAIN;
    memcpy(kbdConf.rgb_preset, tmprgp_preset, sizeof(kbdConf.rgb_preset));


    memcpy(cmdData.data, &kbdConf, sizeof(kbdConf)); 

    if(sendCmd(&cmdData))
        return 1;
}
unsigned char buffer[512];

int main(int argc, char **argv)
{
char    *file = NULL;
char    a = 0, i = 0;
volatile int     sleep = 0xFFFF;

int len;
#if 0
    if(argc < 2){
        printUsage(argv[0]);
        return 1;
    }
    if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0){
        printUsage(argv[0]);
        return 1;
    }
    if(strcmp(argv[1], "-r") == 0){
        leaveBootLoader = 1;
        if(argc >= 3){
            file = argv[2];
        }
    }else{
        file = argv[1];
    }
//    startAddress = sizeof(dataBuffer);
//    endAddress = 0;
    if(file != NULL){   // an upload file was given, load the data
        memset(dataBuffer, -1, sizeof(dataBuffer));
        if(parseIntelHex(file, dataBuffer))
            return 1;
        if(startAddress[0] == endAddress[0]){
            fprintf(stderr, "No data in input file, exiting.\n");
            return 0;
        }
    }
#if 0
{

FILE    *output;
output  = fopen("uploaddata.bin", "w");
fwrite(dataBuffer, 1, sizeof(dataBuffer),output);
fclose(output);
return 0;
}
#endif
    // if no file was given, endAddress is less than startAddress and no data is uploaded
#endif

#if 0
    setConfig();
    return 1;
#endif

#if 0 //def HIDDEBUG
    HIDCMD_debug_t dbgCmd;
    unsigned char buffer[8];


if (strtoi(argv[1], 10) == CMD_DEBUG)
    buffer[0] = 1;      // Report ID
    buffer[1] = strtoi(argv[1], 10);    // Command
    buffer[2] = strtoi(argv[2], 10);    // SubCmd
    buffer[3] = strtoi(argv[3], 10);    // arg3
    buffer[4] = strtoi(argv[4], 10);    // arg4
    buffer[5] = strtoi(argv[5], 10);    // arg5
    buffer[6] = strtoi(argv[6], 10);    // arg6
    buffer[7] = strtoi(argv[7], 10);    // arg7
    

    sendDBGCmd(buffer);
    return 1;
#endif

#if 0
    cmdData.cmd = strtoi(argv[1], 10);
    cmdData.index = strtoi(argv[2], 10);
    cmdData.length = strtoi(argv[3], 10);
    cmdData.data[0] = strtoi(argv[4], 10);

    if(receiveCmd(&cmdData))
        return 1;
return 0;
    pkbdConf = (kbd_configuration_t *)cmdData.data;
    pkbdConf->led_preset_index = 1;
      
    if(sendCmd(&cmdData))
        return 1;
#endif  

    FILE *fp = fopen("tmpKeymap.bin", "rb");
    int fsize;
    fseek(fp, 0, SEEK_END);         // 파일포인터를 파일의 끝으로 이동
    fsize = ftell(fp);              // 현재 파일포인터를 읽으면 파일크기가 됨
       
    fseek(fp, 0, SEEK_SET);         // 파일포인터를 파일의 끝으로 이동

    printf("fsize = %d \n", fsize);
    char *pbuf;
    int readLen;
    pbuf = buffer;
    
    readLen = fread(pbuf, 1, fsize, fp);
    
    printf("read len = %d \n", readLen);
    cmdData.cmd = CMD_KEYMAP;
        

    for (i = 0; i < 4; i++)
    {
        cmdData.index = i;
        memcpy(cmdData.data, &buffer[120*i], 120);
        if(sendCmd(&cmdData))
        {
            fclose(fp);
            return 1;

        }
    }
    fclose(fp);
    return 0;
}

/* ------------------------------------------------------------------------- */


