/* ENG1.DLL 0x3a040..0x3c410: level actor handlers of the train/scroll level (lane x15, plain C).
 * Types/idioms follow calibration/eng1/player_unit_36030.c (prelude copied; Actor +0x10c script_10c,
 * +0x128 owner_128, PanelContext +0x38 tick_038; eng1_05c70 last parameter unsigned char).
 * Same object as the player unit (.data 0x64270..0x65658, .bss 0x707e4..0x707f8). */

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
    unsigned char unknown_034[0x38 - 0x34];
    long tick_038;
    unsigned char unknown_03c[0x5c - 0x3c];
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
    short unknown_03c;
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
    Actor *prev_scene;
    Actor *next_scene;
    unsigned char lifetime;
    unsigned char unknown_0ed;
    short hits;
    void *steps;
    long unknown_0f4;
    unsigned char unknown_0f8[0x108 - 0xf8];
    long unknown_108;
    long *script_10c;
    void *unknown_110;
    short unknown_114;
    unsigned char unknown_116[0x118 - 0x116];
    struct SpawnRecord *records_118;
    unsigned char unknown_11c[0x11e - 0x11c];
    short unknown_11e;
    PendingActionGroup *group_120;
    short unknown_124;
    short unknown_126;
    Actor *owner_128;
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
void eng1_native_code_target_002ae50(Actor *p);
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
void eng1_05c70(void *a, void *b, void *c, int d, unsigned char e);
void dispatch_local_15d00(void);
int dispatch_local_20ea0(short *bounds, int id, int mode, long x);
void dispatch_engine_2c4e0(unsigned long value);
void fx_clear_all_8117(void);
void resource_cancel(int id);
void resource_reset(int id);


typedef struct SVector { short vx, vy, vz, pad; } SVector;
typedef struct EffectDescriptor {   /* 0x644d0 view (calibration/eng1/level_frame_01e50.c) */
    unsigned short prims;
    unsigned short count;
    unsigned char scroll, divisor, density, blend;
    short x, y, z;
    unsigned char r, g, b;
    unsigned char unknown_11[3];
    void (*place)(Actor *, int);
    void (*update)(Actor *, int);
    long *pattern;
    void *palette;
} EffectDescriptor;
typedef struct SpawnPair { short dy; short frame; } SpawnPair;
typedef struct Callback8 {          /* 8-byte kind callbacks at g_configuration_dword_7137c */
    void (*initialize)(Actor *);
    void (*update)(Actor *);
} Callback8;

extern short g_sine_table_5a848[];
extern unsigned char g_player_palette_64270[];
extern long g_effect_timer_64474;
extern long g_effect_widths_64478[];
extern EffectDescriptor g_effect_descriptor_644d0;
extern SpawnPair g_spawn_offsets_644f8[6];
extern unsigned char g_entries_64510[];
extern unsigned char g_entries_646e0[];
extern unsigned char g_entries_647d0[];
extern long *g_script_table_64d28[];
extern long g_eng1_program_64d80[];
extern long g_eng1_program_64db0[];
extern long g_eng1_program_64e08[];
extern long g_eng1_program_64ea8[];
extern long g_eng1_program_64f48[];
extern long g_eng1_program_65008[];
extern long g_script_651f8[];
extern long g_script_652e8[];
extern long g_script_starts_65580[];
extern long g_level_actors_6fe78[];
extern unsigned short g_pipeline_word_6fec4;
extern long g_script_index_6fff4;
extern long g_spawn_phase_6fff8;
extern long g_effect_width_707ec;
extern long g_effect_width_target_707f0;
extern Callback8 *g_configuration_dword_7137c;
extern Bounds g_actor_pipeline_storage_71440;

void engine_diagnostic(const char *format, ...);
void RemoveFxIdLinks_7f8e(unsigned short id, Actor *owner);
int eng1_02500(Actor *p, void *descriptor);
void eng1_02920(Actor *p, int width);
void eng1_131c0(long x, long y, long z, long id);
void attach_entries(Actor *p, void *entries);
Actor *spawn_middle_277e0(long x, long y, long z, unsigned short kind);
void advance_frame(Actor *p);
int eng1_2a710(Bounds *b, Actor *p);
PendingActionGroup *acquire_pending_action_group(void);
PendingActionGroup *retire_pending_action_group(PendingActionGroup *group);
Actor *append_normal_action(PendingActionGroup *group, long x, long y, long z, int kind);
void eng1_37e30(int damage);
void eng1_37f80(void);
void eng1_38490(Actor *p);
void eng1_39d40(Actor *p);
void eng1_39dc0(Actor *p);
void eng1_39ff0(Actor *p);
long eng1_3c1e0(long x);
void eng1_3c230(Actor *p);
void eng1_3c330(Actor *p);
void eng1_3c460(Actor *p);
void eng1_3c490(Actor *p);
void eng1_3c4d0(Actor *p);
void eng1_3c5c0(Actor *p);

void eng1_3a040(Actor *p)
{
    Actor *a;
    int n;

    switch (p->state) {
    case 0x2e:
        eng1_39ff0(p);
        if (eng1_interpret_signed_pairs(p)) {
            eng1_39dc0(p);
        }
        break;
    case 0x32:
    case 0x33:
        eng1_39ff0(p);
        if (eng1_interpret_signed_pairs(p)) {
            eng1_39dc0(p);
        }
        break;
    case 0x34:
        eng1_39ff0(p);
        if (eng1_interpret_signed_pairs(p)) {
            eng1_39dc0(p);
        }
        break;
    case 0x37:
        eng1_interpret_signed_pairs(p);
        p->unknown_108 += 4;
        if (p->unknown_108 > 0xff) {
            p->unknown_108 = 0xff;
        }
        eng1_05c70(g_player_palette_707e0, g_player_palette_64270, g_72874, 0x100, (unsigned char)p->unknown_108);
        n = p->unknown_108;
        if (n == 0xff) {
            eng1_39dc0(p);
            g_player_palette_707e0 = g_player_palette_64270;
        } else if (!(n & 0x1f) && n < 300) {
            a = spawn_normal(g_burst_origin_707e4->x + 0x12c0000,
                             g_burst_origin_707e4->y - (n << 16) + ((random_limit(50) + 30) << 16),
                             g_burst_origin_707e4->z, 0x57);
            if (a != 0) {
                g_configuration_dword_71370[0x57].init(a);
                select_frame(a, (n >> 5) % 4);
                eng1_interpret_signed_pairs(a);
            }
        }
        break;
    case 0x39:
        eng1_interpret_signed_pairs(p);
        n = ++p->unknown_108;
        if (n > 200) {
            eng1_38490(p);
            return;
        }
        if (!(n & 0xf) && n < 0xa0) {
            a = spawn_normal(g_burst_origin_707e4->x - 0x960000,
                             ((random_limit(30) + n * 2 - 300) << 16) + g_burst_origin_707e4->y,
                             g_burst_origin_707e4->z, 0x5b);
            if (a != 0) {
                g_configuration_dword_71370[0x5b].init(a);
                select_frame(a, (n >> 4) & 1);
                eng1_interpret_signed_pairs(a);
            }
        }
        break;
    case 0x2b:
        p->y += 0x10000;
        p->scale += 0x2c;
        eng1_interpret_signed_pairs(p);
        if (++p->unknown_108 == 0x50) {
            p->state = 0x2c;
        }
        break;
    case 0x2c:
        p->y -= 0x10000;
        p->scale -= 0x2c;
        eng1_interpret_signed_pairs(p);
        if (--p->unknown_108 == 0) {
            eng1_39dc0(p);
        }
        break;
    }
    p->x = ((g_player_flag_707f4 - g_camera_x_70ca0) << 16) / 12 + 0x1900000;
    if (p->x < -0x280000) {
        g_effect_timer_64474 = -1;
        g_player_flag_707f4 = 0;
        eng1_27410(p);
        resource_cancel(0x34);
    }
}

void eng1_3a340(Actor *p, int i)
{
    SVector *v;
    int x, z;

    v = (SVector *)p->entries;
    x = 0x78 - random_limit(0xf0);
    v[i].vx = x;
    z = 0x3c - random_limit(0x78);
    v[i].vz = z;
    v[i].vy = 0;
}

void eng1_3a390(Actor *p, int t)
{
    if (!g_engine_interface.context_004->paused_004) {
        if (g_effect_width_target_707f0 - g_effect_width_707ec > 0) {
            g_effect_width_707ec += 2;
        }
        if (g_effect_width_target_707f0 - g_effect_width_707ec < 0) {
            g_effect_width_707ec -= 2;
        }
    }
    if (g_effect_timer_64474 > 0) {
        if (!g_engine_interface.context_004->paused_004) {
            if (--g_effect_timer_64474 <= 0) {
                g_effect_timer_64474 = random_limit(0x10) + 1;
                g_effect_width_target_707f0 = g_effect_widths_64478[random_limit(8)];
            }
        }
        eng1_02920(p, g_effect_width_707ec);
    }
}

void eng1_3a420(Actor *p)
{
    p->flags |= 0x10000000;
    p->render_flags = 0x800;
    g_effect_descriptor_644d0.x = g_frame_owner_72860->x >> 16;
    g_effect_descriptor_644d0.y = g_frame_owner_72860->y >> 16;
    g_effect_descriptor_644d0.z = g_frame_owner_72860->z >> 16;
    eng1_02500(p, &g_effect_descriptor_644d0);
    p->z = g_frame_owner_72860->z;
    g_effect_width_707ec = 0;
    g_effect_timer_64474 = 10;
}

void eng1_3a4a0(Actor *p)
{
    engine_diagnostic("TTRAIN here\n");
    if (p->render_flags & 0x80000000) {
        engine_diagnostic("TTRAIN training\n");
        p->x = g_burst_origin_707e4->x + 0x200000;
        p->y = g_burst_origin_707e4->y + 0x200000;
        if (g_effect_timer_64474 < 0) {
            RemoveFxIdLinks_7f8e(0x256, 0);
            p->render_flags &= ~0x80000000;
            g_effect_timer_64474 = 10;
            g_effect_width_707ec = 0;
        }
    }
}

void eng1_3a530(Actor *p)
{
    advance_frame(p);
    p->render_flags |= 0x10000000;
}

void eng1_3a550(Actor *p)
{
    Actor *o;
    int a;
    int s, c;

    o = p->owner_128;
    if (o->kind == 0x3e && o->unknown_108 == 0x12345675) {
        a = (long)p->script_10c + o->angle_72;
        s = g_sine_table_5a848[a & 0x3ff] << 9;
        c = -g_sine_table_5a848[(a + 0x100) & 0x3ff] << 9;
        p->x = s + o->x;
        p->z = g_frame_owner_72860->z + c + 0x800000;
        p->angle_72 += 0x22;
        eng1_interpret_signed_pairs(p);
        if (p->state == 2 && eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
            eng1_37e30(2);
        }
    } else {
        remove_actor(p);
    }
}

void eng1_3a610(Actor *p)
{
    long x, y, z;
    int i;
    Actor *a;

    p->unknown_108 = 0x12345675;
    x = p->x;
    y = p->y;
    z = p->z;
    for (i = 0; i < 6; i++) {
        a = spawn_normal(x, (g_spawn_offsets_644f8[i].dy << 16) + y, z, 0x3f);
        if (a != 0) {
            g_configuration_dword_71370[0x3f].init(a);
            select_frame(a, g_spawn_offsets_644f8[i].frame);
            eng1_interpret_signed_pairs(a);
            a->owner_128 = p;
            a->script_10c = (long *)(i * 0x400 / 6);
            a->flags |= 0x10000000;
        }
    }
    a = spawn_normal(x, p->y, z, 0x40);
    if (a != 0) {
        g_configuration_dword_71370[0x40].init(a);
    }
}

void eng1_3a700(Actor *p)
{
    if (g_burst_origin_707e4->x - p->x > 0x1400000) {
        p->unknown_108 = 0;
        remove_actor(p);
    } else {
        p->angle_72 += 0x17;
    }
}

void eng1_3a730(Actor *p)
{
    p->flags |= 0x800;
    p->render_flags |= 0x20000600;
    p->sprite = 1;
    p->mode = 4;
    attach_entries(p, g_entries_64510);
}

void eng1_3a770(Actor *p)
{
    p->angle_72 -= 0x17;
    if (g_burst_origin_707e4->x - p->x > 0x1400000) {
        remove_actor(p);
    }
}

void eng1_3a7a0(Actor *p)
{
    p->unknown_108 = p->y;
    p->render_flags |= 0x10000000;
    p->motion_x_velocity = ((0x400 - random_limit(0x800)) << 8) + g_player_cruise_707e8;
    p->motion_y_velocity = -0x150000;
    advance_frame(p);
    select_frame(p, random_limit(6));
    eng1_interpret_signed_pairs(p);
}

void eng1_3a810(Actor *p)
{
    if (p->direction_x == 0) {
        p->x += p->motion_x_velocity;
    } else {
        p->x -= p->motion_x_velocity;
    }
    p->motion_y_velocity += 0xb000;
    p->y += p->motion_y_velocity;
    p->z -= 0xa0000;
    p->angle_70 += 0xc;
    p->angle_72 += 2;
    p->angle_74++;
    eng1_interpret_signed_pairs(p);
    eng1_39d40(p);
    if (p->y > p->unknown_108) {
        remove_actor(p);
    }
}

void eng1_3a8a0(Actor *p)
{
    p->flags |= 0x800;
    p->render_flags |= 0x20000600;
    p->sprite = 1;
    p->mode = 4;
    attach_entries(p, g_entries_646e0);
}

void eng1_3a8e0(Actor *p)
{
    p->angle_72 -= 0xf;
    if (g_burst_origin_707e4->x - p->x > 0x1400000) {
        remove_actor(p);
    }
}

void eng1_3a910(Actor *p)
{
    p->flags |= 0x800;
    p->render_flags |= 0x20000600;
    p->sprite = 1;
    p->mode = 4;
    attach_entries(p, g_entries_647d0);
}

void eng1_3a950(Actor *p)
{
    p->angle_72 -= 8;
    if (g_burst_origin_707e4->x - p->x > 0x1400000) {
        remove_actor(p);
    }
}

