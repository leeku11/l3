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

uint8_t interfaceReady = 0;


/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

static uint8_t keyboardReport[8]; ///< buffer for HID reports
static uint8_t oldReportBuffer[8]; ///< buufer for HID reports save on Overflower
uint8_t reportIndex; // keyboardReport[0] contains modifiers



report_extra_t extraReport;
report_extra_t oldextraReport;



static uint8_t idleRate = 0;        ///< in 4ms units
static uint8_t protocolVer = 1; ///< 0 = boot protocol, 1 = report protocol
uint8_t expectReport = 0;       ///< flag to indicate if we expect an USB-report

AppPtr_t Bootloader = (void *)BOOTLOADER_ADDRESS; 


extern uint8_t swapAltGui;

#define MOUSE_ENABLE 1

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
#if 0
    /* mouse */
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, REPORT_ID_MOUSE,         //   REPORT_ID (1)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
                                   // ----------------------------  Buttons
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x05,                    //     USAGE_MAXIMUM (Button 5)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x05,                    //     REPORT_COUNT (5)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x03,                    //     REPORT_SIZE (3)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
                                   // ----------------------------  X,Y position
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
                                   // ----------------------------  Vertical wheel
    0x09, 0x38,                    //     USAGE (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)        - reset physical
    0x45, 0x00,                    //     PHYSICAL_MAXIMUM (0)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
                                   // ----------------------------  Horizontal wheel
    0x05, 0x0c,                    //     USAGE_PAGE (Consumer Devices)
    0x0a, 0x38, 0x02,              //     USAGE (AC Pan)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0,                          // END_COLLECTION

#else
    /* consumer */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)

    0x85, REPORT_ID_INFO,         //   REPORT_ID (1)
    0x95, 0x07,                    //   REPORT_COUNT (7)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, REPORT_ID_BOOT,          //   REPORT_ID (2)
    0x95, 0x83,              		//   REPORT_COUNT (131)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0,                           // END_COLLECTION
#endif

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

/*
    debug("usbFunctionDescriptor: ");
    debug_hex(rq->bmRequestType); debug(" ");
    debug_hex(rq->bRequest); debug(" ");
    debug_hex16(rq->wValue.word); debug(" ");
    debug_hex16(rq->wIndex.word); debug(" ");
    debug_hex16(rq->wLength.word); debug("\n");
*/
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
    //debug("desc len: "); debug_hex(len); debug("\n");
    return len;
}


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
            usbMsgPtr = (usbMsgPtr_t)keyboardReport;
            return sizeof(keyboardReport);
        } else if (rq->bRequest == USBRQ_HID_SET_REPORT) {
            if (rq->wValue.word == 0x0200 && rq->wIndex.word == 0) {
                // We expect one byte reports
                expectReport = 1;
            }
            return 0xff; // Call usbFunctionWrite with data
        } else if (rq->bRequest == USBRQ_HID_GET_IDLE) {
            usbMsgPtr = (usbMsgPtr_t)&idleRate;
            return 1;
        } else if (rq->bRequest == USBRQ_HID_SET_IDLE) {
            idleRate = rq->wValue.bytes[1];
            if(idleRate > 0)    // not windows
            {
               swapAltGui = 1;
            }else
            {
               swapAltGui = 0;
            }
            DEBUG_PRINT(("idleRate = %2x\n", idleRate));
        } else if (rq->bRequest == USBRQ_HID_GET_PROTOCOL) {
            if (rq->wValue.bytes[1] < 1) {
                protocolVer = rq->wValue.bytes[1];
            }
        } else if(rq->bRequest == USBRQ_HID_SET_PROTOCOL) {
            usbMsgPtr = (usbMsgPtr_t)&protocolVer;
            return 1;
        }
    } else {
        // no vendor specific requests implemented
    }
    return 0;
}

/**
 * The write function is called when LEDs should be set. Normally, we get only
 * one byte that contains info about the LED states.
 * \param data pointer to received data
 * \param len number ob bytes received
 * \return 0x01
 */
uint8_t usbFunctionWrite(uchar *data, uchar len) {
    if (expectReport && (len == 1)) {
        LEDstate = data[0]; // Get the state of all 5 LEDs
        led_3lockupdate(LEDstate);
    }
    expectReport = 0;
    return 0x01;
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


uint8_t usbRollOver = 0;


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
        }
        
    }    
    retval |= 0x01; // must have been a change at some point, since debounce is done
    return retval;
}

#ifdef DEBUG
uint8_t toggle1 = 0;

void dumpreportBuffer(void)
{
    uint8_t i;

    DEBUG_PRINT(("RBuf "));
    for (i = 0; i < sizeof(keyboardReport); i++)
    {
        DEBUG_PRINT(("%02x", keyboardReport[i]));
    }
    DEBUG_PRINT(("\n"));
}
#endif

uint8_t usbmain(void) {
    uint8_t updateNeeded = 0;
    uint8_t idleCounter = 0;
    uint32_t interfaceCount = 0;
#ifdef SUPPORT_I2C
    uint16_t test_cnt = 0, test_cnt2 = 0;
    uint8_t duty = 0;
#endif
	interfaceReady = 0;

    DEBUG_PRINT(("USB\n"));
    

    cli();
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    _delay_ms(20);
    usbDeviceConnect();
    sei();
    
    wdt_enable(WDTO_2S);

    while (1) {
        // main event loop

        if(interfaceReady == 0 && interfaceCount++ > 12000){
		   //Reset_AVR();
			//break;
		}

#ifdef SUPPORT_I2C
        if(test_cnt++%2000 == 0)
        {
			testI2C(test_cnt2++, duty++);
        }
#endif // SUPPORT_I2C
                
        wdt_reset();
        usbPoll();

        updateNeeded = scankey();   // changes?
        if (updateNeeded == 0)      //debounce
        {
            
            continue;
        }
        if (idleRate == 0)                  // report only when the change occured
        {
            if (cmpReportBuffer() == 0)     // exactly same status?
            {
                updateNeeded &= ~(0x01);   // clear key report
            }
            if (bufcmp((uint8_t *)&oldextraReport, (uint8_t *)&extraReport, sizeof(extraReport)) == 0)
            {
                updateNeeded &= ~(0x04);    // clear consumer report
            }
        }

        if (TIFR & (1 << TOV0)) 
        {
            TIFR = (1 << TOV0); // reset flag
            if (idleRate != 0) 
            { // do we need periodic reports?
                if(idleCounter > 4)
                { // yes, but not yet
                    idleCounter -= 5; // 22ms in units of 4ms
                }else 
                { // yes, it is time now
                    updateNeeded |= 0x01;
                    idleCounter = idleRate;
                }
            }
        }
        // if an update is needed, send the report
      
        if((updateNeeded & 0x01)  && usbInterruptIsReady())
        {
            usbSetInterrupt(keyboardReport, sizeof(keyboardReport));
            saveReportBuffer();
        }

        if((updateNeeded & 0x04)  && usbInterruptIsReady3())
        {
            usbSetInterrupt3((uchar *)&extraReport, sizeof(extraReport));
            memcpy(&oldextraReport, &extraReport, sizeof(extraReport));
        }

    }

    wdt_disable();
	USB_INTR_ENABLE &= ~(1 << USB_INTR_ENABLE_BIT);
    return 0;
}
