/* TITLE unit 0x2b70..0x4b70 (C): one object by its private .bss (0x29590..0x29da0, shared by 0x2b70,
   0x3da0, 0x41d0, 0x48a0, 0x4b70) and its .data (0x220d0..0x2214b with 0x4b70's literals last).
   Consolidated from the region files r02a80.c, r03d10.c and r04610.c; functions in address order. */
#include "title_engine.h"
#include "title_screen.h"
#include "title_slots.h"
#include "title_gpu.h"

/* Object passed as the second argument of 0x3420/0x3740/0x39b0/0x3d10. */
/* 40-byte primitive record taken from the pool at g_2df40. */
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

extern TitlePrim *g_2df40;

unsigned long title_189a0(TitleObject *obj);
void title_03100(unsigned char *stream, TitleObject *obj, unsigned long count);
void title_03420(unsigned char *stream, TitleObject *obj, unsigned char count);
void title_03740(unsigned char *stream, TitleObject *obj, unsigned long count, unsigned long y, unsigned long x);
void title_039b0(unsigned char *stream, TitleObject *obj, unsigned long count, unsigned long y, unsigned long x);

extern int g_2d328;
extern int (*g_220d0[])(TitleObject *);

extern int g_2cbe8;
void title_04470(int a, int b, int c);
int title_189e0(int a);
int title_195f0(TitleObject *p);
void title_01dd0(TitleObject *p, unsigned char *q);
void title_02cc0(TitleObject *p);
void title_02dc0(TitleObject *p);

typedef struct TitleTabEntry { unsigned short a_00; unsigned short b_02; } TitleTabEntry;
extern TitleTabEntry g_2d2a0[];
extern unsigned char g_2df50;
extern int g_2df44;
extern char g_22100[];
extern char g_22108[];
extern unsigned char g_2dfac;
extern int g_2bb24;
unsigned char title_04710(char *p, int b, int c, int d);
unsigned char title_046b0(unsigned short key, unsigned int f);
void title_047d0(int a, int v, int b);
void title_042d0(TitleObject *p);
TitleTabEntry *title_047a0(TitleObject *c);
extern int g_29d94;
extern int g_2d32c;
extern unsigned short g_2df60[0x20];
void title_0c890(void *p, int n);

void title_02cc0(TitleObject *p)
{
    unsigned long flags;
    unsigned long sel;
    unsigned long saved34;
    unsigned long saved46;

    flags = p->unknown_054;
    sel = flags & 0x10000;
    if (sel) {
        title_01dd0(p, (unsigned char *)&g_2cc04->ot[TITLE_OT_SIZE - 2] - (p->unknown_010 << 2));
    }
    if ((flags & 0x8000000) && p->unknown_036 != 0) {
        if (!(flags & 0x4000000)) {
            title_02dc0(p);
            g_2d328 = 0;
        }
        saved34 = p->unknown_034;
        saved46 = p->unknown_046;
        p->unknown_034 = p->unknown_036;
        p->unknown_046 = p->unknown_048;
        title_02dc0(p);
        g_2d328 = 0;
        p->unknown_048 = p->unknown_046;
        p->unknown_034 = saved34;
        p->unknown_046 = saved46;
        if (flags & 0x4000000) {
            title_02dc0(p);
        }
        g_2d328 = 1;
    } else {
        title_02dc0(p);
    }
    if (sel) {
        title_01dd0(p, (unsigned char *)&g_2cc04->ot[TITLE_OT_SIZE - 2] - (p->unknown_010 << 2));
    }
}

void title_03050(int a1, TitleObject *p, int a3)
{
    long v1;
    long v2;
    short d;
    long half;

    d = p->unknown_010;
    if (d <= 0)
        return;
    half = g_2cbe8 >> 1;
    v1 = v2 = (long)p->unknown_03a * half / d;
    if (p->unknown_054 & 0x80)
        v2 = (long)p->unknown_03c * half / d;
    if ((v2 | v1) == 0x100) {
        if (p->unknown_074 != 0)
            title_03420(a1, p, a3);
        else
            title_03100(a1, p, a3);
        return;
    }
    if (p->unknown_074 != 0)
        title_039b0(a1, p, a3, v1, v2);
    else
        title_03740(a1, p, a3, v1, v2);
}

