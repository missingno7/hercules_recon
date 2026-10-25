/* ENG1.DLL 0x10120..0x12620: level-1 actor handlers (debris burst 0x10120, spatial effect 0x102c0, the
 * climbing creature state machine 0x105a0 with its state helpers 0x107c0..0x113c0 and callbacks 0x11450..0x11df0)
 * and the HUD/boss-bar update pair 0x11fc0/0x12040 (diverged ENG1 counterparts of ENG3 0x171a0/0x17210,
 * calibration/eng3/handlers_16240.cpp). Lane x17 scratch, plain C (C++ measures identically).
 * 0x10380 and 0x117d0 (FUNCTION_MATCH in src/eng1/actor_handlers_low.c) sit inside this span and are not repeated.
 * <stdlib.h> is a genuine use (abs), <string.h> a genuine use (memset in 0x11fc0, as in ENG3 0x171a0).
 * Types/idioms follow calibration/eng1/player_unit_36030.c and status_panel_32b10.c; offsets are provisional
 * shared names. Jump tables: 0x10780 belongs to 0x105a0, 0x125f0 to 0x12040. */

#include <stdlib.h>
#include <string.h>

typedef struct Actor Actor;
typedef struct FixedPosition { long x, y, z; } FixedPosition;
typedef struct RelativeMeasure {
    short x, y, z;
    short planar_distance;
    unsigned short distance;
    unsigned short direction;
} RelativeMeasure;
typedef struct KindInfo { unsigned char *owner; long count; } KindInfo;
typedef struct Bounds {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds;
typedef struct Offset2 { short x, y; } Offset2;
typedef struct SpawnRecord {        /* 8-byte spawn placement, 0x52100 */
    unsigned short x, y, z;
    unsigned short kind;
} SpawnRecord;
typedef struct PanelState {         /* 0x70820, as calibration/eng1/status_panel_32b10.c */
    unsigned short value_00;
    unsigned short value_02;
    unsigned short shown_04;
    unsigned short shown_06;
    unsigned char rate_08;
    unsigned char rate_09;
    unsigned char rate_0a;
    unsigned char unknown_0b;
    unsigned short flash_0c;
    unsigned short flash_0e;
    short level_10;
    short target_12;
    unsigned char mode_14;
    unsigned char unknown_15;
    short fade_16;
    short x_18;
    short y_1a;
} PanelState;

typedef struct PanelContext {       /* view of g_engine_interface.context_004 */
    signed char level_000;
    signed char sublevel_001;
    unsigned char unknown_002[0x0c - 0x02];
    signed char lives_00c;
    unsigned char unknown_00d;
    signed char difficulty_00e;
    unsigned char unknown_00f[0x22 - 0x0f];
    short value_022;
    short energy_024[6];
    unsigned char unknown_030[0x5c - 0x30];
    unsigned short held_05c;
    unsigned char unknown_05e[0x74 - 0x5e];
    unsigned short previous_key_074;
    unsigned short next_key_076;
} PanelContext;

typedef struct EngineInterfaceView {
    unsigned long signature_000;
    PanelContext *context_004;
} EngineInterfaceView;

struct Actor {
    long x, y, z;
    unsigned char unknown_00c[0x1c - 0x0c];
    unsigned char clut_01c;
    unsigned char unknown_01d[0x21 - 0x1d];
    unsigned char counter_021;
    unsigned char category;
    unsigned char mode;
    unsigned char unknown_024;
    unsigned char control_025;
    unsigned char unknown_026[0x2a - 0x26];
    unsigned short state;
    unsigned char unknown_02c[0x2e - 0x2c];
    unsigned short kind;
    short timer_030;
    unsigned char unknown_032[0x34 - 0x32];
    unsigned short sprite;
    unsigned short entry_count;
    unsigned short profile;
    unsigned short scale;
    unsigned char unknown_03c[0x4c - 0x3c];
    long parameter;
    unsigned long flags;
    unsigned long render_flags;
    long record_offset;
    unsigned char unknown_05c[0x60 - 0x5c];
    Actor *link_060;
    unsigned short *image_064;
    unsigned char unknown_068[0x70 - 0x68];
    unsigned short angle_70;
    unsigned short angle_72;
    unsigned char unknown_074[0x88 - 0x74];
    Bounds bounds;
    unsigned char direction_x;
    unsigned char direction_y;
    unsigned char direction_z;
    unsigned char unknown_097;
    unsigned char unknown_098[0x9a - 0x98];
    short unknown_09a;
    long saved_x, saved_y, saved_z;
    long motion_x_velocity, motion_x_target, motion_x_rate, motion_x_unknown;
    long motion_y_velocity, motion_y_target, motion_y_rate, motion_y_unknown;
    long motion_z_velocity, motion_z_target, motion_z_rate, motion_z_unknown;
    long home_x, home_y, home_z;
    Actor *link_0e4;
    Actor *link_0e8;
    unsigned char lifetime;
    unsigned char unknown_0ed;
    short hits;
    unsigned char unknown_0f0[0xf4 - 0xf0];
    long unknown_0f4;
    unsigned char unknown_0f8[0x108 - 0xf8];
    unsigned long unknown_108;
    unsigned long unknown_10c;
    unsigned char unknown_110[0x124 - 0x110];
    short unknown_124;
    short unknown_126;
    unsigned char unknown_128[0x130 - 0x128];
    Actor *link_130;
};

extern EngineInterfaceView g_engine_interface;
extern KindInfo g_kind_info[];
extern Actor *g_native_pointer_7217c;
extern Actor *g_pipeline_dword_71a18;
extern unsigned short *g_spatial_sound_words_71714;
extern unsigned short *g_configuration_dword_713bc;
extern unsigned short g_native_word_72168;
extern unsigned short g_boss_state_6ffda;
extern long g_boss_threshold_72184;
extern long g_eng1_cursor_721ac;
extern unsigned short g_eng1_words_52790[];
extern unsigned short g_eng1_word_6ffdc;
extern long g_eng1_setup_dword_72188;
extern long g_eng1_dword_721b0;
extern short g_native_word_70832;
extern signed char g_eng1_byte_719de;
extern long g_actor_worklist_716c0[];
extern short g_pipeline_word_71ca0[];
extern short g_pipeline_word_71720[];
extern short g_pipeline_word_71740[];
extern Actor *g_frame_global_734f4;
extern SpawnRecord g_eng1_spawn_records_52100[];
extern unsigned char g_eng1_clut_rows_52b38[][8];
extern PanelState g_panel_70820;
extern long g_eng1_setup_dword_6ef5c;
extern short g_eng1_word_6ef50;
extern signed char g_eng1_setup_byte_6efb4;
extern unsigned short eng1_sim_state_72108;
extern unsigned long eng1_sim_state_72110;
extern short g_eng1_setup_word_72194;
extern short g_eng1_setup_word_7219c;
extern short g_eng1_setup_word_7219e;
extern short g_eng1_setup_word_721a0;
extern short g_eng1_setup_word_721a2;
extern short g_eng1_setup_word_721a4;
extern short g_eng1_setup_word_721a6;
extern long g_eng1_setup_dword_7218c;
extern long g_eng1_setup_clear_72140[6];
extern short g_eng1_weapon_targets_55310[];
extern short g_eng1_life_targets_55330[];

unsigned int random_limit(int limit);
unsigned int dispatch_engine_2c470(void);
void play_sound(unsigned short id, Actor *p);
Actor *spawn_at_group_300c0(long x, long y, long z, unsigned short kind, int group);
int remove_actor(Actor *p);
int remove_actor_variant(Actor *p);
void advance_frame(Actor *p);
void restart_steps(Actor *p);
void select_frame(Actor *p, int index);
void append_chain(Actor *p, Actor *next);
Actor *find_tag(int tag);
void apply_frame_motion_2d(Actor *p);
unsigned char advance_target_motion(Actor *p, unsigned char flags);
int eng1_interpret_signed_pairs(Actor *p);
void eng1_native_code_target_002a290(Actor *p);
int eng1_native_code_target_002aa30(Actor *p, int a, int b);
void eng1_native_code_target_000ef10(Actor *p);
long grid_scan_20b50(long x, long y, long z, int profile);
unsigned long dispatch_local_2a170(long x, long y, long z, int profile);
int eng1_29c20(Actor *p);
int eng1_29e20(Actor *p);
int eng1_2a9e0(Actor *p, int range);
unsigned long eng1_2bd20(Actor *p);
void eng1_2ad70(FixedPosition *a, Actor *p, RelativeMeasure *m);
void eng1_2ae50(Actor *p);
unsigned long eng1_26de0(Actor *p);
void eng1_27410(Actor *p);
void eng1_30290(unsigned long hit, int b);
void eng1_1aba0(void);
int eng1_19b40(int mask);
void measure_relative_vector(FixedPosition *a, FixedPosition *b, RelativeMeasure *m);
int count_chain(Actor *p);
int eng1_2a3d0(Actor *p, Actor *q);
int eng1_2a710(Bounds *b, Actor *p);
long eng1_native_code_target_00266a0(Actor *p, int priority, int value);
unsigned long eng1_2bc50(Actor *p);
int eng1_31000(Actor *p, Offset2 *pos);
void eng1_27450(Actor *p);

void eng1_107c0(Actor *p);
void eng1_10800(Actor *p);
void eng1_108e0(Actor *p);
void eng1_109e0(Actor *p);
void eng1_10b10(Actor *p);
void eng1_10b90(Actor *p);
void eng1_10bd0(Actor *p);
void eng1_10cc0(Actor *p);
void eng1_10dc0(Actor *p);
void eng1_10e40(Actor *p);
void eng1_10e70(Actor *p);
void eng1_11040(Actor *p);
void eng1_111a0(Actor *p);
void eng1_112d0(Actor *p);
void eng1_113c0(Actor *p);
void eng1_11020(Actor *p);
void eng1_11180(Actor *p);
void eng1_10a40(Actor *p);
void eng1_108b0(Actor *p);
void eng1_10900(Actor *p);
void eng1_11290(Actor *p);
void eng1_11300(Actor *p);
void eng1_11d30(Actor *p);

void eng1_10120(Actor *p)
{
    int count, i;
    long dx, v;
    Actor *a;

    count = random_limit(0xb) + 8;
    for (i = 0; i < count; i++) {
        dx = (random_limit(0x60) - 0x30) << 16;
        random_limit(0x20);
        a = spawn_at_group_300c0(p->x + dx, p->y, p->z, 0x4024, 0);
        if (a != 0) {
            a->render_flags |= 0x10000000;
            a->sprite = g_spatial_sound_words_71714[random_limit(2) + 3];
            a->scale = random_limit(0xa0 - count * 3) + 0x40 - count * 3;
            a->direction_x = -random_limit(2);
            a->direction_z = 0xff;
            a->direction_y = 0xff;
            v = ((dispatch_engine_2c470() & 0x1ff) + 0x800) << 8;
            a->motion_y_rate = 0;
            a->motion_y_velocity = (a->motion_y_target = v) >> 3;
            a->motion_y_unknown = random_limit(0x2000) + 0x4000;
            v = ((dispatch_engine_2c470() & 0xff) + 0x100) << 8;
            a->motion_z_velocity = a->motion_z_target = v;
            a->motion_z_rate = random_limit(0x1000) + 0x1000;
            a->motion_z_unknown = 0;
            v = ((dispatch_engine_2c470() & 0x1ff) + 0x100) << 8;
            a->motion_x_velocity = a->motion_x_target = v;
            a->motion_x_rate = random_limit(0x1000) + 0x1000;
            a->motion_x_unknown = 0;
            a->state = 0;
        }
    }
}

void eng1_native_code_target_0102c0(Actor *p)
{
    unsigned long hit;

    p->angle_70 = 0x400 - ((p->z >> 13) & 0x3ff);
    p->angle_72 = 0x400 - ((p->x >> 13) & 0x3ff);
    hit = eng1_2bd20(p);
    if (hit & 2) {
        remove_actor_variant(p);
        return;
    }
    if (p->state < random_limit(2) + 1 && (hit & 1)) {
        play_sound(g_spatial_sound_words_71714[random_limit(2) + 1] | 0xc000, p);
        p->state++;
    }
    if (p->z < (long)0xff000000 || p->y > 0x5800000 || eng1_native_code_target_002aa30(p, 0x80, 0x190))
        remove_actor_variant(p);
}

void eng1_native_code_target_0103c0(Actor *p)
{
    Actor *a;

    g_pipeline_dword_71a18 = p;
    eng1_2ae50(p);
    if (g_engine_interface.context_004->level_000 == 5 && g_boss_state_6ffda == 9 &&
        g_native_pointer_7217c->z > 0x700000) {
        a = p->link_060;
        if (a != 0 && a->kind == p->unknown_108)
            remove_actor(a);
        g_pipeline_dword_71a18 = 0;
        eng1_27410(p);
        return;
    }
    if (p->link_060 != 0 && p->link_060->kind == p->unknown_108) {
        if (!eng1_2a9e0(p, 0x200) || !eng1_2a9e0(p->link_060, 0x200))
            return;
        remove_actor(p->link_060);
    } else if (!eng1_2a9e0(p, 0x200)) {
        if (eng1_2a9e0(p, 0x30)) {
            a = spawn_at_group_300c0(p->x, p->y, p->z, (unsigned short)p->unknown_108, 0);
            if (a != 0) {
                p->link_060 = a;
                a->sprite = p->image_064[random_limit(8) + 2];
            }
            if (p->unknown_10c != 0)
                spawn_at_group_300c0(p->x, p->y, p->z, (unsigned short)p->unknown_10c, 0);
        }
        if (!eng1_2a9e0(p, 0x200))
            return;
    }
    g_pipeline_dword_71a18 = 0;
    remove_actor(p);
}

void eng1_native_code_target_010540(Actor *p)
{
    Actor *t;

    p->flags |= 0x80200;
    advance_frame(p);
    restart_steps(p);
    t = find_tag(0x33);
    if (t != 0)
        append_chain(t, p);
    p->unknown_124 = 5;
    eng1_native_code_target_002a290(p);
    p->hits = 3;
    eng1_107c0(p);
}

void eng1_native_code_target_0105a0(Actor *p)
{
    unsigned long hit;
    int damage;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
        return;
    }
    switch (p->state) {
    case 1:
        eng1_10800(p);
        break;
    case 2:
        eng1_108e0(p);
        break;
    case 4:
        eng1_10b10(p);
        break;
    case 5:
        eng1_10bd0(p);
        break;
    case 3:
    case 6:
        eng1_109e0(p);
        break;
    case 7:
        eng1_10dc0(p);
        break;
    case 8:
        eng1_10e70(p);
        break;
    case 9:
        eng1_11040(p);
        break;
    case 10:
        eng1_112d0(p);
        break;
    case 11:
        eng1_111a0(p);
        break;
    case 12:
    case 13:
        eng1_113c0(p);
        return;
    case 14:
        eng1_interpret_signed_pairs(p);
        return;
    case 15:
        eng1_interpret_signed_pairs(p);
        p->y = 0x1800000;
        return;
    }
    eng1_native_code_target_000ef10(p);
    if (p->z < -0x800000)
        p->z = -0x800000;
    if (p->z > 0x2ff0000)
        p->z = 0x2ff0000;
    hit = eng1_26de0(p);
    if (hit != 0 && p->lifetime == 0) {
        if (hit == 0x80) {
            eng1_1aba0();
            damage = eng1_19b40(~g_native_word_72168 & 0x5f);
            if (damage != 0) {
                p->hits -= damage;
                if (p->state == 1)
                    select_frame(p, 0x1b);
                else
                    select_frame(p, 0x1e);
            }
        } else {
            eng1_30290(hit, 0);
            p->hits -= 100;
            if (p->state == 1)
                select_frame(p, 0x1b);
            else
                select_frame(p, 0x1e);
        }
        p->lifetime = 0x14;
    }
    if (p->hits <= 0) {
        select_frame(p, 0x24);
        p->flags &= ~0x200;
        if (p->link_0e8 != 0) {
            p->link_0e8->link_0e4 = 0;
            p->link_0e8 = 0;
        }
        p->state = 14;
        g_boss_threshold_72184 = 0x100 / (g_engine_interface.context_004->difficulty_00e * 6 + 7) + 1;
    }
}

