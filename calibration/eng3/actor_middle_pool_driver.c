#include <windows.h>
#include <stddef.h>

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
    unsigned char field_094;
    unsigned char field_095;
    unsigned char field_096;
    unsigned char field_097;
    unsigned short field_098;
    unsigned short field_09a;
    unsigned long position_copy_x;
    unsigned long position_copy_y;
    unsigned long position_copy_z;
    unsigned long field_0a8;
    unsigned long field_0ac;
    unsigned long field_0b0;
    unsigned long field_0b4;
    unsigned long field_0b8;
    unsigned long field_0bc;
    unsigned long field_0c0;
    unsigned long field_0c4;
    unsigned long field_0c8;
    unsigned long field_0cc;
    unsigned long field_0d0;
    unsigned long field_0d4;
    unsigned long field_0d8;
    unsigned long field_0dc;
    unsigned long field_0e0;
    unsigned long field_0e4;
    unsigned long field_0e8;
};

typedef char eng3_actor_base_size_is_0x94[(sizeof(Eng3ActorBase) == 0x94) ? 1 : -1];
typedef char eng3_middle_actor_size_is_0xec[(sizeof(Eng3MiddleActor) == 0xec) ? 1 : -1];
typedef char eng3_actor_kind_offset_is_0x2e[(offsetof(Eng3ActorBase, kind) == 0x2e) ? 1 : -1];
typedef char eng3_middle_copy_x_offset_is_0x9c[(offsetof(Eng3MiddleActor, position_copy_x) == 0x9c) ? 1 : -1];

typedef struct Actor Actor;

extern unsigned short g_eng3_middle_limit_primary;
extern Eng3MiddleActor *g_eng3_middle_actor_pool;
extern int remove_actor_variant(Eng3MiddleActor *actor);
extern void reset_eng3_middle_actor(Eng3MiddleActor *actor);
extern void clear_actor(Actor *actor);

void eng3_process_middle_actor_pool(void)
{
    int index;

    for (index = 0; index < g_eng3_middle_limit_primary; ++index) {
        Eng3MiddleActor *actor = &g_eng3_middle_actor_pool[index];
        if (remove_actor_variant(actor) == 0)
            clear_actor((Actor *)actor);
        else
            reset_eng3_middle_actor(actor);
    }
}
