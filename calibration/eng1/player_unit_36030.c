/* ENG1.DLL 0x36030..0x38090: level actor handlers (lift/boulder/debris/burst) and the
 * flying-player controller (setup 0x374c0, per-frame 0x379e0, damage 0x37e30). Lane x12 scratch, plain C.
 * Types/idioms follow calibration/eng1/status_panel_32b10.c (Actor view extended at 0x030/0x032/0x03e/0x0d8/
 * 0x0dc/0x110/0x120, Actor+0x108 signed; PanelContext extended; PendingActionGroup as in eng3/handlers_10370.c).
 * Appended to a copy of the status panel unit every function stays masked equal except 0x379e0 (operand order of
 * held_05c & fire_mask_06c, a context effect of the unit's real <stdlib.h>); see candidates/x12/result.json. */

typedef struct Actor Actor;
typedef struct ActorEntry {         /* 16-byte attached entry */
    unsigned char unknown_00[0x0c];
    unsigned short sprite;
    unsigned short tag;
} ActorEntry;
typedef struct Bounds {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds;
typedef struct FixedPosition { long x, y, z; } FixedPosition;
typedef struct RelativeMeasure {
    short x, y, z;
    short planar_distance;
    short distance;
    unsigned short direction;
} RelativeMeasure;
typedef union PendingActionGroup {  /* counted actor list at Actor+0x120: entries[1..count] */
    long count;
    Actor *entries[1];
} PendingActionGroup;
typedef struct BurstRecord {        /* 8-byte burst placement, 0x632b8 */
    short x, y, z;
    short kind;
} BurstRecord;
typedef struct LevelStart {         /* 24-byte start record: player start at +0 */
    unsigned short x;
    unsigned short y;
    unsigned char z;
    unsigned char unknown_05[0x18 - 0x05];
} LevelStart;
typedef struct DescriptorRow {
    void *pointer_000;
    unsigned long field_004;
    unsigned long field_008;
    LevelStart *starts;
    void *pointer_010;
    void *pointer_014;
} DescriptorRow;
typedef struct MiddleSlot { unsigned char bytes[0xec]; } MiddleSlot;
typedef struct ActorKindCallbacks {
    void (*init)(Actor *);
    void (*update)(Actor *);
    void (*start)(Actor *);
} ActorKindCallbacks;

typedef struct PanelContext {       /* view of g_engine_interface.context_004 */
    signed char level_000;
    signed char sublevel_001;
    unsigned char next_002;
    unsigned char state_003;
    unsigned char paused_004;
    unsigned char unknown_005;
    unsigned char mode_006;
    unsigned char unknown_007[0x0c - 0x07];
    signed char lives_00c;
    unsigned char unknown_00d;
    unsigned char difficulty_00e;
    unsigned char unknown_00f;
    unsigned char flag_010;
    unsigned char unknown_011[0x1e - 0x11];
    short target_01e;
    short value_020;
    short value_022;
    short energy_024[6];
    unsigned long cheats_030;
    unsigned char unknown_034[0x5c - 0x34];
    unsigned short held_05c;
    unsigned char unknown_05e[0x64 - 0x5e];
    unsigned short pressed_064;
    unsigned char unknown_066[0x6c - 0x66];
    unsigned short fire_mask_06c;
    unsigned char unknown_06e[0x1b30 - 0x6e];
    long unknown_1b30;
} PanelContext;

typedef struct EngineInterfaceView {
    unsigned long signature_000;
    PanelContext *context_004;
} EngineInterfaceView;

struct Actor {
    long x, y, z;
    short screen_x, screen_y, screen_z;
    unsigned char unknown_012[0x1c - 0x12];
    unsigned char clut_01c;
    unsigned char unknown_01d[0x1f - 0x1d];
    unsigned char status_01f;
    unsigned char unknown_020;
    unsigned char counter_021;
    unsigned char category;
    unsigned char mode;
    unsigned char unknown_024;
    unsigned char control_025;
    short unknown_026;
    unsigned char unknown_028[0x2a - 0x28];
    unsigned short state;
    unsigned char unknown_02c[0x2e - 0x2c];
    unsigned short kind;
    short unknown_030;
    short unknown_032;
    unsigned short sprite;
    unsigned short entry_count;
    unsigned short profile;
    unsigned short scale;
    unsigned char unknown_03c[0x3e - 0x3c];
    short unknown_03e;
    unsigned char unknown_040[0x4a - 0x40];
    unsigned short unknown_04a;
    long parameter;
    unsigned long flags;
    unsigned long render_flags;
    long record_offset;
    ActorEntry *entries;
    unsigned char unknown_060[0x64 - 0x60];
    void *image_064;
    unsigned char unknown_068[0x6c - 0x68];
    void *sound_06c;
    short angle_70;
    short angle_72;
    short angle_74;
    unsigned char unknown_076[0x88 - 0x76];
    Bounds bounds;
    unsigned char direction_x;
    unsigned char direction_y;
    unsigned char direction_z;
    unsigned char unknown_097;
    unsigned short unknown_098;
    short delay;
    long saved_x, saved_y, saved_z;
    long motion_x_velocity, motion_x_target, motion_x_rate, motion_x_unknown;
    long motion_y_velocity, motion_y_target, motion_y_rate, motion_y_unknown;
    long motion_z_velocity, motion_z_target, motion_z_rate, motion_z_unknown;
    long unknown_0d8;
    long unknown_0dc;
    long limit_0e0;
    unsigned char unknown_0e4[0xec - 0xe4];
    unsigned char lifetime;
    unsigned char unknown_0ed;
    short hits;
    unsigned char unknown_0f0[0xf4 - 0xf0];
    long unknown_0f4;
    unsigned char unknown_0f8[0x108 - 0xf8];
    long unknown_108;
    unsigned char unknown_10c[0x110 - 0x10c];
    void *unknown_110;
    short unknown_114;
    unsigned char unknown_116[0x11e - 0x116];
    short unknown_11e;
    PendingActionGroup *group_120;
    short unknown_124;
    unsigned char unknown_126[0x12c - 0x126];
    Actor *link_12c;
    Actor *link_130;
};

typedef struct PanelState {          /* 0x70820, 0x1c bytes */
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

extern EngineInterfaceView g_engine_interface;
extern PanelState g_panel_70820;
extern Actor *g_native_pointer_7217c;
extern Actor *g_frame_global_734f4;
extern Actor *g_frame_owner_72860;
extern MiddleSlot *g_frame_owner_73494;
extern Actor *g_eng1_setup_pointer_72020;
extern ActorKindCallbacks *g_configuration_dword_71370;
extern DescriptorRow eng1_descriptor_rows_5a690[];
extern unsigned long eng1_sim_state_72110;
extern unsigned short eng1_sim_state_72108;
extern unsigned short g_native_word_72168;
extern unsigned char g_eng1_setup_word_72012;
extern short g_camera_adjust_x_70d02, g_camera_adjust_y_70d04;
extern long g_camera_x_70ca0, g_camera_y_70ca4, g_camera_z_70ca8;
extern long g_camera_center_x_70cdc, g_camera_center_y_70ce0, g_camera_focal_70ce4;
extern unsigned char g_camera_iterations_70cfd;
extern unsigned char g_active_category_70cfe;
extern short g_camera_depth_low_70d0c, g_camera_depth_high_70d0e;
extern unsigned short g_camera_flags_70d10;
extern long g_camera_z_base_70d1c;
extern long g_eng1_setup_dword_6ef88;
extern short g_eng1_setup_word_6ef8c;
extern short g_eng1_setup_word_6ef90;
extern long g_eng1_setup_dword_6efac;
extern long g_resource_transition_ticks_6efb0;
extern signed char g_eng1_setup_byte_6efb4;
extern long g_eng1_setup_dword_717b4;
extern short g_eng1_setup_word_7183c;
extern short g_eng1_setup_word_71848;
extern short g_eng1_setup_word_718e0;
extern short g_eng1_setup_word_71a12;
extern short g_eng1_setup_word_71a14;
extern unsigned char g_eng1_setup_byte_72011;
extern long g_eng1_setup_dword_7201c;
extern long g_eng1_setup_dword_72114;
extern short g_eng1_setup_word_72158;
extern short g_eng1_setup_word_7216a;
extern long g_eng1_setup_dword_72180;
extern long g_eng1_setup_clear_72140[6];
extern unsigned char g_dispatch_key_7286c;
extern unsigned short g_dispatch_setting_4d858;
extern unsigned short g_dispatch_word_6ee80;
extern void *g_72874;
extern short g_weapon_timers_55320[];
extern long g_damage_table_55348[4][8];
extern BurstRecord g_burst_records_632b8[];
extern unsigned char g_eng1_flag_program_063318[];
extern unsigned char g_eng1_flag_program_063338[];
extern unsigned char g_player_program_63350[];
extern unsigned char g_player_program_63378[];
extern unsigned char g_player_program_633e8[];
extern unsigned char g_player_program_634b0[];
extern unsigned char g_player_program_634d8[];
extern unsigned char g_player_program_634e0[];
extern unsigned char g_player_table_63530[];
extern unsigned char g_player_palette_636f0[];
extern unsigned char g_player_palette_63a78[];
extern unsigned char g_player_palette_63ef8[];
extern short g_burst_done_6ffec;
extern long g_player_grace_6fff0;
extern Actor *g_player_glow_707dc;
extern void *g_player_palette_707e0;
extern Actor *g_burst_origin_707e4;
extern long g_player_cruise_707e8;
extern long g_player_flag_707f4;
extern long g_player_flag_707f8;
extern Actor *g_player_trail_707fc;

unsigned int random_limit(int limit);
unsigned int dispatch_engine_2c470(void);
void play_sound(unsigned short id, Actor *p);
Actor *spawn_at_group_300c0(long x, long y, long z, int kind, int group);
Actor *spawn_normal(long x, long y, long z, short kind);
Actor *attach_actor(Actor *p, int tag);
Actor *action_spawn_27d40(Actor *actor, unsigned long action);
void initialize_normal_actor(Actor *p);
void initialize_middle_actor(Actor *p);
int remove_actor(Actor *p);
int remove_actor_variant(Actor *p);
void release_actor(Actor *p);
void remove_counted_value(PendingActionGroup *g, Actor *p);
void reset_frame_state(Actor *p);
void select_frame(Actor *p, int frame);
int eng1_interpret_word_stream(Actor *p);
int eng1_interpret_signed_pairs(Actor *p);
void eng1_native_code_target_002a290(Actor *p);
int eng1_native_code_target_002b130(Actor *p);
int eng1_native_code_target_002aa30(Actor *p, int a, int b);
int eng1_2a9e0(Actor *p, int range);
int eng1_2bd20(Actor *p);
void eng1_2ae50(Actor *p);
unsigned long eng1_26de0(Actor *p);
void eng1_24980(Actor *p);
void eng1_30290(unsigned long hit, int b);
void eng1_1aba0(void);
void eng1_native_code_target_001a3e0(void);
void eng1_native_code_target_001a410(void);
void eng1_27410(Actor *p);
void eng1_27450(Actor *p);
void eng1_empty_c720(Actor *p);
void measure_absolute_vector(FixedPosition *a, FixedPosition *b, RelativeMeasure *m);
void eng1_color_rgb_forward_5a30(int r, int g, int b);
void eng1_tile_draw_2ff0(FixedPosition *xy);
void eng1_05c70(void *a, void *b, void *c, int d, int e);
void dispatch_local_15d00(void);
int dispatch_local_20ea0(short *bounds, int id, int mode, long x);
void dispatch_engine_2c4e0(unsigned long value);
void fx_clear_all_8117(void);
void resource_cancel(int id);
void resource_reset(int id);

void eng1_36030(Actor *p)
{
    p->render_flags |= 0x10000000;
    p->direction_y = 0;
    p->direction_x = p->state;
    p->motion_y_velocity = (p->motion_y_target = ((dispatch_engine_2c470() & 0x3ff) + 0x700) << 8) >> 2;
    p->motion_y_unknown = 0;
    p->motion_y_rate = random_limit(0x2000) + 0x8000;
    p->state = 1;
}

void eng1_360a0(Actor *p)
{
    Actor *a;

    p->flags = 0x41;
    p->render_flags |= 0x10000000;
    p->sprite = 0xb;
    a = attach_actor(p, 0x2028);
    if (a != 0) {
        a->x = p->x;
        a->y = p->y;
        a->z = p->z + 0x400000;
    }
}

void eng1_360f0(Actor *p)
{
    if (eng1_2a9e0(p, 0x140)) {
        release_actor(p);
    }
}

void set_mode_flags_36120(Actor *p)
{
    p->flags = 0x10000;
    p->render_flags |= 0x604;
    p->unknown_04a = 0x10;
    p->sprite = 0x18;
}

void eng1_36150(Actor *p)
{
    PendingActionGroup *g;
    int i;

    p->render_flags = 0;
    eng1_native_code_target_002a290(p);
    if (eng1_native_code_target_002b130(p)) {
        g = p->group_120;
        for (i = 0; i < g->count;) {
            g->entries[++i]->entries = (ActorEntry *)p;
        }
    }
    p->hits = 0x32;
}

void eng1_361a0(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;

    g = p->group_120;
    if (g == 0) {
        return;
    }
    switch (p->state) {
    case 0:
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->y < a->saved_y) {
                a->y += 0x10000;
            }
        }
        break;
    case 1:
        for (i = 0; i < g->count;) {
            g->entries[++i]->y -= 0x20000;
        }
        break;
    case 2:
        eng1_27450(p);
        spawn_at_group_300c0(p->x + 0x100000, p->y - 0x700000, p->z, 0x2027, 0);
        spawn_at_group_300c0(p->x - 0x180000, p->y - 0xb40000, p->z - 0x100000, 0x2027, 0);
        spawn_at_group_300c0(p->x, p->y - 0x200000, p->z - 0x200000, 0x2027, 0);
        a = spawn_at_group_300c0(((random_limit(0x20) - 0x10) << 16) + p->x, p->y - (random_limit(0x20) << 16) - 0x400000,
                                 p->z - 0x200000, 0x2027, 0);
        if (a != 0) {
            a->parameter = random_limit(8) + 4;
        }
        a = spawn_at_group_300c0(((random_limit(0x20) - 0x10) << 16) + p->x, p->y - (random_limit(0x20) << 16) - 0x600000,
                                 p->z - 0x200000, 0x2027, 0);
        if (a != 0) {
            a->parameter = random_limit(0xc) + 6;
        }
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            a->state = 1;
            if (g_native_pointer_7217c->x < a->x) {
                a->direction_x = 0;
            } else {
                a->direction_x = 0xff;
            }
            a->flags &= ~0x800;
            a->saved_y = a->y;
        }
        break;
    }
    p->state = 0;
    if (g->count == 0) {
        remove_actor(p);
    }
}

