/* TITLE.DLL lane w18 region: functions that reached MASKED EQUAL, ascending RVA. */
#include "title_engine.h"

typedef struct TitleSprite {
    int field_00;
    int field_04;
    unsigned char unknown_08[0x23 - 0x08];
    unsigned char field_23;
    unsigned char unknown_24[0x34 - 0x24];
    unsigned short field_34;
    unsigned char unknown_36[0x3e - 0x36];
    unsigned short field_3e;
    unsigned char unknown_40[0x4a - 0x40];
    unsigned short field_4a;
    unsigned char unknown_4c[0x54 - 0x4c];
    unsigned int field_54;
} TitleSprite;

typedef struct TitleRect {
    int a;
    int b;
    int c;
    int d;
} TitleRect;

typedef struct TitleCtxView {
    signed char kind_00;
    unsigned char unknown_01[0x0b - 0x01];
    signed char field_0b;
    signed char field_0c;
    unsigned char unknown_0d;
    signed char field_0e;
    unsigned char unknown_0f;
    unsigned char field_10;
    unsigned char field_11;
    unsigned char unknown_12[0x30 - 0x12];
    int field_30;
    unsigned char unknown_34[0x6c - 0x34];
    TitleRect rect_6c;
    unsigned char unknown_7c[0x1b0c - 0x7c];
    int field_1b0c;
    int field_1b10;
    unsigned char unknown_1b14[0x1b24 - 0x1b14];
    int field_1b24;
} TitleCtxView;

typedef struct TitleObj {
    unsigned char field_00;
    unsigned char field_01;
    unsigned char field_02;
    unsigned char field_03;
    int field_04;
    int field_08;
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
    TitleRect rect_20;
    int field_30;
    int field_34;
    unsigned char field_38;
    unsigned char field_39;
    unsigned char field_3a;
    unsigned char field_3b;
} TitleObj;


int title_06670(void);
void title_0c4f0(int a);
void title_0c2e0(void);
TitleSprite *title_17ad0(int a, int b, int c, int d, int e);

void title_12070(TitleObj *obj)
{
    switch (((TitleCtxView *)g_engine_interface.context_004)->kind_00) {
    case 1: obj->field_04 = 2; break;
    case 2: obj->field_04 = 3; break;
    case 3: obj->field_04 = 5; break;
    case 4: break;
    case 5: obj->field_04 = 6; break;
    case 6: obj->field_04 = 7; break;
    case 7: obj->field_04 = 8; break;
    case 8: obj->field_04 = 9; break;
    case 9: obj->field_04 = 10; break;
    case 10: obj->field_04 = 11; break;
    case 11: obj->field_04 = 12; break;
    }
    obj->field_34 = ((TitleCtxView *)g_engine_interface.context_004)->field_1b24;
    obj->field_08 = ((TitleCtxView *)g_engine_interface.context_004)->field_0c;
    obj->field_0c = ((TitleCtxView *)g_engine_interface.context_004)->field_0b;
    obj->field_10 = ((TitleCtxView *)g_engine_interface.context_004)->field_0e;
    obj->field_14 = ((TitleCtxView *)g_engine_interface.context_004)->field_10;
    obj->field_30 = ((TitleCtxView *)g_engine_interface.context_004)->field_30;
    obj->field_18 = ((TitleCtxView *)g_engine_interface.context_004)->field_1b0c;
    obj->field_1c = ((TitleCtxView *)g_engine_interface.context_004)->field_1b10;
    obj->rect_20 = ((TitleCtxView *)g_engine_interface.context_004)->rect_6c;
    obj->field_00 = 0;
    obj->field_01 = 1;
    obj->field_02 = 2;
    obj->field_03 = 3;
    obj->field_38 = 4;
    obj->field_39 = 5;
    obj->field_3a = 6;
    obj->field_3b = 7;
}

void title_121d0(TitleObj *obj)
{
    ((TitleCtxView *)g_engine_interface.context_004)->field_11 = 0;
    ((TitleCtxView *)g_engine_interface.context_004)->field_1b24 = obj->field_34;
    ((TitleCtxView *)g_engine_interface.context_004)->kind_00 = obj->field_04;
    ((TitleCtxView *)g_engine_interface.context_004)->field_0c = obj->field_08;
    ((TitleCtxView *)g_engine_interface.context_004)->field_0b = obj->field_0c;
    ((TitleCtxView *)g_engine_interface.context_004)->field_0e = obj->field_10;
    ((TitleCtxView *)g_engine_interface.context_004)->field_10 = obj->field_14;
    ((TitleCtxView *)g_engine_interface.context_004)->field_30 = obj->field_30;
    ((TitleCtxView *)g_engine_interface.context_004)->field_1b0c = obj->field_18;
    ((TitleCtxView *)g_engine_interface.context_004)->field_1b10 = obj->field_1c;
    ((TitleCtxView *)g_engine_interface.context_004)->rect_6c = obj->rect_20;
    title_0c2e0();
}