void eng1_107c0(Actor *p)
{
    p->unknown_0f4 = random_limit(3) + 3;
    p->direction_x = -random_limit(1);
    select_frame(p, 0);
    p->state = 1;
}

void eng1_10800(Actor *p)
{
    unsigned long w;

    eng1_interpret_signed_pairs(p);
    apply_frame_motion_2d(p);
    if (!eng1_29c20(p) ||
        ((w = g_configuration_dword_713bc[dispatch_local_2a170(p->x + (p->direction_x ? -0x100000 : 0x100000), p->y,
                                                               (p->z + 0x400000) & 0xff800000, p->profile)]) & 2)) {
        p->x = p->saved_x;
        p->y = p->saved_y;
        select_frame(p, 2);
    }
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
}

void eng1_108b0(Actor *p)
{
    select_frame(p, 0x29);
    p->state = 2;
    p->unknown_0f4 = 0;
}

void eng1_108e0(Actor *p)
{
    eng1_interpret_signed_pairs(p);
    apply_frame_motion_2d(p);
}

void eng1_10900(Actor *p)
{
    p->motion_x_unknown = 0xa000;
    p->motion_y_unknown = 0xa000;
    p->motion_z_unknown = 0xa000;
    p->counter_021 = 10;
    p->state = 3;
    p->motion_x_velocity = 0;
    p->motion_x_target = 0xf0000;
    p->motion_x_rate = 0xb000;
    p->motion_y_velocity = 0;
    p->motion_y_target = 0x80000;
    p->motion_y_rate = 0x6000;
    p->direction_y = 0xff;
    p->motion_z_velocity = 0;
    p->motion_z_target = 0xf0000;
    p->motion_z_rate = 0xb000;
    p->direction_z = 0xff;
    if (p->direction_x)
        p->home_x = p->x + 0x1800000;
    else
        p->home_x = p->x - 0x1800000;
    p->home_y = p->y - 0x300000;
    p->home_z = p->z - 0x1000000;
    select_frame(p, 9);
}