void eng1_3a980(Actor *p)
{
    Actor *a;

    if (!(g_engine_interface.context_004->tick_038 & 0xd) && random_limit(0x8000) < 0x3000) {
        a = spawn_normal((random_limit(0x6000) << 8) + p->x, g_burst_origin_707e4->y + 0x960000,
                         g_burst_origin_707e4->z + 0x2000000, 0x41);
        if (a) {
            g_configuration_dword_71370[0x41].init(a);
        }
    }
    if (g_burst_origin_707e4->x - p->x > 0xa00000) {
        remove_actor(p);
    }
}

void eng1_3aa10(Actor *p)
{
    p->scale = 0x200;
    p->render_flags |= 0x10000000;
    p->angle_70 = 0x3c0;
    p->angle_72 = 0x300;
    p->angle_74 = random_limit(0x400);
    p->motion_x_velocity = -0xc0000;
    p->motion_y_velocity = 0;
    advance_frame(p);
}

void eng1_3aa70(Actor *p)
{
    eng1_39d40(p);
    p->angle_70 -= 0xc;
    p->motion_y_velocity += 0xc000;
    if (p->direction_x == 0) {
        p->x += p->motion_x_velocity;
    } else {
        p->x -= p->motion_x_velocity;
    }
    p->y += p->motion_y_velocity;
    p->angle_74 += 0x14;
    eng1_interpret_signed_pairs(p);
    if (g_burst_origin_707e4->x - p->x > 0x12c0000) {
        remove_actor(p);
    }
}

void eng1_3aaf0(Actor *p)
{
    p->scale = 0x200;
    p->render_flags |= 0x10000000;
    p->angle_70 = random_limit(0x400);
    p->angle_72 = random_limit(0x400);
    p->motion_x_velocity = 0xc0000;
    p->motion_y_velocity = 0;
    advance_frame(p);
}

void eng1_3ab50(Actor *p)
{
    eng1_39d40(p);
    p->angle_70 += 0xc;
    p->motion_y_velocity += 0xc000;
    if (p->direction_x == 0) {
        p->x += p->motion_x_velocity;
    } else {
        p->x -= p->motion_x_velocity;
    }
    p->y += p->motion_y_velocity;
    p->angle_74 += 0x14;
    eng1_interpret_signed_pairs(p);
    if (p->x - g_burst_origin_707e4->x > 0x12c0000) {
        remove_actor(p);
    }
}

void eng1_3abd0(Actor *p)
{
    p->z += 0xc0000;
    if (p->kind == 0x30 || p->kind == 0x45) {
        p->direction_x = 0xff;
        p->render_flags |= 0x10;
    }
    p->state = 0x35;
    advance_frame(p);
}

void eng1_3ac10(Actor *p)
{
    eng1_27410(p);
    if (p->kind == 0x2c) {
        resource_cancel(0x35);
    }
    if (p->kind == 0x2d) {
        resource_cancel(0x36);
    }
    if (p->kind == 0x2e) {
        resource_cancel(0x37);
    }
    if (p->kind == 0x2f) {
        resource_cancel(0x38);
    }
    if (p->kind == 0x30) {
        resource_cancel(0x39);
    }
}

void eng1_3ac80(Actor *p)
{
    engine_diagnostic("God Here, Type %d, X %d, Y %d\n", p->kind, p->x >> 16, p->y >> 16);
    switch (p->state) {
    case 2:
        p->x += 0x60000;
        p->y -= 0x60000;
        if (eng1_interpret_signed_pairs(p)) {
            eng1_3c490(p);
        }
        if (!(g_engine_interface.context_004->tick_038 & 9)) {
            eng1_3c490(p);
        }
        if (g_frame_owner_72860->y - p->y > 0x1900000) {
            eng1_3ac10(p);
            return;
        }
        break;
    case 0x2d:
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 2;
            select_frame(p, 2);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 0x35:
        eng1_interpret_signed_pairs(p);
        if (eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
            play_sound(0x627b, p);
            p->state = 0x2d;
            select_frame(p, 1);
            eng1_interpret_signed_pairs(p);
        }
        break;
    default:
        p->x += 0x60000;
        p->y -= 0x60000;
        eng1_interpret_signed_pairs(p);
        break;
    }
    if (eng1_native_code_target_002aa30(p, 0x200, 0x200)) {
        eng1_3ac10(p);
    }
}

void eng1_3adc0(Actor *p)
{
    if (p->x - g_burst_origin_707e4->x < 0xc80000) {
        g_player_cruise_707e8 -= 0x1999;
        if (g_player_cruise_707e8 < 0) {
            g_player_cruise_707e8 = 0;
        }
    }
    switch (p->state) {
    case 0x35:
        eng1_interpret_signed_pairs(p);
        if (eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
            play_sound(0x627b, p);
            p->state = 0x2d;
            select_frame(p, 1);
            eng1_interpret_signed_pairs(p);
            p->unknown_108 = 0x78;
        }
        break;
    case 0x2d:
        eng1_interpret_signed_pairs(p);
        if (--p->unknown_108 == 0x10) {
            g_dispatch_key_7286c = 0xf8;
            g_dispatch_setting_4d858 = 0x80;
            g_dispatch_word_6ee80 = 0;
            dispatch_engine_2c4e0(1);
            g_engine_interface.context_004->unknown_1b30 = 4;
            dispatch_engine_2c4e0(1);
            g_engine_interface.context_004->unknown_1b30 = 8;
        }
        if (p->unknown_108 <= 0) {
            g_engine_interface.context_004->state_003 = 0x31;
        }
        break;
    }
}

void eng1_3aef0(Actor *p)
{
    g_burst_origin_707e4 = p;
    p->z = g_frame_owner_72860->z;
    g_frame_global_734f4 = p;
    g_player_flag_707f8 = 0;
}

void eng1_3af20(Actor *p)
{
    short bounds[8];

    p->x += g_player_cruise_707e8;
    if (g_player_flag_707f8) {
        if (g_player_flag_707f8 > 0) {
            p->z += 0x20000;
            g_player_flag_707f8 -= 2;
        } else {
            p->z -= 0x20000;
            g_player_flag_707f8 += 2;
        }
    }
    if (p->x - g_frame_owner_72860->x > 0x700000) {
        g_frame_owner_72860->x = p->x - 0x700000;
        g_player_glow_707dc->x = p->x - 0x700000;
        g_player_trail_707fc->x = p->x - 0x700000;
    }
    if (p->x - g_frame_owner_72860->x < -0xa00000) {
        g_frame_owner_72860->x = p->x + 0xa00000;
        g_player_glow_707dc->x = p->x + 0xa00000;
        g_player_trail_707fc->x = p->x + 0xa00000;
    }
    if (p->y - g_frame_owner_72860->y > 0xa00000) {
        p->y = g_frame_owner_72860->y + 0xa00000;
    }
    if (p->y - g_frame_owner_72860->y < 0) {
        p->y = g_frame_owner_72860->y;
    }
    dispatch_local_20ea0(bounds, 1, 8, p->x >> 16);
    if (p->y > (bounds[2] - 0x10) << 16) {
        p->y = (bounds[2] - 0x10) << 16;
    }
}

void eng1_3b050(Actor *p)
{
    if (p->x < g_burst_origin_707e4->x) {
        remove_actor(p);
        return;
    }
    g_player_flag_707f8 += 0x80;
}

void eng1_3b080(Actor *p)
{
    if (p->x < g_burst_origin_707e4->x) {
        remove_actor(p);
        return;
    }
    g_player_flag_707f8 += 0x40;
}

void eng1_3b0b0(Actor *p)
{
    if (p->x < g_burst_origin_707e4->x) {
        remove_actor(p);
        return;
    }
    g_player_flag_707f8 += 0x20;
}

void eng1_3b0e0(Actor *p)
{
    if (p->x < g_burst_origin_707e4->x) {
        remove_actor(p);
        return;
    }
    g_player_flag_707f8 -= 0x20;
}

void eng1_3b110(Actor *p)
{
    if (p->x < g_burst_origin_707e4->x) {
        remove_actor(p);
        return;
    }
    g_player_flag_707f8 -= 0x40;
}

void eng1_3b140(Actor *p)
{
    if (p->x < g_burst_origin_707e4->x) {
        remove_actor(p);
        return;
    }
    g_player_flag_707f8 -= 0x80;
}

void eng1_3b170(Actor *p)
{
    if (p->x < g_burst_origin_707e4->x) {
        remove_actor(p);
    }
}

void eng1_3b190(Actor *p)
{
    p->z += 0x80000;
    p->flags = 0x10000202;
    p->state = 2;
    p->motion_x_velocity = p->motion_x_rate = p->motion_y_velocity =
        p->motion_y_rate = p->motion_z_velocity = p->motion_z_rate = 0;
    advance_frame(p);
    select_frame(p, 0);
    eng1_interpret_signed_pairs(p);
    p->limit_0e0 = 0;
}

void eng1_3b200(Actor *p)
{
    unsigned long hit;
    Actor *a;
    Actor *owner;

    eng1_3c230(p);
    eng1_3c330(p);
    hit = eng1_26de0(p);
    if (hit) {
        if (hit != 0x80) {
            eng1_30290(hit, 0);
        } else {
            eng1_37f80();
        }
        if (random_limit(0x8000) & 4) {
            play_sound(0xe25d, p);
        } else {
            play_sound(0xe25e, p);
        }
        eng1_3c490(p);
        a = spawn_middle_277e0(p->x, p->y, p->z, 0x402a);
        if (a) {
            g_configuration_dword_7137c[0x402a & 0x1ff].initialize(a);
        }
        p->flags &= ~0x200;
        p->state = 0x23;
        p->motion_y_velocity = 0;
        select_frame(p, 1);
        eng1_interpret_signed_pairs(p);
        owner = p->owner_128;
        if (owner->group_120->count == 1) {
            eng1_3c460(p);
        }
        remove_counted_value(owner->group_120, p);
    } else {
        eng1_3c4d0(p);
    }
}

void eng1_3b310(Actor *p)
{
    if (!(g_engine_interface.context_004->tick_038 & 0xf) && random_limit(0x8000) < 600) {
        if (random_limit(0x8000) < 0x4000) {
            play_sound(0xe25b, p);
        } else {
            play_sound(0xe25c, p);
        }
    }
    switch (p->state) {
    case 0x10:
        eng1_3c5c0(p);
        /* fall through */
    case 2:
        eng1_interpret_signed_pairs(p);
        eng1_3b200(p);
        break;
    case 0x4f:
        p->unknown_108 += 8;
        p->x = p->owner_128->x + (g_sine_table_5a848[(p->unknown_108 + 0x100) & 0x3ff] << 8);
        p->y = p->owner_128->y - (g_sine_table_5a848[p->unknown_108 & 0x3ff] << 8);
        eng1_interpret_signed_pairs(p);
        eng1_3b200(p);
        break;
    case 0x23:
        p->motion_y_velocity += 0x8000;
        p->y += p->motion_y_velocity;
        if (eng1_interpret_signed_pairs(p)) {
            remove_actor(p);
            return;
        }
        break;
    case 9:
        if (eng1_interpret_signed_pairs(p)) {
            select_frame(p, 0);
            eng1_interpret_signed_pairs(p);
            p->state = 2;
        }
        eng1_3b200(p);
        break;
    case 0x40:
        p->x += g_player_cruise_707e8;
        if (--p->unknown_108 < 0) {
            p->state = 2;
        }
        break;
    default:
        eng1_interpret_signed_pairs(p);
        eng1_3b200(p);
        break;
    }
    if (g_burst_origin_707e4->x - p->x > 0x1900000) {
        remove_counted_value(p->owner_128->group_120, p);
        p->group_120 = 0;
        remove_actor(p);
    }
}

void eng1_3b530(Actor *p)
{
    PendingActionGroup *group;
    Actor *a;
    long x, y, z;
    int i;
    long *program;
    unsigned short state;

    x = p->x + 0x12c0000;
    z = g_frame_owner_72860->z;
    y = g_burst_origin_707e4->y;
    program = g_eng1_program_64d80;
    state = 2;
    switch (g_spawn_phase_6fff8++ & 3) {
    case 0:
        y -= 0x640000;
        break;
    case 1:
        y += 0x640000;
        break;
    case 2:
        state = 0x10;
        break;
    default:
        program = g_eng1_program_64db0;
        break;
    }
    group = acquire_pending_action_group();
    if (group) {
        p->group_120 = group;
        for (i = 1; i < 0x19; i += 4) {
            a = append_normal_action(group, x, y, z, 0x5c);
            if (!a) {
                break;
            }
            g_configuration_dword_71370[0x5c].init(a);
            a->limit_0e0 = i;
            a->owner_128 = p;
            a->script_10c = program;
            a->state = state;
            x += 0x400000;
        }
    }
}

void eng1_3b640(Actor *p)
{
    PendingActionGroup *group;
    Actor *a;
    long x, y, z;
    int i;
    long j;
    long *program;

    z = g_frame_owner_72860->z;
    y = g_frame_owner_72860->y;
    x = p->x + 0x12c0000;
    j = 0;
    program = g_eng1_program_64e08;
    switch (g_spawn_phase_6fff8++ & 1) {
    case 0:
        y -= 0x640000;
        break;
    case 1:
        y += 0x640000;
        program = g_eng1_program_64ea8;
        break;
    }
    group = acquire_pending_action_group();
    if (group) {
        p->group_120 = group;
        for (i = 1; i < 0x19; i += 4) {
            a = append_normal_action(group, x, y, z, 0x5c);
            if (!a) {
                break;
            }
            a->owner_128 = p;
            g_configuration_dword_71370[0x5c].init(a);
            a->limit_0e0 = i;
            a->unknown_108 = j;
            a->state = 0x40;
            a->script_10c = program;
            j += 8;
        }
    }
}

void eng1_3b730(Actor *p)
{
    PendingActionGroup *group;
    Actor *a;
    long x, y, z;
    int i;
    long j;

    z = g_frame_owner_72860->z + 0x2000000;
    x = p->x + 0x12c0000;
    y = eng1_3c1e0(x) - 0x600000;
    j = 0;
    group = acquire_pending_action_group();
    if (group) {
        p->group_120 = group;
        for (i = 1; i < 0x19; i += 4) {
            a = append_normal_action(group, x, y, z, 0x5c);
            if (!a) {
                break;
            }
            a->owner_128 = p;
            g_configuration_dword_71370[0x5c].init(a);
            a->limit_0e0 = i;
            a->unknown_108 = j;
            a->state = 0x40;
            a->script_10c = g_eng1_program_64f48;
            j += 8;
        }
    }
}

