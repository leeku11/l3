



#define MOUSE_ENABLE 1






/* report id for Keyboard */
#define REPORT_ID_SYSTEM    3
#define REPORT_ID_CONSUMER  4

/* report id for HID command */
#define REPORT_ID_CMD       1
#define REPORT_ID_DATA      2



#define HID_REPORT_KEBOARD  0x0200


#define HID_BOOT_CMD_LEN    0x07
#define HID_BOOT_DATA_LEN   0x83


extern uint8_t clearReportBuffer(void);
extern uint8_t buildHIDreports(uint8_t keyidx);


extern uint8_t reportMatrix;




typedef enum HID_DEBUG_SUB_CMD{
    HID_DEBUG_LED,
    HID_DEBUG_RGB,
    HID_DEBUG_KEYMAPER,
    HID_DEBUG_JMP_BOOTLOADER
}HID_DEBUG_SUB_CMD_E;


typedef struct report_extra{
    uint8_t  report_id;
    uint16_t usage;
}report_extra_t;



typedef enum
{
    CMD_DEBUG = 1,
    CMD_CONFIG = 2,
    CMD_KEYMAP = 3,
    CMD_MACRO  = 4
}BOOT_HID_CMD;

#define HID_REPORT_CMD      (0x0300 | REPORT_ID_CMD)
#define HID_REPORT_DATA     (0x0300 | REPORT_ID_DATA)


typedef struct HIDCMD_debug{
    uint8_t reportID;
    uint8_t cmd;
    uint8_t subcmd;
    uint8_t arg3;
    uint8_t arg4;
    uint8_t arg5;
    uint8_t arg6;
    uint8_t arg7;
}HIDCMD_debug_t;


typedef struct HIDCMD_config{
    uint8_t reportID;
    uint8_t cmd;
    uint8_t rsvd2;
    uint8_t rsvd3;   
    uint8_t rsvd4;
    uint8_t rsvd5;
    uint8_t rsvd6;
    uint8_t rsvd7;
}HIDCMD_config_t;


typedef struct HIDCMD_keymap{
    uint8_t reportID;
    uint8_t cmd;
    uint8_t index;
    uint8_t row;   
    uint8_t col;
    uint8_t reportMatrix;
    uint8_t rsvd6;
    uint8_t rsvd7;
}HIDCMD_keymap_t;

typedef struct HIDCMD_macro{
    uint8_t reportID;
    uint8_t cmd;
    uint8_t index;
    uint8_t length;   
    uint8_t rsvd4;
    uint8_t rsvd5;
    uint8_t rsvd6;
    uint8_t rsvd7;
}HIDCMD_macro_t;

typedef union HIDcommand
{
  uint8_t   byte[8];
  HIDCMD_debug_t debug;
  HIDCMD_config_t config;
  HIDCMD_keymap_t keymap;
  HIDCMD_macro_t macrodata;
} HIDcommand_t;

typedef struct HIDdata{
    uint8_t reportID;
    uint8_t cmd;
    uint8_t parm0;
    uint8_t parm1;   
    uint8_t data[128];
}HIDData_t;



