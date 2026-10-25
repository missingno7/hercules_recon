/* ENG1.DLL 0x2c030..0x2e000: the unrecorded spans of the region (lane x5 scratch,
 * 2026-10-20), in address order. Semantics, types and names follow the ENG1
 * behavioral graph (src/eng1/behavioral/actor_control_update_2cd30.c,
 * actor_control_tail.c, mesh_traversal.c, mesh_emit.c, incomplete_bodies.c) and
 * the recovery.json callee symbols; views cover only the offsets these bodies use.
 *
 * The span probably crosses several original units: 0x2c030/0x2c090 sit beside
 * 0x2c060 wrapped_ratio (src/eng1/actor_handlers_mid.c), 0x2c180.. is the host
 * interface wrapper unit (TITLE u0c1f0 / ENG3 0x1f830 twin), 0x2cc50.. the actor
 * control unit and 0x2da90.. the mesh traversal unit.
 *
 * Count-only context headers (<stdio.h>, <math.h>) are omitted per owner guidance 2026-10-20 (header effects
 * need independent evidence); with them 0x2cd30 is masked equal and 0x2df20 size-exact. <stdlib.h> declares the abs() intrinsic. */
#include <stdlib.h>
typedef struct EngineContext {
    signed char level;                  /* 0x00 */
    unsigned char unknown_001[0x38 - 0x01];
    long tick_038;                      /* 0x38 */
    unsigned char unknown_03c[4];
    long unknown_040;                   /* 0x40 */
} EngineContext;

/* Engine interface record at ENG1 0x70f20 (ENG3 0x407a0); slots named by offset. */
typedef struct EngineInterface {
    unsigned long signature_000;
    EngineContext *context_004;
    unsigned long opaque_008[2];
    void *opaque_010[4];
    unsigned long opaque_020[7];
    int (__cdecl *slot_03c)(void);
    unsigned long opaque_040[3];
    int (__cdecl *slot_04c)(int, int);
    unsigned long opaque_050[11];
    int (__cdecl *slot_07c)(int, int, int);
    unsigned long opaque_080[4];
    int (__cdecl *slot_090)(void);
    unsigned long opaque_094[8];
    int (__cdecl *slot_0b4)(void);
    unsigned long opaque_0b8[9];
    int (__cdecl *slot_0dc)(void);
    unsigned long opaque_0e0[2];
    int (__cdecl *slot_0e8)(void);
    unsigned long opaque_0ec[7];
    int (__cdecl *slot_108)(int, int);
    unsigned long opaque_10c[35];
    int (__cdecl *slot_198)(int, int, int);
    unsigned long opaque_19c[4];
    int (__cdecl *slot_1ac)(void);
    unsigned long opaque_1b0[2];
    int (__cdecl *slot_1b8)(int);
    int (__cdecl *slot_1bc)(int);
    unsigned long opaque_1c0[6];
    int (__cdecl *slot_1d8)(int);
    unsigned long opaque_1dc[1];
    int (__cdecl *slot_1e0)(int);
    unsigned long opaque_1e4[1];
    int (__cdecl *slot_1e8)(int, int);
    unsigned long opaque_1ec[8];
    int (__cdecl *slot_20c)(int, int, int);
    unsigned long opaque_210[20];
    int (__cdecl *slot_260)(int);
    unsigned long opaque_264[3];
    int (__cdecl *slot_270)(int, int, int);
    int (__cdecl *slot_274)(int, int, int);
    unsigned long opaque_278[5];
    int (__cdecl *slot_28c)(int, int);
    unsigned long opaque_290[5];
    int (__cdecl *slot_2a4)(int, int);
    unsigned long opaque_2a8[2];
    int (__cdecl *slot_2b0)(int, int, int, int);
    unsigned long opaque_2b4[6];
    int (__cdecl *slot_2cc)(int);
    int (__cdecl *slot_2d0)(int);
    unsigned long opaque_2d4[5];
    int (__cdecl *slot_2e8)(int, int);
    unsigned long opaque_2ec[1];
    int (__cdecl *slot_2f0)(int);
    unsigned long opaque_2f4[33];
    int (__cdecl *slot_378)(int, int, int);
    int (__cdecl *slot_37c)(int, int, int, int);
    int (__cdecl *slot_380)(int);
    unsigned long opaque_384[6];
    int (__cdecl *slot_39c)(int);
    int (__cdecl *slot_3a0)(int, int);
    int (__cdecl *slot_3a4)(int, int);
    int (__cdecl *slot_3a8)(int);
    unsigned long opaque_3ac[15];
} EngineInterface;

typedef struct Actor Actor;
struct Actor {
    long x, y, z;                       /* 0x000 */
    unsigned char unknown_00c[0x1f - 0x0c];
    unsigned char unknown_01f;          /* 0x01f */
    unsigned char unknown_020;
    unsigned char unknown_021;          /* 0x021 */
    unsigned char category;             /* 0x022 */
    unsigned char unknown_023[2];
    unsigned char state_025;            /* 0x025 */
    unsigned char unknown_026[0x2a - 0x26];
    unsigned short unknown_02a;         /* 0x02a */
    unsigned char unknown_02c[2];
    unsigned short kind;                /* 0x02e */
    short frame_ticks;                  /* 0x030 */
    short frame_state;                  /* 0x032 */
    short active_command;               /* 0x034 */
    short unknown_036;                  /* 0x036 */
    unsigned char unknown_038[0x4c - 0x38];
    long interpolation_position;        /* 0x04c */
    unsigned long flags;                /* 0x050 */
    unsigned long render_flags;         /* 0x054 */
    unsigned char unknown_058[0x94 - 0x58];
    unsigned char direction_x;          /* 0x094 */
    unsigned char unknown_095[0x118 - 0x95];
    void *initial_steps;                /* 0x118 */
    short unknown_11c;                  /* 0x11c */
    short active_step;                  /* 0x11e */
    unsigned char unknown_120[4];
    short action_callback_index;        /* 0x124 */
    short unknown_126;                  /* 0x126 */
};