void eng1_3b7f0(Actor *p)
{
    PendingActionGroup *group;
    Actor *a;
    long x, y, z;
    int i;
    long j;

    x = g_burst_origin_707e4->x + 0x15e0000;
    p->x = x;
    y = eng1_3c1e0(x);
    p->y = y;
    z = g_frame_owner_72860->z;
    j = 0;
    group = acquire_pending_action_group();
    if (group) {
        p->group_120 = group;
        for (i = 0; i < 8; i++) {
            a = append_normal_action(group, x, y, z, 0x5c);
            if (!a) {
                break;
            }
            a->owner_128 = p;
            g_configuration_dword_71370[0x5c].init(a);
            a->unknown_108 = j;
            a->state = 0x4f;
            a->script_10c = 0;
            a->limit_0e0 = 0;
            j += 0x80;
        }
    }
}

void eng1_3b8b0(Actor *p)
{
    PendingActionGroup *group;
    Actor *a;
    long x, y, z;
    int i;
    long j;

    x = p->x + 0x12c0000;
    z = g_frame_owner_72860->z;
    y = g_frame_owner_72860->y - 0x640000;
    j = 0;
    group = acquire_pending_action_group();
    if (group) {
        p->group_120 = group;
        for (i = 1; i < 0x19; i += 4) {
            a = append_normal_action(group, x, y, z, 0x5c);
            if (!a) {
                break;
            }
            a->owner_128 = p;
            g_configuration_dword_71370[0x5c].init(a);
            a->limit_0e0 = i;
            a->unknown_108 = j;
            a->state = 0x40;
            a->script_10c = g_eng1_program_65008;
            j += 8;
        }
    }
}

void eng1_3b970(Actor *p)
{
    eng1_3b8b0(p);
}

void eng1_3b980(Actor *p)
{
    p->x += g_player_cruise_707e8 / 2;
    if (p->group_120 != 0) {
        if (p->group_120->count != 0)
            return;
        retire_pending_action_group(p->group_120);
        p->group_120 = 0;
    }
    remove_actor(p);
}

void eng1_3b9d0(Actor *p)
{
    p->sprite = 0;
    p->unknown_108 = 0;
    p->script_10c = g_script_table_64d28[g_script_index_6fff4];
    g_script_index_6fff4++;
}

void eng1_3ba00(Actor *p)
{
    Actor *a;
    long delay;

    if (--p->unknown_108 < 0) {
        switch (*p->script_10c++) {
        case 0:
            eng1_27410(p);
            return;
        case 2:
            a = spawn_normal(g_burst_origin_707e4->x + 0x12c0000, g_burst_origin_707e4->y,
                             g_frame_owner_72860->z, 0x62);
            if (a != 0)
                g_configuration_dword_71370[0x62].init(a);
            break;
        case 1:
            a = spawn_normal(g_burst_origin_707e4->x, g_burst_origin_707e4->y,
                             g_frame_owner_72860->z, 0x63);
            if (a != 0)
                g_configuration_dword_71370[0x63].init(a);
            break;
        case 3:
            a = spawn_normal(g_burst_origin_707e4->x + 0x12c0000, g_burst_origin_707e4->y,
                             g_frame_owner_72860->z, 0x64);
            if (a != 0)
                g_configuration_dword_71370[0x64].init(a);
            break;
        case 4:
            a = spawn_normal(g_burst_origin_707e4->x + 0x12c0000, g_burst_origin_707e4->y,
                             g_frame_owner_72860->z, 0x65);
            if (a != 0)
                g_configuration_dword_71370[0x65].init(a);
            break;
        case 5:
            a = spawn_normal(g_burst_origin_707e4->x + 0x12c0000, g_burst_origin_707e4->y,
                             g_frame_owner_72860->z, 0x66);
            if (a != 0)
                g_configuration_dword_71370[0x66].init(a);
            break;
        case 6:
            a = spawn_normal(g_burst_origin_707e4->x + 0x12c0000, g_burst_origin_707e4->y,
                             g_frame_owner_72860->z, 0x67);
            if (a != 0)
                g_configuration_dword_71370[0x67].init(a);
            break;
        }
        delay = *p->script_10c++;
        p->unknown_108 = delay;
    }
}

void eng1_3bbe0(Actor *p)
{
    reset_frame_state(p);
    p->scale = 0x180;
    p->unknown_024 = 0x40;
}

void eng1_3bc00(Actor *p)
{
    if (eng1_interpret_word_stream(p))
        remove_actor_variant(p);
}

void eng1_3bc20(Actor *p)
{
    p->z = g_frame_owner_72860->z + 0x80000;
    if (p->x < g_burst_origin_707e4->x) {
        remove_actor(p);
        return;
    }
    p->hits = 4;
    p->flags = 0x10000002;
    p->motion_z_rate = 0;
    p->motion_z_velocity = 0;
    p->motion_y_rate = 0;
    p->motion_y_velocity = 0;
    p->motion_x_rate = 0;
    p->motion_x_velocity = 0;
    advance_frame(p);
    select_frame(p, 0);
    eng1_interpret_signed_pairs(p);
    p->limit_0e0 = 0;
    p->state = 0x37;
}

void eng1_3bcb0(Actor *p)
{
    unsigned long hit;

    if (p->state == 0x22) {
        eng1_3c4d0(p);
        return;
    }
    hit = eng1_26de0(p);
    if (hit == 0)
        return;
    if (hit != 0x80)
        eng1_30290(hit, 0);
    else
        eng1_37f80();
    if (random_limit(0x8000) & 4)
        play_sound(0xe264, p);
    else
        play_sound(0xe265, p);
    eng1_131c0(p->x, p->y, p->z, 0x241);
    p->flags &= ~0x200;
    p->motion_y_velocity = 0;
    select_frame(p, 2);
    eng1_interpret_signed_pairs(p);
    p->state = 0x24;
    if (--p->hits <= 0) {
        eng1_131c0(p->x, p->y, p->z, 0x24a);
        eng1_131c0(p->x + 0x200000, p->y - 0x280000, p->z, 0x243);
        p->state = 0x23;
        p->motion_y_velocity = -0x80000;
    }
}

void eng1_3bdc0(Actor *p)
{
    eng1_3c230(p);
    eng1_3c330(p);
}

/* Case order measured: case 0x51 written as its own body (cross-jumped onto 0x22) and case 0x24
 * last reproduce the original block layout and the 0x50/0x24 shared tail. */
void eng1_3bde0(Actor *p)
{
    switch (p->state) {
    case 2:
        eng1_3c5c0(p);
    case 0x22:
        eng1_interpret_signed_pairs(p);
        eng1_3bcb0(p);
        eng1_3bdc0(p);
        break;
    case 0x23:
        if (g_player_cruise_707e8 < 0x40000)
            g_player_cruise_707e8 += 0x1111;
        p->motion_y_velocity += 0x8000;
        p->y += p->motion_y_velocity;
        eng1_interpret_signed_pairs(p);
        if (p->y - g_burst_origin_707e4->y > 0x12c0000) {
            g_player_cruise_707e8 = 0x40000;
            remove_actor(p);
        }
        break;
    case 0x40:
        if (--p->unknown_108 < 0)
            p->state = 2;
        break;
    case 0x37:
        p->x = g_burst_origin_707e4->x + 0x1100000;
        eng1_interpret_signed_pairs(p);
        g_player_cruise_707e8 -= 0x888;
        if (g_player_cruise_707e8 < 0x20000) {
            p->state = 0x15;
            play_sound(0x25f, 0);
        }
        break;
    case 0x15:
        p->x = g_burst_origin_707e4->x + 0x1100000;
        eng1_interpret_signed_pairs(p);
        g_player_cruise_707e8 -= 0x888;
        if (g_player_cruise_707e8 < 0) {
            g_player_cruise_707e8 = 0;
            select_frame(p, 0);
            eng1_interpret_signed_pairs(p);
            p->state = 0x50;
        }
        break;
    case 0x50:
        p->x -= 0x20000;
        eng1_3c5c0(p);
        if (p->x - g_burst_origin_707e4->x < 0x900000) {
            p->state = 2;
            select_frame(p, 0);
            p->script_10c = g_script_651f8;
            p->limit_0e0 = 1;
        }
        eng1_interpret_signed_pairs(p);
        break;
    case 0x51:
        eng1_interpret_signed_pairs(p);
        eng1_3bcb0(p);
        eng1_3bdc0(p);
        break;
    case 0x24:
        if (eng1_interpret_signed_pairs(p)) {
            p->script_10c = g_script_652e8;
            p->limit_0e0 = 1;
            p->state = 2;
            select_frame(p, 0);
            eng1_interpret_signed_pairs(p);
        }
        break;
    }
}

void eng1_3c050(Actor *p)
{
    p->state = 2;
    advance_frame(p);
}

void eng1_3c070(Actor *p)
{
    switch (p->state) {
    case 2:
        eng1_interpret_signed_pairs(p);
        if (g_pipeline_word_6fec4 && eng1_2a710(&g_actor_pipeline_storage_71440, p)) {
            p->state = 0x23;
            p->motion_y_velocity = 0;
            select_frame(p, 1);
            eng1_interpret_signed_pairs(p);
        }
    case 0x23:
        p->motion_y_velocity += 0x8000;
        p->y += p->motion_y_velocity;
        if (eng1_interpret_signed_pairs(p)) {
            remove_actor(p);
            return;
        }
    }
    eng1_interpret_signed_pairs(p);
    if (eng1_native_code_target_002aa30(p, 0x200, 0x200))
        remove_actor(p);
}

void eng1_3c130(void)
{
    Actor *a;

    g_level_actors_6fe78[0] = 0;
    a = spawn_at_group_300c0(0, 0, 0, 0x38, 0);
    if (a != 0) {
        g_level_actors_6fe78[++g_level_actors_6fe78[0]] = (long)a;
    }
}

/* Statement order measured: the script-index store first reproduces the original store schedule. */
void eng1_3c170(void)
{
    Actor *a;

    g_script_index_6fff4 = g_script_starts_65580[(signed char)g_engine_interface.context_004->next_002];
    g_player_flag_707f4 = 0;
    g_player_cruise_707e8 = 0x40000;
    g_player_flag_707f8 = 0;
    a = spawn_normal(g_frame_owner_72860->x, g_frame_owner_72860->y, g_frame_owner_72860->z, 0x36);
    if (a != 0)
        g_configuration_dword_71370[0x36].init(a);
}

long eng1_3c1e0(long x)
{
    short bounds[8];
    long low;

    dispatch_local_20ea0(bounds, 0, 8, x);
    low = bounds[0];
    dispatch_local_20ea0(bounds, 1, 8, x);
    return ((low + bounds[2]) / 2) << 16;
}

/* Motion script: {delay, opcode, operand}; 0 end, 1..3 velocity, 5 frame, 6..8 rate, 9 jump, 10 state. */
void eng1_3c230(Actor *p)
{
    long *s;

    if (p->limit_0e0 != 0 && --p->limit_0e0 == 0) {
        s = p->script_10c;
        do {
            p->limit_0e0 = *s++;
            switch (*s++) {
            case 0:
                p->limit_0e0 = 0;
                return;
            case 1:
                p->motion_x_velocity = *s++;
                break;
            case 2:
                p->motion_y_velocity = *s++;
                break;
            case 3:
                p->motion_z_velocity = *s++;
                break;
            case 6:
                p->motion_x_rate = *s++;
                break;
            case 7:
                p->motion_y_rate = *s++;
                break;
            case 8:
                p->motion_z_rate = *s++;
                break;
            case 9:
                s = (long *)*s;
                break;
            case 5:
                select_frame(p, *s++);
                eng1_interpret_signed_pairs(p);
                break;
            case 10:
                p->state = (unsigned short)*s++;
                break;
            }
        } while (p->limit_0e0 < 0);
        p->script_10c = s;
    }
}

void eng1_3c330(Actor *p)
{
    short bounds[8];

    p->x += g_player_cruise_707e8 / 2;
    p->motion_x_velocity += p->motion_x_rate;
    p->x += p->motion_x_velocity;
    p->motion_y_velocity += p->motion_y_rate;
    p->y += p->motion_y_velocity;
    p->motion_z_velocity += p->motion_z_rate;
    p->z += p->motion_z_velocity;
    if (p->motion_y_velocity >= 0) {
        dispatch_local_20ea0(bounds, 1, 8, p->x >> 16);
        if (p->y > (bounds[2] - 0x40) << 16)
            p->y = (bounds[2] - 0x40) << 16;
    } else {
        dispatch_local_20ea0(bounds, 0, 8, p->x >> 16);
        if (p->y < bounds[0] << 16)
            p->y = bounds[0] << 16;
    }
}

typedef struct Step3D { short x, y, z; } Step3D;

typedef struct Sprite {             /* PSX SPRT, 0x14 bytes */
    unsigned long tag;
    unsigned char r0, g0, b0, code;
    short x0, y0;
    unsigned char u0, v0;
    unsigned short clut;
    short w, h;
} Sprite;

typedef struct FrameBuffer {        /* ENG1 display buffer: ordering table at 0x88 */
    unsigned char unknown_000[0x88];
    unsigned long ot[0x500];
} FrameBuffer;

typedef struct LevelProfile {       /* 28-byte level record at 0x5df08 */
    unsigned char unknown_00[4];
    unsigned char flags_04;
    unsigned char unknown_05[0x1c - 0x05];
} LevelProfile;

typedef struct ClassifierRow {      /* 12-byte zone row at 0x65678 */
    unsigned short unknown_00;
    unsigned short mask_02;
    unsigned short state_04;
    unsigned short top_06;
    unsigned short base_08;
    unsigned short unknown_0a;
} ClassifierRow;

extern unsigned short *g_spatial_sound_words_71714;
extern unsigned short g_eng1_sound_words_66d68[];
extern LevelProfile g_frame_config_5df08[];
extern ClassifierRow g_eng1_classifier_rows_65678[];
extern unsigned long g_player_dust_index_655a4;
extern long g_player_dust_ids_655a8[10];
extern short g_platform_directions_686c0[];
extern FrameBuffer *g_dispatch_frame_current_71ec4;
extern unsigned char *g_frame_global_73484;
extern unsigned short g_frame_global_73480;
extern unsigned long g_frame_global_73488;
extern unsigned short g_eng1_control_70ee6;
extern unsigned short g_eng1_control_70eea;
extern Actor *g_eng1_control_actor_70eec;
extern unsigned short g_eng1_control_70efc;
extern unsigned short g_eng1_control_70efe;
extern unsigned short g_eng1_control_70f0c;
extern unsigned short g_eng1_control_70f0e, g_eng1_control_70ef0, g_eng1_control_70f04;
extern short g_platform_sound_70004;

