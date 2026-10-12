#include "resource_runtime_data.h"
#include "incomplete.h"
#include "spatial_read_guard.h"
/*
 * Private ordinary-C behavioral candidate for the native ENG1 callback family.
 * The two structs below are offset views only; they do not identify original
 * aggregates or translation-unit ownership. Unknown cells retain RVA/offset
 * names. External behavior not reconstructed here remains an RVA-named call.
 */
#include <stddef.h>

typedef struct SpatialEffectView SpatialEffectView;
struct SpatialEffectView {
    long x, y, z;
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category_022;
    unsigned char mode_023;
    unsigned char byte_024;
    unsigned char byte_025;
    unsigned short word_026;
    unsigned short word_028;
    unsigned short word_02a;
    unsigned char unknown_02c[4];
    unsigned short word_030;
    unsigned short word_032;
    unsigned short sprite_034;
    unsigned short word_036;
    unsigned short word_038;
    unsigned short scale_03a;
    unsigned short word_03c;
    unsigned char unknown_03e[0x4a - 0x3e];
    unsigned short word_04a;
    unsigned long dword_04c;
    unsigned long flags_050;
    unsigned long render_flags_054;
    unsigned char unknown_058[4];
    SpatialEffectView *attachment_05c;
    unsigned char unknown_060[0x70 - 0x60];
    unsigned short word_070;
    unsigned short word_072;
    unsigned short word_074;
};

/* Larger frame/motion object view. Only directly observed cells are named. */
typedef struct SpatialTaskView SpatialTaskView;
struct SpatialTaskView {
    long x, y, z;
    unsigned char unknown_00c[0x1c - 0x0c];
    unsigned char byte_01c;
    unsigned char unknown_01d[0x22 - 0x1d];
    unsigned char byte_022;
    unsigned char byte_023;
    unsigned char byte_024;
    unsigned char byte_025;
    unsigned short word_026;
    unsigned short word_028;
    unsigned short word_02a;
    unsigned char unknown_02c[0x38 - 0x2c];
    unsigned short word_038;
    unsigned char unknown_03a[0x4a - 0x3a];
    unsigned short word_04a;
    unsigned char unknown_04c[4];
    unsigned long dword_050;
    unsigned long flags_054;
    void *pointer_058;
    unsigned char unknown_05c[0x70 - 0x5c];
    unsigned short word_070;
    unsigned short word_072;
    unsigned short word_074;
    unsigned char unknown_076[0x94 - 0x76];
    unsigned char byte_094;
    unsigned char byte_095;
    unsigned char byte_096;
    unsigned char byte_097;
    unsigned char unknown_098[0xa0 - 0x98];
    unsigned long dword_0a0;
    unsigned char unknown_0a4[0xac - 0xa4];
    unsigned long dword_0ac;
    unsigned long dword_0b0;
    unsigned char unknown_0b4[4];
    unsigned long dword_0b8;
    unsigned long dword_0bc;
    unsigned long dword_0c0;
    unsigned long dword_0c4;
    unsigned char unknown_0c8[0xd8 - 0xc8];
    unsigned long dword_0d8;
    unsigned char unknown_0dc[4];
    unsigned long dword_0e0;
    unsigned char unknown_0e4[0x124 - 0xe4];
    unsigned short word_124;
    unsigned short word_126;
};

#define CHECK_OFFSET(type, member, value) \
    typedef char check_##type##_##member[(offsetof(type, member) == (value)) ? 1 : -1]
