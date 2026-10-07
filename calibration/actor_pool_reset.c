#include <stddef.h>

typedef unsigned short ActorKind;
typedef long ActorScalar;
typedef unsigned long ActorFlags;

typedef struct ActorCore ActorCore;
struct ActorCore {
    long x;
    long y;
    long z;
    unsigned short field_00c;
    unsigned short field_00e;
    unsigned short field_010;
    unsigned short field_012;
    unsigned short field_014;
    unsigned short field_016;
    unsigned short field_018;
    unsigned short field_01a;
    unsigned char field_01c;
    unsigned char field_01d;
    unsigned char field_01e;
    unsigned char field_01f;
    unsigned char field_020;
    unsigned char field_021;
    unsigned char category;
    unsigned char field_023;
    unsigned char field_024;
    unsigned char field_025;
    unsigned short field_026;
    unsigned short field_028;
    unsigned short field_02a;
    unsigned short field_02c;
    ActorKind kind;
    unsigned short field_030;
    unsigned short field_032;
    unsigned short field_034;
    unsigned short field_036;
    unsigned short field_038;
    unsigned short field_03a;
    unsigned short field_03c;
    unsigned short field_03e;
    unsigned short field_040;
    unsigned short field_042;
    unsigned short field_044;
    unsigned short field_046;
    unsigned short field_048;
    unsigned short field_04a;
    ActorScalar field_04c;
    ActorFlags field_050;
    ActorFlags field_054;
    unsigned long field_058;
    ActorCore *field_05c;
    ActorCore *field_060;
    unsigned long field_064;
    unsigned long field_068;
    unsigned long field_06c;
    unsigned short field_070;
    unsigned short field_072;
    unsigned short field_074;
    unsigned short field_076;
    unsigned char unknown_078[0x10];
    unsigned short field_088;
    unsigned short field_08a;
    unsigned short field_08c;
    unsigned short field_08e;
    unsigned short field_090;
    unsigned short field_092;
};

