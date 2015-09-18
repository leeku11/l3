#include "global.h"
#include "timer.h"
//#include "keysta.h"
#include "keymap.h"
#include "print.h"
#include "led.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <util/delay.h>     /* for _delay_ms() */
#include <string.h>

#include "usbdrv.h"
#include "matrix.h"
#include "usbmain.h"
#include "hwport.h"
#include "hwaddress.h"
#ifdef SUPPORT_TINY_CMD
#include "tinycmdapi.h"
#endif

#define HID_DEBUG_CMD



/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */
uint8_t interfaceReady = 0;

static uint8_t keyboardReport[8]; ///< buffer for HID reports
static uint8_t oldReportBuffer[8]; ///< buufer for HID reports save on Overflower
uint8_t reportIndex; // keyboardReport[0] contains modifiers
uint8_t reportMatrix = 0 ;
report_extra_t extraReport;
report_extra_t oldextraReport;
static uint8_t idleRate = 0;        ///< in 4ms units
static uint16_t cntIdelRate = 0;
static uint8_t protocolVer = 1; ///< 0 = boot protocol, 1 = report protocol
uint8_t expectReport = 0;       ///< flag to indicate if we expect an USB-report
AppPtr_t Bootloader = (void *)BOOTLOADER_ADDRESS; 
uint8_t bootRxRemains;
HIDcommand_t hidCmd;
HIDData_t hidData;
uint8_t gbootCmdoffset;
uint8_t version[] = "L150205";          // must be length of 7 bytes    HID report size
uint8_t configUpdated = 0;

uint16_t sleepTimeOut;
extern uint8_t kbdsleepmode;


MODIFIERS modifierBitmap[] = {
    MOD_NONE ,
    MOD_CONTROL_LEFT ,
    MOD_SHIFT_LEFT ,
    MOD_ALT_LEFT ,
    MOD_GUI_LEFT ,
    MOD_CONTROL_RIGHT ,
    MOD_SHIFT_RIGHT ,
    MOD_ALT_RIGHT ,
    MOD_GUI_RIGHT
};



/*------------------------------------------------------------------*
 * Descriptors                                                      *
 *------------------------------------------------------------------*/

/*
 * Report Descriptor for keyboard
 *
 * from an example in HID spec appendix
 */
PROGMEM uchar keyboard_hid_report[] = {
    0x05, 0x01,          // Usage Page (Generic Desktop),
    0x09, 0x06,          // Usage (Keyboard),
    0xA1, 0x01,          // Collection (Application),

    0x75, 0x01,          //   Report Size (1),
    0x95, 0x08,          //   Report Count (8),
    0x05, 0x07,          //   Usage Page (Key Codes),
    0x19, 0xE0,          //   Usage Minimum (224),
    0x29, 0xE7,          //   Usage Maximum (231),
    0x15, 0x00,          //   Logical Minimum (0),
    0x25, 0x01,          //   Logical Maximum (1),
    0x81, 0x02,          //   Input (Data, Variable, Absolute), ;Modifier byte
    0x95, 0x01,          //   Report Count (1),
    0x75, 0x08,          //   Report Size (8),
    0x81, 0x03,          //   Input (Constant),                 ;Reserved byte
    0x95, 0x05,          //   Report Count (5),
    0x75, 0x01,          //   Report Size (1),
    0x05, 0x08,          //   Usage Page (LEDs),
    0x19, 0x01,          //   Usage Minimum (1),
    0x29, 0x05,          //   Usage Maximum (5),
    0x91, 0x02,          //   Output (Data, Variable, Absolute), ;LED report
    0x95, 0x01,          //   Report Count (1),
    0x75, 0x03,          //   Report Size (3),
    0x91, 0x03,          //   Output (Constant),                 ;LED report padding
    0x95, 0x06,          //   Report Count (6),
    0x75, 0x08,          //   Report Size (8),
    0x15, 0x00,          //   Logical Minimum (0),
    0x25, 0xFF,          //   Logical Maximum(255),
    0x05, 0x07,          //   Usage Page (Key Codes),
    0x19, 0x00,          //   Usage Minimum (0),
    0x29, 0xFF,          //   Usage Maximum (255),
    0x81, 0x00,          //   Input (Data, Array),
 
   0xc0                 // End Collection
};




