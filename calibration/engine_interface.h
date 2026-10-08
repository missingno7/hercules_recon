#ifndef HERCULES_ENGINE_INTERFACE_H
#define HERCULES_ENGINE_INTERFACE_H
#include <stddef.h>
#include "resource_context.h"
/* Observed copied 250-word prefix only. Callback shapes are physical ABI views;
 * original declarations, signedness and full table extent remain unproved. */
typedef void (__cdecl *EngineNoArgCallback)(void);
typedef struct EngineInterface {
    unsigned long signature_000;
    ResourceCallbackContext * context_004;
    void * arena_008;
    unsigned long arena_bytes_00c;
    unsigned long opaque_010[4];
    int (__cdecl *load_async_020)(const char *, const char *, int, void *, unsigned long);
    int (__cdecl *cancel_load_024)(void);
    unsigned long opaque_028[1];
    void (__cdecl *dispatch_2c270_02c)(void);
    void (__cdecl *dispatch_2c280_030)(void);
    unsigned long (__cdecl *compact_pool_034)(int);
    unsigned long opaque_038[2];
    void (__cdecl *free_allocation_040)(void *);
    void (__cdecl *bulk_release_044)(unsigned long);
    unsigned long (__cdecl *file_length_048)(const char *, const char *);
    unsigned long opaque_04c[1];
    void (__cdecl *save_arena_050)(void *, unsigned long);
    unsigned long opaque_054[2];
    void (__cdecl *dispatch_2c350_05c)(void);
    void (__cdecl *dispatch_2c360_060)(void);
    unsigned long opaque_064[1];
    unsigned long (__cdecl *load_sync_068)(const char *, const char *, void *, unsigned long);
    void (__cdecl *dispatch_2c3b0_06c)(unsigned long);
    unsigned long opaque_070[2];
    void *(__cdecl *allocate_078)(void **, unsigned long, unsigned long);
    unsigned long opaque_07c[2];
    void (__cdecl *dispatch_2c420_084)(void);
    unsigned long opaque_088[1];
    void (__cdecl *dispatch_2c440_08c)(void);
    unsigned long opaque_090[10];
    void (__cdecl *dispatch_2c4e0_0b8)(unsigned long);
    unsigned long opaque_0bc[3];
    void (__cdecl *dispatch_2c500_0c8)(void *, void *);
    void (__cdecl *dispatch_2c520_0cc)(unsigned long);
    unsigned long opaque_0d0[1];
    void (__cdecl *dispatch_2c540_0d4)(unsigned long, unsigned long);
    unsigned long opaque_0d8[2];
    void (__cdecl *dispatch_2c570_0e0)(void *);
    unsigned long opaque_0e4[2];
    void (__cdecl *dispatch_2c5a0_0ec)(unsigned long);
    unsigned long opaque_0f0[20];
    void (__cdecl *dispatch_2c5e0_140)(const void *, unsigned long, unsigned long, unsigned long);
    int (__cdecl *frame_chain_write_144)(void *, unsigned long);
    unsigned long opaque_148[7];
    void (__cdecl *dispatch_2c620_164)(void *);
    unsigned long opaque_168[1];
    void (__cdecl *dispatch_2c630_16c)(unsigned long);
    EngineNoArgCallback (__cdecl *dispatch_2c640_170)(EngineNoArgCallback);
    unsigned long opaque_174[33];
    void (__cdecl *dispatch_2c7e0_1f8)(void);
    unsigned long opaque_1fc[18];
    void (__cdecl *dispatch_2c840_244)(int, int);
    unsigned long opaque_248[1];
    void (__cdecl *dispatch_2c860_24c)(int, int);
    unsigned long opaque_250[2];
    void (__cdecl *update_host_258)(void);
    unsigned long opaque_25c[8];
    void (__cdecl *dispatch_2c8e0_27c)(void *);
    void (__cdecl *dispatch_2c8f0_280)(void *);
    unsigned long opaque_284[10];
    void (__cdecl *dispatch_2c980_2ac)(void *, unsigned long, unsigned long, unsigned long, unsigned long);
    unsigned long opaque_2b0[15];
    unsigned long (__cdecl *dispatch_2ca40_2ec)(unsigned long);
    unsigned long opaque_2f0[16];
    void (__cdecl *dispatch_2caa0_330)(void);
    unsigned long opaque_334[43];
    unsigned long (__cdecl *dispatch_2cc30_3e0)(unsigned long);
    EngineNoArgCallback (__cdecl *dispatch_2cc40_3e4)(EngineNoArgCallback);
} EngineInterface;