/* Six-byte actor control records at actor+0x118. */
typedef struct ControlRecord {
    short opcode;
    short x;
    short y;
} ControlRecord;

/* Loader-zeroed actor control globals at 0x70ed8..0x70f10, named by RVA. They are
 * separate scalars: 0x2cd30 caches and reloads them the way VC5 treats independent
 * scalar globals, which a single aggregate view does not reproduce. The three
 * 50-tick timers are declared first (f0e, ef0, f04); 0x2cd30's OR of the three
 * timers is operand-order sensitive to this declaration order (measured, not a
 * claim about the original header). */
extern unsigned short g_eng1_control_70f0e, g_eng1_control_70ef0, g_eng1_control_70f04;
extern unsigned short g_eng1_control_70ed8;
extern unsigned short g_eng1_control_70eda;
extern unsigned short g_eng1_control_70edc;
extern unsigned short g_eng1_control_70ede;
extern unsigned short g_eng1_control_70ee0;
extern unsigned short g_eng1_control_70ee2;
extern unsigned short g_eng1_control_70ee4;
extern unsigned short g_eng1_control_70ee6;
extern unsigned short g_eng1_control_70ee8;
extern unsigned short g_eng1_control_70eea;
extern Actor *g_eng1_control_actor_70eec;
extern unsigned short g_eng1_control_70ef2;
extern unsigned short g_eng1_control_70ef4;
extern Actor *g_eng1_control_actor_70ef8;
extern unsigned short g_eng1_control_70efc;
extern unsigned short g_eng1_control_70efe;
extern unsigned short g_eng1_control_70f00;
extern unsigned short g_eng1_control_70f02;
extern unsigned short g_eng1_control_70f06;
extern unsigned short g_eng1_control_70f08;
extern unsigned short g_eng1_control_70f0a;
extern unsigned short g_eng1_control_70f0c;

extern EngineInterface g_engine_interface;
extern long eng3_1f7b0(unsigned short a, unsigned short b);

int eng1_2c030(int a, int b)
{
    if (a > b)
        return a + (b >> 2) + (b >> 3);
    return b + (a >> 3) + (a >> 2);
}

int eng1_2c090(long x0, long y0, long x1, long y1)
{
    int quadrant = 0;
    int dx, dy;
    long angle;

    if (x0 > x1) {
        dx = (x0 - x1) >> 16;
        quadrant = 1;
    } else {
        dx = (x1 - x0) >> 16;
    }
    if (y0 > y1) {
        dy = (y0 - y1) >> 16;
    } else {
        dy = (y1 - y0) >> 16;
        quadrant += 2;
    }
    angle = eng3_1f7b0(dx, dy);
    switch (quadrant) {
    case 0:
        return 0xff - (angle / 90 >> 8);
    case 1:
        return (angle / 90 >> 8) + 0x300;
    case 2:
        return (angle / 90 >> 8) + 0x100;
    case 3:
        return 0x2ff - (angle / 90 >> 8);
    }
    return 0;
}

void eng1_2c2c0(void)
{
    g_engine_interface.slot_03c();
}

int eng1_2c310(int a, int b)
{
    return g_engine_interface.slot_04c(a, b);
}

int eng1_2c3e0(int a, int b, int c)
{
    return g_engine_interface.slot_07c(a, b, c);
}

void dispatch_engine_2c450(void)
{
    g_engine_interface.slot_090();
}

void dispatch_engine_2c4d0(void)
{
    g_engine_interface.slot_0b4();
}

void eng1_2c560(void)
{
    g_engine_interface.slot_0dc();
}

void dispatch_engine_2c590(void)
{
    g_engine_interface.slot_0e8();
}

int dispatch_engine_2c5c0(int a, int b)
{
    return g_engine_interface.slot_108(a, b);
}

int dispatch_engine_2c6a0(int a, int b, int c)
{
    return g_engine_interface.slot_198(a, b, c);
}

void dispatch_engine_2c6f0(void)
{
    g_engine_interface.slot_1ac();
}

int dispatch_engine_2c720(int a)
{
    return g_engine_interface.slot_1b8(a);
}

int dispatch_engine_2c730(int a)
{
    return g_engine_interface.slot_1bc(a);
}

int dispatch_engine_2c780(int a)
{
    return g_engine_interface.slot_1d8(a);
}

int dispatch_engine_2c7a0(int a)
{
    return g_engine_interface.slot_1e0(a);
}

int dispatch_engine_2c7c0(int a, int b)
{
    return g_engine_interface.slot_1e8(a, b);
}

int eng1_2c810(int a, int b, int c)
{
    return g_engine_interface.slot_20c(a, b, c);
}

int eng1_2c890(int a)
{
    return g_engine_interface.slot_260(a);
}

int eng1_2c8a0(int a, int b, int c)
{
    return g_engine_interface.slot_270(a, b, c);
}

int eng1_2c8c0(int a, int b, int c)
{
    return g_engine_interface.slot_274(a, b, c);
}

int eng1_2c900(int a, int b)
{
    return g_engine_interface.slot_28c(a, b);
}

int dispatch_engine_2c960(int a, int b)
{
    return g_engine_interface.slot_2a4(a, b);
}

int dispatch_local_2c9b0(int a, int b, int c, int d)
{
    return g_engine_interface.slot_2b0(a, b, c, d);
}

int dispatch_engine_2ca00(int a)
{
    return g_engine_interface.slot_2cc(a);
}

int eng1_2ca10(int a)
{
    return g_engine_interface.slot_2d0(a);
}

int eng1_2ca20(int a, int b)
{
    return g_engine_interface.slot_2e8(a, b);
}

int eng1_2ca50(int a)
{
    return g_engine_interface.slot_2f0(a);
}

int eng1_2cb20(int a, int b, int c)
{
    return g_engine_interface.slot_378(a, b, c);
}

int eng1_2cb40(int a, int b, int c, int d)
{
    return g_engine_interface.slot_37c(a, b, c, d);
}

int eng1_2cb60(int a)
{
    return g_engine_interface.slot_380(a);
}

int eng1_2cbd0(int a)
{
    return g_engine_interface.slot_39c(a);
}

