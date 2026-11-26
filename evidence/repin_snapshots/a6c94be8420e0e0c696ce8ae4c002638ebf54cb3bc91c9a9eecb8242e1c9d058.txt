/* ENG1.DLL lane d: EXACT functions in ascending RVA order.
 * One shared Actor layout; names follow src/shared/effect_state.c where the offset coincides. */
typedef struct Actor Actor;
struct Actor {
    long x;
    long y;
    long z;
    unsigned char unknown_0c[0x21 - 0x0c];
    unsigned char category;
    unsigned char unknown_22;
    unsigned char mode;
    unsigned char unknown_24;
    unsigned char status;
    unsigned char unknown_26[4];
    short state;
    short unknown_2c;
    unsigned short kind;
    short frame_tick;
    short frame;
    unsigned short sprite;
    unsigned short unknown_36;
    unsigned short profile;
    unsigned short scale;
    short unknown_3c;
    unsigned char unknown_3e[0x4a - 0x3e];
    short unknown_4a;
    long timer;
    unsigned long flags;
    unsigned long render_flags;
    long record_offset;
    Actor *attachment;
    Actor *child;
    void *program;
    unsigned char unknown_68[8];
    short speed;
    short step;
    short unknown_74;
    unsigned char unknown_76[0x88 - 0x76];
    short unknown_88;
    short unknown_8a;
    unsigned char unknown_8c[0x94 - 0x8c];
    unsigned char unknown_94;
    unsigned char unknown_95;
    unsigned char unknown_96;
    unsigned char unknown_97;
    short unknown_98;
    short unknown_9a;
    long saved_x, saved_y, saved_z;
    int unknown_a8;
    int unknown_ac;
    int unknown_b0;
    int unknown_b4;
    int unknown_b8;
    int unknown_bc;
    int unknown_c0;
    int unknown_c4;
    int unknown_c8;
    int unknown_cc;
    int unknown_d0;
    int unknown_d4;
    int unknown_d8;
    int unknown_dc;
    int unknown_e0;
    unsigned char unknown_e4[8];
    unsigned char invuln;
    unsigned char unknown_ed;
    short hits;
    unsigned char unknown_f0[0x108 - 0xf0];
    long return_state;
    void *program_start;
    unsigned char unknown_110[4];
    short anim_id;
    unsigned char unknown_116[0x120 - 0x116];
    struct ActorList *list;
};

void set_mode_flags_36120(Actor *p)
{
    p->flags = 0x10000;
    p->render_flags |= 0x604;
    p->unknown_4a = 0x10;
    p->sprite = 0x18;
}

void reset_extent_36b60(Actor *p)
{
    p->sprite = 0;
    p->flags = 2;
    p->unknown_88 = 0xa0;
    p->unknown_8a = 0xc8;
}

void set_render_flags_3e9b0(Actor *p)
{
    p->sprite = 0x7f;
    p->render_flags |= 0x10080;
    p->unknown_3c = 0x200;
}

void set_collide_flags_3eaa0(Actor *p)
{
    p->sprite = 0x7f;
    p->render_flags |= 0x80080;
    p->flags |= 0x800;
}

void set_collide_flags_3eac0(Actor *p)
{
    p->sprite = 0x7f;
    p->render_flags |= 0x90080;
    p->unknown_3c = 0x200;
    p->flags |= 0x800;
}

void set_flags_3f040(Actor *p)
{
    p->unknown_d8 = 1;
    p->render_flags |= 0x10000000;
    p->scale = 0x300;
    p->flags |= 0x100000;
}

void adjust_bounds_3f070(Actor *p)
{
    p->flags = 0x10000;
    p->render_flags |= 0x10000000;
    p->unknown_4a = 0x10;
    p->z += 0x300000;
    p->sprite = 0x40;
    p->x -= 0x200000;
}

void apply_scaled_offsets_40ab0(Actor *p)
{
    int scale;

    if (p->sprite == 0x36)
        p->scale = 0x50;
    scale = p->unknown_98;
    p->speed += (p->unknown_d8 * scale) >> 2;
    p->step += (p->unknown_dc * scale) >> 2;
    p->unknown_74 += (p->unknown_e0 * scale) >> 2;
}

void set_state_flags_41300(Actor *p)
{
    p->flags = 0x10000;
    p->render_flags |= 0x684;
    p->mode = 5;
}

extern unsigned int __cdecl random_limit(int limit);
extern unsigned int __cdecl dispatch_engine_2c470(void);
extern Actor *__cdecl spawn_at_group_300c0(long x, long y, long z,
                                           int kind,
                                           int group);

/* ENG1.DLL 0x41da0 creates the actor's randomized 0x4037 child burst.
 * Measured VC5 steering: after the positive-count guard, retain the actor in
 * source and reuse the incoming argument as the count carrier. Native saves
 * the actor in EBP and overwrites its argument home with the loop count.
 * The integer-valued pointer carrier is never dereferenced; casts describe
 * the original 32-bit address/integer representation. A separate counter
 * local generates an additional stack slot. This does not identify original
 * variable spelling or imply whole-image matching. */
void eng1_41da0(Actor *p)
{
    Actor *source;
    long remaining;

    remaining = (long)(random_limit(8) + 8U);
    if (remaining <= 0)
        return;

    source = p;
    p = (Actor *)remaining; /* positive count, no actor address after this */
    do {
        unsigned long value;
        Actor *child;

        child = spawn_at_group_300c0(source->x,
            source->y - (long)((dispatch_engine_2c470() & 0x1f00UL) << 8),
            source->z, 0x4037U, 0U);
        if (child != 0) {
            child->unknown_94 =
                (unsigned char)(0U -
                    (unsigned char)random_limit(2));
            child->unknown_96 =
                (unsigned char)(0U -
                    (unsigned char)random_limit(2));
            child->unknown_95 = 0xffU;

            value = dispatch_engine_2c470() & 0x1ffUL;
            child->unknown_c0 = 0UL;
            value = (value + 0x800UL) << 8;
            child->unknown_bc = value;
            child->unknown_b8 = value;
            child->unknown_c4 =
                random_limit(0x2000) + 0x8000UL;

            value = dispatch_engine_2c470() & 0x1ffUL;
            value = (value + 0x300UL) << 8;
            child->unknown_cc = value;
            child->unknown_c8 =
                (unsigned long)((long)value >> 1);
            child->unknown_d0 =
                random_limit(0x1000) + 0x6000UL;
            child->unknown_d4 = 0UL;

            value = dispatch_engine_2c470() & 0x1ffUL;
            value = (value + 0x300UL) << 8;
            child->unknown_ac = value;
            child->unknown_a8 =
                (unsigned long)((long)value >> 1);
            child->unknown_b0 =
                random_limit(0x1000) + 0x6000UL;
            child->unknown_b4 = 0UL;
            child->sprite =
                (unsigned short)(random_limit(0x0b) + 1U);
        }

        p = (Actor *)((long)p - 1);
    } while (p != 0);
}


void copy_link_state_41fa0(Actor *p)
{
    Actor *q = p->attachment;

    if (q) {
        p->x = q->x;
        p->y = q->y;
        p->z = q->z;
        p->sprite = q->sprite - 0x13;
    }
}

void set_kind_422d0(Actor *p)
{
    p->flags = 0x41;
    p->render_flags = 0;
}

void set_flags_459e0(Actor *p)
{
    p->sprite = 0x2ad;
    p->render_flags |= 0x4000605;
    p->unknown_4a = 0x80;
}

void reset_links_45a50(Actor *p)
{
    p->unknown_4a = 0x80;
    p->render_flags |= 0x4000605;
    p->sprite = 0x188;
    p->unknown_a8 = 0;
    p->unknown_b8 = 0;
    p->state = 0;
}

int wrap_delta_461a0(int a, int b)
{
    int delta;
    a &= 0x3ff; b &= 0x3ff;
    if (b >= a) delta = b - a; else delta = b - a + 1024;
    if (delta >= 512) delta -= 1024;
    return delta;
}

void set_flags_461d0(Actor *p)
{
    p->unknown_4a = 0x80;
    p->render_flags |= 0x4000605;
    p->sprite = 0x1c8;
}

/* ENG1 0x41f00..0x440d0: native-bracketed continuation of the actor unit.
 * The two accepted entries within the cluster remain above unchanged. */
typedef struct ActorList { long count; Actor *items[1]; } ActorList;
typedef struct BurstEntry { short x, y, z, kind, program, direction; } BurstEntry;
typedef struct FragmentEntry { short x, y, z; unsigned short kind; } FragmentEntry;
typedef struct HighKindInfo { unsigned char *owner; long count; } HighKindInfo;
extern int damped_contact_motion(Actor *);
extern void remove_actor_variant(Actor *);
extern int eng1_native_code_target_002aa30(Actor *, int, int);
extern void eng1_native_code_target_002a290(Actor *);
extern void advance_frame(Actor *);
extern void select_frame(Actor *, int);
extern Actor *attach_actor(Actor *, int);
extern int eng1_interpret_signed_pairs(Actor *);
extern int eng1_native_code_target_002b130(Actor *);
extern int eng1_native_code_target_002a9e0(Actor *, int);
extern int remove_actor(Actor *);
extern void reset_frame_state(Actor *);
extern void eng1_interpret_word_stream(Actor *);
extern void LinkedSoundFx_6da0(unsigned short, Actor *);
extern void eng1_native_code_target_002ae50(Actor *);
extern void release_actor(Actor *);
extern void eng1_tile_draw_2ff0(long *);
extern void eng1_color_rgb_forward_5a30(int, int, int);
extern Actor *spawn_middle_277e0(long, long, long, unsigned short);
extern int eng1_29e20(Actor *);
extern void mark_actor_resource_for_release(Actor *);
extern int contact_motion(Actor *);
extern int eng1_26de0(Actor *);
extern void eng1_1aba0(void);
extern void eng1_30290(int, int);
extern void remove_counted_value(ActorList *, Actor *);
extern unsigned short grid_scan_20b50(int, int, int, int);
extern unsigned short eng1_20d30(int, int, int, int);
extern void eng1_27410(Actor *);
extern int eng1_2a710(void *, Actor *);
extern int eng1_native_code_target_00266a0(Actor *, int, int);
extern int eng1_19b40(int);
extern void eng1_0ef10(Actor *);
extern unsigned short g_eng1_classifier_rows_65678[][6];
extern short g_eng1_word_70000;
extern long g_camera_x_70ca0, g_camera_y_70ca4;
extern BurstEntry g_eng1_burst_65b98[];
extern void *g_eng1_programs_67070[];
extern Actor *g_collision_context;
extern unsigned short g_eng1_setup_word_72012, g_native_word_72168;
extern unsigned char g_cold_start_state_bytes_70820[0x1c];
typedef struct HighEngineInterfacePrefix { unsigned long signature_000; unsigned char *context_004; } HighEngineInterfacePrefix;
extern HighEngineInterfacePrefix g_engine_interface;
extern Actor *g_frame_owner_72860;
extern unsigned short *g_configuration_dword_713bc;
extern HighKindInfo g_kind_info[];
extern unsigned long eng1_sim_state_72110;
extern unsigned short g_eng1_word_70ef0;
#include "actor_control_state.h"
#include "actor_pipeline_storage.h"
extern FragmentEntry g_eng1_fragments_68970[];