void title_03d10(unsigned char *stream, TitleObject *obj, unsigned long count)
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
            title_03420(stream, obj, (unsigned char)count);
        } else {
            title_03100(stream, obj, count);
        }
    } else {
        if (obj->unknown_074 != 0) {
            title_039b0(stream, obj, count, edx, eax);
        } else {
            title_03740(stream, obj, count, edx, eax);
        }
    }
}

void title_041d0(void)
{
    int a;
    TitleRect rect;
    unsigned short *p;
    int cnt;
    int x, hi, lo;
    int ohi, olo;

    if (g_29d94 == 0)
        return;
    a = g_2d32c;
    x = g_2df60[0];
    hi = x / 16;
    lo = x & 15;
    p = g_2df60;
    rect.w = 0x10;
    do {
        rect.x = (hi + 0x30) << 4;
        rect.y = lo << 5;
        cnt = 0x20;
        while (--g_29d94 != 0) {
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
        title_0c890(&rect, a);
        a += cnt << 5;
    } while (g_29d94 != 0);
}


/* 0x42d0: release a context's two cell chains in the 0x2d340 cell table (12-byte cells:
   bit mask, word, four next links). A link k names cell k >> 4 and one quadrant bit k & 15;
   g_264f8 maps the bit to its link slot. The cell is addressed through a byte offset: only
   that spelling gives the explicit row*12 base register + table displacement the original
   uses here (and at the 0x33a7/0x36c9/0x392f/0x3c92/0x18145 loads); indexing g_2d340[k >> 4] directly
   makes VC5 refactor the address as ((row*6 + j)*2) or fold the table into an lea. */
typedef struct TitleCell {
    unsigned short mask_00;
    unsigned short w_02;
    unsigned short next_04[4];
} TitleCell;
extern TitleCell g_2d340[];
extern signed char g_264f8[];

void title_042d0(TitleObject *p)
{
    unsigned short k;
    int last;
    int state;
    unsigned int off;
    TitleTabEntry *t;

    if (p->unknown_044 == 0xffff)
        return;
    state = g_26110[p->unknown_044].state_10;
    k = p->unknown_046;
    if (k != 0xffff) {
        last = 0;
        t = (TitleTabEntry *)g_26110[p->unknown_044].table_0c;
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
                ((TitleCell *)((char *)g_2d340 + off))->mask_00 &= 15 - (k & 15);
                k = ((TitleCell *)((char *)g_2d340 + off))->next_04[g_264f8[k & 15]];
            } while (k != 0xffff);
        }
        p->unknown_046 = 0xffff;
    }
    k = p->unknown_048;
    if (k != 0xffff) {
        last = 0;
        t = (TitleTabEntry *)g_26110[p->unknown_044].table_0c;
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
                ((TitleCell *)((char *)g_2d340 + off))->mask_00 &= 15 - (k & 15);
                k = ((TitleCell *)((char *)g_2d340 + off))->next_04[g_264f8[k & 15]];
            } while (k != 0xffff);
        }
        p->unknown_048 = 0xffff;
    }
    p->unknown_040 = p->unknown_034;
    p->unknown_044 = p->unknown_022;
}

void title_04410(TitleObject *p)
{
    if (p->unknown_044 == p->unknown_022 && p->unknown_040 == p->unknown_034 && p->unknown_042 == p->unknown_036)
        return;
    if (p->unknown_044 != p->unknown_022)
        title_047a0(p);
    title_042d0(p);
    p->unknown_042 = p->unknown_036;
    p->unknown_040 = p->unknown_034;
    p->unknown_044 = p->unknown_022;
}

void title_04610(TitleObject *p);
int title_04b70(int idx, int flag);
void title_04f00(int i);
void title_0c690(void *p, int n);
void title_0c890(void *p, int n);

/* ---- MASKED EQUAL functions, ascending RVA ---- */
/* f_4610 */
void title_04610(TitleObject *p)
{
    unsigned int f = p->unknown_054;
    if (f & 0x10000800)
        return;
    title_042d0(p);
    title_047a0(p);
    if (f & 0x800000) {
        TitleObject *q = p->unknown_060;
        if (q) {
            title_04610(q);
            title_047a0(q);
        }
    }
}

