/* TITLE.DLL lane w27 region: 6 functions that reached MASKED EQUAL, ascending RVA.
 * Shared types, externs and prototypes first. Field names are offsets, not recovered types. */

typedef struct TitleObj {
    int unknown_000;
    int unknown_004;
    unsigned char unknown_008[0x023 - 0x008];
    unsigned char unknown_023;
    unsigned char unknown_024[0x034 - 0x024];
    unsigned short unknown_034;
    unsigned char unknown_036[0x03e - 0x036];
    unsigned short unknown_03e;
    unsigned char unknown_040[0x04a - 0x040];
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x054 - 0x04c];
    unsigned long unknown_054;
} TitleObj;

struct title_pos {
    short x;
    short y;
};

extern unsigned int g_2b308;
extern unsigned int g_2b178;
extern int g_2afe0;
extern TitleObj *g_2b118[7];
extern TitleObj *g_2b014;
extern int g_2adc4;
extern int g_2acf8;
extern int g_2aff8;
extern int g_2affc;
extern int g_2b000;
extern int g_2b004;
extern void *g_2b0ec;
extern int g_2afcc;
extern int g_2afe4;
extern TitleObj *g_2b30c;
extern TitleObj *g_2adc0;
extern int g_2b100;
extern TitleObj *g_2b104;
extern TitleObj *g_2b058;
extern TitleObj *g_2b108;
extern TitleObj *g_2b05c;
extern int g_2af68;
extern int g_2aff4;
extern TitleObj *g_2afd0;
extern int g_26618[];
extern int g_26630[];
extern int g_26634[];
extern TitleObj *g_2b134;
extern void *g_2b300;
extern void *g_29e08;
extern void *g_2b238;
extern void *g_2b31c;
extern void *g_2b060;
extern void *g_2afc8;
extern void *g_2b110;
extern int g_2b1e8[];
extern void *g_2af70[];
extern TitleObj *g_2b330[8];
extern void *g_2b0e8;
extern void *g_2b10c;
extern void *g_2b304;

TitleObj *title_17ad0(int a0, int a1, int a2, int size, int a4);
void title_09350(void *block);
void title_054f0(int a, int b);
void title_18600(TitleObj *o, struct title_pos *pos);
int title_0c4a0(int a);
void title_19a40(int a0, int a1, int a2, int a3, int a4, int a5, int a6);

void title_1bab0(void)
{
    int value;

    if (g_2b100 == 0) {
        value = 6;
        g_2b104 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b104->unknown_034 = 0x57;
        g_2b104->unknown_023 = value;
        g_2b104->unknown_03e = 0x14;
        g_2b104->unknown_04a = 0x80;
        g_2b058 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b058->unknown_034 = 0x57;
        g_2b058->unknown_023 = value;
        g_2b058->unknown_03e = 0x1e;
        g_2b058->unknown_04a = 0xff;
        g_2b058->unknown_054 |= value;
        g_2b058->unknown_000 = 0x20000;
        g_2b058->unknown_004 = 0x20000;
        g_2b108 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b108->unknown_023 = value;
        g_2b108->unknown_03e = 0x14;
        g_2b108->unknown_04a = 0x80;
        g_2b05c = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b05c->unknown_023 = value;
        g_2b05c->unknown_03e = 0x1e;
        g_2b05c->unknown_04a = 0xff;
        g_2b05c->unknown_054 |= value;
        g_2b05c->unknown_000 = 0x20000;
        g_2b05c->unknown_004 = 0x20000;
        g_2b108->unknown_034 = 0x59;
        g_2b05c->unknown_034 = 0x59;
        g_2b100 = 1;
    }
    g_2b104->unknown_04a = (unsigned short)g_2afe4;
    g_2b058->unknown_04a = (unsigned short)(g_2afe4 * 2);
    g_2b108->unknown_04a = (unsigned short)g_2afe4;
    g_2b05c->unknown_04a = (unsigned short)(g_2afe4 * 2);
}

