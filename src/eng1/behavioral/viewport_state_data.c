/* Private data-only views.  Neither object asserts original TU or placement. */
typedef unsigned char u8;
typedef unsigned short u16;

/* Only the shared loader-zero prefix touched by 0x32b10 and 0x32bd0. */
u8 g_cold_start_state_bytes_70820[0x1c];

/* Exact 8-byte pointer target passed by 0x32b10 to dispatch_engine_2c7f0. */
u16 g_stpanel1_upload_rectangle_words[4] = {
    0x0000U, 0x0200U, 0x0080U, 0x0200U
};