void eng1_42db0(Actor *);
void eng1_42f90(Actor *);
void eng1_43160(Actor *);
void eng1_43b50(Actor *, long);
void eng1_43d80(Actor *);
void eng1_43e20(Actor *);
void eng1_43ff0(Actor *);
void eng1_440d0(Actor *, unsigned short);

void eng1_41f00(Actor *p)
{
    p->unknown_74 += 4;
    if (damped_contact_motion(p) & 2) remove_actor_variant(p);
    if (eng1_native_code_target_002aa30(p, 0x80, 0x80)) remove_actor_variant(p);
}
void eng1_41f50(Actor *p)
{
    eng1_native_code_target_002a290(p);
    advance_frame(p);
    attach_actor(p, 0x2028);
}
void eng1_41f80(Actor *p)
{
    if (p->status & 1) eng1_native_code_target_002a290(p);
    else eng1_interpret_signed_pairs(p);
}
unsigned char eng1_native_code_target_0041fd0(unsigned long value)
{
    int high = (short)((long)value >> 16);
    unsigned int row = 0;
    unsigned short *scan;
    unsigned long first = *(unsigned long *)g_eng1_classifier_rows_65678[0];
    unsigned short threshold;
    if ((unsigned short)first != 0xffff) {
        scan = g_eng1_classifier_rows_65678[0];
        do {
            threshold = *scan;
            if (high > 0 && high < (int)threshold) return (unsigned char)row;
            ++row; scan += 6;
        } while (threshold != 0xffff);
    }
    return 0xff;
}

void eng1_42020(Actor *p)
{
    p->render_flags |= 0x10004000;
    p->speed = 0x300;
    p->sprite = 0x37;
    p->scale = 0x140;
    p->frame_tick = 0;
    g_eng1_word_70000 = 0;
    p->unknown_a8 = 0;
    p->unknown_ac = 0x80000;
    p->unknown_b0 = 0x1000;
    eng1_native_code_target_002b130(p);
    attach_actor(p, 0x202c);
}
void eng1_42090(Actor *p)
{
    Actor *q;
    ActorList *list;
    int high, i;
    p->unknown_a8 += p->unknown_b0 - p->unknown_b4;
    if (p->unknown_a8 < 0) p->unknown_a8 = 0;
    if (p->unknown_a8 > p->unknown_ac) p->unknown_a8 = p->unknown_ac;
    high = (short)(p->unknown_a8 >> 16);
    p->step += high;
    q = p->attachment;
    if (q) {
        switch ((unsigned short)q->state) {
        case 0:
            if (p->unknown_a8 == p->unknown_ac && random_limit(20) == 1) {
                p->unknown_a8 = (dispatch_engine_2c470() & 0x3ff) << 8;
                q->state = 2;
                q->step = (short)((p->unknown_ac - p->unknown_a8) >> 16) * 32;
            }
            break;
        case 2:
            if (q->unknown_74 < q->step) q->unknown_74 += 6;
            else { q->step = -q->step; q->state = 1; }
            break;
        case 1:
            if (q->unknown_74 > q->step) q->unknown_74 -= 8;
            else q->state = 3;
            break;
        case 3:
            if (q->unknown_74 < 0) q->unknown_74 += 6;
            else { q->unknown_74 = 0; q->state = 0; }
            break;
        }
    }
    list = p->list;
    if (list) {
        for (i = 0; i < list->count;) {
            q = ((Actor **)list)[++i];
            if (q->kind == 0x4031) q->unknown_98 = high;
        }
    }
    if (eng1_native_code_target_002a9e0(p, 0x200)) remove_actor(p);
}
void eng1_42220(Actor *p)
{
    p->z -= 0x100000;
    p->flags = 0x10000;
    p->render_flags |= 0x10;
    p->unknown_4a = random_limit(0x80) + 0x60;
    p->state = 0;
    reset_frame_state(p);
    p->timer = random_limit(40) + 20;
}
void eng1_42280(Actor *p)
{
    eng1_interpret_word_stream(p);
    if (p->timer-- < 0) {
        LinkedSoundFx_6da0((random_limit(2) + 0xc3) | 0x4000, p);
        p->timer = random_limit(40) + 20;
    }
}
void eng1_422f0(Actor *p)
{
    eng1_native_code_target_002ae50(p);
    if (eng1_native_code_target_002a9e0(p, 0x140)) release_actor(p);
}
void eng1_42320(void)
{
    long v[3];
    v[0] = g_camera_x_70ca0 >> 2;
    v[1] = (g_camera_y_70ca4 - 0x250) / 3;
    eng1_tile_draw_2ff0(v);
    if (v[1] < 32) eng1_color_rgb_forward_5a30(0x3f, 0x75, 0xbc);
    else eng1_color_rgb_forward_5a30(0x7f, 0x8b, 0x47);
}
void eng1_42390(Actor *p)
{
    long x = p->x, y = p->y + 0x400000, z = p->z;
    BurstEntry *e = g_eng1_burst_65b98;
    Actor *q;
    long v;
    int kind = e->kind;
    while (kind) {
        q = spawn_middle_277e0(x + ((long)e->x << 16), y + ((long)e->y << 16), z + ((long)e->z << 16), kind);
        if (q) {
            q->program = g_eng1_programs_67070[e->program];
            q->frame = random_limit(4) + 1;
            q->unknown_94 = (unsigned char)e->direction;
            q->unknown_94 = -(unsigned char)random_limit(2);
            q->unknown_96 = 0xff;
            q->unknown_95 = 0xff;
            v = ((dispatch_engine_2c470() & 0x7ff) + 0x400) << 8;
            q->unknown_c0 = 0x2000;
            q->unknown_bc = v; q->unknown_b8 = v;
            q->unknown_c4 = random_limit(0x2000) + 0xa000;
            v = ((dispatch_engine_2c470() & 0x3ff) + 0x600) << 8;
            q->unknown_d0 = 0; q->unknown_cc = v; q->unknown_c8 = v >> 1;
            q->unknown_d4 = random_limit(0x6000) + 0xf000;
            v = ((dispatch_engine_2c470() & 0x3ff) + 0x600) << 8;
            q->unknown_b0 = 0; q->unknown_ac = v; q->unknown_a8 = v;
            q->unknown_b4 = random_limit(0x6000) + 0xf000;
            q->timer = random_limit(30) + 30;
        }
        ++e; kind=e->kind;
    }
    remove_actor_variant(p);
}
void eng1_42540(Actor *p)
{
    if (!p->state) {
        eng1_interpret_word_stream(p);
        if (p->unknown_95 == 0xff) {
            p->unknown_b8 -= p->unknown_c4;
            if (p->unknown_b8 < 0) p->unknown_b8 = 0;
        } else {
            p->unknown_b8 += p->unknown_c0;
            if (p->unknown_b8 > p->unknown_bc) p->unknown_b8 = p->unknown_bc;
        }
        p->unknown_a8 += p->unknown_b0 - p->unknown_b4;
        if (p->unknown_a8 < 0) p->unknown_a8 = 0;
        if (p->unknown_a8 > p->unknown_ac) p->unknown_a8 = p->unknown_ac;
        p->unknown_c8 += p->unknown_d0 - p->unknown_d4;
        if (p->unknown_c8 < 0) p->unknown_c8 = 0;
        if (p->unknown_c8 > p->unknown_cc) p->unknown_c8 = p->unknown_cc;
        if (p->unknown_94) p->x -= p->unknown_a8; else p->x += p->unknown_a8;
        if (p->unknown_95) p->y -= p->unknown_b8; else p->y += p->unknown_b8;
        if (p->unknown_96) p->z -= p->unknown_c8; else p->z += p->unknown_c8;
        if (!p->unknown_b8) { p->unknown_95 = 0; p->unknown_bc >>= 1; }
        if (!p->unknown_95 && p->y > 0x3000000 && eng1_29e20(p)) {
            ++p->state;
            p->unknown_b8 = 0; p->unknown_c4 = 0; p->unknown_c0 = 0;
            p->unknown_95 = 0xff; p->unknown_b0 = 0; p->unknown_b4 = 0;
        }
    } else {
        if (!p->timer) { remove_actor_variant(p); return; }
        --p->timer;
    }
    if (eng1_native_code_target_002aa30(p, 0x80, 0x80)) remove_actor_variant(p);
}
void eng1_42710(Actor *p)
{
    ActorList *list;
    int i;
    p->render_flags = 0;
    eng1_native_code_target_002a290(p);
    if (!(p->status & 1) && eng1_native_code_target_002b130(p)) {
        list = p->list;
        for (i = 0; i < list->count;) ((Actor **)list)[++i]->attachment = p;
    }
    p->hits = 35;
}
void eng1_42770(Actor *p)
{
    ActorList *list;
    Actor *q;
    int i;
    if (p->status & 1) { eng1_native_code_target_002a290(p); return; }
    list = p->list;
    if (list) {
        switch ((unsigned short)p->state) {
        case 2:
            mark_actor_resource_for_release(p);
            spawn_at_group_300c0(p->x + 0x100000, p->y - 0x700000, p->z, 0x2027, 0);
            spawn_at_group_300c0(p->x - 0x180000, p->y - 0xb40000, p->z - 0x100000, 0x2027, 0);
            spawn_at_group_300c0(p->x, p->y - 0x200000, p->z - 0x200000, 0x2027, 0);
            q = spawn_at_group_300c0(p->x + ((long)(random_limit(32) - 16) << 16), p->y - (long)(random_limit(32) << 16) - 0x400000, p->z - 0x200000, 0x2027, 0);
            if (q) q->timer = random_limit(8) + 4;
            q = spawn_at_group_300c0(p->x + ((long)(random_limit(32) - 16) << 16), p->y - (long)(random_limit(32) << 16) - 0x600000, p->z - 0x200000, 0x2027, 0);
            if (q) q->timer = random_limit(12) + 6;
            for (i = 0; i < list->count;) {
                q = ((Actor **)list)[++i]; q->state = 1;
                if (g_collision_context->x < q->x) q->unknown_94 = 0; else q->unknown_94 = 0xff;
                q->flags &= ~0x800UL; q->saved_y = q->y;
            }
            break;
        case 1:
            for (i = 0; i < list->count;) ((Actor **)list)[++i]->y -= 0x20000;
            break;
        case 0:
            for (i = 0; i < list->count;) {
                q = ((Actor **)list)[++i]; if (q->y < q->saved_y) q->y += 0x10000;
            }
            break;
        }
        p->state = 0;
        if (!list->count) remove_actor(p);
    }
}
void eng1_429c0(Actor *p)
{
    long v;
    p->render_flags |= 0x10000000;
    p->unknown_95 = 0xff; p->unknown_96 = 0xff; p->flags |= 0x120800; p->unknown_d8 = 27;
    switch ((unsigned short)p->state) {
    case 2:
        v = ((dispatch_engine_2c470() & 0x1ff) + 0x400) << 8;
        p->unknown_bc = v; p->unknown_b8 = v;
        p->unknown_c4 = random_limit(0x2000) + 0x5000;
        p->unknown_cc = 0x40000; p->unknown_c8 = 0x10000; p->unknown_d0 = 0x2800;
        p->unknown_ac = 0x70000; p->unknown_a8 = 0x1c000; p->unknown_b0 = 0x3000; p->unknown_b4 = 0;
        p->unknown_e0 = 4; p->unknown_dc = 16; break;
    case 1:
        v = ((dispatch_engine_2c470() & 0x1ff) + 0x400) << 8;
        p->unknown_bc = v; p->unknown_b8 = v;
        p->unknown_c4 = random_limit(0x2000) + 0x5000;
        p->unknown_cc = 0x50000; p->unknown_c8 = 0x14000; p->unknown_d0 = 0x2800;
        p->unknown_ac = 0x70000; p->unknown_a8 = 0x1c000; p->unknown_b0 = 0x3000; p->unknown_b4 = 0;
        p->unknown_e0 = 8; p->unknown_dc = 12; break;
    case 0:
        v = ((dispatch_engine_2c470() & 0x1ff) + 0x400) << 8;
        p->unknown_bc = v; p->unknown_b8 = v;
        p->unknown_c4 = random_limit(0x2000) + 0x5000;
        p->unknown_cc = 0x60000; p->unknown_c8 = 0x18000; p->unknown_d0 = 0x2800;
        p->unknown_ac = 0x70000; p->unknown_a8 = 0x1c000; p->unknown_b0 = 0x3000; p->unknown_b4 = 0;
        p->unknown_e0 = 16; p->unknown_dc = 0; break;
    }
    p->state = 0;
}
void eng1_42bd0(Actor *p)
{
    Actor *q = p->attachment;
    int dead = 0, hit;
    unsigned int mask;
    if (!q) { remove_actor_variant(p); return; }
    if (p->state) {
        p->speed += (short)p->unknown_e0; p->step += (short)p->unknown_dc;
        if (contact_motion(p) & 1) { LinkedSoundFx_6da0(0xd071,p); eng1_43160(p); dead = 1; }
    } else {
        hit = eng1_26de0(p);
        if (hit) {
            if (hit == 0x80) {
                mask = (signed char)(g_eng1_setup_word_72012 & 0x5f); { unsigned int used = g_native_word_72168; mask &= ~used; }
                if (mask) {
                    eng1_1aba0(); g_native_word_72168 |= mask;
                    if ((g_eng1_setup_word_72012 & 2) && *(unsigned short *)(void *)(g_cold_start_state_bytes_70820+14) >= 200) {
                        eng1_42f90(q); LinkedSoundFx_6da0(0xd071,p); g_engine_interface.context_004[2] = 2; q->state = 2;
                    } else {
                        eng1_42db0(q); --q->hits;
                        if (q->hits <= 0) { LinkedSoundFx_6da0(0xd071,p); q->state = 2; g_engine_interface.context_004[2] = 2; }
                        else { LinkedSoundFx_6da0(0xd04c,p); q->state = 1; }
                    }
                }
            } else {
                eng1_30290(hit,0);
                if (p->y == p->saved_y) {
                    eng1_42db0(q); q->hits -= 2;
                    if (q->hits <= 0) { LinkedSoundFx_6da0(0xd071,p); q->state = 2; g_engine_interface.context_004[2] = 2; }
                    else { LinkedSoundFx_6da0(0xd04c,p); q->state = 1; }
                }
            }
        }
    }
    if (p->z < -0x640000) dead = 1;
    if (p->y > 0x3400000) dead = 1;
    if (eng1_native_code_target_002a9e0(p,0x100)) dead = 1;
    if (dead) { if (q->list) remove_counted_value(q->list,p); remove_actor_variant(p); }
}

