/* TITLE region 0x4d80..0x5c60 (sound-effect unit and its neighbours), ascending RVA. */
#include <string.h>
#include "title_engine.h"
#include "title_slots.h"
#include "title_screen.h"

extern int g_22148;

typedef struct TitleRec64 {
    unsigned long unknown_00;
    unsigned long unknown_04;
    short id_08;
    short unknown_0a;
    short word_0c;
    short unknown_0e;
    long unknown_10;
    long unknown_14;
    long unknown_18;
    long unknown_1c;
    long unknown_20;
    long unknown_24;
    unsigned char unknown_28[0x30 - 0x28];
    TitleObject *owner_30;
    unsigned long flags_34;
    long unknown_38;
    unsigned long active_3c;
} TitleRec64;

extern TitleRec64 g_2cca0[24];

extern int g_2cc6c;
extern int g_2cc70;
extern int g_2cc74;
extern int g_2cc78;
extern int g_2cc7c;
extern int g_2cc80;
extern int g_2cc84;

static unsigned long g_29dac;
static unsigned long g_29db8;
static unsigned long g_29e04;

/* ---- f_4d80 ---- */

void title_04b50(int a);
int title_04dd0(int idx, int start);

int title_04d80(int idx, int arg2)
{
    unsigned int st = g_26110[idx].state_10;

    if (st == 2) {
        return title_04dd0(idx, arg2);
    }
    if (st == 0) {
        title_04b50(idx);
        g_26110[idx].state_12 = 3;
    }
    return 0;
}

int title_04dd0(int idx, int start)
{
    ResourceCallbackContext *ctx = g_engine_interface.context_004;
    int loaded = (ctx->total_sectors_0a0 - ctx->remaining_sectors_09e) << 11;
    int *p;
    int v;

    if (loaded < 0x1000)
        return 0;
    p = (int *)g_26110[idx].table_0c + 2 + g_26110[idx].count_04;
    for (;;) {
        if (start >= g_26110[idx].count_04)
            return 0;
        v = p[++start] >> 8;
        if (v != 0)
            return loaded >= v;
    }
}


/* ---- f_4e40 ---- */

void title_04e40(void)
{
    int i = g_22148;
    if (g_26110[i].state_10 == 2) {
        g_26110[i].state_10 = 3;
    }
}

/* ---- f_4e60 ---- */

void title_04b50(int a);
void title_04ea0(int a);
void title_04f00(int a);

void title_04e60(void)
{
    title_04ea0(g_22148);
    if (g_26110[g_22148].state_10 == 4) {
        title_04f00(g_22148);
        title_04b50(g_22148);
    }
}

/* ---- f_4ea0 ---- */

void title_0c2d0(void);

void title_04ea0(int idx)
{
    switch (g_26110[idx].state_10) {
    case 1:
        g_26110[idx].state_10 = 4;
        break;
    case 2:
        g_engine_interface.context_004->load_complete = 0;
        g_engine_interface.context_004->load_failed = 0;
        title_0c2d0();
        g_26110[idx].state_10 = 4;
        break;
    case 3:
        g_26110[idx].state_10 = 4;
        break;
    }
}

/* ---- f_4f00 ---- */

int title_0c330(int a);

void title_04f00(int idx)
{
    if (g_26110[idx].table_0c != 0) {
        title_0c330((int)g_26110[idx].table_0c);
        g_26110[idx].table_0c = 0;
    }
    g_26110[idx].state_10 = 0;
    g_26110[idx].state_12 = 0;
}

/* ---- f_4f40 ---- */

void title_04b00(void);
void title_04ea0(int idx);

void title_04f40(const unsigned char *a, const unsigned char *b)
{
    int i;
    unsigned char c;
    const unsigned char *p;

    for (i = 0; i < 18; i++) {
        if (g_26110[i].state_10 != 0) {
            if (a != 0) {
                c = *a;
                p = a + 1;
                while (c != 0xff) {
                    if (i == c) {
                        goto next;
                    }
                    c = *p;
                    p++;
                }
            }
            if (b != 0) {
                c = *b;
                p = b + 1;
                while (c != 0xff) {
                    if (i == c) {
                        goto next;
                    }
                    c = *p;
                    p++;
                }
            }
            title_04ea0(i);
        }
    next:
        ;
    }
    title_04b00();
}

/* ---- f_5080 ---- */

int title_05080(int id)
{
    int count = 0;
    int i;

    for (i = 0; i < 24; i++) {
        if (g_2cca0[i].active_3c != 0) {
            if (g_2cca0[i].id_08 == id) {
                count++;
            }
        }
    }
    return count;
}

/* ---- f_50b0 ---- */

int title_0c490(void);
int title_0cb70(unsigned char *playing);
int title_0cb80(int channel, unsigned long *info);
int title_0cba0(void);
int title_0cbb0(short left, short right);
int title_0cbd0(void);
int title_0cbe0(int channel, short *left, short *right);
int title_0cc40(int channel, short left, short right);

