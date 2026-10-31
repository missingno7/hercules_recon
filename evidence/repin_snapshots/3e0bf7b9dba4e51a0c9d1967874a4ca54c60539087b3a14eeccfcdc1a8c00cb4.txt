/* ENG1.DLL 0x12620..0x14790: bonus-row driver, resource masks, alternate-row setup, pickup spawner and
 * handlers, checkpoint markers and the two lift objects (lane x19).  Plain C: 19/24 masked equal
 * (as C++: 20/24, but 0x13d70/0x142d0 lose the edi/ebp constant caching).  Types/idioms follow
 * calibration/eng1/player_scene_3a040.c and calibration/eng3/handlers_16240.cpp, whose 0x16f30,
 * 0x17090, 0x17130, 0x168b0, 0x168f0, 0x165f0 and 0x16670 are diverged variants of 0x12620,
 * 0x12790, 0x12960, 0x12a10, 0x12b00, 0x13990 and 0x139f0.
 * Measured spellings: negation is written ~x + 1 (0x12a10, 0x13d70, 0x142d0: -x gives neg);
 * (unsigned long)(x + y) >> 17 (shr); 0x131c0 reuses its int local for the 0xe0 field (sar).
 * Open residues (allocation/context): 0x12620 base/index order and one add/lea temp, 0x12960 base/index
 * (C++ equalises), 0x13880 one temp register, 0x13d70/0x142d0 ecx/edx swap of player pointer and d. */

typedef struct Actor Actor;
typedef struct ActorEntry {
    unsigned char unknown_00[0x0c];
    unsigned short sprite;
    unsigned short tag;
} ActorEntry;
typedef struct Bounds {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds;

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
    signed char difficulty_00e;
    unsigned char unknown_00f;
    unsigned char flag_010;
    unsigned char flags_011;
    unsigned char unknown_012[0x14 - 0x12];
    unsigned short count_014;
    unsigned short count_016;
    unsigned short count_018;
    unsigned short count_01a;
    unsigned short mask_01c;
    short target_01e;
    short value_020;
    short value_022;
    short energy_024[6];
    unsigned long cheats_030;
    unsigned char unknown_034[0x38 - 0x34];
    unsigned long tick_038;
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
    Actor *child;
    void *image_064;
    unsigned char unknown_068[0x70 - 0x68];
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
    unsigned char unknown_0f0[0x11e - 0xf0];
    short unknown_11e;
    unsigned char unknown_120[0x134 - 0x120];
};

/* 0x94-byte alternate-pool row (g_frame_owner_734e0[], rows 2..9 are the bonus row). */
typedef struct AltActor {
    long x, y, z;
    unsigned char unknown_00c[0x21 - 0x0c];
    unsigned char counter_021;
    unsigned char category;
    unsigned char mode;
    unsigned char unknown_024[0x2a - 0x24];
    unsigned short state;
    unsigned char unknown_02c[0x2e - 0x2c];
    unsigned short kind;
    unsigned char unknown_030[0x34 - 0x30];
    unsigned short sprite;
    unsigned char unknown_036[0x3e - 0x36];
    short unknown_03e;
    unsigned char unknown_040[0x4c - 0x40];
    long parameter;
    unsigned long flags;
    unsigned long render_flags;
    unsigned char unknown_058[0x64 - 0x58];
    void *image_064;
    unsigned char unknown_068[0x94 - 0x68];
} AltActor;

typedef struct FramePos { short x, y; } FramePos;
typedef struct LiftRow { short x, y, angle_x, angle_y, angle_z; } LiftRow;
typedef struct KindInfo { unsigned char *owner; long count; } KindInfo;
typedef struct HomeRecord { long x, y, z; unsigned short flags; } HomeRecord;
typedef struct ActorResource {      /* 20-byte resource slot, 0x5cee0 */
    void *path;
    unsigned short entry_count;
    unsigned short unknown_006;
    unsigned long file_length;
    void *references;
    unsigned short residency_state;
    unsigned short unknown_012;
} ActorResource;

extern EngineInterfaceView g_engine_interface;
extern AltActor *g_frame_owner_734e0;
extern Actor *g_frame_owner_72860;
extern Actor *g_native_pointer_7217c;
extern KindInfo g_kind_info[];
extern ActorResource g_actor_resources[];
extern long g_resource_ids_53800[];
extern void *g_bonus_images_537e0[];
extern long g_bonus_x_52b50[];
extern long g_bonus_sprites_52b70[];
extern short g_flash_sprites_534d0[];
extern unsigned long eng1_sim_state_72174;
extern unsigned long eng1_sim_state_72104;
extern short eng1_sim_state_6ef40;
extern unsigned char g_eng1_setup_byte_72010;
extern unsigned long eng1_sim_state_72110;
extern long g_spawn_argument_72198;
extern unsigned char eng1_frame_target_0535a8[];
extern unsigned char eng1_frame_target_0535b0[];
extern unsigned char eng1_frame_target_0535b8[];
extern unsigned char eng1_frame_target_053550[];

extern unsigned char g_eng1_setup_byte_7215c;
extern short g_eng1_setup_word_721a0;
extern short g_eng1_word_7144c;
extern long g_context_default_dword_72190;
extern long g_player_cruise_707e8;
extern LiftRow g_lift_rows_52c08[];
extern LiftRow g_lift_rows_530d8[];
extern unsigned short g_eng1_setup_word_6ef98;
extern short g_eng1_setup_word_6ef9c;
extern long g_lift_sound_6ef60;
extern Actor *g_eng1_spatial_match_actor_6fedc;
extern unsigned long eng1_sim_state_72170;
extern short *g_sine_table_70f34;
extern unsigned char eng1_frame_target_053548[];
extern unsigned char eng1_frame_target_053600[];
extern unsigned char eng1_frame_target_053370[];
extern unsigned char eng1_frame_target_0533c8[];
extern long g_context_default_dword_721a8;
typedef struct PickupType { long kind; void *image; } PickupType;
extern PickupType g_pickup_types_52b90[];

