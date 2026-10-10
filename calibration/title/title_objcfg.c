/* TITLE .data 0x26770..0x269c7: object-system type configuration (data-only object linked after the
   last code unit). Read by 0x95d0 (row g_26980[k] selected by level; 999 terminates/defaults) and
   the allocators 0x85b0..0x9480: per pool a handler table (create, update[, second pass]) and an
   8-byte configuration table. Generated from the original bytes; table extents from the rows
   pointers; names are address labels. */
void title_01dd0(void *p);
void title_0d250(void *p);
void title_0d290(void *p);
void title_0d410(void *p);
void title_0d440(void *p);
void title_0d910(void *p);

typedef struct TitleDisp8 {
    void (*fn_00)(void *p);
    void (*fn_04)(void *p);
} TitleDisp8;
typedef struct TitleDisp12 {
    void (*fn_00)(void *p);
    void (*fn_04)(void *p);
    void (*fn_08)(void *p);
} TitleDisp12;
typedef struct Rec8 {
    unsigned long d0;
    unsigned char b4, b5, b6, b7;
} Rec8;
typedef struct TitleCfg TitleCfg;
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

extern TitleCfg g_2b378[];
extern Rec8 g_2b3b8[];

TitleDisp12 g_26770[4] = {
    {title_01dd0, title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0, title_01dd0},
};
TitleDisp8 g_267a0[4] = {
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
};
Rec8 g_267c0[27] = {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0},
    {0, 0, 0, 1, 0},
    {0, 0, 0, 2, 0},
    {0, 0, 0, 3, 0},
    {0, 0, 0, 3, 0},
    {0, 0, 0, 3, 0},
    {0, 0, 0, 3, 0},
    {0, 0, 0, 4, 0},
    {0, 0, 0, 5, 0},
    {0, 0, 0, 6, 0},
    {0, 0, 0, 7, 0},
    {0, 0, 0, 8, 0},
    {0, 0, 0, 9, 0},
    {0, 0, 0, 10, 0},
    {0, 0, 0, 11, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 12, 0},
    {0, 0, 0, 13, 0},
    {0, 0, 0, 14, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 15, 0},
    {0, 0, 0, 16, 0},
    {0, 0, 0, 17, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
};
TitleDisp8 g_26898[26] = {
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_0d910},
    {title_01dd0, title_01dd0},
    {title_0d250, title_0d290},
    {title_0d410, title_0d440},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
    {title_01dd0, title_01dd0},
};
void (*g_26968[6])(void *p) = {
    title_01dd0, title_01dd0, title_01dd0, title_01dd0, title_01dd0, 0,
};
TitleObjCfg g_26980[1] = {
    {999, 0x1, 0x1, 0x80, 0x1, 0x1, 0x1, 0x1, 0x1, 0x80, 0x0, 0x0,
     g_2b378, g_26770, g_2b3b8, g_267a0, g_267c0, g_26898, 0, g_26968, 0, 0, 0, 0},
};