void eng1_42db0(Actor *p)
{
    int count, remaining;
    long x,y,z,v;
    Actor *q;
    LinkedSoundFx_6da0(0xd070,p);
    count = random_limit(6) + 6;
    if (count > 0) {
    remaining = count;
    do {
        x = (random_limit(32)-16) << 16; y = (random_limit(96)+48) << 16; z = (random_limit(32)-16) << 16;
        q = spawn_at_group_300c0(p->x+x, p->y-y, p->z+z, 0x4041, 0);
        if (q) {
            q->render_flags |= 0x10000000;
            if (random_limit(2)) q->sprite = 0x58; else q->sprite = 0x59;
            q->scale = random_limit((32-count)*4) + (64-(unsigned int)count*4);
            q->unknown_94 = -(unsigned char)random_limit(2);
            q->unknown_96 = -(unsigned char)random_limit(2);
            q->unknown_95 = 0xff;
            v = ((dispatch_engine_2c470() & 0x1ff) + 0x800) << 8;
            q->unknown_c0 = 0; q->unknown_bc = v; q->unknown_b8 = v;
            q->unknown_c4 = random_limit(0x2000) + 0x4000;
            v = ((dispatch_engine_2c470() & 0xff) + 0x200) << 8;
            q->unknown_cc = v; q->unknown_c8 = v;
            q->unknown_d0 = random_limit(0x1000) + 0x2000; q->unknown_d4 = 0;
            v = ((dispatch_engine_2c470() & 0x1ff) + 0x100) << 8;
            q->unknown_ac = v; q->unknown_a8 = v;
            q->unknown_b0 = random_limit(0x1000) + 0x1000; q->unknown_b4 = 0;
            q->state = 0;
        }
    } while (--remaining);
    }
}

void eng1_42f90(Actor *p)
{
    int count, remaining;
    long x,y,z,v;
    Actor *q;
    
    count = random_limit(12) + 12;
    if (count > 0) {
    remaining = count;
    do {
        x = (random_limit(32)-16) << 16; y = random_limit(128) << 16; z = (random_limit(32)-16) << 16;
        q = spawn_at_group_300c0(p->x+x, p->y-y, p->z+z, 0x4041, 0);
        if (q) {
            q->render_flags |= 0x10000000;
            if (random_limit(2)) q->sprite = 0x58; else q->sprite = 0x59;
            q->scale = random_limit(160-count*3) + (64-(unsigned int)count*2);
            q->unknown_94 = -(unsigned char)random_limit(2);
            q->unknown_96 = -(unsigned char)random_limit(2);
            q->unknown_95 = 0xff;
            v = ((dispatch_engine_2c470() & 0x1ff) + 0x800) << 8;
            q->unknown_c0 = 0; q->unknown_bc = v; q->unknown_b8 = v;
            q->unknown_c4 = random_limit(0x2000) + 0x4000;
            v = ((dispatch_engine_2c470() & 0xff) + 0x200) << 8;
            q->unknown_cc = v; q->unknown_c8 = v;
            q->unknown_d0 = random_limit(0x1000) + 0x2000; q->unknown_d4 = 0;
            v = ((dispatch_engine_2c470() & 0x1ff) + 0x100) << 8;
            q->unknown_ac = v; q->unknown_a8 = v;
            q->unknown_b0 = random_limit(0x1000) + 0x1000; q->unknown_b4 = 0;
            q->state = 0;
        }
    } while (--remaining);
    }
}

void eng1_43160(Actor *p)
{
    int count, remaining;
    long x,y,z,v;
    Actor *q;
    spawn_at_group_300c0(p->x,p->y-0x200000,p->z,0x2027,0);
    count = random_limit(11) + 8;
    if (count > 0) {
    remaining = count;
    do {
        x = (random_limit(64)-32) << 16; y = random_limit(48) << 16; z = 0;
        q = spawn_at_group_300c0(p->x+x, p->y-y, p->z+z, 0x4041, 0);
        if (q) {
            q->render_flags |= 0x10000000;
            if (random_limit(2)) q->sprite = 0x58; else q->sprite = 0x59;
            q->scale = random_limit(160-count*3) + (72-(unsigned int)count*3);
            q->unknown_94 = -(unsigned char)random_limit(2);
            q->unknown_96 = -(unsigned char)random_limit(2);
            q->unknown_95 = 0xff;
            v = ((dispatch_engine_2c470() & 0x1ff) + 0x800) << 8;
            q->unknown_c0 = 0; q->unknown_bc = v; q->unknown_b8 = v;
            q->unknown_c4 = random_limit(0x2000) + 0x4000;
            v = ((dispatch_engine_2c470() & 0xff) + 0x200) << 8;
            q->unknown_cc = v; q->unknown_c8 = v;
            q->unknown_d0 = random_limit(0x1000) + 0x2000; q->unknown_d4 = 0;
            v = ((dispatch_engine_2c470() & 0x1ff) + 0x100) << 8;
            q->unknown_ac = v; q->unknown_a8 = v;
            q->unknown_b0 = random_limit(0x1000) + 0x1000; q->unknown_b4 = 0;
            q->state = 0;
        }
    } while (--remaining);
    }
}