unsigned int random_limit(int limit);
void play_sound(unsigned short id, Actor *p);
Actor *spawn_at_group_300c0(long x, long y, long z, int kind, int group);
Actor *action_spawn_27d40(Actor *actor, unsigned long action);
Actor *spawn_middle_277e0(long x, long y, long z, unsigned short kind);
Actor *attach_actor(Actor *p, int tag);
unsigned int dispatch_engine_2c470(void);
void initialize_alternate_actor(AltActor *p);
void release_actor(Actor *p);
void reset_frame_state(Actor *p);
int eng1_interpret_word_stream(Actor *p);
void eng1_native_code_target_000ef10(Actor *p);
void eng1_native_code_target_002a290(Actor *p);
void resource_cancel(int id);
void resource_reset(int id);
void detach_child(Actor *p);
int remove_actor_variant(Actor *p);
unsigned long eng1_2bc50(Actor *p);
unsigned int dispatch_local_30010(long x, long y, long z, unsigned short mask);
int eng1_31000(Actor *p, FramePos *pos);
int actor_bounds_2a880(Actor *p, Bounds *b, int mode);
int planar_bounds_overlap(Bounds *a, Bounds *b);
long grid_scan_20b50(long x, long y, long z, int group);
void eng1_19240(void);
int eng1_2a950(Actor *p, int frame);
int eng1_14d80(Actor *p, long dx, long dy, int mode);
void eng1_20000(Actor *p);
void eng1_27410(Actor *p);
void change_kind(Actor *p, int kind);
void eng1_27450(Actor *p);
int eng1_200e0(Bounds *b, Actor *p);
int eng1_2a710(Bounds *b, Actor *p);
int eng1_2a9e0(Actor *p, int range);

void dispatch_local_12790(AltActor *a, int i);

/* ENG3 counterpart 0x16f30. */
void dispatch_local_12620(void)
{
    int i;
    AltActor *a;

    switch (eng1_sim_state_72174) {
    case 0:
        for (i = 2; i < 10; i++)
            g_frame_owner_734e0[i].y = 0;
        return;
    case 1:
        eng1_sim_state_72174++;
        eng1_sim_state_6ef40 = 0x8c;
        for (i = 0; i < 8; i++) {
            a = &g_frame_owner_734e0[i + 2];
            a->y = 0;
            a->state = i * 4;
            a->parameter = 0x460000;
            if (i == eng1_sim_state_72104) {
                a->counter_021 = 0x3c;
                a->image_064 = g_bonus_images_537e0[i];
                reset_frame_state((Actor *)a);
            } else {
                a->sprite = g_bonus_sprites_52b70[i];
                a->category = 2;
            }
        }
        break;
    case 2:
        if (--eng1_sim_state_6ef40 <= 0)
            eng1_sim_state_72174++;
        break;
    case 3:
        eng1_sim_state_72174++;
        eng1_sim_state_6ef40 = 0x64;
        for (i = 0; i < 8; i++) {
            a = &g_frame_owner_734e0[i + 2];
            a->state = i * 4;
            a->parameter = 0;
        }
        break;
    case 4:
        if (--eng1_sim_state_6ef40 <= 0)
            eng1_sim_state_72174 = 0;
        break;
    }
    for (i = 0; i < 8; i++)
        dispatch_local_12790(&g_frame_owner_734e0[i + 2], i);
}

/* ENG3 counterpart 0x17090. */
void dispatch_local_12790(AltActor *a, int i)
{
    int n;

    if (a->state) {
        if (--a->state == 0)
            play_sound(0xd, 0);
    } else {
        for (n = 0; n < 6; n++) {
            if (a->y < a->parameter)
                a->y += 0x10000;
            if (a->y > a->parameter) {
                a->y -= 0x10000;
                if (a->y <= 0xffff)
                    a->y = 0;
            }
        }
    }
    if (i == eng1_sim_state_72104) {
        if (g_actor_resources[g_resource_ids_53800[i]].residency_state != 3) {
            resource_reset(g_resource_ids_53800[i]);
            reset_frame_state((Actor *)a);
        }
        eng1_interpret_word_stream((Actor *)a);
        if (a->counter_021 == 0) {
            if (a->category == 2) {
                reset_frame_state((Actor *)a);
                resource_cancel(g_resource_ids_53800[i]);
            }
        } else {
            a->counter_021--;
        }
    }
    if (g_engine_interface.context_004->mask_01c & (1 << i))
        a->render_flags &= ~7;
    else
        a->render_flags |= 7;
    if (a->y)
        a->render_flags |= 0x80000000;
    else
        a->render_flags &= ~0x80000000;
}

void eng1_resource_reset_mask_128a0(unsigned char mask)
{
    int i;

    g_eng1_setup_byte_72010 |= mask;
    for (i = 0; i < 8; i++) {
        if ((g_eng1_setup_byte_72010 & (1 << i)) && !(g_engine_interface.context_004->mask_01c & (1 << i))
            && g_actor_resources[g_resource_ids_53800[i]].residency_state != 3)
            resource_reset(g_resource_ids_53800[i]);
    }
}

void eng1_resource_cancel_mask_12910(unsigned char mask)
{
    int i;

    g_eng1_setup_byte_72010 &= ~mask;
    for (i = 0; i < 8; i++) {
        if (mask & (1 << i))
            resource_cancel(g_resource_ids_53800[i]);
    }
}