/* f_4660 */
void title_04660(void)
{
    title_0c690(g_2cc04->ot, TITLE_OT_SIZE);
    if (g_2cc04 == g_2cc08)
        g_2df40 = (TitlePrim *)(g_2bb24 + 0x11800);
    else
        g_2df40 = (TitlePrim *)g_2bb24;
}

/* f_46b0 */
unsigned char title_046b0(unsigned short key, unsigned int f)
{
    unsigned int flag = ((f & 0x80) | 2) >> 1;
    TitleTabEntry *p;
    int i;
    for (p = g_2d2a0, i = 0; i < 0x20; p++, i++) {
        if (p->b_02 == key) {
            p->a_00++;
            return (unsigned char)i;
        }
    }
    for (p = g_2d2a0, i = 0; i < 0x20; p++, i++) {
        if (p->a_00 == 0) {
            g_2d2a0[i].b_02 = key;
            g_2d2a0[i].a_00 = (unsigned short)flag;
            return (unsigned char)(i | 0x80);
        }
    }
    return (unsigned char)(i | 0xff);
}

unsigned char title_04710(char *p, int b, int c, int d)
{
    unsigned int v = title_046b0(b, d);
    if ((v & 0x80) == 0x80) {
        v &= 0x7f;
        title_047d0((int)p, v, c);
    }
    return v;
}

/* f_4760 */
void title_04760(void)
{
    TitleTabEntry *p = g_2d2a0;
    int i = 0x20;
    do {
        p->b_02 = 0xffff;
        p->a_00 = 0;
        p++;
    } while (--i != 0);
    g_2dfac = title_04710(g_22108, 0x7641, 0x20, 5);
}

/* f_4870 */
void title_04870(void)
{
    if (g_2df50) {
        title_0c890(g_22100, g_2df44);
        g_2df50 = 0;
    }
}

/* f_47a0 (integrator) */
TitleTabEntry *title_047a0(TitleObject *c)
{
    TitleTabEntry *entry;
    if (c->unknown_01e != 0xff) {
        entry = &g_2d2a0[c->unknown_01e];
        entry->a_00--;
        c->unknown_01e = 0xff;
        c->unknown_01d = 0xff;
        return entry;
    }
}

/* f_47d0 */
void title_047d0(unsigned short *src, int row, int n)
{
    unsigned short *dst;
    int i;
    dst = (unsigned short *)(g_2df44 + (row << 9));
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
    g_2df50 = 1;
}

/* f_4a40 */
void title_04a40(const unsigned char *s, int x)
{
    unsigned int c;
    if (s == 0)
        return;
    c = *s++;
    if (c == 0xff)
        return;
    do {
        if (c == (unsigned int)x && g_26110[c].table_0c == 0) {
            title_04b70(c, 0);
            g_26110[c].state_12 = 2;
        }
        c = *s++;
    } while (c != 0xff);
}

/* f_4b00 */
void title_04b00(void)
{
    int i;
    for (i = 1; i < 18; i++) {
        int s = g_26110[i].state_10;
        if (s == 4)
            title_04f00(i);
        if (s == 7) {
            title_04f00(i);
            g_26110[i].state_10 = 6;
        }
    }
}

/* f_4b50 */
void title_04b50(int idx)
{
    if (g_26110[idx].state_10 == 0)
        g_26110[idx].state_10 = 1;
}


/* f_4b70 */
extern TitleRecord *g_2d324;
static unsigned long g_29d98;
static unsigned long g_29d9c;
extern int g_22148;
int title_016e0(const char *fmt, ...);
int title_0c350(char *path, const char *archive);
int title_0c450(unsigned char **slot, unsigned long size, int align);
void title_1d770(int a);
int title_0c2a0(char *path, const char *archive, int a, unsigned char *dst, void *pos);
int title_0c3f0(char *path, const char *archive, unsigned char *dst, void *pos);
void title_04e40(void);
void title_04e60(void);
void title_04ea0(int idx);