void eng1_43330(Actor *p)
{
    unsigned int result;
    unsigned short state;
    p->speed = 0x400 - ((p->z >> 13) & 0x3ff);
    p->step = 0x400 - ((p->x >> 13) & 0x3ff);
    result = contact_motion(p);
    if (!(result & 2)) {
        state = p->state;
        if (state < random_limit(2)+1 && (result & 1)) p->state = state+1;
        if (p->z < -0x400000 || p->y > 0x3400000 || eng1_native_code_target_002aa30(p,0x80,0x190)) remove_actor_variant(p);
    } else remove_actor_variant(p);
}
void eng1_433d0(Actor *p)
{
    p->category = ((p->z >> 16)+0x40) >> 7;
    p->flags = 0x202;
    grid_scan_20b50(p->x >> 16,p->y >> 16,p->z >> 16,p->profile);
    p->state = 0x21;
    select_frame(p,1);
    advance_frame(p);
    { int direction = (signed char)p->unknown_94 & 0x10;
    p->render_flags = (p->render_flags & ~0x10UL) | direction; }
    eng1_interpret_signed_pairs(p);
}
void eng1_43440(Actor *p)
{
    long dx,dy,absolute,next,probe;
    unsigned short tile;
    int hit;
    eng1_440d0(p,0x26);
    dx = p->x-g_frame_owner_72860->x; dy=p->y-g_frame_owner_72860->y;
    if (eng1_20d30(p->x >> 16,p->y >> 16,p->z >> 16,p->profile)==0x1a) dy=0xff0000;
    absolute=dx; if (absolute<0) absolute=-absolute;
    switch ((unsigned short)p->state) {
    case 38:
        if ((unsigned long)absolute < random_limit(0x300000)+0x480000) {
            p->state=0x21; select_frame(p,1); eng1_interpret_signed_pairs(p);
        } else { p->frame_tick=99; eng1_interpret_signed_pairs(p); }
        break;
    case 33:
        if (p->frame>5) {
            if (p->frame>9) p->unknown_a8=0x80000;
            else if (p->frame==6) { p->unknown_b8=-0x18000; p->unknown_a8=0x20000; }
            else if (p->frame==7) { p->unknown_b8=-0x30000; p->unknown_a8=0x2aaaa; }
            else if (p->frame>=8) { p->unknown_b8=-0x60000; p->unknown_a8=0x40000; }
            else p->unknown_a8=0x60000;
            p->y+=p->unknown_b8;
            next=p->x;
            if (!p->unknown_94) { next+=p->unknown_a8; probe=next+0x100000; }
            else { next-=p->unknown_a8; probe=next-0x100000; }
            tile=eng1_20d30(probe >> 16,p->y >> 16,p->z >> 16,p->profile);
            if (!(g_configuration_dword_713bc[tile]&7)) p->x=next;
        }
        if (eng1_interpret_signed_pairs(p)) { p->unknown_b8=0; p->state=2; p->unknown_a8=0x80000; select_frame(p,2); p->frame=2; }
        break;
    case 2: case 39:
        if (eng1_interpret_signed_pairs(p)) eng1_43b50(p,dx);
        else {
            if (dy > -0x400000) p->y-=0x60000;
            if (dy < -0x800000) p->y+=0x60000;
            eng1_43d80(p);
        }
        break;
    case 9:
        if (p->frame>13) {
            p->unknown_a8+=0x8000; if(p->unknown_a8>0x80000)p->unknown_a8=0x80000;
        } else {p->unknown_a8-=0x8000;if(p->unknown_a8<0)p->unknown_a8=0;}
        eng1_43d80(p);
        if(eng1_interpret_signed_pairs(p)) eng1_43b50(p,dx);
        else {if(dy > -0x400000)p->y-=0x60000;if(dy < -0x800000)p->y+=0x60000;}
        break;
    case 34:
        if (p->frame<10) {p->unknown_a8-=0x4000;if(p->unknown_a8<0x40000)p->unknown_a8=0x40000;}
        else if(p->frame<16){p->unknown_a8+=0x10000;if(p->unknown_a8>0x100000)p->unknown_a8=0x100000;p->unknown_b8+=0x10000;if(p->unknown_b8>0xc0000)p->unknown_b8=0x60000;}
        else if(p->frame<21){p->unknown_a8-=0x8000;if(p->unknown_a8<0x40000)p->unknown_a8=0x40000;p->unknown_b8-=0x10000;if(p->unknown_b8 < -0x60000)p->unknown_b8=-0x60000;}
        else {p->unknown_a8+=0x8000;if(p->unknown_a8>0x80000)p->unknown_a8=0x80000;}
        if(p->anim_id!=5){p->state=2;p->unknown_a8=0x80000;p->unknown_b8=0;select_frame(p,2);}
        eng1_43d80(p);eng1_interpret_signed_pairs(p);break;
    case 35:
        eng1_43d80(p);eng1_440d0(p,0x26);
        if(eng1_interpret_signed_pairs(p)){eng1_27410(p);return;}
        break;
    case 43:
        if(p->frame<8){p->unknown_a8-=0x8000;if(p->unknown_a8<0)p->unknown_a8=0;}
        else if(p->frame<16){p->z-=0x80000;if(p->frame==8)p->unknown_a8=0;}
        else{p->unknown_a8+=0x8000;if(p->unknown_a8>0x80000)p->unknown_a8=0x80000;}
        eng1_43d80(p);eng1_440d0(p,0x26);
        if(eng1_interpret_signed_pairs(p)){p->z=(p->z+0x400000)&0xff80ffff;p->category=((p->z >> 16)+0x40)>>7;eng1_43b50(p,dx);}break;
    case 44:
        if(p->frame<8){p->unknown_a8-=0x8000;if(p->unknown_a8<0)p->unknown_a8=0;}
        else if(p->frame<16){p->z+=0x80000;if(p->frame==8)p->unknown_a8=0;}
        else{p->unknown_a8+=0x8000;if(p->unknown_a8>0x80000)p->unknown_a8=0x80000;}
        eng1_43d80(p);eng1_440d0(p,0x26);
        if(eng1_interpret_signed_pairs(p)){p->z=(p->z+0x400000)&0xff80ffff;p->category=((p->z >> 16)+0x40)>>7;eng1_43b50(p,dx);}break;
    case 8:
        if(p->frame>13){p->unknown_a8+=0x8000;if(p->unknown_a8>0x80000)p->unknown_a8=0x80000;}
        eng1_43d80(p);eng1_440d0(p,0x26);eng1_interpret_signed_pairs(p);if(p->frame_tick>99)p->frame_tick=199;break;
    case 45:
        p->unknown_b8+=p->unknown_c0;eng1_43d80(p);
        if(eng1_native_code_target_002aa30(p,100,60)){remove_actor(p);return;}
        eng1_interpret_signed_pairs(p);break;
    default:p->state=1;select_frame(p,0);break;
    }
    if(p->state!=35){
        if(eng1_2a710((unsigned char *)g_collision_context+0x88,p)){
            eng1_native_code_target_00266a0(p,17,0);g_eng1_word_70ef0+=2;
        }else{
            hit=eng1_26de0(p);
            if(hit){
                if(hit==0x80){eng1_1aba0();if(eng1_19b40(~g_native_word_72168&0x5f)){eng1_43e20(p);p->flags&=~0x200UL;}}
                else{eng1_30290(hit,0);p->flags&=~0x200UL;eng1_43e20(p);}
            }
        }
        if(p->state!=38 && eng1_29e20(p))eng1_43e20(p);
    }
    eng1_0ef10(p);
    if(eng1_native_code_target_002aa30(p,500,320))remove_actor(p);
}
extern unsigned long grid_query_20d30(long,long,long,unsigned long);
extern Actor *g_normal_actor_pool;
void eng1_43b50(Actor *p,long dx)
{
    int random = random_limit(0x8000), category, x,z;
    long offset, absolute;
    unsigned short tile;
    category=(int)p->category-(int)g_normal_actor_pool->category;
    offset=p->record_offset;
    absolute=p->x-*(long *)(g_kind_info[p->profile].owner+offset);
    if(absolute<0)absolute=-absolute;
    if(absolute>0x2800000){p->state=45;p->unknown_b8=0;p->unknown_c0=-0x8000;select_frame(p,2);}
    else{
        if(category && random>0x4000){
            if(category>0){
                x=p->x>>16;z=(p->z>>16)-1;
                tile=grid_query_20d30(x,0,z,0);
                if(!(g_configuration_dword_713bc[tile]&2)){
                    if(!p->unknown_94)++x;else x+=0xffff;
                    tile=grid_query_20d30(x,0,z,0);
                    if(!(g_configuration_dword_713bc[tile]&2)){p->state=43;select_frame(p,9);goto advance;}
                }
            }else{
                x=p->x>>16;z=(p->z>>16)+1;
                tile=grid_query_20d30(x,0,z,0);
                if(!(g_configuration_dword_713bc[tile]&2)){
                    if(!p->unknown_94)++x;else x+=0xffff;
                    tile=grid_query_20d30(x,0,z,0);
                    if(!(g_configuration_dword_713bc[tile]&2)){p->state=44;select_frame(p,10);goto advance;}
                }
            }
        }
        if(!p->unknown_94){
            if(dx>0x3aaaaa){p->state=9;select_frame(p,3);}
            else if(!(random&6)){p->state=39;select_frame(p,7);}
            else if(dx < -0x3aaaaa && !category && dx > -0xb00000){if(eng1_sim_state_72110){p->state=39;select_frame(p,7);}else{p->state=34;select_frame(p,5);}}
            else{if(!(random&8)){p->state=39;select_frame(p,7);}else{p->state=2;select_frame(p,2);}}
        }else{
            if(dx < -0x3aaaaa){p->state=9;select_frame(p,3);}
            else if(!(random&6)){p->state=39;select_frame(p,7);}
            else if(dx > 0x3aaaaa && !category && dx < 0xb00000){if(eng1_sim_state_72110){p->state=39;select_frame(p,7);}else{p->state=34;select_frame(p,5);}}
            else{if(!(random&8)){p->state=39;select_frame(p,7);}else{p->state=2;select_frame(p,2);}}
        }
    }
advance:
    eng1_interpret_signed_pairs(p);
}
void eng1_43d80(Actor *p)
{
    long next,probe;
    unsigned short tile;
    p->y+=p->unknown_b8;
    if(!p->unknown_94){next=p->x+p->unknown_a8;probe=next+0x100000;}
    else{next=p->x-p->unknown_a8;probe=next-0x100000;}
    tile=eng1_20d30(probe>>16,p->y>>16,p->z>>16,p->profile);
    if(!(g_configuration_dword_713bc[tile]&7))p->x=next;
    else if(p->state!=9 && p->state!=35){p->state=9;select_frame(p,3);}
}
void eng1_43e20(Actor *p)
{
    p->state=35;select_frame(p,6);eng1_interpret_signed_pairs(p);
    if(p->unknown_a8>0x30000)p->unknown_a8=0x30000;
    if(p->unknown_a8 < -0x30000)p->unknown_a8=-0x30000;
    spawn_at_group_300c0(p->x,p->y,p->z,0x2027,0);
    eng1_43ff0(p);++g_eng1_actor_control_state_70ed8.words[0x36 / 2];g_actor_pipeline_storage_71440.word_cells[0xc / 2]=0;
}
void eng1_43ea0(Actor *p)
{
    long next,probe;
    unsigned short tile;
    p->unknown_b8+=0x8000;p->y+=p->unknown_b8;
    next=p->x;
    if(!p->unknown_94){next+=p->unknown_a8;probe=next+0x100000;}
    else{next-=p->unknown_a8;probe=next-0x100000;}
    tile=eng1_20d30(probe>>16,p->y>>16,p->z>>16,p->profile);
    if(!(g_configuration_dword_713bc[tile]&7))p->x=next;
    eng1_interpret_word_stream(p);
    if(eng1_29e20(p)){
        if(g_frame_owner_72860->y-p->y>0x400000)remove_actor_variant(p);
        else{
            p->unknown_b8/=-3;
            if(p->unknown_b8<0x2800 && p->unknown_b8 > -0x2800){p->unknown_a8=0;p->frame_tick=9;}
        }
    }
    if(!p->unknown_a8 && p->unknown_b8<0x8000){
        if(random_limit(0x800)<0x100)*(unsigned short *)&p->scale >>= 1;
        if((unsigned short)p->scale<0x20)remove_actor_variant(p);
    }
    if(eng1_native_code_target_002aa30(p,100,200))remove_actor_variant(p);
}
void eng1_43ff0(Actor *p)
{
    long x=p->x,y=p->y,z=p->z;
    short *e;
    Actor *q;
    e=&g_eng1_fragments_68970[0].z;
    do {
        q=spawn_middle_277e0(x+((long)e[-2]<<16),y+((long)e[-1]<<16),z+((long)e[0]<<16),(unsigned short)e[1]);
        if(q){
            reset_frame_state(q);eng1_interpret_word_stream(q);q->unknown_94=0;
            q->unknown_a8=(random_limit(0xa00)-0x500)<<8;
            q->unknown_b8=(-random_limit(0x600)-0x200)<<8;
            q->unknown_c8=(random_limit(0x1800)-0xc00)<<8;
        }
    e+=4;
    } while ((long)e < (long)&g_eng1_fragments_68970[24].z);
}
void eng1_440d0(Actor *p,unsigned short margin)
{
    int delta;
    delta=(unsigned short)grid_scan_20b50(p->x>>16,p->y>>16,p->z>>16,p->profile)-margin;
    if(delta<0)p->y+=delta<<16;
}