int dispatch_engine_2c460(int index);
void dispatch_engine_2c980(void *p, int dfe, int dtd, int tpage, int tw);
void eng1_2ca50(void *p);
int initialize_command_table(unsigned long bytes, unsigned int opcode, unsigned long count);
int eng1_2a950(Actor *p, int frame);
int planar_bounds_overlap(Bounds *a, Bounds *b);
unsigned char eng1_native_code_target_0041fd0(long x);
void eng1_native_code_target_0041000(Actor *p, int category);
long eng1_native_code_target_00266a0(Actor *p, int a, int b);
int eng1_3eb30(Actor *p, int kind, int count);
void eng1_02d80(int index, unsigned int du, unsigned int dv);
void LinkedSoundFx_6da0(unsigned short id, Actor *owner);
void RemoveFxLinks_7ef6(Actor *p);
int eng1_19b40(int mask);
void eng1_1ab50(void);
void restart_steps(Actor *p);
void apply_frame_motion_3d(Actor *p);
void eng1_3c980(int x, int y, int w, int h, int u, int v, int layer);

/* Three calls (tail-merged by the compiler) reproduce the per-branch pushes. */

void eng1_3c410(Actor *p, long id)
{
    if (p->screen_x > 200)
        eng1_131c0(p->x, p->y - 0x200000, p->z, id | 0x240);
    else if (p->screen_x > 0x28)
        eng1_131c0(p->x, p->y - 0x200000, p->z, id);
    else
        eng1_131c0(p->x, p->y - 0x200000, p->z, id | 0x20);
}

void eng1_3c460(Actor *p)
{
    if (++g_player_dust_index_655a4 >= 10)
        g_player_dust_index_655a4 = 0;
    eng1_3c410(p, g_player_dust_ids_655a8[g_player_dust_index_655a4]);
}

/* Measured: the if/else assignment to n gives the original setge/dec/and/inc and the eax copy. */
void eng1_3c490(Actor *p)
{
    int n;

    n = dispatch_engine_2c460(0x8000);
    if (n < 0x3000) {
        if (n < 0x800)
            n = 3;
        else
            n = 1;
        eng1_3c410(p, n);
    }
}

void eng1_3c4d0(Actor *p)
{
    if (g_frame_owner_72860->state != 0x24 && g_frame_owner_72860->state != 0x23 &&
        eng1_2a710(&g_native_pointer_7217c->bounds, p))
        eng1_37e30(2);
}

void eng1_3c510(Actor *p)
{
    eng1_2a950(p, 1);
    p->bounds.min_x += p->x >> 16;
    p->bounds.max_x += p->x >> 16;
    p->bounds.min_y += p->y >> 16;
    p->bounds.max_y += p->y >> 16;
    eng1_2a950(g_frame_owner_72860, 1);
    g_frame_owner_72860->bounds.min_x += g_frame_owner_72860->screen_x;
    g_frame_owner_72860->bounds.max_x += g_frame_owner_72860->screen_x;
    g_frame_owner_72860->bounds.min_y += g_frame_owner_72860->screen_y;
    g_frame_owner_72860->bounds.max_y += g_frame_owner_72860->screen_y;
    if (planar_bounds_overlap(&g_frame_owner_72860->bounds, &p->bounds))
        eng1_37e30(2);
}

void eng1_3c5c0(Actor *p)
{
    long d;

    d = p->y - g_frame_owner_72860->y;
    if (d < 0) {
        if (d < -0x80000)
            p->y += 0x20000;
    } else if (d > 0x80000) {
        p->y -= 0x20000;
    }
}

/* Three parallax rows, each wrapped over a 0x200-wide texture pair; then five draw-mode packets.
 * Measured: the layer/v locals reproduce the original register allocation (dtd spilled, ebp/ebx for layer/v). */
void eng1_3c600(void)
{
    int dtd;
    int u, w, rest, page, layer, v;

    dtd = g_frame_config_5df08[g_engine_interface.context_004->level_000].flags_04 & 1;

    u = (g_camera_x_70ca0 / 3) & 0x1ff;
    if (u < 0xff) {
        w = 0x100 - u;
        rest = 0x40 - w;
        page = 0;
    } else {
        w = 0x200 - u;
        rest = 0x40 - w;
        page = 1;
    }
    layer = page + 3;
    eng1_3c980(0, 0x98, w, 0x58, u, 0x80, layer);
    if (rest > 0)
        eng1_3c980(w + 0x100, 0x98, rest, 0x58, 0, 0x80, layer);
    eng1_3c980(w, 0x98, 0x100, 0x58, 0, 0x80, !page + 3);

    u = (g_camera_x_70ca0 / 6) & 0x1ff;
    if (u < 0xff) {
        w = 0x100 - u;
        rest = 0x40 - w;
        page = 0;
    } else {
        w = 0x200 - u;
        rest = 0x40 - w;
        page = 1;
    }
    layer = page + 1;
    eng1_3c980(0, 0x70, w, 0x50, u, 0, layer);
    if (rest > 0)
        eng1_3c980(w + 0x100, 0x70, rest, 0x50, 0, 0, layer);
    eng1_3c980(w, 0x70, 0x100, 0x50, 0, 0, !page + 1);

    u = (g_camera_x_70ca0 / 12) & 0x1ff;
    if (u < 0xff) {
        w = 0x100 - u;
        rest = 0x40 - w;
        page = 0;
    } else {
        w = 0x200 - u;
        rest = 0x40 - w;
        page = 1;
    }
    v = page << 7;
    eng1_3c980(0, 0, w, 0x80, u, v, 0);
    if (rest > 0)
        eng1_3c980(w + 0x100, 0, rest, 0x80, 0, v, 0);
    eng1_3c980(w, 0, 0x100, 0x80, 0, !page << 7, 0);

    dispatch_engine_2c980(g_frame_global_73484, 0, dtd, 0x80, 0);
    *(unsigned long *)g_frame_global_73484 = g_dispatch_frame_current_71ec4->ot[1];
    g_dispatch_frame_current_71ec4->ot[1] = (unsigned long)g_frame_global_73484;
    g_frame_global_73484 += 0x28;
    dispatch_engine_2c980(g_frame_global_73484, 0, dtd, 0x9c, 0);
    *(unsigned long *)g_frame_global_73484 = g_dispatch_frame_current_71ec4->ot[2];
    g_dispatch_frame_current_71ec4->ot[2] = (unsigned long)g_frame_global_73484;
    g_frame_global_73484 += 0x28;
    dispatch_engine_2c980(g_frame_global_73484, 0, dtd, 0x9e, 0);
    *(unsigned long *)g_frame_global_73484 = g_dispatch_frame_current_71ec4->ot[3];
    g_dispatch_frame_current_71ec4->ot[3] = (unsigned long)g_frame_global_73484;
    g_frame_global_73484 += 0x28;
    dispatch_engine_2c980(g_frame_global_73484, 0, dtd, 0x9c, 0);
    *(unsigned long *)g_frame_global_73484 = g_dispatch_frame_current_71ec4->ot[4];
    g_dispatch_frame_current_71ec4->ot[4] = (unsigned long)g_frame_global_73484;
    g_frame_global_73484 += 0x28;
    dispatch_engine_2c980(g_frame_global_73484, 0, dtd, 0x9e, 0);
    *(unsigned long *)g_frame_global_73484 = g_dispatch_frame_current_71ec4->ot[5];
    g_dispatch_frame_current_71ec4->ot[5] = (unsigned long)g_frame_global_73484;
    g_frame_global_73484 += 0x28;
}

void eng1_3c980(int x, int y, int w, int h, int u, int v, int layer)
{
    Sprite *s;

    s = (Sprite *)g_frame_global_73484;
    s->x0 = x;
    s->y0 = y;
    s->w = w;
    s->h = h;
    s->u0 = u;
    s->v0 = v;
    s->clut = g_frame_global_73480;
    *(unsigned long *)&s->r0 = g_frame_global_73488;
    eng1_2ca50(s);
    s->tag = g_dispatch_frame_current_71ec4->ot[layer + 1];
    g_dispatch_frame_current_71ec4->ot[layer + 1] = (unsigned long)s;
    s->code = 0x64;
    g_frame_global_73484 = (unsigned char *)(s + 1);
}

void eng1_native_code_target_003ca10(void)
{
    g_eng1_setup_word_718e0 = 0x360;
    g_spatial_sound_words_71714 = g_eng1_sound_words_66d68;
    initialize_command_table(0x14, 0x3781, 0xe);
    g_camera_adjust_y_70d04 = -2;
    g_eng1_setup_word_7183c = -2;
}

void eng1_3ca50(void)
{
    switch (g_eng1_control_70f0c) {
    case 0xc:
    case 0xf:
    case 0x10:
    case 0x11:
        g_eng1_setup_word_7183c = -0x40;
        break;
    default:
        g_eng1_setup_word_7183c = -2;
        break;
    }
}

void eng1_3ca80(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int mask;

    p->x -= 0x20000;
    p->unknown_0d8 = p->y - 0x800000;
    p->unknown_0dc = 0x40;
    p->limit_0e0 = 0xa0;
    p->counter_021 = eng1_native_code_target_0041fd0(p->x);
    if (eng1_native_code_target_002b130(p)) {
        mask = g_eng1_classifier_rows_65678[p->counter_021].mask_02;
        g = p->group_120;
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->kind == 0x2f) {
                a->unknown_0d8 = a->y;
                a->unknown_0dc = p->unknown_0dc;
                a->limit_0e0 = p->limit_0e0;
                eng1_native_code_target_0041000(a, p->counter_021);
            }
            if (a->kind == 0x30) {
                if (mask & 1) {
                    a->flags &= ~0x200;
                    select_frame(a, 0xc);
                    a->state = 9;
                }
                mask >>= 1;
            }
        }
    } else {
        remove_actor(p);
    }
}

/* Platform group handlers (0x3cb90, 0x3cf90, 0x3d440, 0x3d930, 0x3dcd0) share one body shape. Measured:
 * t is computed before the row store and h is the (short) fixed-point integer part held in an int;
 * when link_130 is null the following 0x4031 test reads through the null pointer (original behaviour). */
void eng1_3cb90(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int d, t;
    int h;

    eng1_native_code_target_002ae50(p);
    if (eng1_3eb30(p, 0x30, 9)) {
        if (g_eng1_control_70eea == p->counter_021)
            g_eng1_control_actor_70eec = p;
        g = p->group_120;
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->kind == 0x2f) {
                if (p->unknown_124 == p->unknown_126) {
                    t = (a->y - (a->limit_0e0 << 16) - a->unknown_0d8) >> 16;
                    g_eng1_classifier_rows_65678[p->counter_021].top_06 = (a->unknown_0d8 >> 16) + a->limit_0e0;
                    if (t < 0) {
                        g_eng1_control_70efe = 2;
                        switch (g_eng1_control_70efc) {
                        case 1:
                            a->y += 0x20000;
                            eng1_02d80(0, 0, 1);
                        case 2:
                            a->y += 0x20000;
                            eng1_02d80(0, 0, 2);
                        }
                        if (a->state < 8)
                            a->state |= 4;
                    }
                } else {
                    d = a->unknown_0dc * p->unknown_126 / p->unknown_124;
                    t = (a->y - (d << 16) - a->unknown_0d8) >> 16;
                    g_eng1_classifier_rows_65678[p->counter_021].top_06 = (a->unknown_0d8 >> 16) + d;
                    if (t < 0) {
                        g_eng1_control_70efe = 1;
                        if (g_eng1_control_70efc) {
                            a->y += 0x20000;
                            eng1_02d80(0, 0, 1);
                        }
                    }
                }
                h = (short)((a->y - p->unknown_0d8) >> 16);
                g_eng1_classifier_rows_65678[p->counter_021].state_04 = a->state;
                a = a->link_130;
                if (a)
                    a->unknown_026 = h;
            }
            if (g_eng1_control_70eea == p->counter_021 && a->kind == 0x4031)
                a->unknown_098 = g_eng1_control_70ee6;
        }
        if (g_eng1_control_70ee6) {
            if (!g_platform_sound_70004) {
                LinkedSoundFx_6da0(0x5f, 0);
                g_platform_sound_70004++;
            }
        } else if (g_platform_sound_70004) {
            RemoveFxIdLinks_7f8e(0x5f, 0);
            g_platform_sound_70004 = 0;
        }
    }
    if (eng1_2a9e0(p, 0x200)) {
        RemoveFxIdLinks_7f8e(0x5f, 0);
        g_platform_sound_70004 = 0;
        remove_actor(p);
    }
}

void eng1_3ce10(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int mask;

    p->render_flags |= 0x10000080;
    p->sprite = 1;
    p->flags = 0x80040801;
    p->unknown_0d8 = p->y - 0x280000;
    p->unknown_0dc = 0x40;
    p->limit_0e0 = 0x80;
    p->y += 0x800000;
    p->saved_y = p->y;
    p->counter_021 = eng1_native_code_target_0041fd0(p->x);
    if (eng1_native_code_target_002b130(p)) {
        mask = g_eng1_classifier_rows_65678[p->counter_021].mask_02;
        g = p->group_120;
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->kind == 0x2f) {
                a->unknown_0d8 = a->y;
                a->unknown_0dc = p->unknown_0dc;
                a->limit_0e0 = p->limit_0e0;
                eng1_native_code_target_0041000(a, p->counter_021);
            }
            if (a->kind == 0x4032) {
                a->unknown_0d8 = p->unknown_0d8;
                a->render_flags |= 0x10000;
            }
            if (a->kind == 0x30) {
                if (mask & 1) {
                    a->flags &= ~0x200;
                    select_frame(a, 0xc);
                    a->state = 9;
                }
                mask >>= 1;
            }
        }
        if (g_eng1_classifier_rows_65678[p->counter_021].base_08)
            p->y = g_eng1_classifier_rows_65678[p->counter_021].base_08 << 16;
        else
            g_eng1_classifier_rows_65678[p->counter_021].base_08 = p->y >> 16;
    } else {
        remove_actor(p);
    }
}

