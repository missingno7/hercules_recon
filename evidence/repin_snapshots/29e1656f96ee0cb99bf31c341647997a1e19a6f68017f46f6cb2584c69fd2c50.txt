extern "C" {
/* ENG1.DLL 0x16840..0x188a0: player mode handlers, mode-entry helpers and input probes (lane x22).
 * Original x22 baseline: one C++ unit (functions in extern "C"), C++ 40/43 masked equal,
 * C 37/43; 0x16fa0/0x16fd0 pressed/enabled order and 0x17eb0 z/prev_z order match only in C++.
 * Candidate-only per-access steering (lane nf6): 0x17290 casts each of its two held-word
 * expressions; 0x17ff0 casts its single expression. Pinned VC5 output retains the native
 * access count/order and matches both complete contributions after masking relocations.
 * The held_5c field remains volatile; other handlers keep their existing reload behavior.
 * Declaration order is the incremental authoring order; it was not searched, but the results
 * are phase-sensitive (a reordered extern block moved 0x16fa0, 0x16fd0, 0x174e0).  Types follow calibration/eng3/handlers_16240.cpp, whose ENG3
 * 0x180e0..0x19fd0 player handlers are diverged variants of this code, and
 * calibration/eng1/pickup_lift_unit_12620.c (Actor +0x58..+0x60 names).
 * held_5c is volatile under the owner ruling for LevelContext +0x5c/+0x5e: 0x16990, 0x16b40,
 * 0x17ba0 and 0x17d00 re-read it from memory for every test (non-volatile CSEs it into one
 * register load).  pressed_5e stays non-volatile (volatile breaks 0x16fa0/0x16fd0 here).
 * g_eng1_setup_word_6ef74/6ef78 are defined here (static): every unsigned-short read of them is
 * the "mov eax, dword ptr; and eax, 0xffff" form VC5 emits only for objects defined in the
 * translation unit (extern gives xor/mov ax); 0x182c0 and 0x18550 are equal only with it.
 * g_camera_flags_70d10 is short here: `|= 8` and `&= ~8` both become byte RMW ops only for a
 * signed 16-bit object (unsigned short gives `and word ptr, 0xfff7`).
 * A <string.h> context probe was also measured for these residues, but is not selected here
 * because it changes declaration context for the complete unit. 0x171b0 reads held_5c
 * once through a non-volatile lvalue (owner-approved per-access steering, 2026-10-31): the
 * original's single mov cx,[+0x5c] / test [+0x6e],cx needs exactly that one access. */
typedef struct Actor Actor;

typedef struct Bounds3D {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds3D;

struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x22 - 0x0c];
    unsigned char category;
    unsigned char mode;
    unsigned char unknown_24[0x2a - 0x24];
    short state;
    unsigned char unknown_2c[0x2e - 0x2c];
    unsigned short kind;
    short ticks, phase;
    unsigned short sprite;
    short unknown_36;
    unsigned short profile;
    unsigned short scale;
    unsigned char unknown_3c[0x4a - 0x3c];
    unsigned short unknown_4a;
    long parameter;
    unsigned long flags, render_flags;
    long record_offset;
    void *entries;
    Actor *child;
    void *image;
    unsigned char unknown_68[0x72 - 0x68];
    short unknown_72;
    short unknown_74;
    unsigned char unknown_76[0x88 - 0x76];
    Bounds3D shape;
    unsigned char unknown_94, unknown_95;       /* +0x94 facing (0 / 0xff) */
    unsigned char unknown_96, unknown_97;
    short unknown_98, unknown_9a;
    long prev_x, prev_y, prev_z;
    long unknown_a8, unknown_ac, unknown_b0, unknown_b4;
    long unknown_b8, unknown_bc, unknown_c0, unknown_c4;
    long unknown_c8, unknown_cc, unknown_d0, unknown_d4;
    long unknown_d8, unknown_dc, unknown_e0;
    unsigned char unknown_e4[0xec - 0xe4];
    unsigned char unknown_ec;
    unsigned char unknown_ed;
    short unknown_ee;
    long unknown_f0, unknown_f4;
    long unknown_f8;
    long unknown_fc, unknown_100;
    unsigned char unknown_104[0x108 - 0x104];
    long unknown_108;
    unsigned char unknown_10c[0x11e - 0x10c];
    short unknown_11e;
    unsigned char unknown_120[0x134 - 0x120];
};

/* Interface context (g_engine_interface.context_004). */
typedef struct LevelContext {
    signed char level;
    signed char sublevel;
    unsigned char unknown_02;
    unsigned char unknown_03;
    unsigned char unknown_04[0x24 - 0x04];
    short energy_24[6];
    unsigned char unknown_30[0x5c - 0x30];
    volatile unsigned short held_5c;
    unsigned short pressed_5e;
    unsigned short unknown_60, unknown_62, unknown_64, unknown_66;
    unsigned char unknown_68[0x6c - 0x68];
    unsigned short unknown_6c;
    unsigned short enabled_6e;
    unsigned short unknown_70, unknown_72;
} LevelContext;

typedef struct EngineInterfaceView {
    unsigned long signature_000;
    LevelContext *context_004;
} EngineInterfaceView;

extern EngineInterfaceView g_engine_interface;
extern Actor *g_frame_owner_72860;      /* player actor */
extern Actor *g_native_pointer_7217c;
extern Actor *g_frame_global_734f4;
extern unsigned short eng1_sim_state_72108;     /* player mode */
extern long g_eng1_7210c;
extern unsigned long eng1_sim_state_72110;
extern unsigned long eng1_sim_state_72170;
extern long g_eng1_setup_dword_7201c;
extern short g_eng1_setup_word_72012;
extern Actor *g_eng1_setup_dword_72118;
extern long g_eng1_setup_dword_7211c;
extern long g_eng1_setup_dword_72120;
extern unsigned long g_pipeline_dword_71840;
extern short g_pipeline_word_71bc4;
extern short g_pipeline_word_71c5c;
extern unsigned short g_eng1_6ef70;
extern short g_eng1_setup_word_6ef7c;
extern long g_eng1_setup_dword_6efa8;
extern short g_camera_flags_70d10;
extern unsigned char g_eng1_image_53930[];
extern unsigned char g_eng1_image_53a20[];
extern unsigned char g_eng1_image_53b00[];
extern unsigned char g_eng1_image_53b90[];
extern unsigned char g_eng1_image_53bb0[];
extern unsigned char g_eng1_image_53bc8[];
extern unsigned char g_eng1_image_53bf0[];
extern unsigned char g_eng1_image_53c00[];
extern unsigned char g_eng1_image_53c18[];
extern unsigned char g_eng1_image_53c40[];
extern unsigned char g_eng1_image_53c80[];
extern unsigned char g_eng1_image_53cc0[];
extern unsigned char g_eng1_image_53d10[];
extern unsigned char g_eng1_image_53df8[];
extern unsigned char g_eng1_image_540c0[];
extern unsigned char g_eng1_image_541f8[];
extern unsigned char g_eng1_image_53da0[];
extern unsigned char g_eng1_image_54810[];
extern unsigned char g_eng1_setup_program_54558[];
extern long g_actor_worklist_71a20[];   /* [0] count, then actor pointers */
extern unsigned short g_eng1_setup_word_718e0;