int eng1_2cbe0(int a, int b)
{
    return g_engine_interface.slot_3a0(a, b);
}

int eng1_2cc00(int a, int b)
{
    return g_engine_interface.slot_3a4(a, b);
}

int eng1_2cc20(int a)
{
    return g_engine_interface.slot_3a8(a);
}

typedef union ActorPipelineStorage {
    unsigned short word_cells[0x460];
    unsigned long dword_cells[0x230];
} ActorPipelineStorage;
extern ActorPipelineStorage g_actor_pipeline_storage_71440;
extern unsigned short g_eng1_control_lookup_words_5c3b8[40];
extern unsigned long eng1_sim_state_72110;
extern unsigned long g_eng1_setup_dword_6efa4;
extern long *g_collision_context;
/* Resource group rows at 0x70e4c; the bounds stores in 0x2da90 reload the
 * count at +0x82 each iteration, so the count is a member of the same object. */
typedef struct ResourceGroupState {
    short cumulative_by_row[16];        /* 0x70e4c */
    short bounds_by_row[16][3];         /* 0x70e6c */
    unsigned char clear_byte_080;       /* 0x70ecc */
    unsigned char unknown_081;
    signed char group_count_082;        /* 0x70ece */
} ResourceGroupState;
extern ResourceGroupState g_resource_group_state_70e4c;
extern unsigned short g_eng1_classifier_rows_65678[18][6];

extern int eng1_native_code_target_002c4c0(unsigned long value);
extern unsigned char eng1_native_code_target_0041fd0(unsigned long value);
extern void eng1_native_code_target_002a290(Actor *actor);
extern int eng1_interpret_signed_pairs(Actor *actor);
extern void apply_frame_motion_2d(Actor *actor);
extern void select_frame(Actor *actor, int index);
extern unsigned int dispatch_engine_2c460(int range);
extern void LinkedSoundFx_6da0(unsigned short id, Actor *owner);
int eng1_native_dep_002d3f0(Actor *actor, int selector);
void eng1_native_dep_002d440(Actor *actor);

void eng1_native_code_target_02cd30(Actor *actor)
{
    if (g_eng1_control_70f0e != 0 && --g_eng1_control_70ee4 == 0) {
        g_eng1_control_70f0e--;
        g_eng1_control_70ee4 = 50;
    }
    if (g_eng1_control_70f04 != 0 && --g_eng1_control_70ed8 == 0) {
        g_eng1_control_70f04--;
        g_eng1_control_70ed8 = 50;
    }
    if (g_eng1_control_70ef0 != 0 && --g_eng1_control_70eda == 0) {
        g_eng1_control_70ef0--;
        g_eng1_control_70eda = 12;
    }

    if (actor->unknown_02a != 11 && actor->unknown_02a != 0)
        g_actor_pipeline_storage_71440.word_cells[6] = 0;
    else if ((g_eng1_control_70f04 | g_eng1_control_70ef0 | g_eng1_control_70f0e) == 0)
        g_actor_pipeline_storage_71440.word_cells[6]++;

    if (g_eng1_control_70edc != 0)
        g_eng1_control_70edc--;
    if (g_eng1_control_70ee2 != 0)
        g_eng1_control_70ee2--;
    if (g_eng1_control_70ee0 != 0)
        g_eng1_control_70ee0--;

    if (g_eng1_control_70f08 != 0) {
        g_eng1_control_70f08--;
    } else if (eng1_sim_state_72110 == 0) {
        g_eng1_control_70f02 &= 3;
        g_eng1_control_70ee8 &= 3;
        switch (g_eng1_control_70f0c) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            if (g_eng1_control_70f0e > g_eng1_control_lookup_words_5c3b8[g_eng1_control_70f0c]) {
                eng1_native_code_target_002c4c0(g_eng1_control_lookup_words_5c3b8[12 + g_eng1_control_70f0c]);
                g_eng1_control_70f0e = 0;
                g_eng1_control_70f08 = 250;
            } else if (g_eng1_control_70f04 > 1) {
                if (g_eng1_control_70edc == 0) {
                    eng1_native_code_target_002c4c0(0x1a);
                    g_eng1_control_70edc = 800;
                    g_eng1_control_70f08 = 100;
                    g_eng1_control_70f04 = 0;
                }
            } else if (g_eng1_control_70ef0 > 20) {
                if (g_eng1_control_70ee2 == 0) {
                    eng1_native_code_target_002c4c0(g_eng1_control_lookup_words_5c3b8[28 + g_eng1_control_70f02]);
                    g_eng1_control_70f02++;
                    g_eng1_control_70ee2 = 240;
                    g_eng1_control_70f08 = 160;
                    g_eng1_control_70ef0 = 0;
                }
            } else if (g_actor_pipeline_storage_71440.word_cells[6] > 250) {
                if (g_eng1_control_70ee0 == 0) {
                    if (g_eng1_setup_dword_6efa4 != 0)
                        eng1_native_code_target_002c4c0(g_eng1_control_lookup_words_5c3b8[36 + g_eng1_control_70ee8]);
                    else
                        eng1_native_code_target_002c4c0(g_eng1_control_lookup_words_5c3b8[32 + g_eng1_control_70ee8]);
                    g_eng1_control_70ee8++;
                    g_eng1_control_70ee0 = 350;
                    g_eng1_control_70f08 = 160;
                    g_actor_pipeline_storage_71440.word_cells[6] = 0;
                }
            }
            break;
        case 8:
            if (g_eng1_control_70ef0 > 10) {
                if (g_eng1_control_70ee2 == 0) {
                    eng1_native_code_target_002c4c0(g_eng1_control_lookup_words_5c3b8[28 + g_eng1_control_70f02]);
                    g_eng1_control_70f02++;
                    g_eng1_control_70ee2 = 240;
                    g_eng1_control_70f08 = 160;
                    g_eng1_control_70ef0 = 0;
                }
            } else if (g_actor_pipeline_storage_71440.word_cells[6] > 250) {
                if (g_eng1_control_70ee0 == 0) {
                    if (g_eng1_setup_dword_6efa4 != 0)
                        eng1_native_code_target_002c4c0(g_eng1_control_lookup_words_5c3b8[36 + g_eng1_control_70ee8]);
                    else
                        eng1_native_code_target_002c4c0(g_eng1_control_lookup_words_5c3b8[32 + g_eng1_control_70ee8]);
                    g_eng1_control_70ee8++;
                    g_eng1_control_70ee0 = 350;
                    g_eng1_control_70f08 = 160;
                    g_actor_pipeline_storage_71440.word_cells[6] = 0;
                }
            }
            break;
        case 12:
            if (g_eng1_control_70f0e > 6) {
                eng1_native_code_target_002c4c0(g_eng1_control_lookup_words_5c3b8[12 + g_eng1_control_70f0c]);
                g_eng1_control_70f08 = 250;
                g_eng1_control_70f0e = 0;
            } else if (g_eng1_control_70ef0 > 10) {
                if (g_eng1_control_70ee2 == 0) {
                    eng1_native_code_target_002c4c0(g_eng1_control_lookup_words_5c3b8[28 + g_eng1_control_70f02]);
                    g_eng1_control_70f02++;
                    g_eng1_control_70ee2 = 240;
                    g_eng1_control_70f08 = 160;
                    g_eng1_control_70ef0 = 0;
                }
            } else if (g_actor_pipeline_storage_71440.word_cells[6] > 250) {
                if (g_eng1_control_70ee0 == 0) {
                    eng1_native_code_target_002c4c0(g_eng1_control_lookup_words_5c3b8[36 + g_eng1_control_70ee8]);
                    g_eng1_control_70ee8++;
                    g_eng1_control_70ee0 = 350;
                    g_eng1_control_70f08 = 160;
                    g_actor_pipeline_storage_71440.word_cells[6] = 0;
                }
            }
            break;
        }
    }

    if (actor->state_025 & 1)
        eng1_native_code_target_002a290(actor);
    g_eng1_control_70f0c = eng1_native_code_target_0041fd0(*g_collision_context);
    g_eng1_control_70ef2 = eng1_native_code_target_0041fd0(actor->x);
    g_eng1_control_70efc = 0;
    if (actor->x < 0x4f80000)
        actor->x = 0x4f80000;
    if (g_eng1_control_70f0c != g_eng1_control_70eea &&
        g_eng1_control_70f0c != g_eng1_control_70ef2)
        eng1_native_dep_002d3f0(actor, g_eng1_control_70f0c);
    eng1_native_dep_002d440(actor);
    if (actor->unknown_02a == 10) {
        actor->render_flags &= 0x7fffffff;
    } else {
        actor->render_flags |= 0x80000000;
        eng1_interpret_signed_pairs(actor);
        apply_frame_motion_2d(actor);
        actor->render_flags = (actor->direction_x & 0x10) | (actor->render_flags & ~0x10);
        g_eng1_control_70ee6 = (unsigned short)actor->interpolation_position;
        if (actor->unknown_02a == 7) {
            g_eng1_control_70efc = 1;
            g_eng1_control_70ef4 = actor->unknown_036;
        } else if (g_eng1_control_70efe == 2) {
            g_eng1_control_70efc = 2;
            g_eng1_control_70ee6 = 0x10;
            g_eng1_control_70ef4 = 0x66;
        }
        if (actor->unknown_02a == 2 || actor->unknown_02a == 5)
            g_eng1_control_70f00 = actor->unknown_036;
        else
            g_eng1_control_70f00 = 0;
    }
    g_eng1_control_70efe = 0;
    g_eng1_control_70f06 = 0;
    g_eng1_control_70ede = 0;
}

