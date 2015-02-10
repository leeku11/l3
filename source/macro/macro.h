#define MAX_MACRO_INDEX     44
#define MAX_MACRO_LEN       256





/**
 * This structure can be used as a container for a single 'key'. It consists of
 * the key-code and the modifier-code.
 */
typedef struct {
    uint8_t mode;
    uint8_t key;
} Key;



extern uint8_t initMacroAddr(void);

extern void playMacroUSB(uint8_t macrokey);
extern void playMacroPS2(uint8_t macrokey);
extern void recordMacro(uint8_t macrokey);
extern void sendMatrix(char col, char row);
void resetMacro(void);


extern uint8_t flash_writeinpage (uint8_t *data, uint16_t addr);
