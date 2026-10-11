/* Behavioral TITLE data provider. This is provisional source, not
 * recovery acceptance. Native storage starts loader-zero; ordinary typed C
 * definitions supply only observed cells/prefixes. Extent uncertainty is in
 * evidence/title_behavioral_data.json; no native placement is requested. */
#include "../../../calibration/title/title_screen.h"

typedef struct TitlePair8 { unsigned int a_00, b_04; } TitlePair8;
typedef struct TitleTabEntry { unsigned short a_00, b_02; } TitleTabEntry;
typedef struct TitleCell {
    unsigned short mask_00, w_02;
    unsigned short next_04[4];
} TitleCell;
typedef struct TitleRec64 {
    unsigned long unknown_00, unknown_04;
    short id_08, unknown_0a, word_0c, unknown_0e;
    long unknown_10, unknown_14, unknown_18, unknown_1c;
    long unknown_20, unknown_24;
    unsigned char unknown_28[8];
    TitleObject *owner_30;
    unsigned long flags_34;
    long unknown_38;
    unsigned long active_3c;
} TitleRec64;
typedef struct TitleDisp8 {
    void (*fn_00)(void *p), (*fn_04)(void *p);
} TitleDisp8;
typedef struct TitleDisp12 {
    void (*fn_00)(void *p), (*fn_04)(void *p), (*fn_08)(void *p);
} TitleDisp12;
typedef struct Rec8 { unsigned long d0; unsigned char b4,b5,b6,b7; } Rec8;
typedef struct TitleCfg { int a_00,b_04,c_08; unsigned char d_0c; } TitleCfg;

/* 0x40xx checks the 0x400-byte decoder bound. */
unsigned char g_29590[0x400];
volatile int g_2a244;
/* Native 18d83 reads a distinct DWORD beyond r16110's four-byte private
 * g_2acf0. No encoded direct initializer write exists in the original. */
int g_2acf4;
/* Native tally 1ca55 writes one and 1d0fb reads a full DWORD. No current
 * private array covers this distinct minimum cell at the end of its region. */
int g_2b36c;
/* Pair prefix to the next independently used pointer at 0x2bb20.
 * 128 is provisional capacity; the native writer itself has no bounds check. */
TitlePair8 g_2b720[128];
TitlePair8 *g_2bb20;
char *g_2bb24;
int *g_2bb28;
/* Password actor e8cd stores a full DWORD at 2bb38; input boot166e6 reads
 * its low byte. Keep four-byte storage despite the narrower consumer. */
int g_2bb30, g_2bb34, g_2bb38, g_2bb3c;

unsigned short g_2bf28, g_2bf30, g_2bf32, g_2bf38, g_2bf44;
unsigned short g_2bf4c, g_2bf58, g_2bf60, g_2bf62, g_2bf64;
unsigned short g_2bf6c, g_2bf6e, g_2bf70, g_2bf72, g_2bf80;
unsigned short g_2bff0, g_2bff2;
TitleDisp12 *g_2bf2c;
TitleDisp8 *g_2bf34, *g_2bf54;
TitleCfg *g_2bf40;
Rec8 *g_2bf48, *g_2bf5c;
/* Only table+0x34 has a supported function-pointer view; the other writes
 * copy opaque DWORDs from the recovered configuration row. */
unsigned long g_2bf68, g_2bf78, g_2bf50, g_2bf74;
void (**g_2bf3c)(void *p);
int g_2bf7c, g_2bff4, g_2bff8, g_2bffc;
/* Neighbor-bounded prefix, count is supplied by 0x95d0's configuration row. */
long *g_2bfa0[20];
unsigned char g_2c000;
unsigned char g_2c080[18];
/* Count + replay pointers. Native next independent object is at 0x2c144;
 * 41 dwords is a provisional maximum prefix, not established full capacity. */
long g_2c0a0[41];

/* One three-int view; consumer scalar names must be rebound to [1]/[2]. */
int g_2cbe0[3];
/* These each have a separate four-byte load in 18bf0 and begin outside
 * both the center prefix and the existing g_2cbf0 pointer. */
unsigned int *g_2cbec;
char *g_2cbf0;
unsigned char *g_2cbf4;
signed char g_2cc02, g_2cc03;
int g_2cc0c, g_2cc60, g_2cc64, g_2cc68;
int g_2cc6c, g_2cc70, g_2cc74, g_2cc78, g_2cc7c, g_2cc80, g_2cc84;
/* 0x5080 visits 24 records; 0x4fe0 initializes 256 twelve-byte cells. */
TitleRec64 g_2cca0[24];
TitleTabEntry g_2d2a0[32];
TitleObject *g_2d320;
void *g_2d324;
int g_2d328;
void *g_2d32c;
TitleCell g_2d340[256];
void *g_2df40, *g_2df44;
TitleObject *g_2df48;
void *g_2df4c;
unsigned char g_2df50;
unsigned short g_2df60[32];
void *g_2dfa0;
TitleObject *g_2dfa4;
void *g_2dfa8;
unsigned char g_2dfac;
char *g_2dfb0;
TitleObject *g_2dfb4;

/* Fresh historical compiler must agree with the native record strides. */
typedef char pair_size[(sizeof(TitlePair8)==8)?1:-1];
typedef char cell_size[(sizeof(TitleCell)==12)?1:-1];
typedef char rec_size[(sizeof(TitleRec64)==64)?1:-1];
typedef char cfg_size[(sizeof(TitleCfg)==16)?1:-1];
typedef char disp8_size[(sizeof(TitleDisp8)==8)?1:-1];
typedef char disp12_size[(sizeof(TitleDisp12)==12)?1:-1];