/*
 * Report Descriptor for mouse
 *
 * Mouse Protocol 1, HID 1.11 spec, Appendix B, page 59-60, with wheel extension
 * http://www.microchip.com/forums/tm.aspx?high=&m=391435&mpage=1#391521
 * http://www.keil.com/forum/15671/
 * http://www.microsoft.com/whdc/device/input/wheel.mspx
 */
PROGMEM uchar mouse_hid_report[] = {
    /* Boot HID */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)

    0x85, REPORT_ID_CMD,          //   REPORT_ID (1)
    0x95, 0x07,                    //   REPORT_COUNT (7)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, REPORT_ID_DATA,          //   REPORT_ID (2)
    0x95, 0x83,              		//   REPORT_COUNT (131)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0xc0,                           // END_COLLECTION

    /* system control */
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x80,                    // USAGE (System Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, REPORT_ID_SYSTEM,        //   REPORT_ID (2)
    0x15, 0x01,                    //   LOGICAL_MINIMUM (0x1)
    0x25, 0xb7,                    //   LOGICAL_MAXIMUM (0xb7)
    0x19, 0x01,                    //   USAGE_MINIMUM (0x1)
    0x29, 0xb7,                    //   USAGE_MAXIMUM (0xb7)
    0x75, 0x10,                    //   REPORT_SIZE (16)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x00,                    //   INPUT (Data,Array,Abs)
    0xc0,                          // END_COLLECTION
    /* consumer */
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, REPORT_ID_CONSUMER,      //   REPORT_ID (3)
    0x15, 0x01,                    //   LOGICAL_MINIMUM (0x1)
    0x26, 0x9c, 0x02,              //   LOGICAL_MAXIMUM (0x29c)
    0x19, 0x01,                    //   USAGE_MINIMUM (0x1)
    0x2a, 0x9c, 0x02,              //   USAGE_MAXIMUM (0x29c)
    0x75, 0x10,                    //   REPORT_SIZE (16)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x00,                    //   INPUT (Data,Array,Abs)
    0xc0,                          // END_COLLECTION
};




#if USB_CFG_DESCR_PROPS_CONFIGURATION
PROGMEM const char usbDescriptorConfiguration[] = {    /* USB configuration descriptor */
    9,          /* sizeof(usbDescriptorConfiguration): length of descriptor in bytes */
    USBDESCR_CONFIG,    /* descriptor type */
#if MOUSE_ENABLE
    9 + (9 + 9 + 7) + (9 + 9 + 7),
#else    
    9 + (9 + 9 + 7),
#endif
    0,
    //18 + 7 * USB_CFG_HAVE_INTRIN_ENDPOINT + 7 * USB_CFG_HAVE_INTRIN_ENDPOINT3 + 9, 0,
                /* total length of data returned (including inlined descriptors) */
    2,          /* number of interfaces in this configuration */
    1,          /* index of this configuration */
    0,          /* configuration name string index */
#if USB_CFG_IS_SELF_POWERED
    (1 << 7) | USBATTR_SELFPOWER,       /* attributes */
#else
    (1 << 7),                           /* attributes */
#endif
    USB_CFG_MAX_BUS_POWER/2,            /* max USB current in 2mA units */

    /*
     * Keyboard interface
     */
    /* Interface descriptor */
    9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
    USBDESCR_INTERFACE, /* descriptor type */
    0,          /* index of this interface */
    0,          /* alternate setting for this interface */
    USB_CFG_HAVE_INTRIN_ENDPOINT, /* endpoints excl 0: number of endpoint descriptors to follow */
    USB_CFG_INTERFACE_CLASS,
    USB_CFG_INTERFACE_SUBCLASS,
    USB_CFG_INTERFACE_PROTOCOL,
    0,          /* string index for interface */
    /* HID descriptor */
    9,          /* sizeof(usbDescrHID): length of descriptor in bytes */
    USBDESCR_HID,   /* descriptor type: HID */
    0x01, 0x01, /* BCD representation of HID version */
    0x00,       /* target country code */
    0x01,       /* number of HID Report (or other HID class) Descriptor infos to follow */
    0x22,       /* descriptor type: report */
    sizeof(keyboard_hid_report), 0,  /* total length of report descriptor */
    /* Endpoint descriptor */
#if USB_CFG_HAVE_INTRIN_ENDPOINT    /* endpoint descriptor for endpoint 1 */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    (char)0x81, /* IN endpoint number 1 */
    0x03,       /* attrib: Interrupt endpoint */
    8, 0,       /* maximum packet size */
    USB_CFG_INTR_POLL_INTERVAL, /* in ms */
#endif

#if MOUSE_ENABLE
    /*
     * Mouse interface
     */
    /* Interface descriptor */
    9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
    USBDESCR_INTERFACE, /* descriptor type */
    1,          /* index of this interface */
    0,          /* alternate setting for this interface */
    USB_CFG_HAVE_INTRIN_ENDPOINT3, /* endpoints excl 0: number of endpoint descriptors to follow */
    0x03,       /* CLASS: HID */
    0,          /* SUBCLASS: none */
    0,          /* PROTOCOL: none */
    0,          /* string index for interface */
    /* HID descriptor */
    9,          /* sizeof(usbDescrHID): length of descriptor in bytes */
    USBDESCR_HID,   /* descriptor type: HID */
    0x01, 0x01, /* BCD representation of HID version */
    0x00,       /* target country code */
    0x01,       /* number of HID Report (or other HID class) Descriptor infos to follow */
    0x22,       /* descriptor type: report */
    sizeof(mouse_hid_report), 0,  /* total length of report descriptor */
#if USB_CFG_HAVE_INTRIN_ENDPOINT3   /* endpoint descriptor for endpoint 3 */
    /* Endpoint descriptor */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    (char)(0x80 | USB_CFG_EP3_NUMBER), /* IN endpoint number 3 */
    0x03,       /* attrib: Interrupt endpoint */
    8, 0,       /* maximum packet size */
    USB_CFG_INTR_POLL_INTERVAL, /* in ms */
#endif
#endif
};
#endif