CHECK_OFFSET(SpatialEffectView, category_022, 0x22);
CHECK_OFFSET(SpatialEffectView, word_02a, 0x2a);
CHECK_OFFSET(SpatialEffectView, sprite_034, 0x34);
CHECK_OFFSET(SpatialEffectView, scale_03a, 0x3a);
CHECK_OFFSET(SpatialEffectView, flags_050, 0x50);
CHECK_OFFSET(SpatialEffectView, render_flags_054, 0x54);
CHECK_OFFSET(SpatialEffectView, attachment_05c, 0x5c);
CHECK_OFFSET(SpatialEffectView, word_070, 0x70);
CHECK_OFFSET(SpatialTaskView, byte_01c, 0x1c);
CHECK_OFFSET(SpatialTaskView, word_02a, 0x2a);
CHECK_OFFSET(SpatialTaskView, word_038, 0x38);
CHECK_OFFSET(SpatialTaskView, word_04a, 0x4a);
CHECK_OFFSET(SpatialTaskView, flags_054, 0x54);
CHECK_OFFSET(SpatialTaskView, pointer_058, 0x58);
CHECK_OFFSET(SpatialTaskView, byte_094, 0x94);
CHECK_OFFSET(SpatialTaskView, dword_0a0, 0xa0);
CHECK_OFFSET(SpatialTaskView, dword_0bc, 0xbc);
CHECK_OFFSET(SpatialTaskView, dword_0d8, 0xd8);
CHECK_OFFSET(SpatialTaskView, dword_0e0, 0xe0);
CHECK_OFFSET(SpatialTaskView, word_124, 0x124);
#undef CHECK_OFFSET

/* Direct target calls with no source provider are intentionally unresolved. */
extern long __cdecl eng1_native_dep_020b50(long, long, long, int);
extern unsigned long __cdecl eng1_native_dep_0299d0(void *);
extern unsigned int __cdecl dispatch_engine_2c460(int);
extern unsigned long __cdecl eng1_native_dep_02c470(void);
extern void __cdecl eng1_native_dep_02a290(void *);
extern void __cdecl eng1_native_dep_0283e0(void *);
extern unsigned long __cdecl eng1_native_dep_02a170(long, long, long, int);
extern int __cdecl eng1_native_dep_029c20(void *);
extern void __cdecl eng1_native_dep_02bc50(void *);
extern void __cdecl eng1_native_dep_00ef10(void *);
extern unsigned long __cdecl eng1_native_dep_02bd20(void *);
extern unsigned long __cdecl eng1_native_dep_00ff70(void *, int);
extern void __cdecl eng1_native_dep_006da0(unsigned long, void *);
extern void *__cdecl eng1_native_dep_0300c0(long, long, long, unsigned long, unsigned long);
extern int __cdecl eng1_native_dep_02aa30(void *, int, int);

/* Existing source providers: the ABI follows their compiled C interfaces. */
extern void __cdecl advance_frame(void *);
extern void __cdecl restart_steps(void *);
extern void __cdecl eng1_guarded_apply_frame_motion_3d(void *);
extern void __cdecl select_frame(void *, int);
extern int __cdecl remove_actor(void *);
extern int __cdecl remove_actor_variant(void *);

/* Observed absolute cells remain external: no storage or owner is guessed. */
extern unsigned char eng1_native_data_051fd4[];
extern unsigned short eng1_native_cell_0718e0;
extern unsigned short eng1_native_cell_0716a8;
extern unsigned long eng1_native_cell_071718;
extern unsigned short *eng1_native_ptr_0713bc;
extern unsigned short *eng1_native_ptr_071714;
/* 6fee8 is the inline ResourceKindInfo row owner, not a pointer cell. */
extern const long *eng1_native_ptr_0734f4;

static long add_bits(long a, unsigned long b)
{
    return (long)((unsigned long)a + b);
}

static long sub_bits(long a, unsigned long b)
{
    return (long)((unsigned long)a - b);
}

static unsigned short add_word(unsigned short a, unsigned long b)
{
    return (unsigned short)(a + b);
}

static unsigned short sub_word(unsigned short a, unsigned long b)
{
    return (unsigned short)(a - b);
}

static unsigned short projection_scale_delta(unsigned short base,
                                             unsigned long projected_bits)
{
    long projected = (long)projected_bits;
    return (unsigned short)(base - (unsigned short)(projected >> 17));
}

