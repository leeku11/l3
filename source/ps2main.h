

#define STA_NORMAL              0
#define STA_RXCHAR              1
#define STA_WAIT_SCAN_SET       2
#define STA_WAIT_SCAN_REPLY     3
#define STA_WAIT_ID             4
#define STA_WAIT_ID1            5
#define STA_WAIT_LEDS           6
#define STA_WAIT_AUTOREP        7
#define STA_WAIT_RESET          8
#define STA_DISABLED            9
#define STA_DELAY               11
#define STA_REPEAT              12

#define SC_EXTENDED_CODE        0xE0

#define SC_0 0x45
#define SC_1 0x16
#define SC_2 0x1E
#define SC_3 0x26
#define SC_4 0x25
#define SC_5 0x2E
#define SC_6 0x36
#define SC_7 0x3D
#define SC_8 0x3E
#define SC_9 0x46
#define SC_ENTER 0x5A

#define START_MAKE 0xFF
#define END_MAKE   0xFE
#define NO_REPEAT  0xFD
#define SPLIT      0xFC

#define M_LALT  	0x01
#define M_LSHIFT	0x02
#define M_LCTRL		0x04
#define M_LGUI		0x08
#define M_RALT		0x10
#define M_RSHIFT	0x20
#define M_RCTRL		0x40
#define M_RGUI		0x80


#define QUEUE_SIZE 200

uint8_t pop(void);
void push(uint8_t item);

extern void putKey(uint8_t keyidx, uint8_t isPushed);
extern void tx_state(unsigned char x, unsigned char newstate);