static __inline void title_fx_pan(TitleRec64 *rec)
{
    int l;
    int r;

    g_2cc6c = rec->owner_30->unknown_00c - 160;
    if (g_2cc6c > -20 && g_2cc6c < 20) {
        l = 0;
        r = 0;
    } else {
        l = g_2cc6c > -20 ? g_2cc6c * 2 - 40 : -20 - g_2cc6c;
        r = g_2cc6c > 20 ? g_2cc6c - 20 : (-20 - g_2cc6c) * 2;
    }
    if (l < -640) {
        l = -640;
    } else if (l > 640) {
        l = 640;
    }
    if (r < -640) {
        r = -640;
    } else if (r > 640) {
        r = 640;
    }
    g_2cc70 = 127 - l / 5;
    g_2cc74 = 127 - r / 5;
    if (g_2cc70 < 0) {
        g_2cc70 = 0;
    }
    if (g_2cc70 > 127) {
        g_2cc70 = 127;
    }
    if (g_2cc74 < 0) {
        g_2cc74 = 0;
    }
    if (g_2cc74 > 127) {
        g_2cc74 = 127;
    }
    g_2cc78 = rec->owner_30->unknown_010 * 2 - 320;
    if (rec->flags_34 & 0x1000) {
        g_2cc78 <<= 1;
    }
    if (rec->flags_34 & 0x2000) {
        g_2cc78 >>= 1;
    }
    g_2cc7c = 0x800 - g_2cc78;
    if (g_2cc7c < 0) {
        g_2cc7c = 0;
    }
    if (g_2cc7c > 0x800) {
        g_2cc7c = 0x800;
    }
    g_2cc70 = (g_2cc7c * g_2cc70) >> 11;
    g_2cc74 = (g_2cc7c * g_2cc74) >> 11;
    if (rec->flags_34 & 0x4000) {
        rec->unknown_10 = g_2cc70;
        rec->unknown_14 = g_2cc74;
    } else {
        rec->unknown_10 += (g_2cc70 - rec->unknown_10) >> 1;
        rec->unknown_14 += (g_2cc74 - rec->unknown_14) >> 1;
    }
    if (g_engine_interface.context_004->unknown_000[4] != 0) {
        rec->unknown_10 = 0;
        rec->unknown_14 = 0;
    }
    g_2cc80 = rec->unknown_38 * rec->unknown_10;
    g_2cc80 *= g_2cc80;
    g_2cc84 = rec->unknown_38 * rec->unknown_14;
    g_2cc84 *= g_2cc84;
    rec->unknown_18 = g_2cc80 / 0x4000 * 129 / 128;
    rec->unknown_1c = g_2cc84 / 0x4000 * 129 / 128;
}

void title_050b0(void)
{
    short left;
    short right;
    unsigned long info;
    unsigned char playing[24];
    int i;
    unsigned int j;

    g_29e04 = (g_29e04 - 1) & 1;
    if (g_29e04 != 0) {
        title_0c490();
    }
    title_0cbd0();
    title_0cb70(playing);
    for (i = 0; i < 24; i++) {
        if (playing[i] != 0) {
            title_0cb80(i, &info);
            g_2cca0[i].active_3c = 1;
            if (g_2cca0[i].owner_30 != 0) {
                if (g_2cca0[i].flags_34 & 0x800) {
                    if (g_2cca0[i].unknown_0a > 1) {
                        if (g_2cca0[i].owner_30->unknown_068 != 0) {
                            if (g_2cca0[i].owner_30->unknown_010 != 0) {
                                title_fx_pan(&g_2cca0[i]);
                                if (g_engine_interface.context_004->unknown_1b24 == 0) {
                                    title_0cc40(i,
                                                (short)((g_2cca0[i].unknown_18 + g_2cca0[i].unknown_1c) * g_engine_interface.context_004->unknown_1b18 >> 8),
                                                (short)((g_2cca0[i].unknown_18 + g_2cca0[i].unknown_1c) * g_engine_interface.context_004->unknown_1b18 >> 8));
                                } else {
                                    title_0cc40(i,
                                                (short)(g_2cca0[i].unknown_18 * g_engine_interface.context_004->unknown_1b18 >> 7),
                                                (short)(g_2cca0[i].unknown_1c * g_engine_interface.context_004->unknown_1b18 >> 7));
                                }
                            } else if (g_engine_interface.context_004->unknown_000[4] != 0) {
                                title_0cc40(i, 0, 0);
                            }
                        }
                    } else {
                        g_2cca0[i].unknown_0a++;
                    }
                } else if (g_engine_interface.context_004->unknown_000[4] != 0) {
                    title_0cc40(i, 0, 0);
                }
            } else if (g_engine_interface.context_004->unknown_000[4] != 0) {
                if (g_29db8 == 0) {
                    title_0cc40(i, 0, 0);
                }
                g_29dac = 1;
            } else {
                if (g_29dac == 1) {
                    title_0cc40(i, (short)g_2cca0[i].unknown_18, (short)g_2cca0[i].unknown_1c);
                }
                title_0cbe0(i, &left, &right);
                g_2cca0[i].unknown_18 = left;
                g_2cca0[i].unknown_1c = right;
            }
        } else {
            g_2cca0[i].active_3c = 0;
        }
    }
    if (g_engine_interface.context_004->unknown_000[4] != 0) {
        g_29db8 = 1;
    } else {
        g_29db8 = 0;
        g_29dac = 0;
    }
    title_0cba0();
    if (g_engine_interface.context_004->unknown_1b28 != g_engine_interface.context_004->unknown_1b2c) {
        for (j = 0; j < g_engine_interface.context_004->unknown_1b30; j++) {
            if (g_engine_interface.context_004->unknown_1b28 != g_engine_interface.context_004->unknown_1b2c) {
                if (g_engine_interface.context_004->unknown_1b28 > g_engine_interface.context_004->unknown_1b2c) {
                    g_engine_interface.context_004->unknown_1b28--;
                } else {
                    g_engine_interface.context_004->unknown_1b28++;
                }
            }
        }
        title_0cbb0((short)g_engine_interface.context_004->unknown_1b28, (short)g_engine_interface.context_004->unknown_1b28);
    }
}