void title_1bc60(void)
{
    TitleObj **p;

    if (g_2b308 == 8) {
        g_2b178++;
    }
    if (g_2b178 > 0 && g_2afe0 == 2) {
        for (p = g_2b118; (int)p < (int)(g_2b118 + 7); p++) {
            (*p)->unknown_054 |= 0x80000000;
        }
        g_2b014->unknown_054 |= 0x80000000;
        g_2afe0 = 1;
        g_2adc4 = 1;
    }
    if (g_2b178 > 0x78 && g_2acf8 == 0) {
        g_2acf8 = 1;
    }
}

void title_1bcf0(void)
{
    if (++g_2b178 != 0 && g_2aff8 == 1) {
        if (g_2b0ec == 0) {
            title_054f0(0x318, 0);
        }
        g_2aff8 = 2;
    }
    if (g_2b178 > 0xa && g_2affc == 1) {
        if (g_2b0ec == 0) {
            title_054f0(0x318, 0);
        }
        g_2affc = 2;
    }
    if (g_2b178 > 0x14 && g_2b000 == 1) {
        if (g_2b0ec == 0) {
            title_054f0(0x318, 0);
        }
        g_2b000 = 2;
    }
    if (g_2b178 > 0x1e && g_2b004 == 1) {
        if (g_2b0ec == 0) {
            title_054f0(0x318, 0);
        }
        g_2b004 = 2;
    }
}

void title_1bdc0(void)
{
    switch (g_2afcc) {
    case 0:
        g_2b30c = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b30c->unknown_034 = 2;
        g_2adc0 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2adc0->unknown_034 = 2;
        g_2adc0->unknown_023 = 6;
        g_2b30c->unknown_023 = 6;
        g_2b30c->unknown_03e = 0x14;
        g_2adc0->unknown_03e = 0x1e;
        g_2adc0->unknown_004 = 0x20000;
        g_2adc0->unknown_000 = 0x20000;
        g_2adc0->unknown_054 |= 6;
        g_2adc0->unknown_04a = 0;
        g_2b30c->unknown_04a = 0;
        g_2afcc = 1;
        break;
    case 1:
        g_2b30c->unknown_04a = (unsigned short)g_2afe4;
        g_2adc0->unknown_04a = (unsigned short)(g_2afe4 + g_2afe4);
        break;
    }
}

void title_1beb0(void)
{
    struct title_pos pos;

    switch (g_2af68) {
    case 0:
        g_2afd0 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2afd0->unknown_034 = 0x335;
        g_2adc0->unknown_023 = 6;
        g_2afd0->unknown_023 = 6;
        g_2afd0->unknown_03e = 5;
        g_2aff4 = 0;
        g_2af68 = 1;
        g_2afd0->unknown_000 = 0xff380000;
        g_2afd0->unknown_004 = 0xffe00000;
        break;
    case 1:
        break;
    case 2:
        g_2afd0->unknown_034 = g_26618[g_2aff4];
        g_2aff4++;
        if (g_26618[g_2aff4] == -1) {
            g_2aff4 = 0;
        }
        title_18600(g_2afd0, &pos);
        title_19a40(0x23f, 0x2012,
                    ((title_0c4a0(0x10) + pos.x - 8) << 16) + g_2afd0->unknown_000,
                    ((title_0c4a0(0x10) + pos.y - 8) << 16) + g_2afd0->unknown_004,
                    0, title_0c4a0(1), g_2afe4);
        g_2afd0->unknown_000 += 0x30000;
        if (g_2afd0->unknown_000 > -0x800000) {
            g_2af68 = 3;
            g_2aff4 = 0;
        }
        break;
    case 3:
        title_18600(g_2afd0, &pos);
        title_19a40(0x23f, 0x2012,
                    ((title_0c4a0(0x10) + pos.x - 8) << 16) + g_2afd0->unknown_000,
                    ((title_0c4a0(0x10) + pos.y - 8) << 16) + g_2afd0->unknown_004,
                    0, title_0c4a0(1), g_2afe4);
        g_2afd0->unknown_034 = g_26630[g_2aff4];
        g_2afd0->unknown_03e = g_26634[g_2aff4];
        g_2aff4 += 2;
        if (g_26630[g_2aff4] == -1) {
            g_2aff4 = 0;
        }
        g_2afd0->unknown_000 += 0x30000;
        if (g_2afd0->unknown_000 > 0x400000 && g_2afd0->unknown_034 == 0x335) {
            g_2af68 = 4;
            g_2aff4 = 0;
        }
        break;
    case 4:
        if (g_2afd0->unknown_000 < 0xc80000) {
            g_2afd0->unknown_034 = g_26618[g_2aff4];
            g_2aff4++;
            if (g_26618[g_2aff4] == -1) {
                g_2aff4 = 0;
            }
            title_18600(g_2afd0, &pos);
            title_19a40(0x23f, 0x2012,
                        ((title_0c4a0(0x10) + pos.x - 8) << 16) + g_2afd0->unknown_000,
                        ((title_0c4a0(0x10) + pos.y - 8) << 16) + g_2afd0->unknown_004,
                        0, title_0c4a0(1), g_2afe4);
            g_2afd0->unknown_000 += 0x30000;
        }
        break;
    }
    g_2afd0->unknown_04a = (unsigned short)g_2afe4;
}

