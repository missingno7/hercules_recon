/* TITLE unit from 0x6350 (hypothesis: its ordinary data 0x23368.. follows the scheduler unit's literals;
   0x65e0/0x66e0 use the "bu00:" memory-card device string). Functions in RVA order. */
#include <string.h>

static int g_29fc0;
extern int g_2c144;
extern int g_2c148;
extern int g_2c14c;
extern int g_2c150;
extern int g_2cbc0;
extern int g_2cbc4;
extern int g_2cbc8;
extern int g_2cbcc;
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