void eng1_3cf90(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int d, t, top;
    int n;
    int h;

    n = 0;
    if (eng1_3eb30(p, 0x30, 9)) {
        if (g_eng1_control_70eea == p->counter_021)
            g_eng1_control_actor_70eec = p;
        g = p->group_120;
        d = p->unknown_0dc * p->unknown_126 / p->unknown_124;
        top = ((d << 16) + p->y - p->saved_y) >> 16;
        g_eng1_classifier_rows_65678[p->counter_021].base_08 = (p->saved_y >> 16) - d;
        eng1_02d80(5, 0, -(g_eng1_control_70ee6 >> 4));
        if (top > 0) {
            g_eng1_control_70efe = 1;
            if (g_eng1_control_70efc)
                p->y -= 0x10000;
        }
        for (a = p->next_scene; a; a = a->next_scene) {
            if (a == g_native_pointer_7217c && (~g_native_word_72168 & g_eng1_setup_word_72012 & 0x20)) {
                g_native_word_72168 |= 0x20;
                n = dispatch_engine_2c460(3) + 2;
            }
        }
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->kind == 0x30) {
                a->y = p->y;
                if (n && a->state != 9) {
                    a->hits = -1;
                    g_eng1_control_70f04++;
                    n--;
                }
            }
            if (a->kind == 0x4032) {
                a->y = p->y - 0xa0000;
                a->unknown_026 = (a->y - a->unknown_0d8) >> 16;
            }
            if (a->kind == 0x2f) {
                if (p->unknown_124 == p->unknown_126) {
                    t = (a->y - (a->limit_0e0 << 16) - a->unknown_0d8) >> 16;
                    g_eng1_classifier_rows_65678[p->counter_021].top_06 = (a->unknown_0d8 >> 16) + a->limit_0e0;
                    if (t < 0) {
                        g_eng1_control_70efe = 2;
                        switch (g_eng1_control_70efc) {
                        case 1:
                            a->y += 0x20000;
                            eng1_02d80(5, 0, 2);
                        case 2:
                            a->y += 0x20000;
                            eng1_02d80(5, 0, 2);
                        }
                        if (a->state < 8)
                            a->state |= 4;
                        if (a->state < 8)
                            a->state |= 4;
                    }
                } else {
                    d = a->unknown_0dc * p->unknown_126 / p->unknown_124;
                    t = (a->y - (d << 16) - a->unknown_0d8) >> 16;
                    g_eng1_classifier_rows_65678[p->counter_021].top_06 = (a->unknown_0d8 >> 16) + d;
                    if (t < 0) {
                        g_eng1_control_70efe = 1;
                        if (g_eng1_control_70efc) {
                            a->y += 0x20000;
                            eng1_02d80(5, 0, 2);
                        }
                    }
                }
                h = (short)((a->y - p->unknown_0d8) >> 16);
                g_eng1_classifier_rows_65678[p->counter_021].state_04 = a->state;
                a = a->link_130;
                if (a)
                    a->unknown_026 = h;
            }
            if (g_eng1_control_70eea == p->counter_021 && a->kind == 0x4031)
                a->unknown_098 = g_eng1_control_70ee6;
        }
        if (g_eng1_control_70ee6) {
            if (!g_platform_sound_70004) {
                LinkedSoundFx_6da0(0x60, 0);
                g_platform_sound_70004++;
            }
        } else if (g_platform_sound_70004) {
            RemoveFxIdLinks_7f8e(0x60, 0);
            g_platform_sound_70004 = 0;
        }
    }
    if (eng1_2a9e0(p, 0x200))
        remove_actor(p);
}

void eng1_3d310(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int mask;

    p->render_flags = 0;
    p->sprite = 0;
    p->unknown_0d8 = p->y - 0x800000;
    p->unknown_0dc = 0x50;
    p->limit_0e0 = 0x80;
    p->counter_021 = eng1_native_code_target_0041fd0(p->x);
    if (eng1_native_code_target_002b130(p)) {
        mask = g_eng1_classifier_rows_65678[p->counter_021].mask_02;
        g = p->group_120;
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->kind == 0x2f) {
                if (p->z != 0x800000)
                    a->z = 0x800000;
                a->unknown_0d8 = a->y;
                a->unknown_0dc = p->unknown_0dc;
                a->limit_0e0 = p->limit_0e0;
                eng1_native_code_target_0041000(a, p->counter_021);
            }
            if (a->kind == 0x4034)
                a->unknown_098 = 4;
            if (a->kind == 0x3a) {
                if (mask & 1) {
                    a->flags &= ~0x200;
                    select_frame(a, 0xc);
                    a->state = 9;
                }
                mask >>= 1;
            }
        }
    } else {
        remove_actor(p);
    }
}

/* The angle is masked only when the 0x3a child has no pending sound (nested test measured). */
void eng1_3d440(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int d, t;
    int h;
    long x, z;
    int angle;

    angle = 0;
    x = p->x;
    z = p->z;
    eng1_native_code_target_002ae50(p);
    if (eng1_3eb30(p, 0x3a, 9)) {
        if (g_eng1_control_70eea == p->counter_021)
            g_eng1_control_actor_70eec = p;
        g = p->group_120;
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->kind == 0x43) {
                x = a->x;
                z = a->z;
                angle = a->angle_72;
            } else if (a->kind == 0x2f) {
                if (p->unknown_124 == p->unknown_126) {
                    t = (a->y - (a->limit_0e0 << 16) - a->unknown_0d8) >> 16;
                    g_eng1_classifier_rows_65678[p->counter_021].top_06 = (a->unknown_0d8 >> 16) + a->limit_0e0;
                    if (t < 0) {
                        g_eng1_control_70efe = 2;
                        switch (g_eng1_control_70efc) {
                        case 1:
                            a->y += 0x20000;
                            eng1_02d80(0, 0, 1);
                        case 2:
                            a->y += 0x20000;
                            eng1_02d80(0, 0, 2);
                        }
                        if (a->state < 8)
                            a->state |= 4;
                    }
                } else {
                    d = a->unknown_0dc * p->unknown_126 / p->unknown_124;
                    t = (a->y - (d << 16) - a->unknown_0d8) >> 16;
                    g_eng1_classifier_rows_65678[p->counter_021].top_06 = (a->unknown_0d8 >> 16) + d;
                    if (t < 0) {
                        g_eng1_control_70efe = 1;
                        if (g_eng1_control_70efc) {
                            a->y += 0x20000;
                            eng1_02d80(0, 0, 1);
                        }
                    }
                }
                h = (short)((a->y - p->unknown_0d8) >> 16);
                g_eng1_classifier_rows_65678[p->counter_021].state_04 = a->state;
                a = a->link_130;
                if (a)
                    a->unknown_026 = h;
            } else if (a->kind == 0x3a) {
                a->x = g_sine_table_5a848[angle & 0x3ff] * 240 + x;
                a->z = g_sine_table_5a848[(angle + 0x100) & 0x3ff] * 240 + z;
                a->angle_74 = (g_sine_table_5a848[(angle - 0x200) & 0x3ff] << 8) >> 16;
                if (!a->parameter) {
                    angle &= 0x3ff;
                    if (z < 0x800000 && angle > 0x1f8 && angle < 0x208) {
                        LinkedSoundFx_6da0(0x5867, a);
                        a->parameter = 0x14;
                    }
                }
                angle += 0x100;
            } else if (g_eng1_control_70eea == p->counter_021 && a->kind == 0x4031) {
                a->unknown_098 = g_eng1_control_70ee6;
            }
        }
        if (g_eng1_control_70ee6) {
            if (!g_platform_sound_70004) {
                LinkedSoundFx_6da0(0x5f, 0);
                g_platform_sound_70004++;
            }
        } else if (g_platform_sound_70004) {
            RemoveFxIdLinks_7f8e(0x5f, 0);
            g_platform_sound_70004 = 0;
        }
    }
    if (eng1_2a9e0(p, 0x200))
        remove_actor(p);
}

void eng1_3d780(Actor *p)
{
    p->sprite = 5;
    p->render_flags |= 0x10000040;
    p->angle_72 = (g_engine_interface.context_004->tick_038 >> 3) & 0x3ff;
    eng1_native_code_target_002ae50(p);
}

void eng1_3d7c0(Actor *p)
{
    if (p->z & 0x800000)
        p->angle_72 += 8;
    else
        p->angle_72 -= 8;
    p->angle_72 &= 0x3ff;
    if (!p->sound_06c && (p->status_01f & 2))
        LinkedSoundFx_6da0(p->state, p);
}

void eng1_3d800(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int mask;
    int j;

    p->unknown_0d8 = p->y - 0x800000;
    p->unknown_0dc = 0x40;
    p->limit_0e0 = 0x60;
    p->counter_021 = eng1_native_code_target_0041fd0(p->x);
    if (eng1_native_code_target_002b130(p)) {
        g = p->group_120;
        mask = g_eng1_classifier_rows_65678[p->counter_021].mask_02;
        j = 0;
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->kind == 0x2f) {
                a->unknown_0d8 = a->y;
                a->unknown_0dc = p->unknown_0dc;
                a->limit_0e0 = p->limit_0e0;
                eng1_native_code_target_0041000(a, p->counter_021);
            }
            if (a->kind == 0x2a) {
                a->unknown_097 = g_platform_directions_686c0[j++];
                if (mask & 1) {
                    a->flags &= ~0x200;
                    select_frame(a, 0xc);
                    a->state = 9;
                    a->z = 0x800000;
                }
                mask >>= 1;
            }
        }
    } else {
        remove_actor(p);
    }
}

void eng1_3d930(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int d, t;
    int h;

    if (eng1_3eb30(p, 0x2a, 9)) {
        if (g_eng1_control_70eea == p->counter_021)
            g_eng1_control_actor_70eec = p;
        g = p->group_120;
        eng1_native_code_target_002ae50(p);
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->kind == 0x2f) {
                if (p->unknown_124 == p->unknown_126) {
                    t = (a->y - (a->limit_0e0 << 16) - a->unknown_0d8) >> 16;
                    g_eng1_classifier_rows_65678[p->counter_021].top_06 = (a->unknown_0d8 >> 16) + a->limit_0e0;
                    if (t < 0) {
                        g_eng1_control_70efe = 2;
                        switch (g_eng1_control_70efc) {
                        case 1:
                            a->y += 0x20000;
                            eng1_02d80(0, 0, 1);
                        case 2:
                            a->y += 0x20000;
                            eng1_02d80(0, 0, 2);
                        }
                        if (a->state < 8)
                            a->state |= 4;
                    }
                } else {
                    d = a->unknown_0dc * p->unknown_126 / p->unknown_124;
                    t = (a->y - (d << 16) - a->unknown_0d8) >> 16;
                    g_eng1_classifier_rows_65678[p->counter_021].top_06 = (a->unknown_0d8 >> 16) + d;
                    if (t < 0) {
                        g_eng1_control_70efe = 1;
                        if (g_eng1_control_70efc) {
                            a->y += 0x20000;
                            eng1_02d80(0, 0, 1);
                        }
                    }
                }
                h = (short)((a->y - p->unknown_0d8) >> 16);
                g_eng1_classifier_rows_65678[p->counter_021].state_04 = a->state;
                a = a->link_130;
                if (a)
                    a->unknown_026 = h;
            }
            if (g_eng1_control_70eea == p->counter_021 && a->kind == 0x4031)
                a->unknown_098 = g_eng1_control_70ee6;
        }
        if (g_eng1_control_70ee6) {
            if (!g_platform_sound_70004) {
                LinkedSoundFx_6da0(0x5f, 0);
                g_platform_sound_70004++;
            }
        } else if (g_platform_sound_70004) {
            RemoveFxIdLinks_7f8e(0x5f, 0);
            g_platform_sound_70004 = 0;
        }
    }
    if (eng1_2a9e0(p, 0x200))
        remove_actor(p);
}

void eng1_3db90(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int mask;

    p->render_flags |= 0x10000080;
    p->sprite = 1;
    p->flags = 0x80040801;
    p->unknown_030 = 0x2ff;
    p->unknown_0d8 = p->y;
    p->unknown_0dc = 0x40;
    p->limit_0e0 = 0x60;
    p->counter_021 = eng1_native_code_target_0041fd0(p->x);
    if (!eng1_native_code_target_002b130(p)) {
        remove_actor(p);
        return;
    }
    mask = g_eng1_classifier_rows_65678[p->counter_021].mask_02;
    g = p->group_120;
    for (i = 0; i < g->count;) {
        a = g->entries[++i];
        if (a->kind == 0x4033) {
            a->saved_x = p->x - 0x15c0000;
            a->saved_y = p->y + 0x540000;
        }
        if (a->kind == 0x2f) {
            a->unknown_0d8 = a->y;
            a->unknown_0dc = p->unknown_0dc;
            a->limit_0e0 = p->limit_0e0;
            eng1_native_code_target_0041000(a, p->counter_021);
        }
        if (a->kind == 0x4d) {
            if (mask & 1) {
                a->flags &= ~0x200;
                select_frame(a, 0xc);
                a->state = 9;
            }
            mask >>= 1;
        }
    }
}