USB_PUBLIC usbMsgLen_t usbFunctionDescriptor(struct usbRequest *rq)
{
    usbMsgLen_t len = 0;

    switch (rq->wValue.bytes[1]) {
#if USB_CFG_DESCR_PROPS_CONFIGURATION
        case USBDESCR_CONFIG:
            usbMsgPtr = (usbMsgPtr_t)usbDescriptorConfiguration;
            len = sizeof(usbDescriptorConfiguration);
            break;
#endif
        case USBDESCR_HID:
            switch (rq->wValue.bytes[0]) {
                case 0:
                    usbMsgPtr = (usbMsgPtr_t)(usbDescriptorConfiguration + 9 + 9);
                    len = 9;
                    break;
                case 1:
                    usbMsgPtr = (usbMsgPtr_t)(usbDescriptorConfiguration + 9 + (9 + 9 + 7) + 9);
                    len = 9;
                    break;
            }
            break;
        case USBDESCR_HID_REPORT:
            /* interface index */
            switch (rq->wIndex.word) {
                case 0:
                    usbMsgPtr = (usbMsgPtr_t)keyboard_hid_report;
                    len = sizeof(keyboard_hid_report);
                    break;
                case 1:
                    usbMsgPtr = (usbMsgPtr_t)mouse_hid_report;
                    len = sizeof(mouse_hid_report);
                    break;
            }
            break;
    }
    return len;
}
   

uint8_t txHIDCmd(void)
{
    switch(hidCmd.config.cmd)
    {
#ifdef HID_DEBUG_CMD
        case CMD_DEBUG:
        {
            // TODO : upload to HOST(PC) by hidData.data
            break;
        }
#endif
        case CMD_CONFIG:
        {
            hidData.reportID = 2;
            hidData.cmd = hidCmd.keymap.cmd;
            hidData.parm1 = sizeof(kbdConf);
            memcpy(hidData.data, &kbdConf, sizeof(kbdConf));
            break;
        }
        case CMD_KEYMAP :
        {
            hidData.reportID = 2;
            hidData.cmd = hidCmd.keymap.cmd;
            hidData.parm0 = hidCmd.keymap.index;
            hidData.parm1 = sizeof(currentLayer);

            eeprom_read_block(hidData.data, (void *)(EEPADDR_KEYMAP_LAYER0 + (0x80 * hidCmd.keymap.index)), sizeof(currentLayer));
            break;
        }
        case CMD_MACRO :
        {

        }
        break;
    }
    return 0;
}

