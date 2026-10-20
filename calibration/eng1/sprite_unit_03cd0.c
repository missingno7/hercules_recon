/* ENG1.DLL sprite-cell / primitive unit, functions 0x3cd0..0x59f0 in address order, followed by the colour fade
   group 0x5a60..0x5dc0 (lane x2 scratch, 2026-10-20). Adapted from calibration/title/u02b70.c: the TITLE unit
   compiles unchanged at the ENG1 shared rows; per-DLL differences are the display-buffer layout (ordering table
   at +0x88, objects linked at ot[0x4bf - depth]), the level-dependent primitive pool start (0x5600), the 0x5370
   streaming branch, the 0x5820 palette row and the 0x58e0/0x59f0 drawing-area bracket (TITLE's single 0x1dd0).
   Context declarations are the ENG1 analogues of the TITLE unit's (0x3a50 = TITLE 0x2b70, 0x3bd0 = 0x2cc0,
   0x3540 = 0x4b70, 0x38d0 = 0x4f00). Globals are named by ENG1 RVA. The unit is C with <windows.h> first, as
   decided for the TITLE unit; the colour group may be a separate TU (its rows match here). */
#include <windows.h>
#include "../title/title_engine.h"
/* Records shared by the TITLE screen units (sequence screens and their helpers). */
#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

/* TITLE process record, shared by the scheduler unit and every screen handler.
   A handler is the process callback: the scheduler calls fn_00(proc) when delay_08 counts down to
   zero, and the handler sets state_04 (its phase) and delay_08 (ticks until its next call). */
#ifndef TITLE_PROC_H
#define TITLE_PROC_H

typedef struct TitleProc TitleProc;
struct TitleProc {
    void (*fn_00)(TitleProc *);
    unsigned long state_04;
    long delay_08;
    unsigned long unknown_0c;
    TitleProc *next_10;
    unsigned long unknown_14;
};

#endif

/* Screen context (g_engine_interface.context_004). The handlers read both cursor fields into
   unused locals; the reads are volatile in the original (see docs/title_link_goal.md). */
typedef struct ScreenContext {
    unsigned char unknown_00[0x5c];
    volatile short cursor_5c;
    volatile short cursor_5e;
} ScreenContext;

/* Display object (sprite/actor) record, 0x134 bytes: allocated in a pool of 0x134-byte entries
   (0x1d380). Members are the union of what the reconstructed users touch; unknown_* names keep
   their offsets until a meaning is evidenced. */
typedef struct TitleObject TitleObject;
struct TitleObject {
    long unknown_000;                 /* x, 16.16 fixed point (signed) */
    long unknown_004;                 /* y, 16.16 fixed point (signed) */
    long unknown_008;                 /* z */
    short unknown_00c;
    short unknown_00e;
    short unknown_010;                /* depth: ordering-table slot */
    unsigned char unknown_012[0x014 - 0x012];
    short unknown_014;
    short unknown_016;
    short unknown_018;
    unsigned char unknown_01a[0x01c - 0x01a];
    unsigned char unknown_01c;
    unsigned char unknown_01d;
    unsigned char unknown_01e;
    unsigned char unknown_01f;
    unsigned char unknown_020[0x022 - 0x020];
    unsigned char unknown_022;
    unsigned char unknown_023;        /* render mode */
    unsigned char unknown_024[0x026 - 0x024];
    short unknown_026;                /* ENG1 0x58e0: subtracted from the y position */
    unsigned char unknown_028[0x02e - 0x028];
    unsigned short unknown_02e;
    unsigned char unknown_030[0x034 - 0x030];
    unsigned short unknown_034;       /* frame */
    unsigned short unknown_036;
    unsigned char unknown_038[0x03a - 0x038];
    unsigned short unknown_03a;
    unsigned short unknown_03c;
    unsigned short unknown_03e;
    unsigned short unknown_040;
    unsigned short unknown_042;
    unsigned short unknown_044;
    unsigned short unknown_046;
    unsigned short unknown_048;
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x050 - 0x04c];
    unsigned long unknown_050;
    unsigned long unknown_054;        /* render flags (0x10 mirrors x movement in 0x5cc0) */
    void *unknown_058;
    TitleObject *unknown_05c;
    TitleObject *unknown_060;
    unsigned long unknown_064;
    TitleObject *unknown_068;         /* next object in a list */
    void *unknown_06c;
    unsigned short unknown_070;
    unsigned short unknown_072;
    unsigned short unknown_074;
    unsigned char unknown_076[0x120 - 0x076];
    int *unknown_120;
    unsigned char unknown_124[0x12c - 0x124];
    unsigned long unknown_12c;
    unsigned long unknown_130;
};

#endif
/* TITLE resource slots: one 20-byte record per loadable title file. The table g_5cee0 is
   initialized data of the 0x17cb0 unit (paths and entry counts); +0x0c holds the loaded data
   (released through title_0c330) and +0x10/+0x12 the load state. */
#ifndef TITLE_SLOTS_H
#define TITLE_SLOTS_H

typedef struct TitleRecord {
    char *path_00;
    unsigned short count_04;
    unsigned short unknown_06;
    void *unknown_08;
    unsigned char *table_0c;
    unsigned short state_10;
    unsigned short state_12;
} TitleRecord;

extern TitleRecord g_5cee0[18];

#endif
/* ENG1 display buffer: the PSX DRAWENV/DISPENV pair as in TITLE (title_gpu.h), but the ordering table
   sits at +0x88 (cleared with 0x500 entries by 0x5600); objects are linked at ot[0x4bf - depth]. */
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

/* 40-byte primitive record taken from the pool at g_frame_global_73484. */
typedef struct TitlePrim {
    struct TitlePrim *next_000;
    unsigned long tex_004;
    unsigned long xy_008;
    unsigned short w_00c;
    unsigned short w_00e;
    unsigned long xy_010;
    unsigned short w_014;
    unsigned short w_016;
    unsigned long xy_018;
    unsigned short w_01c;
    unsigned short unknown_01e;
    unsigned long xy_020;
    unsigned short w_024;
    unsigned short unknown_026;
} TitlePrim;

extern TitlePrim *g_frame_global_73484;

unsigned long eng1_311a0(TitleObject *obj);
void eng1_04010(unsigned char *stream, TitleObject *obj, unsigned char count);
void eng1_04330(unsigned char *stream, TitleObject *obj, unsigned char count);
void eng1_04650(unsigned char *stream, TitleObject *obj, unsigned char count, int sx, int sy);
void eng1_048b0(unsigned char *stream, TitleObject *obj, unsigned char count, int sx, int sy);

extern int g_72868;
extern void (*g_frame_callbacks_4d6f8[8])();

extern int g_camera_focal_70ce4;
void eng1_05370(TitleObject *p, int count, int stride);
int eng1_31330(int a);
int eng1_320d0(TitleObject *p);
void eng1_058e0(TitleObject *p, unsigned long *ot);
void eng1_059f0(TitleObject *p, unsigned long *ot);
void eng1_03bd0(TitleObject *p);
void eng1_03cd0(TitleObject *p);
void eng1_03f60(unsigned char *stream, TitleObject *p, unsigned char count);
unsigned short eng1_target_04ca0(unsigned char *q, int n, int m);
void eng1_30790(unsigned char *stream, TitleObject *p, unsigned char count);
int eng1_30d00(TitleObject *p, unsigned char *box, int pass);