void eng1_109e0(Actor *p)
{
    unsigned char r;

    eng1_interpret_signed_pairs(p);
    r = advance_target_motion(p, 7);
    if (p->state == 3) {
        if ((r & 1) && p->motion_x_velocity < 0x60000)
            select_frame(p, 10);
        p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
    }
    if (p->counter_021)
        p->counter_021--;
}

void eng1_10a40(Actor *p)
{
    FixedPosition *home;

    p->direction_x = 0xff;
    p->direction_y = 0xff;
    p->direction_z = 0xff;
    p->counter_021 = 0x28;
    p->state = 4;
    p->motion_x_velocity = 0;
    p->motion_x_target = 0xf0000;
    p->motion_x_rate = 0x8000;
    p->motion_x_unknown = 0xf000;
    p->motion_y_velocity = 0;
    p->motion_y_target = 0x80000;
    p->motion_y_rate = 0x6000;
    p->motion_y_unknown = 0xf000;
    p->motion_z_velocity = 0;
    p->motion_z_target = 0xf0000;
    p->motion_z_rate = 0x8000;
    p->motion_z_unknown = 0xa000;
    home = (FixedPosition *)(g_kind_info[p->profile].owner + p->record_offset);
    if (home != 0) {
        p->home_x = home->x;
        p->home_y = home->y;
        p->home_z = home->z;
    }
    select_frame(p, 0xd);
}

