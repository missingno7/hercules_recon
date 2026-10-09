#include <string.h>
#include <io.h>
#include "title_engine.h"
#include "title_files.h"


static int g_29fc0;
int g_2c144;
int g_2c148;
int g_2c14c;
int g_2c150;
int g_2cbc0;
int g_2cbc4;
int g_2cbc8;
int g_2cbcc;
extern char g_2c160[];
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
extern void title_066f0(void *buf, int n);


extern void title_0c620(int x);
extern void title_01dd0(int a);
extern void title_02090(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
extern void title_0c2a0(char *s, char *name, int b, int c, int d);
extern void title_0c8b0(int a1, int a2, int a3, int a4, int a5, int a6, int a7);
extern int g_2c7c0[];
extern char g_2c9c0[];
extern int g_2a044;
extern int g_2a03c;
extern int *g_2a048;
extern int *g_2a050;
extern int *g_2a058;
extern int *g_2a054;
extern int *g_2a04c;
extern int *g_2a060;
extern int *g_2a064;
extern int *g_2a070;
extern int *g_2a05c;
extern int *g_2a06c;
extern int *g_2a000;
extern int *g_2a004;
extern int *g_29ffc;
extern int *g_29ff4;
extern int g_2a018;
extern int g_2a080;
extern int g_2a088;
extern int g_2a078;
extern char g_29128[];

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
        if (title_0cc60(g_2c144) == 1) return 0;
        if (title_0cc60(g_2c148) == 1) return 1;
        if (title_0cc60(g_2c14c) == 1) return 2;
        if (title_0cc60(g_2c150) == 1) return 3;
    }
}

void title_064f0(void)
{
    title_0cc60(g_2c144);
    title_0cc60(g_2c148);
    title_0cc60(g_2c14c);
    title_0cc60(g_2c150);
}

int title_06530(void)
{
    while (1) {
        if (title_0cc60(g_2cbc0) == 1) return 0;
        if (title_0cc60(g_2cbc4) == 1) return 1;
        if (title_0cc60(g_2cbc8) == 1) return 2;
        if (title_0cc60(g_2cbcc) == 1) return 3;
    }
}

void title_065a0(void)
{
    title_0cc60(g_2cbc0);
    title_0cc60(g_2cbc4);
    title_0cc60(g_2cbc8);
    title_0cc60(g_2cbcc);
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
        g_29fbc = title_065e0((int)g_2c160);
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
    g_29fbc = title_065e0((int)g_2c160);
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
        g_2c7c0[i] = -1;
    }
    path[0x13] = a1;
    path[0x17] = 'V';
    path[0x18] = 'E';
    path[0x19] = 0;
    title_066f0(g_2c7c0, a5);
    memcpy(g_2c9c0, a2, a3);

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
    if (_write(fd, g_2c7c0, 0x400) != 0x400) {
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
