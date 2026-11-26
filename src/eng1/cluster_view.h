/* Native field view for ENG1 0x21f60..0x24090. Unknown members retain offsets.
 * No storage or capacities are inferred by this declaration. */
#ifndef ENG1_CLUSTER_VIEW_H
#define ENG1_CLUSTER_VIEW_H
typedef struct ClusterActor ClusterActor;
struct ClusterActor {
    long x,y,z;
    unsigned char unknown_0c[0x21-0x0c];
    unsigned char bounce,category,mode;
    unsigned char unknown_24[6];
    unsigned short state;
    unsigned short unknown_2c,kind;
    short counter,index;
    unsigned short sprite,entry_count,profile;
    unsigned short scale;
    unsigned short range;
    unsigned char unknown_3e[0x4a-0x3e];
    unsigned short opacity;
    long parameter;
    unsigned long flags,render_flags;
    long record_offset;
    void *entries;
    ClusterActor *child;
    long *cursor;
    long unknown_68;
    long unknown_6c;
    short angle,angle_y,angle_z;
    unsigned char unknown_76[0x94-0x76];
    unsigned char direction;
    unsigned char unknown_95[0xa8-0x95];
    long vx,unknown_ac,unknown_b0,unknown_b4;
    long vy,unknown_bc,unknown_c0,gravity;
    long vz,unknown_cc,unknown_d0,unknown_d4;
    long home_x,home_y,unknown_e0;
    unsigned char unknown_e4[8];
    unsigned char lifetime,lifetime_flag;
    short hits;
    unsigned char unknown_f0[0x108-0xf0];
    long phase;
    long *program;
    unsigned char unknown_110[0x120-0x110];
    ClusterActor **group;
    unsigned char unknown_124[0x134-0x124];
};
typedef struct ClusterGroup {
    ClusterActor *bottom,*lower,*middle,*top,*marker;
    long phase,sound;
} ClusterGroup;
typedef struct ClusterState { long state,index; ClusterActor *target; } ClusterState;
typedef struct ClusterCallbacks { void (*slot[160])(ClusterActor *); } ClusterCallbacks;
extern ClusterActor *g_frame_owner_72860;
extern ClusterActor *g_cluster_actor_6fe3c,*g_cluster_origin_6fe24;
extern ClusterGroup g_cluster_groups_6fd00[];
extern ClusterState g_cluster_left_6fdf0,g_cluster_right_6fdfc;
extern long g_cluster_count_6fe20,g_cluster_hit_6fe10,g_cluster_timer_6fe2c;
extern long g_cluster_once_6fe14,g_cluster_choice_6fe18,g_cluster_last_6fe54;
extern long g_cluster_tick_6fe30,g_cluster_sounds_6fe50;
extern long g_cluster_effect_6fde0,g_cluster_bosses_6fde8,g_cluster_last_6fe58;
extern unsigned short g_configuration_word_713b4;
extern long g_camera_x_70ca0,g_camera_offset_70cc4;
extern long g_boss_threshold_72184,g_eng1_setup_dword_72014,g_eng1_setup_dword_72110;
extern short *g_sine_table_70f34;
extern long g_cluster_program_58d78[],g_cluster_program_58d88[],g_cluster_program_58d98[];
extern long g_cluster_program_58e68[],g_cluster_program_58eb8[],g_cluster_program_58ef8[];
extern long g_cluster_program_58f18[],g_cluster_program_58f80[],g_cluster_program_58f98[];
extern long g_cluster_program_59008[],g_cluster_program_59078[],g_cluster_program_590d0[],g_cluster_program_59140[];
extern unsigned int random_limit(int);
/* The caller uses the promoted C argument; the service consumes its low WORD. */
extern void LinkedSoundFx_6da0(int,void *);
extern void LinkedSoundFxAt_75e3(int,void *,void *);
extern void RemoveFxIdLinks_7f8e(int,void *);
extern void RemoveFxLinks_7ef6(void *);
extern long wrapped_delta(long,long);
extern long eng1_21630(int);
extern void title_0c4d0(int);
void eng1_native_code_target_0220c0(ClusterActor *);
void eng1_22190(ClusterActor *);
void eng1_22280(ClusterActor *);
void eng1_22400(int);
void eng1_22440(ClusterActor *,int,ClusterState *);
ClusterActor *eng1_22fe0(long,long,long,int,int);
void eng1_23040(void);
void eng1_23390(void *,long,long,long);
#endif