void eng1_10b10(Actor *p)
{
    RelativeMeasure m;
    unsigned char r;

    eng1_interpret_signed_pairs(p);
    r = advance_target_motion(p, 7);
    eng1_2ad70((FixedPosition *)p, p, &m);
    if (m.distance < 8)
        eng1_10b90(p);
    if (p->state == 3) {
        if ((r & 1) && p->motion_x_velocity < 0x60000)
            select_frame(p, 0xe);
        p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
    }
}

void eng1_10b90(Actor *p)
{
    select_frame(p, 0x11);
    p->state = 5;
    p->unknown_0f4 = 0;
    if (p->link_0e8 != 0) {
        p->link_0e8->link_0e4 = 0;
        p->link_0e8 = 0;
    }
}

void eng1_10bd0(Actor *p)
{
    long d;

    if ((grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->profile) & 0xffff) == 0x7fff)
        eng1_10cc0(p);
    eng1_interpret_signed_pairs(p);
    apply_frame_motion_2d(p);
    d = p->x - p->home_x;
    if (abs(d) < 0x10000)
        p->x = p->home_x;
    else if (d < 0)
        p->x += 0x10000;
    else
        p->x -= 0x10000;
    d = p->z - p->home_z;
    if (abs(d) < 0x10000)
        p->z = p->home_z;
    else if (d < 0)
        p->z += 0x10000;
    else
        p->z -= 0x10000;
    if (eng1_29e20(p)) {
        eng1_107c0(p);
        p->x = p->home_x;
        p->z = p->home_z;
    }
}

void eng1_10cc0(Actor *p)
{
    FixedPosition *home;

    p->direction_x = 0xff;
    p->direction_y = 0xff;
    p->direction_z = 0xff;
    p->counter_021 = 0x28;
    p->state = 7;
    p->motion_x_velocity = 0;
    p->motion_x_target = 0xf0000;
    p->motion_x_rate = 0x8000;
    p->motion_x_unknown = 0xf000;
    p->motion_y_velocity = 0;
    p->motion_y_target = 0x80000;
    p->motion_y_rate = 0x6000;
    p->motion_y_unknown = 0xf000;
    p->motion_z_velocity = 0;
    p->motion_z_target = 0xf0000;
    p->motion_z_rate = 0x8000;
    p->motion_z_unknown = 0xa000;
    home = (FixedPosition *)(g_kind_info[p->profile].owner + p->record_offset);
    if (home != 0) {
        p->home_x = ((random_limit(0x80) - 0x40) << 16) + home->x;
        p->home_y = 0x3500000;
        p->home_z = g_eng1_words_52790[g_eng1_cursor_721ac] << 16;
        g_eng1_cursor_721ac = (g_eng1_cursor_721ac + 1) & 0x1f;
    }
    select_frame(p, 0xd);
}

void eng1_10dc0(Actor *p)
{
    RelativeMeasure m;
    unsigned char r;

    eng1_interpret_signed_pairs(p);
    r = advance_target_motion(p, 7);
    eng1_2ad70((FixedPosition *)p, p, &m);
    if (m.distance < 0x20)
        eng1_10e40(p);
    if (p->state == 3) {
        if ((r & 1) && p->motion_x_velocity < 0x60000)
            select_frame(p, 0xe);
        p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
    }
}

void eng1_10e40(Actor *p)
{
    p->link_0e8 = 0;
    p->state = 8;
    select_frame(p, 0x15);
    p->counter_021 = 0;
    p->parameter = 0;
}

void eng1_10e70(Actor *p)
{
    Actor *t;
    Actor *q;
    long z, dx, dy;
    int i;

    eng1_interpret_signed_pairs(p);
    if (p->z < p->home_z)
        p->z += 0x10000;
    if (p->z > p->home_z)
        p->z -= 0x10000;
    if (p->counter_021) {
        p->counter_021--;
    } else {
        z = p->z;
        t = g_native_pointer_7217c;
        for (i = 0; i < 1; i++) {
            if ((z <= 0x400000 || t->z >= 0) && (z >= 0x400000 || t->z <= 0x800000)) {
                dx = (t->x - p->x) >> 16;
                if (abs(dx) <= 0x60) {
                    dy = (t->y - p->y) >> 20;
                    if (dy != 0)
                        p->unknown_09a = dx / dy;
                    else
                        p->unknown_09a = 0;
                    eng1_11180(p);
                    return;
                }
            }
        }
        t = find_tag(0x55);
        if (t != 0) {
            for (q = t->link_130; q != 0; q = q->link_130) {
                if (q->link_0e4 == 0 && (p->z <= 0x400000 || q->z >= 0) && (p->z >= 0x400000 || q->z <= 0x800000)) {
                    dx = (q->x - p->x) >> 16;
                    if (abs(dx) <= 0x60) {
                        dy = (q->y - p->y) >> 19;
                        if (dy != 0)
                            p->unknown_09a = dx / dy;
                        else
                            p->unknown_09a = 0;
                        p->link_0e8 = q;
                        q->link_0e4 = p;
                        eng1_11020(p);
                        return;
                    }
                }
            }
        }
        if (++p->parameter > 0x5a)
            eng1_10cc0(p);
    }
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
}

