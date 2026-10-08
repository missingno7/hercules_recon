/* TITLE.DLL EXACT functions merged from candidates/leaf-f.
 * Actor offsets are provisional shared names, not recovered original types. */
typedef struct Actor Actor;
struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x22 - 0x0c];
    unsigned char category;
    unsigned char mode;
    unsigned char unknown_24[0x2a - 0x24];
    short state;
    unsigned char unknown_2c[0x30 - 0x2c];
    short ticks, phase;
    unsigned short sprite;
    unsigned char unknown_36[0x3a - 0x36];
    short scale;
    unsigned char unknown_3c[0x3e - 0x3c];
    unsigned short unknown_3e;
    unsigned char unknown_40[0x4a - 0x40];
    unsigned short unknown_4a;
    long parameter;
    unsigned long flags, render_flags;
    unsigned char unknown_58[0x5c - 0x58];
    Actor *attachment;
    unsigned char unknown_60[0x70 - 0x60];
    short speed, step;
};

void init_title_fields_d410(Actor *p)
{
    p->unknown_3e = 0x14;
    p->render_flags |= 0x2005;
    p->mode = 6;
    p->unknown_4a = 0xff;
    p->sprite = 0xb6;
}