#include "actor_control_state.h"
/* ENG1 0x410b0..0x41da0, excluding accepted 0x41300.
 * Fragment for actor_handlers_high.c, bracketed by its 0x41300 and 0x41da0.
 * The local field view preserves the existing unit's Actor declaration. */
typedef struct HighActor HighActor;
typedef struct HighRecord HighRecord;
struct HighRecord {
    long x, y, z;
    unsigned short flags;
    unsigned char kind;
    unsigned char unknown_0f;
};
struct HighActor {
    long x, y, z;
    short screen_x;
    unsigned char unknown_0e[0x22 - 0x0e];
    unsigned char category, mode, flags_24, flags_25;
    short elevation;
    unsigned short unknown_28, state, unknown_2c, kind;
    short ticks, phase;
    unsigned short sprite, unknown_36, profile;
    short scale, scale_z;
    unsigned char unknown_3e[0x4a - 0x3e];
    unsigned short frame;
    long timer;
    unsigned long flags, render_flags;
    unsigned char unknown_58[4];
    HighActor *attachment;
    unsigned char unknown_60[0x70 - 0x60];
    short speed, step;
    unsigned char unknown_74[0x88 - 0x74];
    short bounds[6];
    unsigned char direction_x, direction_y, direction_z, drop;
    unsigned char unknown_98[4];
    long saved_x, saved_y, saved_z;
    long velocity_x, target_x, rate_x, extra_x;
    long velocity_y, target_y, rate_y, extra_y;
    long velocity_z, target_z, rate_z, extra_z;
    long spin[3];
    HighActor *child;
    unsigned char unknown_e8[4];
    unsigned char cooldown, unknown_ed;
    short hits;
    unsigned char unknown_f0[0x120 - 0xf0];
    long *members;
};
extern HighKindInfo g_kind_info[];
extern unsigned long eng1_sim_state_72110;
extern unsigned short g_eng1_setup_word_718e0;

extern unsigned short g_native_word_72168;
extern void advance_object_motion(void *);
extern int eng1_native_code_target_002a710(void *, void *);
extern int remove_actor_preserving_resource(void *);
extern HighRecord *eng1_26640(void *);
extern void remove_counted_value(long *, long);
extern void unlink_0e4(void *);
extern void eng1_1ab50(void);

void eng1_410b0(HighActor *p)
{
    p->flags |= 0x800;
    advance_frame(p);
    attach_actor(p, 0x202a);
    p->hits = 30;
}

void eng1_410e0(HighActor *p)
{
    eng1_interpret_signed_pairs(p);
    if (p->hits-- == 0) {
        p->x = p->saved_x;
        p->z = p->saved_z;
        p->y = p->saved_y;
        eng1_native_code_target_002a290(p);
        p->velocity_y = p->target_y = 0xf0000;
        p->direction_y = 0xff;
        p->extra_y = 0x9000;
        p->rate_y = 0;
        p->target_z = 0x30000;
        p->velocity_z = 0;
        p->direction_z = 0xff;
        p->extra_z = 0;
        p->rate_z = 0x1000;
        select_frame(p, 3);
        if (p->x > ((HighActor *)(void *)g_collision_context)->x) p->direction_x = 0xff;
        else p->direction_x = 0;
        p->render_flags = (p->render_flags & ~0x10UL) | (p->direction_x & 0x10);
    }
    if (p->state != 0) {
        advance_object_motion(p);
        if (p->phase == 2 && p->velocity_y < 0x4000) p->ticks = 0;
        if (p->velocity_y == 0) {
            p->rate_y = p->extra_y;
            p->extra_y = 0;
            p->direction_y = 0;
            p->extra_z = p->rate_z;
            p->rate_z = 0;
        }
        if (p->state == 1 && p->y < (g_eng1_setup_word_718e0 << 16)) {
            spawn_at_group_300c0(p->x, p->y, p->z, 0x202e, p->profile);
            LinkedSoundFx_6da0(0x405b, p);
            p->state = 2;
        }
        if (p->state < 9 && p->direction_y == 0 && p->y > 0x3600000) {
            spawn_at_group_300c0(p->x, p->y, p->z, 0x202d, p->profile);
            LinkedSoundFx_6da0(0x405c, p);
            p->state = 9;
            p->hits = 60;
        }
    }
    if (eng1_native_code_target_002a710(((HighActor *)(void *)g_collision_context)->bounds, p)) {
        eng1_native_code_target_00266a0(p, 0x13, 0);
        ++g_eng1_actor_control_state_70ed8.words[12];
    }
    if (eng1_native_code_target_002a9e0(p, 0x200)) remove_actor(p);
}

void eng1_41320(HighActor *p)
{
    HighActor *q = p->attachment;
    long height;
    p->y -= 0x20000;
    p->z = q->z;
    p->render_flags &= ~0x10UL;
    p->render_flags |= (q->render_flags & 0x10) | 0x80000000UL;
    p->category = q->category;
    p->flags_24 = q->flags_24;
    p->frame = q->frame;
    p->sprite = q->sprite;
    q->render_flags &= ~0x70000UL;
    p->render_flags &= ~0x70001UL;
    height = 0x4000000 - q->y;
    if (height > 0x1400000) height = 0x1400000;
    if (height < 0) height = 0;
    p->y += height;
    p->scale = p->scale_z = q->scale - (height >> 17);
    p->speed = q->speed - 255;
    p->step = 64;
    p->speed = q->speed + 255;
    p->step = ((short *)g_actor_pipeline_storage_71440.word_cells)[0x268 / 2];
    p->scale_z += ((long *)g_actor_pipeline_storage_71440.dword_cells)[0x2d8 / 4] + 32;
    p->render_flags |= 1;
    p->z = q->z - 0x40000;
    height = (q->y >> 16) - g_eng1_setup_word_718e0;
    q->elevation = height;
    p->render_flags |= 0x50000;
    q->render_flags |= 0x30000;
    p->elevation = q->elevation - 1;
    p->y = (g_eng1_setup_word_718e0 + p->elevation) << 16;
}

void eng1_41460(HighActor *p)
{
    p->flags |= 0x800;
    advance_frame(p);
    attach_actor(p, 0x202a);
    p->hits = 5;
}

