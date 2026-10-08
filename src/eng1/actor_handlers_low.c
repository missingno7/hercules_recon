/* ENG1.DLL lane a: EXACT functions in ascending RVA order.
 * One shared Actor layout; names follow src/shared/effect_state.c where the offset coincides. */
typedef struct Actor Actor;
struct Actor {
    long x;
    long y;
    unsigned char unknown_08[0x23 - 0x08];
    unsigned char mode;
    unsigned char unknown_24[0x2a - 0x24];
    short state;
    unsigned char unknown_2c[0x30 - 0x2c];
    short ticks;
    unsigned char unknown_32[0x34 - 0x32];
    unsigned short sprite;
    unsigned short unknown_36;
    unsigned char unknown_38[0x44 - 0x38];
    unsigned short unknown_44;
    unsigned char unknown_46[0x4a - 0x46];
    unsigned short unknown_4a;
    unsigned char unknown_4c[0x50 - 0x4c];
    unsigned long flags;
    unsigned long render_flags;
    unsigned char unknown_58[0x60 - 0x58];
    unsigned int unknown_60;
    void *unknown_64;
    unsigned char unknown_68[0x88 - 0x68];
    unsigned short unknown_88;
    unsigned short unknown_8a;
    unsigned char unknown_8c[0x8e - 0x8c];
    unsigned short unknown_8e;
    unsigned short unknown_90;
    unsigned short unknown_92;
    unsigned char unknown_94[0x9a - 0x94];
    unsigned short unknown_9a;
    unsigned char unknown_9c[0xa8 - 0x9c];
    int unknown_a8;
    int unknown_ac;
    int unknown_b0;
    int unknown_b4;
    unsigned char unknown_b8[0xd8 - 0xb8];
    int unknown_d8;
    int unknown_dc;
    unsigned char unknown_e0[0x108 - 0xe0];
    unsigned int unknown_108;
    unsigned int unknown_10c;
};

void init_fields_1d50(Actor *p)
{
    p->sprite = 0xa3;
    p->unknown_ac = 0xb0000;
    p->unknown_a8 = 0xb0000;
    p->unknown_b0 = 0x4000;
    p->unknown_b4 = 0;
}

void set_scale_2c20(Actor *p, int n)
{
    int v;

    if (n < 2) {
        n = 2;
    } else if (n > 0x20) {
        n = 0x20;
    }
    v = 0x100 / n;
    if (v != p->unknown_36) {
        p->unknown_36 = (unsigned short)v;
        p->ticks = p->unknown_44;
    }
}

void set_limit_e610(Actor *p)
{
    p->unknown_d8 = 0x1f;
}

void init_flags_f070(Actor *p)
{
    p->flags = 0x10000;
    p->render_flags |= 0x684;
    p->mode = 5;
    p->unknown_4a = 0x10;
}

void load_coords_10380(Actor *p)
{
    unsigned short *q;

    p->unknown_60 = 0;
    p->flags |= 0x40000000;
    q = (unsigned short *)p->unknown_64;
    p->unknown_108 = q[0];
    p->unknown_10c = q[1];
}

void init_defaults_117d0(Actor *p)
{
    p->sprite = 0;
    p->flags = 2;
    p->unknown_88 = 0x320;
    p->unknown_8a = 0x400;
    p->unknown_8e = 0x100;
    p->unknown_90 = 0;
    p->unknown_92 = 0xfda8;
}

void set_mode_13c90(Actor *p)
{
    p->flags = 0x41;
}

void copy_pair_14de0(Actor *p)
{
    p->unknown_d8 = p->x;
    p->unknown_dc = p->y;
    p->state = 0;
    p->unknown_9a = 0;
}