static void project_effect(SpatialEffectView *p, unsigned long projected_bits,
                           unsigned short source_scale, unsigned short step)
{
    unsigned short value;
    p->y = add_bits(p->y, projected_bits);
    value = projection_scale_delta(source_scale, projected_bits);
    p->word_03c = value;
    p->scale_03a = value;
    p->word_070 = sub_word(step, 0xff);
    p->word_072 = 0x40;
}

void __cdecl eng1_native_code_target_00f0a0(SpatialEffectView *p)
{
    SpatialEffectView *source = p->attachment_05c;
    long projected;
    unsigned long projected_bits;

    if ((source->flags_050 & 8) == 0)
        p->y = sub_bits(source->y, 0x20000);
    p->x = source->x;
    p->z = source->z;

    p->render_flags_054 &= 0xf3ffffefUL;
    p->render_flags_054 |= source->render_flags_054 & 0x10;
    p->render_flags_054 |= 0x80000000UL;
    p->render_flags_054 |= source->render_flags_054 & 0x08000000UL;
    p->render_flags_054 |= 0x04000000UL;
    p->category_022 = source->category_022;
    p->sprite_034 = source->sprite_034;
    p->word_036 = source->word_036;

    projected = eng1_native_dep_020b50(p->x >> 16, p->y >> 16,
                                       p->z >> 16, p->word_038);
    projected_bits = (unsigned long)projected << 16;
    if ((long)projected_bits > 0x01000000L) {
        p->render_flags_054 &= 0x7fffffffUL;
        return;
    }
    project_effect(p, projected_bits, source->scale_03a, source->word_070);
}

void __cdecl eng1_native_code_target_00f180(SpatialEffectView *p)
{
    SpatialEffectView *source = p->attachment_05c;
    long projected;
    unsigned long projected_bits;
    unsigned long capped;
    unsigned short world_limit;
    unsigned short height_limit;
    long source_y;
    unsigned short next_step;
    short signed_step;
    long next_y;
    unsigned long world_limit_bits;

    if ((source->flags_050 & 8) == 0) {
        p->y = source->y;
        p->x = source->x;
    }
    p->y = sub_bits(p->y, 0x20000);
    p->z = source->z;
    p->render_flags_054 &= ~0x10UL;
    p->render_flags_054 |= source->render_flags_054 & 0x10;
    p->render_flags_054 |= 0x80000000UL;
    p->category_022 = source->category_022;
    p->sprite_034 = source->sprite_034;
    p->byte_024 = source->byte_024;
    p->word_04a = source->word_04a;
    source->render_flags_054 &= 0xfff8ffffUL;
    p->render_flags_054 &= 0xfff8fffeUL;

    projected = eng1_native_dep_020b50(p->x >> 16, p->y >> 16,
                                       p->z >> 16, p->word_038);
    capped = (unsigned long)projected & 0xffffUL;
    projected = (long)(eng1_native_dep_0299d0(p) & 0xffffUL);
    if (capped > (unsigned long)projected)
        capped = (unsigned long)projected;
    projected_bits = capped << 16;
    if ((long)projected_bits > 0x01000000L) {
        p->render_flags_054 &= 0x7fffffffUL;
        return;
    }

    p->y = add_bits(p->y, projected_bits);
    p->word_03c = projection_scale_delta(source->scale_03a, projected_bits);
    p->scale_03a = p->word_03c;
    p->word_072 = 0x40;
    p->word_070 = sub_word(source->word_070, 0xff);

    world_limit = eng1_native_cell_0718e0;
    if (world_limit != 0) {
        world_limit_bits = (unsigned long)world_limit << 16;
        if (p->y > (long)world_limit_bits) {
            height_limit = eng1_native_cell_0716a8;
            p->word_070 = add_word(source->word_070, 0xff);
            p->word_072 = height_limit;
            p->word_03c = add_word(p->word_03c,
                                   eng1_native_cell_071718 + 0x20UL);
            p->render_flags_054 |= 1;
            p->z = sub_bits(source->z, 0x40000);
            source_y = source->y >> 16;
            if (source_y - (long)world_limit <= 0) {
                p->y = (long)((unsigned long)world_limit << 16);
                return;
            }
            source->word_026 = (unsigned short)(source_y - world_limit);
            next_step = sub_word(source->word_026, 1);
            signed_step = (short)next_step;
            p->word_026 = next_step;
            next_y = (long)world_limit + signed_step;
            p->y = (long)((unsigned long)next_y << 16);
            p->render_flags_054 |= 0x50000UL;
            source->render_flags_054 |= 0x30000UL;
            return;
        }
        p->word_04a = 0;
        p->word_026 = 0;
        source->word_026 = 0;
        return;
    }

    p->word_04a = 0;
    p->render_flags_054 |= source->render_flags_054 & 0x10;
    p->render_flags_054 |= 0x80000000UL;
    p->render_flags_054 |= source->render_flags_054 & 0x08000000UL;
    p->render_flags_054 |= 0x04000000UL;
}