extern short g_eng1_word_7144c;
extern short g_eng1_setup_word_6ef80;
extern unsigned char g_eng1_image_53ea0[];
extern unsigned char g_eng1_image_53f80[];
extern unsigned char g_eng1_image_547e8[];
extern unsigned char g_eng1_image_548e8[];
void eng1_17a50(void);
int eng1_17ae0(void);
extern short g_eng1_setup_word_72194;
extern unsigned char g_eng1_image_53a90[];
extern unsigned char g_eng1_image_53ac8[];
extern unsigned char g_eng1_image_54da0[];
typedef struct ShortPair { short first, second; } ShortPair;
extern ShortPair g_eng1_pairs_55240[];
extern void eng1_291d0(Actor *);
extern int eng1_19cf0(void);
void eng1_16cf0(void);
void eng1_17d60(void);
typedef struct ScriptStep { short op; short arg; } ScriptStep;
typedef struct SwingRow { short angle, sprite, x, y, unknown_8; } SwingRow;
extern SwingRow g_eng1_rows_54c98[];
extern SwingRow g_eng1_rows_54d60[];
extern unsigned short g_eng1_dirs_54c78[];
extern signed char g_eng1_setup_byte_6efb4;
static unsigned short g_eng1_setup_word_6ef74;
static unsigned short g_eng1_setup_word_6ef78;
extern long g_eng1_setup_dword_6efa4;
extern short g_eng1_setup_word_72024;
extern short g_eng1_setup_word_7215a;
extern short g_eng1_setup_word_7216a;
extern Actor *g_eng1_setup_pointer_72018;
extern Actor *g_eng1_setup_pointer_72020;
extern long g_eng1_setup_clear_72140[];
extern unsigned char g_eng1_image_54120[];
extern unsigned char g_eng1_image_541b0[];
extern unsigned char g_eng1_image_541d8[];
extern unsigned char g_eng1_image_54260[];
extern unsigned char g_eng1_image_542a0[];
extern unsigned char g_eng1_image_542c8[];
extern unsigned char g_eng1_image_54318[];
extern unsigned char g_eng1_image_54380[];
extern unsigned char g_eng1_image_543d0[];
extern unsigned char g_eng1_image_54470[];
extern void eng1_1a410(void);
extern void eng1_1a280(void);
extern void eng1_18900(void);
extern int dispatch_local_15d00(void);          /* player script runner (ENG3 eng3_18370) */
extern void dispatch_local_16790(void);
extern int eng1_29c20(Actor *);
extern int eng1_29da0(Actor *);
extern int eng1_29e20(Actor *);
extern void unlink_0e4(Actor *);
extern int actor_bounds_2a880(Actor *, Bounds3D *, int);
extern int point_in_box_2a810(long, long, long, Bounds3D *);
extern unsigned int dispatch_local_30010(long, long, long, unsigned short);
extern void LinkedSoundFx_6da0(unsigned short, Actor *);
extern int point_in_planar_bounds(unsigned int, unsigned int, Bounds3D *);
extern void eng1_18f60(void);
extern int eng1_18b10(void);
extern int eng1_18bb0(void);
extern void eng1_19160(void);
extern int eng1_19820(void);
extern int eng1_19900(void);
extern void eng1_19a10(void);
extern int eng1_197b0(void);
extern int eng1_19890(void);
extern void eng1_19d70(void);
extern int eng1_19650(void);
extern int eng1_19970(int);

int eng1_16990(void);
void eng1_16a60(void);
void eng1_16c40(void);
void eng1_16e70(void);
int eng1_16fa0(void);
int eng1_16fd0(void);
void eng1_17000(void);
void eng1_17060(void);
void eng1_17130(void);
int eng1_17290(void);
void eng1_17340(void);
void eng1_174e0(long);
void eng1_176e0(void);
int eng1_17810(void);
int eng1_17a20(void);
int eng1_17ba0(void);
int eng1_17d00(void);
int eng1_17ff0(void);
int eng1_188a0(void);

void eng1_16840(void)
{
    if (eng1_sim_state_72170) {
        eng1_19160();
        return;
    }
    if (eng1_sim_state_72110) {
        if (!g_eng1_setup_dword_7201c) {
            eng1_18f60();
            return;
        }
        eng1_sim_state_72108 = 0x2f;
        g_frame_owner_72860->image = g_eng1_setup_program_54558;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
        return;
    }
    if (g_frame_owner_72860->image != g_eng1_image_53930)
        g_native_pointer_7217c->render_flags &= ~0x800000;
    g_eng1_7210c = 0;
    g_eng1_setup_word_72012 = 0;
    if (!eng1_29c20(g_frame_owner_72860)) {
        eng1_17340();
        return;
    }
    if (g_engine_interface.context_004->level == 6 && g_frame_global_734f4 != g_native_pointer_7217c)
        return;
    if (!g_eng1_setup_dword_7201c) {
        if (eng1_17ff0())
            return;
        if (eng1_17ba0())
            return;
        if (eng1_17d00())
            return;
        if (eng1_17a20())
            return;
    } else {
        if (eng1_18bb0())
            return;
    }
    if (g_pipeline_dword_71840) {
        if (!(g_frame_owner_72860->z & 0x7f0000)) {
            if (eng1_16fa0())
                return;
            if (eng1_16990())
                return;
            if (eng1_18b10())
                return;
        }
    } else {
        if (eng1_16fa0())
            return;
        if (eng1_16990())
            return;
        if (eng1_18b10())
            return;
    }
    dispatch_local_15d00();
}

