/* ENG1.DLL 0x30010..0x32300: actor spawn services and the primitive/renderer unit shared with TITLE
 * (lane x4 scratch, 2026-10-20). Adapted from calibration/title/r17ad0.c (TITLE 0x17ad0..0x17cb0) and
 * calibration/title/u17cb0.cpp (TITLE 0x17cb0..0x197f0). Per-DLL differences: the ENG1 display buffer
 * (ordering table at +0x88, objects linked at ot[0x4bf - depth]), the 12-byte cell table, the
 * ENG1-only spawn/kind helpers, the extent producers and the camera-relative transform.
 * Globals are named by ENG1 RVA or by their existing project names. Unit data (0x5ce50 matrices, the 0x5cec0
 * handler table, the 81-row 0x5cee0 slot table and its P:\SRC1\... literals) is left extern here.
 * Compiled as one C++ unit (0x30290/0x30380 need the /TP base/index order). <string.h> is a TU-context
 * (declaration-count) hypothesis only: without it 0x304f0 and 0x30d00 take the TITLE/ENG3 register
 * allocation; it is not evidence of a real include. */
#include <string.h>
#include "../../calibration/title/title_engine.h"

extern "C" {

/* ENG1 display buffer (as calibration/eng1/sprite_unit_03cd0.c). */
typedef struct TitleRect {
    short x, y, w, h;
} TitleRect;
typedef struct TitleDrawEnv {
    TitleRect clip;
    short ofs[2];
    TitleRect tw;
    unsigned short tpage;
    unsigned char dtd;
    unsigned char dfe;
    unsigned char isbg;
    unsigned char r0, g0, b0;
    unsigned long dr_env[16];
} TitleDrawEnv;
typedef struct TitleDispEnv {
    TitleRect disp;
    TitleRect screen;
    unsigned char isinter;
    unsigned char isrgb24;
    unsigned char pad0, pad1;
} TitleDispEnv;
#define ENG1_OT_SIZE 0x500
typedef struct Eng1Buffer {
    TitleDrawEnv draw;                /* 0x0000 */
    TitleDispEnv disp;                /* 0x005c */
    unsigned char unknown_070[0x18];  /* 0x0070 */
    unsigned long ot[ENG1_OT_SIZE];   /* 0x0088 ordering table */
} Eng1Buffer;
extern Eng1Buffer *g_dispatch_frame_current_71ec4;
extern Eng1Buffer *g_dispatch_frame_base_71ec8;

typedef struct TitleObj TitleObj;

/* ---- spawn services (TITLE r17ad0.c) ---- */

typedef struct TitleObject {
    unsigned long unknown_000;
    unsigned long unknown_004;
    unsigned char unknown_008[0x00c - 0x008];
    unsigned char unknown_00c[0x01f - 0x00c];
    unsigned char unknown_01f;
    unsigned char unknown_020[0x022 - 0x020];
    unsigned char unknown_022;
    unsigned char unknown_023;
    unsigned char unknown_024[0x02e - 0x024];
    unsigned short unknown_02e;
    unsigned char unknown_030[0x034 - 0x030];
    unsigned short unknown_034;
    unsigned short unknown_036;
    unsigned short unknown_038;
    unsigned char unknown_03a[0x03e - 0x03a];
    unsigned short unknown_03e;
    unsigned char unknown_040[0x04a - 0x040];
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x054 - 0x04c];
    unsigned long unknown_054;
    unsigned char unknown_058[0x064 - 0x058];
    void *unknown_064;
    unsigned char unknown_068[0x070 - 0x068];
    unsigned short unknown_070;
    unsigned short unknown_072;
    unsigned short unknown_074;
    unsigned char unknown_076[0x094 - 0x076];
    unsigned char unknown_094;
    unsigned char unknown_095;
    unsigned char unknown_096;
    unsigned char unknown_097[0x120 - 0x097];
    int *unknown_120;
} TitleObject;
typedef struct TitleVtable8 {
    void (*fn)(void *obj);
    unsigned long unknown_004;
} TitleVtable8;
typedef struct TitleVtable12 {
    void (*fn)(void *obj);
    unsigned char unknown_004[0x0c - 0x04];
} TitleVtable12;
typedef struct KindRecord {           /* 8-byte kind record at g_configuration_dword_71390 */
    void *image_00;
    unsigned char unknown_04[2];
    unsigned char group_06;
    unsigned char unknown_07;
} KindRecord;
typedef struct SpawnPoint {           /* 12-byte spawn point at 0x71ee0 */
    short x, y, z;
    unsigned char unknown_06[6];
} SpawnPoint;

extern TitleVtable12 *g_configuration_dword_71370;
extern TitleVtable8 *g_configuration_dword_7137c;
extern TitleVtable8 *g_configuration_dword_7139c;
extern KindRecord *g_configuration_dword_71390;
extern unsigned short *g_configuration_dword_713bc;
extern TitleObject *g_native_pointer_7217c;
extern SpawnPoint g_71ee0[];
extern unsigned short g_51ac0[][4];
unsigned long dispatch_local_2a170(long x, long y, long z, int step);
unsigned long grid_scan_20b50(long x, long y, long z, int group);
TitleObject *spawn_normal(int a0, int a1, int a2, unsigned short size);
TitleObject *spawn_middle_277e0(int a0, int a1, int a2, unsigned short size);
TitleObject *spawn_kind(int a0, int a1, int a2, unsigned short size);
void append_chain(TitleObject *owner, TitleObject *child);
void reset_frame_state(TitleObject *p);
void eng1_interpret_word_stream(TitleObject *p);
void remove_actor_variant(TitleObject *p);
int eng1_320d0(TitleObj *o);
int eng1_31dc0(char *obj, int *vec);
void eng1_05310(void *p);
void eng1_03cd0(void *p);
void dispatch_engine_2c920(unsigned short *a, unsigned char *b);
void dispatch_engine_2c700(void *a);
void dispatch_engine_2c6c0(void);

unsigned int dispatch_local_30010(long x, long y, long z, unsigned short mask)
{
    unsigned short w;

    w = g_configuration_dword_713bc[dispatch_local_2a170(x, y, z, 0)];
    if (w & 0x8000) {
        return 0;
    }
    return w & mask;
}

int eng1_30050(long x, long y, long z)
{
    unsigned long g;
    unsigned short w;

    w = g_configuration_dword_713bc[dispatch_local_2a170(x, y, z, 0)];
    if (w & 0x100) {
        g = grid_scan_20b50(x >> 16, y >> 16, z >> 16, 0);
        if ((unsigned short)g > 0x100 && (g & 0xffff0000) < 0x1000000) {
            return 1;
        }
    }
    return 0;
}

TitleObject *spawn_at_group_300c0(int a0, int a1, int a2, unsigned short size, int a4)
{
    TitleObject *obj;

    if (size & 0x6000) {
        if (size & 0x4000) {
            obj = spawn_middle_277e0(a0, a1, a2, size);
            if (obj != 0) {
                g_configuration_dword_7137c[size & 0xfff].fn(obj);
            }
        } else {
            obj = spawn_kind(a0, a1, a2, size);
            if (obj != 0) {
                g_configuration_dword_7139c[size & 0xfff].fn(obj);
            }
        }
    } else {
        obj = spawn_normal(a0, a1, a2, size);
        if (obj != 0) {
            g_configuration_dword_71370[size & 0xfff].fn(obj);
        }
    }
    return obj;
}

