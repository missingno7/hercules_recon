#ifndef HERCULES_ENGINE_INTERFACE_H
#define HERCULES_ENGINE_INTERFACE_H
/* Observed copied 250-word prefix; original full declaration remains unproved. */
#include <stddef.h>
#include "resource_context.h"

typedef struct EngineInterface {
    unsigned long signature_000;
    ResourceCallbackContext *context_004;
    void *arena_008;
    unsigned long arena_bytes_00c;
    unsigned long opaque_010[4];
    int (__cdecl *load_async_020)(const char *, const char *, int, void *, unsigned long);
    int (__cdecl *cancel_load_024)(void);
    unsigned long opaque_028[3];
    unsigned long (__cdecl *compact_pool_034)(int);
    unsigned long opaque_038[2];
    void (__cdecl *free_allocation_040)(void *);
    unsigned long opaque_044;
    unsigned long (__cdecl *file_length_048)(const char *, const char *);
    unsigned long opaque_04c;
    void (__cdecl *save_arena_050)(void *, unsigned long);
    unsigned long opaque_054[5];
    unsigned long (__cdecl *load_sync_068)(const char *, const char *, void *, unsigned long);
    unsigned long opaque_06c[3];
    void *(__cdecl *allocate_078)(void **, unsigned long, unsigned long);
    unsigned long opaque_07c[119];
    void (__cdecl *update_host_258)(void);
    unsigned long opaque_25c[99];
} EngineInterface;

typedef char engine_interface_is_1000_bytes[
    sizeof(EngineInterface) == 1000 ? 1 : -1];
typedef char engine_interface_member_offsets[
    (offsetof(EngineInterface, context_004) == 4 &&
     offsetof(EngineInterface, load_async_020) == 0x20 &&
     offsetof(EngineInterface, cancel_load_024) == 0x24 &&
     offsetof(EngineInterface, compact_pool_034) == 0x34 &&
     offsetof(EngineInterface, free_allocation_040) == 0x40 &&
     offsetof(EngineInterface, file_length_048) == 0x48 &&
     offsetof(EngineInterface, save_arena_050) == 0x50 &&
     offsetof(EngineInterface, load_sync_068) == 0x68 &&
     offsetof(EngineInterface, allocate_078) == 0x78 &&
     offsetof(EngineInterface, update_host_258) == 0x258) ? 1 : -1];

extern EngineInterface g_engine_interface;

#endif
