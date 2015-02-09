#define SCAN_DIRTY          0x01 << 2
#define SCAN_PUSHED         0x01 << 1
#define SCAN_RELEASED       0x01 << 0
#define SCAN_CHANGED        (SCAN_PUSHED | SCAN_RELEASED)

#define DEBOUNCE_MAX        4           // 10 is 5ms at 12MHz XTAL

typedef enum HID_REPORT_MODE{
    HID_REPORT_KEY = 1,
    HID_REPORT_MATRIX = 2,
}HID_REPORT_MODE_E;


#define keylayer(layer)  (KEYMAP_LAYER0 + (0x100 * layer))

extern uint32_t MATRIX[MAX_COL];
extern uint32_t curMATRIX[MAX_COL];
extern int8_t debounceMATRIX[MAX_COL][MAX_ROW];
extern uint8_t svkeyidx[MAX_COL][MAX_ROW];
extern uint8_t  currentLayer[MAX_COL][MAX_ROW];

extern uint8_t matrixFN[MAX_LAYER];           // (col << 4 | row)
//extern uint8_t layer;
extern uint8_t kbdsleepmode;

extern void keymap_init(void);
extern uint8_t processFNkeys(uint8_t keyidx);
extern uint8_t getLayer(uint8_t FNcolrow);
extern uint8_t scankey(void);
extern uint8_t scanmatrix(void);
extern uint8_t cntKey(uint8_t keyidx, uint8_t clearmask);

