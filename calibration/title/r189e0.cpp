/* TITLE.DLL lane w23 region (C++ front end, extern "C"): 0x189e0, 0x18a90, 0x18f80, 0x19040, 0x190d0 (ascending).
 * 0x18a90 needs the C++ front end; the other four also match in C++ (see result.json). 0x186e0, 0x18b20 and 0x18bf0 are not in this region. */
#include "title_engine.h"

typedef struct TitleStream {
    unsigned char unknown_000[0x5c];
    unsigned char *stream_05c;
} TitleStream;

typedef struct TitlePair8 {
    unsigned int a_00;
    unsigned int b_04;
} TitlePair8;

typedef struct TitleObj {
    unsigned char unknown_000[0x0c];
    unsigned short w_0c;
    unsigned short w_0e;
    short w_10;
    unsigned char unknown_012[0x1f - 0x12];
    unsigned char flags_01f;
    unsigned char byte_020;
    unsigned char b_021;
    unsigned char b_022;
    unsigned char byte_023;
    unsigned char unknown_024[0x34 - 0x24];
    unsigned short w_034;
    unsigned char unknown_036[0x54 - 0x36];
    unsigned int flags_054;
    unsigned char unknown_058[0x68 - 0x58];
    struct TitleObj *next_068;
    unsigned char unknown_06c[0x76 - 0x6c];
    short w_076;
    unsigned char unknown_078[0x120 - 0x78];
    unsigned int *list_120;
} TitleObj;

extern "C" {

extern TitlePair8 g_2b720[];
extern TitlePair8 *g_2bb20;
extern signed char g_2cc02;
extern char *g_2cbf0;

int title_18a90(TitleObj *o);
void title_18b20(TitleObj *o);
void title_19680(void *p);
void title_196f0(TitleObj *o, int v);
void title_0c760(int *pair);
void title_0c7b0(void);
void title_0c810(void *e);
void title_0c840(void *b);
void title_0c7e0(void *p);
unsigned long make_colour(void *p);
void title_18bf0(TitleObj *o, unsigned char *q, unsigned long c);
int title_19570(TitleObj *o, unsigned int v);

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
            title_19680(arg);
            if (arg->flags_054 & 0x20000000) {
                list = arg->list_120;
                count = *list;
                list++;
                if (count-- != 0) {
                    k = count + 1;
                    do {
                        title_19680((void *)*list++);
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
        if (title_19570(o, q[3]) != 0) {
            return 0;
        }
    }
    title_196f0(o, o->w_076);
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

}
