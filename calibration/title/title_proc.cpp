/* TITLE process scheduler unit (C++).
   Hypothesis: one object from 0x5cc0 up to 0x6350. Its literals follow the file-name lists in
   .data and its .bss block 0x29e08..0x29fb8 follows the sound-effect unit's. Two variables of that
   block (0x29e08, 0x29f98) are read across the whole DLL, so they are ordinary definitions of a C++
   object: a C file would make them communal (shared .bss tail) and C statics cannot be shared. */
#include <string.h>

extern "C" {

typedef struct TitleProc TitleProc;
struct TitleProc {
    void (*fn_00)(TitleProc *);
    unsigned long unknown_04;
    long refs_08;
    unsigned long unknown_0c;
    TitleProc *next_10;
    unsigned long unknown_14;
};

struct TitleObject;

/* .bss of this unit (only variables used by reconstructed code; layout order is not reproduced). */
struct TitleObject *g_29e08;    /* current object, read across the DLL */
unsigned long g_29e10;          /* callback of the process being run */
TitleProc g_29e18[16];          /* process pool */
int g_29f98;                    /* read across the DLL */
unsigned long g_29f9c;          /* scheduler ticks */
int g_29fa4;

extern TitleProc *g_2cc20[16];
extern int g_2cc04;

int title_0c5b0(int a);

void title_05d60(void)
{
    if (g_29fa4 == 1) {
        title_0c5b0(g_2cc04);
        g_29fa4 = 0;
    }
}

void title_05d90(void)
{
    memset(g_2cc20, 0, 0x40);
    memset(g_29e18, 0, 0x180);
}

void title_05e40(void)
{
    TitleProc **pp;
    TitleProc *o;

    g_29f9c++;
    for (pp = g_2cc20; pp < &g_2cc20[16]; pp++) {
        o = *pp;
        while (o != 0) {
            o->refs_08--;
            if (o->refs_08 == 0) {
                g_29e10 = (unsigned long)o->fn_00;
                o->fn_00(o);
            }
            o = o->next_10;
        }
    }
}

void title_05f10(TitleProc *target)
{
    int i;
    unsigned int j;
    TitleProc *s;

    for (i = 0, s = g_29e18; s < g_29e18 + 16; i++, s++) {
        if (s == target) {
            for (j = 0; j < 16; j++) {
                if (g_2cc20[j] == s) {
                    g_2cc20[j] = g_29e18[i].next_10;
                    g_29e18[i].next_10 = 0;
                    g_29e18[i].fn_00 = 0;
                    return;
                }
            }
            for (j = 0; j < 16; j++) {
                if (g_29e18[j].next_10 == s) {
                    g_29e18[j].next_10 = g_29e18[i].next_10;
                    g_29e18[i].next_10 = 0;
                    g_29e18[i].fn_00 = 0;
                    return;
                }
            }
        }
    }
}

} /* extern "C" */
