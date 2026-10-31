/* ENG3.DLL 0x22f00..0x25040 (lane y7, 2026-10-20): ENG3's copy of the shared primitive/renderer unit
 * (TITLE calibration/title/u17cb0.cpp, ENG1 calibration/eng1/render_unit_30010.cpp), adapted with ENG3
 * names and per-DLL constants: ordering table 0x800 entries indexed by (depth & ~3), vertex scratch at
 * interface data +0x10 (ENG1 +0x18), depth limits 0x67f/0x7fe, reversed NCLIP sign, the track-lane colour
 * routine 0x23b00, the track-relative camera transform 0x24bb0 and its callers 0x24a80/0x24b00.
 * Recorded twins keep their recorded symbols (title_*, extent_producer_30e40). One C++ unit (extern "C");
 * No count-only context header (owner guidance 2026-10-20); with <string.h> first, as in the ENG1 unit,
 * 0x22f00 is masked equal too (2 bytes otherwise). Unit data tables are extern. */
#include "../title/title_engine.h"

extern "C" {

/* PSX-style display buffer; the ordering table starts at +0x88. */
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
#define ENG3_OT_SIZE 0x800
typedef struct Eng3Buffer {
    TitleDrawEnv draw;                /* 0x0000 */
    TitleDispEnv disp;                /* 0x005c */
    unsigned char unknown_070[0x18];  /* 0x0070 */
    unsigned long ot[ENG3_OT_SIZE];   /* 0x0088 ordering table */
} Eng3Buffer;
extern Eng3Buffer *g_eng3_frame_41448;
extern Eng3Buffer *g_eng3_frame_4144c;

typedef struct TitleObj TitleObj;
int title_04d80(int index, int key);
int eng3_24a80(TitleObj *o);
int title_19470(char *obj, int *vec);


void eng3_1fcd0(void);
void eng3_1fd20(void *a);

/* ---- primitive / renderer unit ---- */

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
    unsigned short w_038;
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

extern int g_eng3_matrix_394b8[8];
extern void *g_eng3_handlers_39528[8];         /* handler table */
extern TitleRecord g_eng3_records_39548[];
int title_04d80(int index, int key);
extern TitlePair8 g_eng3_40120[];
extern TitlePair8 *g_eng3_40520;
extern signed char g_eng3_shape_count_40788;
extern char *g_eng3_shape_table_40654;
int eng3_23d90(TitleObj *o);
void title_18b20(TitleObj *o);
void eng3_24b00(TitleObj *o);
void eng3_24f50(TitleObj *o, int v);
void eng3_1fd00(void);
void eng3_1fd80(void *e);
void eng3_1fdc0(void *b);
void eng3_1fd40(void *p);
unsigned long eng3_23b00(TitleObj *p);
int eng3_23ef0(TitleObj *o, unsigned char *q, unsigned long c);
int title_19570(TitleShape *obj, int k);
extern int g_eng3_focal_40624;
void eng3_1fca0(void *p);
void eng3_1fd50(void *p);
void eng3_1fdd0(void *p);

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

extern TitleObj *g_eng3_actor_431c0;
extern TitleTile *g_eng3_prim_43164;
void eng3_08ca0(TitleObj *o);
void title_02dc0(TitleObj *o);
int title_194e0(char *obj, int *vec, int k);

void eng3_22f00(TitleObj *obj)
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
    if (g_eng3_records_39548[obj->b_022].type_10 != 3) {
        if (title_04d80(obj->b_022, obj->w_034) == 0) {
            return;
        }
    }
    r = eng3_24a80(obj);
    if (r == -1) {
        obj->flags_01f |= 8;
        return;
    }
    eng3_24f50(obj, r);
    eng3_1fcd0();
    span = (TitleSpan *)obj->stream_05c - 1;
    count = obj->w_036;
    g_eng3_actor_431c0->byte_023 = 0;
    g_eng3_actor_431c0->w_02c = obj->w_02c;
    acc = 0;
    g_eng3_actor_431c0->b_022 = obj->b_022;
    g_eng3_actor_431c0->w_04a = obj->w_04a;
    if (flags & 0x100000) {
        g_eng3_actor_431c0->rot_070 = obj->rot_070;
    } else {
        g_eng3_actor_431c0->rot_070.vx = g_eng3_actor_431c0->rot_070.vy = g_eng3_actor_431c0->rot_070.vz = 0;
    }
    z = obj->rot_070.vz;
    ot = &g_eng3_frame_41448->ot[ENG3_OT_SIZE - 2 - (obj->w_10 & ~3)];
    while (count--) {
        span++;
        if (span->flags_00e & 0x8000) {
            title_19470((char *)xy, (int *)span);
            g_eng3_prim_43164->xy_008 = xy[0];
            *(unsigned long *)&g_eng3_prim_43164->r0 = 0xffffff;
            g_eng3_prim_43164->code = 0x68;
            g_eng3_prim_43164->next_000 = (unsigned long *)*ot;
            *ot = (unsigned long)g_eng3_prim_43164;
            g_eng3_prim_43164++;
        } else if (span->id_00c != 0) {
            if (flags & 0x200000) {
                if (flags & 0x400000) {
                    eng3_1fcd0();
                    acc += span->z_008;
                } else {
                    acc = span->z_008;
                }
                g_eng3_actor_431c0->rot_070.vz = z - acc;
                title_194e0((char *)&g_eng3_actor_431c0->w_0c, (int *)span, acc);
            } else {
                if (flags & 0x400000) {
                    eng3_1fcd0();
                }
                title_19470((char *)&g_eng3_actor_431c0->w_0c, (int *)span);
            }
            id = span->id_00c;
            e = (unsigned short *)g_eng3_records_39548[g_eng3_actor_431c0->b_022].table_0c + (short)(id * 2);
            if (e[0] != 0) {
                g_eng3_actor_431c0->w_034 = id;
                g_eng3_actor_431c0->flags_054 = (obj->flags_054 & 0xffff0000) | span->flags_00e;
                if ((g_eng3_actor_431c0->flags_054 & 0x600) == 0x600) {
                    g_eng3_actor_431c0->pos_000.x = obj->pos_000.x + span->pos_000.x;
                    g_eng3_actor_431c0->pos_000.y = span->pos_000.y + obj->pos_000.y;
                }
                g_eng3_actor_431c0->w_046 = e[1];
                eng3_08ca0(g_eng3_actor_431c0);
                title_02dc0(g_eng3_actor_431c0);
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

extern TitleCell g_eng3_sprite_cells_42560[];
extern signed char g_eng3_cell_order_39ac0[];
extern int g_eng3_42548;
extern unsigned int *g_eng3_40650;
extern unsigned char *g_eng3_uvs_40658;
extern int g_eng3_3fc7c;
extern int *g_eng3_40524;
void title_193e0(TitleObj *o, int k);

void eng3_1fcb0(void *a, void *b, void *c);
void eng3_1fd10(void);
void eng3_1fd90(TitlePrim *p);
void eng3_1fc80(unsigned long a, unsigned long b, unsigned long c);
void eng3_1fce0(void);
void eng3_1fd60(int *p);
void eng3_1fda0(TitlePrim *p);
void eng1_058e0(TitleObj *o, unsigned long *ot);
void eng1_059f0(TitleObj *o, unsigned long *ot);

void eng3_231a0(unsigned char *stream, TitleObj *obj, unsigned char count)
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

    eng3_1fd20(g_eng3_matrix_394b8);
    eng3_1fd50(g_eng3_actor_431c0->trans_078);
    g_eng3_actor_431c0->trans_078[1] += obj->w_028 * 2;
    eng3_1fd40(g_eng3_actor_431c0->trans_078);
    v0.z = 0;
    v1.z = 0;
    v2.z = 0;
    a = (signed char)*stream++;
    b = (signed char)*stream++;
    clut = ((obj->b_01e + 0xe0) << 6) | 0x30;
    flags = obj->flags_054;
    code = *stream++;
    w = obj->w_046;
    ot = &g_eng3_frame_41448->ot[ENG3_OT_SIZE - 2 - (obj->w_10 & ~3)];
    p = (TitlePrim *)g_eng3_prim_43164;
    more = code & 0x80;
    flip = flags & 0x10;
    do {
        row = w >> 4;
        t = g_eng3_cell_order_39ac0[w & 15];
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
        eng3_1fcb0(&v2, &v1, &v0);
        eng3_1fd10();
        off = row * sizeof(TitleCell);
        p->uv_014.words.tpage = ((TitleCell *)((char *)g_eng3_sprite_cells_42560 + off))->w_02;
        p->tex_004 = 0x2e000000;
        eng3_1fd90(p);
        v0.x = v1.x;
        eng3_1fca0(&v0);
        eng3_1fd00();
        p->next_000 = (TitlePrim *)*ot;
        *ot = (unsigned long)p;
        p->uv_00c.words.clut = clut;
        code = *stream;
        more = code & 0x80;
        stream++;
        if (!more) {
            w = ((TitleCell *)((char *)g_eng3_sprite_cells_42560 + off))->next_04[g_eng3_cell_order_39ac0[w & 15]];
        }
        eng3_1fd80(&p->xy_020);
        p++;
    } while (--count);
    g_eng3_prim_43164 = (TitleTile *)p;
}

void eng3_23440(unsigned char *stream, TitleObj *obj, unsigned char count)
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
    ot = &g_eng3_frame_41448->ot[ENG3_OT_SIZE - 2 - (obj->w_10 & ~3)];
    sel = flags & 0x10000;
    if (sel) {
        eng1_058e0(obj, ot);
    }
    if (g_eng3_42548 != 0) {
        eng3_24f50(obj, 0);
        eng3_1fcd0();
    }
    colour = eng3_23b00(obj);
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
    p = (TitlePrim *)g_eng3_prim_43164;
    more = code & 0x80;
    flip = flags & 0x10;
    do {
        row = w >> 4;
        t = g_eng3_cell_order_39ac0[w & 15];
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
        eng3_1fcb0(&v2, &v1, &v0);
        eng3_1fd10();
        off = row * sizeof(TitleCell);
        p->uv_014.words.tpage = ((TitleCell *)((char *)g_eng3_sprite_cells_42560 + off))->w_02 | blend;
        p->tex_004 = colour;
        eng3_1fd90(p);
        v0.x = v1.x;
        eng3_1fca0(&v0);
        eng3_1fd00();
        p->next_000 = (TitlePrim *)*ot;
        *ot = (unsigned long)p;
        p->uv_00c.words.clut = clut;
        more = *stream;
        code = more;
        more &= 0x80;
        stream++;
        if (!more) {
            w = ((TitleCell *)((char *)g_eng3_sprite_cells_42560 + off))->next_04[g_eng3_cell_order_39ac0[w & 15]];
        }
        eng3_1fd80(&p->xy_020);
        p++;
    } while (--count);
    g_eng3_prim_43164 = (TitleTile *)p;
    if (sel) {
        eng1_059f0(obj, ot);
    }
}

int eng3_23720(TitleObj *o, TitleFrameBox *box, int pass)
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
    scale = (o->w_03a * (g_eng3_focal_40624 >> 1)) / o->w_10;
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
unsigned char *title_18680(int index, int key);
int eng3_24870(TitleObj *o, short *out, int mode);

int extent_producer_30e40(TitleObj *o, short *out, int mode)
{
    unsigned char *p;
    signed char n;
    short x0, y0, z0, dx, dy, dz;

    if (o->w_034 == 0) {
        return 0;
    }
    if (o->b_022 == 0) {
        return eng3_24870(o, out, mode);
    }
    if (o->w_034 > g_eng3_records_39548[o->b_022].count_04) {
        return 0;
    }
    p = title_18680(o->b_022, o->w_034);
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

int title_18600(TitleObj *o, struct title_pos *pos)
{
    unsigned char *p;
    int n;

    pos->y = 0;
    pos->x = 0;
    if (o->w_034 > g_eng3_records_39548[o->b_022].count_04) {
        return 0;
    }
    p = title_18680(o->b_022, o->w_034);
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

unsigned char *title_18680(int index, int key)
{
    TitleRecord *rec;
    int *entries;
    int value;

    if (key == 0) {
        return 0;
    }
    if (g_eng3_records_39548[index].type_10 != 3) {
        if (title_04d80(index, key) == 0) {
            return 0;
        }
    }
    rec = &g_eng3_records_39548[index];
    entries = (int *)(rec->table_0c + rec->count_04 * 4 + 8);
    value = entries[(short)key];
    if (value == 0) {
        return 0;
    }
    return (unsigned char *)entries + (value >> 8);
}

typedef struct Eng3Layer {            /* 16-byte track layer header; the shorts are self-relative offsets */
    unsigned char link;
    unsigned char width;
    unsigned char extent;
    unsigned char depth;
    unsigned short rotation;
    short entries;
    short neighbours;
    short shape;
    unsigned char unknown_0c[4];
} Eng3Layer;

extern Eng3Layer *g_eng3_layers_40674;
extern unsigned long *g_eng3_lanes_40684[];
extern short g_eng3_lane_rows_40704[];
extern short g_eng3_lane_columns_40744[];
extern unsigned short *g_eng3_cell_flags_4067c;
extern int *g_eng3_shades_40678;
extern int g_eng3_ambient_40634;
extern long g_eng3_colour_40630;
extern unsigned long g_eng3_colors_41460[8][64];

unsigned long eng3_23b00(TitleObj *p)
{
    unsigned long flags;
    int mode;
    int x, z;
    int col, row;
    unsigned char *q;
    int count;
    int lane;
    unsigned long *e;
    int k;
    int w;
    long c;
    int v;
    short fz;
    int fx;
    short lo, hi;
    int t0, t1;
    int base;
    unsigned int n;

    k = 0;
    flags = p->flags_054;
    mode = flags & 0x600;
    if (mode) {
        x = (short)(p->pos_000.x >> 16) - 0x40;
        z = (short)(p->z_008 >> 16) + 0x300;
        q = (unsigned char *)&g_eng3_layers_40674[p->w_038].entries;
        q += *(short *)q;
        row = z >> 7;
        col = x >> 7;
        count = *q++;
        if (count != 0) {
            lane = *q;
            e = &g_eng3_lanes_40684[lane][(g_eng3_lane_rows_40704[lane] - col - 1) * g_eng3_lane_columns_40744[lane] + row];
            k = (*e >> 11) & 0x1fff;
        }
        w = g_eng3_cell_flags_4067c[k * 2 + 1];
        if (w & 8) {
            v = w >> 4;
        } else {
            v = g_eng3_shades_40678[w >> 4];
            if (mode == 0x600) {
                fz = z & 0x7f;
                fx = x & 0x7f;
                lo = (unsigned char)(v >> 8);
                hi = (unsigned char)(v >> 24);
                t0 = ((((v & 0xff) - lo) * fz) >> 7) + lo;
                t1 = (((((v >> 16) & 0xff) - hi) * fz) >> 7) + hi;
                v = (((t1 - t0) * fx) >> 7) + t0;
            }
        }
        base = g_eng3_ambient_40634 & 0xff;
        v = ((((v - base) * p->b_024) >> 8) + base) >> 2;
        if (v < 0) {
            v = 0;
        } else if (v > 0x3f) {
            v = 0x3f;
        }
        c = g_eng3_colors_41460[w & 7][v];
    } else {
        c = g_eng3_colour_40630;
    }
    n = p->w_04a;
    if (n != 0x80) {
        c = (((unsigned char)(((c >> 16) & 0xff) * n >> 7) << 8 | (unsigned char)(((c >> 8) & 0xff) * n >> 7)) << 8) |
            (unsigned char)((c & 0xff) * n >> 7);
    }
    if (flags & 4) {
        return c | 0x2e000000;
    }
    return c | 0x2c000000;
}

void title_189e0(TitleObj *arg)
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
            eng3_24b00(arg);
            if (arg->flags_054 & 0x20000000) {
                list = arg->list_120;
                count = *list;
                list++;
                if (count-- != 0) {
                    k = count + 1;
                    do {
                        eng3_24b00((TitleObj *)*list++);
                    } while (--k);
                }
            }
            arg = arg->next_068;
        } while (arg != (TitleObj *)-1);
    }
    o = first;
    g_eng3_40520 = g_eng3_40120;
    if (first != (TitleObj *)-1) {
        do {
            if (o->flags_054 & 0x20000000) {
                title_18b20(o);
            } else {
                o->flags_01f |= 8;
                if (o->flags_01f) {
                    o->byte_020 = (unsigned char)(g_eng3_40520 - g_eng3_40120);
                    eng3_23d90(o);
                }
            }
            o = o->next_068;
        } while (o != (TitleObj *)-1);
    }
}

int eng3_23d90(TitleObj *o)
{
    unsigned char *q;
    int idx;

    if ((unsigned int)(o->w_10 - 1) > 0x67f) {
        return 0;
    }
    if ((unsigned int)o->w_034 > (unsigned int)g_eng3_shape_count_40788) {
        return 0;
    }
    o->b_022 = 0;
    idx = o->w_034;
    q = (unsigned char *)(g_eng3_shape_table_40654 + *(int *)(g_eng3_shape_table_40654 + idx * 4));
    q += idx * 4;
    if (o->w_0c > 0x140 || o->w_0e > 0xf0) {
        if (title_19570((TitleShape *)o, q[3]) != 0) {
            return 0;
        }
    }
    eng3_24f50(o, o->rot_070.pad);
    eng3_23ef0(o, q, eng3_23b00(o));
    return 0;
}

void title_18b20(TitleObj *o)
{
    unsigned char *q;
    unsigned int *list;
    unsigned int count;
    unsigned long c;
    TitleObj *p;
    int idx;
    int n;

    eng3_24f50(o, o->rot_070.pad);
    eng3_1fd40(o->trans_078);
    if (o->w_034 == 0) {
        return;
    }
    idx = o->w_034;
    q = (unsigned char *)(g_eng3_shape_table_40654 + *(int *)(g_eng3_shape_table_40654 + idx * 4));
    q += idx * 4 + 2;
    q += *q * 8 + 8;
    n = *(short *)q;
    q += 2;
    ((void (*)(TitleObj *, int *, int))g_eng3_handlers_39528[o->byte_023])(o, (int *)q, n);
    list = o->list_120;
    count = *list;
    list++;
    c = eng3_23b00(o);
    while (count--) {
        p = (TitleObj *)*list++;
        idx = p->w_034;
        q = (unsigned char *)(g_eng3_shape_table_40654 + *(int *)(g_eng3_shape_table_40654 + idx * 4));
        q += idx * 4;
        eng3_24f50(p, p->rot_070.pad);
        eng3_23ef0(p, q, c);
    }
}

int eng3_23ef0(TitleObj *o, unsigned char *q, unsigned long c)
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

    g_eng3_40524 = 0;
    ot = (TitlePrim **)&g_eng3_frame_41448->ot[ENG3_OT_SIZE - 2];
    lvl = o->w_10 >> 5;
    if (lvl < 1) {
        lvl = 1;
    } else if (lvl > 0x1d) {
        lvl = 0x1d;
    }
    rec = (TitlePair8 *)(g_eng3_40650 + *(short *)q);
    q += 2;
    k = *q;
    q += 2;
    col = (lvl << 22) + 0x78300000;
    x = *(int *)q;
    q += k * 8 + 6;
    if (x != 0) {
        title_193e0(o, x);
    } else {
        eng3_1fd40(o->trans_078);
    }
    uvt = g_eng3_uvs_40658;
    o->flags_01f |= 2;
    h = o->byte_023;
    dbl = o->flags_054 & 0x40;
    vtx = (char *)g_engine_interface.data_010 + 0x10;
    prim = (TitlePrim *)g_eng3_prim_43164;
    if (g_engine_interface.context_004->unknown_040 != 0) {
        return 1;
    }
    do {
        cnt = *(short *)q;
        q += 2;
        ((TitleHandler)g_eng3_handlers_39528[h])(o, q, cnt);
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
                if ((w & 4) && g_eng3_3fc7c == 0) {
                    goto next;
                }
            }
            eng3_1fc80(v0->xy, v1->xy, v3->xy);
            eng3_1fce0();
            eng3_1fd60(&clip);
            if (clip > 0) {
                if (!(w & 0x40)) {
                    goto next;
                }
                xy1 = v1->xy;
                xy3 = v3->xy;
                eng3_1fc80(xy1, v2->xy, xy3);
                eng3_1fce0();
                prim->xy_008 = v0->xy;
                prim->xy_010 = xy1;
                prim->xy_018 = xy3;
                eng3_1fd60(&clip);
                if (clip > 0) {
                    goto next;
                }
            } else {
                eng3_1fda0(prim);
            }
        emit:
            prim->xy_020 = v2->xy;
            z = v0->z;
            if (dbl) {
                z = (z + v3->z + v1->z + v2->z) >> 4;
            } else {
                z = (z + v2->z) >> 3;
            }
            if ((unsigned int)z > 0x7fe) {
                z = 0x7fe;
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
                prim->uv_00c.packed = col | t[0];
                prim->uv_014.packed = t[1];
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
                g_eng3_40520->a_00 = d->xy;
                g_eng3_40520->b_04 = d->z;
                g_eng3_40520++;
                b++;
        }
        rec = (TitlePair8 *)(((int)b + 3) & ~3);
    } while ((char)more != 0);
    g_eng3_prim_43164 = (TitleTile *)prim;
    return 1;
}

