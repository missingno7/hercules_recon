/* First-pass blind region: ENG1 216b0..219df. Provisional offset semantics. */
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
extern Effect *g_focus;
extern unsigned long g_clock;
extern long g_delay,g_busy,g_blocked,g_cooldown;
extern long *g_script_cursor,*g_script_start;
extern void release_effect(Effect *p);
extern void finish_effect(Effect *p);
extern unsigned int random_limit(int n);
extern void select_effect_frame(Effect *p,int frame);
extern void play_effect_sound(int sound,int channel);

void initialize_backdrop(Effect *p)
{
    p->render_flags=0x90000000; p->flags|=0x20000;
    p->step=0x200; p->scale=0x3ff; p->sprite=14;
    p->x=0x4000000; p->z+=0x100000; p->mode=4;
}
void initialize_particle(Effect *p)
{ p->flags=2; p->category=1; p->ticks=0; }
void tick_particle(Effect *p)
{ p->ticks^=1; p->y-=0x10000; if(p->ticks) release_effect(p); }
void initialize_flicker(Effect *p)
{ p->flags|=0x20000; p->render_flags|=0x80000005; p->sprite=93; p->phase=random_limit(2); }
int wrapped_delta(long a,long b);
void tick_flicker(Effect *p)
{
    int delta=wrapped_delta(p->x,g_focus->x);
    if(delta<1024 && delta>=-1024) p->render_flags&=0x7fffffff;
    else p->render_flags|=0x80000000;
    if((unsigned short)(g_clock&1)==p->phase) {
        ++p->sprite;
        if(p->sprite>=101) p->sprite=93;
    }
}
int wrapped_delta(long a,long b)
{
    int delta;
    a>>=16; b>>=16;
    if(b>=a) delta=b-a; else delta=b-a+4096;
    if(delta>=2048) delta-=4096;
    return delta;
}
void maybe_start_effect(Effect *p)
{
    if(g_delay) --g_delay;
    else if(!g_busy && random_limit(4)<3 && !g_blocked) {
        g_busy=1; select_effect_frame(p,3);
        if(!random_limit(4)) play_effect_sound(450,0);
    }
}
void next_script_frame(Effect *p)
{
    long opcode=*g_script_cursor++;
    while(opcode<0) { g_script_cursor=g_script_start; opcode=*g_script_cursor++; }
    switch(opcode) {
    case 0: select_effect_frame(p,2); break;
    case 1: select_effect_frame(p,6); break;
    case 2: select_effect_frame(p,5); break;
    }
}
void reset_effect(Effect *p)
{ g_busy=0; p->state=0; g_cooldown=30; }
void initialize_burst(Effect *p)
{ p->render_flags|=5; p->scale*=3; p->ticks=p->sprite=0; }
void tick_burst(Effect *p)
{
    if(--p->ticks<=0) {
        p->ticks=2;
        if(!p->sprite) p->sprite=134; else ++p->sprite;
        if(p->sprite>=148) finish_effect(p);
    }
}
void initialize_sound_effect(Effect *p)
{
    p->mode=5; p->render_flags|=5; p->scale*=3;
    p->phase=p->ticks=0;
    p->y=0x2e00000; p->x=0x4000000; p->z=0x2900000; p->speed=128;
    play_effect_sound(467,0);
}
