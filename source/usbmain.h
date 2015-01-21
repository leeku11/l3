

typedef struct report_extra{
    uint8_t  report_id;
    uint16_t usage;
}report_extra_t;



/* report id */
#define REPORT_ID_INFO      1
#define REPORT_ID_BOOT      2
#define REPORT_ID_SYSTEM    3
#define REPORT_ID_CONSUMER  4


extern uint8_t clearReportBuffer(void);
extern uint8_t buildHIDreports(uint8_t keyidx);