void __cdecl eng1_native_code_target_00f380(SpatialEffectView *p)
{
    p->flags_050 = 0x10000;
    p->render_flags_054 |= 0x10000604UL;
    p->word_04a = 0x10;
    p->category_022 = 0;
    p->sprite_034 = 0;
}

void __cdecl eng1_native_code_target_00f3b0(SpatialEffectView *p)
{
    SpatialEffectView *source = p->attachment_05c;
    long projected;
    unsigned long projected_bits;

    if ((source->flags_050 & 8) == 0)
        p->y = sub_bits(source->y, 0x20000);
    p->x = source->x;
    p->z = add_bits(source->z, 0x280000);
    projected = eng1_native_dep_020b50(p->x >> 16, p->y >> 16,
                                       p->z >> 16, p->word_038);
    projected_bits = (unsigned long)projected << 16;
    if ((long)projected_bits > 0x01000000L) {
        p->render_flags_054 &= 0x7fffffffUL;
        return;
    }
    p->y = add_bits(p->y, projected_bits);
    p->word_072 = 0x80;
    p->scale_03a = projection_scale_delta(source->scale_03a, projected_bits);
}

void __cdecl eng1_native_code_target_00f430(SpatialTaskView *p)
{
    unsigned long value;

    p->word_124 = 3;
    p->word_126 = 4;
    p->byte_096 = 0xff;
    value = dispatch_engine_2c460(4);
    p->byte_01c = eng1_native_data_051fd4[value];
    advance_frame(p);
    restart_steps(p);
    eng1_native_dep_02a290(p);
    value = dispatch_engine_2c460(2);
    p->byte_094 = (unsigned char)(0 - (unsigned char)value);
    p->word_04a = (unsigned short)(dispatch_engine_2c460(0x80) + 0x60);
    p->byte_095 = 0xff;
    value = eng1_native_dep_02c470() & 0x1ffUL;
    p->dword_0bc = (value + 0xc00UL) << 8;
    p->dword_0b8 = 0;
    p->dword_0c0 = 0;
    p->dword_0c4 = dispatch_engine_2c460(0x2000) + 0x8000UL;
}

static unsigned short *tile_words(void)
{
    return eng1_native_ptr_0713bc;
}

static unsigned short tile_word(unsigned long index)
{
    unsigned short *words = tile_words();
    eng1_guard_spatial_read(words, index, 2UL, 2UL, "spatial effect collision");
    return words[index];
}

static unsigned long tile_at(long x, long y, unsigned long plane,
                             unsigned short step)
{
    return eng1_native_dep_02a170(x, y, (long)plane, step);
}