void spawn_actor_with_owner(TitleObject *owner, int a0, int a1, int a2, unsigned short size)
{
    TitleObject *obj;

    if (size & 0x6000) {
        if (size & 0x4000) {
            obj = spawn_middle_277e0(a0, a1, a2, size);
            if (obj != 0) {
                obj->unknown_094 = owner->unknown_094;
                obj->unknown_095 = owner->unknown_095;
                obj->unknown_096 = owner->unknown_096;
                g_configuration_dword_7137c[size & 0xfff].fn(obj);
            }
        } else {
            obj = spawn_kind(a0, a1, a2, size);
            if (obj != 0) {
                g_configuration_dword_7139c[size & 0xfff].fn(obj);
            }
        }
    } else {
        obj = spawn_normal(a0, a1, a2, size);
        if (obj != 0) {
            obj->unknown_094 = owner->unknown_094;
            obj->unknown_095 = owner->unknown_095;
            obj->unknown_096 = owner->unknown_096;
            append_chain(owner, obj);
            g_configuration_dword_71370[size & 0xfff].fn(obj);
        }
    }
}

void eng1_30290(TitleObject *obj, unsigned short event)
{
    unsigned short kind;
    SpawnPoint *pt;

    if ((unsigned int)obj < 100) {
        if ((unsigned int)obj < 0x80) {
            kind = g_51ac0[1][event];
            if (kind != 0) {
                spawn_at_group_300c0((g_71ee0[(int)obj].x + 16) << 16, (g_71ee0[(int)obj].y + 16) << 16,
                                     (g_71ee0[(int)obj].z + 16) << 16, kind, g_native_pointer_7217c->unknown_038);
            }
        }
    } else {
        kind = g_51ac0[obj->unknown_02e & 0xf][event];
        if (kind != 0) {
            obj->unknown_022 = g_configuration_dword_71390[kind & 0xfff].group_06;
            obj->unknown_064 = g_configuration_dword_71390[kind & 0xfff].image_00;
            obj->unknown_02e = (kind & 0xfff) | 0x4000;
            reset_frame_state(obj);
            eng1_interpret_word_stream(obj);
        } else {
            remove_actor_variant(obj);
        }
    }
}

void eng1_30380(TitleObject *obj, int kind)
{
    obj->unknown_022 = g_configuration_dword_71390[kind & 0xfff].group_06;
    obj->unknown_064 = g_configuration_dword_71390[kind & 0xfff].image_00;
    obj->unknown_02e = (kind & 0xfff) | 0x4000;
    reset_frame_state(obj);
    eng1_interpret_word_stream(obj);
}

void eng1_303d0(TitleObject *obj)
{
    int *list;
    int count;
    TitleObject *item;

    if (eng1_320d0((TitleObj *)obj) == -1) {
        obj->unknown_01f |= 8;
        return;
    }
    list = obj->unknown_120;
    count = *list++;
    while (count--) {
        item = (TitleObject *)*list++;
        eng1_05310(item);
        eng1_320d0((TitleObj *)item);
        eng1_03cd0(item);
    }
}

void eng1_30430(TitleObject *obj)
{
    unsigned short buf[4];
    unsigned char out[0x20];
    int r;
    int *list;
    int count;
    TitleObject *item;

    r = eng1_320d0((TitleObj *)obj);
    if (r == -1) {
        obj->unknown_01f |= 8;
        return;
    }
    list = obj->unknown_120;
    count = *list;
    list++;
    buf[0] = obj->unknown_070 << 2;
    buf[1] = (obj->unknown_072 << 2) + r / 2;
    buf[2] = obj->unknown_074 << 2;
    dispatch_engine_2c920(buf, out);
    dispatch_engine_2c700(out);
    dispatch_engine_2c6c0();

    while (count--) {
        item = (TitleObject *)*list++;
        if (item != 0) {
            if (eng1_31dc0((char *)&item->unknown_00c, (int *)item) != 0) {
                eng1_05310(item);
                eng1_03cd0(item);
            }
        }
    }
}

/* ---- primitive / renderer unit (TITLE u17cb0.cpp) ---- */

typedef struct TitleRecord {      /* resource slot: file path and entry count */
    char *path_00;
    unsigned short count_04;
    unsigned char unknown_06[6];
    unsigned char *table_0c;
    unsigned short type_10;
    unsigned short state_12;
} TitleRecord;
typedef struct TitleStream {
    unsigned char unknown_000[0x5c];
    unsigned char *stream_05c;
} TitleStream;
typedef struct TitlePair8 {
    unsigned int a_00;
    unsigned int b_04;
} TitlePair8;
typedef struct TitleSVec {        /* PSX SVECTOR */
    short vx;
    short vy;
    short vz;
    short pad;
} TitleSVec;
typedef struct TitleXY {
    int x;
    int y;
} TitleXY;
struct TitleObj {
    TitleXY pos_000;
    int z_008;
    unsigned short w_0c;
    unsigned short w_0e;
    short w_10;
    unsigned char unknown_012[0x14 - 0x12];
    short w_014;
    short w_016;
    short w_018;
    unsigned char unknown_01a[0x1e - 0x1a];
    unsigned char b_01e;
    unsigned char flags_01f;
    unsigned char byte_020;
    unsigned char b_021;
    unsigned char b_022;
    unsigned char byte_023;
    unsigned char b_024;
    unsigned char unknown_025[0x26 - 0x25];
    short w_026;
    short w_028;
    unsigned char unknown_02a[0x2c - 0x2a];
    unsigned short w_02c;
    unsigned char unknown_02e[0x34 - 0x2e];
    unsigned short w_034;
    unsigned short w_036;
    unsigned char unknown_038[0x3a - 0x38];
    unsigned short w_03a;
    unsigned short w_03c;
    unsigned short w_03e;
    unsigned char unknown_040[0x46 - 0x40];
    unsigned short w_046;
    unsigned char unknown_048[0x4a - 0x48];
    unsigned short w_04a;
    int dword_04c;
    unsigned char unknown_050[0x54 - 0x50];
    unsigned int flags_054;
    unsigned char unknown_058[0x5c - 0x58];
    unsigned char *stream_05c;
    unsigned char unknown_060[0x68 - 0x60];
    struct TitleObj *next_068;
    unsigned char unknown_06c[0x70 - 0x6c];
    TitleSVec rot_070;
    int trans_078[3];
    unsigned char unknown_084[0x120 - 0x84];
    unsigned int *list_120;
};
typedef struct TitleRec {
    unsigned char unknown_000[0x0c];
    unsigned char unknown_00c[0x10 - 0x0c];
    short unknown_010;
    unsigned char unknown_012[0x76 - 0x12];
    unsigned short unknown_076;
    unsigned char unknown_078[0x80 - 0x78];
    int unknown_080;
} TitleRec;
typedef struct TitleShape {
    unsigned char unknown_000[0x0c];
    short unknown_00c;
    short unknown_00e;
    short unknown_010;
    unsigned char unknown_012[0x28 - 0x12];
    short unknown_028;
    unsigned char unknown_02a[0x3a - 0x2a];
    unsigned short unknown_03a;
    unsigned short unknown_03c;
    unsigned char unknown_03e[0x54 - 0x3e];
    unsigned long unknown_054;
} TitleShape;

