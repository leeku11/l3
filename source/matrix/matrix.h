#define SCAN_DIRTY          0x01 << 2
#define SCAN_PUSHED         0x01 << 1
#define SCAN_RELEASED       0x01 << 0
#define SCAN_CHANGED        (SCAN_PUSHED | SCAN_RELEASED)

#define DEBOUNCE_MAX        4           // 10 is 5ms at 12MHz XTAL


extern uint32_t MATRIX[MAX_COL];
extern uint32_t curMATRIX[MAX_COL];
extern int8_t debounceMATRIX[MAX_COL][MAX_ROW];
extern uint8_t svkeyidx[MAX_COL][MAX_ROW];


extern uint8_t matrixFN[MAX_LAYER];           // (col << 4 | row)
extern uint8_t layer;
extern uint8_t ledPortBackup;
extern uint8_t kbdsleepmode;
extern uint8_t isLED3000;


extern void keymap_init(void);
extern uint8_t processFNkeys(uint8_t keyidx);
extern uint8_t getLayer(uint8_t FNcolrow);
extern uint8_t scankey(void);
extern uint8_t scanmatrix(void);