/* ENG3 counterpart 0x17130. */
void eng1_call_12960(void)
{
    int i;
    AltActor *a;

    for (i = 0; i < 8; i++) {
        a = &g_frame_owner_734e0[i + 2];
        initialize_alternate_actor(a);
        a->flags = 6;
        a->kind = 0x200c;
        a->render_flags = 0xc0000000;
        a->unknown_03e = 4;
        a->mode = 6;
        a->sprite = g_bonus_sprites_52b70[i];
        a->category = 2;
        a->x = g_bonus_x_52b50[i];
        a->y = 0x500000;
        a->z = 0;
    }
}

Actor *eng1_129e0(long x, long y, long z, int kind, long argument)
{
    g_spawn_argument_72198 = argument;
    return spawn_at_group_300c0(x, y, z, kind, 0);
}

/* ENG3 counterpart 0x168b0 (first half). */
void eng1_native_code_target_012a10(Actor *o)
{
    o->render_flags |= 5;
    o->angle_74 += random_limit(0x400);
    o->unknown_04a = 0x80;
    reset_frame_state(o);
    o->state = (unsigned short)g_spawn_argument_72198;
    switch ((unsigned short)g_spawn_argument_72198) {
    case 0:
        o->image_064 = eng1_frame_target_0535a8;
        return;
    case 1:
        o->image_064 = eng1_frame_target_0535b0;
        o->parameter = 0x10000;
        return;
    case 2:
        o->image_064 = eng1_frame_target_0535b0;
        o->parameter = 0x20000;
        return;
    case 3:
        o->image_064 = eng1_frame_target_0535b0;
        o->parameter = 0x30000;
        return;
    case 4:
        o->image_064 = eng1_frame_target_0535b0;
        o->parameter = 0x40000;
        return;
    case 5:
        o->image_064 = eng1_frame_target_0535b8;
        o->parameter = 0x10000;
        if (random_limit(2))
            o->parameter = ~o->parameter + 1;
        return;
    default:
        o->image_064 = eng1_frame_target_0535a8;
        o->state = 0;
        o->parameter = 0;
        return;
    }
}

/* ENG3 counterpart 0x168f0. */
void eng1_native_code_target_012b00(Actor *o)
{
    long t[16];

    t[0] = 0;
    t[1] = 0;
    t[2] = 2;
    t[3] = 0;
    t[4] = 1;
    t[5] = 0;
    t[6] = 0;
    t[7] = 0;
    t[8] = 0;
    t[9] = 0;
    t[10] = -2;
    t[11] = 0;
    t[12] = -1;
    t[13] = 0;
    t[14] = 0;
    t[15] = 0;
    switch (o->state) {
    case 1:
    case 2:
    case 3:
    case 4:
        o->y += o->parameter;
        break;
    case 5:
        o->y += o->parameter;
        if (g_frame_owner_72860->x > o->x)
            o->x += 0x20000;
        else
            o->x -= 0x20000;
        break;
    }
    o->counter_021++;
    o->x += t[o->counter_021 & 0xf] << 16;
    o->angle_74 += 4;
    if (o->unknown_04a > 0xc)
        o->unknown_04a -= 0xc;
    else
        o->unknown_04a = 0;
    eng1_interpret_word_stream(o);
}

void eng1_native_code_target_012d10(Actor *o);

void eng1_native_code_target_012c00(Actor *o)
{
    unsigned short kind;
    Actor *c;

    reset_frame_state(o);
    o->unknown_032 = ((unsigned long)(o->x + o->y) >> 17) & 7;
    if (o->kind == 0x2012 && g_engine_interface.context_004->lives_00c == 9) {
        o->kind = 0x2013;
        o->image_064 = eng1_frame_target_053550;
    }
    if (o->kind == 0x2002 && g_engine_interface.context_004->target_01e == 0xff) {
        o->kind = 0x2013;
        o->image_064 = eng1_frame_target_053550;
    }
    kind = o->kind;
    if (kind == 0x2015 || kind == 0x2016 || kind == 0x2017) {
        c = action_spawn_27d40(o, 0xa007);
        if (c) {
            c->unknown_04a = 0x80;
            c->sprite = g_flash_sprites_534d0[2];
        }
        o->unknown_04a = 0xa0;
        o->render_flags |= 0x4000605;
    }
    if (kind == 0x200b) {
        if (o->record_offset) {
            o->state = *(unsigned short *)(g_kind_info[o->profile].owner + o->record_offset + 0xc) & 0x3f;
            o->image_064 = g_bonus_images_537e0[o->state];
            o->unknown_032 = 0;
            eng1_native_code_target_012d10(o);
        } else {
            release_actor(o);
        }
        return;
    }
    eng1_native_code_target_012d10(o);
}

void eng1_native_code_target_012d10(Actor *o)
{
    if (g_engine_interface.context_004->level_000 == 9)
        return;
    switch (o->kind) {
    case 0x2002:
    case 0x2012:
    case 0x2013:
    case 0x201a:
    case 0x201b:
    case 0x201c:
    case 0x201d:
        eng1_native_code_target_002a290(o);
    }
    eng1_native_code_target_000ef10(o);
}

int eng1_12ef0(Actor *o);

void eng1_native_code_target_012d80(Actor *o)
{
    int hit;

    if (o->control_025 & 3) {
        eng1_native_code_target_012d10(o);
        return;
    }
    if (o->kind == 0x200b && g_actor_resources[g_resource_ids_53800[o->state]].residency_state != 3) {
        resource_reset(g_resource_ids_53800[o->state]);
        reset_frame_state(o);
    }
    if (o->child)
        o->child->sprite = g_flash_sprites_534d0[o->unknown_032];
    if (g_engine_interface.context_004->level_000 == 7)
        hit = eng1_200e0(&g_native_pointer_7217c->bounds, o);
    else
        hit = eng1_2a710(&g_native_pointer_7217c->bounds, o);
    if (hit) {
        if (eng1_12ef0(o)) {
            eng1_27450(o);
            if (o->child)
                detach_child(o);
            change_kind(o, 0x2011);
            o->y -= 0x80000;
            return;
        }
    } else if (eng1_2a9e0(o, 0x100)) {
        if (o->kind == 0x200b && !(g_eng1_setup_byte_72010 & (1 << o->state)))
            resource_cancel(g_resource_ids_53800[o->state]);
        if (o->child)
            detach_child(o);
        release_actor(o);
    }
    eng1_interpret_word_stream(o);
}

