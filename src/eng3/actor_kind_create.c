#include <stddef.h>
#include <stdlib.h>  /* TU-context hypothesis (measured, not proven original text): VC5 orders the
                         * table/index SIB operands by symbol identity, and the native order needs the
                         * CRT declarations ahead of the table pointer (work/velocity_i/solSIB/findings.md). */

typedef struct Eng3Actor Eng3Actor;
struct Eng3Actor {
    long x;
    long y;
    long z;
    unsigned char reserved_00c[0x16];
    unsigned char table_category;
    unsigned char reserved_023[0x0b];
    unsigned short kind;
    unsigned char reserved_030[0x34];
    unsigned long table_value;
    unsigned char reserved_068[0x2c];
};

typedef struct Eng3KindEntry Eng3KindEntry;
struct Eng3KindEntry {
    unsigned long value;
    unsigned short reserved_004;
    unsigned char category;
    unsigned char reserved_007;
};

typedef char eng3_actor_extent_is_0x94[(sizeof(Eng3Actor) == 0x94) ? 1 : -1];
typedef char eng3_kind_offset_is_0x2e[(offsetof(Eng3Actor, kind) == 0x2e) ? 1 : -1];
typedef char eng3_category_offset_is_0x22[(offsetof(Eng3Actor, table_category) == 0x22) ? 1 : -1];
typedef char eng3_table_value_offset_is_0x64[(offsetof(Eng3Actor, table_value) == 0x64) ? 1 : -1];
typedef char eng3_kind_entry_stride_is_8[(sizeof(Eng3KindEntry) == 8) ? 1 : -1];

extern Eng3Actor *allocate_eng3_actor(unsigned long kind);
extern void reset_eng3_actor(Eng3Actor *actor);
extern const Eng3KindEntry *g_eng3_kind_entries;

Eng3Actor *create_eng3_kind_actor(long x, long y, long z, unsigned long kind)
{
    Eng3Actor *actor;

    actor = allocate_eng3_actor(kind);
    if (actor != 0) {
        reset_eng3_actor(actor);
        actor->kind = (unsigned short)(kind | 0x2000);
        actor->x = x;
        actor->y = y;
        actor->z = z;
        actor->table_category = g_eng3_kind_entries[kind & 0x0fff].category;
        actor->table_value = g_eng3_kind_entries[kind & 0x0fff].value;
    }
    return actor;
}
