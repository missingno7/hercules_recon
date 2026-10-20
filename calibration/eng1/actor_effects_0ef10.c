/* ENG1.DLL 0xef10..0xf380: actor z query / water reflection handlers (ENG1 counterparts of ENG3 0x15340,
 * 0x145d0 and 0x14850; lane y9, 2026-10-20).
 * Actor/context offsets are provisional shared names, not recovered original types. */
typedef struct Actor Actor;

typedef struct Bounds3D {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds3D;

struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x1c - 0x0c];
    unsigned char unknown_1c;
    unsigned char unknown_1d[0x22 - 0x1d];
    unsigned char category;
    unsigned char mode;
    unsigned char unknown_24;
    unsigned char unknown_25;
    short unknown_26;
    unsigned short unknown_28;
    short state;
    unsigned char unknown_2c[0x2e - 0x2c];
    unsigned short kind;
    short ticks, phase;
    unsigned short sprite;
    short unknown_36;
    unsigned short profile;
    unsigned short scale;
    short unknown_3c;
    unsigned char unknown_3e[0x4a - 0x3e];
    unsigned short unknown_4a;
    long parameter;
    unsigned long flags, render_flags;
    unsigned char *owner_context;
    Actor *attachment;
    unsigned char unknown_60[0x64 - 0x60];
    void *image;
    unsigned char unknown_68[0x70 - 0x68];
    short speed, step;
    short unknown_74;
    unsigned char unknown_76[0x88 - 0x76];
    Bounds3D shape;
    unsigned char unknown_94, unknown_95;
    unsigned char unknown_96[0x98 - 0x96];
    short unknown_98, unknown_9a;
    unsigned char unknown_9c[0xa8 - 0x9c];
    long unknown_a8, unknown_ac, unknown_b0, unknown_b4;
    long unknown_b8, unknown_bc, unknown_c0, unknown_c4;
    long unknown_c8, unknown_cc, unknown_d0, unknown_d4;
    long unknown_d8, unknown_dc, unknown_e0;
    unsigned char unknown_e4[0xec - 0xe4];
    unsigned char unknown_ec;
    unsigned char unknown_ed;
    short unknown_ee;
    unsigned char unknown_f0[0x116 - 0xf0];
    short unknown_116;
    unsigned char unknown_118[0x12c - 0x118];
    const long *unknown_12c;
};

/* Interface context (g_engine_interface.context_004). */
typedef struct LevelContext {
    signed char level;
    signed char sublevel;
    unsigned char unknown_02;
    unsigned char unknown_03[0x0c - 0x03];
    signed char unknown_0c;
    unsigned char unknown_0d[0x11 - 0x0d];
    unsigned char flags_11;
    unsigned char unknown_12[0x14 - 0x12];
    short unknown_14;
    unsigned char unknown_16[0x18 - 0x16];
    short unknown_18;
    unsigned char unknown_1a[0x1c - 0x1a];
    unsigned short unknown_1c;
    short bar_length_target;
    short bar_length;
    short bar_value;
    short bar_value_target;
} LevelContext;

typedef struct EngineInterfaceView {
    unsigned long signature_000;
    LevelContext *context_004;
} EngineInterfaceView;

typedef struct KindInfo {
    unsigned long sidecar_offset;
    unsigned long unknown;
} KindInfo;

/* Per-object record in the owner context, located through g_kind_info[profile]. */
typedef struct Sidecar {
    unsigned char unknown_00[4];
    long y;
    unsigned char unknown_08[4];
    unsigned char kind;
} Sidecar;

/* Follower trail: five 16-byte position records at 0x41d60, cursor at 0x37b60. */
typedef struct TrailPoint {
    long x, y, z;
    unsigned short profile;
    short unknown_0e;
} TrailPoint;


/* ENG1.DLL counterparts of ENG3 0x15340, 0x145d0, 0x14850 (cross-check only). */
extern unsigned short g_eng1_setup_word_718e0;
extern short g_pipeline_dword_716a8;
extern long g_pipeline_dword_71718;
extern long grid_scan_20b50(long, long, long, int);
extern int eng1_299d0(Actor *);

