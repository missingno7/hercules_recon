extern "C" {
typedef struct TitleObj {
    unsigned long dw00;
    unsigned long dw04;
    unsigned long dw08;
    unsigned char unknown_0c[16];
    unsigned char b1c;
    unsigned char unknown_1d[13];
    unsigned short w2a;
    unsigned short unknown_2c;
    unsigned short w2e;
    unsigned char unknown_30[8];
    unsigned short w38;
    unsigned char unknown_3a[22];
    unsigned long d50;
    unsigned long d54;
    char *p58;
    unsigned char unknown_5c[196];
    void *p120;
} TitleObj;

typedef struct TitleTab {
    long a;
    long b;
} TitleTab;

extern TitleTab g_2a0a0[];
extern signed char g_2cc03;

int title_09350(TitleObj *p);
int title_08ae0(TitleObj *p);
void title_09830(TitleObj *p);
void title_097f0(TitleObj *p);
void title_09870(TitleObj *p);
void unlink_0e4(TitleObj *p);
void title_04610(TitleObj *p);

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
                *(unsigned long *)(edi + 0) = p->dw00;
                *(unsigned long *)(edi + 4) = p->dw04;
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
                *(unsigned long *)(edi + 0) = p->dw00;
                *(unsigned long *)(edi + 4) = p->dw04;
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
}
