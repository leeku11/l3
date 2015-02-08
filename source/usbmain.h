

typedef struct report_extra{
    uint8_t  report_id;
    uint16_t usage;
}report_extra_t;



/* report id for Keyboard */
#define REPORT_ID_SYSTEM    3
#define REPORT_ID_CONSUMER  4

/* report id for HID command */
#define REPORT_ID_CMD       1
#define REPORT_ID_DATA      2


extern uint8_t clearReportBuffer(void);
extern uint8_t buildHIDreports(uint8_t keyidx);


extern uint8_t reportMatrix;