typedef char engine_interface_is_1000_bytes[sizeof(EngineInterface)==1000 ? 1:-1];
typedef char engine_interface_observed_offsets[
    (offsetof(EngineInterface,signature_000)==0x0 &&
     offsetof(EngineInterface,context_004)==0x4 &&
     offsetof(EngineInterface,arena_008)==0x8 &&
     offsetof(EngineInterface,arena_bytes_00c)==0xc &&
     offsetof(EngineInterface,load_async_020)==0x20 &&
     offsetof(EngineInterface,cancel_load_024)==0x24 &&
     offsetof(EngineInterface,dispatch_2c270_02c)==0x2c &&
     offsetof(EngineInterface,dispatch_2c280_030)==0x30 &&
     offsetof(EngineInterface,compact_pool_034)==0x34 &&
     offsetof(EngineInterface,free_allocation_040)==0x40 &&
     offsetof(EngineInterface,bulk_release_044)==0x44 &&
     offsetof(EngineInterface,file_length_048)==0x48 &&
     offsetof(EngineInterface,save_arena_050)==0x50 &&
     offsetof(EngineInterface,dispatch_2c350_05c)==0x5c &&
     offsetof(EngineInterface,dispatch_2c360_060)==0x60 &&
     offsetof(EngineInterface,load_sync_068)==0x68 &&
     offsetof(EngineInterface,dispatch_2c3b0_06c)==0x6c &&
     offsetof(EngineInterface,allocate_078)==0x78 &&
     offsetof(EngineInterface,dispatch_2c420_084)==0x84 &&
     offsetof(EngineInterface,dispatch_2c440_08c)==0x8c &&
     offsetof(EngineInterface,dispatch_2c4e0_0b8)==0xb8 &&
     offsetof(EngineInterface,dispatch_2c520_0cc)==0xcc &&
     offsetof(EngineInterface,dispatch_2c500_0c8)==0xc8 &&
     offsetof(EngineInterface,dispatch_2c540_0d4)==0xd4 &&
     offsetof(EngineInterface,dispatch_2c570_0e0)==0xe0 &&
     offsetof(EngineInterface,dispatch_2c5a0_0ec)==0xec &&
     offsetof(EngineInterface,dispatch_2c5e0_140)==0x140 &&
     offsetof(EngineInterface,frame_chain_write_144)==0x144 &&
     offsetof(EngineInterface,dispatch_2c620_164)==0x164 &&
     offsetof(EngineInterface,dispatch_2c630_16c)==0x16c &&
     offsetof(EngineInterface,dispatch_2c640_170)==0x170 &&
     offsetof(EngineInterface,dispatch_2c7e0_1f8)==0x1f8 &&
     offsetof(EngineInterface,dispatch_2c840_244)==0x244 &&
     offsetof(EngineInterface,dispatch_2c860_24c)==0x24c &&
     offsetof(EngineInterface,update_host_258)==0x258 &&
     offsetof(EngineInterface,dispatch_2c8e0_27c)==0x27c &&
     offsetof(EngineInterface,dispatch_2c8f0_280)==0x280 &&
     offsetof(EngineInterface,dispatch_2c980_2ac)==0x2ac &&
     offsetof(EngineInterface,dispatch_2ca40_2ec)==0x2ec &&
     offsetof(EngineInterface,dispatch_2caa0_330)==0x330 &&
     offsetof(EngineInterface,dispatch_2cc30_3e0)==0x3e0 &&
     offsetof(EngineInterface,dispatch_2cc40_3e4)==0x3e4) ? 1:-1];
extern EngineInterface g_engine_interface;
#endif