/* ---- f_54f0 ---- */

void title_0c6f0(void);
void title_0c700(void);
int title_016e0(const char *fmt, ...);
short title_0cc10(short a, short b, short c, short d, short e, short f, short g);

/* ---- f_5a70 ---- */

void title_0c6f0(void);
void title_0c700(void);
void title_0cc00(int a);
int title_016e0(const char *fmt, ...);

void title_05a70(int arg)
{
    int i;
    unsigned long mask;

    if (arg == 0) {
        return;
    }
    mask = 0x8000;
    title_0c6f0();
    title_016e0("\n RemoveFxLinks(0x%x)", arg);
    for (i = 0; i < 24; i++) {
        if (g_2cca0[i].owner_30 == (TitleObject *)arg) {
            if ((g_2cca0[i].flags_34 & mask) == 0) {
                title_0cc00(i);
            }
            g_2cca0[i].owner_30 = 0;
        }
    }
    title_0c700();
}

void title_05ad0(unsigned short id, TitleObject *owner)
{
    int bank;
    int sample;
    int i;

    title_0c6f0();
    bank = ((unsigned char *)g_engine_interface.context_004)[id * 3 + 0x2f0];
    sample = ((unsigned char *)g_engine_interface.context_004)[id * 3 + 0x2f1];
    title_016e0("\n RemoveFxIdLinks(%d,0x%x)", id & ~0xf800, owner);
    if (owner == 0) {
        for (i = 0; i < 24; i++) {
            if (bank == g_2cca0[i].word_0c && sample == g_2cca0[i].unknown_0e) {
                title_0cc00(i);
                if (g_2cca0[i].owner_30 != 0) {
                    g_2cca0[i].owner_30->unknown_06c = 0;
                    g_2cca0[i].owner_30 = 0;
                }
            }
        }
    } else {
        for (i = 0; i < 24; i++) {
            if (owner == g_2cca0[i].owner_30 && bank == g_2cca0[i].word_0c && sample == g_2cca0[i].unknown_0e) {
                title_0cc00(i);
                if (g_2cca0[i].owner_30 != 0) {
                    g_2cca0[i].owner_30->unknown_06c = 0;
                    g_2cca0[i].owner_30 = 0;
                }
            }
        }
    }
    title_0c700();
}


/* 0x5bc0 160 */
/* 0x5bc0: stop every playing effect except the given effect id's bank/sample pair and
   unlink it from its owner. */
void title_05bc0(unsigned short id)
{
    int bank;
    int sample;
    int i;

    title_0c6f0();
    bank = ((unsigned char *)g_engine_interface.context_004)[(id & ~0xf800) * 3 + 0x2f0];
    sample = ((unsigned char *)g_engine_interface.context_004)[(id & ~0xf800) * 3 + 0x2f1];
    title_016e0("\n ClearAllSoundFx_Excpt(%d)", id & ~0xf800u);
    for (i = 0; i < 24; i++) {
        if (bank != g_2cca0[i].word_0c || sample != g_2cca0[i].unknown_0e) {
            title_0cc00(i);
            g_2cca0[i].word_0c = 0xff;
            g_2cca0[i].unknown_0e = 0;
            g_2cca0[i].owner_30 = 0;
        }
    }
    title_0c700();
}

/* ---- f_5c60 ---- */

void title_0c5e0(void);
void title_0c550(int a);
void title_050b0(void);

void title_05c60(int a)
{
    int i;

    title_0c5e0();
    g_29dac = 0;
    g_29db8 = 0;
    if (a == 0) {
        title_0c550((int)title_050b0);
    } else {
        title_0c550(0);
    }
    for (i = 0; i < 24; i++) {
        g_2cca0[i].word_0c = 0xff;
        g_2cca0[i].owner_30 = 0;
    }
}
