enum {
	KEY_NONE=0,
    ErrorRollOver,
    POSTFail,
    ErrorUndefined,
    KEY_A,              // 0x04
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,

    KEY_M,              // 0x10
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_1,              //       1 and !
    KEY_2,              //       2 and @

    KEY_3,              // 0x20  3 and #
    KEY_4,              //       4 and $
    KEY_5,              //       5 and %
    KEY_6,              //       6 and ^
    KEY_7,              //       7 and &
    KEY_8,              //       8 and *
    KEY_9,              //       9 and (
    KEY_0,              // 0x27  0 and )
    KEY_ENTER,          // 0x28  enter
    KEY_ESC,            // 0x29
    KEY_BKSP,           // 0x2A  backspace
    KEY_TAB,            // 0x2B
    KEY_SPACE,          // 0x2C
    KEY_MINUS,          // 0x2D  - and _
    KEY_EQUAL,          // 0x2E  = and +
    KEY_LBR,            // 0x2F  [ and {

    KEY_RBR,            // 0x30  ] and }
    KEY_BKSLASH,        // 0x31  \ and |
    KEY_Europe1,        // 0x32  non-US # and ~
    KEY_COLON,          // 0x33  ; and :
    KEY_QUOTE,          // 0x34  ' and "
    KEY_HASH,           // 0x35  grave accent and tilde
    KEY_COMMA,          // 0x36  , and <
    KEY_DOT,            // 0x37  . and >
    KEY_SLASH,          // 0x38  / and ?
    KEY_CAPS,           // 0x39
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,

    KEY_F7,             // 0x40
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_PRNSCR,
    KEY_SCRLCK,
    KEY_PAUSE,          //Break
    KEY_INSERT,
    KEY_HOME,
    KEY_PGUP,
    KEY_DEL,
    KEY_END,
    KEY_PGDN,
    KEY_RIGHT,
    
    KEY_LEFT,           // 0x50
    KEY_DOWN,
    KEY_UP,
    KEY_NUMLOCK,        // Clear
    KEY_KP_SLASH,
    KEY_KP_AST,
    KEY_KP_MINUS,
    KEY_KP_PLUS,
    KEY_KP_ENTER,
    KEY_KP_1,           // End
    KEY_KP_2,           // Down Arrow
    KEY_KP_3,           // Page Down
    KEY_KP_4,           // Left Arrow
    KEY_KP_5,
    KEY_KP_6,           // Right Arrow
    KEY_KP_7,           // Home
    
    KEY_KP_8,           // 0x60  Up Arrow
    KEY_KP_9,           // Page Up
    KEY_KP_0,           // Insert
    KEY_KP_DOT,         // Delete
    KEY_Europe2,        // non-US \ and |
    KEY_APPS,		    // 102
    KEY_POWER_HID,
    KEY_KP_EQUAL,
    KEY_LED0,           //
    KEY_LED1,           //
    KEY_LED2,           //
    KEY_LED3,           //
    KEY_LFX,       //
    KEY_LPAD,      //
    KEY_LFULL,     //
    KEY_LASD,     //

    KEY_LARR,    // 0x70
    KEY_LVESEL,   
    KEY_MRESET, 
    KEY_RESET,          //254
    KEY_FN,             //255
    KEY_HELP,
    KEY_MENU,
    KEY_SEL,
    KEY_STOP_HID,
    KEY_AGAIN,
    KEY_UNDO,
    KEY_CUT,
    KEY_COPY,
    KEY_PASTE,
    KEY_FIND,
    KEY_MUTE_HID,
    
    KEY_VOLUP,          // 0x80
    KEY_VOLDN,
    KEY_KL_CAP,
    KEY_KL_NUM,
    KEY_KL_SCL,
    KEY_KP_COMMA,
    KEY_EQUAL_SIGN,
    KEY_L0,             //
    KEY_L1,             //
    KEY_L2,             //
    KEY_L3,             //
    KEY_L4,             //
    KEY_L5,             //
    KEY_L6,             //
    KEY_INTL8,
    KEY_INTL9,
    
