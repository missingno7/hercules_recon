#include <string.h>
typedef unsigned long U32;
typedef union PoolWord PoolWord;
union PoolWord { U32 descriptor; void **owner_slot; };
#define POOL_FREE 0x80000000UL
#define POOL_PINNED 0x40000000UL
#define POOL_SIZE 0x3fffffffUL
extern void __cdecl coalesce_pool(U32);
extern void __cdecl host_diagnostic(const char *, ...);
extern const char g_fmt_remalloc[];
void *__cdecl shrink_relocate(void *old_payload, U32 requested_bytes, U32 pool_id, U32 owner_only)
{
    PoolWord *old_header;
    PoolWord *new_header;
    U32 old_descriptor;
    U32 old_words;
    U32 old_payload_bytes;
    U32 rounded_bytes;
    U32 new_words;
    U32 remainder_words;
    U32 retained_flags;
    void **owner_slot;
    void *new_payload;
    if (old_payload == 0) return 0;
    old_header = (PoolWord *)old_payload - 2;
    old_descriptor = old_header[0].descriptor;
    old_words = old_descriptor & POOL_SIZE;
    old_payload_bytes = (old_descriptor << 2) - 8;
    rounded_bytes = ((requested_bytes + 3) >> 2) << 2;
    if (rounded_bytes == old_payload_bytes) return 0;
    if (rounded_bytes >= old_payload_bytes) return old_payload;
    new_words = (rounded_bytes >> 2) + 2;
    remainder_words = old_words - new_words;
    retained_flags = old_descriptor & POOL_PINNED;
    owner_slot = old_header[1].owner_slot;
    new_header = old_header + remainder_words;
    old_header[0].descriptor = POOL_FREE | remainder_words;
    new_header[0].descriptor = retained_flags | new_words;
    if (owner_only != 0)
        new_header[1].owner_slot = old_header[1].owner_slot;
    else
        memmove(&new_header[1], &old_header[1], (new_words << 2) - 4);
    new_payload = &new_header[2];
    *owner_slot = new_payload;
    coalesce_pool(pool_id);
    host_diagnostic(g_fmt_remalloc, pool_id, new_payload, (new_words << 2) - 8);
    return old_payload;
}