typedef struct TitleTabEntry { unsigned short a_00; unsigned short b_02; } TitleTabEntry;
extern TitleTabEntry g_727e0[];
extern unsigned char g_frame_global_73498;
extern int g_frame_owner_7348c;
extern TitleRect g_4d810;
extern unsigned short g_frame_seed_words_4d818[32];
extern unsigned char g_frame_setup_result_734ec;
extern char *g_frame_owner_70c80;
unsigned char dispatch_local_056c0(char *p, int b, int c, int d);
unsigned char dispatch_local_05660(unsigned short key, unsigned int f);
void dispatch_local_05780(int a, int v, int b);
void release_actor_resource_state(TitleObject *p);
TitleTabEntry *release_actor_token(TitleObject *c);
static int g_6ee7c;
extern int g_frame_owner_72870;
extern unsigned short g_frame_upload_coordinates_734a0[0x20];
void dispatch_engine_2c7f0(void *p, int n);

extern TitleRect g_4d800[2];
static int g_6ee78;
extern unsigned short g_713a0;
extern unsigned short g_713a8;
extern unsigned short g_713b6;
extern void *g_72860;
extern TitleObject *g_frame_owner_list_73490;
extern void *g_73494;
extern void *g_734e0;
extern TitleObject *g_frame_resource_list_734e4;
extern TitleObject *g_frame_actor_list_734f8;

void eng1_03cd0(TitleObject *p)
{
    unsigned int off;
    unsigned char count;
    unsigned char *stream;
    int clipped;
    unsigned long flags;
    unsigned char *base;
    long frame;
    unsigned char *s;
    unsigned short n;
    TitleTabEntry *use;
    unsigned long *src;
    unsigned int pal;
    unsigned int slot;
    unsigned short cell;
    unsigned char *pals;
    int y;
    unsigned int key;
    unsigned char *q;

    n = g_5cee0[p->unknown_022].count_04;
    if (p->unknown_034 > n)
        return;
    if (p->unknown_010 <= 0)
        return;
    base = g_5cee0[p->unknown_022].table_0c + n * 4 + 8;
    frame = ((long *)base)[(short)p->unknown_034];
    if (frame == 0)
        return;
    s = base + (frame >> 8);
    count = *s++;
    if (count == 0)
        return;
    clipped = 0;
    s += *(signed char *)s * 6 + 1;
    flags = p->unknown_054;
    if (!(flags & 0x80000)) {
        y = p->unknown_00e;
        if ((unsigned short)p->unknown_00c > 0x140 || (unsigned int)y > 0xf0) {
            clipped = eng1_30d00(p, s, 0);
            if (clipped) {
                if (!(flags & 8))
                    return;
                if (eng1_30d00(p, s, 1))
                    return;
            }
        }
    }
    s += 4;
    stream = s;
    if (p->unknown_046 == 0xffff) {
        use = &((TitleTabEntry *)g_5cee0[p->unknown_022].table_0c)[(short)p->unknown_034];
        if (use->a_00 == 0) {
            src = (unsigned long *)(base + 4) + *(unsigned short *)base;
            ((unsigned long *)g_engine_interface.data_010)[0] = src[0];
            ((unsigned long *)g_engine_interface.data_010)[1] = src[1];
            cell = eng1_target_04ca0(s += 2, count, frame);
            if (cell == 0xffff)
                return;
            use->b_02 = cell;
            if (frame & 0x40)
                use->a_00 = 0x40;
        }
        p->unknown_046 = use->b_02;
        use->a_00++;
    }
    if (p->unknown_01c != 0xff)
        pal = p->unknown_01c;
    else
        pal = frame & 0x1f;
    if (p->unknown_01d != pal) {
        release_actor_token(p);
        if (p->unknown_01d == 0xff) {
            off = pal * 2;
            key = (p->unknown_022 << 8) + pal;
        } else {
            off = p->unknown_01d * 2;
            key = (p->unknown_022 << 8) + p->unknown_01d;
        }
        p->unknown_01e = dispatch_local_05660(key, frame);
        if (p->unknown_01e & 0x80) {
            p->unknown_01e &= 0x7f;
            pals = (unsigned char *)((unsigned long *)(base + 4) + *(unsigned short *)base + 2);
            slot = *(unsigned short *)(pals + off);
            q = base + slot;
            dispatch_local_05780((int)(q + 4), p->unknown_01e, *(unsigned short *)q);
        }
        p->unknown_01d = (unsigned char)pal;
    }
    if (!clipped) {
        p->unknown_01f |= 2;
        if (g_engine_interface.context_004->unknown_040)
            return;
        if (p->unknown_023 == 0)
            eng1_03f60(stream, p, count);
        else
            g_frame_callbacks_4d6f8[p->unknown_023](stream, p, count);
    }
    if (flags & 8) {
        p->unknown_01f |= 2;
        if (g_engine_interface.context_004->unknown_040)
            return;
        eng1_30790(stream, p, count);
    }
}


void eng1_03f60(int a1, TitleObject *p, int a3)
{
    long v1;
    long v2;
    short d;
    long half;

    d = p->unknown_010;
    if (d <= 0)
        return;
    half = g_camera_focal_70ce4 >> 1;
    v1 = v2 = (long)p->unknown_03a * half / d;
    if (p->unknown_054 & 0x80)
        v2 = (long)p->unknown_03c * half / d;
    if ((v2 | v1) == 0x100) {
        if (p->unknown_074 != 0)
            eng1_04330(a1, p, a3);
        else
            eng1_04010(a1, p, a3);
        return;
    }
    if (p->unknown_074 != 0)
        eng1_048b0(a1, p, a3, v1, v2);
    else
        eng1_04650(a1, p, a3, v1, v2);
}

typedef struct TitleCell {
    unsigned short mask_00;
    unsigned short w_02;
    unsigned short next_04[4];
} TitleCell;
extern TitleCell g_72880[];
extern signed char g_5e128[];
extern short g_actor_stage_table_5a848[];

/* 0x4010 (TITLE 0x3100): emit the object's sprite parts (unrotated, unscaled) as textured quads into the
   ordering table. stream: dx, dy, header byte, then per part size byte, x, y and the next
   header byte (bit 7: the part continues in the same cell). */