int title_04b70(int idx, int flag)
{
    unsigned long size;
    unsigned long end;
    unsigned short *p;
    void *pos;
    int i;

    if (idx >= 18) {
        title_016e0("Illegal Dbase %d \n", idx);
        return 1;
    }
    if (g_engine_interface.context_004->mode_09c == 1) {
        return 0;
    }
    if (g_engine_interface.context_004->blocked_0df != 0) {
        return 0;
    }
    if (g_26110[idx].state_10 == 3) {
        return 0;
    }
    g_2d324 = &g_26110[idx];
    size = g_2d324->count_04 * 4 + 4;
    pos = g_2d324->unknown_08;
    if (pos == 0) {
        pos = (void *)title_0c350(g_2d324->path_00, "\ANIMPSX.BIN");
        g_2d324->unknown_08 = pos;
    }
    end = (unsigned long)pos + size;
    p = (unsigned short *)title_0c450(&g_26110[idx].table_0c, (end & ~0x7ff) + 0x1000, 0x20);
    if (p == 0) {
        title_1d770(0);
        p = (unsigned short *)title_0c450(&g_26110[idx].table_0c, (end & ~0x7ff) + 0x1000, 0x20);
    }
    if (p != 0) {
        for (i = 0; i < g_2d324->count_04 + 1; i++) {
            p[i * 2] = 0;
            p[i * 2 + 1] = 0;
        }
        g_26110[idx].state_10 = 2;
        g_29d98 = end;
        g_22148 = idx;
        g_26110[idx].state_12 = 1;
        g_29d9c = g_engine_interface.context_004->tick_038;
        if (flag != 0) {
            g_engine_interface.context_004->load_complete = title_04e40;
            g_engine_interface.context_004->load_failed = title_04e60;
            title_0c2a0(g_2d324->path_00, "\ANIMPSX.BIN", 0, g_2d324->table_0c + size, pos);
            return -1;
        }
        if (title_0c3f0(g_2d324->path_00, "\ANIMPSX.BIN", g_2d324->table_0c + size, pos) != 0) {
            title_04e40();
            return 0;
        }
        title_04ea0(idx);
        return 0;
    }
    if (flag == 0) {
        g_26110[idx].state_10 = 0;
    }
    return 1;
}

/* 0x48a0: reset the title animation state and rebuild every loaded slot's frame index.
   For each loaded slot (state 3) the count+1 index words are cleared; then each entry i whose
   descriptor word (in the descriptor array that follows the index, d = table + (count + 2) * 4)
   has bit 0x40 and a nonzero part count gets index flag 0x40 and the result of 0x3da0 over its
   part list, after the slot's 8-byte header entry (d + d[0].w * 4 + 4) is copied to *g_2bb50. */
extern long *g_2bb50;
extern signed char g_2cc02;
extern int g_29d90;
int title_0c310(int a);
void title_04f40(const unsigned char *a, const unsigned char *b);
void title_04a10(const unsigned char *a, const unsigned char *b);
void title_04fe0(void);
void title_0c6f0(void);
void title_0c6d0(int a);
void title_0c700(void);
unsigned short title_03da0(unsigned char *q, int n, int m);

void title_048a0(const unsigned char *a, const unsigned char *b)
{
    int j;
    unsigned short *p;
    long *d;
    long *src;
    unsigned char *s;
    unsigned char *q;
    unsigned short *e;
    unsigned char n;
    int i;

    g_29d94 = 0;
    g_29d90 = 0;
    title_04f40(a, b);
    title_0c310(0);
    title_04fe0();
    title_04a10(a, b);
    if (g_2cc02)
        g_26110[0].state_10 = 3;
    for (j = 1; j < 18; j++) {
        p = (unsigned short *)g_26110[j].table_0c;
        if (p != 0 && g_26110[j].state_10 == 3) {
            for (i = 0; i <= g_26110[j].count_04; i++) {
                p[0] = 0;
                p[1] = 0;
                p += 2;
            }
            for (i = 1; i <= g_26110[j].count_04; i++) {
                d = (long *)g_26110[j].table_0c + g_26110[j].count_04 + 2;
                if (d[i] & 0x40) {
                    s = (unsigned char *)d + (d[i] >> 8);
                    n = *s++;
                    if (n != 0) {
                        e = (unsigned short *)g_26110[j].table_0c + i * 2;
                        e[0] = 0x40;
                        q = s + (signed char)*s * 6 + 5;
                        src = d + *(unsigned short *)d + 1;
                        g_2bb50[0] = src[0];
                        g_2bb50[1] = src[1];
                        e[1] = title_03da0(q + 2, n, 0x40);
                    }
                }
            }
        }
    }
    title_0c6f0();
    title_041d0();
    title_0c6d0(0);
    title_0c700();
}