void eng1_363e0(Actor *p)
{
    p->render_flags |= 0x10000000;
    p->direction_y = 0xff;
    p->direction_z = 0xff;
    p->flags |= 0x120800;
    p->unknown_0d8 = 0x1a;
    switch (p->state) {
    case 0:
        p->motion_y_velocity = p->motion_y_target = ((dispatch_engine_2c470() & 0x1ff) + 0x400) << 8;
        p->motion_y_unknown = random_limit(0x2000) + 0x5000;
        p->motion_z_target = 0x60000;
        p->motion_z_velocity = 0x18000;
        p->motion_z_rate = 0x2800;
        p->motion_x_target = 0x70000;
        p->motion_x_velocity = 0x1c000;
        p->motion_x_rate = 0x3000;
        p->motion_x_unknown = 0;
        p->limit_0e0 = 0x10;
        p->unknown_0dc = 0;
        break;
    case 1:
        p->motion_y_velocity = p->motion_y_target = ((dispatch_engine_2c470() & 0x1ff) + 0x400) << 8;
        p->motion_y_unknown = random_limit(0x2000) + 0x5000;
        p->motion_z_target = 0x50000;
        p->motion_z_velocity = 0x14000;
        p->motion_z_rate = 0x2800;
        p->motion_x_target = 0x70000;
        p->motion_x_velocity = 0x1c000;
        p->motion_x_rate = 0x3000;
        p->motion_x_unknown = 0;
        p->limit_0e0 = 8;
        p->unknown_0dc = 0xc;
        break;
    case 2:
        p->motion_y_velocity = p->motion_y_target = ((dispatch_engine_2c470() & 0x1ff) + 0x400) << 8;
        p->motion_y_unknown = random_limit(0x2000) + 0x5000;
        p->motion_z_target = 0x40000;
        p->motion_z_velocity = 0x10000;
        p->motion_z_rate = 0x2800;
        p->motion_x_target = 0x70000;
        p->motion_x_velocity = 0x1c000;
        p->motion_x_rate = 0x3000;
        p->motion_x_unknown = 0;
        p->limit_0e0 = 4;
        p->unknown_0dc = 0x10;
        break;
    }
    p->state = 0;
}