void eng1_04010(unsigned char *stream, TitleObject *obj, unsigned char count)
{
    unsigned long tex;
    unsigned long flags;
    int d;
    int x0;
    int y0;
    unsigned short cell;
    unsigned char hb;
    unsigned char flag;
    unsigned long mode;
    unsigned short clut;
    unsigned long *ot;
    unsigned short flip;
    TitlePrim *prim;
    signed char *quad;
    unsigned char u;
    unsigned char v;
    int w;
    int h;
    int x;
    int y;
    int xa;
    int xb;
    unsigned int off;

    tex = eng1_311a0(obj);
    flags = obj->unknown_054;
    d = *(signed char *)stream++;
    if (flags & 0x10)
        d = -d;
    cell = obj->unknown_046;
    x0 = obj->unknown_00c + d;
    d = *(signed char *)stream++;
    y0 = obj->unknown_00e + d;
    flag = *stream;
    hb = flag;
    mode = (flags & 3) << 5;
    clut = ((obj->unknown_01e + 0xe0) << 6) | 0x30;
    flag &= 0x80;
    stream++;
    ot = &g_dispatch_frame_current_71ec4->ot[ENG1_OT_SIZE - 0x41 - obj->unknown_010];
    prim = g_frame_global_73484;
    flip = flags & 0x10;
    do {
        quad = &g_5e128[cell & 15];
        v = ((cell >> 4) * 2 + (*quad >> 1)) << 4;
        u = ((cell >> 8) * 2 + (*quad & 1)) << 4;
        if (flag) {
            u += (hb & 15) << 3;
            v += (hb >> 1) & 0x38;
        }
        w = (*(signed char *)stream & 15) << 3;
        h = (*(signed char *)stream >> 1) & ~7;
        stream++;
        x = *(signed char *)stream++ * 2;
        y = *(signed char *)stream++ * 2;
        y += y0;
        if (flip) {
            xa = (x0 - x) & 0xffff;
            prim->xy_008 = (y << 16) + xa;
            xb = (x0 - w - x) & 0xffff;
            prim->xy_010 = (y << 16) + xb;
            y = (y + h) << 16;
            prim->xy_018 = xa + y;
            prim->xy_020 = y + xb;
            if (u == 0) {
                u = 1;
                w--;
            }
            if (v + h == 0x100)
                h--;
            prim->w_00c = (unsigned char)(u - 1) + (v << 8);
            prim->w_014 = (unsigned char)(u + w - 1) + (v << 8);
            prim->w_01c = (unsigned char)(u - 1) + ((v + h) << 8);
            prim->w_024 = (unsigned char)(u + w - 1) + ((v + h) << 8);
        } else {
            xa = (x + x0) & 0xffff;
            prim->xy_008 = (y << 16) + xa;
            xb = (x + w + x0) & 0xffff;
            prim->xy_010 = (y << 16) + xb;
            y = (y + h) << 16;
            prim->xy_018 = xa + y;
            prim->xy_020 = xb + y;
            if (u + w == 0x100)
                w--;
            if (v + h == 0x100)
                h--;
            prim->w_014 = (unsigned char)(u + w) + (v << 8);
            prim->w_00c = u + (v << 8);
            prim->w_01c = u + ((v + h) << 8);
            prim->w_024 = (unsigned char)(u + w) + ((v + h) << 8);
        }
        off = (cell >> 4) * sizeof(TitleCell);
        prim->w_016 = ((TitleCell *)((char *)g_72880 + off))->w_02 | mode;
        prim->tex_004 = tex;
        prim->w_00e = clut;
        prim->next_000 = (TitlePrim *)*ot;
        *ot = (unsigned long)prim;
        prim++;
        flag = *stream;
        hb = flag;
        flag &= 0x80;
        stream++;
        if (!flag)
            cell = ((TitleCell *)((char *)g_72880 + off))->next_04[*quad];
    } while (--count != 0);
    g_frame_global_73484 = prim;
}

/* 0x4330 (TITLE 0x3420): as 0x4010 with the part corners rotated by the object angle (+0x74, 1024 steps; sine table
   g_actor_stage_table_5a848, cos = entry + 0x100), 16.16 products. Origin: 2026-10-10 draft (src/title/behavioral/emitters.c). */
void eng1_04330(unsigned char *stream, TitleObject *obj, unsigned char count)
{
    unsigned long tex;
    unsigned long flags;
    int dx;
    int x0;
    int y0;
    unsigned char hb;
    unsigned char flag;
    unsigned short cell;
    unsigned long attr;
    unsigned short clut;
    unsigned long *bucket;
    unsigned short flip;
    TitlePrim *prim;
    unsigned int u;
    unsigned int v;
    signed char *q;
    int g;
    int a;
    int w;
    int h;
    int A;
    int Vn;
    int B;
    int W;
    int sin2;
    int cos2;
    unsigned int off;

    tex = eng1_311a0(obj);
    flags = obj->unknown_054;
    dx = (signed char)*stream++;
    if (flags & 0x10)
        dx = -dx;
    x0 = obj->unknown_00c + dx;
    y0 = obj->unknown_00e + (signed char)*stream++;
    cell = obj->unknown_046;
    clut = ((obj->unknown_01e + 0xe0) << 6) | 0x30;
    hb = *stream++;
    flag = hb & 0x80;
    sin2 = g_actor_stage_table_5a848[(short)obj->unknown_074 & 0x3ff] * 2;
    cos2 = g_actor_stage_table_5a848[((short)obj->unknown_074 + 0x100) & 0x3ff] * 2;
    attr = (flags & 3) << 5;
    bucket = &g_dispatch_frame_current_71ec4->ot[ENG1_OT_SIZE - 0x41 - obj->unknown_010];
    prim = g_frame_global_73484;
    flip = (unsigned short)(flags & 0x10);
    do {
        q = &g_5e128[cell & 0xf];
        g = *q;
        u = ((cell >> 4) * 2 + (g >> 1)) << 4;
        v = ((cell >> 8) * 2 + (g & 1)) << 4;
        if (flag) {
            v += (hb & 0xf) * 8;
            u += (hb >> 1) & 0x38;
        }
        a = (signed char)*stream++;
        w = (a & 0xf) << 3;
        h = (a >> 1) & ~7;
        prim->w_00c = (unsigned char)v + (u << 8);
        prim->w_014 = (unsigned char)(v + w - 1) + (u << 8);
        prim->w_01c = (unsigned char)v + ((u + h + 0xff) << 8);
        prim->w_024 = (unsigned char)(v + w - 1) + ((u + h + 0xff) << 8);
        if (flip) {
            A = -(signed char)*stream++ * 2;
            Vn = A - w;
        } else {
            A = (signed char)*stream++ * 2;
            Vn = A + w;
        }
        B = (signed char)*stream++ * 2;
        W = B + h;
        prim->xy_008 = (unsigned short)(x0 + ((cos2 * A - sin2 * B) >> 16)) + ((y0 + ((sin2 * A + cos2 * B) >> 16)) << 16);
        prim->xy_010 = (unsigned short)(x0 + ((cos2 * Vn - sin2 * B) >> 16)) + ((y0 + ((sin2 * Vn + cos2 * B) >> 16)) << 16);
        prim->xy_018 = (unsigned short)(x0 + ((cos2 * A - sin2 * W) >> 16)) + ((y0 + ((sin2 * A + cos2 * W) >> 16)) << 16);
        prim->xy_020 = (unsigned short)(x0 + ((cos2 * Vn - sin2 * W) >> 16)) + ((y0 + ((sin2 * Vn + cos2 * W) >> 16)) << 16);
        off = (cell >> 4) * sizeof(TitleCell);
        prim->w_016 = ((TitleCell *)((char *)g_72880 + off))->w_02 | attr;
        prim->tex_004 = tex;
        prim->next_000 = (TitlePrim *)*bucket;
        *bucket = (unsigned long)prim;
        prim->w_00e = clut;
        hb = *stream;
        prim++;
        flag = hb & 0x80;
        stream++;
        if (!flag)
            cell = ((TitleCell *)((char *)g_72880 + off))->next_04[*q];
    } while (--count);
    g_frame_global_73484 = prim;
}