int eng1_16990(void)
{
    if (g_engine_interface.context_004->held_5c & 0x80) {
        if ((g_frame_owner_72860->render_flags & 0x10) == 0x10) {
            if (!eng1_19820())
                return 0;
            eng1_16a60();
            g_frame_owner_72860->unknown_94 = 0xff;
            return 1;
        }
        if (!eng1_19820() && g_frame_owner_72860->unknown_94 == 0xff)
            return 0;
        g_frame_owner_72860->unknown_94 = 0;
        eng1_16e70();
        return 1;
    }
    if (g_engine_interface.context_004->held_5c & 0x20) {
        if (!(g_frame_owner_72860->render_flags & 0x10)) {
            if (!eng1_19900())
                return 0;
            eng1_16a60();
            g_frame_owner_72860->unknown_94 = 0;
            return 1;
        }
        if (!eng1_19900() && !g_frame_owner_72860->unknown_94)
            return 0;
        g_frame_owner_72860->unknown_94 = 0xff;
        eng1_16e70();
        return 1;
    }
    return 0;
}

void eng1_16a60(void)
{
    eng1_sim_state_72108 = 1;
    if (!g_eng1_setup_dword_7201c) {
        g_frame_owner_72860->image = g_eng1_image_53a20;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
        g_frame_owner_72860->unknown_ac = 0x80000;
        g_frame_owner_72860->unknown_b0 = 0x8000;
        g_frame_owner_72860->unknown_a8 = 0x40000;
    } else {
        g_frame_owner_72860->image = g_eng1_image_53b00;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
        g_frame_owner_72860->unknown_ac = 0x40000;
        g_frame_owner_72860->unknown_b0 = 0x2000;
        g_frame_owner_72860->unknown_a8 = 0x20000;
    }
    g_frame_owner_72860->unknown_b8 = 0;
    g_frame_owner_72860->unknown_bc = 0;
}

void eng1_16b40(void)
{
    if (eng1_sim_state_72170 || eng1_sim_state_72110) {
        eng1_16c40();
        return;
    }
    if (!g_eng1_setup_dword_7201c && (eng1_17ff0() || eng1_17ba0() || eng1_16fd0()))
        return;
    dispatch_local_15d00();
    if (g_frame_owner_72860->unknown_94) {
        if (g_engine_interface.context_004->held_5c & 0x80) {
            eng1_19a10();
            if (!eng1_29c20(g_frame_owner_72860)) {
                eng1_17340();
                return;
            }
            if (!eng1_197b0()) {
                g_frame_owner_72860->x = g_frame_owner_72860->prev_x;
                eng1_16c40();
                eng1_29c20(g_frame_owner_72860);
            }
        } else if (g_engine_interface.context_004->held_5c & 0x20) {
            eng1_16e70();
        } else {
            eng1_16c40();
        }
    } else {
        if (g_engine_interface.context_004->held_5c & 0x20) {
            eng1_19a10();
            if (!eng1_29c20(g_frame_owner_72860)) {
                eng1_17340();
                return;
            }
            if (!eng1_19890()) {
                g_frame_owner_72860->x = g_frame_owner_72860->prev_x;
                eng1_16c40();
                eng1_29c20(g_frame_owner_72860);
            }
        } else if (g_engine_interface.context_004->held_5c & 0x80) {
            eng1_16e70();
        } else {
            eng1_16c40();
        }
    }
}

void eng1_16c40(void)
{
    if (!g_eng1_setup_dword_7201c) {
        if (g_frame_owner_72860->unknown_a8 < 0x40000) {
            dispatch_local_16790();
        } else {
            eng1_sim_state_72108 = 3;
            g_frame_owner_72860->image = g_eng1_image_53b90;
            g_frame_owner_72860->ticks = -1;
            g_frame_owner_72860->phase = -1;
            dispatch_local_15d00();
        }
    } else {
        eng1_sim_state_72108 = 3;
        g_frame_owner_72860->image = g_eng1_image_53bf0;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
    }
    g_eng1_setup_dword_6efa8 = 0;
    g_frame_owner_72860->unknown_ac = 0;
    g_frame_owner_72860->unknown_a8 = 0;
    g_frame_owner_72860->unknown_b0 = 0;
    g_frame_owner_72860->unknown_b8 = 0;
    g_frame_owner_72860->unknown_bc = 0;
}

void eng1_16cf0(void)
{
    if (g_native_pointer_7217c->unknown_96) {
        eng1_sim_state_72108 = 3;
        g_frame_owner_72860->image = g_eng1_image_53bb0;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
    } else {
        eng1_sim_state_72108 = 3;
        g_frame_owner_72860->image = g_eng1_image_53bc8;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
    }
    dispatch_local_15d00();
    g_camera_flags_70d10 |= 8;
    g_frame_owner_72860->unknown_ac = 0;
    g_frame_owner_72860->unknown_a8 = 0;
    g_frame_owner_72860->unknown_b0 = 0;
    g_frame_owner_72860->unknown_b8 = 0;
    g_frame_owner_72860->unknown_bc = 0;
}

void eng1_16da0(void)
{
    eng1_29c20(g_frame_owner_72860);
    if (!g_eng1_setup_dword_7201c && !eng1_sim_state_72170 && !eng1_sim_state_72110) {
        if (eng1_17ff0() || eng1_17ba0())
            return;
        if ((g_frame_owner_72860->z >> 16 == g_pipeline_word_71c5c || g_frame_owner_72860->z >> 16 == g_pipeline_word_71bc4)
            && (eng1_16fa0() || eng1_16990()))
            return;
        if (eng1_17d00() || eng1_17a20())
            return;
    }
    if (dispatch_local_15d00())
        dispatch_local_16790();
}

int eng1_16e30(void)
{
    if (g_frame_owner_72860->unknown_94) {
        if (g_engine_interface.context_004->held_5c & 0x20) {
            eng1_16e70();
            return 1;
        }
    } else if (g_engine_interface.context_004->held_5c & 0x80) {
        eng1_16e70();
        return 1;
    }
    return 0;
}

void eng1_16e70(void)
{
    if (g_eng1_setup_dword_7201c) {
        if (g_frame_owner_72860->unknown_94) {
            eng1_sim_state_72108 = 6;
            g_frame_owner_72860->image = g_eng1_image_53c80;
            g_frame_owner_72860->ticks = -1;
            g_frame_owner_72860->phase = -1;
            dispatch_local_15d00();
        } else {
            eng1_sim_state_72108 = 6;
            g_frame_owner_72860->image = g_eng1_image_53c40;
            g_frame_owner_72860->ticks = -1;
            g_frame_owner_72860->phase = -1;
            dispatch_local_15d00();
        }
        return;
    }
    if (eng1_sim_state_72108 == 1) {
        eng1_sim_state_72108 = 6;
        g_frame_owner_72860->image = g_eng1_image_53c18;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
    } else {
        eng1_sim_state_72108 = 6;
        g_frame_owner_72860->image = g_eng1_image_53c00;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
    }
}

