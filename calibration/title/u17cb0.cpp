#include "title_engine.h"

extern "C" {
#include "title_gpu.h"


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
typedef struct TitleObj {
    TitleXY pos_000;
    unsigned char unknown_008[0x0c - 0x08];
    unsigned short w_0c;
    unsigned short w_0e;
    short w_10;
    unsigned char unknown_012[0x1f - 0x12];
    unsigned char flags_01f;
    unsigned char byte_020;
    unsigned char b_021;
    unsigned char b_022;
    unsigned char byte_023;
    unsigned char unknown_024[0x2c - 0x24];
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
} TitleObj;
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
typedef struct TitleSlot {
    int unknown_000;
    int unknown_004;
    int unknown_008;
    unsigned char unknown_00c[0x23 - 0x0c];
    unsigned char unknown_023;
    unsigned char unknown_024[0x34 - 0x24];
    unsigned short unknown_034;
    unsigned char unknown_036[0x3e - 0x36];
    unsigned short unknown_03e;
    unsigned char unknown_040[0x4a - 0x40];
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x54 - 0x4c];
    int unknown_054;
    unsigned char unknown_058[0x74 - 0x58];
    unsigned short unknown_074;
} TitleSlot;

/* Initialized data of this unit (TITLE.DLL .data 0x260c0..0x26277), in address order, with the slot
   paths as its literals (emitted after the data, in reverse order). */
int g_260c0[8] = {8192, 0, 536870912, -536870912, 0, 0, 0, 0};
int g_260e0[4] = {8192, 8192, 8192, 0};
void title_18f80(int a1, int *vals, int count);
void title_19040(TitleStream *s, int *vals, int n);
void title_190d0(TitleStream *s);
void title_19120(TitleObj *o, int *vals, int count);
void title_19270(TitleObj *o, int *vals, int count);
void *g_260f0[8] = {  /* handler table, NULL-terminated */
    (void *)title_18f80, (void *)title_18f80, (void *)title_19040, (void *)title_19120, (void *)title_18f80, (void *)title_190d0, (void *)title_19270, 0,
};
extern char g_29128[];
TitleRecord g_26110[18] = {
    {g_29128, 0},
    {"M:\\language\\grafix\\chop\\title\\menu1", 257},
    {"M:\\language\\grafix\\chop\\title\\optns", 165},
    {"M:\\language\\grafix\\chop\\title\\pswd", 579},
    {"M:\\language\\grafix\\chop\\title\\seq1", 16},
    {"M:\\language\\grafix\\chop\\title\\seq2", 16},
    {"M:\\language\\grafix\\chop\\title\\seq3", 16},
    {"M:\\language\\grafix\\chop\\title\\seq4", 16},
    {"M:\\language\\grafix\\chop\\title\\seq5", 16},
    {"M:\\language\\grafix\\chop\\title\\seq6", 16},
    {"M:\\language\\grafix\\chop\\title\\seq7", 16},
    {"M:\\language\\grafix\\chop\\title\\seq8", 16},
    {"M:\\language\\grafix\\chop\\title\\tallyup", 828},
    {"M:\\language\\grafix\\chop\\title\\seq9", 16},
    {"M:\\language\\grafix\\chop\\title\\joypad", 1},
    {"M:\\language\\grafix\\chop\\title\\seq10", 16},
    {"M:\\language\\grafix\\chop\\title\\lsgame", 33},
    {"M:\\language\\grafix\\chop\\title\\cheats", 29},
};
int title_04d80(int index, int key);
extern TitlePair8 g_2b720[];
extern TitlePair8 *g_2bb20;
extern signed char g_2cc02;
extern char *g_2cbf0;
int title_18a90(TitleObj *o);
void title_18b20(TitleObj *o);
void title_19680(TitleRec *rec);
void title_196f0(TitleObj *o, int v);
void title_0c7b0(void);
void title_0c810(void *e);
void title_0c840(void *b);
void title_0c7e0(void *p);
unsigned long make_colour(void *p);
void title_18bf0(TitleObj *o, unsigned char *q, unsigned long c);
int title_19570(TitleShape *obj, int k);
extern int g_2cbe8;
extern int g_2b1e8[];
extern int g_2aea8[];
extern int g_2ad00[];
extern int g_2adc8[];
extern int g_2b1d0;
extern TitleSlot *g_2b240[];
void title_09350(void *p);
short title_0c4a0(int a);
void title_0c760(void *p);
void title_0c7f0(void *p);
void title_0c810(void *p);
void title_0c850(void *p);
int title_196c0(void *unused);

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

