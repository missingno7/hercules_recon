#include <string.h>
#include <io.h>
#include "title_engine.h"
#include "title_files.h"


static int g_29fc0;
/* Memory-card communals of this unit (.bss 0x2c144..0x2cbcf), defined here: the software and hardware
   card events tested by 0x6480/0x6530 (IOE, error, timeout, new card), the card directory (15 entries of
   40 bytes, walked by 0x65e0), and the load and save card images (0x400 bytes each, written whole by
   0x6a50/0x6840; the data half starts at +0x200). Names follow the PSX card sample conventions and were
   chosen so VC5's identifier-hash order reproduces the original layout (scripts/layout_names.py). */
typedef struct TitleDirEntry {
    char name[20];
    long attr;
    long size;
    struct TitleDirEntry *next;
    long head;
    char system[4];
} TitleDirEntry;
int ev0, ev1, ev2, ev3;
TitleDirEntry card_dir[15];
int load_image[0x100];
int save_image[0x100];
int ev10, ev11, ev12, ev13;
static int g_29fbc;
int title_0c610(int a);
int title_0c600(int a);
int title_06480(void);
void title_065a0(void);
int title_06530(void);
int title_0cc60(int a);
int title_0c630(int a, int b, char *c);
int title_0c710(int a, int b);
int title_0c900(int a);
int title_065e0(int a);
int title_06430(int a);
void title_0c5f0(void);
int title_0c730(int a);
extern int title_06480(void);
extern void title_064f0(void);
extern void title_065a0(void);
/* Initialized data of this unit (TITLE.DLL .data 0x23368..0x2348b), in address order: the memory-card
   icon palette and pixels used by 0x66f0 (pixels one per byte, extent to the first literal). */
unsigned short g_23368[16] = {0x2d6b, 0x2d8d, 0x2d8e, 0x29b0, 0x29d1, 0x2df1, 0x2a14, 0x3255, 0x3256, 0x3676, 0x3697, 0x3699, 0x3ad9, 0x3efa, 0x473b, 0x4b7d};
unsigned char g_23388[260] = {
    0, 0, 0, 0, 2, 7, 14, 15, 15, 14, 7, 2, 0, 0, 0, 0,
    0, 0, 0, 5, 13, 14, 15, 15, 15, 15, 15, 14, 5, 0, 0, 0,
    0, 0, 5, 12, 11, 11, 11, 11, 11, 10, 12, 14, 15, 5, 0, 0,
    0, 5, 12, 11, 10, 12, 14, 15, 15, 14, 9, 11, 14, 14, 5, 0,
    2, 12, 11, 8, 12, 13, 13, 12, 12, 13, 13, 9, 10, 13, 13, 2,
    7, 11, 6, 6, 11, 11, 10, 9, 9, 12, 14, 13, 7, 8, 13, 7,
    12, 11, 3, 4, 6, 6, 9, 14, 13, 13, 13, 12, 9, 4, 12, 12,
    11, 11, 3, 6, 6, 6, 7, 11, 12, 9, 8, 7, 7, 4, 11, 12,
    9, 11, 4, 8, 7, 6, 10, 10, 10, 5, 4, 6, 7, 6, 10, 11,
    8, 9, 3, 6, 7, 6, 12, 13, 14, 13, 12, 10, 8, 6, 8, 8,
    6, 8, 6, 4, 6, 6, 6, 10, 14, 14, 11, 7, 7, 7, 8, 6,
    2, 8, 7, 4, 6, 6, 6, 9, 13, 9, 6, 6, 6, 7, 8, 2,
    0, 2, 8, 7, 4, 4, 6, 8, 7, 3, 3, 4, 7, 8, 2, 0,
    0, 0, 2, 9, 8, 6, 4, 4, 3, 3, 6, 8, 9, 2, 0, 0,
    0, 0, 0, 2, 10, 11, 12, 13, 13, 13, 12, 10, 2, 0, 0, 0,
    0, 0, 0, 0, 2, 6, 11, 13, 14, 12, 6, 2, 0, 0, 0, 0,
    0, 0, 0, 0,
};
/* PlayStation memory-card file header (title frame). */
typedef struct CardHeader {
    char magic[2];
    unsigned char icon_flag;
    unsigned char block_count;
    unsigned short title[32];
    unsigned char reserved[0x1c];
    unsigned short clut[16];
    unsigned char icon[16][8];
} CardHeader;

extern unsigned short g_23368[16];
extern unsigned char g_23389[];

void title_066f0(CardHeader *header, int blank);


