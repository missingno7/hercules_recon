/* TITLE.DLL lane w04 region: functions reaching MASKED EQUAL, ascending RVA.
 * Shared types and externs first; the display buffers are in title_gpu.h. */
#include "title_gpu.h"
typedef struct Actor Actor;
struct Actor {
    long x, y, z;
    short unknown_0c, unknown_0e, unknown_10;
    unsigned char unknown_12[0x1f - 0x12];
    unsigned char unknown_1f;
    unsigned char unknown_20[0x23 - 0x20];
    unsigned char mode;
    unsigned char unknown_24[0x34 - 0x24];
    unsigned short unknown_34;
    unsigned short unknown_36;
    unsigned char unknown_38[0x3a - 0x38];
    unsigned short unknown_3a;
    unsigned short unknown_3c;
    unsigned char unknown_3e[0x46 - 0x3e];
    unsigned short unknown_46;
    unsigned short unknown_48;
    unsigned char unknown_4a[0x54 - 0x4a];
    unsigned long render_flags;
    unsigned char unknown_58[0x68 - 0x58];
    Actor *next;
    unsigned char unknown_6c[0x74 - 0x6c];
    short unknown_74;
};

extern int g_2dfa0;
extern unsigned short g_2bf6e;
extern int g_2d328;
extern Actor *g_2dfa4;
extern int g_2df48;
extern int g_2dfb4;
extern int g_2df4c;
extern unsigned short g_2bf58;
extern int g_2d320;
extern int g_2cbe8;
extern unsigned short g_2bf60;
extern unsigned short g_220f2;
extern short g_220f6;
extern int g_29d94;
extern int g_29d90;
extern int (*g_220d0[])(Actor *);

void title_04470(int a, int b, int c);
int title_189e0(int a);
int title_195f0(Actor *p);
void title_01dd0(Actor *p, unsigned char *q);
void title_02cc0(Actor *p);
void title_02dc0(Actor *p);
void title_03100(int a, Actor *p, int c);
void title_03420(int a, Actor *p, int c);
void title_03740(int a, Actor *p, int c, int v1, int v2);
void title_039b0(int a, Actor *p, int c, int v1, int v2);

void title_02a80(void)
{
    title_0cab0(&g_2cc08[0].draw, 0, 0, 0x140, 0x100);
    title_0cab0(&g_2cc08[1].draw, 0, 0x100, 0x140, 0x100);
    title_0ca80(&g_2cc08[0].disp, 0, 0x100, 0x140, 0xf0);
    title_0ca80(&g_2cc08[1].disp, 0, 0, 0x140, 0xf0);
    g_2cc08[1].disp.screen.x = 0;
    g_2cc08[0].disp.screen.x = 0;
    g_2cc08[1].disp.screen.y = 0;
    g_2cc08[0].disp.screen.y = 0;
    g_2cc08[1].disp.screen.h = 0xf0;
    g_2cc08[0].disp.screen.h = 0xf0;
    g_2cc08[1].disp.screen.w = 0;
    g_2cc08[0].disp.screen.w = 0;
}

void title_02cc0(Actor *p)
{
    unsigned long flags;
    unsigned long sel;
    unsigned long saved34;
    unsigned long saved46;

    flags = p->render_flags;
    sel = flags & 0x10000;
    if (sel) {
        title_01dd0(p, (unsigned char *)&g_2cc04->ot[TITLE_OT_SIZE - 2] - (p->unknown_10 << 2));
    }
    if ((flags & 0x8000000) && p->unknown_36 != 0) {
        if (!(flags & 0x4000000)) {
            title_02dc0(p);
            g_2d328 = 0;
        }
        saved34 = p->unknown_34;
        saved46 = p->unknown_46;
        p->unknown_34 = p->unknown_36;
        p->unknown_46 = p->unknown_48;
        title_02dc0(p);
        g_2d328 = 0;
        p->unknown_48 = p->unknown_46;
        p->unknown_34 = saved34;
        p->unknown_46 = saved46;
        if (flags & 0x4000000) {
            title_02dc0(p);
        }
        g_2d328 = 1;
    } else {
        title_02dc0(p);
    }
    if (sel) {
        title_01dd0(p, (unsigned char *)&g_2cc04->ot[TITLE_OT_SIZE - 2] - (p->unknown_10 << 2));
    }
}