extern TitleObj *g_2dfa0;
extern TitleTile *g_2df40;
int title_195f0(TitleObj *o);
void title_0c790(void);
void title_02cc0(TitleObj *o);
void title_02dc0(TitleObj *o);
int title_19470(char *obj, int *vec);
int title_194e0(char *obj, int *vec, int k);

void title_17cb0(TitleObj *obj)
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
    if (g_26110[obj->b_022].type_10 != 3) {
        if (title_04d80(obj->b_022, obj->w_034) == 0) {
            return;
        }
    }
    r = title_195f0(obj);
    if (r == -1) {
        obj->flags_01f |= 8;
        return;
    }
    title_196f0(obj, r);
    title_0c790();
    span = (TitleSpan *)obj->stream_05c - 1;
    count = obj->w_036;
    g_2dfa0->byte_023 = 0;
    g_2dfa0->w_02c = obj->w_02c;
    acc = 0;
    g_2dfa0->b_022 = obj->b_022;
    g_2dfa0->w_04a = obj->w_04a;
    if (flags & 0x100000) {
        g_2dfa0->rot_070 = obj->rot_070;
    } else {
        g_2dfa0->rot_070.vx = g_2dfa0->rot_070.vy = g_2dfa0->rot_070.vz = 0;
    }
    z = obj->rot_070.vz;
    ot = &g_2cc04->ot[TITLE_OT_SIZE - 2 - obj->w_10];
    while (count--) {
        span++;
        if (span->flags_00e & 0x8000) {
            title_19470((char *)xy, (int *)span);
            g_2df40->xy_008 = xy[0];
            *(unsigned long *)&g_2df40->r0 = 0xffffff;
            g_2df40->code = 0x68;
            g_2df40->next_000 = (unsigned long *)*ot;
            *ot = (unsigned long)g_2df40;
            g_2df40++;
        } else if (span->id_00c != 0) {
            if (flags & 0x200000) {
                if (flags & 0x400000) {
                    title_0c790();
                    acc += span->z_008;
                } else {
                    acc = span->z_008;
                }
                g_2dfa0->rot_070.vz = z - acc;
                title_194e0((char *)&g_2dfa0->w_0c, (int *)span, acc);
            } else {
                if (flags & 0x400000) {
                    title_0c790();
                }
                title_19470((char *)&g_2dfa0->w_0c, (int *)span);
            }
            id = span->id_00c;
            e = (unsigned short *)g_26110[g_2dfa0->b_022].table_0c + (short)(id * 2);
            if (e[0] != 0) {
                g_2dfa0->w_034 = id;
                g_2dfa0->flags_054 = (obj->flags_054 & 0xffff0000) | span->flags_00e;
                if ((g_2dfa0->flags_054 & 0x600) == 0x600) {
                    g_2dfa0->pos_000.x = obj->pos_000.x + span->pos_000.x;
                    g_2dfa0->pos_000.y = span->pos_000.y + obj->pos_000.y;
                }
                g_2dfa0->w_046 = e[1];
                title_02cc0(g_2dfa0);
                title_02dc0(g_2dfa0);
            }
        }
    }
}

struct title_pos {
    short x;
    short y;
};
unsigned char *title_18680(int index, int key);

int title_18600(TitleObj *o, struct title_pos *pos)
{
    unsigned char *p;
    int n;

    pos->y = 0;
    pos->x = 0;
    if (o->w_034 > g_26110[o->b_022].count_04) {
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
    if (g_26110[index].type_10 != 3) {
        if (title_04d80(index, key) == 0) {
            return 0;
        }
    }
    rec = &g_26110[index];
    entries = (int *)(rec->table_0c + rec->count_04 * 4 + 8);
    value = entries[(short)key];
    if (value == 0) {
        return 0;
    }
    return (unsigned char *)entries + (value >> 8);
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
            title_19680((TitleRec *)arg);
            if (arg->flags_054 & 0x20000000) {
                list = arg->list_120;
                count = *list;
                list++;
                if (count-- != 0) {
                    k = count + 1;
                    do {
                        title_19680((TitleRec *)*list++);
                    } while (--k);
                }
            }
            arg = arg->next_068;
        } while (arg != (TitleObj *)-1);
    }
    o = first;
    g_2bb20 = g_2b720;
    if (first != (TitleObj *)-1) {
        do {
            if (o->flags_054 & 0x20000000) {
                title_18b20(o);
            } else {
                o->flags_01f |= 8;
                if (o->flags_01f) {
                    o->byte_020 = (unsigned char)(g_2bb20 - g_2b720);
                    title_18a90(o);
                }
            }
            o = o->next_068;
        } while (o != (TitleObj *)-1);
    }
}

