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
    unsigned char unknown_e4[0xee - 0xe4];
    short hits;
    unsigned char unknown_f0[0x114 - 0xf0];
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
                                           unsigned short kind,
                                           unsigned short group);

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
typedef struct ProfileRows { unsigned char *rows; long count; } ProfileRows;
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
extern void eng1_02ff0(long *);
extern void eng1_05a30(int, int, int);
extern Actor *spawn_middle_277e0(long, long, long, unsigned short);
extern int eng1_29e20(Actor *);
extern void mark_actor_resource_for_release(Actor *);
extern int contact_motion(Actor *);
extern int eng1_26de0(Actor *);
extern void eng1_1aba0(void);
extern void eng1_30290(int, int);
extern void eng1_2b320(ActorList *, Actor *);
extern unsigned short eng1_20b50(int, int, int, int);
extern unsigned short eng1_20d30(int, int, int, int);
extern void eng1_27410(Actor *);
extern int eng1_2a710(void *, Actor *);
extern void eng1_native_code_target_00266a0(Actor *, int, int);
extern int eng1_19b40(int);
extern void eng1_0ef10(Actor *);
extern unsigned short g_eng1_classifier_rows_65678[][6];
extern short g_eng1_word_70000;
extern long g_eng1_dword_70ca0, g_eng1_dword_70ca4;
extern BurstEntry g_eng1_burst_65b98[];
extern void *g_eng1_programs_67070[];
extern Actor *g_collision_context;
extern unsigned short g_eng1_setup_word_72012, g_native_word_72168;
extern unsigned short g_eng1_word_7082e;
extern unsigned char *g_eng1_ptr_70f24;
extern Actor *g_eng1_actor_72860;
extern unsigned short *g_configuration_dword_713bc;
extern ProfileRows g_eng1_profiles_6fee8[];
extern long g_eng1_dword_72110;
extern unsigned short g_eng1_word_70ef0, g_eng1_word_70f0e, g_eng1_word_7144c;
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
    v[0] = g_eng1_dword_70ca0 >> 2;
    v[1] = (g_eng1_dword_70ca4 - 0x250) / 3;
    eng1_02ff0(v);
    if (v[1] < 32) eng1_05a30(0x3f, 0x75, 0xbc);
    else eng1_05a30(0x7f, 0x8b, 0x47);
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
                mask = (unsigned char)g_eng1_setup_word_72012 & 0x5f;
                { unsigned int used = g_native_word_72168; mask &= ~used; }
                if (mask) {
                    eng1_1aba0(); g_native_word_72168 |= mask;
                    if ((g_eng1_setup_word_72012 & 2) && g_eng1_word_7082e >= 200) {
                        eng1_42f90(q); LinkedSoundFx_6da0(0xd071,p); g_eng1_ptr_70f24[2] = 2; q->state = 2;
                    } else {
                        eng1_42db0(q); --q->hits;
                        if (q->hits <= 0) { LinkedSoundFx_6da0(0xd071,p); q->state = 2; g_eng1_ptr_70f24[2] = 2; }
                        else { LinkedSoundFx_6da0(0xd04c,p); q->state = 1; }
                    }
                }
            } else {
                eng1_30290(hit,0);
                if (p->y == p->saved_y) {
                    eng1_42db0(q); q->hits -= 2;
                    if (q->hits <= 0) { LinkedSoundFx_6da0(0xd071,p); q->state = 2; g_eng1_ptr_70f24[2] = 2; }
                    else { LinkedSoundFx_6da0(0xd04c,p); q->state = 1; }
                }
            }
        }
    }
    if (p->z < -0x640000) dead = 1;
    if (p->y > 0x3400000) dead = 1;
    if (eng1_native_code_target_002a9e0(p,0x100)) dead = 1;
    if (dead) { if (q->list) eng1_2b320(q->list,p); remove_actor_variant(p); }
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
    eng1_20b50(p->x >> 16,p->y >> 16,p->z >> 16,p->profile);
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
    dx = p->x-g_eng1_actor_72860->x; dy=p->y-g_eng1_actor_72860->y;
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
void eng1_43b50(Actor *p,long dx)
{
    int random = random_limit(0x8000), category, x,z;
    long offset, absolute;
    unsigned short tile;
    category=(int)p->category-(int)g_eng1_actor_72860->category;
    offset=p->record_offset;
    absolute=p->x-*(long *)(g_eng1_profiles_6fee8[p->profile].rows+offset);
    if(absolute<0)absolute=-absolute;
    if(absolute>0x2800000){p->state=45;p->unknown_b8=0;p->unknown_c0=-0x8000;select_frame(p,2);}
    else{
        if(category && random>0x4000){
            if(category>0){
                z=(p->z>>16)-1;x=p->x>>16;
                tile=eng1_20d30(x,0,z,0);
                if(!(g_configuration_dword_713bc[tile]&2)){
                    if(!p->unknown_94)++x;else x+=0xffff;
                    tile=eng1_20d30(x,0,z,0);
                    if(!(g_configuration_dword_713bc[tile]&2)){p->state=43;select_frame(p,9);goto advance;}
                }
            }else{
                z=(p->z>>16)+1;x=p->x>>16;
                tile=eng1_20d30(x,0,z,0);
                if(!(g_configuration_dword_713bc[tile]&2)){
                    if(!p->unknown_94)++x;else x+=0xffff;
                    tile=eng1_20d30(x,0,z,0);
                    if(!(g_configuration_dword_713bc[tile]&2)){p->state=44;select_frame(p,10);goto advance;}
                }
            }
        }
        if(!p->unknown_94){
            if(dx>0x3aaaaa){p->state=9;select_frame(p,3);goto advance;}
            if(!(random&6)){p->state=39;select_frame(p,7);goto advance;}
            if(dx < -0x3aaaaa && !category && dx > -0xb00000)goto attack;
        }else{
            if(dx < -0x3aaaaa){p->state=9;select_frame(p,3);goto advance;}
            if(!(random&6)){p->state=39;select_frame(p,7);goto advance;}
            if(dx > 0x3aaaaa && !category && dx < 0xb00000)goto attack;
        }
        if(!(random&8)){p->state=39;select_frame(p,7);}else{p->state=2;select_frame(p,2);}
        goto advance;
attack:
        if(g_eng1_dword_72110){p->state=39;select_frame(p,7);}else{p->state=34;select_frame(p,5);}
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
    eng1_43ff0(p);++g_eng1_word_70f0e;g_eng1_word_7144c=0;
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
        if(g_eng1_actor_72860->y-p->y>0x400000)remove_actor_variant(p);
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
    delta=(unsigned short)eng1_20b50(p->x>>16,p->y>>16,p->z>>16,p->profile)-margin;
    if(delta<0)p->y+=delta<<16;
}
