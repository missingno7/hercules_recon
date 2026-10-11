/* Isolated provider for the observed host-pool metadata prefix. */
#include <stddef.h>

typedef unsigned long U32;
typedef unsigned char U8;

typedef union PoolWord PoolWord;
union PoolWord {
    U32 descriptor;
    void **owner_slot;
};

typedef struct HostPoolState HostPoolState;
struct HostPoolState {
    PoolWord *starts[4];
    PoolWord *ends[4];
    U8 *saved_arena;
    U32 saved_bytes;
    PoolWord *bulk_first_header;
    U32 bulk_first_words;
};

typedef char host_pool_state_size_is_48[(sizeof(HostPoolState) == 48) ? 1 : -1];
typedef char host_pool_starts_offset_is_0[(offsetof(HostPoolState, starts) == 0) ? 1 : -1];
typedef char host_pool_ends_offset_is_16[(offsetof(HostPoolState, ends) == 16) ? 1 : -1];
typedef char host_pool_saved_arena_offset_is_32[(offsetof(HostPoolState, saved_arena) == 32) ? 1 : -1];
typedef char host_pool_saved_bytes_offset_is_36[(offsetof(HostPoolState, saved_bytes) == 36) ? 1 : -1];
typedef char host_pool_bulk_header_offset_is_40[(offsetof(HostPoolState, bulk_first_header) == 40) ? 1 : -1];
typedef char host_pool_bulk_words_offset_is_44[(offsetof(HostPoolState, bulk_first_words) == 44) ? 1 : -1];

HostPoolState g_host_pool = {0};