void eng1_native_code_target_012ed0(Actor *o)
{
    o->unknown_04a = 0xa0;
    o->render_flags |= 0x4000605;
    reset_frame_state(o);
}

int eng1_12ef0(Actor *o)
{
    int taken;
    unsigned short sound;
    unsigned short kind;

    taken = 0;
    sound = 0;
    if (eng1_sim_state_72110)
        return 0;
    kind = o->kind;
    if (kind == 0x4012)
        kind = o->state;
    switch (kind) {
    case 0x2019:
        taken = 1;
        sound = 0xc03f;
        if (g_engine_interface.context_004->count_014 < 1000)
            g_engine_interface.context_004->count_014++;
        break;
    case 0x2018:
        taken = 1;
        sound = 0xc03f;
        if (g_engine_interface.context_004->count_018 < 1000)
            g_engine_interface.context_004->count_018++;
        break;
    case 0x2013:
        g_engine_interface.context_004->energy_024[0] += 0x20;
        if (g_engine_interface.context_004->energy_024[0] > g_engine_interface.context_004->target_01e)
            g_engine_interface.context_004->energy_024[0] = g_engine_interface.context_004->target_01e;
        taken = 1;
        if (!g_eng1_setup_byte_7215c) {
            sound = 0xc03b;
            g_eng1_setup_byte_7215c = 0x32;
        } else {
            sound = 0xc03f;
        }
        break;
    case 0x2002:
        g_engine_interface.context_004->target_01e += 0x20;
        if (g_engine_interface.context_004->target_01e > 0xff)
            g_engine_interface.context_004->target_01e = 0xff;
        taken = 1;
        sound = 0xc03e;
        break;
    case 0x2015:
        g_engine_interface.context_004->energy_024[1] += 0x80;
        if (g_engine_interface.context_004->energy_024[1] > 0xff)
            g_engine_interface.context_004->energy_024[1] = 0xff;
        taken = 1;
        sound = 0xc03e;
        break;
    case 0x2016:
        g_engine_interface.context_004->energy_024[2] += 0x80;
        if (g_engine_interface.context_004->energy_024[2] > 0xff)
            g_engine_interface.context_004->energy_024[2] = 0xff;
        taken = 1;
        sound = 0xc03e;
        break;
    case 0x2017:
        g_engine_interface.context_004->energy_024[3] += 0x80;
        if (g_engine_interface.context_004->energy_024[3] > 0xff)
            g_engine_interface.context_004->energy_024[3] = 0xff;
        taken = 1;
        sound = 0xc03e;
        break;
    case 0x2014:
        g_engine_interface.context_004->energy_024[4] += 0x80;
        if (g_engine_interface.context_004->energy_024[4] > 0xff)
            g_engine_interface.context_004->energy_024[4] = 0xff;
        taken = 1;
        sound = 0xc03e;
        break;
    case 0x2012:
        g_engine_interface.context_004->lives_00c++;
        if (g_engine_interface.context_004->lives_00c > 9)
            g_engine_interface.context_004->lives_00c = 9;
        g_eng1_setup_word_721a0 = 1;
        taken = 1;
        sound = 0xc03d;
        break;
    case 0x200b:
        eng1_sim_state_72104 = o->state;
        eng1_sim_state_72174 = 1;
        g_engine_interface.context_004->mask_01c |= 1 << o->state;
        taken = 1;
        sound = 0xc03c;
        break;
    case 0x201a:
        g_engine_interface.context_004->flags_011 |= 1;
        taken = 1;
        sound = 0xc03e;
        break;
    case 0x201b:
        g_engine_interface.context_004->flags_011 |= 2;
        taken = 1;
        sound = 0xc03e;
        break;
    case 0x201c:
        g_engine_interface.context_004->flags_011 |= 4;
        taken = 1;
        sound = 0xc03e;
        break;
    case 0x201d:
        g_engine_interface.context_004->flags_011 |= 8;
        taken = 1;
        sound = 0xc03e;
        break;
    }
    if (sound)
        play_sound(sound, o);
    g_eng1_word_7144c = 0;
    return taken;
}