/* 0x4650 (TITLE 0x3740): as 0x4010 with the part offsets and sizes scaled by sx/sy (8.8 fixed point). Origin: lane tA. */
void eng1_04650(unsigned char *stream, TitleObject *obj, unsigned char count, int sx, int sy)
{
    unsigned long tex;
    unsigned long flags;
    int d;
    int x0;
    int y0;
    unsigned short cell;
    unsigned char hb;
    unsigned char flag;
    unsigned long mode;
    unsigned short clut;
    unsigned long *ot;
    unsigned short flip;
    TitlePrim *prim;
    signed char *quad;
    unsigned int k;
    int g;
    int u;
    int v;
    int w;
    int h;
    int xa;
    int xb;
    int y;
    unsigned short x1;
    unsigned short x2;
    int y1;
    int y2;
    int y3;
    unsigned int off;

    tex = eng1_311a0(obj);
    flags = obj->unknown_054;
    d = sx * *(signed char *)stream++ >> 8;
    if (flags & 0x10)
        d = -d;
    cell = obj->unknown_046;
    x0 = obj->unknown_00c + d;
    y0 = obj->unknown_00e + (sy * *(signed char *)stream++ >> 8);
    flag = *stream;
    hb = flag;
    prim = g_frame_global_73484;
    clut = ((obj->unknown_01e + 0xe0) << 6) | 0x30;
    flag &= 0x80;
    stream++;
    mode = (flags & 3) << 5;
    ot = &g_dispatch_frame_current_71ec4->ot[ENG1_OT_SIZE - 0x41 - obj->unknown_010];
    flip = flags & 0x10;
    do {
        quad = &g_5e128[cell & 15];
        g = *quad;
        k = cell >> 4;
        v = (k * 2 + (g >> 1)) << 4;
        u = ((cell >> 8) * 2 + (g & 1)) << 4;
        if (flag) {
            u += (hb & 15) * 8;
            v += (hb >> 1) & 0x38;
        }
        w = (*(signed char *)stream & 15) << 3;
        h = (*(signed char *)stream >> 1) & ~7;
        stream++;
        prim->w_00c = (unsigned char)u + (v << 8);
        prim->w_014 = (unsigned char)(u + w - 1) + (v << 8);
        prim->w_01c = (unsigned char)u + ((v + h - 1) << 8);
        prim->w_024 = (unsigned char)(u + w - 1) + ((v + h - 1) << 8);
        if (flip) {
            xa = -*(signed char *)stream++ * 2;
            xb = xa - w;
        } else {
            xa = *(signed char *)stream++ * 2;
            xb = xa + w;
        }
        y = *(signed char *)stream++ * 2;
        xa *= sx;
        xb *= sx;
        y2 = (y + h) * sy;
        y *= sy;
        x1 = (xa >> 8) + x0;
        y1 = (y >> 8) + y0;
        prim->xy_008 = (y1 << 16) + x1;
        x2 = (xb >> 8) + x0;
        prim->xy_010 = (y1 << 16) + x2;
        y3 = (y2 >> 8) + y0;
        prim->xy_018 = (y3 << 16) + x1;
        prim->xy_020 = (y3 << 16) + x2;
        off = k * sizeof(TitleCell);
        prim->w_016 = ((TitleCell *)((char *)g_72880 + off))->w_02 | mode;
        prim->tex_004 = tex;
        prim->w_00e = clut;
        prim->next_000 = (TitlePrim *)*ot;
        *ot = (unsigned long)prim;
        prim++;
        flag = *stream;
        hb = flag;
        flag &= 0x80;
        stream++;
        if (!flag)
            cell = ((TitleCell *)((char *)g_72880 + off))->next_04[*quad];
    } while (--count != 0);
    g_frame_global_73484 = prim;
}

/* 0x48b0 (TITLE 0x39b0): as 0x4330 with the corners first scaled by sx/sy (8.8); the -sin products are negated once and
   reused. Origin: 2026-10-10 draft (src/title/behavioral/emitters.c) plus the quadrant pointer q (the
   original spills &g_5e128[cell & 15] and rereads it for the next link). */
void eng1_048b0(unsigned char *stream, TitleObject *obj, unsigned char count, int sx, int sy)
{
    unsigned long tex;
    unsigned long flags;
    int dx;
    int x0;
    int y0;
    unsigned char hb;
    unsigned char flag;
    unsigned short cell;
    unsigned long attr;
    unsigned short clut;
    unsigned long *bucket;
    unsigned short flip;
    TitlePrim *prim;
    unsigned int u;
    unsigned int v;
    signed char *q;
    int g;
    int a;
    int w;
    int h;
    int A;
    int Vn;
    int B;
    int W;
    int sin2;
    int cos2;
    int sa;
    int ca;
    int sb;
    int cb;
    int sv;
    int cv;
    int sw;
    int cw;
    unsigned int off;

    tex = eng1_311a0(obj);
    flags = obj->unknown_054;
    dx = (sx * (signed char)*stream++) >> 8;
    if (flags & 0x10)
        dx = -dx;
    x0 = obj->unknown_00c + dx;
    y0 = ((sy * (signed char)*stream) >> 8) + obj->unknown_00e;
    hb = stream[1];
    cos2 = g_actor_stage_table_5a848[((short)obj->unknown_074 + 0x100) & 0x3ff] * 2;
    clut = ((obj->unknown_01e + 0xe0) << 6) | 0x30;
    attr = (flags & 3) << 5;
    bucket = &g_dispatch_frame_current_71ec4->ot[ENG1_OT_SIZE - 0x41 - obj->unknown_010];
    stream += 2;
    sin2 = g_actor_stage_table_5a848[(short)obj->unknown_074 & 0x3ff] * 2;
    flag = hb & 0x80;
    flip = (unsigned short)(flags & 0x10);
    prim = g_frame_global_73484;
    cell = obj->unknown_046;
    do {
        q = &g_5e128[cell & 0xf];
        g = *q;
        v = ((cell >> 8) * 2 + (g & 1)) << 4;
        u = ((cell >> 4) * 2 + (g >> 1)) << 4;
        if (flag) {
            v += (hb & 0xf) * 8;
            u += (hb >> 1) & 0x38;
        }
        a = (signed char)*stream++;
        w = (a & 0xf) << 3;
        h = (a >> 1) & ~7;
        prim->w_00c = (unsigned char)v + (u << 8);
        prim->w_014 = (unsigned char)(v + w - 1) + (u << 8);
        prim->w_01c = (unsigned char)v + ((u + h + 0xff) << 8);
        prim->w_024 = (unsigned char)(v + w - 1) + ((u + h + 0xff) << 8);
        if (flip) {
            A = -(signed char)*stream++ * 2;
            Vn = A - w;
        } else {
            A = (signed char)*stream++ * 2;
            Vn = A + w;
        }
        A *= sx;
        Vn *= sx;
        A >>= 8;
        sa = sin2 * A;
        ca = cos2 * A;
        B = (signed char)*stream++ * 2;
        W = h + B;
        B *= sy;
        W *= sy;
        B >>= 8;
        sb = -(sin2 * B);
        cb = cos2 * B;
        Vn >>= 8;
        sv = sin2 * Vn;
        cv = cos2 * Vn;
        W >>= 8;
        sw = -(sin2 * W);
        cw = cos2 * W;
        prim->xy_008 = (unsigned short)(((sb + ca) >> 16) + x0) + ((((cb + sa) >> 16) + y0) << 16);
        prim->xy_010 = (unsigned short)(((cv + sb) >> 16) + x0) + ((((sv + cb) >> 16) + y0) << 16);
        prim->xy_018 = (unsigned short)(((sw + ca) >> 16) + x0) + ((((sa + cw) >> 16) + y0) << 16);
        prim->xy_020 = (unsigned short)(((sw + cv) >> 16) + x0) + ((((sv + cw) >> 16) + y0) << 16);
        off = (cell >> 4) * sizeof(TitleCell);
        prim->w_016 = ((TitleCell *)((char *)g_72880 + off))->w_02 | attr;
        prim->tex_004 = tex;
        prim->w_00e = clut;
        prim->next_000 = (TitlePrim *)*bucket;
        *bucket = (unsigned long)prim;
        hb = *stream;
        prim++;
        flag = hb & 0x80;
        stream++;
        if (!flag)
            cell = ((TitleCell *)((char *)g_72880 + off))->next_04[*q];
    } while (--count);
    g_frame_global_73484 = prim;
}

