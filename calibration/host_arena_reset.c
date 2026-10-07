typedef unsigned long U32;
typedef unsigned char U8;

#define POOL_FREE 0x80000000UL

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

extern HostPoolState g_host_pool;
extern void __cdecl coalesce_pool(U32 pool_id);
extern void __cdecl host_diagnostic(const char *format, ...);
extern const char g_fmt_reinit_pool[];

void __cdecl reinitialize_pool0(U8 *new_arena)
{
    U32 old_bytes;
    long old_words;
    U32 new_words;
    U32 first_words;

    if (new_arena) {
        old_bytes = (U32)new_arena - (U32)g_host_pool.saved_arena;
        g_host_pool.saved_arena = new_arena;
        g_host_pool.saved_bytes -= old_bytes;
    }

    old_words = ((U8 *)g_host_pool.ends[0] -
                 (U8 *)g_host_pool.starts[0]) >> 2;
    new_words = (g_host_pool.saved_bytes - 8) >> 2;
    first_words = new_words - old_words + g_host_pool.bulk_first_words;
    ((PoolWord *)g_host_pool.saved_arena)->descriptor = POOL_FREE | first_words;
    g_host_pool.starts[0] = (PoolWord *)g_host_pool.saved_arena;
    g_host_pool.ends[0] =
        (PoolWord *)(g_host_pool.saved_arena + g_host_pool.saved_bytes - 8);

    coalesce_pool(0);
    host_diagnostic(g_fmt_reinit_pool, 0,
                    g_host_pool.saved_arena, g_host_pool.saved_bytes);
}