extern int g_5ce50[8];
extern void *g_5cec0[8];         /* handler table */
extern TitleRecord g_5cee0[];
int ensure_actor_resource_ready(int index, int key);
extern TitlePair8 g_70840[];
extern TitlePair8 *g_70c40;
extern signed char g_header_byte_70ecd;
extern char *g_header_pointer_70d2c;
int eng1_313e0(TitleObj *o);
void eng1_31470(TitleObj *o);
void eng1_32130(TitleObj *o);
void eng1_32810(TitleObj *o, int v);
void dispatch_engine_2c6e0(void);
void dispatch_engine_2c760(void *e);
void dispatch_engine_2c7a0(void *b);
void dispatch_engine_2c720(void *p);
unsigned long eng1_311a0(TitleObj *p);
int eng1_31540(TitleObj *o, unsigned char *q, unsigned long c);
int eng1_32050(TitleShape *obj, int k);
extern int g_camera_focal_70ce4;
void dispatch_engine_2c690(void *p);
void dispatch_engine_2c730(void *p);
void dispatch_engine_2c7b0(void *p);

typedef struct TitleSpanXY {
    int x;
    int y;
} TitleSpanXY;
typedef struct TitleSpan {         /* 16-byte child record of a group object */
    TitleSpanXY pos_000;
    int z_008;
    unsigned short id_00c;
    unsigned short flags_00e;
} TitleSpan;
typedef struct TitleTile {         /* 12-byte single-pixel tile primitive (code 0x68) */
    unsigned long *next_000;
    unsigned char r0, g0, b0, code;
    unsigned long xy_008;
} TitleTile;

extern TitleObj *g_734e0;
extern TitleTile *g_frame_global_73484;
void eng1_03bd0(TitleObj *o);
int eng1_31e30(char *obj, int *vec, int k);

void eng1_304f0(TitleObj *obj)
{
    unsigned int flags;
    int z;
    int xy[2];
    TitleSpan *span;
    int acc;
    unsigned long *ot;
    int count;
    int r;
    unsigned short id;
    unsigned short *e;

    flags = obj->flags_054;
    if (g_5cee0[obj->b_022].type_10 != 3) {
        if (ensure_actor_resource_ready(obj->b_022, obj->w_034) == 0) {
            return;
        }
    }
    r = eng1_320d0(obj);
    if (r == -1) {
        obj->flags_01f |= 8;
        return;
    }
    eng1_32810(obj, r);
    dispatch_engine_2c6c0();
    span = (TitleSpan *)obj->stream_05c - 1;
    count = obj->w_036;
    g_734e0->byte_023 = 0;
    g_734e0->w_02c = obj->w_02c;
    acc = 0;
    g_734e0->b_022 = obj->b_022;
    g_734e0->w_04a = obj->w_04a;
    if (flags & 0x100000) {
        g_734e0->rot_070 = obj->rot_070;
    } else {
        g_734e0->rot_070.vx = g_734e0->rot_070.vy = g_734e0->rot_070.vz = 0;
    }
    z = obj->rot_070.vz;
    ot = &g_dispatch_frame_current_71ec4->ot[ENG1_OT_SIZE - 0x41 - obj->w_10];
    while (count--) {
        span++;
        if (span->flags_00e & 0x8000) {
            eng1_31dc0((char *)xy, (int *)span);
            g_frame_global_73484->xy_008 = xy[0];
            *(unsigned long *)&g_frame_global_73484->r0 = 0xffffff;
            g_frame_global_73484->code = 0x68;
            g_frame_global_73484->next_000 = (unsigned long *)*ot;
            *ot = (unsigned long)g_frame_global_73484;
            g_frame_global_73484++;
        } else if (span->id_00c != 0) {
            if (flags & 0x200000) {
                if (flags & 0x400000) {
                    dispatch_engine_2c6c0();
                    acc += span->z_008;
                } else {
                    acc = span->z_008;
                }
                g_734e0->rot_070.vz = z - acc;
                eng1_31e30((char *)&g_734e0->w_0c, (int *)span, acc);
            } else {
                if (flags & 0x400000) {
                    dispatch_engine_2c6c0();
                }
                eng1_31dc0((char *)&g_734e0->w_0c, (int *)span);
            }
            id = span->id_00c;
            e = (unsigned short *)g_5cee0[g_734e0->b_022].table_0c + (short)(id * 2);
            if (e[0] != 0) {
                g_734e0->w_034 = id;
                g_734e0->flags_054 = (obj->flags_054 & 0xffff0000) | span->flags_00e;
                if ((g_734e0->flags_054 & 0x600) == 0x600) {
                    g_734e0->pos_000.x = obj->pos_000.x + span->pos_000.x;
                    g_734e0->pos_000.y = span->pos_000.y + obj->pos_000.y;
                }
                g_734e0->w_046 = e[1];
                eng1_03bd0(g_734e0);
                eng1_03cd0(g_734e0);
            }
        }
    }
}

typedef struct TitleCell {
    unsigned short mask_00;
    unsigned short w_02;
    unsigned short next_04[4];
} TitleCell;

typedef struct TitleVtx {
    short x;
    short y;
    short z;
    short pad;
} TitleVtx;

typedef struct TitleFrameBox {
    unsigned short ext_00;
    unsigned short ext_02;
    signed char dx_04;
    signed char dy_05;
} TitleFrameBox;

typedef struct TitlePrim {
    struct TitlePrim *next_000;
    unsigned long tex_004;
    unsigned long xy_008;
    union {
        unsigned long packed;
        struct { unsigned short uv; unsigned short clut; } words;
    } uv_00c;
    unsigned long xy_010;
    union {
        unsigned long packed;
        struct { unsigned short uv; unsigned short tpage; } words;
    } uv_014;
    unsigned long xy_018;
    union {
        unsigned long packed;
        struct { unsigned short uv; unsigned short pad; } words;
    } uv_01c;
    unsigned long xy_020;
    union {
        unsigned long packed;
        struct { unsigned short uv; unsigned short pad; } words;
    } uv_024;
} TitlePrim;

typedef struct TitleVertex {
    unsigned long xy;
    long z;
} TitleVertex;
typedef void (*TitleHandler)(TitleObj *o, unsigned char *p, int n);

extern TitleCell g_72880[];
extern signed char g_5e128[];
extern int g_72868;
extern unsigned int *g_70d28;
extern unsigned char *g_70d30;
extern int g_6ffbc;
extern int *g_70c44;
void eng1_31d30(TitleObj *o, int k);

void dispatch_engine_2c6a0(void *a, void *b, void *c);
void dispatch_engine_2c6f0(void);
void dispatch_engine_2c770(TitlePrim *p);
void dispatch_engine_2c670(unsigned long a, unsigned long b, unsigned long c);
void dispatch_engine_2c6d0(void);
void dispatch_engine_2c740(int *p);
void dispatch_engine_2c780(TitlePrim *p);
void eng1_058e0(TitleObj *o, unsigned long *ot);
void eng1_059f0(TitleObj *o, unsigned long *ot);