void eng1_16f40(void)
{
    if (!g_eng1_setup_dword_7201c && !eng1_sim_state_72170 && !eng1_sim_state_72110
        && (eng1_17d00() || eng1_17a20() || eng1_16fa0()))
        return;
    eng1_29c20(g_frame_owner_72860);
    if (dispatch_local_15d00())
        dispatch_local_16790();
}

int eng1_16fa0(void)
{
    if ((g_engine_interface.context_004->pressed_5e & g_engine_interface.context_004->enabled_6e)
        && !g_eng1_setup_dword_7201c) {
        eng1_17000();
        return 1;
    }
    return 0;
}

int eng1_16fd0(void)
{
    if ((g_engine_interface.context_004->pressed_5e & g_engine_interface.context_004->enabled_6e)
        && !g_eng1_setup_dword_7201c) {
        eng1_17060();
        return 1;
    }
    return 0;
}

void eng1_17000(void)
{
    eng1_sim_state_72108 = 4;
    g_frame_owner_72860->image = g_eng1_image_53cc0;
    g_frame_owner_72860->ticks = -1;
    g_frame_owner_72860->phase = -1;
    dispatch_local_15d00();
    g_eng1_6ef70 = 4;
    eng1_17130();
    g_frame_owner_72860->unknown_108 = 0x40000;
    g_frame_owner_72860->unknown_b0 = 0x8000;
}

void eng1_17060(void)
{
    eng1_sim_state_72108 = 2;
    g_frame_owner_72860->image = g_eng1_image_53d10;
    g_frame_owner_72860->ticks = -1;
    g_frame_owner_72860->phase = -1;
    dispatch_local_15d00();
    g_eng1_6ef70 = 2;
    eng1_17130();
    unlink_0e4(g_frame_owner_72860);
    g_frame_owner_72860->unknown_108 = g_frame_owner_72860->unknown_a8;
}

void eng1_170c0(void)
{
    eng1_sim_state_72108 = 0x17;
    g_frame_owner_72860->image = g_eng1_image_53cc0;
    g_frame_owner_72860->ticks = -1;
    g_frame_owner_72860->phase = -1;
    dispatch_local_15d00();
    eng1_17130();
    unlink_0e4(g_frame_owner_72860);
    g_frame_owner_72860->unknown_108 = 0x40000;
    g_frame_owner_72860->unknown_b0 = 0x8000;
}

void eng1_17130(void)
{
    g_frame_owner_72860->unknown_95 = 0xff;
    g_frame_owner_72860->unknown_b8 = 0xe0000;
    g_frame_owner_72860->unknown_c0 = 0x7000;
    g_frame_owner_72860->unknown_c4 = 0xe000;
    g_frame_owner_72860->unknown_bc = 0xd0000;
    g_frame_owner_72860->unknown_ac = 0;
    g_eng1_setup_word_6ef7c = 0;
    g_native_pointer_7217c->render_flags &= ~0x800000;
    g_eng1_setup_word_72012 = 0;
}

void eng1_171b0(void)
{
    eng1_174e0(0x40000);
    if (!eng1_sim_state_72110) {
        if (eng1_17810() || eng1_188a0())
            return;
        eng1_17290();
        if (g_eng1_6ef70 < 0x2000 && (eng1_sim_state_72108 == 4 || eng1_sim_state_72108 == 2)
            && !(*((unsigned short *)&g_engine_interface.context_004->held_5c) & g_engine_interface.context_004->enabled_6e)) {
            g_frame_owner_72860->unknown_b8 -= 0xc000;
            if (g_frame_owner_72860->unknown_b8 < 0)
                g_frame_owner_72860->unknown_b8 = 0;
        }
    }
    eng1_29da0(g_native_pointer_7217c);
    if (g_frame_owner_72860->sprite != 0x1c7 && g_frame_owner_72860->sprite != 0x1c8)
        eng1_19d70();
    if (g_frame_owner_72860->sprite == 0x1c9)
        unlink_0e4(g_frame_owner_72860);
    dispatch_local_15d00();
}

int eng1_17290(void)
{
    /* Two conditional input reads stay at their original sites; each casts one access. */
    if (g_eng1_setup_word_6ef7c && (*((unsigned short *)&g_engine_interface.context_004->held_5c) & g_engine_interface.context_004->unknown_6c)
        && !g_eng1_setup_dword_7211c) {
        g_frame_owner_72860->image = g_eng1_image_541f8;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        g_eng1_setup_dword_7211c = 1;
        g_eng1_setup_word_6ef7c = 0;
        return 1;
    }
    if (g_eng1_setup_word_6ef7c && (*((unsigned short *)&g_engine_interface.context_004->held_5c) & g_engine_interface.context_004->unknown_70)) {
        g_frame_owner_72860->image = g_eng1_image_540c0;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        g_eng1_setup_word_6ef7c = 0;
        return 1;
    }
    return 0;
}

void eng1_17340(void)
{
    eng1_sim_state_72108 = 7;
    g_frame_owner_72860->image = g_eng1_image_53df8;
    g_frame_owner_72860->ticks = -1;
    g_frame_owner_72860->phase = -1;
    dispatch_local_15d00();
    g_frame_owner_72860->unknown_108 = g_frame_owner_72860->unknown_ac;
    g_frame_owner_72860->unknown_ac = 0x40000;
    g_frame_owner_72860->unknown_b4 = 0x8000;
    g_frame_owner_72860->unknown_b0 = 0x2000;
    g_frame_owner_72860->unknown_b8 = 0x40000;
    g_frame_owner_72860->unknown_bc = 0xd0000;
    g_frame_owner_72860->unknown_c0 = 0x8000;
    g_frame_owner_72860->unknown_95 = 0;
    g_eng1_6ef70 = 7;
}

void eng1_17400(void)
{
    Bounds3D bounds;

    eng1_19d70();
    if (!eng1_sim_state_72110) {
        if (g_eng1_6ef70 != 0x12 && (eng1_188a0() || eng1_17810()))
            return;
        eng1_17290();
        if (g_engine_interface.context_004->level == 3 && g_eng1_setup_dword_72118 && g_eng1_setup_dword_72120 == 1
            && actor_bounds_2a880(g_eng1_setup_dword_72118, &bounds, 3)
            && point_in_planar_bounds(g_frame_owner_72860->x, g_frame_owner_72860->y, &bounds)) {
            g_eng1_setup_dword_72120 = 2;
            eng1_sim_state_72108 = 0xd;
            return;
        }
    }
    if (eng1_29e20(g_native_pointer_7217c)) {
        eng1_176e0();
        return;
    }
    eng1_174e0(0x20000);
    dispatch_local_15d00();
}