void eng1_367b0(Actor *p);
void eng1_36990(Actor *p);

void eng1_365f0(Actor *p)
{
    int remove;
    Actor *owner;
    unsigned long hit;
    int n;

    remove = 0;
    owner = (Actor *)p->entries;
    if (owner == 0) {
        remove_actor_variant(p);
        return;
    }
    if (p->state != 0) {
        p->angle_70 += p->limit_0e0;
        p->angle_72 += p->unknown_0dc;
        if (eng1_2bd20(p) & 1) {
            play_sound(0xd150, p);
            eng1_24980(p);
            remove = 1;
        }
    } else {
        hit = eng1_26de0(p);
        if (hit) {
            if (hit == 0x80) {
                n = g_eng1_setup_word_72012 & 0x5f & ~g_native_word_72168;
                if (n) {
                    eng1_1aba0();
                    g_native_word_72168 |= n;
                    if ((g_eng1_setup_word_72012 & 2) && g_panel_70820.flash_0e >= 0xc8) {
                        eng1_36990(owner);
                        play_sound(0xd150, p);
                        owner->state = 2;
                    } else {
                        eng1_367b0(owner);
                        if (--owner->hits > 0) {
                            play_sound(0xd152, p);
                            owner->state = 1;
                        } else {
                            play_sound(0xd150, p);
                            owner->state = 2;
                        }
                    }
                }
            } else {
                eng1_30290(hit, 0);
                if (p->y == p->saved_y) {
                    eng1_367b0(owner);
                    if (--owner->hits <= 0) {
                        play_sound(0xd150, p);
                        owner->state = 2;
                    } else {
                        play_sound(0xd152, p);
                        owner->state = 1;
                    }
                }
            }
        }
    }
    if (p->z < -0xb40000) {
        remove = 1;
    }
    if (p->y > 0x5800000) {
        remove = 1;
    }
    if (eng1_2a9e0(p, 0x100)) {
        remove = 1;
    }
    if (remove) {
        if (owner->group_120 != 0) {
            remove_counted_value(owner->group_120, p);
        }
        remove_actor_variant(p);
    }
}