void eng1_11020(Actor *p)
{
    p->state = 9;
    select_frame(p, 0x18);
}

void eng1_11040(Actor *p)
{
    Actor *q;
    long d;

    if ((grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->profile) & 0xffff) == 0x7fff)
        eng1_10cc0(p);
    eng1_interpret_signed_pairs(p);
    q = p->link_0e8;
    if (q != 0) {
        p->y += 0x80000;
        p->x += p->unknown_09a << 16;
        d = (p->x - q->x) >> 16;
        if (abs(d) < 8) {
            p->x = q->x;
            p->z = q->z;
        }
        if (p->direction_x) {
            if (d < 0)
                p->direction_x = ~p->direction_x;
        } else if (d > 0) {
            p->direction_x = 0xff;
        }
    }
    if (eng1_2a3d0(p, q)) {
        p->y -= 0x80000;
        eng1_11290(p);
        q->state = 1;
        select_frame(q, 5);
        play_sound(q->unknown_09a, q);
    }
    if (eng1_29e20(p)) {
        eng1_108b0(p);
        q->link_0e4 = 0;
        p->link_0e8 = 0;
    }
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
}

void eng1_11180(Actor *p)
{
    p->state = 11;
    select_frame(p, 0x2c);
}

void eng1_111a0(Actor *p)
{
    long d;

    if ((grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->profile) & 0xffff) == 0x7fff)
        eng1_10cc0(p);
    eng1_interpret_signed_pairs(p);
    p->y += 0x100000;
    p->x += p->unknown_09a << 16;
    d = (p->x - g_native_pointer_7217c->x) >> 16;
    if (p->direction_x) {
        if (d >= 0)
            p->direction_x = ~p->direction_x;
    } else if (d < 0) {
        p->direction_x = 0xff;
    }
    if (eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
        eng1_native_code_target_00266a0(p, 3, 0);
        eng1_10cc0(p);
    }
    if (eng1_29e20(p))
        eng1_107c0(p);
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
}

void eng1_11290(Actor *p)
{
    select_frame(p, 8);
    p->state = 10;
    p->unknown_0f4 = 0;
    p->counter_021 = random_limit(0x3c) + 0x1e;
}

void eng1_112d0(Actor *p)
{
    eng1_interpret_signed_pairs(p);
    apply_frame_motion_2d(p);
    if (p->counter_021)
        p->counter_021--;
    else
        eng1_10a40(p);
}

void eng1_11300(Actor *p)
{
    FixedPosition *home;

    p->state = 12;
    p->motion_x_velocity = 0;
    p->motion_x_target = 0xf0000;
    p->motion_x_rate = 0xb000;
    p->motion_x_unknown = 0xa000;
    p->motion_y_velocity = 0;
    p->motion_y_target = 0x20000;
    p->motion_y_rate = 0x6000;
    p->motion_y_unknown = 0x4000;
    p->direction_y = 0xff;
    p->motion_z_velocity = 0;
    p->motion_z_target = 0xf0000;
    p->motion_z_rate = 0xb000;
    p->motion_z_unknown = 0xa000;
    p->direction_z = 0xff;
    home = (FixedPosition *)(g_kind_info[p->profile].owner + p->record_offset);
    if (home != 0) {
        p->home_x = home->x;
        p->home_y = 0x800000;
        p->home_z = home->z;
    }
    select_frame(p, 9);
}

void eng1_113c0(Actor *p)
{
    unsigned char r;

    eng1_interpret_signed_pairs(p);
    r = advance_target_motion(p, 7);
    if (p->state == 12 && (r & 1) && p->motion_x_velocity < 0x60000)
        select_frame(p, 0x2e);
    eng1_native_code_target_000ef10(p);
    if (eng1_native_code_target_002aa30(p, 0x40, 0x40)) {
        if (p->link_0e8 != 0) {
            p->link_0e8->link_0e4 = 0;
            p->link_0e8 = 0;
        }
        eng1_27410(p);
    } else if (p->y < 0x900000) {
        if (p->link_0e8 != 0) {
            p->link_0e8->link_0e4 = 0;
            p->link_0e8 = 0;
        }
        eng1_27410(p);
    }
}

void eng1_native_code_target_011450(Actor *p)
{
    Actor *t;
    long dx, adx;
    int r;

    switch (p->state) {
    case 1:
        if (p->z < 0x1000000) {
            t = g_native_pointer_7217c;
            if (abs((p->z - t->z) >> 16) < 0x10) {
                dx = (p->x - t->x) >> 16;
                adx = abs(dx);
                if (adx > 0x40) {
                    if (dx < 0 && p->direction_x == 0xff)
                        select_frame(p, 2);
                    else if (dx > 0 && p->direction_x == 0)
                        select_frame(p, 2);
                    else if (dx > 0x50 && random_limit(4) == 0)
                        eng1_108b0(p);
                } else if (random_limit(10) != 0) {
                    if (adx > 0x28)
                        select_frame(p, 5);
                    else
                        select_frame(p, 2);
                } else {
                    eng1_108b0(p);
                }
            } else {
                r = random_limit(0xe);
                if (r < 2)
                    select_frame(p, 2);
                if (r < 4)
                    select_frame(p, 0x21);
                if (r > 6)
                    eng1_108b0(p);
            }
        } else {
            r = random_limit(7);
            if (r == 1)
                select_frame(p, 0x21);
            if (r == 2)
                select_frame(p, 2);
            if (r > 3)
                eng1_108b0(p);
        }
        break;
    case 2:
        eng1_10900(p);
    case 3:
        if (p->counter_021 == 0 && random_limit(2) == 0)
            eng1_10cc0(p);
        break;
    }
}

void eng1_native_code_target_0115f0(Actor *p)
{
    advance_frame(p);
    p->unknown_124 = 0;
    p->direction_x = 0xff;
    p->render_flags |= 0x10;
    p->motion_x_velocity = 0;
    p->motion_x_target = 0x140000;
    p->motion_x_rate = 0xb000;
    p->motion_x_unknown = 0x4000;
    p->motion_y_velocity = 0;
    p->motion_y_target = 0x10000;
    p->motion_y_rate = 0x2000;
    p->motion_y_unknown = 0x2000;
    p->direction_y = 0xff;
    p->home_x = 0x22a00000;
    p->home_y = 0x800000;
    p->home_z = p->z;
    eng1_native_code_target_002a290(p);
}

