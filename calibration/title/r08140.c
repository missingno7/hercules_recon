/* TITLE.DLL lane w10 region: functions that reached MASKED EQUAL, ascending RVA. */

typedef struct TitleRec {
    unsigned char unknown_00[0x34];
    unsigned short field_34;
} TitleRec;

typedef struct TitleObj {
    int x_00;
    int y_04;
    unsigned char unknown_08[0x2c];
    unsigned short frame_34;
} TitleObj;

extern int g_2a008;
extern int g_2a010;
extern int g_2a024;
extern int g_2a02c;
extern int g_2a034;
extern int g_2a080;
extern int g_2a084;
extern TitleRec *g_2a048;
extern TitleRec *g_2a04c;
extern TitleRec *g_2a050;
extern TitleRec *g_2a054;
extern TitleRec *g_2a058;
extern TitleRec *g_2a05c;
extern TitleRec *g_2a060;
extern TitleRec *g_2a064;
extern TitleRec *g_2a06c;
extern TitleRec *g_2a070;
extern unsigned short g_23af8[];
extern unsigned short g_23668[];
extern unsigned short g_238b0[];
extern unsigned short g_23d40[];
extern unsigned short g_23f88[];

extern int g_2cc64;
extern int g_2cc60;
extern int g_2a044;
extern int g_2a03c;
extern int g_2a020;
extern int g_2a014;
extern int g_2a07c;
extern TitleObj *g_29e08;
extern short g_24230[];
extern short g_241a4[];
extern short g_24278[];
extern short g_241e2[];
extern short g_24202[];
extern short g_242b8[];
extern short g_232b0[];
void title_054f0(int a, int b);
int title_0c4a0(int a);

extern int g_2bffc;
extern int g_2bf7c;
void title_08c90(void);
void title_09090(void);
void title_09480(void);

/* from f_08140.c */
/* from f_08320.c */
/* from f_085b0.c */
void title_08140(void)
{
    int state;
    short neg = -1;

    g_2a050->field_34 = g_23af8[g_2a034] + 0x3c;
    g_2a054->field_34 = g_23668[g_2a02c] + 0x5b;
    g_2a048->field_34 = g_238b0[g_2a024] + 0x7c;
    g_2a04c->field_34 = g_23d40[g_2a008] + 0xaf;
    g_2a058->field_34 = g_23f88[g_2a010] + 0xce;
    g_2a034++;
    g_2a02c++;
    g_2a024++;
    g_2a008++;
    g_2a010++;
    if (++g_2a084 == 0x3c) {
        state = g_2a080;
        g_2a084 = 0;
        if (state == 2) {
            state = 3;
            g_2a080 = state;
        }
        if (state == 6) g_2a080 = 7;
    }
    if ((short)g_23af8[g_2a034] == neg) g_2a034 = 0x17;
    if ((short)g_23668[g_2a02c] == neg) g_2a02c = 0x17;
    if ((short)g_238b0[g_2a024] == neg) g_2a024 = 0x17;
    if ((short)g_23d40[g_2a008] == neg) g_2a008 = 0x17;
    if ((short)g_23f88[g_2a010] == neg) g_2a010 = 0x17;
    g_2a064->field_34 = g_2a050->field_34;
    g_2a05c->field_34 = g_2a054->field_34;
    g_2a060->field_34 = g_2a048->field_34;
    g_2a06c->field_34 = g_2a04c->field_34;
    g_2a070->field_34 = g_2a058->field_34;
}

void title_08320(void)
{
    switch (g_2cc64) {
    case 0:
        g_29e08->frame_34 = g_24230[g_2cc60] + 0x16;
        g_2cc60++;
        if (g_24230[g_2cc60] == -1) g_2cc60 = 4;
        g_29e08->x_00 += 0x30000;
        g_2a044 = g_29e08->x_00 >> 16;
        g_2a03c = g_241a4[g_29e08->frame_34] + (g_29e08->y_04 >> 16) - 0xa9;
        if (g_29e08->x_00 > (int)0xffd00000) {
            g_29e08->frame_34 = 7;
            g_2cc64 = 1;
            g_2cc60 = 0;
        }
        return;
    case 1:
        g_29e08->frame_34 = g_24278[g_2cc60] + 7;
        g_2cc60++;
        if (g_24278[g_2cc60] == -1) {
            g_2cc64 = 2;
            g_2cc60 = 0;
            g_29e08->frame_34 = 0x26;
            g_2a03c += 0x14;
            g_2a020 = 0x80;
            return;
        }
        g_2a044 = g_241e2[g_29e08->frame_34] + (g_29e08->x_00 >> 16);
        g_2a03c = g_24202[g_29e08->frame_34] + (g_29e08->y_04 >> 16) - 0xa9;
        return;
    case 2:
        g_29e08->frame_34 = g_242b8[g_2cc60] + 0x26;
        g_2cc60++;
        if (g_242b8[g_2cc60] == -1) {
            g_2cc64 = 3;
            g_2cc60 = 0;
        }
        break;
    case 3:
        g_29e08->frame_34 = g_232b0[g_2cc60] + 0x26;
        g_2cc60++;
        if (g_232b0[g_2cc60] == -1) g_2cc60 = 0;
        g_29e08->y_04 += 0x60000;
        if (g_29e08->y_04 > 0x1900000) g_2cc64 = 4;
        break;
    default:
        return;
    }
    g_2a044 = 0;
    if (g_2a03c < 0) {
        g_2a03c += 0x14;
        return;
    }
    if (g_2a014 != 0) {
        title_054f0(0x326, 0);
        g_2a014 = 0;
    }
    if (g_29e08->frame_34 < 0x2e) {
        g_2a03c = -title_0c4a0(0x10);
    } else {
        g_2a03c = 0;
        g_2a07c = 2;
    }
}

void title_085b0(void)
{
    g_2bffc = 0;
    g_2bf7c = 0;
    title_08c90();
    title_09090();
    title_09480();
}