void eng1_native_code_target_02d1f0(Actor *actor)
{
    if (actor->unknown_02a == 7) {
        if (g_eng1_control_70efe == 2)
            select_frame(actor, 0x13);
        if (g_eng1_control_70efe == 0)
            select_frame(actor, 0x13);
        return;
    }
    if (g_eng1_control_70efe != 0) {
        if (g_eng1_control_70efe == 1) {
            actor->direction_x = 0;
            select_frame(actor, 0x11);
        }
    } else if (actor->unknown_02a == 0) {
        if (actor->direction_x == 0xff) {
            if (actor->x < *g_collision_context) {
                select_frame(actor, 3);
                return;
            }
        } else if (actor->x > *g_collision_context) {
            select_frame(actor, 3);
            return;
        }
        switch (dispatch_engine_2c460(6)) {
        case 0: select_frame(actor, 0x16); return;
        case 1: select_frame(actor, 0x19); return;
        case 2: select_frame(actor, 0x1c); return;
        case 3: select_frame(actor, 0x25); return;
        case 4: select_frame(actor, 0x28); return;
        case 5: select_frame(actor, 0x2b); return;
        }
    }
}

void eng1_native_code_target_02d320(Actor *actor)
{
    if (actor->unknown_02a != 0)
        return;
    if (actor->direction_x == 0xff) {
        if (actor->x < *g_collision_context) {
            select_frame(actor, 3);
            return;
        }
    } else if (actor->x > *g_collision_context) {
        select_frame(actor, 3);
        return;
    }
    switch (dispatch_engine_2c460(4)) {
    case 0: select_frame(actor, 0x16); return;
    case 1: select_frame(actor, 0x19); return;
    case 2: select_frame(actor, 0x1c); return;
    case 3: select_frame(actor, 0x25); return;
    case 4: select_frame(actor, 0x28); return;
    case 5: select_frame(actor, 0x2b); return;
    }
}

int eng1_native_dep_002d3f0(Actor *actor, int selector)
{
    ControlRecord *record = (ControlRecord *)actor->initial_steps;
    short index = 0;
    int opcode;

    for (;;) {
        opcode = record->opcode;
        if (opcode == 20 && record->y == selector) {
            g_eng1_control_70eea = (unsigned short)selector;
            actor->unknown_11c = index;
            return 1;
        }
        index++;
        record++;
        if (opcode == 0)
            return 0;
    }
}


