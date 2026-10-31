#include <stddef.h>
#include <stdlib.h>  /* TU-context hypothesis (measured, not proven original text): VC5 orders the
                         * table/index SIB operands by symbol identity, and the native order needs the
                         * CRT declarations ahead of the table pointer (work/velocity_i/solSIB/findings.md). */

typedef struct Eng3ActorBase Eng3ActorBase;
struct Eng3ActorBase {
    long x;
    long y;
    long z;
    unsigned char reserved_00c[0x16];
    unsigned char category;
    unsigned char reserved_023[0x0b];
    unsigned short kind;
    unsigned char reserved_030[0x34];
    unsigned long table_value;
    unsigned char reserved_068[0x2c];
};

typedef struct Eng3MiddleActor Eng3MiddleActor;
struct Eng3MiddleActor {
    Eng3ActorBase base;
    unsigned char reserved_094[8];
    long position_copy_x;
    long position_copy_y;
    long position_copy_z;
    unsigned char reserved_0a8[0xec - 0xa8];
};

typedef struct Eng3KindEntry Eng3KindEntry;
struct Eng3KindEntry {
    unsigned long value;
    unsigned short reserved_004;
    unsigned char category;
    unsigned char reserved_007;
};

typedef char eng3_actor_prefix_size[(sizeof(Eng3ActorBase) == 0x94) ? 1 : -1];
typedef char eng3_middle_actor_size[(sizeof(Eng3MiddleActor) == 0xec) ? 1 : -1];
typedef char eng3_kind_entry_size[(sizeof(Eng3KindEntry) == 8) ? 1 : -1];
typedef char eng3_middle_copy_x_offset[(offsetof(Eng3MiddleActor, position_copy_x) == 0x9c) ? 1 : -1];

extern Eng3MiddleActor *allocate_eng3_middle_actor(unsigned long kind);
extern void reset_eng3_middle_actor(Eng3MiddleActor *actor);
extern const Eng3KindEntry *g_eng3_middle_kind_entries;

Eng3MiddleActor *create_eng3_middle_actor(long x, long y, long z, unsigned long kind)
{
    Eng3MiddleActor *actor;

    actor = allocate_eng3_middle_actor(kind);
    if (actor != 0) {
        reset_eng3_middle_actor(actor);
        actor->base.kind = (unsigned short)(kind | 0x4000);
        actor->base.x = x;
        actor->position_copy_x = x;
        actor->base.y = y;
        actor->position_copy_y = y;
        actor->base.z = z;
        actor->position_copy_z = z;
        actor->base.category = g_eng3_middle_kind_entries[kind & 0x0fff].category;
        actor->base.table_value = g_eng3_middle_kind_entries[kind & 0x0fff].value;
    }
    return actor;
}