void rxHIDCmd(void)
{
    switch(hidCmd.config.cmd)
    {
#ifdef HID_DEBUG_CMD
         case CMD_DEBUG:
            {
                // TODO : debugging action by hidData.data downloaded from HOST(PC)
            }
            break;
#endif            
        case CMD_CONFIG:
            {
                memcpy(&kbdConf, hidData.data, sizeof(kbdConf));
                configUpdated = 15;
            }
            break;
        case CMD_KEYMAP :
            {
               eeprom_update_block(hidData.data, EEPADDR_KEYMAP_LAYER0 + (0x80 * hidCmd.keymap.index), sizeof(currentLayer));
               keymap_init();
            }
            break;

        case CMD_MACRO :
            {


            }
            break;
    }
}


/**
 * This function is called whenever we receive a setup request via USB.
 * \param data[8] eight bytes of data we received
 * \return number of bytes to use, or 0xff if usbFunctionWrite() should be
 * called
 */

uint8_t usbFunctionSetup(uint8_t data[8]) {
    usbRequest_t *rq = (void *)data;
    
	interfaceReady = 1;
    
    usbMsgPtr = (usbMsgPtr_t)keyboardReport;
    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        // class request type
        if (rq->bRequest == USBRQ_HID_GET_REPORT) {

            // wValue: ReportType (highbyte), ReportID (lowbyte)
            // we only have one report type, so don't look at wValue
            if (rq->wValue.word == HID_REPORT_KEBOARD)
            {
                usbMsgPtr = (usbMsgPtr_t)keyboardReport;
                return sizeof(keyboardReport);
            }else if (rq->wValue.word == HID_REPORT_CMD)
            {
                usbMsgPtr = (usbMsgPtr_t)version;
                return sizeof(version);                
            }else if (rq->wValue.word == HID_REPORT_DATA)
            {
                txHIDCmd();
                usbMsgPtr = (usbMsgPtr_t)&hidData;
                return sizeof(hidData);
            }
        }

        else if (rq->bRequest == USBRQ_HID_SET_REPORT) {
            if (rq->wValue.word == HID_REPORT_KEBOARD && rq->wIndex.word == 0) {
                // We expect one byte reports
                expectReport = 1;
            }else if (rq->wValue.word == HID_REPORT_CMD)
            {
                expectReport = 2;
                bootRxRemains = HID_BOOT_CMD_LEN;
            }else if (rq->wValue.word == HID_REPORT_DATA)
            {
                expectReport = 3;
                bootRxRemains = HID_BOOT_DATA_LEN;
            }
                return USB_NO_MSG; // Call usbFunctionWrite with data
            }


        else if (rq->bRequest == USBRQ_HID_GET_IDLE) {
            usbMsgPtr = (usbMsgPtr_t)&idleRate;
            return 1;
            }


        else if (rq->bRequest == USBRQ_HID_SET_IDLE) {
            idleRate = rq->wValue.bytes[1];
            cntIdelRate = idleRate *2; 
            if(idleRate > 0)    // not windows
            {
               kbdConf.swapAltGui = 1;
            }else
            {
               kbdConf.swapAltGui = 0;
            }
            DEBUG_PRINT(("idleRate = %2x\n", idleRate));
        } 

        else if (rq->bRequest == USBRQ_HID_GET_PROTOCOL) {
            if (rq->wValue.bytes[1] < 1) {
                protocolVer = rq->wValue.bytes[1];
            }
        } 

        else if(rq->bRequest == USBRQ_HID_SET_PROTOCOL) {
            usbMsgPtr = (usbMsgPtr_t)&protocolVer;
            return 1;
        }
    } else {
        // no vendor specific requests implemented
    }
    return 0;
}

uint8_t usbFuncDebugCmdHandler(void)
{

    switch (hidCmd.debug.subcmd)
    {
        case HID_DEBUG_LED: 

            break;
        case HID_DEBUG_RGB: 

            break;
        case HID_DEBUG_KEYMAPER: 
            reportMatrix = hidCmd.debug.arg3;
            break;
        case HID_DEBUG_JMP_BOOTLOADER:
            eeprom_write_byte(EEPADDR_BOOTLOADER_ACT, 0xCA);
            Reset_AVR();
            break;
    }
    return 0;
}

