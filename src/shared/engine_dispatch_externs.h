#ifndef HERCULES_ENGINE_DISPATCH_EXTERNS_H
#define HERCULES_ENGINE_DISPATCH_EXTERNS_H
#include "engine_interface.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;

/* Address-based diagnostic aliases to actual callees; names are not historical. */
extern u32 g_dispatch_entry_state_6fcec;
extern u8 g_dispatch_startup_gate_6fce8;
extern u32 g_dispatch_global_58b00;
extern u32 g_dispatch_global_58af8;
extern u32 g_dispatch_descriptors_5a6a4[];
extern u32 g_dispatch_callback_flag_71ec0;
extern u8 g_dispatch_key_7286c;
extern u16 g_dispatch_setting_4d858;
extern u16 g_dispatch_word_6ee80;
extern u8 *g_dispatch_frame_current_71ec4;
extern u8 *g_dispatch_frame_base_71ec8;
extern u32 g_dispatch_global_6ff88;
extern const u8 g_dispatch_diagnostic_58ac8[];

extern void __cdecl dispatch_engine_2c860(int, int);
extern void __cdecl dispatch_engine_2c350(void);
extern void __cdecl dispatch_local_083d3(void);
extern void __cdecl dispatch_local_082b7(int);
extern void __cdecl dispatch_engine_2c3b0(u32);
extern u32 __cdecl dispatch_engine_2cc30(u32);
extern void __cdecl dispatch_local_0827f(int);
extern void __cdecl dispatch_local_1f0f0(void);
extern void __cdecl dispatch_local_1f750(void);
extern void __cdecl dispatch_local_32b10(void);
extern void __cdecl dispatch_local_255f0(void);
extern void __cdecl dispatch_local_05ea0(void);
extern void __cdecl dispatch_engine_2c840(int, int);
extern void __cdecl dispatch_local_1f320(void);
extern void __cdecl dispatch_engine_2c5a0(u32);
extern void __cdecl dispatch_engine_2c360(void);
extern void __cdecl dispatch_engine_2c540(u32, u32);
extern EngineNoArgCallback __cdecl dispatch_engine_2cc40(EngineNoArgCallback);
extern void __cdecl dispatch_engine_2c4e0(u32);
extern void __cdecl dispatch_engine_2c520(u32);
extern void __cdecl dispatch_local_01000(const void *);
extern void __cdecl dispatch_engine_2c270(void);
extern void __cdecl dispatch_local_0e670(void);
extern u32 __cdecl dispatch_engine_2ca40(u32);
extern void __cdecl dispatch_engine_2c420(void);
extern void __cdecl dispatch_local_06005(void);
extern void __cdecl dispatch_local_02180(void);
extern void __cdecl dispatch_local_1f870(int);
extern void __cdecl dispatch_engine_2c440(void);
extern void __cdecl dispatch_local_05600(void);
extern void __cdecl dispatch_local_2fb70(void);
extern void __cdecl dispatch_local_2f840(void);
extern void __cdecl dispatch_local_258e0(void);
extern void __cdecl dispatch_local_034d0(void);
extern void __cdecl dispatch_local_156a0(void);
extern void __cdecl dispatch_local_1fd90(void);
extern void __cdecl dispatch_local_25d00(void);
extern void __cdecl dispatch_local_256e0(void);
extern void __cdecl dispatch_local_03470(void);
extern void __cdecl dispatch_local_05d50(void);
extern void __cdecl dispatch_local_03a50(void);
extern void __cdecl dispatch_local_32bd0(void);
extern void __cdecl dispatch_local_2da90(void);
extern void __cdecl dispatch_local_1f6f0(void);
extern void __cdecl dispatch_engine_2c630(u32);
extern void __cdecl dispatch_engine_2c570(void *);
extern void __cdecl dispatch_local_02cb0(void);
extern void __cdecl dispatch_local_050d0(void);
extern void __cdecl dispatch_local_058b0(void);
extern void __cdecl dispatch_engine_2c620(void *);
extern void __cdecl dispatch_local_0666b(void);
extern int __cdecl dispatch_local_060f9(void);
extern void __cdecl dispatch_completion_gate(void);

#endif
