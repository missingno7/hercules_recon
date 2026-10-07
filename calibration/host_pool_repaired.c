/* PC-derived host pool frame; names and translation-unit ownership are inferred. */
typedef unsigned long U32;
typedef unsigned char U8;

#define POOL_FREE   0x80000000UL
#define POOL_PINNED 0x40000000UL
#define POOL_SIZE   0x3fffffffUL

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
extern const char g_fmt_init_pool[];
extern const char g_fmt_alloc_failed[];
extern const char g_fmt_alloc[];
extern const char g_fmt_free[];
extern void __cdecl host_diagnostic(const char *format, ...);

void __cdecl save_arena(U8 *arena, U32 bytes)
{
    g_host_pool.saved_arena = arena;
    g_host_pool.saved_bytes = bytes;
}

void __cdecl initialize_pool(U8 *arena, U32 bytes, U32 pool_id)
{
    U32 words = (bytes >> 2) - 2;
    PoolWord *header = (PoolWord *)arena;
    PoolWord *cursor;

    host_diagnostic(g_fmt_init_pool, pool_id, arena, bytes);
    g_host_pool.starts[pool_id] = header;
    g_host_pool.ends[pool_id] = (PoolWord *)(arena + (words << 2));
    header[0].descriptor = words | POOL_FREE;

    cursor = header + 2;
    --words;
    do {
        cursor->descriptor = 1;
        ++cursor;
        --words;
    } while (words != 0);
}

void __cdecl coalesce_pool(U32 pool_id)
{
    PoolWord *cursor = g_host_pool.starts[pool_id];
    PoolWord *end = g_host_pool.ends[pool_id];

    while (cursor < end) {
        U32 descriptor = cursor->descriptor;
        if ((descriptor & POOL_FREE) == POOL_FREE) {
            U32 next_descriptor =
                cursor[descriptor & POOL_SIZE].descriptor;
            if ((next_descriptor & POOL_FREE) == POOL_FREE) {
                cursor->descriptor =
                    descriptor + (next_descriptor & POOL_SIZE);
                continue;
            }
        }
        cursor += descriptor;
    }
}

void *__cdecl host_allocate(void **output_slot, U32 requested_bytes,
                            U32 flags)
{
    void *previous = *output_slot;
    U32 pool_id;
    U32 pinned = 0;
    U32 preserve_output = 0;
    U32 need_words;
    U32 selected_words = 0;
    PoolWord *cursor;
    PoolWord *end;
    PoolWord *selected = 0;
    U32 descriptor;

    if (previous)
        host_diagnostic(g_fmt_alloc_failed, output_slot, previous);

    if (flags & 0x10)
        pinned = POOL_PINNED;
    if (flags & 0x20)
        preserve_output = 1;

    pool_id = flags & 3;
    need_words = ((requested_bytes + 3) >> 2) + 2;
    cursor = g_host_pool.starts[pool_id];
    end = g_host_pool.ends[pool_id];

    while (cursor < end) {
        descriptor = cursor->descriptor;
        if ((descriptor & POOL_FREE) == POOL_FREE) {
            U32 words = descriptor & POOL_SIZE;
            if (words >= need_words) {
                selected = cursor;
                selected_words = words;
            }
        }
        cursor += descriptor;
    }

    if (selected == 0) {
        if (preserve_output == 0)
            *output_slot = 0;
        return 0;
    }

    {
        U32 remaining = selected_words - need_words;
        U8 *payload;

        if (remaining == 0) {
            selected[0].descriptor = pinned | need_words;
            selected[1].owner_slot = output_slot;
            payload = (U8 *)&selected[2];
        } else {
            selected[0].descriptor = POOL_FREE | remaining;
            selected[remaining].descriptor = pinned | need_words;
            selected[remaining + 1].owner_slot = output_slot;
            payload = (U8 *)&selected[remaining + 2];
        }

        host_diagnostic(g_fmt_alloc, pool_id, payload,
                        (need_words << 2) - 8);
        *output_slot = payload;
        return payload;
    }
}

void __cdecl host_free(void *payload)
{
    U32 *end_cell = (U32 *)g_host_pool.ends;
    U32 pool_id = 0;
    U32 payload_address = (U32)payload;
    PoolWord *header;
    U32 descriptor;
    PoolWord *owner_word;

    if (payload == 0)
        return;

    do {
        if (payload_address > end_cell[-4] &&
            payload_address < end_cell[0])
            break;
        ++end_cell;
        ++pool_id;
    } while (pool_id < 4);

    if (pool_id >= 4)
        return;

    header = (PoolWord *)((U8 *)payload - 8);
    descriptor = header->descriptor | POOL_FREE;
    header->descriptor = descriptor;

    host_diagnostic(g_fmt_free, pool_id, payload,
                    (descriptor << 2) - 8);
    coalesce_pool(pool_id);

    owner_word = (PoolWord *)((U8 *)payload - 4);
    if (owner_word->owner_slot)
        *owner_word->owner_slot = 0;
}