Actor *eng1_131c0(long x, long y, long z, unsigned long spec)
{
    Actor *p;
    Actor *c;
    unsigned short type;
    int n;

    p = spawn_middle_277e0(x, y, z, 0x4012);
    if (p) {
        p->delay = 3;
        p->unknown_097 = (unsigned char)(spec >> 16);
        if (!p->unknown_097)
            p->unknown_097 = 0xfa;
        p->parameter = (spec >> 24) & 0x1f;
        if (spec & 0x2000)
            p->parameter = random_limit(0x14);
        type = spec & 0xf;
        if (type == 4 && g_engine_interface.context_004->lives_00c == 9)
            type = 3;
        if (type == 4) {
            if ((g_engine_interface.context_004->difficulty_00e == 0 && g_context_default_dword_72190 > 1)
                || (g_engine_interface.context_004->difficulty_00e >= 1 && g_context_default_dword_72190))
                type = 3;
            if (g_context_default_dword_72190 <= 5)
                g_context_default_dword_72190++;
        }
        if (type == 10 && g_engine_interface.context_004->target_01e == 0xff)
            type = 3;
        if (type == 11 || type == 12 || type == 13) {
            c = action_spawn_27d40(p, 0xa007);
            if (c) {
                c->unknown_04a = 0x80;
                c->sprite = g_flash_sprites_534d0[2];
            }
            p->unknown_04a = 0xa0;
            p->render_flags |= 0x4000605;
        }
        n = 0;
        if (type == 9) {
            if (g_context_default_dword_721a8 <= 1) {
                type = 5;
                g_context_default_dword_721a8 = 2;
            } else if (g_context_default_dword_721a8 == 2) {
                type = 6;
                g_context_default_dword_721a8 = 4;
            } else if (g_context_default_dword_721a8 == 4) {
                type = 7;
                g_context_default_dword_721a8 = 8;
            } else if (g_context_default_dword_721a8 == 8) {
                type = 8;
                g_context_default_dword_721a8 = 0x10;
            } else {
                type = 2;
            }
        }
        switch (type) {
        case 5:
            n = 1;
            break;
        case 6:
            n = 2;
            break;
        case 7:
            n = 4;
            break;
        case 8:
            n = 8;
            break;
        }
        if (g_engine_interface.context_004->flags_011 & n)
            type = 2;
        if (type == 1 && g_engine_interface.context_004->count_016 < 1000)
            g_engine_interface.context_004->count_016++;
        if (type == 2 && g_engine_interface.context_004->count_01a < 1000)
            g_engine_interface.context_004->count_01a++;
        if (spec & 0x1000) {
            spec = (spec & ~0x460) | 0x880;
            p->direction_z = 0xff;
            p->motion_z_target = 0x80000;
            p->motion_z_unknown = 0x4000;
            p->motion_z_rate = 0;
            p->motion_z_velocity = p->motion_z_target;
        }
        p->state = (unsigned short)g_pickup_types_52b90[type].kind;
        p->image_064 = g_pickup_types_52b90[type].image;
        p->counter_021 = 1;
        if (g_engine_interface.context_004->level_000 != 7 && g_engine_interface.context_004->level_000 != 6)
            p->z = (p->z + 0x400000) & 0xf800000;
        p->direction_y = 0xff;
        p->unknown_032 = ((unsigned long)(p->x + p->y) >> 17) & 0xf;
        if (spec & 0x800) {
            p->motion_y_target = 0x60000;
            p->motion_y_unknown = 0x5000;
        } else if (spec & 0x400) {
            p->motion_y_target = 0x40000;
            p->motion_y_unknown = 0x3000;
        } else {
            p->motion_y_target = ((dispatch_engine_2c470() & 0xff) + 0x500) << 8;
            p->motion_y_unknown = random_limit(0x1000) + 0x4000;
        }
        p->motion_y_velocity = p->motion_y_target;
        p->motion_y_rate = 0;
        n = spec & 0xe0;
        if (n) {
            p->direction_x = n >> 6;
            if (p->direction_x > 1)
                p->direction_x = random_limit(2);
            if (spec & 0x200)
                p->motion_x_target = 0x30000;
            else if (spec & 0x100)
                p->motion_x_target = 0x10000;
            else
                p->motion_x_target = ((dispatch_engine_2c470() & 0x1ff) + 0x80) << 8;
            p->motion_x_unknown = 0x1000;
            p->motion_x_rate = 0;
            p->motion_x_velocity = p->motion_x_target;
        }
        reset_frame_state(p);
        attach_actor(p, 0x2023);
    }
    return p;
}

void eng1_native_code_target_013570(Actor *o)
{
    unsigned long hit;
    Actor *c;
    unsigned int left, right;

    if (o->parameter) {
        o->parameter--;
        if (eng1_2a9e0(o, 0x100)) {
            if (o->child)
                detach_child(o);
            remove_actor_variant(o);
        }
        return;
    }
    eng1_interpret_word_stream(o);
    if (o->child)
        o->child->sprite = g_flash_sprites_534d0[o->unknown_032];
    if (g_engine_interface.context_004->level_000 == 9)
        o->x += g_player_cruise_707e8;
    hit = eng1_2bc50(o);
    if (hit) {
        if (o->image_064 == eng1_frame_target_053548)
            o->image_064 = eng1_frame_target_053550;
    } else if (o->direction_y) {
        if (o->motion_y_velocity > 0x20000 || !(g_engine_interface.context_004->tick_038 & 3)) {
            c = eng1_129e0(o->x + ((random_limit(12) - 6) << 16), o->y + 0x20000, o->z, 0x2010, 0);
            if (c) {
                c->state = 0;
                c->image_064 = eng1_frame_target_0535b0;
            }
            if (o->counter_021) {
                c = eng1_129e0(o->x + ((random_limit(12) - 6) << 16), o->y + 0x20000, o->z, 0x2010, 1);
                if (c) {
                    c->state = 1;
                    c->image_064 = eng1_frame_target_0535b0;
                }
            }
        }
    } else {
        o->counter_021 = 0;
    }
    if (hit & 3) {
        if (o->delay)
            o->delay--;
        if (!(hit & 2)) {
            switch (o->delay) {
            case 1:
                play_sound(0x335, o);
                break;
            case 2:
                play_sound(0x334, o);
                break;
            default:
                play_sound(0x336, o);
                break;
            }
        }
    }
    left = dispatch_local_30010(o->x - 0x200000, o->y - 0x10000, o->z, 1);
    right = dispatch_local_30010(o->x + 0x200000, o->y - 0x10000, o->z, 1);
    if (left && right)
        return;
    if (left)
        o->x += 0x20000;
    if (right)
        o->x -= 0x20000;
    if (g_engine_interface.context_004->level_000 == 9 && o->motion_y_velocity > 0)
        o->delay = 0;
    if (eng1_2a710(&g_native_pointer_7217c->bounds, o)) {
        if (o->delay <= 2 && eng1_12ef0(o)) {
            spawn_at_group_300c0(o->x, o->y - 0x80000, o->z, 0x2011, 0);
            if (o->child)
                detach_child(o);
            remove_actor_variant(o);
        }
    } else if (--o->unknown_097 == 0) {
        spawn_at_group_300c0(o->x, o->y - 0x100000, o->z, 0x2011, 0);
        if (o->child)
            detach_child(o);
        remove_actor_variant(o);
    } else if (eng1_2a9e0(o, 0x100)) {
        if (o->child)
            detach_child(o);
        remove_actor_variant(o);
    }
}

