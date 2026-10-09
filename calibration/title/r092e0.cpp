extern "C" {
#include "title_engine.h"

typedef struct TitleSlot {
    unsigned long d00;
    unsigned long d04;
    unsigned long d08;
    unsigned char unknown_0c[0x22 - 0x0c];
    unsigned char b22;
    unsigned char unknown_23[0x2e - 0x23];
    unsigned short w2e;
    unsigned char unknown_30[0x64 - 0x30];
    unsigned long d64;
} TitleSlot;

typedef struct Rec8 { unsigned long d0; unsigned char b4, b5, b6, b7; } Rec8;
extern Rec8 *g_2bf5c;

void *title_091a0(unsigned long flags);
void title_094c0(TitleSlot *p);

TitleSlot *title_092e0(unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4)
{
    TitleSlot *p = (TitleSlot *)title_091a0(a4);
    if (p != 0) {
        title_094c0(p);
        p->w2e = (unsigned short)(a4 | 0x2000);
        p->d00 = a1;
        p->d04 = a2;
        p->d08 = a3;
        p->b22 = g_2bf5c[a4 & 0xfff].b6;
        p->d64 = g_2bf5c[a4 & 0xfff].d0;
    }
    return p;
}
}