void eng1_native_dep_002d440(Actor *actor)
{
    ControlRecord *record;
    long x, y;
    int sx, sy;

    if (actor->unknown_11c < 0)
        actor->unknown_11c = 0;
    record = (ControlRecord *)actor->initial_steps + actor->unknown_11c;
    sx = record->x;
    sy = record->y;
    x = sx << 16;
    y = sy << 16;
    switch (record->opcode) {
    case 0:
        return;
    case 1:
        actor->x = x;
        actor->y = y;
        actor->unknown_11c++;
        return;
    case 2:
        if (sy == 1 && g_eng1_control_70f06 != 0)
            x = (long)g_eng1_control_70f06 << 16;
        if (sy == 2 && g_eng1_control_70ede != 0)
            x = (long)g_eng1_control_70ede << 16;
        if (actor->unknown_02a == 1) {
            if (abs(actor->x - x) < 0x420000) {
                select_frame(actor, 8);
                return;
            }
            if ((actor->unknown_01f & 3) == 1) {
                long lo = ((long)g_resource_group_state_70e4c.bounds_by_row[actor->z >> 24][0] << 22) - 0x400000;
                long hi = (long)(g_resource_group_state_70e4c.bounds_by_row[actor->z >> 24][1] + 1) << 22;
                if (x > lo && actor->x < lo)
                    actor->x = lo;
                if (x < hi && actor->x > hi)
                    actor->x = hi;
            }
        } else if (actor->unknown_02a == 0x15) {
            if (abs(actor->x - x) > 0x420000)
                select_frame(actor, 7);
            return;
        } else if (actor->unknown_02a == 0x16) {
            if (abs(actor->x - x) < 0x100000)
                actor->x = x;
            actor->unknown_11c++;
            return;
        } else {
            select_frame(actor, 6);
        }
        if (actor->x < x)
            actor->direction_x = 0;
        else
            actor->direction_x = 0xff;
        return;
    case 4:
        if (g_eng1_control_actor_70eec != 0 &&
            g_eng1_control_actor_70eec->action_callback_index == g_eng1_control_actor_70eec->unknown_126)
            actor->unknown_11c++;
        return;
    case 5:
        if (actor->unknown_02a == 0 || actor->unknown_02a == 11) {
            if (g_eng1_classifier_rows_65678[g_eng1_control_70ef2][2] < 8) {
                if (g_eng1_control_70f0a != 0) {
                    select_frame(actor, 0x22);
                    g_eng1_control_70f0a = 0;
                    actor->direction_x = 0;
                }
                return;
            }
            if (g_eng1_classifier_rows_65678[g_eng1_control_70ef2][5] == 0) {
                select_frame(actor, 0x22);
                actor->direction_x = 0;
                g_eng1_control_70f0a = 0;
                return;
            }
            actor->unknown_02a = 4;
            actor->direction_x = 0;
            g_eng1_control_70f0a = 0;
        } else if (actor->unknown_02a == 5) {
            if (g_eng1_control_actor_70ef8 != 0 && g_eng1_control_actor_70ef8->unknown_02a == 0) {
                g_eng1_control_actor_70ef8->unknown_02a = 3;
                LinkedSoundFx_6da0(0x4049, g_eng1_control_actor_70ef8);
            }
        } else if (actor->unknown_02a == 9) {
            if (g_eng1_control_actor_70ef8->unknown_02a == 9)
                actor->unknown_11c++;
        }
        return;
    case 6:
        if (actor->unknown_02a == 0 || actor->unknown_02a == 11) {
            if (g_eng1_classifier_rows_65678[g_eng1_control_70ef2][2] < 8) {
                if (g_eng1_control_70f0a != 0) {
                    select_frame(actor, 0x31);
                    g_eng1_control_70f0a = 0;
                    actor->direction_x = 0;
                }
                return;
            }
            if (g_eng1_classifier_rows_65678[g_eng1_control_70ef2][5] == 0) {
                select_frame(actor, 0x31);
                actor->direction_x = 0;
                g_eng1_control_70f0a = 0;
                return;
            }
            actor->unknown_02a = 4;
            actor->direction_x = 0;
            g_eng1_control_70f0a = 0;
        } else if (actor->unknown_02a == 5) {
            if (g_eng1_control_actor_70ef8 != 0 && g_eng1_control_actor_70ef8->unknown_02a == 0) {
                g_eng1_control_actor_70ef8->unknown_02a = 1;
                LinkedSoundFx_6da0(0x404a, g_eng1_control_actor_70ef8);
            }
        } else if (actor->unknown_02a == 9) {
            if (*g_collision_context > x && g_eng1_control_actor_70ef8 != 0)
                actor->unknown_11c++;
        }
        return;
    case 3:
        actor->action_callback_index = sx;
        g_eng1_control_70eea = sy;
        actor->unknown_11c++;
        return;
    case 8:
        actor->z = x;
        actor->unknown_11c++;
        return;
    case 7:
        select_frame(actor, 0x2e);
        actor->unknown_11c++;
        return;
    case 9:
        if (*g_collision_context > x) {
            actor->unknown_02a = 0;
            actor->unknown_11c++;
        } else {
            actor->unknown_02a = 10;
        }
        return;
    case 11: {
        long root, diff;

        if (actor->x < x)
            actor->x = x;
        if (actor->x > y)
            actor->x = y;
        root = *g_collision_context;
        diff = actor->x - root;
        if ((actor->unknown_01f & 3) == 1) {
            long lo = (long)(g_resource_group_state_70e4c.bounds_by_row[actor->z >> 24][0] - 2) << 22;
            long hi = (long)(g_resource_group_state_70e4c.bounds_by_row[actor->z >> 24][1] + 2) << 22;
            if (root > lo && actor->x < lo)
                actor->x = lo;
            if (root < hi && actor->x > hi)
                actor->x = hi;
            if (actor->x < root)
                actor->direction_x = 0;
            else
                actor->direction_x = 0xff;
        }
        if (actor->unknown_02a == 1) {
            if (actor->direction_x != 0) {
                if (diff < -0x180000)
                    select_frame(actor, 8);
            } else if (diff > 0x180000) {
                select_frame(actor, 8);
            }
        } else if (actor->unknown_02a == 0) {
            if (actor->direction_x != 0) {
                if (diff <= 0)
                    return;
            } else if (diff >= 0) {
                return;
            }
            if (abs(diff) > 0x600000)
                select_frame(actor, 7);
        }
        return;
    }
    case 12:
        if (g_eng1_classifier_rows_65678[sy][2] >= 4)
            actor->unknown_11c += sx;
        actor->unknown_11c++;
        return;
    case 13:
        select_frame(actor, 0);
        actor->unknown_11c++;
        return;
    case 14:
        return;
    default:
        actor->unknown_11c++;
        return;
    }
}