void eng1_30790(unsigned char *stream, TitleObj *obj, unsigned char count)
{
    unsigned char code;
    unsigned char more;
    unsigned short w;
    unsigned char u0;
    unsigned short flip;
    unsigned long flags;
    int b;
    unsigned int row;
    unsigned long *ot;
    unsigned short clut;
    TitleVtx v0;
    TitleVtx v1;
    TitleVtx v2;
    int a;
    int h;
    int t;
    int c;
    unsigned int dw;
    int u;
    int vv;
    int x;
    int x2;
    int y;
    unsigned char u1;
    unsigned int off;
    TitlePrim *p;

    dispatch_engine_2c700(g_5ce50);
    dispatch_engine_2c730(g_734e0->trans_078);
    g_734e0->trans_078[1] += obj->w_028 * 2;
    dispatch_engine_2c720(g_734e0->trans_078);
    v0.z = 0;
    v1.z = 0;
    v2.z = 0;
    a = (signed char)*stream++;
    b = (signed char)*stream++;
    clut = ((obj->b_01e + 0xe0) << 6) | 0x30;
    flags = obj->flags_054;
    code = *stream++;
    w = obj->w_046;
    ot = &g_dispatch_frame_current_71ec4->ot[ENG1_OT_SIZE - 0x41 - obj->w_10];
    p = (TitlePrim *)g_frame_global_73484;
    more = code & 0x80;
    flip = flags & 0x10;
    do {
        row = w >> 4;
        t = g_5e128[w & 15];
        vv = ((t >> 1) + row * 2) << 4;
        u = ((t & 1) + (w >> 8) * 2) << 4;
        if (more) {
            u += (code & 15) * 8;
            vv += (code >> 1) & 0x38;
        }
        c = (signed char)*stream;
        dw = (c & 15) * 8;
        h = (c >> 1) & ~7;
        stream++;
        u0 = (unsigned char)u;
        p->uv_00c.words.uv = u0 + (vv << 8);
        u1 = (unsigned char)(u + dw - 1);
        p->uv_014.words.uv = u1 + (vv << 8);
        p->uv_01c.words.uv = u0 + ((vv + h + 0xff) << 8);
        p->uv_024.words.uv = ((vv + h + 0xff) << 8) + u1;
        if (flip) {
            x = -a - (signed char)*stream++ * 2;
            x2 = x - dw;
        } else {
            x = a + (signed char)*stream++ * 2;
            x2 = x + dw;
        }
        y = b + (signed char)*stream++ * 2;
        v0.x = x;
        v2.x = x;
        v1.y = y;
        v0.y = y + h;
        v2.y = y;
        v1.x = x2;
        dispatch_engine_2c6a0(&v2, &v1, &v0);
        dispatch_engine_2c6f0();
        off = row * sizeof(TitleCell);
        p->uv_014.words.tpage = ((TitleCell *)((char *)g_72880 + off))->w_02;
        p->tex_004 = 0x2e000000;
        dispatch_engine_2c770(p);
        v0.x = v1.x;
        dispatch_engine_2c690(&v0);
        dispatch_engine_2c6e0();
        p->next_000 = (TitlePrim *)*ot;
        *ot = (unsigned long)p;
        p->uv_00c.words.clut = clut;
        code = *stream;
        more = code & 0x80;
        stream++;
        if (!more) {
            w = ((TitleCell *)((char *)g_72880 + off))->next_04[g_5e128[w & 15]];
        }
        dispatch_engine_2c760(&p->xy_020);
        p++;
    } while (--count);
    g_frame_global_73484 = (TitleTile *)p;
}

void eng1_30a20(unsigned char *stream, TitleObj *obj, unsigned char count)
{
    unsigned char code;
    unsigned char more;
    unsigned short w;
    unsigned char u0;
    int flip;
    unsigned long flags;
    unsigned long sel;
    unsigned long colour;
    int zoff;
    unsigned long blend;
    int b;
    unsigned int row;
    unsigned long *ot;
    unsigned short clut;
    TitleVtx v0;
    TitleVtx v1;
    TitleVtx v2;
    int a;
    int h;
    int t;
    int c;
    unsigned int dw;
    int u;
    int vv;
    int x;
    int x2;
    int y;
    unsigned char u1;
    unsigned int off;
    TitlePrim *p;

    flags = obj->flags_054;
    if (flags & 0x2000) {
        obj->w_10 = obj->w_03e;
    }
    ot = &g_dispatch_frame_current_71ec4->ot[ENG1_OT_SIZE - 0x41 - obj->w_10];
    sel = flags & 0x10000;
    if (sel) {
        eng1_058e0(obj, ot);
    }
    if (g_72868 != 0) {
        eng1_32810(obj, 0);
        dispatch_engine_2c6c0();
    }
    colour = eng1_311a0(obj);
    zoff = 0;
    if (flags & 0x40000) {
        zoff = obj->w_026;
    }
    v0.z = 0;
    v1.z = 0;
    v2.z = 0;
    a = (signed char)*stream++;
    b = (signed char)*stream++;
    blend = (flags & 3) << 5;
    code = *stream++;
    clut = ((obj->b_01e + 0xe0) << 6) | 0x30;
    w = obj->w_046;
    p = (TitlePrim *)g_frame_global_73484;
    more = code & 0x80;
    flip = flags & 0x10;
    do {
        row = w >> 4;
        t = g_5e128[w & 15];
        vv = ((t >> 1) + row * 2) << 4;
        u = ((t & 1) + (w >> 8) * 2) << 4;
        if (more) {
            u += (code & 15) * 8;
            vv += (code >> 1) & 0x38;
        }
        c = (signed char)*stream;
        dw = (c & 15) * 8;
        h = (c >> 1) & ~7;
        stream++;
        u0 = (unsigned char)u;
        p->uv_00c.words.uv = u0 + (vv << 8);
        u1 = (unsigned char)(u + dw - 1);
        p->uv_014.words.uv = u1 + (vv << 8);
        p->uv_01c.words.uv = u0 + ((vv + h + 0xff) << 8);
        p->uv_024.words.uv = ((vv + h + 0xff) << 8) + u1;
        if ((unsigned short)flip) {
            x = -a - (signed char)*stream++ * 2;
            x2 = x - dw;
        } else {
            x = a + (signed char)*stream++ * 2;
            x2 = x + dw;
        }
        y = b + (signed char)*stream++ * 2 + zoff;
        v0.y = y + h;
        v0.x = x;
        v2.x = x;
        v1.y = y;
        v2.y = y;
        v1.x = x2;
        dispatch_engine_2c6a0(&v2, &v1, &v0);
        dispatch_engine_2c6f0();
        off = row * sizeof(TitleCell);
        p->uv_014.words.tpage = ((TitleCell *)((char *)g_72880 + off))->w_02 | blend;
        p->tex_004 = colour;
        dispatch_engine_2c770(p);
        v0.x = v1.x;
        dispatch_engine_2c690(&v0);
        dispatch_engine_2c6e0();
        p->next_000 = (TitlePrim *)*ot;
        *ot = (unsigned long)p;
        p->uv_00c.words.clut = clut;
        more = *stream;
        code = more;
        more &= 0x80;
        stream++;
        if (!more) {
            w = ((TitleCell *)((char *)g_72880 + off))->next_04[g_5e128[w & 15]];
        }
        dispatch_engine_2c760(&p->xy_020);
        p++;
    } while (--count);
    g_frame_global_73484 = (TitleTile *)p;
    if (sel) {
        eng1_059f0(obj, ot);
    }
}