void eng1_native_code_target_011690(Actor *p)
{
    Actor *t;

    p->render_flags &= ~8;
    if (p->state == 2) {
        advance_target_motion(p, 7);
        if (eng1_native_code_target_002aa30(p, 0x40, 0x40)) {
            eng1_27410(p);
            return;
        }
        if (p->y < 0x900000) {
            eng1_27410(p);
            return;
        }
    } else {
        p->y = 0x3a00000;
        t = g_native_pointer_7217c;
        if (abs((p->x - t->x) >> 16) < 0x200) {
            if (t->y < 0x3a00000) {
                if (p->state == 0)
                    select_frame(p, 4);
                p->y = g_native_pointer_7217c->y;
                eng1_native_code_target_000ef10(p);
            } else if (p->state == 1) {
                select_frame(p, 0);
            }
            if (g_eng1_word_6ffdc != 0) {
                g_configuration_dword_713bc[0x19] = 0;
                p->state = 2;
                select_frame(p, 5);
            }
        }
        if (eng1_2a710(&g_native_pointer_7217c->bounds, p))
            eng1_native_code_target_00266a0(p, 3, 0);
    }
    eng1_interpret_signed_pairs(p);
}

void eng1_native_code_target_0117b0(Actor *p)
{
    if (g_native_pointer_7217c->x > 0x29fe0000)
        g_native_pointer_7217c->x = 0x29fe0000;
}

/* 0x88..0x92 hold this emitter's radii and push vector (set by init_defaults_117d0). */
void eng1_native_code_target_011810(Actor *p)
{
    RelativeMeasure m;
    int inner, outer, vx, vy, vz;
    unsigned short d;

    outer = p->bounds.min_y;
    inner = p->bounds.min_x;
    vx = p->bounds.max_z;
    vy = p->bounds.max_x;
    vz = p->bounds.max_y;
    measure_relative_vector((FixedPosition *)p, (FixedPosition *)g_frame_global_734f4, &m);
    d = m.distance;
    if (d < outer) {
        if (d < inner) {
            g_pipeline_word_71ca0[++g_pipeline_word_71ca0[0]] = vx;
            g_pipeline_word_71720[++g_pipeline_word_71720[0]] = vy;
            g_pipeline_word_71740[++g_pipeline_word_71740[0]] = vz;
        } else {
            g_pipeline_word_71ca0[++g_pipeline_word_71ca0[0]] = vx - vx * d / outer;
            g_pipeline_word_71720[++g_pipeline_word_71720[0]] = vy - vy * d / outer;
            g_pipeline_word_71740[++g_pipeline_word_71740[0]] = vz - vz * d / outer;
        }
    }
    eng1_2ae50(p);
}

void eng1_native_code_target_011950(Actor *p)
{
    p->render_flags = 0;
    p->flags = 0x81;
    p->unknown_097 = 0x55;
    g_actor_worklist_716c0[++g_actor_worklist_716c0[0]] = (long)p;
    spawn_at_group_300c0(0x26a00000, 0x4600000, 0, 0x2029, 0);
}

void eng1_native_code_target_0119a0(Actor *p)
{
    int r;

    if (g_eng1_byte_719de == 3) {
        p->unknown_126 = count_chain(p);
        if (p->timer_030-- < 0) {
            r = random_limit(0xc);
            if (p->unknown_126 >= 0x14) {
                p->timer_030 = random_limit(0x14) + 0x14;
            } else {
                if (p->unknown_126 > 10)
                    p->timer_030 = random_limit(0x14) + 0x14;
                else if (p->unknown_126 > 5)
                    p->timer_030 = random_limit(0x14);
                else
                    p->timer_030 = random_limit(10);
                spawn_at_group_300c0(g_eng1_spawn_records_52100[r].x << 16, g_eng1_spawn_records_52100[r].y << 16,
                                     g_eng1_spawn_records_52100[r].z << 16, g_eng1_spawn_records_52100[r].kind, 0);
            }
        }
    }
}

void eng1_native_code_target_011a80(Actor *p)
{
    p->render_flags = 0;
    p->flags = 0x81;
    p->unknown_097 = 0x33;
    g_actor_worklist_716c0[++g_actor_worklist_716c0[0]] = (long)p;
    g_eng1_cursor_721ac = 0;
}

void eng1_native_code_target_011ac0(Actor *p)
{
    Actor *q;

    g_eng1_dword_721b0 = 0;
    if (g_eng1_setup_dword_72188 == 0 && g_eng1_word_6ffdc == 0) {
        if (g_native_pointer_7217c->x > 0x23000000 && g_native_pointer_7217c->x < 0x2a000000 &&
            g_native_pointer_7217c->y > 0x4000000)
            g_eng1_setup_dword_72188 = 2;
    } else if ((g_eng1_setup_dword_72188 == 1 || g_eng1_setup_dword_72188 == 4) && g_native_word_70832 == 0) {
        g_eng1_word_6ffdc = 0x63;
    }
    if (g_eng1_word_6ffdc != 0) {
        for (q = p->link_130; q != 0; q = q->link_130) {
            if (q->kind == 0x38) {
                if (q->state == 0xf)
                    eng1_27410(q);
                if (q->state < 0xc)
                    eng1_11300(q);
            }
        }
        eng1_27450(p);
        return;
    }
    for (q = p->link_130; q != 0; q = q->link_130)
        if (q->kind == 0x38 && q->state < 0xe)
            g_eng1_dword_721b0++;
    if (g_eng1_dword_721b0 < 3) {
        for (q = p->link_130; q != 0; q = q->link_130) {
            if (q->kind == 0x38 && q->state == 0xf) {
                q->hits = 3;
                eng1_10a40(q);
            }
        }
    }
}

void eng1_native_code_target_011c10(Actor *p)
{
    eng1_11d30(p);
    p->unknown_0f4 = random_limit(4) + 2;
    p->clut_01c = g_eng1_clut_rows_52b38[0][random_limit(8)];
    p->flags |= 0x80000;
    p->unknown_09a = (random_limit(2) + 0x146) | 0x5800;
}