void eng1_native_code_target_02d9e0(Actor *actor)
{
    if (g_eng1_control_70eea == actor->unknown_021)
        g_eng1_control_70f06 = (unsigned short)(actor->x >> 16);
    actor->active_command = g_eng1_control_70ef4;
}

void eng1_native_code_target_02da10(Actor *actor)
{
    actor->y = 0x3200000;
    actor->z = 0x1000000;
    actor->unknown_021 = eng1_native_code_target_0041fd0(actor->x);
    actor->active_command = actor->unknown_036 = 0x83;
}

void eng1_native_code_target_02da40(Actor *actor)
{
    if (g_eng1_control_70eea == actor->unknown_021)
        g_eng1_control_70ede = (unsigned short)(actor->x >> 16);
    if (g_eng1_control_70f00 != 0) {
        actor->unknown_036 = g_eng1_control_70f00;
        actor->active_command = g_eng1_control_70f00;
    } else {
        actor->active_command = actor->unknown_036;
    }
}

/* Mesh visit record shared by the 0x2da90 traversal and the 0x2df20 emitter
 * (layout from src/eng1/behavioral/mesh_visit.h). */
typedef struct Matrix {
    short m[3][3];
    long t[3];
} Matrix;

typedef struct MeshVisit {
    unsigned char *header_000, *header_004, *child_008;
    short projected_00c[2];
    short depth_010;
    unsigned char unknown_012[3];
    unsigned char column_counter_015;
    unsigned char column_stop_016;
    unsigned char row_stop_017;
    long row_018;
    long column_01c;
    unsigned long packed_020;
    unsigned char winding_024, variant_025, shift_026, limit_027;
    unsigned char mode_028, side_029;
    unsigned short cell_02a, flags_02c;
    unsigned char unknown_02e[6];
    long tick_034, layer_y_038, layer_z_03c, cached_040, cached_044;
    long row_local_048, column_local_04c, anchor_row_050;
    Matrix transform_054;
} MeshVisit;

typedef struct FrameConfig {
    unsigned char unknown_00[8];
    unsigned char limit_08;
    unsigned char shift_09;
    unsigned char unknown_0a[0x1c - 0x0a];
} FrameConfig;

extern Matrix g_render_identity_5ce28;
extern Matrix g_render_transform_70c60;
extern long g_camera_x_70ca0, g_camera_z_70ca8, g_camera_z_base_70d1c;
extern short g_element_width_70d14;
extern unsigned char g_mode_table_value_70d00;
extern unsigned char *g_header_pointer_70d54, *g_header_pointer_70d58;
extern unsigned long *g_rebuilt_row_pointers_70dcc[];
extern short g_rebuilt_row_offsets_70e0c[];
extern void (*g_frame_global_70d24)(void);
extern FrameConfig g_frame_config_5df08[];

int dispatch_local_2df20(MeshVisit *visit, int layer);

void dispatch_local_2da90(void)
{
    MeshVisit visit;
    long remaining, layer, step, distance, starts[2], ends[2];
    long width, layer_start, layer_count, n, end;
    long side, row, row_seen, column;
    unsigned long *base, *p;
    int i;

    ((Matrix *)g_engine_interface.opaque_010[0])[0] = g_render_identity_5ce28;
    ((Matrix *)g_engine_interface.opaque_010[0])[1] = g_render_identity_5ce28;
    visit.transform_054 = g_render_transform_70c60;
    for (i = 0; i < g_resource_group_state_70e4c.group_count_082; i++)
        g_resource_group_state_70e4c.bounds_by_row[i][0] = -127;
    visit.header_004 = g_header_pointer_70d58;
    visit.header_000 = g_header_pointer_70d54;
    visit.cached_040 = visit.cached_044 = -1;
    visit.tick_034 = g_engine_interface.context_004->tick_038 << 8 >> 8;
    visit.limit_027 = g_frame_config_5df08[g_engine_interface.context_004->level].limit_08;
    visit.mode_028 = g_mode_table_value_70d00;
    visit.shift_026 = g_frame_config_5df08[g_engine_interface.context_004->level].shift_09;
    width = g_element_width_70d14;
    if (visit.mode_028 == 0) {
        remaining = 6;
        layer = (g_camera_z_70ca8 + 0x180) / 128 + 5;
        if (layer < 6)
            layer = 6;
        starts[0] = starts[1] = 0;
        ends[0] = ends[1] = 24;
        distance = 10;
        step = 0;
    } else {
        starts[0] = starts[1] = 0;
        remaining = 2;
        layer = 2;
        ends[0] = ends[1] = 32;
        distance = 16;
        step = 2;
    }

    while (remaining-- != 0) {
        if (--layer >= g_resource_group_state_70e4c.group_count_082)
            continue;
        if (layer < 0)
            break;
        if (visit.mode_028 == 2) {
            if (layer == 0)
                starts[0] = starts[1] = 0;
        } else if (visit.mode_028 == 1) {
            step = -2;
            starts[0] = (8 - layer) * 2;
            starts[1] = 17 - layer * 2;
        }
        visit.cached_040 = visit.cached_044 = -1;
        visit.layer_y_038 = g_resource_group_state_70e4c.cumulative_by_row[layer] - g_camera_z_base_70d1c;
        visit.layer_z_03c = g_camera_z_base_70d1c - g_camera_z_70ca8;
        layer_count = g_rebuilt_row_offsets_70e0c[layer];
        layer_start = g_rebuilt_row_offsets_70e0c[layer + 16];
        base = g_rebuilt_row_pointers_70dcc[layer] - layer_start * width;
        for (side = 0; side < 2; side++) {
            n = starts[side] + side;
            end = ends[side] + side;
            visit.row_stop_017 = 0;
            visit.side_029 = (unsigned char)side;
            for (; n < end; n++) {
                row_seen = (signed char)visit.row_stop_017;
                row = ((side & 1) ? -n : n) + visit.transform_054.t[1];
                visit.row_018 = row;
                if (visit.mode_028 == 0) {
                    if ((unsigned long)(row - layer_start) >= (unsigned long)layer_count)
                        break;
                } else {
                    row = (row & 0x3f) + 2;
                    visit.row_018 = row;
                }
                p = base + row * width;
                visit.column_stop_016 = 0;
                for (visit.column_counter_015 = 0; visit.column_counter_015 < distance; visit.column_counter_015++) {
                    column = visit.transform_054.t[2] - visit.column_counter_015;
                    visit.column_01c = column;
                    if ((unsigned long)(column + step) >= (unsigned long)width)
                        break;
                    visit.packed_020 = p[column];
                    if (visit.packed_020 & 0x7fe) {
                        if (dispatch_local_2df20(&visit, layer))
                            goto finished;
                        if (visit.column_stop_016 && visit.column_counter_015 > 2)
                            break;
                    } else if (visit.packed_020 & 0x80000000) {
                        break;
                    }
                }
                visit.column_stop_016 = 0;
                for (visit.column_counter_015 = 1; visit.column_counter_015 < distance; visit.column_counter_015++) {
                    column = visit.transform_054.t[2] + visit.column_counter_015;
                    visit.column_01c = column;
                    if ((unsigned long)(column + step) >= (unsigned long)width)
                        break;
                    visit.packed_020 = p[column];
                    if (visit.packed_020 & 0x7fe) {
                        if (dispatch_local_2df20(&visit, layer))
                            goto finished;
                        if (visit.column_stop_016 && visit.column_counter_015 > 1)
                            break;
                    } else if (visit.packed_020 & 1) {
                        break;
                    }
                }
                if (row_seen != 0 && visit.mode_028 == 0)
                    break;
            }
            ends[side] = n + 2;
            if (side)
                g_resource_group_state_70e4c.bounds_by_row[layer][0] = (short)(g_camera_x_70ca0 / 64 - n);
            else
                g_resource_group_state_70e4c.bounds_by_row[layer][1] = (short)(g_camera_x_70ca0 / 64 + n);
        }
    }
finished:
    if (g_frame_global_70d24)
        g_frame_global_70d24();
}