int eng1_30d00(TitleObj *o, TitleFrameBox *box, int pass)
{
    int flip;
    int x;
    int y;
    int w;
    short right, left, bottom, top;
    int scale;
    int sx;
    short r, l, b, tp;
    short tmp;

    x = (short)o->w_0c;
    y = (short)o->w_0e;
    flip = o->flags_054 & 0x10;
    if (flip) {
        x -= box->dx_04;
    } else {
        x += box->dx_04;
    }
    y += box->dy_05;
    w = box->ext_00;
    right = (signed char)w;
    left = (signed char)(w >> 8);
    w = box->ext_02;
    bottom = (signed char)w;
    top = (signed char)(w >> 8);
    scale = (o->w_03a * (g_camera_focal_70ce4 >> 1)) / o->w_10;
    sx = scale;
    if (pass == 1) {
        sx = scale * 2;
        y += (o->w_028 * scale) >> 8;
    }
    r = (short)((sx * right) >> 7);
    l = (short)((sx * left) >> 7);
    b = (short)((scale * bottom) >> 7);
    tp = (short)((scale * top) >> 7);
    if (flip) {
        tmp = -r;
        r = -l;
        l = tmp;
    }
    if (x + l < 0) {
        return 1;
    }
    if (y + tp < 0) {
        return 1;
    }
    if (x + r > 0x140) {
        return 1;
    }
    return y + b > 0xf0;
}

struct title_pos {
    short x;
    short y;
};
unsigned char *resource_metadata_pointer(int index, int key);
int profile_extent_31ec0(TitleObj *o, short *out, int mode);

int extent_producer_30e40(TitleObj *o, short *out, int mode)
{
    unsigned char *p;
    signed char n;
    short x0, y0, z0, dx, dy, dz;

    if (o->w_034 == 0) {
        return 0;
    }
    if (o->b_022 == 0) {
        return profile_extent_31ec0(o, out, mode);
    }
    if (o->w_034 > g_5cee0[o->b_022].count_04) {
        return 0;
    }
    p = resource_metadata_pointer(o->b_022, o->w_034);
    if (p == 0) {
        return 0;
    }
    n = *++p;
    if (mode == 0) {
        if (n == 0) {
            return 0;
        }
    } else {
        if (mode > n) {
            return 0;
        }
        p += mode * 6 - 6;
    }
    p++;
    if (((short *)p)[0] == 0 && ((short *)p)[1] == 0 && ((short *)p)[2] == 0) {
        return 0;
    }
    x0 = (signed char)*p++;
    y0 = (signed char)*p++;
    z0 = (signed char)*p++ >> 1;
    dx = (signed char)*p++ - x0;
    dy = (signed char)*p++ - y0;
    dz = ((signed char)*p >> 1) - z0;
    if (o->flags_054 & 0x80) {
        if (o->w_03a != 0x100) {
            x0 = (x0 * o->w_03a) >> 8;
            dx = (dx * o->w_03a) >> 8;
            z0 = (z0 * o->w_03a) >> 8;
            dz = (dz * o->w_03a) >> 8;
        }
        if (o->w_03c != 0x100) {
            y0 = (y0 * o->w_03a) >> 8;
            dy = (dy * o->w_03a) >> 8;
        }
    } else if (o->w_03a != 0x100) {
        x0 = (x0 * o->w_03a) >> 8;
        y0 = (y0 * o->w_03a) >> 8;
        z0 = (z0 * o->w_03a) >> 8;
        dx = (dx * o->w_03a) >> 8;
        dy = (dy * o->w_03a) >> 8;
        dz = (dz * o->w_03a) >> 8;
    }
    out[0] = x0;
    out[1] = y0;
    out[2] = z0;
    out[3] = dx;
    out[4] = dy;
    out[5] = dz;
    return n;
}

int eng1_31000(TitleObj *o, struct title_pos *pos)
{
    unsigned char *p;
    int n;

    pos->y = 0;
    pos->x = 0;
    if (o->w_034 > g_5cee0[o->b_022].count_04) {
        return 0;
    }
    p = resource_metadata_pointer(o->b_022, o->w_034);
    if (p == 0) {
        return 0;
    }
    n = (signed char)*++p;
    p += n * 6 + 5;
    pos->x = (signed char)p[0];
    pos->y = (signed char)p[1];
    if (o->flags_054 & 0x10) {
        pos->x = -pos->x;
    }
    return 1;
}

unsigned char *resource_metadata_pointer(int index, int key)
{
    TitleRecord *rec;
    int *entries;
    int value;

    if (key == 0) {
        return 0;
    }
    if (g_5cee0[index].type_10 != 3) {
        if (ensure_actor_resource_ready(index, key) == 0) {
            return 0;
        }
    }
    rec = &g_5cee0[index];
    entries = (int *)(rec->table_0c + rec->count_04 * 4 + 8);
    value = entries[(short)key];
    if (value == 0) {
        return 0;
    }
    return (unsigned char *)entries + (value >> 8);
}

extern unsigned char g_frame_setup_result_734ec;
void eng1_03f60(unsigned char *stream, TitleObj *p, unsigned char count);

void eng1_310e0(unsigned char *stream, TitleObj *p, unsigned char count)
{
    unsigned int flags;
    int a;
    int pal;

    flags = p->flags_054;
    if (flags & 0x100) {
        a = p->w_04a;
        pal = p->b_01e;
        p->w_04a = 0x80 - (0x80 - a) * (0x80 - a) / 128;
        p->flags_054 = (flags & ~2) | 1;
        eng1_03f60(stream, p, count);
        p->w_04a = a * a / 128;
        p->b_01e = g_frame_setup_result_734ec;
        p->flags_054 = (flags & ~1) | 2;
        eng1_03f60(stream, p, count);
        p->b_01e = pal;
        p->w_04a = a;
        p->flags_054 = flags;
    } else {
        eng1_03f60(stream, p, count);
    }
}

extern unsigned long g_6f4e8[8][64];
extern int *g_70d50;
extern int g_70cfc;
extern unsigned long g_70cf4;

unsigned long eng1_311a0(TitleObj *p)
{
    unsigned long flags;
    unsigned int w;
    unsigned long c;
    int v;
    short fx, fy;
    short lo, hi;
    int t0, t1;
    int base;
    unsigned int n;
    unsigned int r, g, b;

    flags = p->flags_054;
    if (flags & 0x600) {
        w = p->w_02c;
        if (w & 8) {
            c = g_6f4e8[w & 7][(w >> 6) & 0xff];
        } else {
            v = g_70d50[w >> 4];
            if ((flags & 0x600) == 0x600) {
                fx = (p->pos_000.x >> 16) & 0x3f;
                fy = 0x3f - ((p->pos_000.y >> 16) & 0x3f);
                lo = (unsigned char)(v >> 8);
                t0 = ((((v & 0xff) - lo) * fx) >> 6) + lo;
                hi = (unsigned char)(v >> 24);
                t1 = (((((v >> 16) & 0xff) - hi) * fx) >> 6) + hi;
                v = (((t1 - t0) * fy) >> 6) + t0;
            }
            base = g_70cfc & 0xff;
            v = (((v - base) * p->b_024) >> 10) + base;
            if (v < 0) {
                v = 0;
            } else if (v > 0x3f) {
                v = 0x3f;
            }
            c = g_6f4e8[w & 7][v];
        }
    } else {
        c = g_70cf4;
    }
    n = p->w_04a;
    if (n != 0x80) {
        r = (unsigned char)((unsigned char)(c >> 16) * n >> 7);
        g = (unsigned char)((unsigned char)(c >> 8) * n >> 7);
        b = (unsigned char)((unsigned char)c * n >> 7);
        c = (((r << 8) + g) << 8) + b;
    }
    c |= 0x2c000000;
    if (flags & 4) {
        c |= 0x2000000;
    }
    return c;
}