void __cdecl eng1_native_code_target_00f4f0(SpatialTaskView *p)
{
    unsigned long plane;
    unsigned long tile;
    unsigned short tile_flags;
    unsigned short selector;
    long anchor_delta;

    p->flags_054 |= 8;
    p->byte_097 = 0;
    p->word_028 = 0;
    if (p->byte_025 & 1) {
        eng1_native_dep_02a290(p);
    } else {
        eng1_native_dep_0283e0(p);
        eng1_guarded_apply_frame_motion_3d(p);
        plane = ((unsigned long)add_bits(p->z, 0x400000UL)) & 0x0f800000UL;
        selector = p->word_02a;
        if (selector == 1 || selector == 11) {
            if (p->z < (long)p->dword_0e0) {
                tile = tile_at(p->x, p->y, plane, p->word_038);
                tile_flags = tile_word(tile);
                if ((tile_flags & 0x20) != 0) {
                    p->dword_0e0 = add_bits((long)p->dword_0e0, 0x800000UL);
                } else {
                    p->byte_094 = (unsigned char)~p->byte_094;
                    p->z = (long)p->dword_0e0;
                    select_frame(p, 0x15);
                    p->word_02a = 0;
                }
            }
        } else if (selector == 2 || selector == 12) {
            if (p->z > (long)p->dword_0e0) {
                tile = tile_at(p->x, p->y, plane, p->word_038);
                tile_flags = tile_word(tile);
                if ((tile_flags & 0x10) != 0) {
                    p->dword_0e0 = add_bits((long)p->dword_0e0, 0x800000UL);
                } else {
                    p->byte_094 = (unsigned char)~p->byte_094;
                    p->z = (long)p->dword_0e0;
                    select_frame(p, 0x15);
                    p->word_02a = 0;
                }
            }
        } else if (selector != 3) {
            long tile_x;
            long tile_y;
            unsigned long directional;

            tile_y = sub_bits(p->y, 0x100000UL);
            directional = p->byte_094 ? 0xffd00000UL : 0;
            tile_x = add_bits(p->x, directional + 0x180000UL);
            tile = tile_at(tile_x, tile_y, plane, p->word_038);
            tile_flags = tile_word(tile);
            if ((tile_flags & 3) != 0) {
                select_frame(p, 9);
            } else {
                tile = tile_at(p->x, p->y, plane, p->word_038);
                tile_flags = tile_word(tile);
                if ((tile_flags & 0xc0) != 0)
                    select_frame(p, 0x1b);
            }
        }
    }

    if (eng1_native_dep_029c20(p) == 0) {
        eng1_native_dep_02bc50(p);
        eng1_native_dep_00ef10(p);
    }
    p->flags_054 = (p->flags_054 & ~0x10UL) | (p->byte_094 & 0x10);

    if (p->y > 0x04f00000L) {
        if (p->pointer_058 != 0) {
            unsigned short index = p->word_038;
            unsigned long offset;
            unsigned short *record;
            if (index >= 1)
                eng1_incomplete_behavior(0x6fee8UL, "spatial callback kind index outside measured one-row prefix");
            offset = (unsigned long)g_kind_info[index].owner;
            record = (unsigned short *)((unsigned long)p->pointer_058 + offset + 0x0cUL);
            *record &= 0xfeff;
        }
        remove_actor(p);
        return;
    }

    anchor_delta = sub_bits(p->x, (unsigned long)eng1_native_ptr_0734f4[0]) >> 16;
    {
        unsigned long absolute_delta = (unsigned long)anchor_delta;
        if (anchor_delta < 0)
            absolute_delta = 0UL - absolute_delta;
        if ((long)absolute_delta > 0x500L)
            remove_actor(p);
    }
}

void __cdecl eng1_native_code_target_00fd00(SpatialTaskView *p)
{
    unsigned long value;
    unsigned short selector = p->word_02a;

    p->byte_094 = (unsigned char)selector;
    p->flags_054 |= 0x10000000UL;
    p->byte_095 = 0;

    if (selector == 0) {
        value = eng1_native_dep_02c470() & 0x1ffUL;
        p->dword_0bc = (value + 0x480UL) << 8;
        p->dword_0b8 = p->dword_0bc >> 2;
        p->dword_0c0 = dispatch_engine_2c460(0x1000) + 0x2000UL;
        p->dword_0ac = 0x20000;
        p->dword_0b0 = 0x1000;
        p->byte_094 = 0xff;
        p->dword_0e0 = 8;
        p->dword_0d8 = 2;
    } else if (selector == 1) {
        value = eng1_native_dep_02c470() & 0x1ffUL;
        p->dword_0bc = (value + 0x600UL) << 8;
        p->dword_0b8 = p->dword_0bc >> 2;
        p->dword_0c0 = dispatch_engine_2c460(0x2000) + 0x2000UL;
        p->dword_0e0 = 0;
        p->dword_0d8 = 6;
    } else if (selector == 2) {
        value = eng1_native_dep_02c470() & 0x1ffUL;
        p->dword_0bc = (value + 0x400UL) << 8;
        p->dword_0b8 = p->dword_0bc >> 2;
        p->dword_0c0 = dispatch_engine_2c460(0x1000) + 0x1000UL;
        p->dword_0ac = 0x20000;
        p->dword_0b0 = 0x1000;
        p->byte_094 = 0;
        p->dword_0e0 = 0xfffffff4UL;
        p->dword_0d8 = 0;
    }
}

