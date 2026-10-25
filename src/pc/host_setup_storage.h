#ifndef HERCULES_HOST_SETUP_STORAGE_H
#define HERCULES_HOST_SETUP_STORAGE_H
#include "../shared/engine_interface.h"
#include "host_callback_protocol_externs.h"

typedef struct HostSetupObservedPrefix {
    unsigned long argument_prefix_000[4];
    unsigned short context_defaults_010[8];
} HostSetupObservedPrefix;
typedef char setup_prefix_size[(sizeof(HostSetupObservedPrefix) == 32) ? 1 : -1];
typedef char setup_defaults_offset[(offsetof(HostSetupObservedPrefix, context_defaults_010) == 16) ? 1 : -1];
extern HostSetupObservedPrefix g_host_setup_prefix_3a630;
extern unsigned long g_host_setup_slots_first_3744c;
/* The original A350 consumer proves these are the existing public-file owners. */
extern int g_public_file_count;
extern int g_public_file_handles[16];
extern EngineInterface g_host_engine_interface;
extern void __cdecl initialize_pool(unsigned char *, unsigned long, unsigned long);
typedef union PoolWord PoolWord;
union PoolWord { unsigned long descriptor; void **owner_slot; };
typedef struct HostPoolState {
    PoolWord *starts[4];
    PoolWord *ends[4];
    unsigned char *saved_arena;
    unsigned long saved_bytes;
    PoolWord *bulk_first_header;
    unsigned long bulk_first_words;
} HostPoolState;
typedef char setup_pool_view_size[(sizeof(HostPoolState) == 48) ? 1 : -1];
typedef char setup_pool_saved_offsets[(offsetof(HostPoolState, saved_arena) == 32 && offsetof(HostPoolState, saved_bytes) == 36) ? 1 : -1];
extern HostPoolState g_host_pool;
#endif