void eng1_31330(TitleObj *arg)
{
    TitleObj *cur;
    TitleObj *first;
    TitleObj *o;
    unsigned int *list;
    unsigned int count;
    unsigned int k;

    first = arg;
    if (arg != (TitleObj *)-1) {
        do {
            eng1_32130(arg);
            if (arg->flags_054 & 0x20000000) {
                list = arg->list_120;
                count = *list;
                list++;
                if (count-- != 0) {
                    k = count + 1;
                    do {
                        eng1_32130((TitleObj *)*list++);
                    } while (--k);
                }
            }
            arg = arg->next_068;
        } while (arg != (TitleObj *)-1);
    }
    o = first;
    g_70c40 = g_70840;
    if (first != (TitleObj *)-1) {
        do {
            if (o->flags_054 & 0x20000000) {
                eng1_31470(o);
            } else {
                o->flags_01f |= 8;
                if (o->flags_01f) {
                    o->byte_020 = (unsigned char)(g_70c40 - g_70840);
                    eng1_313e0(o);
                }
            }
            o = o->next_068;
        } while (o != (TitleObj *)-1);
    }
}

int eng1_313e0(TitleObj *o)
{
    unsigned char *q;
    int idx;

    if ((unsigned int)(o->w_10 - 1) > 0x43f) {
        return 0;
    }
    if ((unsigned int)o->w_034 > (unsigned int)g_header_byte_70ecd) {
        return 0;
    }
    o->b_022 = 0;
    idx = o->w_034;
    q = (unsigned char *)(g_header_pointer_70d2c + *(int *)(g_header_pointer_70d2c + idx * 4));
    q += idx * 4;
    if (o->w_0c > 0x140 || o->w_0e > 0xf0) {
        if (eng1_32050((TitleShape *)o, q[3]) != 0) {
            return 0;
        }
    }
    eng1_32810(o, o->rot_070.pad);
    eng1_31540(o, q, eng1_311a0(o));
    return 0;
}

void eng1_31470(TitleObj *o)
{
    unsigned char *q;
    unsigned int *list;
    unsigned int count;
    unsigned long c;
    TitleObj *p;
    int idx;
    int n;

    eng1_32810(o, o->rot_070.pad);
    dispatch_engine_2c720(o->trans_078);
    if (o->w_034 == 0) {
        return;
    }
    idx = o->w_034;
    q = (unsigned char *)(g_header_pointer_70d2c + *(int *)(g_header_pointer_70d2c + idx * 4));
    q += idx * 4 + 2;
    q += *q * 8 + 8;
    n = *(short *)q;
    q += 2;
    ((void (*)(TitleObj *, int *, int))g_5cec0[o->byte_023])(o, (int *)q, n);
    list = o->list_120;
    count = *list;
    list++;
    c = eng1_311a0(o);
    while (count--) {
        p = (TitleObj *)*list++;
        idx = p->w_034;
        q = (unsigned char *)(g_header_pointer_70d2c + *(int *)(g_header_pointer_70d2c + idx * 4));
        q += idx * 4;
        eng1_32810(p, p->rot_070.pad);
        eng1_31540(p, q, c);
    }
}

int eng1_31540(TitleObj *o, unsigned char *q, unsigned long c)
{
    TitleVertex *v1;
    short more;
    TitleVertex *v0;
    int lvl;
    int dbl;
    unsigned int u2;
    unsigned long xy1;
    unsigned short m;
    unsigned short h;
    unsigned short n;
    TitleVertex *v3;
    int k;
    unsigned int w;
    TitlePrim **ot;
    TitleVertex *v2;
    unsigned long col;
    int x;
    int clip;
    TitlePrim *prim;
    unsigned long xy3;
    int z;
    TitlePair8 *rec;
    unsigned char *uvt;
    unsigned int uv;
    char *vtx;
    unsigned char *b;
    unsigned int *t;
    TitleVertex *d;
    unsigned short i;
    int cnt;

    g_70c44 = 0;
    ot = (TitlePrim **)&g_dispatch_frame_current_71ec4->ot[ENG1_OT_SIZE - 0x41];
    lvl = o->w_10 >> 5;
    if (lvl < 1) {
        lvl = 1;
    } else if (lvl > 0x1d) {
        lvl = 0x1d;
    }
    rec = (TitlePair8 *)(g_70d28 + *(short *)q);
    q += 2;
    k = *q;
    q += 2;
    col = (lvl << 22) + 0x78300000;
    x = *(int *)q;
    q += k * 8 + 6;
    if (x != 0) {
        eng1_31d30(o, x);
    } else {
        dispatch_engine_2c720(o->trans_078);
    }
    uvt = g_70d30;
    o->flags_01f |= 2;
    h = o->byte_023;
    dbl = o->flags_054 & 0x40;
    vtx = (char *)g_engine_interface.data_010 + 0x18;
    prim = (TitlePrim *)g_frame_global_73484;
    if (g_engine_interface.context_004->unknown_040 != 0) {
        return 1;
    }
    do {
        cnt = *(short *)q;
        q += 2;
        ((TitleHandler)g_5cec0[h])(o, q, cnt);
        n = ((short *)rec)[0];
        more = ((short *)rec)[1];
        rec = (TitlePair8 *)((char *)rec - 4);
        do {
            rec++;
            w = rec->a_00;
            v0 = (TitleVertex *)(vtx + (w & 0x3f) * 8);
            w >>= 3;
            v1 = (TitleVertex *)(vtx + (w & 0x1f8));
            w >>= 6;
            v2 = (TitleVertex *)(vtx + (w & 0x1f8));
            w >>= 6;
            v3 = (TitleVertex *)(vtx + (w & 0x1f8));
            w >>= 9;
            if (w & 8) {
                if (w & 1) {
                    prim->xy_008 = v0->xy;
                    prim->xy_010 = v1->xy;
                    prim->xy_018 = v3->xy;
                    goto emit;
                }
                if ((w & 4) && g_6ffbc == 0) {
                    goto next;
                }
            }
            dispatch_engine_2c670(v0->xy, v1->xy, v3->xy);
            dispatch_engine_2c6d0();
            dispatch_engine_2c740(&clip);
            if (clip < 0) {
                if (!(w & 0x40)) {
                    goto next;
                }
                xy1 = v1->xy;
                xy3 = v3->xy;
                dispatch_engine_2c670(xy1, v2->xy, xy3);
                dispatch_engine_2c6d0();
                prim->xy_008 = v0->xy;
                prim->xy_010 = xy1;
                prim->xy_018 = xy3;
                dispatch_engine_2c740(&clip);
                if (clip < 0) {
                    goto next;
                }
            } else {
                dispatch_engine_2c780(prim);
            }
        emit:
            prim->xy_020 = v2->xy;
            z = v2->z;
            if (dbl) {
                z += v3->z;
                z += v0->z;
                z += v1->z;
                z >>= 4;
            } else {
                z += v0->z;
                z >>= 3;
            }
            if ((unsigned int)z > 0x4bf) {
                z = 0x4bf;
            }
            prim->next_000 = ot[-z];
            ot[-z] = prim;
            prim->tex_004 = c;
            uv = rec->b_04;
            if (uv & 0xff000000) {
                uv = (unsigned short)uv;
                uv |= 0xff00;
                prim->uv_00c.packed = uv | col;
                prim->uv_014.packed = uv | 0x9c0000;
                prim->uv_01c.words.uv = (unsigned short)uv;
                prim->uv_024.words.uv = (unsigned short)uv;
            } else {
                t = (unsigned int *)(uvt + uv);
                u2 = t[2];
                prim->uv_01c.words.uv = (unsigned short)u2;
                prim->uv_014.packed = t[1];
                prim->uv_00c.packed = col | t[0];
                prim->uv_024.words.uv = (unsigned short)(u2 >> 16);
            }
            prim++;
        next:
            ;
        } while (--n);
        if (more == 0) {
            break;
        }
        b = (unsigned char *)(rec + 1);
        q += cnt * 4 + 2;
        m = (unsigned short)more >> 8;
        for (i = 0; i != m; i++) {
                d = (TitleVertex *)(vtx + (*b & 0x3f) * 8);
                g_70c40->a_00 = d->xy;
                g_70c40->b_04 = d->z;
                g_70c40++;
                b++;
        }
        rec = (TitlePair8 *)(((int)b + 3) & ~3);
    } while ((char)more != 0);
    g_frame_global_73484 = (TitleTile *)prim;
    return 1;
}

