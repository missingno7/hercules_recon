typedef struct TitleObject {
    unsigned long unknown_000;
    unsigned long unknown_004;
    unsigned char unknown_008[0x23 - 0x08];
    unsigned char unknown_023;
    unsigned char unknown_024[0x34 - 0x24];
    unsigned short unknown_034;
    unsigned char unknown_036[0x3e - 0x36];
    unsigned short unknown_03e;
    unsigned char unknown_040[0x4a - 0x40];
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x54 - 0x4c];
    unsigned long unknown_054;
    unsigned char unknown_058[0x74 - 0x58];
    unsigned short unknown_074;
} TitleObject;

extern TitleObject *g_29e08;
extern TitleObject *g_29ff4;
extern TitleObject *g_29ffc;
extern TitleObject *g_2a000;
extern TitleObject *g_2a004;
extern TitleObject *g_2a048;
extern TitleObject *g_2a04c;
extern TitleObject *g_2a050;
extern TitleObject *g_2a054;
extern TitleObject *g_2a058;
extern TitleObject *g_2a05c;
extern TitleObject *g_2a060;
extern TitleObject *g_2a064;
extern TitleObject *g_2a06c;
extern TitleObject *g_2a070;
extern int g_2a008;
extern int g_2a010;
extern int g_2a024;
extern int g_2a02c;
extern int g_2a034;
extern int g_2cc64;

TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4);

void title_06bb0(void)
{
    g_2a004 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a004->unknown_054 |= 5;
    g_29ff4 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_29ff4->unknown_054 |= 6;
    g_2a000 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a000->unknown_054 |= 5;
    g_29ffc = title_17ad0(0, 0, 0, 0x2002, 0);
    g_29ffc->unknown_054 |= 6;
    g_2a000->unknown_034 = 1;
    g_29ffc->unknown_034 = 1;
    g_29e08 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_29e08->unknown_034 = 0x16;
    g_29e08->unknown_000 = 0xfea20000;
    g_29e08->unknown_004 = 0x500000;
    g_2cc64 = -1;

    g_2a050 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a050->unknown_034 = 0x3d;
    g_2a054 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a054->unknown_034 = 0x5c;
    g_2a048 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a048->unknown_034 = 0x7d;
    g_2a04c = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a04c->unknown_034 = 0xb0;
    g_2a058 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a058->unknown_034 = 0xcf;
    g_2a064 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a064->unknown_034 = 0x3d;
    g_2a05c = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a05c->unknown_034 = 0x5c;
    g_2a060 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a060->unknown_034 = 0x7d;
    g_2a06c = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a06c->unknown_034 = 0xb0;
    g_2a070 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a070->unknown_034 = 0xcf;

    g_29e08->unknown_023 = 6;
    g_29e08->unknown_03e = 0x1e;
    g_2a048->unknown_023 = 6;
    g_2a048->unknown_03e = 0x18;
    g_2a050->unknown_023 = 6;
    g_2a050->unknown_03e = 0x17;
    g_2a058->unknown_023 = 6;
    g_2a058->unknown_03e = 0x14;
    g_2a054->unknown_023 = 6;
    g_2a054->unknown_03e = 0x15;
    g_2a04c->unknown_023 = 6;
    g_2a04c->unknown_03e = 0x16;
    g_2a064->unknown_023 = 6;
    g_2a064->unknown_03e = 0x1e;
    g_2a05c->unknown_023 = 6;
    g_2a05c->unknown_03e = 0x1e;
    g_2a060->unknown_023 = 6;
    g_2a060->unknown_03e = 0x1e;
    g_2a06c->unknown_023 = 6;
    g_2a06c->unknown_03e = 0x1e;
    g_2a070->unknown_023 = 6;
    g_2a070->unknown_03e = 0x1e;

    g_2a064->unknown_054 |= 4;
    g_2a05c->unknown_054 |= 4;
    g_2a060->unknown_054 |= 4;
    g_2a06c->unknown_054 |= 4;
    g_2a070->unknown_054 |= 4;

    g_2a060->unknown_074 = 0x80;
    g_2a064->unknown_074 = 0x70;
    g_2a070->unknown_074 = 0xff80;
    g_2a05c->unknown_074 = 0xff90;
    g_2a06c->unknown_074 = 0xffa0;

    g_2a070->unknown_04a = 0;
    g_2a06c->unknown_04a = 0;
    g_2a060->unknown_04a = 0;
    g_2a05c->unknown_04a = 0;
    g_2a064->unknown_04a = 0;

    g_2a004->unknown_023 = 6;
    g_2a004->unknown_03e = 10;
    g_29ff4->unknown_023 = 6;
    g_29ff4->unknown_03e = 0xf;
    g_2a000->unknown_023 = 6;
    g_2a000->unknown_03e = 10;
    g_29ffc->unknown_023 = 6;
    g_29ffc->unknown_03e = 0xf;

    g_2a010 = 0;
    g_2a008 = 0;
    g_2a024 = 0;
    g_2a02c = 0;
    g_2a034 = 0;
}
