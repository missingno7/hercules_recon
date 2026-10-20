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
    unsigned long tick_038;
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
    long *script_10c;
    void *unknown_110;
    short unknown_114;
    unsigned char unknown_116[0x11e - 0x116];
    short unknown_11e;
    PendingActionGroup *group_120;
    short unknown_124;
    unsigned char unknown_126[0x128 - 0x126];
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