void eng1_41490(HighActor *p)
{
    unsigned long result, damage;
    eng1_interpret_signed_pairs(p);
    if (p->state == 0) {
        if (p->hits-- < 0) {
            if (((HighActor *)(void *)g_collision_context)->direction_x) {
                p->direction_x = 0;
                p->x = p->saved_x - 0xbc0000;
            } else {
                p->direction_x = 0xff;
                p->x = p->saved_x + 0xbc0000;
            }
            p->y = p->saved_y;
            eng1_native_code_target_002a290(p);
            p->target_x = 0xc0000;
            p->velocity_x = 0;
            p->extra_x = 0;
            p->rate_x = 0x6800;
            p->velocity_y = p->target_y = 0xc0000;
            p->direction_y = 0xff;
            p->extra_y = 0x6800;
            p->rate_y = 0;
            p->flags &= ~0x200UL;
            select_frame(p, 6);
        }
    }
    if (p->state != 0) {
        advance_object_motion(p);
        if (p->phase == 4 && p->velocity_x > 0x90000) p->ticks = 0;
        if (p->velocity_y == 0) {
            p->rate_y = p->extra_y;
            p->extra_y = 0;
            p->direction_y = 0;
            p->extra_x = p->rate_x;
            p->rate_x = 0;
        }
        if (p->state == 1 && p->y < (g_eng1_setup_word_718e0 << 16)) {
            spawn_at_group_300c0(p->x, p->y, p->z, 0x202e, p->profile);
            LinkedSoundFx_6da0(0x405b, p);
            p->state = 2;
        }
        if (p->state < 9 && p->direction_y == 0 && p->y > 0x3600000) {
            spawn_at_group_300c0(p->x, p->y, p->z, 0x202d, p->profile);
            LinkedSoundFx_6da0(0x405c, p);
            p->state = 9;
            p->hits = 60;
        }
        if (p->y > 0x3d00000) {
            if (p->state == 21) goto remove;
            select_frame(p, 0);
        }
    }
    if (p->state < 20 && p->cooldown == 0) {
        result = eng1_26de0(p);
        if (result) {
            if (result == 0x80) {
                damage = eng1_19b40(~g_native_word_72168 & 0x5f);
                if (damage) {
                    eng1_1ab50();
                    p->hits -= (short)damage;
                    p->state = 20;
                    if (p->direction_y != 0xff) goto hit;
                    goto reverse;
                }
            } else {
                eng1_30290(result, 0);
                p->hits -= 100;
                p->state = 20;
                if (p->direction_y == 0xff) {
reverse:
                    p->rate_y = p->extra_y;
                    p->extra_y = 0;
                    p->direction_y = 0;
                    p->extra_x = p->rate_x;
                    p->rate_x = 0;
                    p->direction_x = ~p->direction_x;
                }
hit:
                p->flags &= ~0x200UL;
                select_frame(p, 9);
            }
            p->cooldown = 30;
            if (p->hits <= 0) p->state = 21;
        } else if (eng1_native_code_target_002a710(((HighActor *)(void *)g_collision_context)->bounds, p)) {
            eng1_native_code_target_00266a0(p, 0x13, 0);
            g_eng1_actor_control_state_70ed8.words[12] += 3;
        }
    }
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10UL);
    if (eng1_native_code_target_002a9e0(p, 0x320)) {
remove:
        remove_actor(p);
    }
}

void eng1_417e0(HighActor *p)
{
    p->y = g_eng1_setup_word_718e0 << 16;
    p->z -= 0x100000;
    reset_frame_state(p);
}

void eng1_41810(HighActor *p)
{
    eng1_interpret_word_stream(p);
}

void eng1_41820(HighActor *p)
{
    HighRecord *record;
    p->flags |= 0x20000;
    record = eng1_26640(p);
    if (record) {
        if (p->x > record->x) {
            p->x = record->x;
            p->y = record->y;
            p->state = record->kind;
        }
        p->attachment = (HighActor *)((unsigned char *)record - g_kind_info[0].owner);
    }
    eng1_native_code_target_002a290(p);
    if (eng1_native_code_target_002b130(p)) {
        if (record) record->flags |= 0x8000;
        p->state = 0;
        p->hits = 30;
    } else remove_actor(p);
}

void eng1_418a0(HighActor *p)
{
    long *list;
    long i;
    int state;
    HighActor *q;
    HighRecord *record;
    if (p->flags_25 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        if (eng1_sim_state_72110) return;
        list = p->members;
        state = 0;
        if (p->state == 0) {
            q = ((HighActor *)(void *)g_collision_context)->child;
            if (q) {
                while (q->child) q = q->child;
                if (q->kind != 0x4035 && q->kind != 0x403e) p->hits = 30;
                else {
                    --p->hits;
                    if (p->hits < 0) {
                        p->state = 1;
                        state = 2;
                    }
                }
            } else p->hits = 30;
        } else state = 2;
        i = 0;
        while (i < list[0]) {
            q = (HighActor *)list[++i];
            if (q->state == 0 && state == 2) q->state = 1;
            state = q->state;
            if (state == 4) {
                remove_counted_value(list, (long)q);
                remove_actor_variant(q);
            }
        }
        if (list[0] < 1) {
            if (p->attachment) {
                record = (HighRecord *)p->attachment;
                record = (HighRecord *)((long)record + (long)g_kind_info[p->profile].owner);
                record->flags = (record->flags & 0x3fff) | 0x4000;
            }
            remove_actor_preserving_resource(p);
            return;
        }
    }
    eng1_native_code_target_002ae50(p);
    if (p->screen_x > 1008 || p->screen_x < -1008) {
        if (p->state == 0) {
            if (p->attachment) {
                record = (HighRecord *)p->attachment;
                record = (HighRecord *)((long)record + (long)g_kind_info[p->profile].owner);
                record->flags &= 0x7fff;
            }
            remove_actor(p);
        } else {
            if (p->attachment) {
                record = (HighRecord *)p->attachment;
                record = (HighRecord *)((long)record + (long)g_kind_info[p->profile].owner);
                record->flags = (record->flags & 0x3fff) | 0x4000;
            }
            remove_actor_preserving_resource(p);
        }
    }
}

void eng1_41a50(HighActor *p)
{
    unsigned long value;
    p->flags |= 0x40000;
    p->render_flags |= 0x10000080;
    p->drop = (unsigned char)p->state;
    p->state = 0;
    p->direction_y = 0xff;
    value = dispatch_engine_2c470() & 0x3ff;
    p->velocity_y = 0;
    p->target_y = (value + 0xe00) << 8;
    p->rate_y = 0;
    p->extra_y = random_limit(0x6000) + 0x8000;
    p->timer = 6;
}

void eng1_41ad0(HighActor *p)
{
    if (eng1_sim_state_72110) return;
    switch (p->state) {
    case 1:
        if (--p->timer < 0) {
            p->state = 2;
            if (p->drop) {
                --p->drop;
                LinkedSoundFx_6da0(((unsigned short)p->drop + 0x6a) | 0xc000, p);
                p->drop = 0;
                p->flags &= ~0x40000UL;
                unlink_0e4(p);
            }
        }
    case 2:
        if (damped_contact_motion(p) & 1) {
            eng1_41da0((Actor *)p);
            p->sprite = 0;
            p->state = 4;
        }
        break;
    }
}

void eng1_41b70(HighActor *p)
{
    if (eng1_sim_state_72110) return;
    switch (p->state) {
    case 1:
        if (--p->timer < 0) {
            p->state = 2;
            if (p->drop) {
                --p->drop;
                LinkedSoundFx_6da0(((unsigned short)p->drop + 0x6a) | 0xc000, p);
                p->drop = 0;
            }
        }
    case 2:
        if (damped_contact_motion(p) & 1) {
            eng1_41da0((Actor *)p);
            p->sprite = 0;
            p->state = 4;
        }
        if (p->y > (g_eng1_setup_word_718e0 << 16)) {
            spawn_at_group_300c0(p->x, p->y, p->z, 0x202e, p->profile);
            p->sprite = 0;
            p->state = 4;
        }
        break;
    }
}

void eng1_41c30(HighActor *p)
{
    HighActor *source;
    long remaining;
    remaining = (long)(random_limit(4) + 4U);
    source = p;
    if (remaining > 0) {
        p = (HighActor *)remaining;
        do {
            unsigned long value;
            HighActor *child;
            child = (HighActor *)spawn_at_group_300c0(source->x,
                source->y - (long)((dispatch_engine_2c470() & 0x1f00UL) << 8),
                source->z, 0x4037, 0);
            if (child) {
                child->direction_x = (unsigned char)(0U - (unsigned char)random_limit(2));
                child->direction_z = (unsigned char)(0U - (unsigned char)random_limit(2));
                child->direction_y = 0xff;
                value = dispatch_engine_2c470() & 0x1ff;
                child->velocity_y = 0;
                child->target_y = (value + 0x800) << 8;
                child->rate_y = 0;
                child->extra_y = random_limit(0x2000) + 0xe000;
                value = (dispatch_engine_2c470() & 0x1ff) + 0x100;
                value <<= 8;
                child->velocity_z = child->target_z = value;
                child->rate_z = random_limit(0x1000) + 0x3000;
                child->extra_z = 0;
                value = (dispatch_engine_2c470() & 0x1ff) + 0x100;
                value <<= 8;
                child->velocity_x = child->target_x = value;
                child->rate_x = random_limit(0x1000) + 0x3000;
                child->extra_x = 0;
                child->sprite = (unsigned short)(random_limit(11) + 1);
            }
            p = (HighActor *)((long)p - 1);
        } while (p);
    }
    remove_actor_variant(source);
}

extern int __cdecl abs(int);
extern long __cdecl labs(long);

/* Native continuation bracketed by accepted 459e0, 45a50 and 461a0. */
typedef struct Cursor Cursor;
typedef struct Entry16 { long x,y,z,time; } Entry16;
struct Cursor { Entry16 *base; long index,count,remaining,last,extra; long step,step2,step3; long ax,bx,cx,dx,ay,by,cy,dy,az,bz,cz,dz; long x,vx,ddx,dddx,y,vy,ddy,dddy,z,vz,ddz,dddz; };
typedef struct Callback12 { void (*initialize)(Actor *); void (*update)(Actor *); void (*draw)(Actor *); } Callback12;
typedef struct Callback8 { void (*initialize)(Actor *); void (*update)(Actor *); } Callback8;
extern Callback12 *g_configuration_dword_71370;
extern Callback8 *g_configuration_dword_7139c;
extern Actor *g_eng1_native_actor_701a0;
extern long *g_eng1_native_dword_7018c, *g_eng1_native_dword_70198;
extern unsigned long g_eng1_setup_dword_72014;
extern long g_eng1_native_dword_70190,g_eng1_native_dword_70194,g_eng1_native_dword_7019c;
extern long g_eng1_native_state_7028c,g_eng1_native_dword_70288;
extern unsigned long g_eng1_native_dword_70290,g_eng1_native_dword_70294;
extern long g_eng1_native_dword_7029c,g_eng1_native_state_70244,g_eng1_native_state_702a0;
extern long g_eng1_native_dword_70188;
extern unsigned long eng1_sim_state_72110;
extern Actor *g_normal_actor_pool;
extern long g_boss_threshold_72184;
extern Entry16 g_eng1_path_70018[];
extern Cursor g_eng1_cursor_701b8;
extern long g_eng1_native_vector_701a8[3];
extern long g_eng1_bounds_69358[6],g_eng1_bounds_69370[6];
extern unsigned char g_eng1_palette_70248[];
extern long g_eng1_program_69988[],g_eng1_program_69ab8[],g_eng1_program_69be8[],g_eng1_program_69ca0[],g_eng1_program_69d58[],g_eng1_program_69df0[],g_eng1_program_69e88[],g_eng1_program_6a010[],g_eng1_program_6a198[],g_eng1_program_6a408[];
extern unsigned char g_eng1_sequences_693e8[][96];
extern short g_native_word_70832,g_eng1_atan_68d78[],g_sine_table_5a848[];