typedef struct Eng1Buffer {
    unsigned char unknown_000[0x88];
    unsigned long ot[0x500];            /* 0x0088 ordering table */
} Eng1Buffer;

typedef struct MeshVertex {
    unsigned long xy;
    unsigned long rgb;
} MeshVertex;

extern Eng1Buffer *g_dispatch_frame_current_71ec4;
extern unsigned char *g_frame_global_73484;
extern unsigned char *g_header_pointer_70d30, *g_header_pointer_70d34;
extern unsigned long g_mesh_colors_6f4e8[];

extern int dispatch_local_2e3b0(MeshVisit *visit);
extern int dispatch_local_2e7a0(MeshVisit *visit);
extern void dispatch_local_2ef60(MeshVisit *visit);
extern void dispatch_local_2e600(unsigned char *stream, MeshVisit *visit, int count);
extern void dispatch_local_2e6c0(unsigned char *stream, MeshVisit *visit, int count);
extern void dispatch_local_2ea50(unsigned char *stream, MeshVisit *visit, int count);
extern void dispatch_local_2ed10(unsigned char *stream, MeshVisit *visit, int count, int prep);
extern void dispatch_local_2f120(unsigned char *stream, MeshVisit *visit, int count);
extern void dispatch_local_2f340(unsigned char *stream, MeshVisit *visit, int count);
extern void dispatch_engine_2c670(unsigned long xy0, unsigned long xy1, unsigned long xy2);
extern void dispatch_engine_2c6d0(void);
extern void dispatch_engine_2c740(long *normal);
extern void dispatch_engine_2c770(void *packet);
extern void dispatch_engine_2c790(void *packet);

