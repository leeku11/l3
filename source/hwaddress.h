#ifndef HWADDRESS_H
#define HWADDRESS_H

#define EEPADDR_BOOTLOADER_ACT      (void *)0

#define EEPADDR_MACRO_SET           (uint8_t *)0x100    // 256-511 (1Byte x 52)
#define EEPADDR_KBD_CONF            (void *)0x200    // 512-767

#define EEPADDR_KEYMAP_LAYER0       (void *)0x300    // 768 -895
#define EEPADDR_KEYMAP_LAYER1       (void *)0x380    // 896 -1023
#define EEPADDR_KEYMAP_LAYER2       (void *)0x400    // 1024 - 1151
#define EEPADDR_KEYMAP_LAYER3       (void *)0x480    // 1152 - 1279

#define EEPSIZE_KEYMAP              0x80


#define EEP_KEYMAP_ADDR(layer)  (EEPADDR_KEYMAP_LAYER0 + (EEPSIZE_KEYMAP * layer))



#define MAX_RGB_CHAIN       20


typedef struct {
    uint8_t index;
    union {
       uint8_t max_rgb[3]; // grb
       struct {
          uint8_t g;
          uint8_t r;
          uint8_t b;
       }max;
    };
    uint8_t high_hold;
    uint8_t low_hold;
    uint8_t accel_mode;
    uint8_t dir;
    uint8_t level;
    uint8_t cnt;
    uint8_t hcnt;
    uint8_t lcnt;
} rgb_effect_param_type;


typedef struct kbd_conf
{
    uint8_t ps2usb_mode;                    // 0: PS2, 1: USB
    uint8_t keymapLayerIndex;                     // KEYMAP layer index;
    uint8_t swapCtrlCaps;                   // 1: Swap Capslock <-> Left Ctrl
    uint8_t swapAltGui;                     // 1: Swap Alt <-> GUI(WIN) 
    uint8_t led_preset_index;               // LED effect  preset index
    uint8_t led_preset[3][5];               // Block configuration for LED effect  preset
    uint8_t rgb_effect_index;               // RGB effect index
    uint8_t rgb_chain;                      // RGB5050 numbers (H/W dependent)
    uint8_t rgb_preset[MAX_RGB_CHAIN][3];   // Chain color
    rgb_effect_param_type rgb_effect_param; // RGB effect parameter
}kbd_configuration_t;


#define KEYMAP_LAYER0         0x6300
#define KEYMAP_LAYER1         0x6400
#define KEYMAP_LAYER2         0x6500
#define KEYMAP_LAYER3         0x6600
#define LEDMODE_ADDRESS       0x6800      // 0x9800:0x98FF  (256Bytes)
#define MACRO_ADDR_START      0x4400     // 0x4400 ~ 0x5FFF  (7KBytes - 256B x 28)

extern kbd_configuration_t kbdConf;
extern void updateConf(void);

#endif // HWADDRESS_H
