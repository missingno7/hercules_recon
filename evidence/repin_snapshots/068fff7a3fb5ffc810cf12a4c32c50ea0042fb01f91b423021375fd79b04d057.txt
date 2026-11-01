/* ENG1.DLL lane d: EXACT functions in ascending RVA order.
 * One shared Actor layout; names follow src/shared/effect_state.c where the offset coincides. */
typedef struct Actor Actor;
struct Actor {
    long x;
    long y;
    long z;
    unsigned char unknown_0c[0x23 - 0x0c];
    unsigned char mode;
    unsigned char unknown_24[0x2a - 0x24];
    short state;
    unsigned char unknown_2c[0x34 - 0x2c];
    unsigned short sprite;
    unsigned char unknown_36[0x3a - 0x36];
    short scale;
    short unknown_3c;
    unsigned char unknown_3e[0x4a - 0x3e];
    short unknown_4a;
    unsigned char unknown_4c[0x50 - 0x4c];
    unsigned long flags;
    unsigned long render_flags;
    unsigned char unknown_58[0x5c - 0x58];
    Actor *attachment;
    unsigned char unknown_60[0x70 - 0x60];
    short speed;
    short step;
    short unknown_74;
    unsigned char unknown_76[0x88 - 0x76];
    short unknown_88;
    short unknown_8a;
    unsigned char unknown_8c[0x98 - 0x8c];
    short unknown_98;
    unsigned char unknown_9a[0xa8 - 0x9a];
    int unknown_a8;
    unsigned char unknown_ac[0xb8 - 0xac];
    int unknown_b8;
    unsigned char unknown_bc[0xd8 - 0xbc];
    int unknown_d8;
    int unknown_dc;
    int unknown_e0;
};

void set_mode_flags_36120(Actor *p)
{
    p->flags = 0x10000;
    p->render_flags |= 0x604;
    p->unknown_4a = 0x10;
    p->sprite = 0x18;
}

void reset_extent_36b60(Actor *p)
{
    p->sprite = 0;
    p->flags = 2;
    p->unknown_88 = 0xa0;
    p->unknown_8a = 0xc8;
}

void set_render_flags_3e9b0(Actor *p)
{
    p->sprite = 0x7f;
    p->render_flags |= 0x10080;
    p->unknown_3c = 0x200;
}

void set_collide_flags_3eaa0(Actor *p)
{
    p->sprite = 0x7f;
    p->render_flags |= 0x80080;
    p->flags |= 0x800;
}

void set_collide_flags_3eac0(Actor *p)
{
    p->sprite = 0x7f;
    p->render_flags |= 0x90080;
    p->unknown_3c = 0x200;
    p->flags |= 0x800;
}

void set_flags_3f040(Actor *p)
{
    p->unknown_d8 = 1;
    p->render_flags |= 0x10000000;
    p->scale = 0x300;
    p->flags |= 0x100000;
}

void adjust_bounds_3f070(Actor *p)
{
    p->flags = 0x10000;
    p->render_flags |= 0x10000000;
    p->unknown_4a = 0x10;
    p->z += 0x300000;
    p->sprite = 0x40;
    p->x -= 0x200000;
}

void apply_scaled_offsets_40ab0(Actor *p)
{
    int scale;

    if (p->sprite == 0x36)
        p->scale = 0x50;
    scale = p->unknown_98;
    p->speed += (p->unknown_d8 * scale) >> 2;
    p->step += (p->unknown_dc * scale) >> 2;
    p->unknown_74 += (p->unknown_e0 * scale) >> 2;
}

void set_state_flags_41300(Actor *p)
{
    p->flags = 0x10000;
    p->render_flags |= 0x684;
    p->mode = 5;
}

void copy_link_state_41fa0(Actor *p)
{
    Actor *q = p->attachment;

    if (q) {
        p->x = q->x;
        p->y = q->y;
        p->z = q->z;
        p->sprite = q->sprite - 0x13;
    }
}

void set_kind_422d0(Actor *p)
{
    p->flags = 0x41;
    p->render_flags = 0;
}

void set_flags_459e0(Actor *p)
{
    p->sprite = 0x2ad;
    p->render_flags |= 0x4000605;
    p->unknown_4a = 0x80;
}

void reset_links_45a50(Actor *p)
{
    p->unknown_4a = 0x80;
    p->render_flags |= 0x4000605;
    p->sprite = 0x188;
    p->unknown_a8 = 0;
    p->unknown_b8 = 0;
    p->state = 0;
}

int wrap_delta_461a0(int a, int b)
{
    int delta;
    a &= 0x3ff; b &= 0x3ff;
    if (b >= a) delta = b - a; else delta = b - a + 1024;
    if (delta >= 512) delta -= 1024;
    return delta;
}

void set_flags_461d0(Actor *p)
{
    p->unknown_4a = 0x80;
    p->render_flags |= 0x4000605;
    p->sprite = 0x1c8;
}