typedef struct ActorMiddle ActorMiddle;
struct ActorMiddle {
    ActorCore base;
    unsigned char field_094;
    unsigned char field_095;
    unsigned char field_096;
    unsigned char field_097;
    unsigned short field_098;
    unsigned short field_09a;
    unsigned long field_09c;
    unsigned long field_0a0;
    unsigned long field_0a4;
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

typedef struct ActorNormal ActorNormal;
struct ActorNormal {
    ActorMiddle base;
    unsigned char field_0ec;
    unsigned char field_0ed;
    unsigned short field_0ee;
    unsigned long field_0f0;
    unsigned long field_0f4;
    unsigned long field_0f8;
    unsigned long field_0fc;
    unsigned long field_100;
    unsigned long field_104;
    unsigned long field_108;
    unsigned long field_10c;
    unsigned long field_110;
    unsigned short field_114;
    unsigned short field_116;
    unsigned long field_118;
    unsigned short field_11c;
    unsigned short field_11e;
    unsigned long field_120;
    unsigned short field_124;
    unsigned short field_126;
    unsigned long field_128;
    unsigned long field_12c;
    unsigned long field_130;
};

typedef char actor_core_size_is_0x94[(sizeof(ActorCore) == 0x94) ? 1 : -1];
typedef char actor_middle_size_is_0xec[(sizeof(ActorMiddle) == 0xec) ? 1 : -1];
typedef char actor_normal_size_is_0x134[(sizeof(ActorNormal) == 0x134) ? 1 : -1];
typedef char actor_kind_offset_is_0x2e[(offsetof(ActorCore, kind) == 0x2e) ? 1 : -1];
typedef char actor_scalar_offset_is_0x4c[(offsetof(ActorCore, field_04c) == 0x4c) ? 1 : -1];
typedef char actor_flags_offsets_are_0x50_0x54[(offsetof(ActorCore, field_050) == 0x50 && offsetof(ActorCore, field_054) == 0x54) ? 1 : -1];
typedef char actor_pointer_offsets_are_0x5c_0x60[(offsetof(ActorCore, field_05c) == 0x5c && offsetof(ActorCore, field_060) == 0x60) ? 1 : -1];
typedef char actor_scalar_width_is_4[(sizeof(((ActorCore *)0)->field_04c) == 4) ? 1 : -1];
typedef char actor_flags_width_is_4[(sizeof(((ActorCore *)0)->field_050) == 4 && sizeof(((ActorCore *)0)->field_054) == 4) ? 1 : -1];
typedef char actor_pointer_width_is_4[(sizeof(((ActorCore *)0)->field_05c) == 4 && sizeof(((ActorCore *)0)->field_060) == 4) ? 1 : -1];

extern ActorNormal *g_normal_actor_pool;
extern ActorCore *g_alternate_actor_pool;
extern unsigned short g_normal_cursor_primary;
extern unsigned short g_normal_limit_primary;
extern unsigned short g_normal_limit_wrap;
extern unsigned short g_normal_cursor_alternate;
extern unsigned short g_normal_limit_wrap_start;
extern unsigned short g_alternate_cursor_primary;
extern unsigned short g_alternate_limit_primary;
extern unsigned short g_alternate_limit_wrap;
extern unsigned short g_alternate_cursor_alternate;
extern unsigned short g_alternate_limit_wrap_start;

ActorCore *allocate_alternate_actor(ActorKind kind)
{
    ActorCore *pool;
    int original;
    int limit;
    int index;

    if (kind & 0x8000) {
        pool = g_alternate_actor_pool;
        original = g_alternate_cursor_primary;
        limit = g_alternate_limit_primary;
        index = original;
        while (index < limit && pool[index].kind != 0)
            ++index;
        if (index < limit) {
            g_alternate_cursor_primary = (unsigned short)index;
            return &pool[index];
        }
        index = g_alternate_limit_wrap;
        if (index >= original)
            return 0;
        while (index < original && pool[index].kind != 0)
            ++index;
        if (index >= original)
            return 0;
        g_alternate_cursor_primary = (unsigned short)index;
        return &pool[index];
    }

    pool = g_alternate_actor_pool;
    original = g_alternate_cursor_alternate;
    limit = g_alternate_limit_wrap;
    index = original;
    while (index < limit && pool[index].kind != 0)
        ++index;
    if (index < limit) {
        g_alternate_cursor_alternate = (unsigned short)index;
        return &pool[index];
    }
    index = g_alternate_limit_wrap_start;
    if (index >= original)
        return 0;
    while (index < original && pool[index].kind != 0)
        ++index;
    if (index >= original)
        return 0;
    g_alternate_cursor_alternate = (unsigned short)index;
    return &pool[index];
}

ActorNormal *allocate_normal_actor(ActorKind kind)
{
    ActorNormal *pool;
    int original;
    int limit;
    int index;

    if (kind & 0x8000) {
        pool = g_normal_actor_pool;
        original = g_normal_cursor_primary;
        limit = g_normal_limit_primary;
        index = original;
        while (index < limit && pool[index].base.base.kind != 0)
            ++index;
        if (index < limit) {
            g_normal_cursor_primary = (unsigned short)index;
            return &pool[index];
        }
        index = g_normal_limit_wrap;
        if (index >= original)
            return 0;
        while (index < original && pool[index].base.base.kind != 0)
            ++index;
        if (index >= original)
            return 0;
        g_normal_cursor_primary = (unsigned short)index;
        return &pool[index];
    }

    pool = g_normal_actor_pool;
    original = g_normal_cursor_alternate;
    limit = g_normal_limit_wrap;
    index = original;
    while (index < limit && pool[index].base.base.kind != 0)
        ++index;
    if (index < limit) {
        g_normal_cursor_alternate = (unsigned short)index;
        return &pool[index];
    }
    index = g_normal_limit_wrap_start;
    if (index >= original)
        return 0;
    while (index < original && pool[index].base.base.kind != 0)
        ++index;
    if (index >= original)
        return 0;
    g_normal_cursor_alternate = (unsigned short)index;
    return &pool[index];
}

void initialize_alternate_actor(ActorCore *actor)
{
    actor->field_01d = 0xff;
    actor->field_01e = 0xff;
    actor->field_01c = 0xff;
    actor->field_054 = 0x80000000UL;
    actor->field_01f = 0;
    actor->category = 1;
    actor->field_030 = 0;
    actor->kind = 0;
    actor->field_050 = 2;
    actor->field_032 = 0xffff;
    actor->field_038 = 0;
    actor->field_036 = 0;
    actor->field_034 = 0;
    actor->field_012 = 0;
    actor->field_010 = 0;
    actor->field_00e = 0;
    actor->field_00c = 0;
    actor->field_076 = 0;
    actor->field_074 = 0;
    actor->field_072 = 0;
    actor->field_070 = 0;
    actor->field_03e = 0x100;
    actor->field_03c = 0x100;
    actor->field_03a = 0x100;
    actor->field_08c = 0;
    actor->field_08a = 0;
    actor->field_088 = 0;
    actor->field_048 = 0xffff;
    actor->field_042 = 0xffff;
    actor->field_046 = 0xffff;
    actor->field_044 = 0xffff;
    actor->field_040 = 0xffff;
    actor->field_092 = 0;
    actor->field_090 = 0;
    actor->field_020 = 0xff;
    actor->field_08e = 0;
    actor->field_023 = 0;
    actor->field_02a = 0;
    actor->field_06c = 0;
    actor->field_068 = 0;
    actor->field_064 = 0;
    actor->field_058 = 0;
    actor->field_05c = 0;
    actor->field_060 = 0;
    actor->z = 0;
    actor->y = 0;
    actor->x = 0;
    actor->field_04a = 0x80;
    actor->field_024 = 0x80;
    actor->field_02c = 0;
    actor->field_026 = 0;
    actor->field_021 = 0;
    actor->field_04c = 0;
    actor->field_025 = 0;
    actor->field_018 = 0;
    actor->field_016 = 0;
    actor->field_014 = 0;
    actor->field_028 = 0;
}

void initialize_middle_actor(ActorMiddle *actor)
{
    initialize_alternate_actor(&actor->base);
    actor->field_094 = 0;
    actor->field_096 = 0;
    actor->field_095 = 0;
    actor->field_097 = 0;
    actor->field_09a = 0;
    actor->field_098 = 0;
    actor->field_0a4 = 0;
    actor->field_0a0 = 0;
    actor->field_09c = 0;
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

void initialize_normal_actor(ActorNormal *actor)
{
    initialize_middle_actor(&actor->base);
    actor->field_0ed = 0;
    actor->field_0ec = 0;
    actor->field_0ee = 1;
    actor->field_0f8 = 0;
    actor->field_0f4 = 0;
    actor->field_0f0 = 0;
    actor->field_104 = 0;
    actor->field_100 = 0;
    actor->field_0fc = 0;
    actor->field_10c = 0;
    actor->field_108 = 0;
    actor->field_110 = 0;
    actor->field_114 = 0xffff;
    actor->field_116 = 0;
    actor->field_118 = 0;
    actor->field_11c = 0xffff;
    actor->field_11e = 0;
    actor->field_120 = 0;
    actor->field_124 = 0;
    actor->field_126 = 0;
    actor->field_130 = 0;
    actor->field_12c = 0;
    actor->field_128 = 0;
}


extern ActorMiddle *g_middle_actor_pool;
extern unsigned short g_middle_cursor_primary;
extern unsigned short g_middle_limit_primary;
extern unsigned short g_middle_wrap_start_primary;
extern unsigned short g_middle_cursor_alternate;
extern unsigned short g_middle_wrap_start_alternate;

extern int remove_actor(void *actor);
extern int remove_middle_actor(void *actor);
extern int release_actor(void *actor);
extern void clear_actor(void *actor);

ActorMiddle *allocate_middle_actor(ActorKind kind)
{
    ActorMiddle *pool;
    int original;
    int limit;
    int index;

    pool = g_middle_actor_pool;
    if (kind & 0x8000) {
        original = g_middle_cursor_primary;
        limit = g_middle_limit_primary;
        index = original;
        while (index < limit && pool[index].base.kind != 0)
            ++index;
        if (index < limit) {
            g_middle_cursor_primary = (unsigned short)index;
            return &pool[index];
        }
        index = g_middle_wrap_start_primary;
        if (index >= original)
            return 0;
        while (index < original && pool[index].base.kind != 0)
            ++index;
        if (index >= original)
            return 0;
        g_middle_cursor_primary = (unsigned short)index;
        return &pool[index];
    }

    original = g_middle_cursor_alternate;
    limit = g_middle_wrap_start_primary;
    index = original;
    while (index < limit && pool[index].base.kind != 0)
        ++index;
    if (index < limit) {
        g_middle_cursor_alternate = (unsigned short)index;
        return &pool[index];
    }
    index = g_middle_wrap_start_alternate;
    if (index >= original)
        return 0;
    while (index < original && pool[index].base.kind != 0)
        ++index;
    if (index >= original)
        return 0;
    g_middle_cursor_alternate = (unsigned short)index;
    return &pool[index];
}

void reset_normal_actor_pool(void)
{
    ActorNormal *actor;
    int index;

    if (g_normal_limit_primary == 0)
        return;
    actor = g_normal_actor_pool;
    for (index = 0; index < g_normal_limit_primary; ++index, ++actor) {
        if (remove_actor(actor))
            initialize_normal_actor(actor);
        else
            clear_actor(actor);
    }
}

void reset_middle_actor_pool(void)
{
    ActorMiddle *actor;
    int index;

    if (g_middle_limit_primary == 0)
        return;
    actor = g_middle_actor_pool;
    for (index = 0; index < g_middle_limit_primary; ++index, ++actor) {
        if (remove_middle_actor(actor))
            initialize_middle_actor(actor);
        else
            clear_actor(actor);
    }
}

void reset_alternate_actor_pool(void)
{
    ActorCore *actor;
    int index;

    if (g_alternate_limit_primary == 0)
        return;
    actor = g_alternate_actor_pool;
    for (index = 0; index < g_alternate_limit_primary; ++index, ++actor) {
        if (release_actor(actor))
            initialize_alternate_actor(actor);
        else
            clear_actor(actor);
    }
}