volatile uint8_t gLEDstate;     ///< current state of the LEDs
extern uint32_t scankeycntms;
/**
 * The write function is called when LEDs should be set. Normally, we get only
 * one byte that contains info about the LED states.
 * \param data pointer to received data
 * \param len number ob bytes received
 * \return 0x01
 */
uint8_t usbFunctionWrite(uchar *data, uchar len) 
{
    uint8_t result = 1;
    uint8_t LEDstate;

    if ((expectReport) && (len == 1)) {
        
        LEDstate = (data[0] & LED_NUM) | (data[0] & LED_CAPS) | (data[0] & LED_SCROLL);

        if(kbdsleepmode == LED_POWERDOWN)
        {
           led_restore();
           kbdsleepmode = LED_ACTIVE;
           sleepTimeOut = 5000;
           scankeycntms = SCAN_COUNT_IN_MIN * kbdConf.sleepTimerMin;
        }


        if(gLEDstate != LEDstate)
        {
            gLEDstate = LEDstate;            // Get the state of all 5 LEDs
            led_3lockupdate(LEDstate);

           
        }
        expectReport = 0;
        result = 1;     // last block received
    }else if (expectReport == 2){
    
        memcpy(&hidCmd, &data[0], len);
#ifdef HID_DEBUG_CMD
        if(hidCmd.debug.cmd == CMD_DEBUG)
        {
            usbFuncDebugCmdHandler();

        }else
#endif
            {
            if(hidCmd.keymap.cmd == CMD_KEYMAP)
            {
                reportMatrix = hidCmd.keymap.reportMatrix;
            }
            result = 1;     // last block received
        }

    }else if (expectReport == 3)
    {
        
        if(bootRxRemains == HID_BOOT_DATA_LEN)
        {
            hidData.cmd = data[1];
            hidData.parm0 = data[2];
            hidData.parm1 = data[3];
            bootRxRemains -=4;
            len -=4;
            data += 4;
            gbootCmdoffset = 0;
            
        }
        
        for(;len>0; len--)
        {
            hidData.data[gbootCmdoffset++] = *data++;
            bootRxRemains--;
        }

        if(bootRxRemains < 8)
        {
            expectReport = 0;
            result = 1;     // last block received
            rxHIDCmd();
        }else
        {
            result = 0;
        }
    }
    return result;
}


uint8_t clearReportBuffer(void)
{   
    reportIndex = 2; // keyboardReport[0] contains modifiers
    memset(keyboardReport, 0, sizeof(keyboardReport)); // clear report
    extraReport.usage = 0;
    return 0;
}

uint8_t saveReportBuffer(void)
{
    memcpy(oldReportBuffer, keyboardReport, sizeof(keyboardReport));
    return 0;
}

uint8_t restoreReportBuffer(void)
{
    memcpy(keyboardReport, oldReportBuffer, sizeof(keyboardReport));
    return 0;
}


uint8_t bufcmp(uint8_t *s1, uint8_t *s2, uint8_t size)
{
    uint8_t result = 0;
    uint8_t i;
    
    for (i = 0; i < size; i++)
    {
        if(*s1 != *s2)
        {
            result = 1;
            break;
        }
        s1++;
        s2++;
        
    }
    return result;
}


uint8_t cmpReportBuffer(void)
{
    uint8_t result = 0;
    uint8_t *s1, *s2;
    uint8_t size;

    s1 = keyboardReport;
    s2 = oldReportBuffer;
    size = sizeof(keyboardReport);
    result = bufcmp(s1, s2, size);
    
    return result;
}


//extern void testTinyCmd(uint8_t keyidx);