void eng1_04c10(unsigned char *stream, TitleObject *obj, unsigned long count)
{
    unsigned long edx; unsigned long eax; unsigned long esi;
    obj->unknown_010 = obj->unknown_03e;
    eax = obj->unknown_03a;
    edx = eax;
    if (obj->unknown_054 & 0x80) {
        eax = obj->unknown_03c;
    }
    esi = eax | edx;
    if (esi == 0x100) {
        if (obj->unknown_074 != 0) {
            eng1_04330(stream, obj, (unsigned char)count);
        } else {
            eng1_04010(stream, obj, count);
        }
    } else {
        if (obj->unknown_074 != 0) {
            eng1_048b0(stream, obj, count, edx, eax);
        } else {
            eng1_04650(stream, obj, count, edx, eax);
        }
    }
}

/* 0x4ca0 (TITLE 0x3da0): place n sprite parts (4-byte headers at q, RLE pixel data after them) into free
   quadrants of the 0x2d340 cells; returns the first link (cell * 16 + quadrant bits). */
extern signed char g_5e120[4];
extern unsigned char g_actor_group_sequence_5e138[0x70];
extern unsigned char g_frame_decode_scratch_6e660[0x400];
void dispatch_engine_2c630(int a);
void dispatch_local_050d0(void);

unsigned short eng1_target_04ca0(unsigned char *q, int n, int m)
{
    unsigned char *src;
    unsigned short *link;
    int first;
    int cur;
    int cnt;
    int c;
    int quad;
    int i;
    int k;
    int pos;
    int bits;
    int slot;
    int rows;
    int w;
    int t;
    unsigned short mask;
    unsigned char *buf;
    unsigned short *dst;
    unsigned short *p;
    char b;
    unsigned int off;
    unsigned char *s;

    src = q + n * 4;
    link = 0;
    first = -1;
    cur = g_6ee78;
    cnt = g_6ee7c;
    slot = 0;
    do {
        c = (char)*q;
        if (!(c & 0x80)) {
            quad = ((c >> 1) & 1) + ((c >> 2) & 2);
            if (quad != 3) {
                i = 0;
                if (i < cnt) {
                next:
                    pos = i;
                    k = g_frame_upload_coordinates_734a0[i];
                    mask = g_72880[k].mask_00;
                    if (mask != 15) {
                        if (quad == 0)
                            goto hit0;
                        if (quad == 1) {
                            if (!(mask & 3))
                                goto hit3;
                            if (!(mask & 12))
                                goto hit12;
                        }
                        if (quad == 2) {
                            if (!(mask & 5))
                                goto hit5;
                            if (mask == 0)
                                goto hit9;
                        }
                    }
                    if (++i < cnt)
                        goto next;
                }
                goto miss;
            hit0:
                slot = 0;
                bits = 1;
                if (mask & bits) {
                    bits = 2;
                    slot = 1;
                    if (mask & bits) {
                        bits = 4;
                        slot = 2;
                        if (mask & bits) {
                            bits = 8;
                            slot = 3;
                        }
                    }
                }
                g_72880[k].mask_00 |= bits;
                goto found;
            hit3:
                bits = 3;
                slot = 0;
                g_72880[k].mask_00 |= bits;
                goto found;
            hit12:
                bits = 12;
                slot = 2;
                g_72880[k].mask_00 |= bits;
                goto found;
            hit5:
                bits = 5;
                slot = 0;
                g_72880[k].mask_00 |= bits;
                goto found;
            hit9:
                bits = 9;
                slot = 1;
                g_72880[k].mask_00 |= bits;
                goto found;
            miss:
                slot = 0;
            }
            bits = g_5e120[quad];
            if (m & 0x40) {
                for (i = 0x6f; i > 0x54; i--) {
                    if (g_72880[g_actor_group_sequence_5e138[i]].mask_00 == 0) {
                        cur = i;
                        goto got;
                    }
                }
            }
            for (t = 0; t < 0x54; t++) {
                if (++cur > 0x53)
                    cur = 0;
                if (g_72880[g_actor_group_sequence_5e138[cur]].mask_00 == 0)
                    goto got;
            }
            g_6ee78 = cur;
            g_6ee7c = cnt;
            k = first;
            while ((unsigned short)k != 0xffff) {
                off = (k >> 4) * sizeof(TitleCell);
                ((TitleCell *)((char *)g_72880 + off))->mask_00 &= 15 - (k & 15);
                k = ((TitleCell *)((char *)g_72880 + off))->next_04[g_5e128[k & 15]];
            }
            return 0xffff;
        got:
            if (cnt >= 0x20) {
                dispatch_engine_2c630(0);
                g_6ee7c = cnt;
                dispatch_local_050d0();
                dispatch_engine_2c630(0);
                cnt = 0;
            }
            k = g_actor_group_sequence_5e138[cur];
            g_frame_upload_coordinates_734a0[cnt] = g_actor_group_sequence_5e138[cur];
            pos = cnt++;
            g_72880[k].mask_00 = bits;
        found:
            if (link)
                *link = (k << 4) + bits;
            else
                first = (k << 4) + bits;
            link = &g_72880[k].next_04[slot];
            *link = 0xffff;
            c = (char)*q;
            w = (c & 3) * 2 + 2;
            rows = (c & 12) * 2 + 8;
            dst = (unsigned short *)(g_frame_owner_72870 + ((((slot & 2) + pos * 4) << 4) + (slot & 1) << 4));
            buf = g_frame_decode_scratch_6e660;
            s = src;
            for (;;) {
                b = *s++;
                if (b <= 0) {
                    *buf++ = b;
                    continue;
                }
                if (b == 2)
                    break;
                if (buf + (unsigned char)b > g_frame_decode_scratch_6e660 + sizeof(g_frame_decode_scratch_6e660)) {
                    if (cur > 0x53)
                        cur = 0;
                    g_6ee78 = cur;
                    g_6ee7c = cnt;
                    return first;
                }
                memset(buf, *s++, (unsigned char)b);
                buf += (unsigned char)b;
            }
            p = (unsigned short *)g_frame_decode_scratch_6e660;
            do {
                for (i = 0; i < w * 2; i++)
                    *dst++ = *p++;
                dst += (8 - w) * 2;
            } while (--rows != 0);
            src = s;
        }
        slot = 0;
        q += 4;
    } while (--n != 0);
    if (cur > 0x53)
        cur = 0;
    g_6ee7c = cnt;
    g_6ee78 = cur;
    return first;
}