void eng3_24280(int a1, int *vals, int count)
{
    int pair[2];
    unsigned char *dst;
    int v;

    dst = (unsigned char *)g_engine_interface.data_010 + 8;
    v = *vals;
    vals++;
    pair[0] = v >> 8;
    pair[1] = (signed char)v;
    eng3_1fca0(pair);
    eng3_1fd00();
    v = *vals;
    vals++;
    pair[0] = v >> 8;
    pair[1] = (signed char)v;
    dst += 8;
    if (--count == 0) {
        goto tail;
    }
    do {
        eng3_1fd80(dst);
        eng3_1fdc0(dst + 4);
        eng3_1fca0(pair);
        eng3_1fd00();
        v = *vals;
        vals++;
        pair[0] = v >> 8;
        pair[1] = (signed char)v;
        dst += 8;
    } while (--count);
tail:
    eng3_1fd80(dst);
    dst += 4;
    eng3_1fdc0(dst);
}

void eng3_24340(TitleStream *s, int *vals, int n)
{
    TitlePair8 *dst = (TitlePair8 *)((char *)g_engine_interface.data_010 + 0x10);
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
        eng3_1fca0(pair);
        eng3_1fd00();
        d1 = dst + p[1];
        d1->a_00 = d0->a_00;
        p += 2;
        d1->b_04 = d0->b_04;
        eng3_1fd80(d0);
        eng3_1fdc0(&d0->b_04);
    } while (--n);
}

