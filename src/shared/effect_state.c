/* Matched effect initialization and wrapped-coordinate helpers.
 * Provisional shared offsets; not a recovered original translation unit. */
typedef struct Effect {
    long x,y,z;
    unsigned char unknown_c[0x22-0xc];
    unsigned char category, mode;
    unsigned char unknown_24[6];
    short state;
    unsigned char unknown_2c[4];
    short ticks, phase;
    unsigned short sprite;
    unsigned char unknown_36[4];
    short scale;
    unsigned char unknown_3c[0x50-0x3c];
    unsigned long flags, render_flags;
    unsigned char unknown_58[0x70-0x58];
    short speed, step;
} Effect;
void initialize_backdrop(Effect *p)
{
    p->render_flags=0x90000000; p->flags|=0x20000;
    p->step=0x200; p->scale=0x3ff; p->sprite=14;
    p->x=0x4000000; p->z+=0x100000; p->mode=4;
}
void initialize_particle(Effect *p)
{ p->flags=2; p->category=1; p->ticks=0; }
int wrapped_delta(long a,long b)
{
    int delta;
    a>>=16; b>>=16;
    if(b>=a) delta=b-a; else delta=b-a+4096;
    if(delta>=2048) delta-=4096;
    return delta;
}
void initialize_burst(Effect *p)
{ p->render_flags|=5; p->scale*=3; p->ticks=0; p->sprite=0; }
