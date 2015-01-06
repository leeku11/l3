#define MAX_MACRO_INDEX     32
#define MAX_MACRO_LEN       256

extern uint8_t initMacroAddr(void);

extern void playMacroUSB(uint8_t macrokey);
extern void playMacroPS2(uint8_t macrokey);
extern void recordMacro(uint8_t macrokey);
void resetMacro(void);


extern int8_t flash_writeinpage (uint8_t *data, unsigned long addr);