void eng1_native_code_target_000ef10(Actor *o)
{
    unsigned int c;

    if (o->z < -0x100000) {
        o->unknown_25 |= 2;
        o->render_flags &= ~8;
        return;
    }
    c = (unsigned short)grid_scan_20b50(o->x >> 16, o->y >> 16, o->z >> 16, o->profile);
    if (c == 0x7fff) {
        o->unknown_25 |= 2;
        o->render_flags &= ~8;
        return;
    }
    o->unknown_25 &= ~2;
    if (c > 0x100) {
        o->render_flags &= ~8;
        return;
    }
    o->unknown_28 = c;
    o->render_flags |= 8;
}

void eng1_native_code_target_00f0a0(Actor *o)
{
    Actor *p = o->attachment;
    long h;

    if (~p->flags & 8)
        o->y = p->y - 0x20000;
    o->x = p->x;
    o->z = p->z;
    o->render_flags &= 0xf3ffffef;
    o->render_flags |= p->render_flags & 0x10;
    o->render_flags |= 0x80000000;
    o->render_flags |= p->render_flags & 0x8000000;
    o->render_flags |= 0x4000000;
    o->category = p->category;
    o->sprite = p->sprite;
    o->unknown_36 = p->unknown_36;
    h = grid_scan_20b50(o->x >> 16, o->y >> 16, o->z >> 16, o->profile) << 16;
    if (h > 0x1000000) {
        o->render_flags &= 0x7fffffff;
        return;
    }
    o->y += h;
    o->scale = o->unknown_3c = p->scale - (h >> 17);
    o->speed = p->speed - 0xff;
    o->step = 0x40;
}

void eng1_native_code_target_00f180(Actor *o)
{
    Actor *p = o->attachment;
    long h, lim, d;

    if (~p->flags & 8) {
        o->y = p->y;
        o->x = p->x;
    }
    o->y -= 0x20000;
    o->z = p->z;
    o->render_flags &= ~0x10;
    o->render_flags |= p->render_flags & 0x10;
    o->render_flags |= 0x80000000;
    o->category = p->category;
    o->sprite = p->sprite;
    o->unknown_24 = p->unknown_24;
    o->unknown_4a = p->unknown_4a;
    p->render_flags &= 0xfff8ffff;
    o->render_flags &= 0xfff8fffe;
    h = (unsigned short)grid_scan_20b50(o->x >> 16, o->y >> 16, o->z >> 16, o->profile);
    lim = (unsigned short)eng1_299d0(o);
    if (h > lim)
        h = lim;
    h <<= 16;
    if (h > 0x1000000) {
        o->render_flags &= 0x7fffffff;
        return;
    }
    o->y += h;
    o->scale = o->unknown_3c = p->scale - (h >> 17);
    o->speed = p->speed - 0xff;
    o->step = 0x40;
    if (g_eng1_setup_word_718e0) {
        if (o->y > g_eng1_setup_word_718e0 << 16) {
            o->speed = p->speed + 0xff;
            o->step = g_pipeline_dword_716a8;
            o->unknown_3c += g_pipeline_dword_71718 + 0x20;
            o->render_flags |= 1;
            o->z = p->z - 0x40000;
            d = (p->y >> 16) - g_eng1_setup_word_718e0;
            if (d > 0) {
                p->unknown_26 = d;
                o->render_flags |= 0x50000;
                p->render_flags |= 0x30000;
                o->unknown_26 = p->unknown_26 - 1;
                o->y = (g_eng1_setup_word_718e0 + o->unknown_26) << 16;
                return;
            }
            o->y = g_eng1_setup_word_718e0 << 16;
            return;
        }
        o->unknown_4a = 0;
        o->speed = p->speed - 0xff;
        o->unknown_26 = 0;
        p->unknown_26 = 0;
        return;
    }
    o->render_flags |= p->render_flags & 0x10 | 0x80000000;
    o->render_flags |= p->render_flags & 0x8000000;
    o->render_flags |= 0x4000000;
    o->unknown_4a = 0;
}