void eng1_367b0(Actor *p)
{
    int n;
    int i;
    long x, y, z;
    Actor *a;

    play_sound(0xd151, p);
    n = random_limit(6) + 6;
    for (i = 0; i < n; i++) {
        x = (random_limit(0x20) - 0x10) << 16;
        y = (random_limit(0x60) + 0x30) << 16;
        z = (random_limit(0x20) - 0x10) << 16;
        a = spawn_at_group_300c0(p->x + x, p->y - y, p->z + z, 0x4028, 0);
        if (a != 0) {
            a->render_flags |= 0x10000000;
            if (random_limit(2)) {
                a->sprite = 9;
            } else {
                a->sprite = 10;
            }
            a->scale = random_limit((0x20 - n) * 4) + 0x40 - n * 4;
            a->direction_x = -random_limit(2);
            a->direction_z = -random_limit(2);
            a->direction_y = 0xff;
            a->motion_y_velocity = a->motion_y_target = ((dispatch_engine_2c470() & 0x1ff) + 0x800) << 8;
            a->motion_y_rate = 0;
            a->motion_y_unknown = random_limit(0x2000) + 0x4000;
            a->motion_z_velocity = a->motion_z_target = ((dispatch_engine_2c470() & 0xff) + 0x200) << 8;
            a->motion_z_rate = random_limit(0x1000) + 0x2000;
            a->motion_z_unknown = 0;
            a->motion_x_velocity = a->motion_x_target = ((dispatch_engine_2c470() & 0x1ff) + 0x100) << 8;
            a->motion_x_rate = random_limit(0x1000) + 0x1000;
            a->motion_x_unknown = 0;
            a->state = 0;
        }
    }
}

void eng1_36990(Actor *p)
{
    int n;
    int i;
    long x, y, z;
    Actor *a;

    n = random_limit(0xc) + 0xc;
    for (i = 0; i < n; i++) {
        x = (random_limit(0x20) - 0x10) << 16;
        y = random_limit(0x80) << 16;
        z = (random_limit(0x20) - 0x10) << 16;
        a = spawn_at_group_300c0(p->x + x, p->y - y, p->z + z, 0x4028, 0);
        if (a != 0) {
            a->render_flags |= 0x10000000;
            if (random_limit(2)) {
                a->sprite = 9;
            } else {
                a->sprite = 10;
            }
            a->scale = random_limit(0xa0 - n * 3) + 0x40 - n * 2;
            a->direction_x = -random_limit(2);
            a->direction_z = -random_limit(2);
            a->direction_y = 0xff;
            a->motion_y_velocity = a->motion_y_target = ((dispatch_engine_2c470() & 0x1ff) + 0x800) << 8;
            a->motion_y_rate = 0;
            a->motion_y_unknown = random_limit(0x2000) + 0x4000;
            a->motion_z_velocity = a->motion_z_target = ((dispatch_engine_2c470() & 0xff) + 0x200) << 8;
            a->motion_z_rate = random_limit(0x1000) + 0x2000;
            a->motion_z_unknown = 0;
            a->motion_x_velocity = a->motion_x_target = ((dispatch_engine_2c470() & 0x1ff) + 0x100) << 8;
            a->motion_x_rate = random_limit(0x1000) + 0x1000;
            a->motion_x_unknown = 0;
            a->state = 0;
        }
    }
}

void reset_extent_36b60(Actor *p)
{
    p->sprite = 0;
    p->flags = 2;
    p->bounds.min_x = 0xa0;
    p->bounds.min_y = 0xc8;
}

void eng1_36b90(Actor *p)
{
    RelativeMeasure m;
    int hi;
    int lo;
    int d;

    hi = p->bounds.min_y;
    lo = p->bounds.min_x;
    measure_absolute_vector((FixedPosition *)p, (FixedPosition *)g_frame_global_734f4, &m);
    if (m.z <= 0x40 && m.y <= 0x80) {
        d = (unsigned short)m.planar_distance;
        if (d < hi) {
            if (d < lo) {
                g_camera_adjust_y_70d04 = -0x20;
            } else {
                g_camera_adjust_y_70d04 = ((lo - d) << 5) / (hi - lo) - 0x20;
            }
        } else {
            g_camera_adjust_y_70d04 = -0x40;
        }
        eng1_2ae50(p);
        if (eng1_native_code_target_002aa30(p, hi + 0x80, hi + 0x80)) {
            release_actor(p);
        }
    }
}

void eng1_36c40(void)
{
    FixedPosition xy;

    xy.x = g_camera_x_70ca0 >> 2;
    xy.y = (g_camera_y_70ca4 - 0x250) / 5;
    if (xy.y < 0x50) {
        eng1_color_rgb_forward_5a30(0xa1, 0xbc, 0xbb);
    } else {
        eng1_color_rgb_forward_5a30(0x11, 0x1e, 0x39);
    }
    eng1_tile_draw_2ff0(&xy);
}

void eng1_36cb0(Actor *p)
{
    p->z = 0x800000;
    select_frame(p, 0);
    eng1_interpret_signed_pairs(p);
}

void eng1_36ce0(Actor *p)
{
    p->x = g_frame_owner_72860->x;
    p->y = g_frame_owner_72860->y;
    if (g_frame_owner_72860->state != 0x38) {
        eng1_interpret_signed_pairs(p);
    }
}