void eng3_243d0(TitleStream *s)
{
    TitlePair8 *dst = (TitlePair8 *)((char *)g_engine_interface.data_010 + 0x10);
    unsigned char *p = s->stream_05c;
    unsigned int n = *p++;
    do {
        unsigned int di = p[0];
        unsigned int src = p[1];
        TitlePair8 *d = dst + di;
        TitlePair8 *e = (TitlePair8 *)g_eng3_40120 + src;
        p += 2;
        d->a_00 = e->a_00;
        d->b_04 = e->b_04;
    } while (--n);
}

void eng3_24420(TitleObj *o, int *vals, int count)
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
        eng3_24280((int)o, vals, count);
        return;
    }
    if (g_eng3_40524 != 0) {
        src = g_eng3_40524;
    } else {
        q = (unsigned char *)(g_eng3_shape_table_40654 + *(int *)(g_eng3_shape_table_40654 + o->w_036 * 4));
        q += o->w_036 * 4 + 2;
        src = (int *)(q + *q * 8 + 0xa);
    }
    dst = (char *)g_engine_interface.data_010 + 8;
    dst4 = dst + 4;
    do {
        a = *vals++;
        b = *src++;
        mid = (a << 8) >> 16;
        eng3_1fd80(dst);
        eng3_1fdc0(dst4);
        dst4 += 8;
        hi = (((b >> 24) - (a >> 24)) * t >> 8) + (a >> 24);
        pair[0] = ((((b << 8) >> 16) - mid) * t >> 8) + mid + (hi << 16);
        dst += 8;
        pair[1] = (((signed char)b - (signed char)a) * t >> 8) + (signed char)a;
        eng3_1fca0(pair);
        eng3_1fd00();
    } while (--count);
    eng3_1fd80(dst);
    eng3_1fdc0(dst + 4);
    g_eng3_40524 = src + 1;
}

