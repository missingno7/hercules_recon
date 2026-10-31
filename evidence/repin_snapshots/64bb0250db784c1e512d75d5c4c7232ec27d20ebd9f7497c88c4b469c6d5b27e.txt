/* ENG1 relocation-free handlers and geometry tests, 0x201b0..0x2c060.
 * Provisional shared Actor/Bounds3D layouts; not a recovered original translation unit. */
typedef struct Bounds3D {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds3D;

typedef struct Entry {
    unsigned char unknown_00[0x0e];
    unsigned short tag;
} Entry;

typedef struct Actor Actor;
struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x22 - 0x0c];
    unsigned char category, mode;
    unsigned char unknown_24[0x34 - 0x24];
    unsigned short sprite, unknown_36;
    unsigned char unknown_38[0x3a - 0x38];
    short scale;
    unsigned char unknown_3c[0x4c - 0x3c];
    long parameter;
    unsigned long flags, render_flags;
    unsigned char unknown_58[0x5c - 0x58];
    Entry *entries;
};

int overlap_wrapped_bounds(Bounds3D *a, Bounds3D *b)
{
    int d = a->min_x - b->min_x;
    int r;
    if (d > 0x800) { a->min_x -= 4096; a->max_x -= 4096; }
    if (d < -0x800) { b->min_x -= 4096; b->max_x -= 4096; }
    r = (b->max_y < a->min_y);
    r |= (a->max_y < b->min_y);
    r |= (b->max_z < a->min_z);
    r |= (a->max_z < b->min_z);
    r |= (b->max_x < a->min_x);
    r |= (a->max_x < b->min_x);
    return !r;
}

void set_wide_scale(Actor *p)
{ p->scale = 0x200; }

void reset_placement(Actor *p)
{
    p->flags = 2; p->category = 0; p->render_flags = 0xb0000000; p->mode = 0;
    p->sprite = 0; p->x = 0; p->y = 0; p->z = 0;
}

long signed_step(long a, long b, long step)
{
    long from = a >> 16, to = b >> 16;
    if (from < to) return step << 16;
    if (from > to) return -(step << 16);
    return 0;
}

void set_flags_two(Actor *p)
{ p->flags = 2; }

void show_sprite_82(Actor *p)
{ p->sprite = 0x82; p->flags |= 2; p->render_flags |= 0x80000005; }

void show_sprite_7a(Actor *p)
{ p->sprite = 0x7a; p->flags |= 2; p->render_flags |= 0x80000005; }

void reset_render_parameter(Actor *p)
{ p->render_flags = 0; p->parameter = 0x3c; }

void attach_entries(Actor *p, Entry *entries)
{
    Entry *e;
    unsigned short count = 0;
    p->entries = entries;
    for (e = entries; e->tag != 0xffff; e++)
        count++;
    p->unknown_36 = count;
}

int wrapped_ratio(int a, int b, int divisor)
{
    int delta = (a - (b & 0x3ff)) << 22;
    delta >>= 22;
    if (divisor != 0) return delta / divisor;
    return 0;
}