extern void title_0c620(int x);
extern void title_01dd0(int a);
extern void title_02090(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
extern void title_0c2a0(char *s, char *name, int b, int c, int d);
extern void title_0c8b0(int a1, int a2, int a3, int a4, int a5, int a6, int a7);

int title_06350(int a)
{
    int r;

    if (g_29fc0 == 0) return -1;
    while (title_0c610(a) == 0) ;
    r = title_06480();
    if (r == 0) return 1;
    if (r == 1) return 3;
    if (r == 2) return 0;
    if (r == 3) {
        title_065a0();
        title_0c600(a);
        title_06530();
        return 0;
    }
    return r;
}

int title_063c0(int a)
{
    char buf[0x80];
    int r;

    title_065a0();
    title_0c630(a, 0, buf);
    r = title_06530();
    if (r == 1) return 3;
    if (r == 2) return 0;
    if (buf[0] == 'M' && buf[1] == 'C') return 2;
    return 4;
}

int title_06480(void)
{
    while (1) {
        if (title_0cc60(ev0) == 1) return 0;
        if (title_0cc60(ev1) == 1) return 1;
        if (title_0cc60(ev2) == 1) return 2;
        if (title_0cc60(ev3) == 1) return 3;
    }
}

void title_064f0(void)
{
    title_0cc60(ev0);
    title_0cc60(ev1);
    title_0cc60(ev2);
    title_0cc60(ev3);
}

int title_06530(void)
{
    while (1) {
        if (title_0cc60(ev10) == 1) return 0;
        if (title_0cc60(ev11) == 1) return 1;
        if (title_0cc60(ev12) == 1) return 2;
        if (title_0cc60(ev13) == 1) return 3;
    }
}

void title_065a0(void)
{
    title_0cc60(ev10);
    title_0cc60(ev11);
    title_0cc60(ev12);
    title_0cc60(ev13);
}

int title_065e0(int a)
{
    char buf[0x80];
    int i;

    strcpy(buf, "bu00:");
    strcat(buf, "*");
    i = 0;
    if (title_0c710((int)buf, a) == a) {
        do {
            a += 0x28;
            i++;
        } while (title_0c900(a) == a);
    }
    return i;
}

int title_06670(void)
{
    switch (title_06430(0)) {
    case 0:
        title_0c5f0();
        g_29fbc = title_065e0((int)card_dir);
        return 0;
    case 1:
        return -1;
    case 2:
        return -1;
    case 3:
        return -1;
    default:
        return -1;
    }
}

void title_066c0(void)
{
    g_29fbc = title_065e0((int)card_dir);
}

void title_066e0(void)
{
    title_0c730((int)"bu00:");
}

int title_06840(char a1, const void *a2, int a3, int a4, int a5)
{
    char path[0x100];
    char rec[0x200];
    int r;
    int fd;
    int i;

    title_0c610(0);
    r = title_06480();
    if (r == 1 || r == 2) {
        return 0;
    }
    if (r == 3) {
        title_065a0();
        title_0c600(0);
        return 2;
    }
    title_064f0();
    title_0c620(0);
    title_06480();

    strcpy(path, "bu00:B-sces-00891");
    path[6] = 'A';
    path[0x11] = ' ';
    path[0x12] = ' ';
    path[0x14] = ' ';
    path[0x16] = 'A';
    path[0x15] = 'S';
    for (i = 0; i < 0x100; i++) {
        save_image[i] = -1;
    }
    path[0x13] = a1;
    path[0x17] = 'V';
    path[0x18] = 'E';
    path[0x19] = 0;
    title_066f0(save_image, a5);
    memcpy((char *)save_image + 0x200, a2, a3);

    if (a4 == 1) {
        fd = _open(path, 0x10100);
        if (fd == -1) {
            return 1;
        }
        _close(fd);
    }

    fd = _open(path, 1);
    if (fd == -1) {
        return 4;
    }
    if (_write(fd, save_image, 0x400) != 0x400) {
        return 4;
    }
    _close(fd);

    fd = _open(path, 0);
    if (fd == -1) {
        return 5;
    }
    if (_read(fd, rec, 0x200) != 0x200) {
        return 5;
    }
    _close(fd);
    return 3;
}

int title_06a50(char a1, void *a2, int a3)
{
    char path[0x100];
    int r;
    int fd;

    title_0c610(0);
    r = title_06480();
    if (r == 1 || r == 2) {
        return 0;
    }
    if (r == 3) {
        title_065a0();
        title_0c600(0);
        return 2;
    }
    title_064f0();
    title_0c620(0);
    title_06480();

    strcpy(path, "bu00:B-sces-00891");
    path[6] = 'A';
    path[0x11] = ' ';
    path[0x12] = ' ';
    path[0x13] = a1;
    path[0x14] = ' ';
    path[0x15] = 'S';
    path[0x16] = 'A';
    path[0x17] = 'V';
    path[0x18] = 'E';
    path[0x19] = 0;

    fd = _open(path, 0);
    if (fd == -1) {
        return 6;
    }
    if (_read(fd, load_image, 0x400) != 0x400) {
        return 7;
    }
    _close(fd);
    memcpy(a2, (char *)load_image + 0x200, a3);
    return 8;
}

void title_066f0(CardHeader *header, int blank)
{
    unsigned short title[32] = {
        0xe181, 0x6782, 0x6482, 0x7182, 0x6282, 0x7482, 0x6b82, 0x6482,
        0x7282, 0xe281, 0x4081, 0x4081, 0x4081, 0x4081, 0x4081, 0x4081,
        0x4081, 0x4081, 0x4081, 0x4081, 0x4081, 0x4081, 0x4081, 0x4081,
        0x4081, 0x4081, 0x4081, 0x4081, 0x4081, 0x4081, 0x4081, 0x4081
    };
    int i, row, column;
    unsigned char *source, *pixel;

    header->magic[0] = 'S';
    header->magic[1] = 'C';
    header->icon_flag = 0x11;
    header->block_count = 1;
    for (i = 0; i < 0x1c; i++)
        header->reserved[i] = 0;
    for (i = 0; i < 16; i++)
        header->clut[i] = g_23368[i];
    memcpy(header->title, title, sizeof(title));
    source = g_23389;
    for (row = 0; row < 16; row++) {
        column = 0;
        pixel = source;
        source += 16;
        for (; column < 8; pixel += 2, column++) {
            if (blank != 0)
                header->icon[row][column] = 0;
            else
                header->icon[row][column] = (*pixel << 4) | pixel[-1];
        }
    }
}