void eng1_3dcd0(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int d, t;
    int h;
    int s;
    int delta;
    long dx, dy;
    int n;

    n = 0;
    if (p->unknown_097 == 0) {
        if (p->status_01f & 1) {
            p->unknown_097 = 1;
            LinkedSoundFx_6da0(0x4864, p);
        }
    } else {
        p->unknown_030 = (p->unknown_030 + 8) & 0x3ff;
    }
    s = (short)((g_sine_table_5a848[p->unknown_030 & 0x3ff] << 8) >> 16);
    delta = p->unknown_032 - s;
    p->unknown_032 = s;
    dx = g_sine_table_5a848[s & 0x3ff] * 320;
    dy = g_sine_table_5a848[(s + 0x100) & 0x3ff] * 320;
    p->saved_z = p->x;
    p->x = p->saved_x + dx;
    p->y = p->saved_y + dy;
    if (s == 0x7f && p->unknown_097 == 1) {
        p->unknown_097 = 2;
        LinkedSoundFx_6da0(0x4863, p);
    }
    if (s == -0x80 && p->unknown_097 == 2) {
        p->unknown_097 = 1;
        LinkedSoundFx_6da0(0x4864, p);
    }
    eng1_02d80(1, 0, -delta);
    for (a = p->next_scene; a; a = a->next_scene) {
        a->x += p->x - p->saved_z;
        a->saved_x += p->x - p->saved_z;
    }
    if (eng1_3eb30(p, 0x4d, 9)) {
        if (g_eng1_control_70eea == p->counter_021)
            g_eng1_control_actor_70eec = p;
        g = p->group_120;
        for (a = p->next_scene; a; a = a->next_scene) {
            if (a == g_native_pointer_7217c && (~g_native_word_72168 & g_eng1_setup_word_72012 & 0x20)) {
                g_native_word_72168 |= 0x20;
                n = dispatch_engine_2c460(3) + 2;
            }
        }
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->kind == 0x2f) {
                if (p->unknown_124 == p->unknown_126) {
                    t = (a->y - (a->limit_0e0 << 16) - a->unknown_0d8) >> 16;
                    g_eng1_classifier_rows_65678[p->counter_021].top_06 = (a->unknown_0d8 >> 16) + a->limit_0e0;
                    if (t < 0) {
                        g_eng1_control_70efe = 2;
                        switch (g_eng1_control_70efc) {
                        case 1:
                            a->y += 0x20000;
                            eng1_02d80(5, 0, 1);
                        case 2:
                            a->y += 0x20000;
                            eng1_02d80(5, 0, 2);
                        }
                        if (a->state < 8)
                            a->state |= 4;
                    }
                } else {
                    d = a->unknown_0dc * p->unknown_126 / p->unknown_124;
                    t = (a->y - (d << 16) - a->unknown_0d8) >> 16;
                    g_eng1_classifier_rows_65678[p->counter_021].top_06 = (a->unknown_0d8 >> 16) + d;
                    if (t < 0) {
                        g_eng1_control_70efe = 1;
                        if (g_eng1_control_70efc) {
                            a->y += 0x20000;
                            eng1_02d80(5, 0, 1);
                        }
                    }
                }
                h = (short)((a->y - p->unknown_0d8) >> 16);
                g_eng1_classifier_rows_65678[p->counter_021].state_04 = a->state;
                a = a->link_130;
                if (a)
                    a->unknown_026 = h;
            }
            if (a->kind == 0x4d) {
                a->y = a->unknown_0dc + p->y;
                a->x = a->saved_x + dx;
                if (n && a->state != 9) {
                    a->hits = -1;
                    g_eng1_control_70f04++;
                    n--;
                }
            }
            if (a->kind == 0x4032) {
                a->y = p->y - 0x80000;
                a->x = a->saved_x + dx;
                a->angle_74 = -s;
            }
            if (a->kind == 0x4033) {
                a->x = p->x - 0xb40000;
                a->y = p->y;
            }
            if (a->kind == 0x4031) {
                if (a->z > 0x1200000)
                    a->unknown_098 = delta;
                else if (g_eng1_control_70eea == p->counter_021)
                    a->unknown_098 = g_eng1_control_70ee6;
            }
        }
        if (g_eng1_control_70ee6) {
            if (!g_platform_sound_70004) {
                LinkedSoundFx_6da0(0x5f, 0);
                g_platform_sound_70004++;
            }
        } else if (g_platform_sound_70004) {
            RemoveFxIdLinks_7f8e(0x5f, 0);
            g_platform_sound_70004 = 0;
        }
    }
    if (eng1_2a9e0(p, 0x200))
        remove_actor(p);
}

void eng1_3e160(Actor *p)
{
    unsigned long hit;
    int n;

    eng1_interpret_signed_pairs(p);
    if (p->state != 9) {
        if (!p->lifetime && (hit = eng1_26de0(p)) != 0) {
            if (hit == 0x80) {
                n = eng1_19b40(~g_native_word_72168 & 0x5f);
                if (n) {
                    eng1_1ab50();
                    p->hits -= n;
                    select_frame(p, 0x14);
                }
            } else {
                eng1_30290(hit, 0);
                p->hits -= 100;
                select_frame(p, 0x14);
            }
            p->lifetime = 10;
        }
        if (p->hits <= 0) {
            p->flags &= ~0x200;
            select_frame(p, 0xb);
            p->state = 9;
            g_eng1_control_70f0e++;
        }
    }
}

void eng1_3e220(Actor *p)
{
    advance_frame(p);
    select_frame(p, 0xd);
    p->flags = 0xa02;
    p->unknown_124 = 0;
    p->hits = 3;
    p->unknown_0dc = p->state << 16;
    p->state = 0;
    attach_actor(p, 0x2022);
}

void eng1_3e280(Actor *p)
{
    unsigned long hit;
    int n;

    eng1_interpret_signed_pairs(p);
    if (p->state != 9) {
        if (!p->lifetime && (hit = eng1_26de0(p)) != 0) {
            if (hit == 0x80) {
                n = eng1_19b40(~g_native_word_72168 & 0x5f);
                if (n) {
                    eng1_1ab50();
                    p->hits -= n;
                    select_frame(p, 0x14);
                }
            } else {
                eng1_30290(hit, 0);
                p->hits -= 100;
                select_frame(p, 0x14);
            }
            p->lifetime = 10;
        }
        if (p->hits <= 0) {
            p->flags &= ~0x200;
            select_frame(p, 0xb);
            p->state = 9;
            g_eng1_control_70f0e++;
        } else if (eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
            eng1_native_code_target_00266a0(p, 1, 0);
            g_eng1_control_70ef0++;
        }
    }
}

void eng1_3e370(Actor *p)
{
    advance_frame(p);
    select_frame(p, 0);
    restart_steps(p);
    p->unknown_0f4 = 0;
    p->flags = 0x202;
    p->unknown_124 = 1;
    p->hits = 4;
    attach_actor(p, 0x2022);
}

void eng1_3e3d0(Actor *p)
{
    unsigned long hit;
    int n;
    short v;
    short z;

    eng1_interpret_signed_pairs(p);
    apply_frame_motion_3d(p);
    if (p->unknown_11e >= 0) {
        v = ((Step3D *)p->steps)[p->unknown_0f4].z;
        if (p->direction_z)
            v = ~v + 1;     /* measured: not/inc, -v gives neg */
        eng1_02d80(p->unknown_097, 0, v);
    }
    z = p->z >> 16;
    if (z < -0x18) {
        p->z = -0x180000;
        select_frame(p, 1);
        p->direction_z = 0xff;
    }
    if (z > 0x130) {
        p->z = 0x1300000;
        select_frame(p, 2);
        p->direction_z = 0;
    }
    if (p->state != 9 && !p->lifetime && (hit = eng1_26de0(p)) != 0) {
        if (hit == 0x80) {
            n = eng1_19b40(~g_native_word_72168 & 0x5f);
            if (n) {
                eng1_1ab50();
                p->hits -= n;
                if (p->state == 1)
                    select_frame(p, 5);
                else
                    select_frame(p, 8);
                RemoveFxLinks_7ef6(p);
            }
        } else {
            eng1_30290(hit, 0);
            p->hits -= 100;
            if (p->state == 1)
                select_frame(p, 5);
            else
                select_frame(p, 8);
            RemoveFxLinks_7ef6(p);
        }
        p->lifetime = 0x14;
        if (p->hits <= 0) {
            p->flags &= ~0x200;
            select_frame(p, 0xb);
            p->state = 9;
            g_eng1_control_70f0e++;
        }
    }
    if (!p->lifetime && eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
        eng1_native_code_target_00266a0(p, 1, 0);
        g_eng1_control_70ef0 += 3;
    }
}


typedef struct SpawnRecord {        /* 12-byte child placement: offsets, kind, sprite, direction */
    short x, y, z;
    short kind;
    short sprite;
    unsigned char direction_x;
    unsigned char unknown_0b;
} SpawnRecord;
typedef struct KindInfo { unsigned char *owner; long count; } KindInfo;

extern KindInfo g_kind_info[];
extern long g_actor_worklist_717c0[];
extern unsigned short *g_configuration_dword_713bc;
extern Callback8 *g_configuration_dword_7139c;
extern unsigned short g_eng1_control_70ede;
extern Actor *g_eng1_control_actor_70ef8;
extern unsigned short g_switch_masks_70008[5];
extern unsigned short g_lane_sprites_66040[];
extern long g_switch_sound_rows_66ea0[][4];
extern long g_burst_sound_rows_66738[][4];
extern SpawnRecord g_spawn_records_661a0[];

int abs(int);
void spawn_actor_with_owner(Actor *owner, long x, long y, long z, int kind);
int eng1_2c090(long x0, long y0, long x1, long y1);
long grid_scan_20b50(long x, long y, long z, int group);
void measure_xz_vector(FixedPosition *a, FixedPosition *b, RelativeMeasure *m);
void measure_relative_vector(FixedPosition *a, FixedPosition *b, RelativeMeasure *m);
void eng1_2ad70(FixedPosition *a, Actor *p, RelativeMeasure *m);
unsigned short lookup_cosine_phase(int x, int y);
int eng1_26cd0(Actor *p);
void eng1_41da0(Actor *p);
unsigned long damped_contact_motion(Actor *p);
int actor_bounds_2a880(Actor *p, Bounds *b, int mode);
int bounds_overlap(Bounds *a, Bounds *b);
void advance_object_motion(Actor *p);
int point_in_box_2a810(long x, long y, long z, Bounds *b);
unsigned long dispatch_local_2a170(long x, long y, long z, int profile);
void eng1_30380(Actor *p, int kind);
int eng1_29e20(Actor *p);
int eng1_2a660(Actor *a, Actor *b);
void detach_attachment(Actor *p);
Actor *eng1_27db0(long x, long y, long z, int kind);
void eng1_2b5e0(Actor *p, Actor *target, unsigned char flags);
unsigned char advance_target_motion(Actor *p, unsigned char flags);
void eng1_native_code_target_000ef10(Actor *p);

void eng1_3e580(Actor *p)
{
    advance_frame(p);
    select_frame(p, 0xd);
    p->flags = 0x202;
    p->unknown_124 = 0;
    p->hits = 5;
    p->parameter = 0x14;
    attach_actor(p, 0x2022);
}

void eng1_3e5d0(Actor *p)
{
    unsigned long hit;
    int n;

    eng1_interpret_signed_pairs(p);
    if (p->state != 9) {
        if (p->parameter)
            p->parameter--;
        if (!p->lifetime) {
            if ((hit = eng1_26de0(p)) != 0) {
                if (hit == 0x80) {
                    n = eng1_19b40(~g_native_word_72168 & 0x5f);
                    if (n) {
                        eng1_1ab50();
                        p->hits -= n;
                        select_frame(p, 0x14);
                    }
                } else {
                    eng1_30290(hit, 0);
                    p->hits -= 100;
                    select_frame(p, 0x14);
                }
                p->lifetime = 10;
                if (p->hits <= 0) {
                    p->flags &= ~0x200;
                    select_frame(p, 0xb);
                    p->state = 9;
                    g_eng1_control_70f0e++;
                }
            }
            if (!p->lifetime && eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
                eng1_native_code_target_00266a0(p, 1, 0);
                g_eng1_control_70ef0++;
            }
        }
    }
}

void eng1_3e6d0(Actor *p)
{
    Actor *a;

    advance_frame(p);
    select_frame(p, 0xd);
    p->direction_y = 0xff;
    p->motion_y_target = ((dispatch_engine_2c470() & 0x1ff) + 0xe00) << 8;
    p->motion_y_velocity = p->motion_y_target >> 4;
    p->motion_y_rate = 0;
    p->motion_y_unknown = dispatch_engine_2c460(0x2000) + 0x8000;
    p->state = 0;
    attach_actor(p, 0x2022);
    spawn_actor_with_owner(p, 0, 0, 0, 0x34);
    a = p->link_130;
    if (a)
        a->render_flags &= ~0x10000;
}

void eng1_3e770(Actor *p)
{
    RelativeMeasure m;
    Bounds b;
    unsigned long hit;
    long d;

    eng1_interpret_signed_pairs(p);
    if (p->state < 9 && (hit = eng1_26de0(p)) != 0) {
        if (hit == 0x80) {
            if (eng1_19b40(~g_native_word_72168 & 0x5f))
                eng1_1ab50();
        } else {
            eng1_30290(hit, 0);
        }
        eng1_131c0(p->x, p->y + 0x400000, p->z, 2);
        p->flags &= ~0x200;
        select_frame(p, 0xb);
        p->state = 9;
        p->lifetime = 0x1e;
    }
    switch (p->state) {
    case 0:
        measure_absolute_vector((FixedPosition *)g_native_pointer_7217c, (FixedPosition *)p, &m);
        if (m.x < 0x20 && m.z < 0x20)
            select_frame(p, 0x17);
        break;
    case 1:
        if (actor_bounds_2a880(p, &b, 0))
            d = (b.max_y << 16) - p->y;
        else
            d = 0;
        p->y += d;
        p->saved_y += d;
        if (damped_contact_motion(p) & 1) {
            LinkedSoundFx_6da0(0x4040, p);
            select_frame(p, 0xb);
            p->state = 9;
            p->lifetime = 0x1e;
        }
        p->y -= d;
        p->saved_y -= d;
        if (bounds_overlap(&g_native_pointer_7217c->bounds, &b)) {
            eng1_native_code_target_00266a0(p, 1, 0);
            g_eng1_control_70ef0++;
        }
        break;
    case 9:
        if (!p->lifetime) {
            p->state = 0x13;
            LinkedSoundFx_6da0(0x4044, p);
        }
    case 0x13:
        p->y -= 0x20000;
        if (eng1_native_code_target_002aa30(p, 0x80, 0x40))
            eng1_27410(p);
        break;
    }
    if (eng1_2a9e0(p, 0x200))
        remove_actor(p);
}

void eng1_3e9d0(Actor *p)
{
    Actor *o;

    o = p->link_12c;
    if (o) {
        p->x = o->x;
        p->y = o->y;
        p->z = o->z + 0x40000;
    } else {
        remove_actor(p);
    }
}

void eng1_3ea00(Actor *p)
{
    p->y = 0x2f40000;
    p->z = 0x1000000;
    p->sprite = 0x7f;
    p->render_flags |= 0x80;
    p->unknown_03c = 0x200;
    p->counter_021 = eng1_native_code_target_0041fd0(p->x);
}