int title_18a90(TitleObj *o)
{
    unsigned char *q;
    int idx;

    if ((unsigned int)(o->w_10 - 1) > 0x47e) {
        return 0;
    }
    if ((unsigned int)o->w_034 > (unsigned int)g_2cc02) {
        return 0;
    }
    o->b_022 = 0;
    idx = o->w_034;
    q = (unsigned char *)(g_2cbf0 + *(int *)(g_2cbf0 + idx * 4));
    q += idx * 4;
    if (o->w_0c > 0x140 || o->w_0e > 0xf0) {
        if (title_19570((TitleShape *)o, q[3]) != 0) {
            return 0;
        }
    }
    title_196f0(o, o->rot_070.pad);
    title_18bf0(o, q, make_colour(o));
    return 0;
}

void title_18f80(int a1, int *vals, int count)
{
    int pair[2];
    unsigned char *dst;
    int v;

    dst = (unsigned char *)g_engine_interface.data_010 + 0x10;
    v = *vals;
    vals++;
    pair[0] = v >> 8;
    pair[1] = (signed char)v;
    title_0c760(pair);
    title_0c7b0();
    v = *vals;
    vals++;
    pair[0] = v >> 8;
    pair[1] = (signed char)v;
    dst += 8;
    if (--count == 0) {
        goto tail;
    }
    do {
        title_0c810(dst);
        title_0c840(dst + 4);
        title_0c760(pair);
        title_0c7b0();
        v = *vals;
        vals++;
        pair[0] = v >> 8;
        pair[1] = (signed char)v;
        dst += 8;
    } while (--count);
tail:
    title_0c810(dst);
    dst += 4;
    title_0c840(dst);
}

void title_19040(TitleStream *s, int *vals, int n)
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
        title_0c760(pair);
        title_0c7b0();
        d1 = dst + p[1];
        d1->a_00 = d0->a_00;
        p += 2;
        d1->b_04 = d0->b_04;
        title_0c810(d0);
        title_0c840(&d0->b_04);
    } while (--n);
}

void title_190d0(TitleStream *s)
{
    TitlePair8 *dst = (TitlePair8 *)((char *)g_engine_interface.data_010 + 0x18);
    unsigned char *p = s->stream_05c;
    unsigned int n = *p++;
    do {
        unsigned int di = p[0];
        unsigned int src = p[1];
        TitlePair8 *d = dst + di;
        TitlePair8 *e = (TitlePair8 *)g_2b720 + src;
        p += 2;
        d->a_00 = e->a_00;
        d->b_04 = e->b_04;
    } while (--n);
}

extern int *g_2bb28;

void title_19120(TitleObj *o, int *vals, int count)
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
        title_18f80((int)o, vals, count);
        return;
    }
    if (g_2bb28 != 0) {
        src = g_2bb28;
    } else {
        q = (unsigned char *)(g_2cbf0 + *(int *)(g_2cbf0 + o->w_036 * 4));
        q += o->w_036 * 4 + 2;
        src = (int *)(q + *q * 8 + 0xa);
    }
    dst = (char *)g_engine_interface.data_010 + 0x10;
    dst4 = dst + 4;
    do {
        a = *vals++;
        b = *src++;
        mid = (a << 8) >> 16;
        title_0c810(dst);
        title_0c840(dst4);
        dst4 += 8;
        hi = (((b >> 24) - (a >> 24)) * t >> 8) + (a >> 24);
        pair[0] = ((((b << 8) >> 16) - mid) * t >> 8) + mid + (hi << 16);
        dst += 8;
        pair[1] = (((signed char)b - (signed char)a) * t >> 8) + (signed char)a;
        title_0c760(pair);
        title_0c7b0();
    } while (--count);
    title_0c810(dst);
    title_0c840(dst + 4);
    g_2bb28 = src + 1;
}