int dispatch_local_2df20(MeshVisit *visit, int layer)
{
    unsigned char *stream, *packet, *uv, *lookup, *child;
    unsigned long *ot, *head, *faces, *uvp;
    MeshVertex *vertices, *v0, *v1, *v2, *v3;
    unsigned long index, uv_word, shade, color, xy0, xy1, xy2, xy3, tex0, tex1;
    long segment;
    long face, depth, normal, selector;
    int count, face_count, mode, prep;
    unsigned long cell;

    visit->winding_024 = (unsigned char)visit->packed_020 & 1;
    mode = 0;
    index = (visit->packed_020 >> 1) & 0x3ff;
    segment = *(short *)(g_header_pointer_70d34 + (index - 1) * 4) + index;
    stream = g_header_pointer_70d34 + (segment - 1) * 4;
    selector = *(short *)(g_header_pointer_70d34 + (segment - 1) * 4);
    count = stream[2];
    stream += 2;
    if (count == 0)
        return 0;
    face_count = stream[1];
    stream += 2;
    if (g_engine_interface.context_004->unknown_040 != 0)
        return 0;
    lookup = g_header_pointer_70d54 + ((visit->packed_020 >> 11) & 0x1fff) * 4;
    cell = *(unsigned short *)lookup;
    visit->cell_02a = cell;
    child = visit->header_004 + (cell << 4);
    visit->child_008 = child;
    visit->variant_025 = (*(unsigned short *)(child + 0xe) >> 14) & 3;
    visit->flags_02c = *(unsigned short *)(lookup + 2);
    if (visit->variant_025) {
        if (selector == 1) {
            dispatch_local_2ef60(visit);
            dispatch_local_2f120(stream, visit, count);
            visit->column_counter_015++;
        } else {
            if (dispatch_local_2e7a0(visit) == -1)
                return 0;
            if (visit->flags_02c & 8) {
                dispatch_local_2ea50(stream, visit, count);
            } else {
                dispatch_local_2f340(stream, visit, count);
                mode = 1;
            }
        }
    } else {
        prep = dispatch_local_2e3b0(visit);
        if (prep == -1)
            return 0;
        if (visit->flags_02c & 8) {
            if (prep == 0)
                dispatch_local_2e600(stream, visit, count);
            else
                dispatch_local_2e6c0(stream, visit, count);
        } else {
            dispatch_local_2ed10(stream, visit, count, prep);
            mode = 1;
        }
    }

    ot = &g_dispatch_frame_current_71ec4->ot[0x4ff - visit->depth_010];
    faces = (unsigned long *)stream + count - 1;
    packet = g_frame_global_73484;
    uv = g_header_pointer_70d30;
    vertices = (MeshVertex *)((unsigned char *)g_engine_interface.opaque_010[0] + 0x40);
    depth = (visit->depth_010 - 0x40) >> visit->shift_026;
    if (depth < 0)
        depth = 0;
    else if (depth > visit->limit_027)
        depth = visit->limit_027;
    shade = (depth << 22) + 0x78300000;

    switch (mode) {
    case 1:
        do {
            faces += 2;
            face = faces[0];
            v0 = &vertices[face & 0x3f];
            face >>= 3;
            v1 = (MeshVertex *)((unsigned char *)vertices + (face & 0x1f8));
            face >>= 6;
            v2 = (MeshVertex *)((unsigned char *)vertices + (face & 0x1f8));
            face >>= 6;
            dispatch_engine_2c670(v0->xy, v1->xy, v2->xy);
            dispatch_engine_2c6d0();
            v3 = (MeshVertex *)((unsigned char *)vertices + (face & 0x1f8));
            face >>= 9;
            dispatch_engine_2c740(&normal);
            if (visit->winding_024 == 0) {
                if (normal < 0)
                    continue;
            } else if (normal > 0) {
                continue;
            }
            {
                dispatch_engine_2c790(packet);
                *(unsigned long *)(packet + 0x2c) = v3->xy;
                *(unsigned long *)(packet + 4) = v0->rgb | ((unsigned long)(((face & 0x20) | 0x3c0) >> 4) << 24);
                *(unsigned long *)(packet + 0x10) = v1->rgb;
                *(unsigned long *)(packet + 0x1c) = v2->rgb;
                *(unsigned long *)(packet + 0x28) = v3->rgb;
                uv_word = faces[1];
                head = (unsigned long *)((unsigned char *)ot - ((uv_word >> 22) & 0x3fc));
                *(unsigned long *)packet = *head;
                *head = (unsigned long)packet;
                if (face & 0x80) {
                    uv_word |= 0xff00;
                    *(unsigned short *)(packet + 0x24) = (unsigned short)uv_word;
                    *(unsigned short *)(packet + 0x30) = (unsigned short)uv_word;
                    uv_word &= 0xffff;
                    *(unsigned long *)(packet + 0xc) = shade | uv_word;
                    *(unsigned long *)(packet + 0x18) = 0x9c0000 | uv_word;
                } else {
                    uvp = (unsigned long *)(uv + (uv_word & 0xffff));
                    tex0 = uvp[0];
                    tex1 = uvp[1];
                    uv_word = uvp[2];
                    *(unsigned long *)(packet + 0xc) = tex0 | shade;
                    *(unsigned long *)(packet + 0x18) = tex1;
                    *(unsigned short *)(packet + 0x24) = (unsigned short)uv_word;
                    *(unsigned short *)(packet + 0x30) = (unsigned short)(uv_word >> 16);
                }
                packet += 0x34;
            }
        } while (--face_count);
        break;
    case 0:
        color = g_mesh_colors_6f4e8[((visit->flags_02c >> 6) & 0xff) + ((visit->flags_02c & 7) << 6)] | 0x2c000000;
        do {
            faces += 2;
            face = faces[0];
            xy0 = vertices[face & 0x3f].xy;
            face >>= 3;
            xy1 = *(unsigned long *)((unsigned char *)vertices + (face & 0x1f8));
            face >>= 6;
            xy2 = *(unsigned long *)((unsigned char *)vertices + (face & 0x1f8));
            face >>= 6;
            dispatch_engine_2c670(xy0, xy1, xy2);
            dispatch_engine_2c6d0();
            xy3 = *(unsigned long *)((unsigned char *)vertices + (face & 0x1f8));
            face >>= 9;
            dispatch_engine_2c740(&normal);
            if (visit->winding_024 == 0) {
                if (normal < 0)
                    continue;
            } else if (normal > 0) {
                continue;
            }
            {
                dispatch_engine_2c770(packet);
                *(unsigned long *)(packet + 0x20) = xy3;
                *(unsigned long *)(packet + 4) = ((face & 0x20) << 20) | color;
                uv_word = faces[1];
                head = (unsigned long *)((unsigned char *)ot - ((uv_word >> 22) & 0x3fc));
                *(unsigned long *)packet = *head;
                *head = (unsigned long)packet;
                if (face & 0x80) {
                    uv_word |= 0xff00;
                    *(unsigned short *)(packet + 0x1c) = (unsigned short)uv_word;
                    *(unsigned short *)(packet + 0x24) = (unsigned short)uv_word;
                    uv_word &= 0xffff;
                    *(unsigned long *)(packet + 0xc) = shade | uv_word;
                    *(unsigned long *)(packet + 0x14) = 0x9c0000 | uv_word;
                } else {
                    uvp = (unsigned long *)(uv + (uv_word & 0xffff));
                    tex0 = uvp[0];
                    tex1 = uvp[1];
                    uv_word = uvp[2];
                    *(unsigned long *)(packet + 0xc) = tex0 | shade;
                    *(unsigned long *)(packet + 0x14) = tex1;
                    *(unsigned short *)(packet + 0x1c) = (unsigned short)uv_word;
                    *(unsigned short *)(packet + 0x24) = (unsigned short)(uv_word >> 16);
                }
                packet += 0x28;
            }
        } while (--face_count);
        break;
    }
    g_frame_global_73484 = packet;
    return 0;
}