void dispatch_local_050d0(void)
{
    int a;
    TitleRect rect;
    unsigned short *p;
    int cnt;
    int x, hi, lo;
    int ohi, olo;

    if (g_6ee7c == 0)
        return;
    a = g_frame_owner_72870;
    x = g_frame_upload_coordinates_734a0[0];
    hi = x / 16;
    lo = x & 15;
    p = g_frame_upload_coordinates_734a0;
    rect.w = 0x10;
    do {
        rect.x = (hi + 0x30) << 4;
        rect.y = lo << 5;
        cnt = 0x20;
        while (--g_6ee7c != 0) {
            ohi = hi;
            olo = lo;
            x = *++p;
            hi = x / 16;
            lo = x & 15;
            if ((hi - ohi) | (lo - olo - 1))
                break;
            cnt += 0x20;
        }
        rect.h = cnt;
        dispatch_engine_2c7f0(&rect, a);
        a += cnt << 5;
    } while (g_6ee7c != 0);
}


/* 0x51d0 (TITLE 0x42d0): release a context's two cell chains in the 0x2d340 cell table (12-byte cells:
   bit mask, word, four next links). A link k names cell k >> 4 and one quadrant bit k & 15;
   g_5e128 maps the bit to its link slot. The cell is addressed through a byte offset: only
   that spelling gives the explicit row*12 base register + table displacement the original
   uses here (and at the 0x33a7/0x36c9/0x392f/0x3c92/0x18145 loads); indexing g_72880[k >> 4] directly
   makes VC5 refactor the address as ((row*6 + j)*2) or fold the table into an lea. */

void release_actor_resource_state(TitleObject *p)
{
    unsigned short k;
    int last;
    int state;
    unsigned int off;
    TitleTabEntry *t;

    if (p->unknown_044 == 0xffff)
        return;
    state = g_5cee0[p->unknown_044].state_10;
    k = p->unknown_046;
    if (k != 0xffff) {
        last = 0;
        t = (TitleTabEntry *)g_5cee0[p->unknown_044].table_0c;
        if (t) {
            t += (short)p->unknown_040;
            if (--t->a_00 == 0)
                last = 1;
        }
        if (state == 0)
            last = 1;
        if (last) {
            do {
                off = (k >> 4) * sizeof(TitleCell);
                ((TitleCell *)((char *)g_72880 + off))->mask_00 &= 15 - (k & 15);
                k = ((TitleCell *)((char *)g_72880 + off))->next_04[g_5e128[k & 15]];
            } while (k != 0xffff);
        }
        p->unknown_046 = 0xffff;
    }
    k = p->unknown_048;
    if (k != 0xffff) {
        last = 0;
        t = (TitleTabEntry *)g_5cee0[p->unknown_044].table_0c;
        if (t) {
            t += (short)p->unknown_042;
            if (--t->a_00 == 0)
                last = 1;
        }
        if (state == 0)
            last = 1;
        if (last) {
            do {
                off = (k >> 4) * sizeof(TitleCell);
                ((TitleCell *)((char *)g_72880 + off))->mask_00 &= 15 - (k & 15);
                k = ((TitleCell *)((char *)g_72880 + off))->next_04[g_5e128[k & 15]];
            } while (k != 0xffff);
        }
        p->unknown_048 = 0xffff;
    }
    p->unknown_040 = p->unknown_034;
    p->unknown_044 = p->unknown_022;
}

void eng1_05310(TitleObject *p)
{
    if (p->unknown_044 == p->unknown_022 && p->unknown_040 == p->unknown_034 && p->unknown_042 == p->unknown_036)
        return;
    if (p->unknown_044 != p->unknown_022)
        release_actor_token(p);
    release_actor_resource_state(p);
    p->unknown_042 = p->unknown_036;
    p->unknown_040 = p->unknown_034;
    p->unknown_044 = p->unknown_022;
}
void clear_actor(TitleObject *p);
int eng1_03540(int idx, int flag);
void eng1_038d0(int i);
void engine_frame_chain_write(void *p, int n);
void dispatch_engine_2c7f0(void *p, int n);

/* 0x5370 (TITLE 0x4470, plus the ENG1 streaming branch): link the live objects of one pool (count records of stride bytes, walked from the
   last) into the per-frame lists: 0x800 objects onto g_frame_actor_list_734f8, 0x10000000 objects onto g_frame_owner_list_73490,
   the rest (once their slot data is resident) onto the draw list g_frame_resource_list_734e4, an attached
   0x800000 partner first or after it by its 0x4000000 flag; hidden objects are released. */
extern TitleObject *g_frame_resource_list_734e4;
extern TitleObject *g_frame_owner_list_73490;
extern TitleObject *g_frame_actor_list_734f8;
int ensure_actor_resource_ready(int idx, int arg2);

extern unsigned long g_6ee70;
extern int g_6eee4;
void eng1_02400(int a);