void eng1_native_code_target_011c70(Actor *p)
{
    eng1_11d30(p);
    p->unknown_0f4 = random_limit(2) + 1;
    p->clut_01c = g_eng1_clut_rows_52b38[1][random_limit(8)];
    p->flags |= 0x80000;
    p->unknown_09a = (random_limit(2) + 0x144) | 0x5800;
}

void eng1_native_code_target_011cd0(Actor *p)
{
    eng1_11d30(p);
    p->unknown_0f4 = random_limit(2) + 2;
    p->clut_01c = g_eng1_clut_rows_52b38[2][random_limit(8)];
    p->flags |= 0x80000;
    p->unknown_09a = (random_limit(2) + 0x144) | 0x5800;
}

void eng1_11d30(Actor *p)
{
    Actor *t;

    t = find_tag(0x55);
    if (t != 0)
        append_chain(t, p);
    advance_frame(p);
    restart_steps(p);
    if (p->z & 0x800000)
        p->z += random_limit(0x30) << 16;
    else
        p->z -= random_limit(0x18) << 16;
    if (p->x < 0x25000000)
        p->direction_x = 0;
    else
        p->direction_x = 0xff;
    eng1_native_code_target_002a290(p);
    p->state = 0;
    p->direction_y = 0xff;
    p->motion_y_target = 0x60000;
    p->motion_y_velocity = 0;
    p->motion_y_rate = 0;
    p->motion_y_unknown = 0x6000;
}

void eng1_native_code_target_011df0(Actor *p)
{
    Offset2 pos;
    Actor *a;
    unsigned long w;

    eng1_interpret_signed_pairs(p);
    switch (p->state) {
    case 0:
        apply_frame_motion_2d(p);
        if (!eng1_29c20(p) ||
            ((w = g_configuration_dword_713bc[dispatch_local_2a170(p->x + (p->direction_x ? -0x100000 : 0x100000), p->y,
                                                                   (p->z + 0x400000) & 0xff800000, p->profile)]) & 4)) {
            p->x = p->saved_x;
            p->y = p->saved_y;
            select_frame(p, 2);
        }
        break;
    case 1:
        eng1_31000(p, &pos);
        a = p->link_0e4;
        if (a == 0) {
            p->state = 2;
            select_frame(p, 8);
            p->z = (p->z + 0x400000) & 0xf800000;
            if (p->z == 0 && p->y > 0x45e0000)
                p->y = 0x45e0000;
            if (p->z == 0x800000 && p->y > 0x3de0000)
                p->y = 0x3de0000;
            play_sound((p->unknown_09a + 1) & 0xfffe, p);
        } else {
            p->x = (pos.x << 16) + a->x;
            p->y = a->y - (pos.y << 16);
            p->z = a->z;
        }
        break;
    case 2:
        if (eng1_2bc50(p) & 1) {
            p->state = 3;
            p->z = (p->z + 0x400000) & 0xff800000;
            select_frame(p, 10);
        }
        break;
    }
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
    if (p->z < 0x7f0000) {
        if (p->x > 0x2a700000 || p->x < 0x22a00000)
            remove_actor(p);
    } else {
        if (p->x > 0x28f00000 || p->x < 0x24100000)
            remove_actor(p);
    }
}

void dispatch_local_11fc0(void)
{
    g_panel_70820.value_02 = 0xb9;
    g_eng1_setup_word_7219e = 0xb9;
    g_panel_70820.flash_0e = 0;
    g_panel_70820.value_00 = 0x7d;
    g_eng1_setup_word_7219c = 0x7d;
    g_eng1_setup_word_721a2 = 0;
    g_eng1_setup_word_721a4 = 0;
    g_eng1_setup_word_72194 = 0;
    g_eng1_setup_word_721a6 = 100;
    g_eng1_setup_word_721a0 = 0;
    g_eng1_setup_dword_6ef5c = 0;
    g_eng1_setup_dword_72188 = 0;
    g_engine_interface.context_004->value_022 = 0;
    memset(g_eng1_setup_clear_72140, 0, sizeof(g_eng1_setup_clear_72140));
}

