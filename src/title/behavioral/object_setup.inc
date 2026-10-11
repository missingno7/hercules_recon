extern "C" {
/* Minimal 0x94-byte object header view used by the recovered reset routine. */
typedef struct TitleHead {
    unsigned long d00;
    unsigned long d04;
    unsigned long d08;
    unsigned short w0c;
    unsigned short w0e;
    unsigned short w10;
    unsigned short w12;
    unsigned short w14;
    unsigned short w16;
    unsigned short w18;
    unsigned char unknown_1a[2];
    unsigned char b1c;
    unsigned char b1d;
    unsigned char b1e;
    unsigned char b1f;
    unsigned char b20;
    unsigned char b21;
    unsigned char b22;
    unsigned char b23;
    unsigned char b24;
    unsigned char b25;
    unsigned short w26;
    unsigned short w28;
    unsigned short w2a;
    unsigned short w2c;
    unsigned short w2e;
    unsigned short w30;
    short w32;
    unsigned short w34;
    unsigned short w36;
    unsigned short w38;
    unsigned short w3a;
    unsigned short w3c;
    unsigned short w3e;
    unsigned short w40;
    unsigned short w42;
    unsigned short w44;
    unsigned short w46;
    unsigned short w48;
    unsigned short w4a;
    unsigned long d4c;
    unsigned long d50;
    unsigned long d54;
    unsigned long d58;
    unsigned long d5c;
    unsigned long d60;
    unsigned long d64;
    unsigned long d68;
    unsigned long d6c;
    unsigned short w70;
    unsigned short w72;
    unsigned short w74;
    unsigned short w76;
    unsigned char unknown_78[0x10];
    unsigned short w88;
    unsigned short w8a;
    unsigned short w8c;
    unsigned short w8e;
    unsigned short w90;
    unsigned short w92;
} TitleHead;


void title_094c0(void *slot)
{
    TitleHead *p;

    p = (TitleHead *)slot;
    if (p != 0) {
        p->b1d = 0xff;
        p->b1e = 0xff;
        p->b1c = 0xff;
        p->d54 = 0x80000000;
        p->b1f = 0;
        p->b22 = 1;
        p->w30 = 0;
        p->w2e = 0;
        p->d50 = 2;
        p->w32 = -1;
        p->w38 = 0;
        p->w36 = 0;
        p->w34 = 0;
        p->w12 = 0;
        p->w10 = 0;
        p->w0e = 0;
        p->w0c = 0;
        p->w76 = 0;
        p->w74 = 0;
        p->w72 = 0;
        p->w70 = 0;
        p->w3e = 0x100;
        p->w3c = 0x100;
        p->w3a = 0x100;
        p->w8c = 0;
        p->w8a = 0;
        p->w48 = 0xffff;
        p->w88 = 0;
        p->w42 = 0xffff;
        p->w46 = 0xffff;
        p->w44 = 0xffff;
        p->w40 = 0xffff;
        p->w92 = 0;
        p->w90 = 0;
        p->b20 = 0xff;
        p->w8e = 0;
        p->b23 = 0;
        p->w2a = 0;
        p->d6c = 0;
        p->d68 = 0;
        p->d64 = 0;
        p->d58 = 0;
        p->d5c = 0;
        p->d60 = 0;
        p->d08 = 0;
        p->d04 = 0;
        p->d00 = 0;
        p->w4a = 0x80;
        p->b24 = 0x80;
        p->w2c = 0;
        p->w26 = 0;
        p->b21 = 0;
        p->d4c = 0;
        p->b25 = 0;
        p->w18 = 0;
        p->w16 = 0;
        p->w14 = 0;
        p->w28 = 0;
    }
}


/* 0x48-byte configuration row/table view used by title_095d0. Existing TU types: TitleCfg, Rec8, TitleDisp12, TitleDisp8. */
typedef struct TitleObjCfg {
    short id_00;
    unsigned short size134_02;
    unsigned short sizeec_04;
    unsigned short size94_06;
    unsigned short low134_08;
    unsigned short lowec_0a;
    unsigned short low94_0c;
    unsigned short high134_0e;
    unsigned short highec_10;
    unsigned short high94_12;
    unsigned short w14;
    unsigned short count_16;
    TitleCfg *cfg134_18;
    TitleDisp12 *disp134_1c;
    Rec8 *cfgec_20;
    TitleDisp8 *dispec_24;
    Rec8 *cfg94_28;
    TitleDisp8 *disp94_2c;
    unsigned long d30;
    void (**fn_34)(void *p);
    unsigned long d38;
    unsigned long d3c;
    unsigned long d40;
    unsigned long d44;
} TitleObjCfg;

extern TitleObjCfg g_26980[];
extern unsigned char g_2c000;
extern unsigned short g_2bf4c;
extern unsigned long g_2bf68;
extern void (**g_2bf3c)(void *p);
extern unsigned long g_2bf78;
extern unsigned long g_2bf50;
extern unsigned long g_2bf74;


void title_095d0(void)
{
    int k;

    g_2c000 = 0;
    for (k = 0; g_26980[k].id_00 != 999; k++) {
        if (g_26980[k].id_00 == (signed char)((TitleMenuState *)g_engine_interface.context_004)->level_000)
            break;
    }
    g_2bf62 = g_26980[k].lowec_0a;
    g_2bf60 = g_26980[k].high134_0e;
    g_2bf4c = g_26980[k].w14;
    g_2bf6e = g_26980[k].high94_12;
    g_2bf58 = g_26980[k].highec_10;
    g_2bf38 = g_26980[k].count_16;
    g_2bf6c = g_26980[k].size134_02;
    g_2bf40 = g_26980[k].cfg134_18;
    g_2bf72 = g_26980[k].size94_06;
    g_2bff2 = g_26980[k].sizeec_04;
    g_2bf44 = g_26980[k].low94_0c;
    g_2bf32 = g_26980[k].low134_08;
    g_2bf34 = g_26980[k].dispec_24;
    g_2bf70 = g_2bf62;
    g_2bf64 = g_2bf58;
    g_2bf28 = g_2bf60;
    g_2bf30 = g_2bf6e;
    g_2bf68 = g_26980[k].d30;
    g_2bf48 = g_26980[k].cfgec_20;
    g_2bf78 = g_26980[k].d38;
    g_2bf5c = g_26980[k].cfg94_28;
    g_2bf50 = g_26980[k].d40;
    g_2bf2c = g_26980[k].disp134_1c;
    g_2bf80 = g_2bf32;
    g_2bff0 = g_2bf44;
    g_2bf3c = g_26980[k].fn_34;
    g_2bf54 = g_26980[k].disp94_2c;
    g_2bf74 = g_26980[k].d44;
}

}
