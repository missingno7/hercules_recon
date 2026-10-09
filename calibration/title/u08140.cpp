/* Hypothesis: TITLE.DLL 0x8140..0x94bf is one C++ translation unit.
 * Shared declarations first, then the 14 MASKED EQUAL functions in ascending RVA.
 * No includes are needed: none of the 14 bodies uses the engine interface header. */

extern "C" {

/* ---- shared record types ---- */

typedef struct TitleTab {
    long a;
    long b;
} TitleTab;

typedef struct TitleCfg {
    int a_00;
    int b_04;
    int c_08;
    unsigned char d_0c;
    unsigned char unknown_0d[3];
} TitleCfg;

typedef struct Rec8 {
    unsigned long d0;
    unsigned char b4, b5, b6, b7;
} Rec8;

typedef struct TitleCell94 {
    unsigned char unknown_00[0x94];
} TitleCell94;

typedef struct TitleObj {
    int x_00;                      /* signed in 0x8320; copied as dw00 by 0x8ae0/0x8f50 */
    int y_04;                      /* signed in 0x8320; copied as dw04 by 0x8ae0/0x8f50 */
    unsigned long dw08;
    unsigned char unknown_0c[16];
    unsigned char b1c;
    unsigned char unknown_1d[13];
    unsigned short w2a;
    unsigned short unknown_2c;
    unsigned short w2e;
    unsigned char unknown_30[4];
    unsigned short frame_34;
    unsigned char unknown_36[2];
    unsigned short w38;
    unsigned char unknown_3a[22];
    unsigned long d50;
    unsigned long d54;
    char *p58;
    unsigned char unknown_5c[196];
    void *p120;
} TitleObj;

/* 0x134-byte record: 0x8a50/0x8ed0/0x92e0 record views, 0x8cd0 view, and the
 * 0xec-prefix view used by 0x90f0 (b94..de8). Field offsets are the union of all. */
typedef struct TitleRec {
    unsigned long d00;
    unsigned long d04;
    unsigned long d08;
    unsigned char unknown_0c[0x16];
    unsigned char b22;
    unsigned char unknown_23[0x0b];
    unsigned short w2e;
    unsigned char unknown_30[4];
    unsigned short field_34;
    unsigned char unknown_36[0x2e];
    unsigned long d64;
    unsigned char unknown_68[0x2c];
    unsigned char b94, b95, b96, b97;
    unsigned short w98, w9a;
    unsigned long d9c, da0, da4, da8, dac, db0, db4, db8, dbc, dc0;
    unsigned long dc4, dc8, dcc, dd0, dd4, dd8, ddc, de0, de4, de8;
    unsigned char b0ec, b0ed;
    unsigned short w0ee;
    unsigned long d0f0, d0f4, d0f8, d0fc, d100, d104, d108, d10c, d110;
    short w114;
    unsigned short w116;
    unsigned long d118;
    short w11c;
    unsigned short w11e;
    unsigned long d120;
    unsigned short w124, w126;
    unsigned long d128, d12c, d130;
} TitleRec;

/* ---- globals ---- */

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

extern int g_2bffc;
extern int g_2bf7c;
extern TitleCfg *g_2bf40;
extern TitleTab g_2a0a0[];
extern signed char g_2cc03;
extern char *g_2d320;
extern char *g_2df4c;
extern unsigned short g_2bf6c;
extern unsigned short g_2bff2;
extern unsigned short g_2bf38;
extern long *g_2bfa0[];
extern Rec8 *g_2bf48;
extern Rec8 *g_2bf5c;
extern TitleCell94 *g_2dfa0;
extern unsigned short g_2bf72;

/* ---- callees ---- */

void title_054f0(int a, int b);
int title_0c4a0(int a);
TitleRec *title_08900(int flags);
void *title_08d80(unsigned long flags);
void *title_091a0(unsigned long flags);
void title_094c0(void *slot);
void title_01dd0(void *p);
void title_04610(TitleObj *p);
void unlink_12c(TitleObj *p);
void title_098f0(TitleObj *p);
void clear_command_high_bit(TitleObj *p);
void title_097f0(TitleObj *p);
void title_09830(TitleObj *p);
void title_09870(TitleObj *p);
void unlink_0e4(TitleObj *p);

void title_08c90(void);
void title_09090(void);
void title_09480(void);
void title_085b0(void);
void title_08cd0(void *pv);
void title_090f0(void *pv);
int title_08ae0(TitleObj *p);
int title_08f50(TitleObj *p);
int title_09350(TitleObj *p);

/*@FUNCS@*/

/*@BEGIN_FUNC 0x8140 _title_08140*/
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
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8320 _title_08320*/
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
/*@END_FUNC*/

/*@BEGIN_FUNC 0x85b0 _title_085b0*/
void title_085b0(void)
{
    g_2bffc = 0;
    g_2bf7c = 0;
    title_08c90();
    title_09090();
    title_09480();
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8a50 _title_08a50*/
TitleRec *title_08a50(int a1, int a2, int a3, int a4)
{
    TitleRec *rec;

    rec = title_08900(a4);
    if (rec != 0) {
        title_08cd0(rec);
        rec->w2e = (unsigned short)a4;
        rec->d00 = a1;
        rec->d9c = a1;
        rec->d04 = a2;
        rec->da0 = a2;
        rec->d08 = a3;
        rec->da4 = a3;
        rec->b22 = g_2bf40[a4 & 0xffff].d_0c;
        rec->d64 = g_2bf40[a4 & 0xffff].a_00;
        rec->d110 = g_2bf40[a4 & 0xffff].b_04;
        rec->d118 = g_2bf40[a4 & 0xffff].c_08;
    }
    return rec;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8ae0 _title_08ae0*/
int title_08ae0(TitleObj *p)
{
    char *frame;
    unsigned short index;
    unsigned short flags;

    if (p->w2e & 0x6000)
        return title_08f50(p);
    if (p->w2e == 0)
        return 1;
    if (p->d54 & 0x800) {
        title_098f0(p);
        return 0;
    }
    if (p->d50 & 0x10000000) {
        unlink_12c(p);
        p->w2e = 0;
        title_04610(p);
        return 1;
    }
    if (p->p120)
        title_01dd0(p->p120);
    frame = p->p58;
    if (p->p58 == 0) {
        clear_command_high_bit(p);
        title_097f0(p);
        title_09830(p);
        title_09870(p);
        unlink_12c(p);
        unlink_0e4(p);
        /* Original stores the low word of the (null) frame register. */
        p->w2e = (unsigned short)(unsigned long)frame;
        title_04610(p);
        return 1;
    }
    index = p->w38;
    flags = *(unsigned short *)(frame + g_2a0a0[index].a + 0xc);
    frame += g_2a0a0[index].a;
    if (flags & 0x2000)
        return 0;
    if (flags & 0x100) {
        if (p->b1c != 0xff)
            p->w2a |= (unsigned short)(p->b1c << 8);
        if ((short)index == (short)g_2cc03) {
            *(unsigned short *)(frame + 0xe) = p->w2a;
            *(unsigned long *)(frame + 0) = p->x_00;
            *(unsigned long *)(frame + 4) = p->y_04;
            *(unsigned long *)(frame + 8) = p->dw08;
        }
    }
    if (*(unsigned char *)(frame + 0xc) & 0x80) {
        if (p->b1c != 0xff)
            p->w2a |= (unsigned short)(p->b1c << 8);
        if ((short)p->w38 == (short)g_2cc03)
            *(unsigned short *)(frame + 0xe) = p->w2a;
    }
    clear_command_high_bit(p);
    title_09830(p);
    title_09870(p);
    title_097f0(p);
    unlink_12c(p);
    unlink_0e4(p);
    p->w2e = 0;
    *(unsigned short *)(frame + 0xc) &= 0x7fff;
    title_04610(p);
    return 1;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8c90 _title_08c90*/
void title_08c90(void)
{
    int i;
    char *base;

    i = 0;
    if (g_2bf6c > i) {
        int off;
        int n;

        off = 0;
        do {
            base = g_2d320;
            title_08cd0(base + off);
            n = g_2bf6c;
            i++;
            off += 0x134;
        } while (i < n);
    }
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8cd0 _title_08cd0*/
void title_08cd0(void *pv)
{
    TitleRec *p;
    unsigned long z;
    long m;

    p = (TitleRec *)pv;

    title_090f0(p);
    z = 0;
    m = -1;
    p->b0ed = (unsigned char)z;
    p->b0ec = (unsigned char)z;
    p->w0ee = 1;
    p->d0f8 = z;
    p->d0f4 = z;
    p->d0f0 = z;
    p->d104 = z;
    p->d100 = z;
    p->d0fc = z;
    p->d10c = z;
    p->d108 = z;
    p->d110 = z;
    p->w114 = m;
    p->w116 = (unsigned short)z;
    p->d118 = z;
    p->w11c = m;
    p->w11e = (unsigned short)z;
    p->d120 = z;
    p->w124 = (unsigned short)z;
    p->w126 = (unsigned short)z;
    p->d130 = z;
    p->d12c = z;
    p->d128 = z;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8ed0 _title_08ed0*/
TitleRec *title_08ed0(unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4)
{
    TitleRec *p = (TitleRec *)title_08d80(a4);
    if (p != 0) {
        title_090f0(p);
        p->w2e = (unsigned short)(a4 | 0x4000);
        p->d00 = a1;
        p->d9c = a1;
        p->d04 = a2;
        p->da0 = a2;
        p->d08 = a3;
        p->da4 = a3;
        p->b22 = g_2bf48[a4 & 0xfff].b6;
        p->d64 = g_2bf48[a4 & 0xfff].d0;
    }
    return p;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8f50 _title_08f50*/
int title_08f50(TitleObj *p)
{
    char *edi;
    unsigned short ax;
    unsigned short cx;

    if (p->w2e == 0)
        return 1;
    if (p->w2e & 0x4000) {
        if (p->d50 & 0x10000000) {
            p->w2e = 0;
            title_04610(p);
            return 1;
        }
        edi = p->p58;
        if (p->p58 == 0) {
            title_09830(p);
            title_097f0(p);
            title_09870(p);
            unlink_0e4(p);
            p->w2e = (unsigned short)(unsigned long)edi;
            title_04610(p);
            return 1;
        }
        ax = p->w38;
        cx = *(unsigned short *)(edi + g_2a0a0[ax].a + 0xc);
        edi += g_2a0a0[ax].a;
        if (cx & 0x2000)
            return 0;
        if (cx & 0x100) {
            if ((short)ax == (short)g_2cc03) {
                *(unsigned long *)(edi + 0) = p->x_00;
                *(unsigned long *)(edi + 4) = p->y_04;
                *(unsigned long *)(edi + 8) = p->dw08;
                *(unsigned short *)(edi + 0xe) = p->w2a;
            }
        }
        if (*(unsigned char *)&cx & 0x80) {
            if ((short)p->w38 == (short)g_2cc03) {
                cx = p->w2a;
                *(unsigned short *)(edi + 0xe) = cx;
            }
        }
        title_09830(p);
        title_097f0(p);
        title_09870(p);
        unlink_0e4(p);
        p->w2e = 0;
        *(unsigned short *)(edi + 0xc) &= 0x7fff;
        title_04610(p);
        return 1;
    }
    return title_09350(p);
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x9090 _title_09090*/
void title_09090(void)
{
    char *base;
    int i;

    i = 0;
    if (g_2bff2 > i) {
        int off;
        int n;

        off = 0;
        do {
            base = g_2df4c;
            title_090f0(base + off);
            n = g_2bff2;
            i++;
            off += 0xec;
        } while (i < n);
    }
    i = 0;
    if (g_2bf38 > i) {
        do {
            *g_2bfa0[i] = -1;
            i++;
        } while (i < g_2bf38);
    }
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x90f0 _title_090f0*/
void title_090f0(void *pv)
{
    TitleRec *p;
    unsigned long z;

    p = (TitleRec *)pv;

    title_094c0(p);
    z = 0;
    p->b94 = (unsigned char)z;
    p->b96 = (unsigned char)z;
    p->b95 = (unsigned char)z;
    p->b97 = (unsigned char)z;
    p->w9a = (unsigned short)z;
    p->w98 = (unsigned short)z;
    p->da4 = z;
    p->da0 = z;
    p->d9c = z;
    p->db4 = z;
    p->db0 = z;
    p->dac = z;
    p->da8 = z;
    p->dc4 = z;
    p->dc0 = z;
    p->dbc = z;
    p->db8 = z;
    p->dd4 = z;
    p->dd0 = z;
    p->dcc = z;
    p->dc8 = z;
    p->de0 = z;
    p->ddc = z;
    p->dd8 = z;
    p->de8 = z;
    p->de4 = z;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x92e0 _title_092e0*/
TitleRec *title_092e0(unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4)
{
    TitleRec *p = (TitleRec *)title_091a0(a4);
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
/*@END_FUNC*/

/*@BEGIN_FUNC 0x9350 _title_09350*/
int title_09350(TitleObj *p)
{
    char *edi;
    unsigned short ax;
    unsigned short cx;

    if (p == 0)
        return 1;
    if (p->w2e == 0)
        return 1;
    if (p->w2e & 0x2000) {
        if (p->d50 & 0x10000000) {
            p->w2e = 0;
            title_04610(p);
            return 1;
        }
        edi = p->p58;
        if (p->p58 == 0) {
            title_09830(p);
            title_09870(p);
            title_097f0(p);
            p->w2e = (unsigned short)(unsigned long)edi;
            title_04610(p);
            return 1;
        }
        ax = p->w38;
        cx = *(unsigned short *)(edi + g_2a0a0[ax].a + 0xc);
        edi += g_2a0a0[ax].a;
        if (cx & 0x2000)
            return 0;
        if (cx & 0x100) {
            if ((short)ax == (short)g_2cc03) {
                *(unsigned long *)(edi + 0) = p->x_00;
                *(unsigned long *)(edi + 4) = p->y_04;
                *(unsigned long *)(edi + 8) = p->dw08;
                *(unsigned short *)(edi + 0xe) = p->w2a;
            }
        }
        if (*(unsigned char *)&cx & 0x80) {
            if ((short)p->w38 == (short)g_2cc03) {
                cx = p->w2a;
                *(unsigned short *)(edi + 0xe) = cx;
            }
        }
        title_09830(p);
        title_09870(p);
        title_097f0(p);
        p->w2e = 0;
        *(unsigned short *)(edi + 0xc) &= 0x7fff;
        title_04610(p);
        return 1;
    }
    return title_08ae0(p);
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x9480 _title_09480*/
void title_09480(void)
{
    int i;
    for (i = 0; i < g_2bf72; i++)
        title_094c0(&g_2dfa0[i]);
}
/*@END_FUNC*/

} /* extern "C" */