void eng1_05370(TitleObject *p, int count, int stride)
{
    TitleObject *q;
    unsigned long f;
    int slot;
    int state;

    p = (TitleObject *)((char *)p + (count - 1) * stride);
    while (count-- != 0) {
        if (p->unknown_02e != 0) {
            f = p->unknown_054;
            if ((f & 0x80000000) && ((f & 0x20000000) || p->unknown_034 != 0)) {
                if (f & 0x800) {
                    p->unknown_068 = g_frame_actor_list_734f8;
                    g_frame_actor_list_734f8 = p;
                } else if (f & 0x10000000) {
                    p->unknown_044 = p->unknown_022;
                    p->unknown_068 = g_frame_owner_list_73490;
                    g_frame_owner_list_73490 = p;
                } else {
                    slot = p->unknown_022;
                    eng1_05310(p);
                    if (slot != 0) {
                        state = g_5cee0[slot].state_10;
                        if (state == 3 || ensure_actor_resource_ready(p->unknown_022, p->unknown_034) != 0) {
                            if (f & 0x800000) {
                                q = p->unknown_060;
                                if (q != 0) {
                                    eng1_05310(q);
                                    q->unknown_000 = p->unknown_000 += p->unknown_014 << 16;
                                    q->unknown_004 = p->unknown_004 += p->unknown_016 << 16;
                                    q->unknown_008 = p->unknown_008 += p->unknown_018 << 16;
                                    if (!(q->unknown_054 & 0x4000000)) {
                                        q->unknown_068 = g_frame_resource_list_734e4;
                                        g_frame_resource_list_734e4 = q;
                                    }
                                    p->unknown_068 = g_frame_resource_list_734e4;
                                    g_frame_resource_list_734e4 = p;
                                    if (q->unknown_054 & 0x4000000) {
                                        q->unknown_068 = g_frame_resource_list_734e4;
                                        g_frame_resource_list_734e4 = q;
                                    }
                                }
                            } else {
                                p->unknown_068 = g_frame_resource_list_734e4;
                                g_frame_resource_list_734e4 = p;
                            }
                        } else if (state == 2 && (long)(g_engine_interface.context_004->tick_038 - g_6ee70) > 0x3c) {
                            if ((f & 0x40000000) == 0x40000000) {
                                p->unknown_00c = (short)(p->unknown_000 >> 16);
                                p->unknown_00e = (short)(p->unknown_004 >> 16);
                            } else if (eng1_320d0(p) == -1) {
                                goto next;
                            }
                            if ((unsigned short)p->unknown_00c < 0x140 && (unsigned short)p->unknown_00e < 0xf0 &&
                                g_6eee4 == 0 && g_engine_interface.context_004->unknown_000[4] == 0)
                                eng1_02400(0x80);
                        }
                    }
                }
            } else {
                clear_actor(p);
            }
        }
    next:
        p->unknown_01f = 1;
        p = (TitleObject *)((char *)p - stride);
    }
}
/* 0x55b0 (TITLE 0x4610) */
void clear_actor(TitleObject *p)
{
    unsigned int f = p->unknown_054;
    if (f & 0x10000800)
        return;
    release_actor_resource_state(p);
    release_actor_token(p);
    if (f & 0x800000) {
        TitleObject *q = p->unknown_060;
        if (q) {
            clear_actor(q);
            release_actor_token(q);
        }
    }
}

/* 0x5600 (TITLE 0x4660) */
/* Level record (28 bytes) at ENG1 0x5df08; +6 is the level's primitive-pool start (in records). */
typedef struct Eng1Level {
    unsigned char unknown_00[6];
    unsigned short prim_06;
    unsigned char unknown_08[0x1c - 8];
} Eng1Level;
extern Eng1Level g_frame_config_5df08[];

void dispatch_local_05600(void)
{
    engine_frame_chain_write(g_dispatch_frame_current_71ec4->ot, ENG1_OT_SIZE);
    if (g_dispatch_frame_current_71ec4 == g_dispatch_frame_base_71ec8)
        g_frame_global_73484 = (TitlePrim *)g_frame_owner_70c80 + g_frame_config_5df08[(signed char)g_engine_interface.context_004->unknown_000[0]].prim_06;
    else
        g_frame_global_73484 = (TitlePrim *)g_frame_owner_70c80;
}

/* 0x5660 (TITLE 0x46b0) */
unsigned char dispatch_local_05660(unsigned short key, unsigned int f)
{
    unsigned int flag = ((f & 0x80) | 2) >> 1;
    TitleTabEntry *p;
    int i;
    for (p = g_727e0, i = 0; i < 0x20; p++, i++) {
        if (p->b_02 == key) {
            p->a_00++;
            return (unsigned char)i;
        }
    }
    for (p = g_727e0, i = 0; i < 0x20; p++, i++) {
        if (p->a_00 == 0) {
            g_727e0[i].b_02 = key;
            g_727e0[i].a_00 = (unsigned short)flag;
            return (unsigned char)(i | 0x80);
        }
    }
    return (unsigned char)(i | 0xff);
}

unsigned char dispatch_local_056c0(char *p, int b, int c, int d)
{
    unsigned int v = dispatch_local_05660(b, d);
    if ((v & 0x80) == 0x80) {
        v &= 0x7f;
        dispatch_local_05780((int)p, v, c);
    }
    return v;
}

/* 0x5710 (TITLE 0x4760) */
void dispatch_local_05710(void)
{
    TitleTabEntry *p = g_727e0;
    int i = 0x20;
    do {
        p->b_02 = 0xffff;
        p->a_00 = 0;
        p++;
    } while (--i != 0);
    g_frame_setup_result_734ec = dispatch_local_056c0((char *)g_frame_seed_words_4d818, 0x7641, 0x20, 5);
}

/* 0x5750 (TITLE 0x47a0) */
TitleTabEntry *release_actor_token(TitleObject *c)
{
    TitleTabEntry *entry;
    if (c->unknown_01e != 0xff) {
        entry = &g_727e0[c->unknown_01e];
        entry->a_00--;
        c->unknown_01e = 0xff;
        c->unknown_01d = 0xff;
        return entry;
    }
}

/* 0x5780 (TITLE 0x47d0) */
void dispatch_local_05780(unsigned short *src, int row, int n)
{
    unsigned short *dst;
    int i;
    dst = (unsigned short *)(g_frame_owner_7348c + (row << 9));
    if (n != 0x100) {
        *dst++ = 0;
        for (i = 1; i < n; i++)
            *dst++ = src[i] | 0x8000;
    } else {
        for (i = 0x100; i != 0; i--)
            *dst++ = *src++;
    }
    if (n != 0x100 && n < 0x100) {
        i = 0x100 - n;
        do {
            *dst = dst[-n];
            dst++;
        } while (--i != 0);
    }
    g_frame_global_73498 = 1;
}

/* 0x5820: as 0x5780 for a 32-entry palette row: entry 0 transparent, the rest with the STP bit;
   other counts are copied as they are, then the row is repeated up to 256 entries. */
void eng1_05820(unsigned short *src, int row, int n)
{
    unsigned short *dst;
    int i;
    dst = (unsigned short *)(g_frame_owner_7348c + (row << 9));
    if (n == 0x20) {
        *dst++ = 0;
        for (i = 1; i < 0x20; i++)
            *dst++ = src[i] | 0x8000;
    } else {
        for (i = 0; i < n; i++)
            *dst++ = *src++;
    }
    if (n != 0x100 && n < 0x100) {
        i = 0x100 - n;
        do {
            *dst = dst[-n];
            dst++;
        } while (--i != 0);
    }
    g_frame_global_73498 = 1;
}


/* 0x58b0 (TITLE 0x4870) */
void dispatch_local_058b0(void)
{
    if (g_frame_global_73498) {
        dispatch_engine_2c7f0(&g_4d810, g_frame_owner_7348c);
        g_frame_global_73498 = 0;
    }
}

/* 0x58e0 / 0x59f0: bracket an object drawn with flag 0x10000 by a drawing-area change. 0x58e0 links a
   DR_AREA-style primitive for the object's clip band into the ordering-table entry; 0x59f0 restores the
   area from the second rectangle. */
extern int g_70c64;
extern TitleRect g_4d800[2];
void dispatch_engine_2c5c0(unsigned long *ot, TitlePrim *prim);
void dispatch_engine_2c690(TitleRect *r);
void dispatch_engine_2c6e0(void);
void dispatch_engine_2c760(TitleRect *r);
void dispatch_engine_2c960(TitlePrim *prim, TitleRect *r);

