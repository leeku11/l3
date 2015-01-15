#ifndef TINYCMDPKT_H
#define TINYCMDPKT_H

#define TINYCMD_LED_MAX               15
#define TINYCMD_THREE_LOCK_NUM        3
#define TINYCMD_TEST_DATA_LEN         (TINYCMD_LED_MAX * 3 + TINYCMD_THREE_LOCK_NUM)

enum {
  TINY_RESET_HARD = 0,
  TINY_RESET_SOFT,
  TINY_RESET_DATA,
  TINY_RESET_MAX
} reset_type_e;

typedef struct
{
  uint8_t g;
  uint8_t r;
  uint8_t b;
} led_type;

typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
} tinycmd_ver_req_type;

typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t type;
} tinycmd_reset_req_type;

typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t lock;
} tinycmd_three_lock_req_type;

typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t num;
  uint8_t offset;
  led_type led[TINYCMD_LED_MAX];
} tinycmd_led_req_type;

typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t data_len;
  uint8_t data[TINYCMD_TEST_DATA_LEN];
} tinycmd_test_req_type;


typedef union
{
  uint8_t cmd_code;
  tinycmd_ver_req_type                 ver;
  tinycmd_three_lock_req_type          three_lock;
  tinycmd_led_req_type                 led;
  tinycmd_test_req_type                test;  
} tinycmd_pkt_req_type;

#endif // TINYCMDPKT_H

