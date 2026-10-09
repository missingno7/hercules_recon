#ifndef TITLE_W05_COMMON_H
#define TITLE_W05_COMMON_H
#include "title_engine.h"

/* Object passed as the second argument of 0x3420/0x3740/0x39b0/0x3d10. */
typedef struct TitleObj {
    unsigned char unknown_000[0x0c];
    short x_00c;
    short y_00e;
    short w_010;
    unsigned char unknown_012[0x1e - 0x12];
    unsigned char b_01e;
    unsigned char unknown_01f[0x3a - 0x1f];
    unsigned short w_03a;
    unsigned short w_03c;
    unsigned short w_03e;
    unsigned char unknown_040[0x46 - 0x40];
    unsigned short w_046;
    unsigned char unknown_048[0x54 - 0x48];
    unsigned long flags_054;
    unsigned char unknown_058[0x74 - 0x58];
    short w_074;
} TitleObj;

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
extern short g_243e0[];
extern signed char g_264f8[];
extern unsigned short g_2d342[][6];

unsigned long title_189a0(TitleObj *obj);
void title_03100(unsigned char *stream, TitleObj *obj, unsigned long count);
void title_03420(unsigned char *stream, TitleObj *obj, unsigned char count);
void title_03740(unsigned char *stream, TitleObj *obj, unsigned long count, unsigned long y, unsigned long x);
void title_039b0(unsigned char *stream, TitleObj *obj, unsigned long count, unsigned long y, unsigned long x);

#endif


void title_03d10(unsigned char *stream, TitleObj *obj, unsigned long count)
{
    unsigned long edx; unsigned long eax; unsigned long esi;
    obj->w_010 = obj->w_03e;
    eax = obj->w_03a;
    edx = eax;
    if (obj->flags_054 & 0x80) {
        eax = obj->w_03c;
    }
    esi = eax | edx;
    if (esi == 0x100) {
        if (obj->w_074 != 0) {
            title_03420(stream, obj, (unsigned char)count);
        } else {
            title_03100(stream, obj, count);
        }
    } else {
        if (obj->w_074 != 0) {
            title_039b0(stream, obj, count, edx, eax);
        } else {
            title_03740(stream, obj, count, edx, eax);
        }
    }
}