void eng1_318d0(int a1, int *vals, int count)
{
    int pair[2];
    unsigned char *dst;
    int v;

    dst = (unsigned char *)g_engine_interface.data_010 + 0x10;
    v = *vals;
    vals++;
    pair[0] = v >> 8;
    pair[1] = (signed char)v;
    dispatch_engine_2c690(pair);
    dispatch_engine_2c6e0();
    v = *vals;
    vals++;
    pair[0] = v >> 8;
    pair[1] = (signed char)v;
    dst += 8;
    if (--count == 0) {
        goto tail;
    }
    do {
        dispatch_engine_2c760(dst);
        dispatch_engine_2c7a0(dst + 4);
        dispatch_engine_2c690(pair);
        dispatch_engine_2c6e0();
        v = *vals;
        vals++;
        pair[0] = v >> 8;
        pair[1] = (signed char)v;
        dst += 8;
    } while (--count);
tail:
    dispatch_engine_2c760(dst);
    dst += 4;
    dispatch_engine_2c7a0(dst);
}

void eng1_31990(TitleStream *s, int *vals, int n)
{
    TitlePair8 *dst = (TitlePair8 *)((char *)g_engine_interface.data_010 + 0x18);
    unsigned char *p = s->stream_05c;
    int pair[2];
    TitlePair8 *d0;
    TitlePair8 *d1;
    int v;
    n = *p++;
    do {
        d0 = dst + p[0];
        v = vals[p[0]];
        pair[0] = v >> 8;
        pair[1] = (signed char)v;
        dispatch_engine_2c690(pair);
        dispatch_engine_2c6e0();
        d1 = dst + p[1];
        d1->a_00 = d0->a_00;
        p += 2;
        d1->b_04 = d0->b_04;
        dispatch_engine_2c760(d0);
        dispatch_engine_2c7a0(&d0->b_04);
    } while (--n);
}

void eng1_31a20(TitleStream *s)
{
    TitlePair8 *dst = (TitlePair8 *)((char *)g_engine_interface.data_010 + 0x18);
    unsigned char *p = s->stream_05c;
    unsigned int n = *p++;
    do {
        unsigned int di = p[0];
        unsigned int src = p[1];
        TitlePair8 *d = dst + di;
        TitlePair8 *e = (TitlePair8 *)g_70840 + src;
        p += 2;
        d->a_00 = e->a_00;
        d->b_04 = e->b_04;
    } while (--n);
}

void eng1_31a70(TitleObj *o, int *vals, int count)
{
    int t;
    unsigned char *q;
    char *dst;
    char *dst4;
    int a, b, mid, hi;
    int pair[2];
    int *src;

    t = o->dword_04c;
    if (t == 0) {
        eng1_318d0((int)o, vals, count);
        return;
    }
    if (g_70c44 != 0) {
        src = g_70c44;
    } else {
        q = (unsigned char *)(g_header_pointer_70d2c + *(int *)(g_header_pointer_70d2c + o->w_036 * 4));
        q += o->w_036 * 4 + 2;
        src = (int *)(q + *q * 8 + 0xa);
    }
    dst = (char *)g_engine_interface.data_010 + 0x10;
    dst4 = dst + 4;
    do {
        a = *vals++;
        b = *src++;
        mid = (a << 8) >> 16;
        dispatch_engine_2c760(dst);
        dispatch_engine_2c7a0(dst4);
        dst4 += 8;
        hi = (((b >> 24) - (a >> 24)) * t >> 8) + (a >> 24);
        pair[0] = ((((b << 8) >> 16) - mid) * t >> 8) + mid + (hi << 16);
        dst += 8;
        pair[1] = (((signed char)b - (signed char)a) * t >> 8) + (signed char)a;
        dispatch_engine_2c690(pair);
        dispatch_engine_2c6e0();
    } while (--count);
    dispatch_engine_2c760(dst);
    dispatch_engine_2c7a0(dst + 4);
    g_70c44 = src + 1;
}

void eng1_31bc0(TitleObj *o, int *vals, int count)
{
    int *src;
    unsigned char *s;
    unsigned char *q;
    int t;
    int i;
    int n;
    int c;
    int a, b, mid, hi;
    int *dst;
    int *dst4;
    int pair[2];

    if (g_70c44 != 0) {
        src = g_70c44;
    } else {
        q = (unsigned char *)(g_header_pointer_70d2c + *(int *)(g_header_pointer_70d2c + o->w_036 * 4));
        q += o->w_036 * 4 + 2;
        src = (int *)(q + *q * 8 + 0xa);
    }
    dst = (int *)((char *)g_engine_interface.data_010 + 0x18);
    t = o->dword_04c;
    s = o->stream_05c + 1;
    n = *s;
    i = 0;
    dst4 = dst + 1;
    do {
        if (i == n) {
            vals++;
            c = s[1];
            s += 2;
            *dst = g_70840[c].a_00;
            *dst4 = g_70840[c].b_04;
            n = *s;
            src++;
        } else {
            a = *vals++;
            b = *src++;
            hi = a >> 24;
            hi += ((b >> 24) - hi) * t >> 8;
            mid = (a << 8) >> 16;
            pair[0] = ((((b << 8) >> 16) - mid) * t >> 8) + mid + (hi << 16);
            mid = (signed char)a;
            pair[1] = (((signed char)b - mid) * t >> 8) + mid;
            dispatch_engine_2c690(pair);
            dispatch_engine_2c6e0();
            dispatch_engine_2c760(dst);
            dispatch_engine_2c7a0(dst4);
        }
        dst += 2;
        dst4 += 2;
        i++;
    } while (i != count);
    g_70c44 = src + 1;
}