void eng1_36d10(Actor *p)
{
    p->render_flags = 0xc0000000;
    p->mode = 6;
    p->unknown_03e = 0x4b7;
    p->state = 2;
    reset_frame_state(p);
    p->motion_x_velocity = (0xa00000 - p->x) / 5;
    p->motion_y_velocity = 0x2e0000;
    p->unknown_108 = 5;
    p->scale = 0xe0;
}

void eng1_36e90(Actor *p);

void eng1_36d70(Actor *p)
{
    switch (p->state) {
    case 2:
        p->scale += 0x37;
        if (p->direction_x == 0) {
            p->x += p->motion_x_velocity;
        } else {
            p->x -= p->motion_x_velocity;
        }
        p->y += p->motion_y_velocity;
        eng1_interpret_word_stream(p);
        break;
    }
    if (--p->unknown_108 < 0) {
        eng1_36e90(p);
        remove_actor(p);
    }
}

void eng1_36df0(Actor *p)
{
    reset_frame_state(p);
    p->unknown_108 = p->y;
}

void eng1_36e10(Actor *p)
{
    p->motion_y_velocity += 0x8000;
    if (p->direction_x == 0) {
        p->x += p->motion_x_velocity;
    } else {
        p->x -= p->motion_x_velocity;
    }
    p->y += p->motion_y_velocity;
    p->z += p->motion_z_velocity;
    p->x += p->motion_x_velocity;
    p->y += p->motion_y_velocity;
    p->z += p->motion_z_velocity;
    eng1_interpret_word_stream(p);
    eng1_empty_c720(p);
    if (p->y > p->unknown_108) {
        remove_actor(p);
    }
}

void eng1_36e90(Actor *p)
{
    long x, y, z;
    int i;
    Actor *a;

    play_sound(0x252, 0);
    x = g_burst_origin_707e4->x + 0x300000;
    y = g_burst_origin_707e4->y + 0x500000;
    z = g_burst_origin_707e4->z + 0x17c0000;
    for (i = 0; i < 6; i++) {
        a = spawn_normal((g_burst_records_632b8[i].x << 16) + x, (g_burst_records_632b8[i].y << 16) + y,
                         (g_burst_records_632b8[i].z << 16) + z, g_burst_records_632b8[i].kind);
        if (a != 0) {
            g_configuration_dword_71370[0x3a].init(a);
            a->state = 2;
            select_frame(a, i & 7);
            eng1_interpret_signed_pairs(a);
            a->direction_x = 0;
            a->motion_x_velocity = (0x200 - random_limit(0x400)) << 8;
            a->motion_y_velocity = (-random_limit(0x180) - 0xc00) << 8;
            a->motion_z_velocity = -0x20000;
        }
    }
    for (i = 0; i < 12; i++) {
        a = spawn_normal((g_burst_records_632b8[i].x << 17) + x, (g_burst_records_632b8[i].y << 17) + y,
                         (g_burst_records_632b8[i].z << 17) + z, g_burst_records_632b8[i].kind);
        if (a != 0) {
            g_configuration_dword_71370[0x3a].init(a);
            a->state = 0x2e;
            select_frame(a, i & 7);
            eng1_interpret_signed_pairs(a);
            a->direction_x = 0;
            a->motion_x_velocity = (0x600 - random_limit(0xc00)) << 8;
            a->motion_y_velocity = (-random_limit(0x200) - 0x200) << 8;
            a->motion_z_velocity = -0x10000;
        }
    }
    g_burst_done_6ffec = 1;
}

void eng1_37080(void)
{
    if (eng1_sim_state_72110 == 0 && g_frame_owner_72860->state != 0x23 &&
        g_engine_interface.context_004->state_003 != 1) {
        g_engine_interface.context_004->energy_024[0] = 0;
        play_sound(0x601c, g_frame_owner_72860);
        g_frame_owner_72860->state = 0x23;
        g_frame_owner_72860->unknown_108 = 3;
        g_engine_interface.context_004->sublevel_001 = g_engine_interface.context_004->next_002;
        eng1_sim_state_72110++;
        g_engine_interface.context_004->unknown_005 = 1;
        if (g_engine_interface.context_004->lives_00c > 0) {
            g_engine_interface.context_004->lives_00c--;
        }
        if (g_engine_interface.context_004->cheats_030 & 8) {
            g_engine_interface.context_004->lives_00c++;
        }
        if (g_engine_interface.context_004->lives_00c <= 0) {
            g_engine_interface.context_004->state_003 = 1;
        }
        select_frame(g_frame_owner_72860, 8);
        eng1_interpret_signed_pairs(g_frame_owner_72860);
        g_player_glow_707dc->image_064 = g_player_program_634b0;
        reset_frame_state(g_player_glow_707dc);
        g_player_trail_707fc->image_064 = g_player_program_634d8;
        reset_frame_state(g_player_trail_707fc);
        eng1_interpret_word_stream(g_player_trail_707fc);
    }
}

void eng1_371a0(void)
{
    if (g_eng1_setup_pointer_72020->kind != 0) {
        if (g_eng1_setup_clear_72140[g_eng1_setup_byte_6efb4] == 0) {
            g_engine_interface.context_004->energy_024[g_eng1_setup_byte_6efb4]--;
            if (g_engine_interface.context_004->energy_024[g_eng1_setup_byte_6efb4] <= 0) {
                g_engine_interface.context_004->energy_024[g_eng1_setup_byte_6efb4] = 0;
                g_eng1_setup_byte_6efb4 = 0;
                eng1_native_code_target_001a410();
                return;
            }
        }
        g_eng1_setup_pointer_72020->x = g_native_pointer_7217c->x + 0x340000;
        g_eng1_setup_pointer_72020->y = g_native_pointer_7217c->y - 0x140000;
        g_eng1_setup_pointer_72020->angle_74 = 0x300;
    }
}

void eng1_37230(void)
{
    g_frame_owner_72860->state = 2;
    select_frame(g_frame_owner_72860, 0);
    eng1_interpret_signed_pairs(g_frame_owner_72860);
    g_player_glow_707dc->image_064 = g_eng1_flag_program_063318;
    reset_frame_state(g_player_glow_707dc);
    eng1_interpret_word_stream(g_player_glow_707dc);
    g_player_trail_707fc->image_064 = g_eng1_flag_program_063338;
    reset_frame_state(g_player_trail_707fc);
    eng1_interpret_word_stream(g_player_trail_707fc);
}

