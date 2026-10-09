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

/* Display object (sprite) record. */
typedef struct TitleObject {
    long unknown_000;                 /* x, 16.16 fixed point (signed) */
    long unknown_004;                 /* y, 16.16 fixed point (signed) */
    unsigned char unknown_008[0x00c - 0x008];
    unsigned char unknown_00c[0x01f - 0x00c];
    unsigned char unknown_01f;
    unsigned char unknown_020[0x023 - 0x020];
    unsigned char unknown_023;
    unsigned char unknown_024[0x034 - 0x024];
    unsigned short unknown_034;
    unsigned char unknown_036[0x03e - 0x036];
    unsigned short unknown_03e;
    unsigned char unknown_040[0x04a - 0x040];
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x054 - 0x04c];
    unsigned long unknown_054;
    unsigned char unknown_058[0x070 - 0x058];
    unsigned short unknown_070;
    unsigned short unknown_072;
    unsigned short unknown_074;
    unsigned char unknown_076[0x120 - 0x076];
    int *unknown_120;
} TitleObject;

#endif