void title_1c240(void)
{
    TitleObj **p;
    int off;

    if (g_2afd0 != 0) {
        title_09350(g_2afd0);
        g_2afd0 = 0;
    }
    if (g_2b134 != 0) {
        title_09350(g_2b134);
        g_2b134 = 0;
    }
    if (g_2b300 != 0) {
        title_09350(g_2b300);
        g_2b300 = 0;
    }
    if (g_29e08 != 0) {
        title_09350(g_29e08);
        g_29e08 = 0;
    }
    if (g_2b238 != 0) {
        title_09350(g_2b238);
        g_2b238 = 0;
    }
    if (g_2b31c != 0) {
        title_09350(g_2b31c);
        g_2b31c = 0;
    }
    if (g_2b060 != 0) {
        title_09350(g_2b060);
        g_2b060 = 0;
    }
    if (g_2afc8 != 0) {
        title_09350(g_2afc8);
        g_2afc8 = 0;
    }
    if (g_2b110 != 0) {
        title_09350(g_2b110);
        g_2b110 = 0;
    }
    if (g_2b30c != 0) {
        title_09350(g_2b30c);
        g_2b30c = 0;
    }
    if (g_2adc0 != 0) {
        title_09350(g_2adc0);
        g_2adc0 = 0;
    }
    for (off = 0; off < 0x50; off += 4) {
        if (*(int *)((char *)g_2b1e8 + off) != -1 && *(void **)((char *)g_2af70 + off) != 0) {
            title_09350(*(void **)((char *)g_2af70 + off));
            *(void **)((char *)g_2af70 + off) = 0;
        }
    }
    for (p = g_2b118; (int)p < (int)(g_2b118 + 7); p++) {
        if (*p != 0) {
            title_09350(*p);
            *p = 0;
        }
    }
    for (p = g_2b330; (int)p < (int)(g_2b330 + 8); p++) {
        if (*p != 0) {
            title_09350(*p);
            *p = 0;
        }
    }
    if (g_2b014 != 0) {
        title_09350(g_2b014);
        g_2b014 = 0;
    }
    if (g_2b0e8 != 0) {
        title_09350(g_2b0e8);
        g_2b0e8 = 0;
    }
    if (g_2b10c != 0) {
        title_09350(g_2b10c);
        g_2b10c = 0;
    }
    if (g_2b304 != 0) {
        title_09350(g_2b304);
        g_2b304 = 0;
    }
}