extern Actor *spawn_normal(long,long,long,int),*spawn_kind(long,long,long,int);
extern void eng1_131c0(long,long,long,int);
extern void title_0c4d0(unsigned long);
extern void LinkedSoundFxAt_75e3(unsigned short,Actor *,Actor *);
extern void RemoveFxLinks_7ef6(Actor *);
extern int actor_bounds_2a880(Actor *,long *,int),bounds_overlap(void *,long *);
extern int dispatch_local_30010(long,long,long,int);
extern void eng1_1ab50(void),eng1_05820(void *,int,int),eng1_446d0(void *),eng1_44720(void);
extern int eng1_2c030(int,int),eng1_45010(Cursor *);
extern void init_cursor_44740(Entry16 *,int,Cursor *,int);
void eng1_45780(void);
int eng1_44f80(void);
int eng1_45bf0(Actor *,int,int);
int eng1_46110(long,long,long,long);

void eng1_447c0(Actor *p)
{
    long x=(p->x-g_normal_actor_pool->x)>>16,y=(p->y-g_normal_actor_pool->y)>>16;
    long distance=x*x+y*y, direction=0,flag,finished;
    Actor *q;
    int i,best,delta,kind,damage;
    long px,py,pz;
    if(x>=0)direction=1;
    if(p->child->unknown_0c[0x1e-0x0c]!=255)
        eng1_05820(g_eng1_palette_70248,p->child->unknown_0c[0x1e-0x0c],32);
    flag=0;finished=0;
    do {
        switch((unsigned short)p->state){
        case 0:
            if(p->unknown_94!=direction){
                p->program=g_eng1_program_69d58;p->program_start=g_eng1_program_69d58;p->frame_tick=0;p->return_state=2;p->state=1;
                if(p->unknown_94){p->program=g_eng1_program_69df0;p->program_start=g_eng1_program_69df0;p->frame_tick=0;p->return_state=2;p->state=1;}
            }else if(distance<=0x24000 && !g_eng1_setup_dword_72014){
                if(!p->unknown_94){
                    switch(random_limit(2)){
                    case 0:p->program=g_eng1_program_69be8;p->program_start=g_eng1_program_69be8;p->frame_tick=0;p->return_state=0;p->state=1;break;
                    case 1:if(g_eng1_native_dword_7019c){p->program=g_eng1_program_69be8;p->program_start=g_eng1_program_69be8;p->frame_tick=0;p->return_state=0;p->state=1;}
                        else{p->program=g_eng1_program_6a198;p->program_start=g_eng1_program_6a198;p->frame_tick=0;p->return_state=0;p->state=1;g_eng1_native_state_70244=-1;}break;
                    }
                }else{
                    switch(random_limit(2)){
                    case 0:p->program=g_eng1_program_69ca0;p->program_start=g_eng1_program_69ca0;p->frame_tick=0;p->return_state=0;p->state=1;break;
                    case 1:if(g_eng1_native_dword_7019c){p->program=g_eng1_program_69ca0;p->program_start=g_eng1_program_69ca0;p->frame_tick=0;p->return_state=0;p->state=1;}
                        else{p->program=g_eng1_program_6a408;p->program_start=g_eng1_program_6a408;p->frame_tick=0;p->return_state=0;p->state=1;g_eng1_native_state_70244=-1;}break;
                    }
                }
            }else{
                switch(random_limit(2)){
                case 0:p->program=g_eng1_program_69be8;p->program_start=g_eng1_program_69be8;p->frame_tick=0;p->return_state=0;p->state=1;
                    if(p->unknown_94){p->program=g_eng1_program_69ca0;p->program_start=g_eng1_program_69ca0;p->frame_tick=0;p->return_state=0;p->state=1;}break;
                case 1:p->program=g_eng1_program_69e88;p->program_start=g_eng1_program_69e88;p->frame_tick=0;p->return_state=0;p->state=1;
                    if(p->unknown_94){p->program=g_eng1_program_6a010;p->program_start=g_eng1_program_6a010;p->frame_tick=0;p->return_state=0;p->state=1;}break;
                }
            }
            break;
        case 1:
            finished=1;
            if(--p->frame_tick<=0){
                i=*(long *)p->program;p->sprite=i;p->program=(long *)p->program+1;
                switch(i){
                case -4:i=*(long *)p->program;p->program=(long *)p->program+1;eng1_446d0(g_eng1_sequences_693e8[i]);finished=0;break;
                case -3:q=spawn_normal(0,0,0,42);if(q)g_configuration_dword_71370[42].initialize(q);g_eng1_native_state_70244=1;finished=0;break;
                case -2:p->child->sprite=*(short *)p->program;flag=0x800000;p->program=(long *)p->program+1;p->child->render_flags&=~0x10;p->child->render_flags|=p->render_flags&0x10;finished=0;break;
                case -1:i=*(long *)p->program;p->program=(long *)p->program+1;LinkedSoundFx_6da0(i,p);finished=0;break;
                case 0:p->state=p->return_state;finished=0;break;
                default:p->frame_tick=*(short *)p->program;p->program=(long *)p->program+1;p->render_flags&=~0x800000;p->render_flags|=flag;break;
                }
            }break;
        case 2:p->state=0;p->unknown_94^=1;finished=0;break;
        }
    }while(!finished);
    if(g_eng1_native_state_70244>=0){
        while(!eng1_45010(&g_eng1_cursor_701b8)){eng1_44720();g_eng1_native_state_702a0=g_eng1_setup_dword_72014;}
        if(!g_eng1_native_state_702a0){
            px=g_eng1_cursor_701b8.x>>16;py=g_eng1_cursor_701b8.y>>16;pz=g_eng1_cursor_701b8.z>>16;
            if(py>=726){if(px<289)px=289;if(px>993)px=993;}
            if(py<726 && py>=605){if(px<166)px=166;if(px>1136)px=1136;}
            if(py<605){if(px<95)px=95;if(px>1186)px=1186;}
            p->x=px<<16;p->y=py<<16;if(g_eng1_cursor_701b8.extra)p->z=pz<<16;
        }
    }
    if(--g_eng1_native_dword_70194<=0){
        g_eng1_native_dword_70194=random_limit(90)+15;
        px=g_eng1_native_dword_70288>>16;best=999999;
        for(i=0;i<6;++i){delta=abs(px-g_eng1_bounds_69358[i]);if(delta<best){best=delta;direction=i;}
            delta=abs(px-g_eng1_bounds_69370[i]);if(delta<best){best=delta;direction=i;}}
        if(px<g_eng1_bounds_69358[direction])px=g_eng1_bounds_69358[direction];
        if(px>g_eng1_bounds_69370[direction])px=g_eng1_bounds_69370[direction];
        q=spawn_normal(px<<16,0x400000,g_normal_actor_pool->z,41);if(q)g_configuration_dword_71370[41].initialize(q);
        g_eng1_native_dword_70288=g_normal_actor_pool->x;
    }
    if(--g_eng1_native_dword_70190<=0){
        g_eng1_native_dword_70190=random_limit(240)+240;kind=32;px=0x560000;
        if(!random_limit(2)){kind=64;px=0x4a60000;}g_eng1_native_dword_70188=px;
        switch(random_limit(4)){
        case 0:eng1_131c0(px,0x600000,g_normal_actor_pool->z,kind|11);break;
        case 1:eng1_131c0(px,0x600000,g_normal_actor_pool->z,kind|12);break;
        case 2:eng1_131c0(px,0x600000,g_normal_actor_pool->z,kind|13);break;
        case 3:eng1_131c0(px,0x600000,g_normal_actor_pool->z,kind|14);break;
        }
    }
    if(p->invuln<=0 && !eng1_sim_state_72110){
        if(!g_eng1_setup_dword_72014){
            i=eng1_26de0(p);
            if(i){
                if(i==128){damage=eng1_19b40(~g_native_word_72168&0x5f);eng1_1ab50();}
                else{eng1_30290(i,0);damage=2;}
                if(damage){i=256/(200/damage);p->hits-=damage;if(p->hits>0){if(i>=g_native_word_70832)i=g_native_word_70832-1;}else i=g_native_word_70832;*(long *)&g_boss_threshold_72184=i;}
                p->program=g_eng1_program_69988;p->program_start=g_eng1_program_69988;p->frame_tick=0;p->return_state=0;p->state=1;
                if(p->unknown_94){p->program=g_eng1_program_69ab8;p->program_start=g_eng1_program_69ab8;p->frame_tick=0;p->return_state=0;p->state=1;}
                g_eng1_native_state_70244=0;p->invuln=8;
                {int drop=eng1_44f80();if(drop){kind=0x220;if(p->x>0x2800000)kind=0x240;eng1_131c0(p->x,p->y,g_normal_actor_pool->z,drop|kind);}}
            }
            if(p->hits<=0){p->hits=1;g_eng1_native_state_7028c=300;g_eng1_setup_dword_72014=1;eng1_45780();}
        }
    }
    if(g_eng1_setup_dword_72014){
        i=1;do{
            q=spawn_kind(p->x+((random_limit(64)-32)<<16),p->y+((random_limit(64)-32)<<16),p->z,45);
            if(q)g_configuration_dword_7139c[45].initialize(q);
        }while(--i);
        if(!(g_eng1_native_state_7028c&7)){
            q=spawn_kind(p->x,p->y,p->z,41);if(q)g_configuration_dword_7139c[41].initialize(q);q->timer=1;
        }
    }
}

