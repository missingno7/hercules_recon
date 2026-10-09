/* Sequence screen 4 unit: handler 0x13ba0 and helpers 0x14120, 0x14230.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


static TitleObject *g_2abd0;
static int g_2abd4;
static TitleObject *g_2abd8;
static int g_2abdc;
static TitleObject *g_2abf0;
static int g_2abf4;
static int g_2abfc;
extern unsigned short g_25f50[];
int title_0c4f0(int a);

/* 0x13ba0: not reconstructed yet */

void title_14120(void)
{
    g_2abf4 = 10;
    g_2abd0 = title_17ad0(0, 0, 0, 0x200c, 0);
    g_2abd0->unknown_034 = 1;
    g_2abd0->unknown_054 |= 5;
    g_2abd0->unknown_04a = 0;
    g_2abf0 = title_17ad0(0, 0, 0, 0x200c, 0);
    g_2abf0->unknown_034 = 1;
    g_2abf0->unknown_054 |= 6;
    g_2abf0->unknown_04a = 0;
    g_2abd0->unknown_023 = 6;
    g_2abd0->unknown_03e = 10;
    g_2abf0->unknown_023 = 6;
    g_2abf0->unknown_03e = 20;
    g_2abf0->unknown_000 = 0x10000;
    g_2abf0->unknown_004 = 0x10000;
    g_2abd8 = title_17ad0(0, 0, 0, 0x200c, 0);
    g_2abd8->unknown_034 = 2;
    g_2abd8->unknown_054 |= 0x10;
    g_2abd8->unknown_000 = 0x640000;
    g_2abd8->unknown_004 = 0x280000;
    g_2abdc = 0;
    g_2abd4 = 0;
}

void title_14230(void)
{
    unsigned short code;

    g_2abfc ^= 1;
    if (g_2abfc != 0) {
        return;
    }
    switch (g_2abdc) {
    case 0:
        break;
    case 1:
        g_2abd8->unknown_034++;
        if (g_2abd8->unknown_034 != 0x10) {
            return;
        }
        g_2abdc = 2;
        return;
    case 2:
        g_2abf4 = 0xc;
        g_2abdc = 3;
        return;
    default:
        return;
    }
    g_2abd8->unknown_034 = g_25f50[g_2abd4] + 1;
    g_2abd4++;
    code = g_25f50[g_2abd4];
    if (code == 0xa && g_2abf4 == 0xa) {
        g_2abf4 = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    g_2abdc = 1;
}