void eng1_31d30(TitleObj *o, int k)
{
    int v[4];
    int ox = (signed char)k;
    int oy = (short)(k >> 8);
    int oz = k >> 24;

    v[0] = (ox * o->w_03a) >> 7;
    if (o->flags_054 & 0x80) {
        v[1] = (o->w_03c * oy) >> 7;
        v[2] = (o->w_03e * oz) >> 7;
    } else {
        v[1] = (oy * o->w_03a) >> 7;
        v[2] = (oz * o->w_03a) >> 7;
    }
    v[0] += o->trans_078[0];
    v[1] += o->trans_078[1];
    v[2] += o->trans_078[2];
    dispatch_engine_2c720(v);
}

int eng1_31dc0(char *obj, int *vec)
{
    short v[3];

    v[0] = (short)(vec[0] >> 16);
    v[1] = (short)(vec[1] >> 16);
    v[2] = (short)(vec[2] >> 16);
    dispatch_engine_2c690(v);
    dispatch_engine_2c6e0();
    dispatch_engine_2c760(obj);
    obj += 4;
    dispatch_engine_2c7b0(obj);
    return *(short *)obj > 0x40;
}

int eng1_31e30(char *obj, int *vec, int k)
{
    short v[3];

    v[0] = (short)((((short *)g_engine_interface.data_014)[(k * 4 - 0x400) & 0xfff] * (short)(vec[0] >> 16)) >> 12);
    v[1] = (short)((((short *)g_engine_interface.data_014)[(k & 0x3ff) * 4] * (short)(vec[0] >> 16)) >> 12);
    v[2] = 0;
    dispatch_engine_2c690(v);
    dispatch_engine_2c6e0();
    dispatch_engine_2c760(obj);
    obj += 4;
    dispatch_engine_2c7b0(obj);
    return *(short *)obj > 0x40;
}

int profile_extent_31ec0(TitleObj *o, short *out, int mode)
{
    unsigned char *q;
    unsigned int count;
    int base;
    int v;
    int x, y, z;
    unsigned int w, h, d;

    q = (unsigned char *)(g_header_pointer_70d2c + *(int *)(g_header_pointer_70d2c + o->w_034 * 4));
    q += o->w_034 * 4 + 2;
    count = *q;
    q += 2;
    base = *(int *)q;
    q += 4;
    if (mode == 0) {
        if (count == 0) {
            return 0;
        }
    } else {
        if (mode > (int)count) {
            return 0;
        }
        q += mode * 8 - 8;
    }
    v = *(int *)q;
    q += 4;
    z = (signed char)v;
    x = (short)(v >> 8);
    y = (signed char)(v >> 24);
    if (base != 0) {
        x += (signed char)base;
        y += (short)(base >> 8);
        z += base >> 24;
    }
    v = *(int *)q;
    d = v & 0xff;
    w = (v >> 8) & 0xffff;
    h = (v >> 24) & 0xff;
    if (o->flags_054 & 0x80) {
        if ((o->w_03a | o->w_03c | o->w_03e) != 0x100) {
            x = x * o->w_03a >> 8;
            w = w * o->w_03a >> 8;
            y = y * o->w_03c >> 8;
            h = h * o->w_03c >> 8;
            z = z * o->w_03e >> 8;
            d = d * o->w_03e >> 8;
        }
    } else if (o->w_03a != 0x100) {
        x = x * o->w_03a >> 8;
        y = y * o->w_03a >> 8;
        z = z * o->w_03a >> 8;
        w = w * o->w_03a >> 8;
        h = h * o->w_03a >> 8;
        d = d * o->w_03a >> 8;
    }
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = w;
    out[4] = h;
    out[5] = d;
    return count;
}

int eng1_32050(TitleShape *obj, int k)
{
    int h = g_camera_focal_70ce4 >> 1;
    int q1 = (obj->unknown_03a * h) / obj->unknown_010;
    int q = q1;
    int m1, m, b;

    if (obj->unknown_054 & 0x80) {
        q = (obj->unknown_03c * h) / obj->unknown_010;
    }
    m1 = (q1 * k) >> 8;
    m = (q * k) >> 8;
    if ((unsigned int)(obj->unknown_00c + m1) > (unsigned int)(m1 * 2 + 0x140)) {
        return 1;
    }
    b = obj->unknown_00e + m;
    return (unsigned int)(m * 2 + 0xf0) < (unsigned int)b;
}

int eng1_32300(TitleObj *o);

int eng1_320d0(TitleObj *o)
{
    int r;

    r = eng1_32300(o);
    o->rot_070.pad = (short)r;
    dispatch_engine_2c760(&o->w_0c);
    dispatch_engine_2c730(o->trans_078);
    o->w_10 = (short)(o->trans_078[2] / 4);
    if ((unsigned int)(o->w_10 - 1) > 0x47f) {
        return -1;
    }
    return r;
}

typedef struct TitleMatrix {      /* PSX MATRIX */
    short m[3][3];
    long t[3];
} TitleMatrix;

extern unsigned char g_mode_table_value_70d00;
extern int g_frame_state_70c48;
extern int g_camera_z_base_70d1c;
extern int g_70c70;
extern int g_70c64;
extern short *g_sine_table_70f34;
extern TitleMatrix g_5ce70;
void dispatch_engine_2c710(TitleMatrix *m);

void eng1_32130(TitleObj *o)
{
    short v[3];
    int t[3];
    unsigned int flags;
    int dz;
    int a;
    int r;
    unsigned short c;
    int s0, s1;
    int x, z;
    int b;

    if (g_mode_table_value_70d00 != 0 && g_frame_state_70c48 != 0) {
        flags = o->flags_054;
        dz = (short)(o->z_008 >> 16) - g_camera_z_base_70d1c;
        if (!(flags & 0x2000000)) {
            dz -= 0x28c;
        }
        a = 0x40 - (short)(o->pos_000.x >> 16) - g_70c70;
        b = a - 0x400;
        r = a * 2;
        v[0] = (short)((g_sine_table_70f34[-b & 0xfff] * dz) >> 12);
        v[2] = (short)((g_sine_table_70f34[(-b - 0x400) & 0xfff] * dz) >> 12);
        v[1] = (short)((o->pos_000.y >> 16) - g_70c64);
        dispatch_engine_2c690(v);
        if (flags & 0x1000) {
            x = o->w_014;
            z = o->w_018;
            c = (unsigned short)g_70c70;
            s0 = g_sine_table_70f34[(c - 0x400) & 0xfff];
            s1 = g_sine_table_70f34[c & 0xfff];
            t[0] = ((x * s1 - z * s0) >> 12) + g_5ce70.t[0];
            t[1] = o->w_016 + g_5ce70.t[1];
            t[2] = ((z * s1 + x * s0) >> 12) + g_5ce70.t[2];
            dispatch_engine_2c720(t);
            dispatch_engine_2c6e0();
        } else {
            dispatch_engine_2c710(&g_5ce70);
            dispatch_engine_2c6e0();
        }
    } else {
        r = eng1_32300(o);
        if (r == -1) {
            o->flags_01f |= 8;
            return;
        }
    }
    o->rot_070.pad = (short)r;
    dispatch_engine_2c760(&o->w_0c);
    dispatch_engine_2c730(o->trans_078);
    o->w_10 = (short)(o->trans_078[2] / 4);
}

} /* extern "C" */
