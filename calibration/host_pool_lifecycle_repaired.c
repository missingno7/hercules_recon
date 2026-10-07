/* Private ordinary-C host pool lifecycle candidate; names/ownership inferred. */
typedef unsigned long U32;
typedef signed long S32;
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
extern const char g_fmt_compact[];
extern void __cdecl host_free(void *payload);
extern void __cdecl coalesce_pool(U32 pool_id);
extern U32 __cdecl count_used(U32 pool_id);
extern void __cdecl reverse_dword_copy(void *destination,
                                       const void *source, U32 bytes);
extern void __cdecl host_diagnostic(const char *format, ...);
extern U32 __cdecl compact_pool(U32 pool_id, U32 byte_limit);

U32 __cdecl compact_all_wrapper(U32 pool_id)
{
    return compact_pool(pool_id, 0);
}

void __cdecl bulk_free(U32 pool_id)
{
    PoolWord *cursor = g_host_pool.starts[pool_id];
    PoolWord *end = g_host_pool.ends[pool_id];
    U32 descriptor;

    g_host_pool.bulk_first_header = cursor;
    descriptor = cursor->descriptor;
    g_host_pool.bulk_first_words = descriptor & POOL_SIZE;

    while (cursor < end) {
        descriptor = cursor->descriptor;
        if ((descriptor & (POOL_FREE | POOL_PINNED)) == 0) {
            void **owner_slot = cursor[1].owner_slot;
            void *owner = *owner_slot;
            if (owner != 0)
                host_free(owner);
        }
        cursor += descriptor & POOL_SIZE;
    }

    compact_all_wrapper(pool_id);
}

U32 __cdecl compact_pool(U32 pool_id, U32 byte_limit)
{
    U32 remaining = count_used(pool_id) - 1;
    U32 moved_words = 0;

    if ((S32)remaining < 0)
        return 1;

    while ((S32)remaining >= 0) {
        PoolWord *cursor = g_host_pool.starts[pool_id];
        PoolWord *end = g_host_pool.ends[pool_id];
        U32 ordinal = 0;
        U32 descriptor;
        U32 words;
        U32 next_descriptor;
        U32 free_words;
        U32 bytes;
        void **owner_slot;
        PoolWord *destination;
        void *new_payload;

        while (cursor < end) {
            descriptor = cursor->descriptor;
            if ((descriptor & POOL_FREE) == 0) {
                if (remaining == ordinal)
                    goto selected_block;
                ++ordinal;
            }
            cursor += descriptor & POOL_SIZE;
        }

        --remaining;
        continue;

selected_block:
        descriptor = cursor->descriptor;
        if (descriptor & POOL_PINNED) {
            --remaining;
            continue;
        }

        words = descriptor & POOL_SIZE;
        next_descriptor = cursor[words].descriptor;
        if ((next_descriptor & POOL_FREE) == 0) {
            --remaining;
            continue;
        }

        free_words = next_descriptor & POOL_SIZE;
        bytes = words << 2;
        owner_slot = cursor[1].owner_slot;
        destination = cursor + free_words;

        if (byte_limit != 0 && moved_words + bytes > byte_limit)
            return 0;

        reverse_dword_copy(destination, cursor, bytes);
        moved_words += words;

        cursor->descriptor = POOL_FREE | free_words;
        destination->descriptor = words;
        destination[1].owner_slot = owner_slot;
        new_payload = (void *)&destination[2];
        *owner_slot = new_payload;

        coalesce_pool(pool_id);
        host_diagnostic(g_fmt_compact, pool_id, new_payload, bytes - 8);

        if (moved_words > 0x8000)
            return 0;

        --remaining;
    }

    return 1;
}