void eng1_372b0(void)
{
    g_frame_owner_72860->state = 2;
    select_frame(g_frame_owner_72860, 0);
    eng1_interpret_signed_pairs(g_frame_owner_72860);
    g_frame_owner_72860->unknown_032 = g_player_trail_707fc->unknown_032 - 1;
}

void eng1_37300(Actor *p)
{
    if (p == g_player_glow_707dc) {
        p->image_064 = g_eng1_flag_program_063318;
    } else {
        p->image_064 = g_eng1_flag_program_063338;
    }
    p->x = g_frame_owner_72860->x;
    p->y = g_frame_owner_72860->y;
    p->z = g_frame_owner_72860->z;
    p->scale = 0x100;
    p->category = 0x30;
    reset_frame_state(p);
    eng1_interpret_word_stream(p);
    p->mode = 5;
    p->flags |= 0x20000;
    p->unknown_04a = 0x80;
    p->direction_x = g_native_pointer_7217c->direction_x;
    p->direction_y = 0xff;
}

void eng1_37390(Actor *p)
{
    p->x = g_frame_owner_72860->x;
    p->y = g_frame_owner_72860->y;
    p->z = g_frame_owner_72860->z;
    if (g_frame_owner_72860->state == 0x23) {
        if (eng1_interpret_word_stream(p)) {
            g_player_glow_707dc->image_064 = g_player_program_634e0;
            reset_frame_state(g_player_glow_707dc);
            eng1_interpret_word_stream(g_player_glow_707dc);
        }
    } else {
        eng1_interpret_word_stream(p);
    }
}

void eng1_37400(Actor *p)
{
    p->x = g_frame_owner_72860->x;
    p->y = g_frame_owner_72860->y;
    p->z = g_frame_owner_72860->z;
    p->render_flags |= 0x2000;
    p->unknown_03e = g_frame_owner_72860->screen_z - 2;
    if (g_frame_owner_72860->state != 0x23) {
        eng1_interpret_word_stream(p);
    }
}

void eng1_37460(void)
{
    initialize_middle_actor(g_player_glow_707dc);
    initialize_middle_actor(g_player_trail_707fc);
    g_player_glow_707dc->kind = 0x4028;
    g_player_trail_707fc->kind = 0x4029;
    eng1_37300(g_player_glow_707dc);
    eng1_37300(g_player_trail_707fc);
}

void eng1_374c0(void)
{
    Actor *player;
    LevelStart *start;
    void *palette;
    Actor *a;

    player = g_frame_owner_72860;
    eng1_sim_state_72110 = 0;
    g_eng1_setup_dword_6ef88 = 0;
    g_eng1_setup_word_6ef8c = 0;
    g_eng1_setup_dword_717b4 = 0;
    g_eng1_setup_word_6ef90 = 0;
    g_eng1_setup_dword_6efac = 0;
    g_resource_transition_ticks_6efb0 = 0;
    g_eng1_setup_byte_6efb4 = 0;
    g_native_pointer_7217c = player;
    initialize_normal_actor(player);
    g_engine_interface.context_004->unknown_005 = 0;
    g_engine_interface.context_004->paused_004 = 0;
    g_eng1_setup_word_72158 = 0;
    g_eng1_setup_dword_72114 = 0;
    g_native_pointer_7217c->kind = 1;
    g_frame_owner_72860->category = 1;
    g_eng1_setup_byte_72011 = 0;
    g_eng1_setup_dword_7201c = 0;
    g_eng1_setup_dword_72180 = g_native_pointer_7217c->motion_x_target;
    g_native_pointer_7217c->render_flags |= 0x600;
    g_native_pointer_7217c->unknown_04a = 0xe0;
    g_camera_flags_70d10 |= 0x40;
    g_eng1_setup_word_7216a = 1;
    g_eng1_setup_word_7183c = 0;
    g_eng1_setup_word_71a12 = 0;
    g_eng1_setup_word_71a14 = 0;
    g_eng1_setup_word_71848 = 0;
    g_camera_depth_low_70d0c = 0x20;
    g_camera_depth_high_70d0e = 0xd8;
    start = &eng1_descriptor_rows_5a690[g_engine_interface.context_004->level_000]
                 .starts[g_engine_interface.context_004->sublevel_001];
    player->x = start->x << 16;
    player->y = start->y << 16;
    player->z = start->z << 23;
    g_camera_x_70ca0 = start->x;
    g_camera_y_70ca4 = start->y;
    g_camera_z_70ca8 = 0;
    g_active_category_70cfe = 0;
    g_camera_adjust_x_70d02 = 0x3c;
    g_camera_adjust_y_70d04 = -0x40;
    g_camera_center_x_70cdc = 0xa0;
    g_camera_center_y_70ce0 = 0x80;
    g_camera_focal_70ce4 = 0x140;
    g_camera_z_base_70d1c = 0;
    g_camera_iterations_70cfd = 2;
    g_camera_flags_70d10 = 0x5e;
    g_eng1_setup_word_718e0 = 0;
    g_eng1_setup_pointer_72020 = (Actor *)&g_frame_owner_73494[0];
    g_player_flag_707f8 = 0;
    g_player_glow_707dc = (Actor *)&g_frame_owner_73494[3];
    g_player_trail_707fc = (Actor *)&g_frame_owner_73494[4];
    g_frame_owner_72860->render_flags |= 0xc000000;
    eng1_sim_state_72108 = 0x28;
    g_frame_owner_72860->image_064 = g_player_program_633e8;
    g_frame_owner_72860->unknown_030 = -1;
    g_frame_owner_72860->unknown_032 = -1;
    dispatch_local_15d00();
    g_native_pointer_7217c->category = 0x30;
    g_frame_owner_72860->unknown_110 = g_player_table_63530;
    eng1_37230();
    eng1_37460();
    if (g_engine_interface.context_004->next_002 == 2) {
        palette = g_player_palette_63ef8;
    } else if (g_engine_interface.context_004->next_002 == 1) {
        palette = g_player_palette_636f0;
    } else {
        palette = g_player_palette_63a78;
    }
    g_player_palette_707e0 = palette;
    eng1_05c70(palette, palette, g_72874, 0x100, 2);
    a = action_spawn_27d40(g_native_pointer_7217c, 0xa007);
    if (a != 0) {
        a->unknown_04a = 0x80;
        a->render_flags |= 0x4000605;
        a->category = 0x30;
        a->sprite = 0;
        g_frame_owner_72860->render_flags &= ~0x800000;
    }
}