    KEY_HANJA,          // 0x90
    KEY_HANGLE,
    KEY_KATA,
    KEY_HIRA,
    KEY_System,         //KEY_LANG5,
    KEY_POWER,          //KEY_LANG6,
    KEY_SLEEP,          //KEY_LANG7,
    KEY_WAKE,           //KEY_LANG8,
    KEY_KEYLOCK,        //KEY_LANG9,
    KEY_WINKEYLOCK,
    KEY_SYSREQ,
    KEY_CANCEL,
    KEY_CLEAR,
    KEY_PRIOR,
    KEY_RETURN,
    KEY_SPERATOR,
    
    KEY_OUT,            // 0xA0
    KEY_OPER,
    KEY_CLR_AGIN,
    KEY_CRSEL,
    KEY_EXCEL,
    
/* These are NOT standard USB HID - handled specially in decoding, 
     so they will be mapped to the modifier byte in the USB report */
	KEY_Modifiers,
	KEY_LCTRL,          // 0x01
	KEY_LSHIFT,         // 0x02
	KEY_LALT,           // 0x04
	KEY_LGUI,           // 0x08
	KEY_RCTRL,          // 0x10
	KEY_RSHIFT,         // 0x20
	KEY_RALT,           // 0x40
	KEY_RGUI,           // 0x80
	KEY_Modifiers_end,

   KEY_NEXT_TRK,        
   KEY_PREV_TRK,        // 0xB0
   KEY_STOP,
   KEY_PLAY,
   KEY_MUTE,
   KEY_BASS_BST,
   KEY_LOUDNESS,
   KEY_VOL_UP,
   KEY_VOL_DOWN,
   KEY_BASS_UP,
   KEY_BASS_DN,
   KEY_TRE_UP,
   KEY_TRE_DN,

   KEY_MEDIA_SEL,      // 0xc0
   KEY_MAIL,
   KEY_CALC,
   KEY_MYCOM,
   KEY_WWW_SEARCH,
   KEY_WWW_HOME,
   KEY_WWW_BACK,
   KEY_WWW_FORWARD,
   KEY_WWW_STOP,
   KEY_WWW_REFRESH,
   KEY_WWW_FAVORITE,
   KEY_EJECT,
   KEY_SCREENSAVE,
   KEY_REC,
	KEY_REWIND,
	KEY_MINIMIZE,

   KEY_M01,            // 0xd0 //KEY_F13,
   KEY_M02,            //KEY_F14,
   KEY_M03,            //KEY_F15,
   KEY_M04,            //KEY_F16,
   KEY_M05,            //KEY_F17,
   KEY_M06,            //KEY_F18,
   KEY_M07,            //KEY_F19,
   KEY_M08,            //KEY_F20,
   KEY_M09,            //KEY_F21,        // 0x70
   KEY_M10,            //KEY_F22,
   KEY_M11,            //KEY_F23,
   KEY_M12,            //KEY_F24,
   KEY_M13,            //KEY_INTL1,
   KEY_M14,            //KEY_INTL2,
   KEY_M15,            //KEY_INTL3,
   KEY_M16,            //KEY_INTL4,

   KEY_M17,            // 0xe0 //KEY_INTL5,
   KEY_M18,            //KEY_INTL6,
   KEY_M19,            //KEY_INTL7,
   KEY_M20,        
   KEY_M21,        
   KEY_M22,        
   KEY_M23,       
   KEY_M24,   
   KEY_M25,            //213
   KEY_M26,            //214
   KEY_M27,            //215
   KEY_M28,            //216
   KEY_M29,            //217
   KEY_M30,            //218
   KEY_M31,            //219
   KEY_M32,            //220

   KEY_M33,            // 0xf0//221
   KEY_M34,            //222
   KEY_M35,            //223
   KEY_M36,            //224
   KEY_M37,            //225
   KEY_M38,            //226
   KEY_M39,            //227
   KEY_M40,            //228
   KEY_M41,            //229
   KEY_M42,            //230
   KEY_M43,            //231
   KEY_M44,            //232
   KEY_M45,            //233
   KEY_M46,            //234
   KEY_M47,            //235
   KEY_M48,            //236
   KEY_M49,
   KEY_M50,
   KEY_M51,
   KEY_M52
};
   