void eng3_24570(TitleObj *o, int *vals, int count)
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

    if (g_eng3_40524 != 0) {
        src = g_eng3_40524;
    } else {
        q = (unsigned char *)(g_eng3_shape_table_40654 + *(int *)(g_eng3_shape_table_40654 + o->w_036 * 4));
        q += o->w_036 * 4 + 2;
        src = (int *)(q + *q * 8 + 0xa);
    }
    dst = (int *)((char *)g_engine_interface.data_010 + 0x10);
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
            *dst = g_eng3_40120[c].a_00;
            *dst4 = g_eng3_40120[c].b_04;
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
            eng3_1fca0(pair);
            eng3_1fd00();
            eng3_1fd80(dst);
            eng3_1fdc0(dst4);
        }
        dst += 2;
        dst4 += 2;
        i++;
    } while (i != count);
    g_eng3_40524 = src + 1;
}

void title_193e0(TitleObj *o, int k)
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
    eng3_1fd40(v);
}

int title_19470(char *obj, int *vec)
{
    short v[3];

    v[0] = (short)(vec[0] >> 16);
    v[1] = (short)(vec[1] >> 16);
    v[2] = (short)(vec[2] >> 16);
    eng3_1fca0(v);
    eng3_1fd00();
    eng3_1fd80(obj);
    obj += 4;
    eng3_1fdd0(obj);
    return *(short *)obj > 0x40;
}