void eng1_174e0(long speed)   /* argument unused (0x40000 / 0x20000 at the call sites) */
{
    g_frame_owner_72860->unknown_ac = 0;
    if (eng1_sim_state_72108 == 0x13) {
        g_frame_owner_72860->unknown_a8 = 0x30000;
        g_frame_owner_72860->unknown_ac = 0x30000;
    } else if (g_engine_interface.context_004->held_5c & 0xa0) {
        g_frame_owner_72860->unknown_ac = g_frame_owner_72860->unknown_108;
    }
    if (g_frame_owner_72860->unknown_94) {
        if (g_engine_interface.context_004->held_5c & 0x20) {
            if (g_eng1_6ef70 == 5) {
                if (g_frame_owner_72860->unknown_a8 < 0xc000) {
                    g_frame_owner_72860->unknown_b0 = 0x6000;
                    g_frame_owner_72860->unknown_94 = 0;
                    g_native_pointer_7217c->render_flags = g_native_pointer_7217c->unknown_94 & 0x10
                                                         | g_native_pointer_7217c->render_flags & ~0x10;
                } else {
                    g_frame_owner_72860->unknown_ac = 0x4000;
                    g_frame_owner_72860->unknown_b4 = 0x5000;
                }
            } else {
                g_frame_owner_72860->unknown_94 = 0;
                g_native_pointer_7217c->render_flags = g_native_pointer_7217c->unknown_94 & 0x10
                                                     | g_native_pointer_7217c->render_flags & ~0x10;
            }
        }
    } else {
        if (g_engine_interface.context_004->held_5c & 0x80) {
            if (g_eng1_6ef70 == 5) {
                if (g_frame_owner_72860->unknown_a8 < 0xc000) {
                    g_frame_owner_72860->unknown_b0 = 0x6000;
                    g_frame_owner_72860->unknown_94 = 0xff;
                    g_native_pointer_7217c->render_flags = g_native_pointer_7217c->unknown_94 & 0x10
                                                         | g_native_pointer_7217c->render_flags & ~0x10;
                } else {
                    g_frame_owner_72860->unknown_ac = 0x4000;
                    g_frame_owner_72860->unknown_b4 = 0x5000;
                }
            } else {
                g_frame_owner_72860->unknown_94 = 0xff;
                g_native_pointer_7217c->render_flags = g_native_pointer_7217c->unknown_94 & 0x10
                                                     | g_native_pointer_7217c->render_flags & ~0x10;
            }
        }
    }
    if (eng1_sim_state_72108 == 0x13) {
        eng1_19a10();
    } else {
        unsigned int left, right;
        int hit;

        eng1_19a10();
        if (!eng1_19650())
            g_frame_owner_72860->x = g_frame_owner_72860->prev_x;
        left = dispatch_local_30010(g_frame_owner_72860->x - 0x1c0000, g_frame_owner_72860->y - 0x10000,
                                    g_frame_owner_72860->z, 1);
        right = dispatch_local_30010(g_frame_owner_72860->x + 0x1c0000, g_frame_owner_72860->y - 0x10000,
                                     g_frame_owner_72860->z, 1);
        if (left && right)
            return;
        if (left)
            g_frame_owner_72860->x += 0x20000;
        if (right)
            g_frame_owner_72860->x -= 0x20000;
        hit = eng1_19970(0);
        if (hit) {
            if (((Actor *)g_actor_worklist_71a20[hit])->x > g_frame_owner_72860->x)
                g_frame_owner_72860->x -= 0x20000;
            else
                g_frame_owner_72860->x += 0x20000;
        }
    }
}

void eng1_176e0(void)
{
    eng1_sim_state_72108 = 8;
    g_frame_owner_72860->image = g_eng1_image_53da0;
    g_frame_owner_72860->ticks = -1;
    g_frame_owner_72860->phase = -1;
    dispatch_local_15d00();
    g_eng1_6ef70 = 0;
    g_frame_owner_72860->unknown_a8 = 0;
    g_frame_owner_72860->unknown_108 = 0;
    g_frame_owner_72860->unknown_b0 = 0;
    g_frame_owner_72860->y &= 0xffff0000;
    if (g_eng1_setup_word_718e0 && g_frame_owner_72860->y >> 16 > g_eng1_setup_word_718e0
        && (g_engine_interface.context_004->level == 3 || g_engine_interface.context_004->level == 1))
        LinkedSoundFx_6da0(0x4276, g_native_pointer_7217c);
    g_native_pointer_7217c->render_flags &= ~0x800000;
}

void eng1_177a0(void)
{
    if (!eng1_sim_state_72170 && !eng1_sim_state_72110
        && (eng1_17ff0() || eng1_17ba0() || eng1_16fa0() || eng1_16990() || eng1_17d00() || eng1_17a20()
            || eng1_16e30()))
        return;
    eng1_29c20(g_frame_owner_72860);
    if (dispatch_local_15d00())
        dispatch_local_16790();
}

int eng1_17810(void)
{
    Bounds3D bounds;
    int i;
    Actor *p;

    if (actor_bounds_2a880(g_native_pointer_7217c, &bounds, 3)) {
        for (i = 0; i < g_actor_worklist_71a20[0]; i++) {
            p = (Actor *)g_actor_worklist_71a20[i + 1];
            if (point_in_box_2a810(p->x, p->y, p->z, &bounds)) {
                /* identical arms: the original still tests +0x94 before the store */
                g_frame_owner_72860->x = p->x;
                g_frame_owner_72860->y = p->y + 0x300000;
                eng1_sim_state_72108 = 9;
                g_frame_owner_72860->image = g_eng1_image_54810;
                g_frame_owner_72860->sprite = 0x126;
                g_frame_owner_72860->ticks = 1;
                g_frame_owner_72860->phase = -1;
                return 1;
            }
        }
    }
    return 0;
}

void eng1_178f0(void)
{
    g_eng1_word_7144c = 0;
    if (!dispatch_local_15d00())
        return;
    eng1_sim_state_72108 = 5;
    g_frame_owner_72860->image = g_eng1_image_548e8;
    g_frame_owner_72860->ticks = -1;
    g_frame_owner_72860->phase = -1;
    dispatch_local_15d00();
    g_eng1_6ef70 = 5;
    g_frame_owner_72860->unknown_95 = 0xff;
    g_frame_owner_72860->unknown_108 = 0x40000;
    g_frame_owner_72860->unknown_b8 = 0xf0000;
    g_frame_owner_72860->unknown_c0 = 0x7000;
    g_frame_owner_72860->unknown_c4 = 0xe000;
    g_frame_owner_72860->unknown_bc = 0xd0000;
    g_frame_owner_72860->unknown_ac = 0x40000;
    g_frame_owner_72860->unknown_a8 = 0x40000;
    g_frame_owner_72860->unknown_b0 = 0x4000;
    if (eng1_sim_state_72110) {
        g_frame_owner_72860->unknown_ac = 0x8000;
        g_frame_owner_72860->unknown_a8 = 0x8000;
        g_frame_owner_72860->unknown_b0 = 0x1000;
    }
    g_eng1_setup_word_6ef7c = 0;
    g_frame_owner_72860->unknown_108 = g_frame_owner_72860->unknown_a8;
}