void eng1_native_code_target_013840(Actor *o)
{
    o->flags = 0x802;
    reset_frame_state(o);
    eng1_interpret_word_stream(o);
    eng1_native_code_target_002a290(o);
    eng1_native_code_target_000ef10(o);
}

void eng1_native_code_target_013880(Actor *o)
{
    FramePos pos;

    if (o->control_025 & 3) {
        eng1_native_code_target_002a290(o);
        return;
    }
    eng1_31000(o, &pos);
    if (!eng1_sim_state_72170) {
        actor_bounds_2a880(o, &o->bounds, 0);
        if (planar_bounds_overlap(&g_native_pointer_7217c->bounds, &o->bounds)) {
            play_sound(0x4012, g_native_pointer_7217c);
            eng1_19240();
            o->image_064 = eng1_frame_target_053600;
            reset_frame_state(o);
            o->unknown_04a = 0x80;
            o->render_flags |= 5;
        }
        if (g_engine_interface.context_004->tick_038 & 1)
            eng1_129e0(o->x + ((random_limit(0) + pos.x) << 16), o->y + ((pos.y + 8) << 16), o->z, 0x2010, 0);
    }
    if (eng1_2a9e0(o, 0x140))
        release_actor(o);
    eng1_interpret_word_stream(o);
}

/* ENG3 counterpart 0x165f0 (diverged). */
void eng1_native_code_target_013990(Actor *o)
{
    o->flags = 0x802;
    if (((o->kind + 6) & 0xf) == g_engine_interface.context_004->next_002)
        o->image_064 = eng1_frame_target_053370;
    else
        o->image_064 = eng1_frame_target_0533c8;
    o->parameter = 1;
    reset_frame_state(o);
    eng1_interpret_word_stream(o);
    eng1_native_code_target_002a290(o);
}

/* ENG3 counterpart 0x16670 (diverged). */
void eng1_native_code_target_0139f0(Actor *o)
{
    FramePos pos;
    long h;

    if ((o->control_025 & 3) && g_engine_interface.context_004->level_000 != 9) {
        eng1_native_code_target_002a290(o);
        return;
    }
    actor_bounds_2a880(o, &o->bounds, 0);
    if (o->image_064 == eng1_frame_target_0533c8) {
        if (g_engine_interface.context_004->next_002 == ((o->kind + 6) & 0xf)
            || planar_bounds_overlap(&g_native_pointer_7217c->bounds, &o->bounds)) {
            play_sound(0x28, 0);
            o->image_064 = eng1_frame_target_053370;
            g_engine_interface.context_004->next_002 = (o->kind + 6) & 0xf;
            reset_frame_state(o);
        } else if (g_engine_interface.context_004->level_000 != 9) {
            h = grid_scan_20b50(o->x >> 16, o->y >> 16, o->z >> 16, 0);
            o->parameter += 4;
            o->y += ((g_sine_table_70f34[(o->parameter & 0xff) * 16] & ~0x1ff) + ((h - 0x10) << 9)) << 7;
        }
    } else if (g_engine_interface.context_004->next_002 != ((o->kind + 6) & 0xf)) {
        o->image_064 = eng1_frame_target_0533c8;
        reset_frame_state(o);
    }
    eng1_interpret_word_stream(o);
    if (g_engine_interface.context_004->next_002 == ((o->kind + 6) & 0xf)) {
        eng1_31000(o, &pos);
        eng1_129e0(o->x + ((random_limit(5) + pos.x) << 16), o->y + ((random_limit(5) + pos.y) << 16), o->z, 0x2010, 2);
        if (g_engine_interface.context_004->tick_038 & 1) {
            eng1_129e0(o->x + ((random_limit(2) + pos.x) << 16), o->y + ((random_limit(2) + pos.y) << 16), o->z, 0x2010, 0);
            eng1_129e0(o->x + ((random_limit(5) + pos.x) << 16), o->y + ((random_limit(5) + pos.y) << 16), o->z, 0x2010, 1);
        }
        if (g_engine_interface.context_004->level_000 != 9) {
            h = grid_scan_20b50(o->x >> 16, o->y >> 16, o->z >> 16, 0) << 16;
            if (h > 0x320000)
                o->y += 0x10000;
            if (h < 0x320000)
                o->y -= 0x10000;
        }
    }
    if (eng1_2a9e0(o, 0x140))
        remove_actor_variant(o);
}

void set_mode_13c90(Actor *o)
{
    o->flags = 0x41;
}

void eng1_native_code_target_013ca0(Actor *o)
{
    switch (g_engine_interface.context_004->level_000) {
    case 1:
        o->sprite = 0x38;
        break;
    case 3:
        o->sprite = 2;
        break;
    }
    o->render_flags |= 0x10000000;
    o->flags |= 0x140000;
    o->direction_z = 0xff;
    o->direction_y = 0xff;
    o->unknown_0d8 = 7;
    o->unknown_11e = 3;
    o->state = 1;
    o->lifetime = 2;
    o->unknown_0ed = 10;
    o->motion_y_target = 0x60000;
    o->motion_y_velocity = 0;
    o->motion_y_unknown = 0x6000;
    o->delay = 0;
    eng1_2a950(o, 0);
    eng1_native_code_target_002a290(o);
    o->y -= g_lift_rows_52c08[0].angle_y << 16;
    o->motion_y_velocity = o->z;
    eng1_2a950(o, 1);
}

