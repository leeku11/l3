
#define EEPADDR_LEDMODE_INDEX   (uint8_t *)11
#define EEPADDR_USBPS2_MODE  (uint8_t *)12
#define EEPADDR_KEYLAYER     (uint8_t *)13
#define EEPADDR_SWAPCTRLCAPS (uint8_t *)14
#define EEPADDR_SWAPALTGUI   (uint8_t *)15

#define EEPADDR_MACRO_SET    (uint8_t *)100   // 100~152 (1Byte x 52)

#define EEPADDR_LED_MODE     (uint8_t *)200

#define KEYMAP_LAYER0         0x6300
#define KEYMAP_LAYER1         0x6400
#define KEYMAP_LAYER2         0x6500
#define KEYMAP_LAYER3         0x6600
#define LEDMODE_ADDRESS       0x6800      // 0x9800:0x98FF  (256Bytes)
#define MACRO_ADDR_START      0x4400     // 0x4400 ~ 0x5FFF  (7KBytes - 256B x 28)
