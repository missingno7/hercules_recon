/* Records shared by the TITLE screen units (sequence screens and their helpers). */
#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "title_proc.h"

/* Screen context (g_engine_interface.context_004). The handlers read both cursor fields into
   unused locals; the reads are volatile in the original (see docs/title_link_goal.md). */
typedef struct ScreenContext {
    unsigned char unknown_00[0x5c];
    volatile short cursor_5c;
    volatile short cursor_5e;
} ScreenContext;

/* Display object (sprite/actor) record, 0x134 bytes: allocated in a pool of 0x134-byte entries
   (0x1d380). Members are the union of what the reconstructed users touch; unknown_* names keep
   their offsets until a meaning is evidenced. */
typedef struct TitleObject TitleObject;
struct TitleObject {
    long unknown_000;                 /* x, 16.16 fixed point (signed) */
    long unknown_004;                 /* y, 16.16 fixed point (signed) */
    long unknown_008;                 /* z */
    short unknown_00c;
    short unknown_00e;
    short unknown_010;                /* depth: ordering-table slot */
    unsigned char unknown_012[0x014 - 0x012];
    short unknown_014;
    short unknown_016;
    short unknown_018;
    unsigned char unknown_01a[0x01c - 0x01a];
    unsigned char unknown_01c;
    unsigned char unknown_01d;
    unsigned char unknown_01e;
    unsigned char unknown_01f;
    unsigned char unknown_020[0x022 - 0x020];
    unsigned char unknown_022;
    unsigned char unknown_023;        /* render mode */
    unsigned char unknown_024[0x02e - 0x024];
    unsigned short unknown_02e;
    unsigned char unknown_030[0x034 - 0x030];
    unsigned short unknown_034;       /* frame */
    unsigned short unknown_036;
    unsigned char unknown_038[0x03a - 0x038];
    unsigned short unknown_03a;
    unsigned short unknown_03c;
    unsigned short unknown_03e;
    unsigned short unknown_040;
    unsigned short unknown_042;
    unsigned short unknown_044;
    unsigned short unknown_046;
    unsigned short unknown_048;
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x050 - 0x04c];
    unsigned long unknown_050;
    unsigned long unknown_054;        /* render flags (0x10 mirrors x movement in 0x5cc0) */
    void *unknown_058;
    TitleObject *unknown_05c;
    TitleObject *unknown_060;
    unsigned long unknown_064;
    TitleObject *unknown_068;         /* next object in a list */
    void *unknown_06c;
    unsigned short unknown_070;
    unsigned short unknown_072;
    unsigned short unknown_074;
    unsigned char unknown_076[0x120 - 0x076];
    int *unknown_120;
    unsigned char unknown_124[0x12c - 0x124];
    unsigned long unknown_12c;
    unsigned long unknown_130;
};

#endif