int eng1_44f80(void)
{
    long value=*g_eng1_native_dword_7018c++;
    while(value<=0){
        if(value==0){g_eng1_native_dword_70290=*g_eng1_native_dword_7018c++;g_eng1_native_dword_70294=*g_eng1_native_dword_7018c++;}
        if(value<0)g_eng1_native_dword_7018c=g_eng1_native_dword_70198;
        value=*g_eng1_native_dword_7018c++;
    }
    if(g_eng1_native_dword_70290>=random_limit(g_eng1_native_dword_70294)+1 && g_eng1_native_dword_70290!=g_eng1_native_dword_70294){--g_eng1_native_dword_7018c;return 0;}
    return value;
}

void eng1_45780(void)
{
    int x=g_eng1_native_actor_701a0->x>>16,y=g_eng1_native_actor_701a0->y>>16,z=g_eng1_native_actor_701a0->z>>16;
    int px,py,pz,dx,dy,steps,base,offset,sign,rise;
    long *e;
    g_eng1_path_70018[2].x=x<<16;g_eng1_path_70018[2].y=y<<16;g_eng1_path_70018[2].z=z<<16;g_eng1_path_70018[2].time=16;
    px=g_normal_actor_pool->x>>16;py=g_normal_actor_pool->y>>16;pz=g_normal_actor_pool->z>>16;
    g_eng1_path_70018[3].x=px<<16;g_eng1_path_70018[3].y=py<<16;g_eng1_path_70018[3].z=pz<<16;g_eng1_path_70018[3].time=16;
    dx=abs(x-px);dy=abs(y-py);steps=eng1_2c030(dx,dy)>>4;if(steps<2)steps=2;g_eng1_path_70018[2].time=steps;
    base=px;offset=128;sign=1;rise=-16;
    for(e=&g_eng1_path_70018[4].y;(long)e<(long)&g_eng1_path_70018[21].y;e+=4){
        if(py>=726){if(px<289)px=289;if(px>993)px=993;}
        if(py<726 && py>=605){if(px<166)px=166;if(px>1136)px=1136;}
        if(py<605){if(px<95)px=95;if(px>1186)px=1186;}
        e[-1]=px<<16;e[0]=py<<16;e[1]=pz<<16;e[2]=16;
        py+=rise;px=sign*offset+base;offset+=32;sign=-sign;rise-=8;
    }
    g_eng1_path_70018[21].x=0x2000000;g_eng1_path_70018[21].y=pz<<16;g_eng1_path_70018[21].time=16;
    init_cursor_44740(g_eng1_path_70018,20,&g_eng1_cursor_701b8,1);
}
void eng1_45930(Actor *p)
{
    p->sprite=0x9c;
    if(g_eng1_setup_dword_72014){p->unknown_4a=0x80;p->render_flags|=0x4000605;}
}
void eng1_45960(Actor *p)
{
    if(--p->frame_tick<=0){p->frame_tick=3;if(++p->sprite>0xb1)p->sprite=0x9c;}
    if(!p->timer){p->x=g_eng1_native_actor_701a0->x;p->y=g_eng1_native_actor_701a0->y;p->z=g_eng1_native_actor_701a0->z;}
    else{p->y-=0x10000;p->scale-=6;if(p->scale<=8)release_actor(p);}
}
void eng1_45a00(Actor *p)
{
    if(--p->frame_tick<=0){p->frame_tick=3;if(++p->sprite>=0x2ba)p->sprite=0x2ad;}
    p->y+=0x20000;p->scale-=6;if(p->scale<=8)release_actor(p);
}
void eng1_45a80(Actor *p)
{
    int remove=g_eng1_setup_dword_72014;
    unsigned long height;
    Actor *q;
    if((p->unknown_0c[0x1f-0x0c]&3)==3 && !p->unknown_a8){p->unknown_a8=1;LinkedSoundFxAt_75e3(0xcac1,p,g_eng1_native_actor_701a0);}
    if(--p->frame_tick<=0){p->frame_tick=1;if(++p->sprite>=0x190)p->sprite=0x188;}
    switch((unsigned short)p->state){
    case 0:height=(unsigned short)grid_scan_20b50(p->x>>16,p->y>>16,p->z>>16,p->profile);if((unsigned long)p->unknown_b8>=height){
            eng1_native_code_target_002a290(p);q=spawn_kind(p->x,p->y,p->z,42);if(q)g_configuration_dword_7139c[42].initialize(q);p->state=1;
        }else{p->y+=p->unknown_b8<<16;if(++p->unknown_b8>8)p->unknown_b8=8;}break;
    case 1:p->scale-=24;if(p->scale<=24)remove=1;break;
    }
    if(actor_bounds_2a880(p,g_eng1_native_vector_701a8,1) && bounds_overlap((char *)g_collision_context+0x88,g_eng1_native_vector_701a8))eng1_45bf0(p,18,0);
    if(remove)remove_actor(p);
}
int eng1_45bf0(Actor *p,int kind,int arg)
{
    int result=0,value;
    if(!random_limit(8)){
        result=eng1_native_code_target_00266a0(p,kind,arg);
        if(result){
            value=random_limit(5);if(value==g_eng1_native_dword_7029c){--value;if(value<0)value=4;}g_eng1_native_dword_7029c=value;
            switch(value){
            case 0:title_0c4d0(0x70);break;
            case 1:title_0c4d0(0x71);break;
            case 2:title_0c4d0(0x73);break;
            case 3:title_0c4d0(0x75);break;
            case 4:LinkedSoundFx_6da0(0x4acb,g_eng1_native_actor_701a0);break;
            }
        }
    }
    return result;
}
void eng1_45cc0(Actor *p)
{
    p->unknown_4a=0x80;p->render_flags|=0x4000605;p->sprite=0x183;p->unknown_36=3;p->state=0;
    switch(random_limit(4)){
    case 0:LinkedSoundFxAt_75e3(0x4ac2,p,g_eng1_native_actor_701a0);break;
    case 1:LinkedSoundFxAt_75e3(0x4ac3,p,g_eng1_native_actor_701a0);break;
    case 2:LinkedSoundFxAt_75e3(0x4ac4,p,g_eng1_native_actor_701a0);break;
    }
}
void eng1_45d50(Actor *p)
{
    int remove=g_eng1_setup_dword_72014;
    if(--p->frame_tick<=0){
        ++p->sprite;p->frame_tick=3;
        switch((unsigned short)p->state){
        case 0:if(p->sprite>=0x188){p->sprite=0x174;p->state=1;}break;
        case 1:if(p->sprite>=0x17e){if(--p->unknown_36<=0)p->state=2;else p->sprite=0x174;}break;
        case 2:if(p->sprite>=0x183)remove=1;break;
        }
    }
    if(actor_bounds_2a880(p,g_eng1_native_vector_701a8,1) && bounds_overlap((char *)g_collision_context+0x88,g_eng1_native_vector_701a8))eng1_45bf0(p,17,0);
    if(remove)release_actor(p);
}
void eng1_45e20(Actor *p)
{
    p->unknown_4a=0x80;p->render_flags|=0x4000605;p->mode=5;
    p->unknown_94=g_eng1_native_actor_701a0->unknown_94;
    p->x=g_eng1_native_actor_701a0->x+0x780000;p->unknown_a8=0;
    if(p->unknown_94){p->x=g_eng1_native_actor_701a0->x-0x780000;p->unknown_a8=0x200;}
    p->y=g_eng1_native_actor_701a0->y-0x100000;p->z=g_eng1_native_actor_701a0->z;
    p->sprite=0x1d0;p->unknown_b8=1;p->unknown_c8=16;p->unknown_36=240;g_eng1_native_dword_7019c=1;
    LinkedSoundFxAt_75e3(0x4ac9,p,g_eng1_native_actor_701a0);
}
void eng1_45ef0(Actor *p)
{
    int remove=g_eng1_setup_dword_72014,angle,delta;
    unsigned long height;
    Actor *q;
    if(g_eng1_native_state_7028c&1){q=spawn_kind(p->x,p->y,p->z,44);if(q)g_configuration_dword_7139c[44].initialize(q);}
    if(--p->frame_tick<=0){p->frame_tick=2;if(++p->sprite>=0x1d4)p->sprite=0x1d0;}
    angle=eng1_46110(g_normal_actor_pool->x,g_normal_actor_pool->y,p->x,p->y);
    delta=wrap_delta_461a0(angle,p->unknown_a8);
    if(delta>p->unknown_b8)delta=p->unknown_b8;if(delta < -p->unknown_b8)delta=-p->unknown_b8;
    p->unknown_a8+=delta;if(p->unknown_b8<32)++p->unknown_b8;
    p->x+=(g_sine_table_5a848[(p->unknown_a8+256)&1023]<<1)*p->unknown_c8;
    p->y+=(g_sine_table_5a848[p->unknown_a8&1023]<<1)*p->unknown_c8;
    if(!(g_eng1_native_state_7028c&1) && p->unknown_c8>10)--p->unknown_c8;
    if(p->unknown_94)p->unknown_74-=128;else p->unknown_74+=128;
    height=(unsigned short)grid_scan_20b50(p->x>>16,(p->y-0x280000)>>16,p->z>>16,p->profile);if(height==0x7fff)remove=1;
    if(dispatch_local_30010(p->x,p->y,p->z,1))remove=1;
    if(--p->unknown_36<=0)remove=1;
    if(actor_bounds_2a880(p,g_eng1_native_vector_701a8,1) && bounds_overlap((char *)g_collision_context+0x88,g_eng1_native_vector_701a8) && eng1_45bf0(p,3,0))remove=1;
    if(remove){RemoveFxLinks_7ef6(p);if(p->unknown_36>0)LinkedSoundFx_6da0(0xc2ca,p);remove_actor(p);g_eng1_native_dword_7019c=0;}
}
int eng1_46110(long x1,long y1,long x2,long y2)
{
    long ax,ay,angle;
    x1=(x2-x1)>>16;ax=labs(x1);
    y1=(y2-y1)>>16;ay=labs(y1);
    if(ay==ax)angle=ax?128:0;
    else if(ay<ax)angle=g_eng1_atan_68d78[(ay<<9)/ax];
    else angle=256-g_eng1_atan_68d78[(ax<<9)/ay];
    if(x1<0)angle=512-angle;if(y1<0)angle=1024-angle;
    return angle;
}
