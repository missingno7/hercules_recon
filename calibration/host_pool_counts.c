typedef unsigned long U32;
typedef union PoolWord PoolWord;
union PoolWord { U32 descriptor; };
typedef struct HostPoolState HostPoolState;
struct HostPoolState { PoolWord *starts[4]; PoolWord *ends[4]; };
extern HostPoolState g_host_pool;
#define POOL_FREE 0x80000000UL
#define POOL_SIZE 0x3fffffffUL

U32 __cdecl count_used(U32 pool_id)
{
    PoolWord *cursor = g_host_pool.starts[pool_id];
    PoolWord *end = g_host_pool.ends[pool_id];
    U32 count = 0;

    while (cursor < end) {
        U32 descriptor = cursor->descriptor;
        if ((descriptor & POOL_FREE) != POOL_FREE)
            ++count;
        cursor += descriptor & POOL_SIZE;
    }

    return count;
}
