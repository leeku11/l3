#define SCAN_DIRTY          0x01 << 2
#define SCAN_PUSHED         0x01 << 1
#define SCAN_RELEASED       0x01 << 0
#define SCAN_CHANGED        (SCAN_PUSHED | SCAN_RELEASED)


#define STANDBY_LOOP    13000  // scan matix entry is 2.2msec @ 12Mh x-tal : 5min
#define SWAP_TIMER  0x400
#define KEYLOCK_TIMER  0x600
#define KEYLOCK_COUNTER_START 0x8000


#define DEBOUNCE_MAX        4       ////4 /*4*/ 6           // 10 is 5ms at 12MHz XTAL


#define MATRIX_COL_PORT     PORTC
#define MATRIX_COL_DDR      DDRC
#define MATRIX_COL_PIN0     2  

#define MATRIX_ROW_PORT0     PORTA
#define MATRIX_ROW_DDR0      DDRA
#define MATRIX_ROW_PIN0      PINA


#define MATRIX_ROW_PORT1     PORTB
#define MATRIX_ROW_DDR1      DDRB
#define MATRIX_ROW_PIN1      PINB


#define MATRIX_ROW_PORT2     PORTD
#define MATRIX_ROW_DDR2      DDRD
#define MATRIX_ROW_PIN2      PIND


typedef enum HID_REPORT_MODE{
    HID_REPORT_KEY = 1,
    HID_REPORT_MATRIX = 2,
}HID_REPORT_MODE_E;


typedef enum KEY_LAYER_NUM{
    KEY_LAYER_0,
    KEY_LAYER_1,
    KEY_LAYER_2,
    KEY_LAYER_FN
}KEY_LAYER_NUM_E;


extern uint32_t MATRIX[MATRIX_MAX_ROW];
extern uint32_t curMATRIX[MATRIX_MAX_ROW];
extern int8_t debounceMATRIX[MATRIX_MAX_ROW][MATRIX_MAX_COL];
extern uint8_t svkeyidx[MATRIX_MAX_ROW][MATRIX_MAX_COL];
extern uint8_t  currentLayer[MATRIX_MAX_ROW][MATRIX_MAX_COL];

extern uint8_t matrixFN[MAX_LAYER];           // (col << 4 | row)
//extern uint8_t layer;
extern uint8_t kbdsleepmode;

extern void keymap_init(void);
extern uint8_t processFNkeys(uint8_t keyidx);
extern uint8_t getLayer(uint8_t FNcolrow);
extern uint8_t scankey(void);
extern uint8_t scanmatrix(void);
extern uint8_t cntKey(uint8_t keyidx, uint8_t clearmask);