int eng1_17a20(void)
{
    if ((g_engine_interface.context_004->held_5c & 0x40) && !g_eng1_setup_word_6ef80) {
        eng1_17a50();
        return 1;
    }
    return 0;
}

void eng1_17a50(void)
{
    eng1_sim_state_72108 = 0xb;
    g_frame_owner_72860->image = g_eng1_image_547e8;
    g_frame_owner_72860->ticks = -1;
    g_frame_owner_72860->phase = -1;
    dispatch_local_15d00();
}

void eng1_17a90(void)
{
    if (!eng1_29c20(g_frame_owner_72860)) {
        eng1_17340();
        return;
    }
    if (eng1_16fa0())
        return;
    eng1_17ae0();
    if (!(g_engine_interface.context_004->held_5c & 0x40) && eng1_16990())
        return;
    if (dispatch_local_15d00())
        dispatch_local_16790();
}

int eng1_17ae0(void)
{
    if (g_frame_owner_72860->unknown_94 == 0xff && (g_engine_interface.context_004->held_5c & 0x20)) {
        g_frame_owner_72860->unknown_94 = 0;
        g_native_pointer_7217c->render_flags = g_native_pointer_7217c->unknown_94 & 0x10
                                             | g_native_pointer_7217c->render_flags & ~0x10;
        if (g_native_pointer_7217c->child)
            g_native_pointer_7217c->child->render_flags = g_native_pointer_7217c->unknown_94 & 0x10
                                                             | g_native_pointer_7217c->child->render_flags & ~0x10;
        return 1;
    }
    if (!g_frame_owner_72860->unknown_94 && (g_engine_interface.context_004->held_5c & 0x80)) {
        g_frame_owner_72860->unknown_94 = 0xff;
        g_native_pointer_7217c->render_flags = g_native_pointer_7217c->unknown_94 & 0x10
                                             | g_native_pointer_7217c->render_flags & ~0x10;
        if (g_native_pointer_7217c->child)
            g_native_pointer_7217c->child->render_flags = g_native_pointer_7217c->unknown_94 & 0x10
                                                             | g_native_pointer_7217c->child->render_flags & ~0x10;
        return 1;
    }
    return 0;
}

int eng1_17ba0(void)
{
    if (g_engine_interface.context_004->held_5c & g_engine_interface.context_004->unknown_70) {
        eng1_sim_state_72108 = 0x29;
        g_frame_owner_72860->image = g_eng1_image_53ea0;
    } else if (g_engine_interface.context_004->held_5c & g_engine_interface.context_004->unknown_72) {
        eng1_sim_state_72108 = 0x2a;
        g_frame_owner_72860->image = g_eng1_image_53f80;
    } else {
        return 0;
    }
    g_frame_owner_72860->ticks = -1;
    g_frame_owner_72860->phase = -1;
    dispatch_local_15d00();
    g_frame_owner_72860->unknown_a8 = 0;
    return 1;
}

void eng1_17c30(void)
{
    if (!eng1_29c20(g_frame_owner_72860)) {
        eng1_17340();
        return;
    }
    if (eng1_16fa0())
        return;
    eng1_17ae0();
    if (dispatch_local_15d00())
        dispatch_local_16790();
}

void eng1_17c70(void)
{
    if ((g_eng1_setup_word_72012 & 2) && g_eng1_setup_word_72194) {
        if ((g_eng1_setup_word_72194 -= 0x3c) < 0)
            g_eng1_setup_word_72194 = 0;
    }
    if (!eng1_29c20(g_frame_owner_72860)) {
        eng1_17340();
        return;
    }
    if (eng1_16fa0())
        return;
    if (dispatch_local_15d00())
        dispatch_local_16790();
}

void eng1_17cd0(void)
{
    if (!eng1_29c20(g_frame_owner_72860)) {
        eng1_17340();
        return;
    }
    if (!eng1_16fa0() && dispatch_local_15d00())
        dispatch_local_16790();
}

int eng1_17d00(void)
{
    if (g_pipeline_dword_71840) {
        if ((g_engine_interface.context_004->held_5c & 0x10) && g_frame_owner_72860->z >> 16 < g_pipeline_word_71bc4) {
            eng1_17d60();
            return 1;
        }
        if ((g_engine_interface.context_004->held_5c & 0x40) && g_frame_owner_72860->z >> 16 > g_pipeline_word_71c5c) {
            eng1_17d60();
            return 1;
        }
    }
    return 0;
}

void eng1_17d60(void)
{
    g_frame_owner_72860->unknown_cc = 0;
    g_frame_owner_72860->unknown_d0 = 0;
    g_frame_owner_72860->unknown_c8 = 0;
    g_frame_owner_72860->unknown_ac = 0;
    g_frame_owner_72860->unknown_a8 = 0;
    g_frame_owner_72860->unknown_b8 = 0;
    g_frame_owner_72860->unknown_bc = 0;
    g_native_pointer_7217c->unknown_f0 = (long)g_eng1_image_54da0;
    g_native_pointer_7217c->unknown_f8 = 0xa;
    g_native_pointer_7217c->unknown_11e = 0xf;
    g_native_pointer_7217c->unknown_72 = 0;
    if (g_engine_interface.context_004->held_5c & 0x10) {
        eng1_sim_state_72108 = 0x1c;
        g_frame_owner_72860->image = g_eng1_image_53a90;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
        g_frame_owner_72860->unknown_96 = 0xff;
        g_native_pointer_7217c->unknown_f4 = g_eng1_pairs_55240[g_engine_interface.context_004->level].first;
    } else {
        eng1_sim_state_72108 = 0x1c;
        g_frame_owner_72860->image = g_eng1_image_53ac8;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
        g_frame_owner_72860->unknown_96 = 0;
        g_native_pointer_7217c->unknown_f4 = g_eng1_pairs_55240[g_engine_interface.context_004->level].second;
    }
}