void eng1_37770(void)
{
    short bounds[8];

    dispatch_local_20ea0(bounds, 1, 8, g_frame_owner_72860->x >> 16);
    if (g_frame_owner_72860->y > (bounds[2] - 0x40) << 16) {
        g_frame_owner_72860->y = (bounds[2] - 0x40) << 16;
    }
    dispatch_local_20ea0(bounds, 0, 8, g_frame_owner_72860->x >> 16);
    if (g_frame_owner_72860->y < bounds[0] << 16) {
        g_frame_owner_72860->y = bounds[0] << 16;
    }
}

void eng1_377f0(void)
{
    if (g_frame_owner_72860->state == 0x23 && g_frame_owner_72860->unknown_114 != 8) {
        g_frame_owner_72860->x -= 0x20000;
        g_frame_owner_72860->y += 0x30000;
        return;
    }
    if ((g_engine_interface.context_004->held_05c & 0x40) && g_native_pointer_7217c->motion_y_velocity < 0x78000) {
        g_native_pointer_7217c->motion_y_velocity += 0x20000;
    }
    if ((g_engine_interface.context_004->held_05c & 0x10) && g_native_pointer_7217c->motion_y_velocity > -0x78000) {
        g_native_pointer_7217c->motion_y_velocity -= 0x20000;
    }
    eng1_37770();
    if (g_native_pointer_7217c->motion_y_velocity > 0) {
        if (!(g_engine_interface.context_004->held_05c & 0x10)) {
            g_native_pointer_7217c->motion_y_velocity -= 0x10000;
            if (g_native_pointer_7217c->motion_y_velocity < 0) {
                g_native_pointer_7217c->motion_y_velocity = 0;
            }
        }
    } else if (!(g_engine_interface.context_004->held_05c & 0x40)) {
        g_native_pointer_7217c->motion_y_velocity += 0x10000;
        if (g_native_pointer_7217c->motion_y_velocity > 0) {
            g_native_pointer_7217c->motion_y_velocity = 0;
        }
    }
    if ((g_engine_interface.context_004->held_05c & 0x20) &&
        g_native_pointer_7217c->motion_x_velocity < g_player_cruise_707e8 + 0x78000) {
        g_native_pointer_7217c->motion_x_velocity += 0x20000;
    }
    if ((g_engine_interface.context_004->held_05c & 0x80) &&
        g_native_pointer_7217c->motion_x_velocity > g_player_cruise_707e8 - 0x78000) {
        g_native_pointer_7217c->motion_x_velocity -= 0x20000;
    }
    if (g_native_pointer_7217c->motion_x_velocity > g_player_cruise_707e8) {
        if (!(g_engine_interface.context_004->held_05c & 0x80)) {
            g_native_pointer_7217c->motion_x_velocity -= 0x10000;
            if (g_native_pointer_7217c->motion_x_velocity < g_player_cruise_707e8) {
                g_native_pointer_7217c->motion_x_velocity = g_player_cruise_707e8;
            }
        }
    } else if (!(g_engine_interface.context_004->held_05c & 0x20)) {
        g_native_pointer_7217c->motion_x_velocity += 0x10000;
        if (g_native_pointer_7217c->motion_x_velocity > g_player_cruise_707e8) {
            g_native_pointer_7217c->motion_x_velocity = g_player_cruise_707e8;
        }
    }
    g_native_pointer_7217c->x += g_native_pointer_7217c->motion_x_velocity;
    g_native_pointer_7217c->y += g_native_pointer_7217c->motion_y_velocity;
}

