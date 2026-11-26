/* ENG1 effect handlers reached through the data tables at 0x58108..0x58750.
 * Uses the provisional shared Effect layout of src/shared/effect_state.c,
 * extended by the countdown at 0x108. Not a recovered original translation unit. */
typedef struct Effect Effect;
struct Effect {
    long x, y, z;
    unsigned char unknown_c[0x22 - 0xc];
    unsigned char category, mode;
    unsigned char unknown_24[6];
    short state;
    unsigned char unknown_2c[4];
    short ticks, phase;
    unsigned short sprite;
    unsigned char unknown_36[4];
    short scale, unknown_3c;
    unsigned char unknown_3e[0x4a - 0x3e];
    short unknown_4a;
    long parameter;
    unsigned long flags, render_flags;
    unsigned char unknown_58[4];
    Effect *attachment;
    unsigned char unknown_60[0x70 - 0x60];
    short speed, step, unknown_74;
    unsigned char unknown_76[0x108 - 0x76];
    long countdown;
};

void initialize_wide_effect(Effect *p)
{ p->ticks=1; p->sprite=0x50; p->x=0x8600000; p->y=0x3ee0000; p->z=0x2800000; }

void initialize_wider_effect(Effect *p)
{ p->ticks=1; p->sprite=0x60; p->x=0x8e00000; p->y=0x3ee0000; p->z=0x2800000; }

void initialize_low_backdrop(Effect *p)
{
    p->render_flags=0x90000000; p->flags|=0x20000;
    p->step=0x200; p->scale=0x3ff; p->sprite=3;
    p->x=0x4000000; p->z+=0x100000; p->mode=4;
}

void select_side_sprite(Effect *p)
{
    if(p->x>0x8000000) { p->sprite=10; p->render_flags|=0x12000000; }
    else { p->sprite=9; p->render_flags|=0x12000000; }
}

void start_cycle_effect(Effect *p)
{ p->mode=5; p->render_flags|=0x80001000; p->sprite=0x15; p->countdown=3; }

void advance_cycle_effect(Effect *p)
{
    if(--p->countdown>0) return;
    if(++p->sprite<0x1d) { p->countdown=3; return; }
    p->countdown=3; p->sprite=0x15;
}

void start_sequence_effect(Effect *p)
{
    p->sprite=0x1d; p->render_flags|=0x80000000; p->countdown=3; p->mode=5;
    p->speed=0x300; p->x=0xf000000; p->y=0x3d80000; p->z=0x26f0000; p->scale=0x3ff;
}

void advance_sequence_effect(Effect *p)
{
    if(--p->countdown>0) return;
    p->countdown=3;
    if(p->sprite<0x2d) p->sprite++;
}

void initialize_flagged_effect(Effect *p)
{ p->unknown_4a=0; p->flags|=0x10000; p->sprite=0x3e; p->render_flags|=0x684; }

void follow_attachment_scale(Effect *p)
{
    long scale;
    p->x=p->attachment->x;
    p->y=0x3e00000;
    p->z=p->attachment->z;
    scale=((0x3e00000-p->attachment->y)>>19)+((p->parameter<<8)/20);
    if(scale>1023) scale=1023;
    p->scale=(short)scale;
}

void clear_effect_animation(Effect *p)
{ p->render_flags|=5; p->ticks=0; p->sprite=0; }

void initialize_high_backdrop(Effect *p)
{
    p->render_flags=0x90000000; p->flags|=0x20000;
    p->step=0x3c0; p->speed=0; p->unknown_74=0; p->scale=0x3ff; p->sprite=0x48;
    p->x=0xf000000; p->y=0x500000; p->z=0x26f0000;
    p->ticks=0x3ae; p->mode=7; p->state=0; p->unknown_3c=0;
}
