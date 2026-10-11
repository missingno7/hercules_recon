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

extern void initialize_alternate_actor(Eng3ActorBase *actor);

void reset_eng3_middle_actor(Eng3MiddleActor *actor)
{
    initialize_alternate_actor(&actor->base);
    actor->field_094 = 0;
    actor->field_096 = 0;
    actor->field_095 = 0;
    actor->field_097 = 0;
    actor->field_09a = 0;
    actor->field_098 = 0;
    actor->position_copy_z = 0;
    actor->position_copy_y = 0;
    actor->position_copy_x = 0;
    actor->field_0b4 = 0;
    actor->field_0b0 = 0;
    actor->field_0ac = 0;
    actor->field_0a8 = 0;
    actor->field_0c4 = 0;
    actor->field_0c0 = 0;
    actor->field_0bc = 0;
    actor->field_0b8 = 0;
    actor->field_0d4 = 0;
    actor->field_0d0 = 0;
    actor->field_0cc = 0;
    actor->field_0c8 = 0;
    actor->field_0e0 = 0;
    actor->field_0dc = 0;
    actor->field_0d8 = 0;
    actor->field_0e8 = 0;
    actor->field_0e4 = 0;
}