int title_194e0(char *obj, int *vec, int k)
{
    short v[3];

    v[0] = (short)((((short *)g_engine_interface.data_014)[(k * 4 - 0x400) & 0xfff] * (short)(vec[0] >> 16)) >> 12);
    v[1] = (short)((((short *)g_engine_interface.data_014)[(k & 0x3ff) * 4] * (short)(vec[0] >> 16)) >> 12);
    v[2] = 0;
    eng3_1fca0(v);
    eng3_1fd00();
    eng3_1fd80(obj);
    obj += 4;
    eng3_1fdd0(obj);
    return *(short *)obj > 0x40;
}

int eng3_24870(TitleObj *o, short *out, int mode)
{
    unsigned char *q;
    unsigned int count;
    int base;
    int v;
    int x, y, z;
    unsigned int w, h, d;

    q = (unsigned char *)(g_eng3_shape_table_40654 + *(int *)(g_eng3_shape_table_40654 + o->w_034 * 4));
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

int title_19570(TitleShape *obj, int k)
{
    int h = g_eng3_focal_40624 >> 1;
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

typedef struct TitleMatrix {      /* PSX MATRIX */
    short m[3][3];
    long t[3];
} TitleMatrix;

typedef struct Eng3Cell {             /* 16-byte track cell (g_eng3_cells_40540[layer][column]) */
    long x;
    long z;
    short angle_08;
    short angle_0a;
    unsigned short unknown_0c;
    unsigned char curve;
    unsigned char pivot;
} Eng3Cell;
typedef struct Eng3View {             /* camera transform at 0x405a0 */
    long x, y, z;
    long pitch, yaw;
} Eng3View;
typedef struct Eng3Range {            /* 12-byte per-layer visible column range */
    long tick;
    long first;
    long end;
} Eng3Range;

extern Eng3Range *g_eng3_ranges_4078c;
extern Eng3Cell *g_eng3_cells_40540[];
extern Eng3View g_eng3_view_405a0;
extern short g_eng3_height_40640;
extern TitleMatrix g_eng3_matrix_394d8;
extern TitleMatrix g_eng3_matrix_394f8;
extern long g_eng3_scale_39518[];
void eng3_1fd30(TitleMatrix *m);
void eng3_1ff40(short *angles, TitleMatrix *m);
void eng3_1ff60(TitleMatrix *m, long *scale);
int eng3_24bb0(TitleObj *o);

int eng3_24a80(TitleObj *o)
{
    int col;
    int r;
    Eng3Range *range;

    col = (short)(o->pos_000.x >> 16) >> 7;
    range = &g_eng3_ranges_4078c[o->w_038];
    if (col < range->first) {
        return -1;
    }
    if (col >= range->end) {
        return -1;
    }
    r = eng3_24bb0(o);
    eng3_1fd80(&o->w_0c);
    eng3_1fdd0(&o->w_10);
    if ((unsigned int)(o->w_10 - 0x20) > 0x620) {
        return -1;
    }
    return r;
}

void eng3_24b00(TitleObj *o)
{
    Eng3Range *range;
    int x;

    range = &g_eng3_ranges_4078c[o->w_038];
    x = (short)(o->pos_000.x >> 16);
    if (x >> 7 >= range->end) {
        o->w_10 = 0;
        o->flags_01f |= 8;
        return;
    }
    if ((x - ((o->w_03a << 5) >> 8) - 0x30) >> 7 < range->first) {
        o->w_10 = 0;
        o->flags_01f |= 8;
        return;
    }
    o->rot_070.pad = (short)eng3_24bb0(o);
    eng3_1fd80(&o->w_0c);
    eng3_1fd50(o->trans_078);
    o->w_10 = (short)(o->trans_078[2] / 4);
}

int eng3_24bb0(TitleObj *o)
{
    short angles[3];
    short *v;
    int col;
    Eng3Cell *cell;
    Eng3Cell *pc;
    int curve;
    int pivot;
    int yaw;
    int a;
    int dx, dz;
    int s, c;
    int x, y;
    int k;
    int r;

    v = (short *)g_engine_interface.data_010;
    col = (short)(o->pos_000.x >> 16) >> 7;
    if (col < 0) {
        col = 0;
    }
    cell = g_eng3_cells_40540[o->w_038] + col;
    o->w_02c = 0;
    yaw = g_eng3_view_405a0.yaw;
    pivot = cell->pivot;
    curve = cell->curve;
    if (curve == 0) {
        dx = cell->x - g_eng3_view_405a0.x;
        r = -(cell->angle_08 + yaw);
        dz = cell->z - g_eng3_view_405a0.z;
        s = ((short *)g_engine_interface.data_014)[yaw & 0xfff];
        c = ((short *)g_engine_interface.data_014)[(yaw - 0x400) & 0xfff];
        dz >>= 6;
        dx >>= 6;
        g_eng3_matrix_394f8.t[0] = ((dx * s - dz * c) >> 18) << 1;
        g_eng3_matrix_394f8.t[1] = 0;
        g_eng3_matrix_394f8.t[2] = (((dz * s + dx * c) >> 18) + g_eng3_height_40640) * 2;
        angles[0] = (short)g_eng3_view_405a0.pitch;
        angles[1] = (short)r;
        angles[2] = 0;
        eng3_1ff40(angles, &g_eng3_matrix_394f8);
        eng3_1ff60(&g_eng3_matrix_394f8, g_eng3_scale_39518);
        eng3_1fd20(&g_eng3_matrix_394f8);
        eng3_1fd30(&g_eng3_matrix_394f8);
        v[0] = (short)(o->z_008 >> 16);
        v[2] = (short)((o->pos_000.x >> 16) - (pivot << 7));
    } else {
        a = -(cell->angle_0a + yaw);
        pc = g_eng3_cells_40540[o->w_038] + pivot;
        dz = (pc->z - g_eng3_view_405a0.z) >> 6;
        dx = (pc->x - g_eng3_view_405a0.x) >> 6;
        s = ((short *)g_engine_interface.data_014)[yaw & 0xfff];
        c = ((short *)g_engine_interface.data_014)[(yaw - 0x400) & 0xfff];
        x = (dx * s - dz * c) >> 6;
        y = (dz * s + dx * c) >> 6;
        if (curve == 1) {
            x += ((short *)g_engine_interface.data_014)[a & 0xfff] * 0x518;
            k = a + 0x400;
        } else {
            x += ((short *)g_engine_interface.data_014)[(a - 0x800) & 0xfff] * 0x518;
            k = a - 0x400;
        }
        g_eng3_matrix_394d8.t[0] = (x >> 12) << 1;
        g_eng3_matrix_394d8.t[1] = 0;
        y += ((short *)g_engine_interface.data_014)[k & 0xfff] * 0x518;
        g_eng3_matrix_394d8.t[2] = ((y >> 12) + g_eng3_height_40640) << 1;
        angles[0] = (short)g_eng3_view_405a0.pitch;
        angles[1] = 0;
        angles[2] = 0;
        eng3_1ff40(angles, &g_eng3_matrix_394d8);
        eng3_1ff60(&g_eng3_matrix_394d8, g_eng3_scale_39518);
        eng3_1fd20(&g_eng3_matrix_394d8);
        eng3_1fd30(&g_eng3_matrix_394d8);
        if (curve == 1) {
            k = 0x518 - (short)(o->z_008 >> 16);
            a += ((short)(o->pos_000.x >> 16) - (pivot << 7)) / 2 - 0x400;
        } else {
            k = (short)(o->z_008 >> 16) + 0x518;
            a -= ((short)(o->pos_000.x >> 16) - (pivot << 7)) / 2 - 0x400;
        }
        r = -(cell->angle_08 + yaw);
        v[0] = (short)((((short *)g_engine_interface.data_014)[(a - 0x400) & 0xfff] * k) >> 12);
        v[2] = (short)((((short *)g_engine_interface.data_014)[a & 0xfff] * k) >> 12);
    }
    v[1] = (short)((o->pos_000.y >> 16) - g_eng3_view_405a0.y);
    eng3_1fca0(v);
    eng3_1fd00();
    return r * 2;
}

void eng3_24f50(TitleObj *o, int v)
{
    short src[4];
    int scale[4];
    int m[8];
    int s;
    unsigned short x, z, y;

    src[0] = (short)(o->rot_070.vx << 2);
    src[1] = (short)((o->rot_070.vy << 2) + v / 2);
    src[2] = (short)(o->rot_070.vz << 2);
    eng3_1ff40(src, (TitleMatrix *)m);
    if (o->flags_054 & 0x80) {
        x = o->w_03a;
        z = o->w_03e;
        y = o->w_03c;
        if ((unsigned short)(x | y | z) != 0x100) {
            scale[0] = x << 5;
            scale[1] = y << 5;
            scale[2] = z << 5;
            eng3_1ff60((TitleMatrix *)m, (long *)scale);
        }
    } else if (o->w_03a != 0x100) {
        s = o->w_03a << 5;
        scale[2] = s;
        scale[1] = s;
        scale[0] = s;
        eng3_1ff60((TitleMatrix *)m, (long *)scale);
    } else {
        eng3_1ff60((TitleMatrix *)m, g_eng3_scale_39518);
    }
    eng3_1fd20(m);
}

} /* extern "C" */