uint8_t buildHIDreports(uint8_t keyidx)
{
    uint8_t retval = 0;
   
    if((keyidx > K_Modifiers) && (keyidx < K_Modifiers_end))
    {
        keyboardReport[0] |= modifierBitmap[keyidx-K_Modifiers];
    }else if((keyidx >= K_NEXT_TRK) && (keyidx <= K_MINIMIZE))
    {
        extraReport.report_id = REPORT_ID_CONSUMER;
        extraReport.usage = KEYCODE2CONSUMER(keyidx);
        
        retval |= 0x04;                                                             // continue decoding to get modifiers

    }else if((K_System <= keyidx) && (keyidx <= K_WAKE))
    {
        extraReport.report_id = REPORT_ID_SYSTEM;
        extraReport.usage = KEYCODE2SYSTEM(keyidx);
        
        retval |= 0x04;                                                             // continue decoding to get modifiers

    }else
    {
        if (reportIndex >= sizeof(keyboardReport))
        {   
            // too many keycodes
            memset(keyboardReport+2, ErrorRollOver, sizeof(keyboardReport)-2);
            retval |= 0x02;                                                             // continue decoding to get modifiers
        }else
        {
            keyboardReport[reportIndex] = keyidx; // set next available entry
            reportIndex++;

//            testTinyCmd(keyidx);
        }
        
    }    
    retval |= 0x01; // must have been a change at some point, since debounce is done
    return retval;
}



uint8_t checkSleep(void)
{
// Power down
    if(usbSofCount == 0)
    {
        if((--sleepTimeOut == 0))
        {
            kbdsleepmode = LED_POWERDOWN;
            sleepTimeOut = 1000;
            led_sleep();
        }
    }else 
    {
        sleepTimeOut = 1000;
        usbSofCount = 0;
    }
// Timer Sleep   
    if ((--scankeycntms == 0) && (kbdsleepmode == LED_ACTIVE))   // 5min
    {
        led_sleep();
        kbdsleepmode = LED_SLEEP;
    }
    return kbdsleepmode;
}
uint8_t usbmain(void) {
    uint8_t i, updated = 0;
    uint8_t updateNeeded = 0;
    uint8_t idleCounter = 0;
    uint16_t interfaceCount = 2500; // 1cycle takes 2ms approximatly 
    interfaceReady = 0;
    configUpdated = 0;

    cli();
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    sei();

    wdt_enable(WDTO_2S);

    while (1)       // main event loop
    {
        checkSleep();
        if((interfaceReady == 0) && !(--interfaceCount) && (kbdsleepmode != LED_POWERDOWN))
        {
            Reset_AVR();
            break;
        }
        wdt_reset();
        usbPoll();

        if (configUpdated)
        {
            if(--configUpdated == 0)
            {
                updateConf();
                keymap_init();
                led_restore();
                configUpdated = 0;
            }
        }
        updateNeeded = scankey();   // scan matrix
        
        if (updateNeeded == 0)      //debounce
        {
            continue;
        }

        if (idleRate == 0)                  // report only when the change occured
        {
            if (cmpReportBuffer() == 0)     // exactly same status?
            {
                updateNeeded &= ~(0x01);    // clear key report
            }
            if (bufcmp((uint8_t *)&oldextraReport, (uint8_t *)&extraReport, sizeof(extraReport)) == 0)
            {
                updateNeeded &= ~(0x04);    // clear consumer report
            }
        }

        if((idleRate != 0) && (--cntIdelRate == 1))      // report if idleRate(4ms Unit) expired
        {
            updateNeeded |= 1;
            cntIdelRate = idleRate*2;                    // 1 loop = 2ms  
        }

        // if an update is needed, send the report

        if((updateNeeded & 0x01)  && usbInterruptIsReady())
        {
            usbSetInterrupt(keyboardReport, sizeof(keyboardReport));
            saveReportBuffer();
            updated = 1;
        }
        if((updateNeeded & 0x04)  && usbInterruptIsReady3())
        {
            usbSetInterrupt3((uchar *)&extraReport, sizeof(extraReport));
            memcpy(&oldextraReport, &extraReport, sizeof(extraReport));
            updated = 1;
        }
        if (updated == 1)         // if any update?
        {
            if (kbdsleepmode == LED_SLEEP)
            {
                led_restore();
                kbdsleepmode = LED_ACTIVE;
            }else
            {
                scankeycntms = (uint32_t)SCAN_COUNT_IN_MIN * (uint32_t)kbdConf.sleepTimerMin;
            }
            updated = 0;
        }
         
    }
    return 0;
}