void eng1_379e0(void)
{
    int done;

    if (g_player_grace_6fff0 > 0) {
        g_player_grace_6fff0--;
    }
    eng1_377f0();
    done = eng1_interpret_signed_pairs(g_frame_owner_72860);
    switch (g_frame_owner_72860->state) {
    case 0x24:
        if (done) {
            eng1_37230();
            g_player_grace_6fff0 = 0xf;
        }
        break;
    case 0x23:
        if (done) {
            if (g_frame_owner_72860->unknown_108 == 2) {
                g_dispatch_key_7286c = 0xf8;
                g_dispatch_setting_4d858 = 0x80;
                g_dispatch_word_6ee80 = 0;
                dispatch_engine_2c4e0(1);
                g_engine_interface.context_004->unknown_1b30 = 4;
                fx_clear_all_8117();
            }
            if (g_frame_owner_72860->unknown_108-- <= 0) {
                if (g_engine_interface.context_004->lives_00c == 0) {
                    g_engine_interface.context_004->state_003 = 0x22;
                } else {
                    g_engine_interface.context_004->state_003 = 0x21;
                }
            }
        }
        break;
    case 2:
        if (g_engine_interface.context_004->fire_mask_06c & g_engine_interface.context_004->held_05c) {
            switch (g_eng1_setup_byte_6efb4) {
            case 0:
                select_frame(g_frame_owner_72860, 2);
                eng1_interpret_signed_pairs(g_frame_owner_72860);
                g_frame_owner_72860->state = 0x48;
                break;
            case 1:
                select_frame(g_frame_owner_72860, 3);
                eng1_interpret_signed_pairs(g_frame_owner_72860);
                g_frame_owner_72860->state = 0x49;
                break;
            case 2:
                select_frame(g_frame_owner_72860, 6);
                eng1_interpret_signed_pairs(g_frame_owner_72860);
                g_frame_owner_72860->state = 0x4c;
                break;
            }
        }
        break;
    case 0x49:
        if (done) {
            select_frame(g_frame_owner_72860, 4);
            eng1_interpret_signed_pairs(g_frame_owner_72860);
            g_frame_owner_72860->state = 0x4a;
            eng1_native_code_target_001a3e0();
            eng1_371a0();
        }
        break;
    case 0x4a:
        if ((g_engine_interface.context_004->fire_mask_06c & g_engine_interface.context_004->held_05c) &&
            g_eng1_setup_byte_6efb4 == 1) {
            eng1_371a0();
        } else {
            select_frame(g_frame_owner_72860, 5);
            eng1_interpret_signed_pairs(g_frame_owner_72860);
            g_frame_owner_72860->state = 0x4b;
            eng1_native_code_target_001a410();
        }
        break;
    case 0x48:
    case 0x4b:
        if (done) {
            eng1_372b0();
        }
        break;
    case 0x4c:
        if (done) {
            select_frame(g_frame_owner_72860, 7);
            eng1_interpret_signed_pairs(g_frame_owner_72860);
            g_frame_owner_72860->state = 0x4d;
            spawn_at_group_300c0(g_frame_owner_72860->x + 0x200000, g_frame_owner_72860->y + 0x140000,
                                 g_frame_owner_72860->z, 0x4005, 0);
        }
        break;
    case 0x4d:
        if (done) {
            select_frame(g_frame_owner_72860, 5);
            eng1_interpret_signed_pairs(g_frame_owner_72860);
            g_frame_owner_72860->state = 0x4b;
        }
        break;
    }
    if (g_frame_owner_72860->state != 0x23) {
        if ((g_engine_interface.context_004->pressed_064 & 0x400) && (g_engine_interface.context_004->cheats_030 & 0x4000)) {
            eng1_sim_state_72110 = 0;
            eng1_37080();
        }
        if ((g_engine_interface.context_004->pressed_064 & 0x100) && (g_engine_interface.context_004->cheats_030 & 0x4000)) {
            eng1_sim_state_72110 = 0;
            eng1_37080();
            if (g_engine_interface.context_004->flag_010 == 0) {
                g_engine_interface.context_004->flag_010 = 1;
            }
        }
        if ((g_engine_interface.context_004->pressed_064 & 0x200) && (g_engine_interface.context_004->cheats_030 & 0x4000)) {
            eng1_sim_state_72110 = 0;
            eng1_37080();
            g_engine_interface.context_004->flag_010 = 0;
        }
    }
    if ((g_engine_interface.context_004->pressed_064 & 0x800) && eng1_sim_state_72108 != 0x19 &&
        (g_engine_interface.context_004->cheats_030 & 0x2000)) {
        g_engine_interface.context_004->state_003 = 0x31;
    }
    eng1_37390(g_player_glow_707dc);
    eng1_37400(g_player_trail_707fc);
    g_eng1_setup_clear_72140[0] = 5;
    if (g_engine_interface.context_004->cheats_030 & 0x20) {
        g_eng1_setup_clear_72140[1] = 5;
    }
    if (g_engine_interface.context_004->cheats_030 & 0x40) {
        g_eng1_setup_clear_72140[2] = 5;
    }
    if (g_engine_interface.context_004->cheats_030 & 0x80) {
        g_eng1_setup_clear_72140[3] = 5;
    }
    if (g_engine_interface.context_004->cheats_030 & 0x100) {
        g_eng1_setup_clear_72140[4] = 5;
    }
    if (--g_eng1_setup_clear_72140[g_eng1_setup_byte_6efb4] < 0) {
        g_eng1_setup_clear_72140[g_eng1_setup_byte_6efb4] = g_weapon_timers_55320[g_eng1_setup_byte_6efb4];
    }
}

void eng1_37e30(int damage)
{
    if (eng1_sim_state_72110 == 0 && !(g_engine_interface.context_004->cheats_030 & 2) && g_player_grace_6fff0 <= 0) {
        eng1_native_code_target_001a410();
        g_frame_owner_72860->render_flags &= ~0x800000;
        if (g_frame_owner_72860->state != 0x24 && g_frame_owner_72860->state != 0x23) {
            if (damage > 7) {
                g_engine_interface.context_004->energy_024[0]--;
            } else {
                g_engine_interface.context_004->energy_024[0] -=
                    g_damage_table_55348[g_engine_interface.context_004->difficulty_00e & 3][damage];
            }
            if (g_engine_interface.context_004->energy_024[0] <= 0 && !(g_engine_interface.context_004->cheats_030 & 1)) {
                eng1_37080();
                return;
            }
            if (random_limit(0x8000) & 0x4000) {
                play_sound(0x6279, g_frame_owner_72860);
            } else {
                play_sound(0x6278, g_frame_owner_72860);
            }
            g_frame_owner_72860->state = 0x24;
            select_frame(g_frame_owner_72860, 1);
            eng1_interpret_signed_pairs(g_frame_owner_72860);
            g_player_glow_707dc->image_064 = g_player_program_63350;
            reset_frame_state(g_player_glow_707dc);
            eng1_interpret_word_stream(g_player_glow_707dc);
            g_player_trail_707fc->image_064 = g_player_program_63378;
            reset_frame_state(g_player_trail_707fc);
            eng1_interpret_word_stream(g_player_trail_707fc);
        }
    }
}

void eng1_37f80(void)
{
    play_sound(0x6010, g_frame_owner_72860);
}

void eng1_37fa0(Actor *p)
{
    eng1_27410(p);
    resource_cancel(0x31);
    resource_cancel(0x3b);
    resource_reset(0x32);
    g_player_flag_707f4 = 0;
}

void eng1_37fe0(Actor *p)
{
    int r;

    r = random_limit(0x8000);
    if (p->x < 0) {
        p->state = 0x39;
        select_frame(p, 1);
        p->unknown_108 = 0;
        eng1_empty_c720(p);
        eng1_interpret_signed_pairs(p);
    } else if (r < 0x3000) {
        p->state = 0x2e;
        select_frame(p, 1);
        eng1_interpret_signed_pairs(p);
    } else if (r < 0x6000) {
        p->state = 0x1a;
        select_frame(p, 2);
        eng1_interpret_signed_pairs(p);
    } else {
        p->state = 0xd;
        select_frame(p, 3);
        eng1_interpret_signed_pairs(p);
    }
}
