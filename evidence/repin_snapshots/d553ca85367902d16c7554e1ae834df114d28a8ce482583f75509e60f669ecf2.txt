/* ENG1.DLL actor-region declarations shared by the boss-handler unit (region.c) and the actor-pipeline unit
   (region.cpp); lane x7 scratch, 2026-10-20. Actor offsets are provisional shared names; globals are named by
   RVA or by existing project names. */
#ifndef X7_ACTOR_REGION_H
#define X7_ACTOR_REGION_H
#include "../shared/engine_interface.h"

typedef struct Actor Actor;
typedef struct Entry {
    unsigned char unknown_00[0x0e];
    unsigned short tag;
} Entry;

typedef struct Motion {
    long velocity;
    long target;
    long rate;
    long unknown;
} Motion;

typedef struct Bounds {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds;

typedef union Spin { long value; short step; } Spin;
typedef struct Offset2 { short x, y; } Offset2;
typedef struct FixedPosition { long x, y, z; } FixedPosition;
typedef struct RelativeMeasure {
    short x, y, z;
    short planar_distance;
    short distance;
    unsigned short direction;
} RelativeMeasure;

struct Actor {
    long x, y, z;
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category;
    unsigned char mode;
    unsigned char unknown_024[0x2a - 0x24];
    unsigned short state;
    unsigned char unknown_02c[0x2e - 0x2c];
    unsigned short kind;
    unsigned char unknown_030[0x34 - 0x30];
    unsigned short sprite;
    unsigned short entry_count;
    unsigned short profile;
    unsigned short scale;
    unsigned char unknown_03c[0x4c - 0x3c];
    long parameter;
    unsigned long flags;
    unsigned long render_flags;
    long record_offset;
    Entry *entries;
    Actor *child;
    unsigned char unknown_064[0x70 - 0x64];
    short angle;
    short angle_72;
    short angle_74;
    unsigned char unknown_076[0x88 - 0x76];
    Bounds bounds;
    unsigned char direction_x;
    unsigned char direction_y;
    unsigned char direction_z;
    unsigned char unknown_097[0x9a - 0x97];
    short delay;
    long saved_x, saved_y, saved_z;
    Motion motion_x, motion_y, motion_z;
    Spin spin_x, spin_y, spin_z;
    unsigned char unknown_0e4[0xec - 0xe4];
    unsigned char lifetime;
    unsigned char lifetime_flag;
    short hits;
    unsigned char unknown_0f0[0x108 - 0xf0];
    unsigned long unknown_108;
    unsigned char unknown_10c[0x124 - 0x10c];
    short unknown_124;
    unsigned char unknown_126[0x12c - 0x126];
    long unknown_12c;
    long unknown_130;
};

typedef struct Callback8 { void (*initialize)(Actor *); void (*update)(Actor *); } Callback8;
typedef struct Callback12 { void (*initialize)(Actor *); void (*update)(Actor *); void (*draw)(Actor *); } Callback12;
typedef struct AltActor {
    long x, y, z;
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category;
    unsigned char unknown_023[0x2e - 0x23];
    unsigned short kind;
    unsigned char unknown_030[0x50 - 0x30];
    unsigned long flags;
    unsigned char unknown_054[0x94 - 0x54];
} AltActor;
typedef struct MiddleActor {
    long x, y, z;
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category;
    unsigned char unknown_023[0x2e - 0x23];
    unsigned short kind;
    unsigned char unknown_030[0x50 - 0x30];
    unsigned long flags;
    unsigned char unknown_054[0x9c - 0x54];
    long saved_x, saved_y;
    unsigned char unknown_0a4[0xec - 0xa4];
} MiddleActor;

typedef struct ResourceEntry { unsigned char state; unsigned char unknown_01[0x14 - 0x01]; } ResourceEntry;
typedef struct LevelProfile { unsigned char unknown_00[3]; unsigned char wrap_x; unsigned char unknown_04[0x1c - 0x04]; } LevelProfile;
typedef struct KindInfo { unsigned char *owner; long count; } KindInfo;
typedef struct ForestRecord {
    long x, y, z;
    unsigned short flags, state, kind, unknown_12;
} ForestRecord;
typedef struct ForestPlacement { unsigned short x, y, z, type; } ForestPlacement;
typedef struct SpatialType { short x, y, z; unsigned short flags, kind; } SpatialType;

extern long *g_script_cursor_6fcf4;
extern long *g_script_start_6fcf8;
extern unsigned long g_script_low_6fe34;
extern unsigned long g_script_high_6fe38;
extern KindInfo g_kind_info[];
extern unsigned short g_boss_state_6ffda;
extern unsigned short g_boss_script_5a060[];
extern unsigned short g_boss_script_index_71e24;
extern Actor *g_boss_71e28;
extern Callback8 *g_configuration_dword_7139c;
extern Actor *g_collision_context;
extern unsigned short g_eng1_setup_word_72012;
extern unsigned short g_native_word_72168;
extern long g_eng1_setup_dword_72188;
extern long g_boss_threshold_72184;
extern short g_native_word_70832;
extern unsigned short *g_configuration_dword_713bc;
extern Actor *g_pipeline_dword_71a18;
extern long g_eng1_setup_dword_7201c;

extern unsigned int random_limit(int);
extern unsigned int dispatch_engine_2c470(void);
extern Actor *spawn_kind(long, long, long, int);
extern void advance_frame(Actor *);
extern void select_frame(Actor *, int);
extern void eng1_interpret_signed_pairs(Actor *);
extern void eng1_native_code_target_002a290(Actor *);
extern int eng1_29c20(Actor *);
extern int eng1_26d50(Actor *);
extern int eng1_2a710(Bounds *, Actor *);
extern int eng1_2a660(Actor *, Actor *);
extern void eng1_20000(Actor *);
extern int remove_actor(Actor *);
long eng1_native_code_target_00266a0(Actor *, int, int);
extern void measure_absolute_vector(FixedPosition *, FixedPosition *, RelativeMeasure *);
extern long eng3_1f7b0(unsigned short, unsigned short);
extern long eng1_2bef0(long);
extern void spawn_actor_with_owner(Actor *, long, long, long, int);

extern long g_pipeline_dword_71680[];
extern unsigned short g_pipeline_word_6fec4;
extern unsigned char g_pipeline_actor_slots_71440[];
extern unsigned short g_boss_drop_count_6ffe4;
extern int eng1_2bd20(Actor *);
extern void eng1_31000(Actor *, Offset2 *);
extern void unlink_12c(Actor *);
extern void eng1_131c0(long, long, long, int);
extern int eng1_native_code_target_002aa30(Actor *, int, int);
extern Actor *spawn_at_group_300c0(long, long, long, int, int);
extern void play_sound(int, Actor *);
extern int remove_actor_variant(Actor *);
extern int eng1_30050(long, long, long);
extern int eng1_2a9e0(Actor *, int);
extern void release_actor(Actor *);
void attach_entries(Actor *, Entry *);
extern Entry g_entries_5a450[];
extern unsigned long g_pipeline_dword_71844;
extern unsigned short g_pipeline_word_713ae;
extern unsigned short g_pipeline_word_71376;
extern unsigned short g_pipeline_word_71ca0;
extern unsigned short g_pipeline_word_71740;
extern unsigned short g_pipeline_word_71720;
extern unsigned short g_pipeline_word_71cc0;
extern unsigned short g_pipeline_word_71c60;
extern long g_actor_worklist_714c0[];
extern long g_actor_worklist_71580[];
extern long g_actor_worklist_716c0[];
extern long g_actor_worklist_71760[];
extern long g_actor_worklist_717c0[];
extern long g_actor_worklist_71a20[];
extern long g_actor_worklist_71b20[];
extern long g_actor_worklist_71be0[];
extern unsigned short g_configuration_word_71378;
extern unsigned short g_configuration_word_713aa;
extern unsigned short g_configuration_word_7138c;
extern unsigned short g_configuration_word_713c4;
extern unsigned short g_configuration_word_713b8;
extern unsigned short g_configuration_word_71430;
extern unsigned short g_configuration_word_713b6;
extern unsigned short g_configuration_word_713a0;
extern unsigned short g_configuration_word_713a8;
extern AltActor *g_alternate_actor_pool;
extern MiddleActor *g_middle_actor_pool;
extern Actor *g_normal_actor_pool;
extern Callback8 *g_configuration_dword_7137c;
extern Callback12 *g_configuration_dword_71370;
extern void eng1_call_274e0(void);
extern void eng1_call_1fdf0(void);
extern void eng1_call_282e0(void);
extern void eng1_call_2af80(void);
extern void eng1_call_279f0(void);
extern void eng1_call_27fa0(void);
extern void eng1_call_01e50(void);
extern void eng1_call_12960(void);
extern void eng1_call_1fd20(void);
extern void reset_normal_actor_pool(void);
extern void reset_middle_actor_pool(void);
extern void reset_alternate_actor_pool(void);
void eng1_call_26040(void);
void dispatch_local_262a0(void);
extern unsigned short g_debris_sprites_601ac[];
extern unsigned char g_debris_sprites_5ef60[];

void eng1_24c60(Actor *);
unsigned short eng1_24680(void);
void eng1_24980(Actor *);
void eng1_241f0(Actor *, int, int, int);
extern unsigned long g_pipeline_dword_71840;
extern unsigned short g_pipeline_word_71bc4;
extern unsigned short g_pipeline_word_71c5c;
extern long g_pipeline_dword_716a8;
extern long g_pipeline_dword_71718;
extern short g_sine_table_5a848[];
extern ResourceEntry g_actor_resources[81];
extern unsigned char g_resource_state_snapshot_719c0[81];
void eng1_call_26480(void);
extern long g_actor_worklist_71460[];
extern long g_actor_worklist_71ce0[];
extern long g_actor_worklist_71900[];
extern unsigned short g_pipeline_word_6fec0;
extern Actor *g_pipeline_actor_7200c;
extern long g_pipeline_dword_71ae0;
extern LevelProfile g_level_profiles_5df08[];
extern unsigned short g_configuration_word_713b4;
extern unsigned short g_configuration_word_71432;
extern unsigned short g_configuration_word_713ba;
extern unsigned short actor_bounds_2a880(Actor *, Bounds *, int);
extern void eng1_call_26730(void);
extern SpatialType *g_configuration_dword_71398;
extern unsigned char *g_root_group_rows_70d5c[];
extern signed char g_root_count_70ed0;
extern void *engine_pool_allocate(void **, unsigned long, unsigned long);
extern long g_camera_x_70ca0;
extern long g_camera_center_x_70cdc;
extern Actor *g_frame_global_734f4;
extern signed char g_active_category_70cfe;
extern void eng1_call_270c0(ForestRecord *, int);
extern void eng1_call_27720(ForestRecord *, int);
extern void eng1_call_27c40(ForestRecord *, int);
extern void eng1_native_code_target_028290(unsigned long);
extern unsigned short eng1_sim_state_6feb4;
extern unsigned short g_hit_direction_6feb8;
extern short g_hit_value_6febc;
extern long g_hit_result_72008;
extern void measure_relative_vector(FixedPosition *, FixedPosition *, RelativeMeasure *);

#endif