void eng1_17eb0(void)
{
    int d;

    g_camera_flags_70d10 &= ~8;
    g_eng1_setup_word_6ef80 = 0x32;
    if (g_native_pointer_7217c->unknown_96) {
        if (g_engine_interface.context_004->held_5c & 0x40) {
            g_frame_owner_72860->image = g_eng1_image_53ac8;
            g_frame_owner_72860->unknown_96 = 0;
        }
    } else if (g_engine_interface.context_004->held_5c & 0x10) {
        g_frame_owner_72860->image = g_eng1_image_53a90;
        g_frame_owner_72860->unknown_96 = 0xff;
    }
    eng1_291d0(g_native_pointer_7217c);
    if (eng1_19cf0()) {
        if (!eng1_29c20(g_frame_owner_72860)) {
            eng1_17340();
            return;
        }
        eng1_16cf0();
        return;
    }
    if (!(g_engine_interface.context_004->held_5c & 0x50)) {
        d = (g_frame_owner_72860->prev_z >> 16 & 0x7f) - (g_frame_owner_72860->z >> 16 & 0x7f);
        if (g_native_pointer_7217c->unknown_96) {
            if (d >= 0) {
                eng1_29c20(g_native_pointer_7217c);
                g_frame_owner_72860->z &= 0x7f800000;
                eng1_16cf0();
                return;
            }
        } else if (d <= 0) {
            eng1_29c20(g_native_pointer_7217c);
            g_frame_owner_72860->z = (g_frame_owner_72860->z & 0x7f800000) + 0x800000;
            eng1_16cf0();
            return;
        }
    }
    if (!eng1_29c20(g_frame_owner_72860)) {
        eng1_17340();
        return;
    }
    dispatch_local_15d00();
}

int eng1_17ff0(void)
{
    /* This input test has one explicit held-word read, matching the native entry. */
    if (*((unsigned short *)&g_engine_interface.context_004->held_5c) & g_engine_interface.context_004->unknown_6c) {
    g_eng1_setup_word_6ef78 = 0;
    g_eng1_setup_word_72024 = 0;
    g_eng1_setup_word_7215a = 0;
    switch (g_eng1_setup_byte_6efb4) {
    case 1:
        eng1_sim_state_72108 = 0x2b;
        g_frame_owner_72860->image = g_eng1_image_54260;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
        g_eng1_setup_word_7216a = 0xd9;
        g_eng1_setup_pointer_72020->x = (g_eng1_rows_54c98[g_eng1_setup_word_6ef78].x << 16) + g_native_pointer_7217c->x;
        g_eng1_setup_pointer_72020->y = (g_eng1_rows_54c98[g_eng1_setup_word_6ef78].y << 16) + g_native_pointer_7217c->y;
        g_eng1_setup_pointer_72020->unknown_74 = g_eng1_rows_54c98[g_eng1_setup_word_6ef78].unknown_8;
        g_eng1_setup_word_6ef74 = g_eng1_rows_54c98[g_eng1_setup_word_6ef78].angle;
        if (g_frame_owner_72860->unknown_94)
            g_eng1_setup_word_6ef78 = 0x13;
        g_eng1_setup_word_6ef74 = g_eng1_rows_54c98[g_eng1_setup_word_6ef78].angle;
        break;
    case 2:
        eng1_sim_state_72108 = 0x2c;
        g_frame_owner_72860->image = g_eng1_image_542a0;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
        g_eng1_setup_word_7216a = 0xd9;
        if (g_frame_owner_72860->unknown_94)
            g_eng1_setup_word_6ef78 = 5;
        g_eng1_setup_word_6ef74 = g_eng1_rows_54d60[g_eng1_setup_word_6ef78].angle;
        break;
    case 3:
        eng1_sim_state_72108 = 0x2d;
        g_frame_owner_72860->image = g_eng1_image_543d0;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
        break;
    default:
        if (g_eng1_setup_dword_7211c)
            return 0;
        eng1_sim_state_72108 = 0xc;
        g_frame_owner_72860->image = g_eng1_image_54120;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
        g_eng1_setup_dword_7211c = 1;
        break;
    }
    g_frame_owner_72860->unknown_a8 = 0;
    return 1;
    }
    return 0;
}

void eng1_18240(void)
{
    g_eng1_setup_dword_6efa4 = 0x32;
    if (!eng1_29c20(g_frame_owner_72860)) {
        eng1_17340();
        return;
    }
    if (g_frame_owner_72860->image == g_eng1_image_541b0 && eng1_17ff0())
        return;
    if (eng1_16fa0()) {
        g_eng1_setup_word_6ef7c = 0;
        g_native_pointer_7217c->render_flags &= ~0x800000;
        return;
    }
    eng1_17ae0();
    if (dispatch_local_15d00()) {
        dispatch_local_16790();
        g_eng1_setup_dword_7211c = 0;
    }
}

void eng1_182c0(void)
{
    unsigned int buttons;
    int dir;

    g_eng1_setup_dword_6efa4 = 0x32;
    if (!eng1_29c20(g_frame_owner_72860)) {
        eng1_1a410();
        eng1_17340();
        return;
    }
    if (eng1_16fa0()) {
        eng1_1a410();
        return;
    }
    if (g_eng1_setup_pointer_72020->kind) {
        if (!g_eng1_setup_clear_72140[g_eng1_setup_byte_6efb4]) {
            g_engine_interface.context_004->energy_24[g_eng1_setup_byte_6efb4]--;
            if (g_engine_interface.context_004->energy_24[g_eng1_setup_byte_6efb4] <= 0) {
                g_engine_interface.context_004->energy_24[g_eng1_setup_byte_6efb4] = 0;
                g_eng1_setup_byte_6efb4 = 0;
                eng1_1a410();
                eng1_sim_state_72108 = 0x2e;
                g_frame_owner_72860->image = g_eng1_image_541d8;
                g_frame_owner_72860->ticks = -1;
                g_frame_owner_72860->phase = -1;
                dispatch_local_15d00();
                return;
            }
        }
        buttons = g_engine_interface.context_004->held_5c & 0xf0;
        dir = 0;
        if (buttons & 0x10)
            dir = 1;
        if (buttons & 0x20)
            dir += 2;
        if (buttons & 0x40)
            dir += 4;
        if (buttons & 0x80)
            dir += 8;
        if (dir && (g_eng1_setup_word_6ef74 = g_eng1_dirs_54c78[dir]) != 200) {
            if ((g_eng1_setup_word_6ef74 = g_eng1_dirs_54c78[dir]) == 100)
                g_eng1_setup_word_6ef74 = g_frame_owner_72860->unknown_94 ? 24 : 16;
        } else {
            g_eng1_setup_word_6ef74 = g_frame_owner_72860->unknown_94 ? 40 : 0;
        }
        if (g_eng1_setup_word_6ef74 != g_eng1_rows_54c98[g_eng1_setup_word_6ef78].angle) {
            if (g_eng1_setup_word_6ef74 > g_eng1_rows_54c98[g_eng1_setup_word_6ef78].angle)
                g_eng1_setup_word_6ef78++;
            else
                g_eng1_setup_word_6ef78--;
        }
        if (--g_eng1_setup_word_72024 < 0) {
            g_eng1_setup_word_72024 = 1;
            if (++g_eng1_setup_word_7215a >= 5)
                g_eng1_setup_word_7215a = 0;
        }
        g_eng1_setup_word_7216a = g_eng1_rows_54c98[g_eng1_setup_word_6ef78].sprite + g_eng1_setup_word_7215a;
        if (g_eng1_rows_54c98[g_eng1_setup_word_6ef78].angle > 0x14)
            g_frame_owner_72860->unknown_94 = 0xff;
        else
            g_frame_owner_72860->unknown_94 = 0;
        g_native_pointer_7217c->render_flags = g_native_pointer_7217c->unknown_94 & 0x10
                                             | g_native_pointer_7217c->render_flags & ~0x10;
        g_eng1_setup_pointer_72020->x = (g_eng1_rows_54c98[g_eng1_setup_word_6ef78].x << 16) + g_native_pointer_7217c->x;
        g_eng1_setup_pointer_72020->y = (g_eng1_rows_54c98[g_eng1_setup_word_6ef78].y << 16) + g_native_pointer_7217c->y;
        g_eng1_setup_pointer_72020->unknown_74 = g_eng1_rows_54c98[g_eng1_setup_word_6ef78].unknown_8;
    }
    if (dispatch_local_15d00()) {
        eng1_sim_state_72108 = 0x2e;
        g_frame_owner_72860->image = g_eng1_image_541d8;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
    }
}

