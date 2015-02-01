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
} tinycmd_led_type;

typedef struct
{
  uint8_t led_max;
  uint8_t level_max;
} tinycmd_config_type;

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

// TINY_CMD_BL_LED_ALL_F
typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t on; // on or off
  tinycmd_led_type led;
} tinycmd_bl_led_all_req_type;

// TINY_CMD_BL_LED_POS_F
typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t pos; // position
  tinycmd_led_type led;
} tinycmd_bl_led_pos_req_type;

// TINY_CMD_BL_LED_RANGE_F
typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t num;
  uint8_t offset;
  tinycmd_led_type led[TINYCMD_LED_MAX];
} tinycmd_bl_led_range_req_type;

// TINY_CMD_BL_LED_EFFECT_F
typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t effect;
} tinycmd_bl_led_effect_req_type;

// TINY_CMD_SET_LED_MODE_F
typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t storage;
  uint8_t block;
  uint8_t mode;
} tinycmd_set_led_mode_req_type;

// TINY_CMD_SET_LED_MODE_ALL_F
typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t data[15];
} tinycmd_set_led_mode_all_req_type;

typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t channel;
  uint8_t enable;
  uint8_t duty;
} tinycmd_pwm_req_type;

typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  tinycmd_config_type value;
} tinycmd_config_req_type;

typedef union
{
  uint8_t cmd;
} tinycmd_test_data_type;

typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  tinycmd_test_data_type data;
} tinycmd_test_req_type;

typedef union
{
  uint8_t cmd_code;
  tinycmd_ver_req_type                 ver;
  tinycmd_three_lock_req_type          three_lock;
  tinycmd_bl_led_all_req_type          bl_led_all;
  tinycmd_bl_led_pos_req_type          bl_led_pos;
  tinycmd_bl_led_range_req_type        bl_led_range;
  tinycmd_bl_led_effect_req_type       bl_led_effect;
  tinycmd_pwm_req_type                 pwm;
  tinycmd_config_req_type              config;
  tinycmd_test_req_type                test;
  tinycmd_set_led_mode_req_type        set_led_mode;
  tinycmd_set_led_mode_all_req_type    set_led_mode_all;
} tinycmd_pkt_req_type;


// Response type
typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
} tinycmd_rsp_type;


typedef struct
{
  uint8_t cmd_code;
  uint8_t pkt_len;
  uint8_t version;
} tinycmd_ver_rsp_type;

typedef union
{
  uint8_t cmd_code;
  tinycmd_rsp_type gen;
  tinycmd_ver_rsp_type ver;
} tinycmd_pkt_rsp_type;

#endif // TINYCMDPKT_H