void eng1_12040(void)
{
    int old;
    short a, b;

    if (g_eng1_setup_dword_6ef5c != 0 && --g_eng1_setup_dword_6ef5c <= 0) {
        g_eng1_setup_dword_6ef5c = 0;
        play_sound(0x14, 0);
    }
    if (g_eng1_setup_word_721a0 && g_eng1_setup_word_721a6 > 10)
        g_eng1_setup_word_721a6 = 10;
    if (g_eng1_setup_word_721a6 > 0x200)
        g_eng1_setup_word_721a6 = 0x200;
    if (--g_eng1_setup_word_721a6 == 0) {
        if (!g_eng1_setup_word_721a0 && g_eng1_setup_word_7219c != 0x7d) {
            g_eng1_setup_word_7219c = 0x7d;
            g_eng1_setup_word_721a4 = 0;
            g_eng1_setup_word_721a6 = 0x7f;
        } else {
            if (g_engine_interface.context_004->lives_00c > 9)
                g_engine_interface.context_004->lives_00c = 9;
            g_eng1_setup_word_7219c = g_eng1_life_targets_55330[g_engine_interface.context_004->lives_00c];
            g_eng1_setup_word_721a4 = 1;
            g_eng1_setup_word_721a6 = 0x7f;
            g_eng1_setup_word_721a0 = 0;
        }
    }
    if (eng1_sim_state_72108 != 0x2b && eng1_sim_state_72108 != 0x2c && eng1_sim_state_72108 != 0x2d &&
        eng1_sim_state_72110 == 0 && g_panel_70820.value_02 % 12 == 5) {
        old = g_eng1_setup_byte_6efb4;
        if (g_engine_interface.context_004->held_05c & g_engine_interface.context_004->previous_key_074) {
            g_eng1_setup_word_721a2 = 1;
            g_eng1_setup_dword_6ef5c = 0;
            do {
                if (--g_eng1_setup_byte_6efb4 < 0)
                    g_eng1_setup_byte_6efb4 = 4;
            } while (g_eng1_setup_byte_6efb4 != 0 && g_engine_interface.context_004->energy_024[g_eng1_setup_byte_6efb4] == 0);
        } else if (g_engine_interface.context_004->held_05c & g_engine_interface.context_004->next_key_076) {
            g_eng1_setup_word_721a2 = 0;
            g_eng1_setup_dword_6ef5c = 0;
            do {
                if (++g_eng1_setup_byte_6efb4 > 4)
                    g_eng1_setup_byte_6efb4 = 0;
            } while (g_eng1_setup_byte_6efb4 != 0 && g_engine_interface.context_004->energy_024[g_eng1_setup_byte_6efb4] == 0);
        }
        g_eng1_setup_word_7219e = g_eng1_weapon_targets_55310[g_eng1_setup_byte_6efb4];
        if (g_eng1_setup_byte_6efb4 != old && g_eng1_setup_byte_6efb4 == 4)
            g_eng1_setup_dword_6ef5c = 0x14;
        if (g_eng1_setup_byte_6efb4 != old)
            play_sound(0x27, 0);
    }
    if (--g_eng1_word_6ef50 & 1) {
        if (g_panel_70820.value_00 != g_eng1_setup_word_7219c) {
            if (g_eng1_setup_word_721a4) {
                g_panel_70820.value_00++;
                if (g_panel_70820.value_00 % 12 == 0)
                    g_panel_70820.value_00 = g_eng1_setup_word_7219c - 5;
            } else {
                g_panel_70820.value_00--;
                if (g_panel_70820.value_00 % 12 == 0)
                    g_panel_70820.value_00 = g_eng1_setup_word_7219c + 6;
            }
        }
    }
    if (g_panel_70820.value_02 != g_eng1_setup_word_7219e) {
        if (g_eng1_setup_word_721a2) {
            g_panel_70820.value_02++;
            if (g_panel_70820.value_02 % 12 == 0)
                g_panel_70820.value_02 = g_eng1_setup_word_7219e - 5;
        } else {
            g_panel_70820.value_02--;
            if (g_panel_70820.value_02 % 12 == 0)
                g_panel_70820.value_02 = g_eng1_setup_word_7219e + 6;
        }
    }
    if (g_eng1_setup_word_72194 != 0 && eng1_sim_state_72108 != 0x2a)
        g_eng1_setup_word_72194 = 0;
    a = (g_eng1_setup_word_72194 + 7) >> 4;
    if (a >= 0x10)
        a = 0x10;
    b = (g_panel_70820.flash_0e + 7) >> 4;
    if (b >= 0x10)
        b = 0x10;
    a -= b;
    if (a != 0) {
        if (a < 0) {
            g_panel_70820.flash_0e -= 0x10;
            if (g_panel_70820.flash_0e > 0xff)
                g_panel_70820.flash_0e = 0;
        } else {
            g_panel_70820.flash_0e += 0x10;
            if (g_panel_70820.flash_0e >= 0xff)
                g_panel_70820.flash_0e = 0xff;
        }
    }
    if (g_eng1_setup_byte_6efb4 != 0) {
        a = (g_engine_interface.context_004->energy_024[g_eng1_setup_byte_6efb4] + 7) >> 4;
        if (a >= 0x10)
            a = 0x10;
    } else {
        a = 0x10;
    }
    b = (g_panel_70820.flash_0c + 7) >> 4;
    if (b >= 0x10)
        b = 0x10;
    a -= b;
    if (a != 0) {
        if (a < 0) {
            g_panel_70820.flash_0c -= 0x10;
            if (g_panel_70820.flash_0c > 0xff)
                g_panel_70820.flash_0c = 0;
        } else {
            g_panel_70820.flash_0c += 0x10;
            if (g_panel_70820.flash_0c >= 0xff)
                g_panel_70820.flash_0c = 0xff;
        }
    }
    switch (g_eng1_setup_dword_72188) {
    case 0:
        g_panel_70820.x_18 = 0x154;
        g_panel_70820.mode_14 = 0;
        break;
    case 1:
    case 8:
        if (g_boss_threshold_72184 != 0) {
            g_panel_70820.target_12 -= g_boss_threshold_72184;
            if (g_panel_70820.target_12 < 0)
                g_panel_70820.target_12 = 0;
        }
        if (g_panel_70820.level_10 + g_panel_70820.target_12 == 0) {
            g_eng1_setup_dword_7218c = 0x19;
            g_boss_threshold_72184 = 0;
            g_eng1_setup_dword_72188 = g_eng1_setup_dword_72188 == 1 ? 4 : 7;
            break;
        }
        g_boss_threshold_72184 = 0;
        break;
    case 2:
        g_panel_70820.x_18 = 0x140;
        g_panel_70820.y_1a = 0xd0;
        g_eng1_setup_dword_72188 = 3;
        g_panel_70820.level_10 = 0;
        g_panel_70820.target_12 = 0;
        g_panel_70820.mode_14 = 1;
        g_boss_threshold_72184 = 0;
        break;
    case 3:
        g_panel_70820.x_18 -= 8;
        if (g_panel_70820.x_18 <= 0xd0) {
            g_panel_70820.x_18 = 0xd0;
            g_eng1_setup_dword_72188 = 1;
            g_panel_70820.target_12 = 0xff;
            g_boss_threshold_72184 = 0;
        }
        break;
    case 4:
        if (g_eng1_setup_dword_7218c != 0) {
            g_eng1_setup_dword_7218c--;
            break;
        }
        g_panel_70820.x_18 += 8;
        if (g_panel_70820.x_18 > 0x148)
            g_eng1_setup_dword_72188 = 0;
        break;
    case 5:
        g_eng1_setup_dword_72188 = 6;
        g_panel_70820.x_18 = 0xd0;
        g_panel_70820.y_1a = 0xd0;
        g_panel_70820.level_10 = 0;
        g_panel_70820.target_12 = 0;
        g_panel_70820.mode_14 = 2;
        g_boss_threshold_72184 = 0;
        g_panel_70820.fade_16 = 0;
        break;
    case 6:
        g_panel_70820.fade_16 += 4;
        if (g_panel_70820.fade_16 > 0x80) {
            g_panel_70820.fade_16 = 0x80;
            g_panel_70820.target_12 = 0xff;
            g_boss_threshold_72184 = 0;
            g_eng1_setup_dword_72188 = 8;
        }
        break;
    case 7:
        g_panel_70820.fade_16 -= 8;
        if (g_panel_70820.fade_16 <= 0) {
            g_eng1_setup_dword_72188 = 0;
            g_panel_70820.fade_16 = 0;
        }
        break;
    }
}