void eng1_native_code_target_013d70(Actor *o)
{
    long d;

    o->render_flags &= ~0x30;
    if (o->control_025 & 1) {
        eng1_native_code_target_002a290(o);
        if (o->control_025 & 1)
            return;
        o->y -= g_lift_rows_52c08[0].angle_y << 16;
        return;
    }
    if (o->delay == 0) {
        o->flags = (o->flags & ~0x800000) | 0x140000;
        eng1_native_code_target_002a290(o);
        o->y -= g_lift_rows_52c08[0].angle_y << 16;
        return;
    }
    if (o->delay == 1) {
        o->flags &= ~0x140000;
        if (g_eng1_setup_word_6ef98 == 0x2f) {
            o->delay = 2;
        } else if (g_eng1_setup_word_6ef98 == 0x30) {
            o->delay = 4;
        } else if (g_eng1_setup_word_6ef98 == 0) {
            eng1_native_code_target_002a290(o);
            o->y -= g_lift_rows_52c08[0].angle_y << 16;
            return;
        } else {
            o->saved_x = o->x;
            d = g_lift_rows_52c08[0].x;
            if (!g_frame_owner_72860->direction_x)
                d = g_lift_rows_52c08[0].y;
            d <<= 16;
            o->y = (g_lift_rows_52c08[g_eng1_setup_word_6ef98].y << 16) - (g_lift_rows_52c08[0].angle_y << 16)
                + g_frame_owner_72860->y;
            o->angle_70 = g_lift_rows_52c08[g_eng1_setup_word_6ef98].angle_x;
            o->angle_72 = g_lift_rows_52c08[g_eng1_setup_word_6ef98].angle_y;
            o->angle_74 = g_lift_rows_52c08[g_eng1_setup_word_6ef98].angle_z;
            if (g_frame_owner_72860->direction_x) {
                o->x = ~(g_lift_rows_52c08[g_eng1_setup_word_6ef98].x << 16) + d + g_frame_owner_72860->x;
                o->angle_72 = ~o->angle_72 + 1;
            } else {
                o->x = (g_lift_rows_52c08[g_eng1_setup_word_6ef98].x << 16) + d + g_frame_owner_72860->x;
            }
            o->angle_72 += g_eng1_setup_word_6ef9c;
            if (eng1_14d80(o, 0x200000, 0, 7) && g_eng1_spatial_match_actor_6fedc != o)
                o->x = o->saved_x;
            return;
        }
    }
    if (o->delay == 2) {
        g_lift_sound_6ef60 = 0;
        o->direction_z = 0xff;
        o->direction_y = 0xff;
        o->direction_x = g_frame_owner_72860->direction_x;
        o->motion_y_target = 0x10000;
        o->motion_y_velocity = 0x10000;
        o->motion_x_target = 0x40000;
        o->motion_x_velocity = 0x40000;
        o->motion_x_unknown = 0x1000;
        o->motion_y_unknown = 0x6000;
        o->motion_y_rate = 0;
        o->motion_z_velocity = 0;
        o->motion_z_target = 0;
        o->delay = 3;
        o->flags |= 0x940000;
    }
    if (o->delay == 3) {
        if (o->angle_74 < 0) {
            o->angle_74 += 2;
            if (o->angle_74 > 0)
                o->angle_74 = 0;
        }
        if (o->angle_74 > 0) {
            o->angle_74 -= 2;
            if (o->angle_74 > 0x7f)
                o->angle_74 = 0;
        }
        o->y += g_lift_rows_52c08[0].angle_y << 16;
        o->saved_x = o->x;
        if ((eng1_2bc50(o) & 1) && !g_lift_sound_6ef60) {
            play_sound(0x404c, o);
            g_lift_sound_6ef60 = 1;
        }
        if (eng1_2bc50(o) & 2) {
            o->delay = 0;
            o->y -= g_lift_rows_52c08[0].angle_y << 16;
        } else {
            o->y -= g_lift_rows_52c08[0].angle_y << 16;
            if (!o->direction_y) {
                o->motion_y_target = 0xa0000;
                o->motion_y_rate = 0x40000;
                o->motion_y_unknown = 0;
            }
        }
        if (eng1_14d80(o, 0x200000, 0, 7) && g_eng1_spatial_match_actor_6fedc != o)
            o->x = o->saved_x;
    }
    if (o->delay == 4) {
        o->direction_z = 0;
        o->direction_y = 0xff;
        o->direction_x = g_frame_owner_72860->direction_x;
        o->flags = (o->flags & ~0x140000) | 0x800000;
        o->motion_x_target = 0;
        o->motion_x_velocity = 0;
        o->motion_y_target = 0x80000;
        o->motion_y_velocity = 0x80000;
        o->motion_y_unknown = 0x6000;
        o->motion_y_rate = 0;
        o->motion_z_velocity = 0x80000;
        o->motion_z_target = 0x80000;
        o->motion_x_unknown = 0x3000;
        o->motion_x_rate = 0;
        o->delay = 5;
    }
    if (o->delay == 5) {
        o->y += g_lift_rows_52c08[0].angle_y << 16;
        if (eng1_2bc50(o) & 2) {
            eng1_20000(o);
            eng1_27410(o);
            return;
        }
        o->y -= g_lift_rows_52c08[0].angle_y << 16;
        if (!o->direction_y) {
            o->motion_y_target = 0xa0000;
            o->motion_y_rate = 0x40000;
            o->motion_y_unknown = 0;
        }
    }
}

void eng1_native_code_target_014210(Actor *o)
{
    o->sprite = 0x11;
    o->render_flags |= 0x10000000;
    o->flags |= 0x140000;
    o->direction_z = 0xff;
    o->direction_y = 0xff;
    o->unknown_0d8 = 7;
    o->unknown_11e = 3;
    o->state = 1;
    o->lifetime = 2;
    o->unknown_0ed = 10;
    o->motion_y_target = 0x60000;
    o->motion_y_velocity = 0;
    o->motion_y_unknown = 0x6000;
    o->delay = 0;
    eng1_2a950(o, 0);
    eng1_native_code_target_002a290(o);
    o->y -= g_lift_rows_530d8[0].angle_y << 16;
    o->motion_y_velocity = o->z;
    eng1_2a950(o, 1);
}