void title_19270(TitleObj *o, int *vals, int count)
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

    if (g_2bb28 != 0) {
        src = g_2bb28;
    } else {
        q = (unsigned char *)(g_2cbf0 + *(int *)(g_2cbf0 + o->w_036 * 4));
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
            *dst = g_2b720[c].a_00;
            *dst4 = g_2b720[c].b_04;
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
            title_0c760(pair);
            title_0c7b0();
            title_0c810(dst);
            title_0c840(dst4);
        }
        dst += 2;
        dst4 += 2;
        i++;
    } while (i != count);
    g_2bb28 = src + 1;
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
    title_0c7e0(v);
}

int title_19470(char *obj, int *vec)
{
    short v[3];

    v[0] = (short)(vec[0] >> 16);
    v[1] = (short)(vec[1] >> 16);
    v[2] = (short)(vec[2] >> 16);
    title_0c760(v);
    title_0c7b0();
    title_0c810(obj);
    obj += 4;
    title_0c850(obj);
    return *(short *)obj > 0x40;
}

int title_194e0(char *obj, int *vec, int k)
{
    short v[3];

    v[0] = (short)((((short *)g_engine_interface.data_014)[(k * 4 - 0x400) & 0xfff] * (short)(vec[0] >> 16)) >> 12);
    v[1] = (short)((((short *)g_engine_interface.data_014)[(k & 0x3ff) * 4] * (short)(vec[0] >> 16)) >> 12);
    v[2] = 0;
    title_0c760(v);
    title_0c7b0();
    title_0c810(obj);
    obj += 4;
    title_0c850(obj);
    return *(short *)obj > 0x40;
}

int title_19570(TitleShape *obj, int k)
{
    int h = g_2cbe8 >> 1;
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

extern int g_2cbe0;
extern int g_2cbe4;

typedef struct TitleBox {
    int unknown_000;
    int unknown_004;
    int unknown_008;
    short unknown_00c;
    short unknown_00e;
    short unknown_010;
} TitleBox;

int title_195f0(TitleObj *o)
{
    TitleBox *box = (TitleBox *)o;
    int r = title_196c0(box);
    int d = 0x400 - (box->unknown_008 >> 16);

    title_0c810(&box->unknown_00c);
    title_0c850(&box->unknown_010);
    box->unknown_00c = (short)(((box->unknown_000 >> 16) * d >> 10) + g_2cbe0);
    box->unknown_00e = (short)(((box->unknown_004 >> 16) * d >> 10) + g_2cbe4);
    box->unknown_010 = (short)((box->unknown_008 >> 16) + (g_2cbe8 >> 1));
    return r;
}

void title_19680(TitleRec *rec)
{
    rec->unknown_076 = (unsigned short)title_196c0(rec);
    title_0c810(rec->unknown_00c);
    title_0c7f0(rec->unknown_078);
    rec->unknown_010 = (short)(rec->unknown_080 / 4);
}

int title_196c0(void *unused)
{
    unsigned short *p = (unsigned short *)g_engine_interface.data_010;

    p[0] = 0;
    p[2] = 0;
    p[1] = 0x80;
    title_0c760(p);
    title_0c7b0();
    return 0;
}


void title_0ca40(void *a, void *b);
void title_0ca60(void *a, void *b);
void title_0c7d0(void *p);

void title_196f0(TitleObj *o, int v)
{
    short src[3];
    int scale[4];
    int m[8];
    int s;
    unsigned short z, y, x;

    src[0] = (short)(o->rot_070.vx << 2);
    src[1] = (short)(o->rot_070.vy << 2);
    src[2] = (short)(o->rot_070.vz << 2);
    title_0ca40(src, m);
    if (o->flags_054 & 0x80) {
        z = o->w_03e;
        y = o->w_03c;
        x = o->w_03a;
        if ((x | y | z) != 0x100) {
            scale[0] = x << 5;
            scale[1] = y << 5;
            scale[2] = z << 5;
        }
        title_0ca60(m, scale);
        title_0c7d0(m);
    } else if (o->w_03a != 0x100) {
        s = o->w_03a << 5;
        scale[2] = s;
        scale[1] = s;
        scale[0] = s;
        title_0ca60(m, scale);
        title_0c7d0(m);
    } else {
        title_0ca60(m, g_260e0);
        title_0c7d0(m);
    }
}

} /* extern "C" */
