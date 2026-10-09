/* TITLE display buffers: the PlayStation double-buffer layout kept by the PC port.
   Offsets are measured from the users: the drawing environment's dtd/isbg/r0/g0/b0 bytes
   (+0x16..+0x1b) and the display environment's screen rectangle (+0x64..+0x6a) are set for
   both buffers 0x1478 bytes apart, the ordering table at +0x70 is cleared with 0x500 entries,
   and the pair is allocated as one 0x28f0-byte block. Layouts follow the PSX libgpu
   DRAWENV/DISPENV records; field names are those records' names, not recovered identifiers. */
#ifndef TITLE_GPU_H
#define TITLE_GPU_H

typedef struct TitleRect {
    short x, y, w, h;
} TitleRect;

typedef struct TitleDrawEnv {         /* PSX DRAWENV, 0x5c bytes */
    TitleRect clip;                   /* 0x00 */
    short ofs[2];                     /* 0x08 */
    TitleRect tw;                     /* 0x0c */
    unsigned short tpage;             /* 0x14 */
    unsigned char dtd;                /* 0x16 */
    unsigned char dfe;                /* 0x17 */
    unsigned char isbg;               /* 0x18 */
    unsigned char r0, g0, b0;         /* 0x19 */
    unsigned long dr_env[16];         /* 0x1c */
} TitleDrawEnv;

typedef struct TitleDispEnv {         /* PSX DISPENV, 0x14 bytes */
    TitleRect disp;                   /* 0x00 */
    TitleRect screen;                 /* 0x08 */
    unsigned char isinter;            /* 0x10 */
    unsigned char isrgb24;            /* 0x11 */
    unsigned char pad0, pad1;
} TitleDispEnv;

#define TITLE_OT_SIZE 0x500

typedef struct TitleBuffer {          /* one display buffer, 0x1478 bytes */
    TitleDrawEnv draw;                /* 0x0000 */
    TitleDispEnv disp;                /* 0x005c */
    unsigned long ot[TITLE_OT_SIZE];  /* 0x0070 ordering table */
    unsigned char unknown_1470[8];    /* 0x1470 */
} TitleBuffer;

extern TitleBuffer *g_2cc04;          /* buffer being drawn */
extern TitleBuffer *g_2cc08;          /* both buffers (allocated together) */

#endif
