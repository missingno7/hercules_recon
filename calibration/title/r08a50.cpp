extern "C" {
typedef struct TitleSlot {
    int field_00;
    int field_04;
    int field_08;
    unsigned char unknown_0c[0x16];
    unsigned char field_22;
    unsigned char unknown_23[0x0b];
    unsigned short type_2e;
    unsigned char unknown_30[0x34];
    int field_64;
    unsigned char unknown_68[0x34];
    int field_9c;
    int field_a0;
    int field_a4;
    unsigned char unknown_a8[0x68];
    int field_110;
    int field_114;
    int field_118;
    unsigned char unknown_11c[0x18];
} TitleSlot;

typedef struct TitleCfg {
    int a_00;
    int b_04;
    int c_08;
    unsigned char d_0c;
    unsigned char unknown_0d[3];
} TitleCfg;

extern TitleCfg *g_2bf40;
TitleSlot *title_08900(int flags);
void title_08cd0(TitleSlot *rec);

TitleSlot *title_08a50(int a1, int a2, int a3, int a4)
{
    TitleSlot *rec;

    rec = title_08900(a4);
    if (rec != 0) {
        title_08cd0(rec);
        rec->type_2e = (unsigned short)a4;
        rec->field_00 = a1;
        rec->field_9c = a1;
        rec->field_04 = a2;
        rec->field_a0 = a2;
        rec->field_08 = a3;
        rec->field_a4 = a3;
        rec->field_22 = g_2bf40[a4 & 0xffff].d_0c;
        rec->field_64 = g_2bf40[a4 & 0xffff].a_00;
        rec->field_110 = g_2bf40[a4 & 0xffff].b_04;
        rec->field_118 = g_2bf40[a4 & 0xffff].c_08;
    }
    return rec;
}
}