void eng1_3ea40(Actor *p)
{
    if (g_eng1_control_70eea == p->counter_021)
        g_eng1_control_70ede = (p->x >> 16) - 0x1a;
}

void eng1_3ea70(Actor *p)
{
    p->sprite = 0x7f;
    p->render_flags |= 0x80080;
    p->unknown_03c = 0x200;
    p->counter_021 = eng1_native_code_target_0041fd0(p->x);
}

void eng1_3eaf0(Actor *p)
{
    int h;

    p->angle_74 = eng1_2c090(p->x, p->y, p->saved_x, p->saved_y);
    h = (short)((p->y - p->saved_y) >> 16);
    p->unknown_026 = h;
}

int eng1_3eb30(Actor *p, int kind, int state)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int mask, bit, matched, count;

    g = p->group_120;
    if (g) {
        mask = 0;
        matched = 0;
        count = 0;
        bit = 1;
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->kind == kind) {
                if (a->state == state) {
                    matched++;
                    mask |= bit;
                }
                bit <<= 1;
                count++;
            }
        }
        p->unknown_126 = matched;
        g_eng1_classifier_rows_65678[p->counter_021].mask_02 = mask;
        p->unknown_124 = count;
        return p->unknown_124 != 0;
    }
    return 0;
}

void eng1_3ebc0(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;

    p->render_flags |= 0x10000000;
    p->flags |= 0x100000;
    p->unknown_0d8 = 1;
    p->state = 0;
    eng1_native_code_target_002a290(p);
    p->counter_021 = eng1_native_code_target_0041fd0(p->x);
    p->sprite = 0x35;
    p->state = g_eng1_classifier_rows_65678[p->counter_021].unknown_0a;
    p->limit_0e0 = p->z + 0xe00000;
    if (p->state > 2)
        p->z = p->limit_0e0;
    if (eng1_native_code_target_002b130(p)) {
        g = p->group_120;
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (a->kind == 0x4039)
                a->sprite = g_lane_sprites_66040[p->counter_021];
            if (a->kind == 0x202b) {
                if (p->state > 0)
                    a->entry_count = 0xa3;
                else
                    a->entry_count = 0x83;
            }
        }
    } else {
        remove_actor(p);
    }
}

void eng1_3ecb0(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int dy;

    if (g_eng1_control_70eea == p->counter_021)
        g_eng1_control_actor_70ef8 = p;
    g_eng1_classifier_rows_65678[p->counter_021].unknown_0a = p->state;
    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        dy = 0;
        if (p->state == 3) {
            if (p->z < p->limit_0e0) {
                p->z += 0x40000;
                dy = -8;
            } else {
                p->state = 9;
                RemoveFxLinks_7ef6(g_eng1_control_actor_70ef8);
            }
        }
        if (p->records_118) {
            g = p->group_120;
            for (i = 0; i < g->count; i++) {
                a = g->entries[i + 1];
                if (a->kind != 0x202b) {
                    a->x = (p->records_118[i].x << 16) + p->x;
                    a->y = (p->records_118[i].y << 16) + p->y;
                    a->z = (p->records_118[i].z << 16) + p->z;
                    if (a->kind == 0x4031)
                        a->unknown_098 = dy * 4;
                }
            }
        }
    }
    if (eng1_2a9e0(p, 0x200) && p->counter_021 != g_eng1_control_70f0c)
        remove_actor(p);
}

void eng1_3edf0(Actor *p)
{
    p->render_flags |= 0x10000000;
    p->flags |= 0x100000;
    p->unknown_0d8 = 1;
    p->state = 0;
    eng1_native_code_target_002a290(p);
    p->direction_y = 0xff;
    p->scale = 0x300;
    p->motion_y_target = 0x130000;
    p->motion_y_velocity = 0;
    p->motion_y_rate = 0;
    p->motion_y_unknown = 0x1f000;
    p->unknown_0dc = p->y - 0x800000;
    p->counter_021 = eng1_native_code_target_0041fd0(p->x);
    p->sprite = g_lane_sprites_66040[p->counter_021];
    p->state = g_eng1_classifier_rows_65678[p->counter_021].unknown_0a;
    if (!eng1_native_code_target_002b130(p))
        remove_actor(p);
    attach_actor(p, 0x202f);
}

void eng1_3eec0(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;

    if (g_eng1_control_70eea == p->counter_021)
        g_eng1_control_actor_70ef8 = p;
    g_eng1_classifier_rows_65678[p->counter_021].unknown_0a = p->state;
    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        switch (p->state) {
        case 1:
            if (p->y > p->unknown_0dc)
                p->y -= 0x20000;
            if (p->x + 0xc00000 < g_native_pointer_7217c->x) {
                p->state = 2;
                LinkedSoundFx_6da0(0x404b, p);
            }
            break;
        case 2:
            if (damped_contact_motion(p) & 1) {
                p->state = 4;
                LinkedSoundFx_6da0(0x404c, p);
            }
            break;
        case 4:
            if (damped_contact_motion(p) & 2)
                p->state = 0;
            break;
        }
        if (p->records_118 && (g = p->group_120) != 0) {
            for (i = 0; i < g->count; i++) {
                a = g->entries[i + 1];
                if (a->kind == 0x403b) {
                    a->x = (p->records_118[i].x << 16) + p->x;
                    a->y = (p->records_118[i].y << 16) + p->y;
                    a->z = (p->records_118[i].z << 16) + p->z;
                } else {
                    a->y = ((p->unknown_0dc - p->y + 0x800000) >> 2) + 0x2f40000;
                }
            }
        }
    }
    if (eng1_2a9e0(p, 0x200))
        remove_actor(p);
}

void eng1_3f0b0(Actor *p)
{
    Actor *o;
    long h;

    o = (Actor *)p->entries;
    if (o->state == 1 || o->state == 2) {
        p->y = o->y - 0x20000;
        h = grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->profile) << 16;
        if (h > 0x1000000) {
            p->render_flags &= ~0x80000000;
        } else {
            p->render_flags |= 0x80000000;
            p->y += h;
            p->scale = 0x240 - (h >> 16);
            p->angle_72 = 0x80;
        }
    } else {
        p->render_flags &= ~0x80000000;
    }
}

void eng1_3f140(Actor *p)
{
    p->render_flags |= 0x10000000;
    p->flags |= 0x100200;
    p->unknown_0d8 = 2;
    p->sprite = 0x41;
    eng1_native_code_target_002a290(p);
    p->state = 0;
    p->angle_72 = dispatch_engine_2c460(0x100) - 0x80;
    p->hits = 3;
    p->angle_70 = -0x100;
    p->unknown_0dc = dispatch_engine_2c460(4) * 4 + 4;
}

void eng1_3f1c0(Actor *p)
{
    RelativeMeasure m;
    unsigned long hit;
    int n;
    int angle;
    int direction;
    Actor *a;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        if (p->hits)
            p->hits--;
        switch (p->state) {
        case 0:
            if (p->status_01f & 2) {
                p->state = 1;
                LinkedSoundFx_6da0(0x4056, p);
            }
            break;
        case 1:
            p->angle_70 += 4;
            if (p->angle_70 > 0) {
                p->angle_70 = 0;
                p->state = 2;
            }
            break;
        case 2:
            p->angle_72 += (short)p->unknown_0dc;
            measure_xz_vector((FixedPosition *)g_native_pointer_7217c, (FixedPosition *)p, &m);
            angle = lookup_cosine_phase(m.x, (unsigned short)m.planar_distance);
            direction = m.direction;
            if (direction == 6)
                angle += 0x100;
            if (direction == 4)
                angle = 0x300 - angle;
            if (direction == 0xc)
                angle += 0x300;
            if (direction == 0xe)
                angle = 0x100 - angle;
            angle &= 0x3ff;
            p->angle_72 &= 0x3ff;
            if (abs(p->angle_72 - angle) < 0xc && !p->hits && !eng1_sim_state_72110) {
                a = spawn_at_group_300c0(p->x, p->y - 0x200000, p->z, 0x4028, 0);
                if (a) {
                    a->motion_x_target = g_sine_table_5a848[angle & 0x3ff] << 5;
                    a->motion_z_target = g_sine_table_5a848[(angle + 0x100) & 0x3ff] << 5;
                    a->motion_x_velocity = a->motion_x_target;
                    a->motion_z_velocity = a->motion_z_target;
                    p->hits = 0x14;
                }
            }
            p->angle_72 &= 0x3ff;
            break;
        }
        if (p->state > 0) {
            if ((hit = eng1_26de0(p)) != 0) {
                if (hit == 0x80) {
                    n = eng1_19b40(~g_native_word_72168 & 0x5f);
                    if (n) {
                        eng1_1ab50();
                        p->hits -= n;
                        p->flags &= ~0x200;
                    }
                } else {
                    eng1_30290(hit, 0);
                    p->hits -= 100;
                    p->flags &= ~0x200;
                }
                if (p->hits < 0) {
                    LinkedSoundFx_6da0(0xc05a, p);
                    eng1_41da0(p);
                    eng1_27410(p);
                    return;
                }
            }
            if (eng1_26cd0(p)) {
                eng1_41da0(p);
                LinkedSoundFx_6da0(0xc05a, p);
                eng1_27410(p);
                return;
            }
        }
    }
    if (eng1_2a9e0(p, 0x200))
        remove_actor(p);
}

void eng1_3f460(Actor *p)
{
    p->render_flags |= 0x10000000;
    p->flags |= 0x100200;
    p->unknown_0d8 = 2;
    p->sprite = 4;
    eng1_native_code_target_002a290(p);
    p->state = 0;
    p->angle_70 = -0x100;
}

void eng1_3f4b0(Actor *p)
{
    RelativeMeasure m;
    unsigned long hit;
    int n;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        switch (p->state) {
        case 0:
            if (p->status_01f & 2) {
                p->state = 1;
                LinkedSoundFx_6da0(0x4056, p);
            }
            break;
        case 1:
            p->angle_70 += 4;
            if (p->angle_70 > 0) {
                p->angle_70 = 0;
                p->state = 2;
            }
            break;
        case 2:
            measure_xz_vector((FixedPosition *)g_native_pointer_7217c, (FixedPosition *)p, &m);
            break;
        }
        if (p->state > 0) {
            if ((hit = eng1_26de0(p)) != 0) {
                if (hit == 0x80) {
                    n = eng1_19b40(~g_native_word_72168 & 0x5f);
                    if (n) {
                        eng1_1ab50();
                        p->hits -= n;
                        p->flags &= ~0x200;
                    }
                } else {
                    eng1_30290(hit, 0);
                    p->hits -= 100;
                    p->flags &= ~0x200;
                }
                if (p->hits <= 0) {
                    LinkedSoundFx_6da0(0xc05a, p);
                    eng1_41da0(p);
                    eng1_27410(p);
                    return;
                }
            }
            if (eng1_26cd0(p)) {
                eng1_41da0(p);
                LinkedSoundFx_6da0(0xc05a, p);
                eng1_27410(p);
                return;
            }
        }
    }
    if (eng1_2a9e0(p, 0x200))
        remove_actor(p);
}

void eng1_3f620(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int mask;

    p->x -= 0x20000;
    p->counter_021 = eng1_native_code_target_0041fd0(p->x);
    if (p->record_offset)
        p->unknown_097 = g_kind_info[p->profile].owner[p->record_offset + 0x0c] & 0x3f;
    if (eng1_native_code_target_002b130(p)) {
        mask = g_switch_masks_70008[p->unknown_097];
        g = p->group_120;
        for (i = 0; i < g->count;) {
            a = g->entries[++i];
            if (i & 1)
                a->sprite = 2;
            else
                a->sprite = 4;
            if (a->kind == 0x4b && g_switch_masks_70008[p->unknown_097] == 0xffff)
                select_frame(a, 4);
            if (a->kind == 0x4a) {
                if (mask & 1) {
                    a->state = 9;
                    a->flags = 0;
                    a->sprite = 0;
                }
                mask >>= 1;
            }
        }
    } else {
        remove_actor(p);
    }
}

void eng1_3f720(Actor *p)
{
    PendingActionGroup *g;
    Actor *a;
    int i;
    int n;
    int j;

    if (g_switch_masks_70008[p->unknown_097] == 0xffff) {
        eng1_native_code_target_002ae50(p);
        if (eng1_2a9e0(p, 0x200))
            remove_actor(p);
        return;
    }
    if (eng1_3eb30(p, 0x4a, 9)) {
        if (p->unknown_124 == p->unknown_126) {
            g = p->group_120;
            for (i = 0; i < g->count;) {
                a = g->entries[++i];
                if (a->kind == 0x4b && !a->unknown_114) {
                    select_frame(a, 1);
                    n = 0;
                    for (j = 0; j < 5; j++)
                        if (g_switch_masks_70008[j] == 0xffff)
                            n++;
                    eng1_131c0(a->x, a->y, a->z, g_switch_sound_rows_66ea0[n][0]);
                    eng1_131c0(a->x, a->y, a->z, g_switch_sound_rows_66ea0[n][1]);
                    eng1_131c0(a->x, a->y, a->z, g_switch_sound_rows_66ea0[n][2]);
                    eng1_131c0(a->x, a->y, a->z, g_switch_sound_rows_66ea0[n][3]);
                }
            }
            g_switch_masks_70008[p->unknown_097] = 0xffff;
        } else {
            g_switch_masks_70008[p->unknown_097] = g_eng1_classifier_rows_65678[p->counter_021].mask_02;
        }
    }
    eng1_native_code_target_002ae50(p);
    if (eng1_2a9e0(p, 0x200))
        remove_actor(p);
}