void eng1_058e0(TitleObject *p, unsigned long *ot)
{
    TitleRect *area;
    TitleRect r;
    int y;

    area = (TitleRect *)g_engine_interface.data_010;
    area->y = (short)(p->unknown_004 >> 16) - p->unknown_026 - g_70c64;
    dispatch_engine_2c690(area);
    dispatch_engine_2c6e0();
    y = p->unknown_00e;
    dispatch_engine_2c5c0(ot, g_frame_global_73484);
    dispatch_engine_2c760(&r);
    if (p->unknown_054 & 0x20000) {
        if (p->unknown_054 & 0x40000)
            p->unknown_00e += y - r.y;
        else
            y = r.y;
        if (y < 0)
            y = 0;
        else if (y > 0xef)
            y = 0xef;
        g_4d800[1].h = y;
        g_4d800[1].y = g_4d800[0].y;
    } else {
        y = r.y;
        if (y < 0)
            y = 0;
        else if (y > 0xef)
            y = 0xef;
        g_4d800[1].y = y + g_4d800[0].y;
        g_4d800[1].h = g_4d800[0].h - y;
    }
    dispatch_engine_2c960(g_frame_global_73484, g_4d800);
    g_frame_global_73484 = (TitlePrim *)((char *)g_frame_global_73484 + 12);
}

void eng1_059f0(TitleObject *p, unsigned long *ot)
{
    dispatch_engine_2c960(g_frame_global_73484, &g_4d800[1]);
    dispatch_engine_2c5c0(ot, g_frame_global_73484);
    g_frame_global_73484 = (TitlePrim *)((char *)g_frame_global_73484 + 12);
}

/* ---- ENG1 colour fade group 0x5a60..0x5e00 (fade level g_frame_word_6ee84, packed RGB words) ---- */
extern short g_frame_word_6ee84;
extern short g_dispatch_word_6ee80;
extern int g_dispatch_setting_4d858;
extern signed char g_dispatch_key_7286c;
typedef struct Eng1Rgb {
    unsigned char r, g, b, cd;
} Eng1Rgb;
extern Eng1Rgb g_packed_byte_70cf0;
extern unsigned long g_frame_global_70cf4;
extern unsigned long g_frame_global_70cf8;
extern unsigned char g_frame_global_70cfc;
extern unsigned long g_mesh_colors_6f4e8[8][64];
extern unsigned long g_73488;
extern unsigned long g_734f0;
void eng1_frame_color_write_5e00(int level);
void eng1_color_profile_5dc0(int level);
void dispatch_engine_2c830(unsigned char level);

/* 0x5a60: base colour (r0, g0, b0) plus per-channel rates scaled by the fade level; builds the current
   colour and the 8 x 64 table of per-step colours (bank bit 0/1/2 selects the stepped r/g/b). */
void eng1_color_grid_update_5a60(int r0, int g0, int b0, int rr, int rg, int rb)
{
    int r;
    int g;
    int b;
    int ar;
    int ag;
    int ab;
    int tr;
    int tg;
    int tb;
    unsigned long c;
    int i;

    rr = (rr << 1) * g_frame_word_6ee84;
    rg = (rg << 1) * g_frame_word_6ee84;
    rb = (rb << 1) * g_frame_word_6ee84;
    g_frame_global_70cf8 = (rr >> 8) + ((rg >> 8) << 8) + ((rb >> 8) << 16);
    r = ((rr * g_packed_byte_70cf0.r) >> 15) + r0;
    if (r > 0xff)
        r = 0xff;
    g = ((rg * g_packed_byte_70cf0.g) >> 15) + g0;
    if (g > 0xff)
        g = 0xff;
    b = ((rb * g_packed_byte_70cf0.b) >> 15) + b0;
    if (b > 0xff)
        b = 0xff;
    g_frame_global_70cfc = (r + g + b) / 3;
    g <<= 8;
    b <<= 16;
    c = r + g + b;
    if (c == g_frame_global_70cf4)
        return;
    g_frame_global_70cf4 = c;
    ar = 0;
    ag = 0;
    ab = 0;
    for (i = 0; i < 64; i++) {
        tr = (ar >> 13) + r0;
        if (tr > 0xff)
            tr = 0xff;
        tg = (ag >> 13) + g0;
        if (tg > 0xff)
            tg = 0xff;
        tb = (ab >> 13) + b0;
        if (tb > 0xff)
            tb = 0xff;
        tg <<= 8;
        tb <<= 16;
        g_mesh_colors_6f4e8[0][i] = r + g + b;
        g_mesh_colors_6f4e8[1][i] = tr + g + b;
        g_mesh_colors_6f4e8[2][i] = r + tg + b;
        g_mesh_colors_6f4e8[3][i] = tr + tg + b;
        g_mesh_colors_6f4e8[4][i] = r + g + tb;
        g_mesh_colors_6f4e8[5][i] = tr + g + tb;
        g_mesh_colors_6f4e8[6][i] = r + tg + tb;
        g_mesh_colors_6f4e8[7][i] = tr + tg + tb;
        ar += rr;
        ag += rg;
        ab += rb;
    }
    eng1_frame_color_write_5e00(g_frame_word_6ee84);
}

/* 0x5c70: blend two 15-bit palettes (a toward b by t/256, rounded) into palette row 'row'; entry 0 is
   transparent, the rest get the STP bit. */
void eng1_05c70(unsigned short *a, unsigned short *b, int row, int n, unsigned char t)
{
    unsigned short *dst;
    int i;
    int ca;
    int cb;
    int r;
    int g;
    int bl;

    dst = (unsigned short *)(g_frame_owner_7348c + (row << 9));
    *dst++ = 0;
    for (i = 1; i < n; i++) {
        ca = *++a;
        cb = *++b;
        r = ca & 0x1f;
        g = (ca >> 5) & 0x1f;
        bl = (ca >> 10) & 0x1f;
        r += (((cb & 0x1f) - r) * t + 0x80) >> 8;
        g += ((((cb >> 5) & 0x1f) - g) * t + 0x80) >> 8;
        bl += ((((cb >> 10) & 0x1f) - bl) * t + 0x80) >> 8;
        *dst++ = (((bl << 5) + g) << 5) + r | 0x8000;
    }
    g_frame_global_73498 = 1;
}

/* 0x5d50: step the fade level by g_dispatch_key_7286c, stopping at g_dispatch_setting_4d858 (rising) or g_dispatch_word_6ee80. */
void eng1_color_fade_step_5d50(void)
{
    if (g_dispatch_key_7286c != 0) {
        g_frame_word_6ee84 += g_dispatch_key_7286c;
        if (g_dispatch_key_7286c > 0 && (unsigned short)g_frame_word_6ee84 > (unsigned short)g_dispatch_setting_4d858) {
            g_frame_word_6ee84 = g_dispatch_setting_4d858;
            g_dispatch_key_7286c = 0;
        } else if (g_frame_word_6ee84 < g_dispatch_word_6ee80) {
            g_frame_word_6ee84 = g_dispatch_word_6ee80;
            g_dispatch_key_7286c = 0;
        }
        eng1_color_profile_5dc0(g_frame_word_6ee84);
        dispatch_engine_2c830(g_frame_word_6ee84);
    }
}

void eng1_color_profile_5dc0(int level)
{
    g_73488 = 0x2c000000 + level * 0x10101;
    eng1_color_grid_update_5a60(0, 0, 0, 0x80, 0x80, 0x80);
}