void eng1_native_code_target_0142d0(Actor *o)
{
    long d;

    o->render_flags &= ~0x30;
    if (o->control_025 & 1) {
        eng1_native_code_target_002a290(o);
        if (o->control_025 & 1)
            return;
        o->y -= g_lift_rows_530d8[0].angle_y << 16;
        return;
    }
    if (o->delay == 0) {
        o->flags = (o->flags & ~0x800000) | 0x140000;
        eng1_native_code_target_002a290(o);
        o->y -= g_lift_rows_530d8[0].angle_y << 16;
        return;
    }
    if (o->delay == 1) {
        o->flags &= ~0x140000;
        if (g_eng1_setup_word_6ef98 == 0x2f) {
            o->delay = 2;
        } else if (g_eng1_setup_word_6ef98 == 0x30) {
            o->delay = 4;
        } else if (g_eng1_setup_word_6ef98 == 0) {
            eng1_native_code_target_002a290(o);
            o->y -= g_lift_rows_530d8[0].angle_y << 16;
            return;
        } else {
            o->saved_x = o->x;
            d = g_lift_rows_530d8[0].x;
            if (!g_frame_owner_72860->direction_x)
                d = g_lift_rows_52c08[0].y;
            d <<= 16;
            o->y = (g_lift_rows_52c08[g_eng1_setup_word_6ef98].y << 16) - (g_lift_rows_52c08[0].angle_y << 16)
                + g_frame_owner_72860->y;
            o->angle_70 = g_lift_rows_530d8[g_eng1_setup_word_6ef98].angle_x;
            o->angle_72 = g_lift_rows_530d8[g_eng1_setup_word_6ef98].angle_y;
            o->angle_74 = g_lift_rows_530d8[g_eng1_setup_word_6ef98].angle_z;
            if (g_frame_owner_72860->direction_x) {
                o->x = ~(g_lift_rows_530d8[g_eng1_setup_word_6ef98].x << 16) + d + g_frame_owner_72860->x;
                o->angle_72 = ~o->angle_72 + 1;
            } else {
                o->x = (g_lift_rows_530d8[g_eng1_setup_word_6ef98].x << 16) + d + g_frame_owner_72860->x;
            }
            o->angle_72 += g_eng1_setup_word_6ef9c;
            if (eng1_14d80(o, 0x200000, 0, 7) && g_eng1_spatial_match_actor_6fedc != o)
                o->x = o->saved_x;
            return;
        }
    }
    if (o->delay == 2) {
        g_lift_sound_6ef60 = 0;
        o->direction_z = 0xff;
        o->direction_y = 0xff;
        o->direction_x = g_frame_owner_72860->direction_x;
        o->motion_y_target = 0x10000;
        o->motion_y_velocity = 0x10000;
        o->motion_x_target = 0x40000;
        o->motion_x_velocity = 0x40000;
        o->motion_x_unknown = 0x1000;
        o->motion_y_unknown = 0x6000;
        o->motion_y_rate = 0;
        o->motion_z_velocity = 0;
        o->motion_z_target = 0;
        o->delay = 3;
        o->flags |= 0x940000;
    }
    if (o->delay == 3) {
        if (o->angle_72 < 0) {
            o->angle_72 += 6;
            if (o->angle_72 > 0)
                o->angle_72 = 0;
        }
        if (o->angle_72 > 0) {
            o->angle_72 -= 6;
            if (o->angle_72 > 0x7f)
                o->angle_72 = 0;
        }
        o->y += g_lift_rows_530d8[0].angle_y << 16;
        o->saved_x = o->x;
        if ((eng1_2bc50(o) & 1) && !g_lift_sound_6ef60) {
            play_sound(0x4151, o);
            g_lift_sound_6ef60 = 1;
        }
        if (eng1_2bc50(o) & 2) {
            o->delay = 0;
            o->y -= g_lift_rows_530d8[0].angle_y << 16;
        } else {
            o->y -= g_lift_rows_530d8[0].angle_y << 16;
            if (!o->direction_y) {
                o->motion_y_target = 0xa0000;
                o->motion_y_rate = 0x40000;
                o->motion_y_unknown = 0;
            }
        }
        if (eng1_14d80(o, 0x200000, 0, 7) && g_eng1_spatial_match_actor_6fedc != o)
            o->x = o->saved_x;
    }
    if (o->delay == 4) {
        o->direction_z = 0;
        o->direction_y = 0xff;
        o->direction_x = g_frame_owner_72860->direction_x;
        o->flags = (o->flags & ~0x140000) | 0x800000;
        o->motion_x_target = 0;
        o->motion_x_velocity = 0;
        o->motion_y_target = 0xc0000;
        o->motion_y_velocity = 0xc0000;
        o->motion_y_unknown = 0x6000;
        o->motion_y_rate = 0;
        o->motion_z_velocity = 0x40000;
        o->motion_z_target = 0x40000;
        o->motion_x_unknown = 0x6000;
        o->motion_x_rate = 0;
        o->delay = 5;
    }
    if (o->delay == 5) {
        o->y += g_lift_rows_530d8[0].angle_y << 16;
        if (o->x < 0x15600000)
            o->x += 0x20000;
        if (!(eng1_2bc50(o) & 1) && (o->z < 0xc00000 || o->y < 0x4400000)) {
            o->y -= g_lift_rows_530d8[0].angle_y << 16;
            if (!o->direction_y) {
                o->motion_y_target = 0xa0000;
                o->motion_y_rate = 0x40000;
                o->motion_y_unknown = 0;
            }
        } else {
            eng1_20000(o);
            eng1_27410(o);
        }
    }
}