void __cdecl eng1_native_code_target_00fe60(SpatialTaskView *p)
{
    unsigned long collision;
    unsigned long value;
    unsigned long z;
    long y;
    long x;
    unsigned short event;
    void *spawned;

    p->word_070 = add_word(p->word_070, p->dword_0d8);
    p->word_074 = add_word(p->word_074, p->dword_0e0);
    p->y = add_bits(p->y, 0x200000UL);
    p->dword_0a0 = add_bits((long)p->dword_0a0, 0x200000UL);

    collision = eng1_native_dep_02bd20(p);
    if ((collision & 1) != 0) {
        value = eng1_native_dep_00ff70(p, 0x10);
        event = *eng1_native_ptr_071714;
        event |= 0xc000;
        eng1_native_dep_006da0((value & 0xffff0000UL) | event, p);

        z = (unsigned long)p->z - 0x200000UL;
        x = p->x;
        y = sub_bits(p->y, 0x200000UL);
        (void)eng1_native_dep_0300c0(x, y, (long)z, 0x2027, 0);

        z = (unsigned long)p->z - 0x200000UL;
        value = dispatch_engine_2c460(0x20);
        y = sub_bits(p->y, (unsigned long)value << 16);
        y = sub_bits(y, 0x100000UL);
        value = dispatch_engine_2c460(0x20);
        x = add_bits(p->x, ((unsigned long)(value - 0x10UL)) << 16);
        spawned = eng1_native_dep_0300c0(x, y, (long)z, 0x2027, 0);
        if (spawned != 0) {
            value = dispatch_engine_2c460(10);
            *(unsigned long *)((unsigned char *)spawned + 0x4c) = value + 10;
        }
        remove_actor_variant(p);
    }

    p->y = sub_bits(p->y, 0x200000UL);
    p->dword_0a0 = (unsigned long)sub_bits((long)p->dword_0a0, 0x200000UL);
}

void __cdecl eng1_native_code_target_0102c0(SpatialTaskView *p)
{
    unsigned long collision;
    unsigned long value;
    unsigned long gate;
    unsigned short *table;
    unsigned short event;

    p->word_070 = (unsigned short)(0x400 - (((unsigned long)(p->z >> 13)) & 0x3ff));
    p->word_072 = (unsigned short)(0x400 - (((unsigned long)(p->x >> 13)) & 0x3ff));
    collision = eng1_native_dep_02bd20(p);
    if ((collision & 2) != 0) {
        remove_actor_variant(p);
        return;
    }

    value = dispatch_engine_2c460(2);
    gate = value + 1;
    if ((unsigned long)p->word_02a < gate && (collision & 1) != 0) {
        value = dispatch_engine_2c460(2);
        table = eng1_native_ptr_071714;
        event = table[value + 1];
        event |= 0xc000;
        eng1_native_dep_006da0((value & 0xffff0000UL) | event, p);
        p->word_02a = add_word(p->word_02a, 1);
    }

    if (p->z < (long)0xff000000UL || p->y > 0x05800000L) {
        remove_actor_variant(p);
        return;
    }
    if (eng1_native_dep_02aa30(p, 0x80, 0x190) != 0)
        remove_actor_variant(p);
}