void eng1_18550(void)
{
    unsigned int buttons;
    int dir;

    g_eng1_setup_dword_6efa4 = 0x32;
    if (!eng1_29c20(g_frame_owner_72860)) {
        eng1_17340();
        return;
    }
    if (eng1_16fa0())
        return;
    if (((ScriptStep *)g_frame_owner_72860->image)[g_frame_owner_72860->phase].op == -28) {
        buttons = g_engine_interface.context_004->held_5c & 0xf0;
        dir = 0;
        if (buttons & 0x10)
            dir = 1;
        if (buttons & 0x20)
            dir += 2;
        if (buttons & 0x40)
            dir += 4;
        if (buttons & 0x80)
            dir += 8;
        if (dir && (g_eng1_setup_word_6ef74 = g_eng1_dirs_54c78[dir]) != 200) {
            if ((g_eng1_setup_word_6ef74 = g_eng1_dirs_54c78[dir]) == 100)
                g_eng1_setup_word_6ef74 = g_frame_owner_72860->unknown_94 ? 24 : 16;
        } else {
            g_eng1_setup_word_6ef74 = g_frame_owner_72860->unknown_94 ? 40 : 0;
        }
        if (g_eng1_setup_word_6ef74 != g_eng1_rows_54d60[g_eng1_setup_word_6ef78].angle) {
            if (g_eng1_setup_word_6ef74 > g_eng1_rows_54d60[g_eng1_setup_word_6ef78].angle)
                g_eng1_setup_word_6ef78++;
            else
                g_eng1_setup_word_6ef78--;
        } else {
            switch (g_eng1_rows_54d60[g_eng1_setup_word_6ef78].sprite) {
            case 0xeb:
                g_frame_owner_72860->image = g_eng1_image_54318;
                break;
            case 0xf5:
                g_frame_owner_72860->image = g_eng1_image_54380;
                break;
            case 0xfa:
                g_frame_owner_72860->image = g_eng1_image_542c8;
                break;
            }
            g_frame_owner_72860->ticks = -1;
            g_frame_owner_72860->phase = -1;
        }
        if (--g_eng1_setup_word_72024 < 0) {
            g_eng1_setup_word_72024 = 1;
            if (++g_eng1_setup_word_7215a >= 5)
                g_eng1_setup_word_7215a = 0;
        }
        g_eng1_setup_word_7216a = g_eng1_rows_54d60[g_eng1_setup_word_6ef78].sprite + g_eng1_setup_word_7215a;
        if (g_eng1_rows_54d60[g_eng1_setup_word_6ef78].angle > 0x14)
            g_frame_owner_72860->unknown_94 = 0xff;
        else
            g_frame_owner_72860->unknown_94 = 0;
        g_native_pointer_7217c->render_flags = g_native_pointer_7217c->unknown_94 & 0x10
                                             | g_native_pointer_7217c->render_flags & ~0x10;
    }
    if (dispatch_local_15d00()) {
        eng1_sim_state_72108 = 0x2e;
        g_frame_owner_72860->image = g_eng1_image_541d8;
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
    }
}

void eng1_18770(void)
{
    g_eng1_setup_dword_6efa4 = 0x32;
    if (!eng1_29c20(g_frame_owner_72860)) {
        eng1_17340();
        return;
    }
    if (eng1_16fa0())
        return;
    if (g_eng1_setup_pointer_72018->kind && !g_eng1_setup_clear_72140[g_eng1_setup_byte_6efb4]) {
        g_engine_interface.context_004->energy_24[g_eng1_setup_byte_6efb4]--;
        if (g_engine_interface.context_004->energy_24[g_eng1_setup_byte_6efb4] <= 0) {
            g_engine_interface.context_004->energy_24[g_eng1_setup_byte_6efb4] = 0;
            g_eng1_setup_byte_6efb4 = 0;
            eng1_1a280();
            eng1_sim_state_72108 = 0x2e;
            g_frame_owner_72860->image = g_eng1_image_54470;
            g_frame_owner_72860->ticks = -1;
            g_frame_owner_72860->phase = -1;
            return;
        }
    }
    if (dispatch_local_15d00())
        dispatch_local_16790();
}

void eng1_18830(void)
{
    if (!g_eng1_setup_byte_6efb4 && g_eng1_setup_word_6ef7c && eng1_17ff0())
        return;
    if (eng1_16fa0() || eng1_16990() || eng1_17ba0() || eng1_17a20())
        return;
    if (!eng1_29c20(g_frame_owner_72860)) {
        eng1_17340();
        return;
    }
    if (dispatch_local_15d00())
        dispatch_local_16790();
}

int eng1_188a0(void)
{
    if (eng1_sim_state_72108 != 0x13
        && dispatch_local_30010(g_frame_owner_72860->x + (g_frame_owner_72860->unknown_94 ? -0x240000 : 0x240000),
                                g_frame_owner_72860->y - 0x300000, g_frame_owner_72860->z, 0x3000)) {
        eng1_18900();
        return 1;
    }
    return 0;
}
}