void eng1_3f8d0(Actor *p)
{
    unsigned long hit;
    int n;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
        return;
    }
    switch (p->state) {
    case 0:
        if (p->angle_70 > 0) {
            p->angle_70 -= 4;
            if (p->angle_70 < -0x40)
                p->angle_70 = -0x100;
        }
        if (p->status_01f & 2) {
            p->state = 1;
            LinkedSoundFx_6da0(0x4056, p);
        }
        break;
    case 1:
        p->angle_70 += 4;
        if (p->angle_70 > 0) {
            p->angle_70 = 0;
            p->state = 2;
        }
        break;
    case 9:
        if (p->hits)
            p->hits--;
        else
            p->sprite = 0;
        break;
    }
    if (p->state < 9) {
        if (!(p->status_01f & 2))
            p->state = 0;
        if (p->angle_70 >= -0xc0) {
            if ((hit = eng1_26de0(p)) != 0) {
                if (hit == 0x80) {
                    n = eng1_19b40(~g_native_word_72168 & 0x5f);
                    if (n) {
                        eng1_1ab50();
                        p->hits -= n;
                        p->flags &= ~0x200;
                    }
                } else {
                    eng1_30290(hit, 0);
                    p->hits -= 100;
                    p->flags &= ~0x200;
                }
                if (p->hits <= 0) {
                    LinkedSoundFx_6da0(0xc05a, p);
                    eng1_41da0(p);
                    p->state = 9;
                    p->sprite = 0;
                    return;
                }
            }
            if (eng1_26cd0(p)) {
                LinkedSoundFx_6da0(0xc05a, p);
                eng1_41da0(p);
                p->state = 9;
                p->hits = 3;
                g_eng1_control_70f0e += 2;
            }
        }
    }
}

void eng1_3fa50(Actor *p)
{
    p->direction_x = p->direction_z = 0xff;
    p->motion_z_velocity = p->motion_z_target = ((dispatch_engine_2c470() & 0x1ff) + 0xa80) << 8;
    p->motion_z_rate = 0x4000;
    p->motion_z_unknown = 0;
    p->motion_x_velocity = p->motion_x_target = ((dispatch_engine_2c470() & 0x1ff) + 0xa80) << 8;
    p->motion_x_rate = dispatch_engine_2c460(0x2000) + 0x3000;
    p->motion_x_unknown = 0;
    p->scale = 0x80;
    p->render_flags |= 5;
    p->unknown_097 = 4;
    reset_frame_state(p);
}

void eng1_3fb00(Actor *p)
{
    unsigned long cell;

    eng1_interpret_word_stream(p);
    if ((p->status_01f & 2) && !p->sound_06c)
        LinkedSoundFx_6da0((dispatch_engine_2c460(3) + 0x57) | 0x5800, p);
    advance_object_motion(p);
    if (point_in_box_2a810(p->x, p->y, p->z, &g_native_pointer_7217c->bounds)) {
        eng1_native_code_target_00266a0(p, 1, 0);
        eng1_30380(p, 0x4025);
        return;
    }
    if (p->unknown_097) {
        p->unknown_097--;
        return;
    }
    cell = dispatch_local_2a170((p->direction_x ? -0x100000 : 0x100000) + p->x, p->y,
                                (p->z + 0x400000) & 0xf800000, p->profile);
    if (cell == 0x18 && (p->z & 0x800000) < 0x400000) {
        eng1_30380(p, 0x4025);
        return;
    }
    if (cell == 0x19 && (p->z & 0x800000) > 0x400000) {
        eng1_30380(p, 0x4025);
        return;
    }
    if (g_configuration_dword_713bc[cell] & 7) {
        eng1_30380(p, 0x4025);
        return;
    }
    if (p->z < -0x600000 || p->z > 0x2000000 || (p->status_01f & 3) == 1)
        remove_actor_variant(p);
}

void eng1_3fc60(Actor *p)
{
    SpawnRecord *r;
    Actor *a;
    int kind;

    LinkedSoundFx_6da0(0x8071, p);
    for (r = g_spawn_records_661a0; (kind = r->kind) != 0; r++) {
        a = spawn_middle_277e0((r->x << 16) + p->x, (r->y << 16) + p->y, (r->z << 16) + p->z, kind);
        if (a) {
            g_configuration_dword_7137c[kind & 0x1ff].initialize(a);
            a->sprite = r->sprite;
        }
    }
}

void eng1_3fcf0(Actor *p)
{
    p->render_flags |= 0x10000000;
    p->flags |= 0x800000;
    p->direction_x = -dispatch_engine_2c460(2);
    p->direction_z = -dispatch_engine_2c460(2);
    p->direction_y = 0xff;
    p->motion_y_velocity = p->motion_y_target = ((dispatch_engine_2c470() & 0x3ff) + 0x300) << 8;
    p->motion_y_rate = 0;
    p->motion_y_unknown = dispatch_engine_2c460(0x2000) + 0x4000;
    p->motion_z_target = ((dispatch_engine_2c470() & 0xff) + 0x200) << 8;
    p->motion_z_velocity = p->motion_z_target >> 1;
    p->motion_z_rate = dispatch_engine_2c460(0x1000) + 0x2000;
    p->motion_z_unknown = 0;
    p->motion_x_target = ((dispatch_engine_2c470() & 0x3ff) + 0x300) << 8;
    p->motion_x_velocity = p->motion_x_target >> 1;
    p->motion_x_rate = dispatch_engine_2c460(0x1000) + 0x6000;
    p->motion_x_unknown = 0;
    p->state = 0;
}

void eng1_3fe00(Actor *p)
{
    if ((damped_contact_motion(p) & 2) || eng1_native_code_target_002aa30(p, 0x80, 0x80))
        remove_actor_variant(p);
}

void eng1_3fe40(Actor *p)
{
    p->flags = 0x400202;
    p->unknown_124 = 1;
    spawn_actor_with_owner(p, p->x, p->y, p->z + 0x100000, 0x3b);
    p->y = 0x3880000;
    p->motion_x_velocity = 0;
    p->motion_x_target = 0x80000;
    p->motion_x_rate = dispatch_engine_2c460(0x4000) + 0x6000;
    p->motion_x_unknown = dispatch_engine_2c460(0x4000) + 0x4000;
    p->direction_x = 0xff;
    p->direction_z = 0xff;
    p->motion_z_velocity = 0;
    p->motion_z_target = 0x80000;
    p->motion_z_rate = 0x8000;
    p->motion_z_unknown = 0;
    p->unknown_0d8 = p->x;
    p->unknown_0dc = p->y;
    p->limit_0e0 = p->z;
    p->counter_021 = eng1_native_code_target_0041fd0(p->x);
    advance_frame(p);
}

void eng1_3ff30(Actor *p)
{
    Actor *child;
    unsigned char flags;
    RelativeMeasure m;
    long x, y, z;
    int i;
    int kind;
    int n;
    unsigned long hit;
    Actor *a;
    int left, right;

    flags = 5;
    child = p->link_130;
    eng1_interpret_signed_pairs(p);
    eng1_2ad70((FixedPosition *)g_native_pointer_7217c, p, &m);
    switch (p->state) {
    case 0:
        break;
    case 1:
        p->motion_y_velocity += p->motion_y_rate - p->motion_y_unknown;
        if (p->motion_y_velocity < 0)
            p->motion_y_velocity = 0;
        if (p->motion_y_velocity > p->motion_y_target)
            p->motion_y_velocity = p->motion_y_target;
        if (p->direction_y)
            p->y -= p->motion_y_velocity;
        else
            p->y += p->motion_y_velocity;
        if (p->motion_y_velocity == 0) {
            p->motion_y_rate = p->motion_y_unknown;
            p->motion_y_unknown = 0;
            p->direction_y = 0;
        }
        if (child && child->unknown_114 < 3 && !p->direction_y && p->y > 0x3500000)
            select_frame(child, 3);
        if (p->y > 0x3880000) {
            p->y = 0x3880000;
            p->state = 0;
        }
        break;
    case 2:
        eng1_native_code_target_000ef10(p);
        p->motion_y_velocity += p->motion_y_rate - p->motion_y_unknown;
        if (p->motion_y_velocity < 0)
            p->motion_y_velocity = 0;
        if (p->motion_y_velocity > p->motion_y_target)
            p->motion_y_velocity = p->motion_y_target;
        if (p->direction_y)
            p->y -= p->motion_y_velocity;
        else
            p->y += p->motion_y_velocity;
        p->motion_z_velocity += p->motion_z_rate - p->motion_z_unknown;
        if (p->motion_z_velocity < 0)
            p->motion_z_velocity = 0;
        if (p->motion_z_velocity > p->motion_z_target)
            p->motion_z_velocity = p->motion_z_target;
        p->z -= p->motion_z_velocity;
        if (p->motion_y_velocity == 0) {
            p->motion_y_rate = p->motion_y_unknown;
            p->motion_y_unknown = 0;
            p->direction_y = 0;
            p->motion_z_unknown = p->motion_z_rate;
            p->motion_z_rate = 0;
        }
        if (!p->direction_y) {
            if (child && child->unknown_114 < 3)
                select_frame(child, 6);
            if (eng1_29e20(p)) {
                p->y &= 0xffff0000;
                p->motion_y_velocity = 0;
                p->motion_y_target = 0;
                p->motion_y_unknown = 0;
                p->motion_z_target = 0x60000;
                p->motion_z_rate = dispatch_engine_2c460(0x4000) + 0x7000;
                p->motion_z_unknown = dispatch_engine_2c460(0x4000) + 0x4000;
                p->direction_z = 0xff;
                p->state = 4;
                LinkedSoundFx_6da0(0x4854, p);
            }
        }
        break;
    case 4:
    case 5:
        if (!p->entries)
            attach_actor(p, 0x2032);
        if (p->lifetime) {
            p->lifetime--;
            flags = 0x28;
        }
        for (i = 0; i < g_actor_worklist_717c0[0];) {
            a = (Actor *)g_actor_worklist_717c0[++i];
            if (a != p && !a->lifetime && eng1_2a660(p, a)) {
                p->direction_x = ~p->direction_x;
                p->direction_z = ~p->direction_z;
                a->direction_x = ~a->direction_x;
                a->direction_z = ~a->direction_z;
                p->lifetime = dispatch_engine_2c460(10) + 10;
                a->lifetime = dispatch_engine_2c460(10) + 10;
                flags = 0;
            }
        }
        hit = eng1_26de0(p);
        if (hit) {
            p->lifetime = dispatch_engine_2c460(10) + 10;
            if (hit == 0x80) {
                n = eng1_19b40(~g_native_word_72168 & 0x5f);
                if (n) {
                    eng1_1aba0();
                    p->hits -= n;
                }
            } else {
                eng1_30290(hit, 0);
                p->hits -= 100;
            }
            if (p->hits <= 0) {
                p->flags &= ~0x200;
                x = p->x;
                y = p->y;
                z = p->z;
                eng1_27450(p);
                RemoveFxLinks_7ef6(p);
                LinkedSoundFx_6da0(0x4055, p);
                detach_attachment(p);
                select_frame(p, 4);
                for (i = 0; (kind = p->records_118[i].kind) != 0; i++) {
                    if (kind & 0x4000) {
                        a = spawn_middle_277e0((p->records_118[i].x << 16) + x, (p->records_118[i].y << 16) + y,
                                               (p->records_118[i].z << 16) + z, kind);
                        if (a) {
                            g_configuration_dword_7137c[kind & 0x1ff].initialize(a);
                            if ((n = p->records_118[i].sprite) != 0) {
                                a->sprite = n;
                                a->direction_x = p->records_118[i].direction_x;
                            }
                        }
                    }
                    if (kind & 0x2000) {
                        a = eng1_27db0((p->records_118[i].x << 16) + x, (p->records_118[i].y << 16) + y,
                                       (p->records_118[i].z << 16) + z, kind);
                        if (a)
                            g_configuration_dword_7139c[kind & 0x1ff].initialize(a);
                    }
                }
                for (i = 0; i < 4; i++)
                    eng1_131c0(p->x, p->y - 0x200000, p->z, g_burst_sound_rows_66738[dispatch_engine_2c460(4)][i]);
            }
        }
        if (!p->lifetime) {
            if (eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
                eng1_native_code_target_00266a0(p, 3, 0);
                p->direction_x = ~p->direction_x;
                p->direction_z = ~p->direction_z;
                p->lifetime = 10;
                g_eng1_control_70ef0++;
                flags = 0;
            }
            left = g_configuration_dword_713bc[dispatch_local_2a170(p->x - 0x180000, p->y - 0x200000,
                                                                     p->z + 0x100000, p->profile)];
            right = g_configuration_dword_713bc[dispatch_local_2a170(p->x + 0x180000, p->y - 0x200000,
                                                                      p->z + 0x100000, p->profile)];
            if ((left & 7) || (right & 7)) {
                p->direction_x = ~p->direction_x;
                p->direction_z = ~p->direction_z;
                p->lifetime = 10;
                flags = 0;
            }
        }
        if (!eng1_sim_state_72110 && m.x < 0x140)
            eng1_2b5e0(p, g_native_pointer_7217c, flags);
        else
            advance_target_motion(p, flags);
        break;
    }
}

void eng1_405e0(Actor *p)
{
    RelativeMeasure m;
    Actor *child;
    Actor *a;
    int i;
    int n;
    int state;

    child = p->link_130;
    measure_relative_vector((FixedPosition *)g_native_pointer_7217c, (FixedPosition *)p, &m);
    n = 0;
    for (i = 0; i < g_actor_worklist_717c0[0];) {
        a = (Actor *)g_actor_worklist_717c0[++i];
        state = a->state;
        if (state > 1 && state < 9)
            n++;
    }
    switch (p->state) {
    case 0:
        if ((n == 0 && (abs(p->x - g_native_pointer_7217c->x) & ~0xffff) < 0x1000000) ||
            (n == 1 && (abs(p->x - g_native_pointer_7217c->x) & ~0xffff) < 0x1000000 &&
             !dispatch_engine_2c460(2))) {
            p->motion_y_velocity = p->motion_y_target = 0xe0000;
            p->motion_y_rate = 0;
            p->motion_y_unknown = dispatch_engine_2c460(0x2000) + 0x8000;
            p->direction_y = 0xff;
            p->state = 2;
            LinkedSoundFx_6da0(0x4853, p);
            if (child)
                select_frame(child, 1);
        } else {
            p->motion_y_velocity = p->motion_y_target = 0xa0000;
            p->motion_y_rate = 0;
            p->motion_y_unknown = dispatch_engine_2c460(0x2000) + 0x6000;
            p->direction_y = 0xff;
            p->state = 1;
            LinkedSoundFx_6da0(0x4852, p);
            if (child)
                select_frame(child, 1);
        }
        break;
    case 4:
        if ((unsigned short)m.distance < 0x80) {
            select_frame(p, 1);
            p->state = 5;
        }
        break;
    case 5:
        if ((unsigned short)m.distance < 0x80) {
            select_frame(p, 0);
            p->state = 4;
        }
        break;
    }
}
