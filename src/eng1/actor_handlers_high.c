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
typedef struct HighKindInfo { unsigned char *owner; long count; } HighKindInfo;
extern HighKindInfo g_kind_info[];
extern unsigned long eng1_sim_state_72110;
extern unsigned short g_eng1_setup_word_718e0;
extern short g_pipeline_dword_716a8;
extern long g_pipeline_dword_71718;
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
    p->step = g_pipeline_dword_716a8;
    p->scale_z += g_pipeline_dword_71718 + 32;
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
