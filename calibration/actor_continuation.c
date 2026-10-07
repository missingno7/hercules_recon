/* ENG1 219e0..21f5f: provisional semantic region, not historical TU identity.
 * Source hypotheses follow PC field accesses, including low-word spin updates. */
typedef struct Actor Actor;
typedef union Spin { long value; short step; } Spin;
struct Actor {
    long x,y,z;
    unsigned char unknown_c[0x22-0xc];
    unsigned char category,mode;
    unsigned char unknown_24[6];
    short state;
    unsigned char unknown_2c[4];
    short ticks,phase;
    unsigned short frame;
    unsigned char unknown_36[4];
    short scale;
    unsigned char unknown_3c[0x4a-0x3c];
    short field_4a;
    long parameter;
    unsigned long flags,render_flags;
    unsigned char unknown_58[4];
    Actor *owner;
    unsigned char unknown_60[0x70-0x60];
    short angle_x,angle_y,angle_z;
    unsigned char unknown_76[0x94-0x76];
    unsigned char direction_x;
    unsigned char unknown_95[0xa8-0x95];
    long velocity_x;
    unsigned char unknown_ac[12];
    long velocity_y;
    unsigned char unknown_bc[12];
    long velocity_z;
    unsigned char unknown_cc[12];
    Spin spin_x,spin_y,spin_z;
};
typedef struct ScriptWord { short value,unknown; } ScriptWord;
typedef struct Callback8 { void (*initialize)(Actor *); long unknown; } Callback8;
typedef struct Callback12 { void (*initialize)(Actor *); long unknown[2]; } Callback12;
extern ScriptWord g_frame_script[];
extern unsigned char g_spawn_gate;
extern long g_countdown,g_effect_speed,g_effect_phase;
extern Actor *g_focus;
extern unsigned char *g_collision_context;
extern Callback8 *g_alt_callbacks;
extern Callback12 *g_callbacks;
extern unsigned int random_limit(int);
extern Actor *spawn_normal(long,long,long,int);
extern Actor *spawn_kind(long,long,long,int);
extern void release_actor(Actor *);
extern void remove_actor(Actor *);
extern void play_sound(int,int);
extern void finish_actor(Actor *);
extern void attach_actor(Actor *,int);
extern int query_collision(void *,Actor *);
extern void resolve_collision(Actor *,int,int);
extern void play_actor_sound(int,Actor *,Actor *);

void tick_scripted_actor(Actor *p)
{
    --p->ticks;
    if(p->ticks<=0) {
        p->ticks=g_frame_script[p->phase++].value;
        if(p->ticks>=0) p->frame=g_frame_script[p->phase++].value+0x76;
        else release_actor(p);
    }
}
void start_spawn_phase(Actor *p)
{
    Actor *child=spawn_kind(p->x,p->y,p->z,0x2033);
    if(child) g_alt_callbacks[51].initialize(child);
    p->state=2;
    g_countdown=180;
    play_sound(461,0);
}
void tick_spawn_phase(Actor *p)
{
    Actor *child;
    long offset;
    if(!(g_spawn_gate&3)) {
        if(!random_limit(8)) {
            child=spawn_normal((long)random_limit(0x1000)<<16,
                p->y-0x1680000,p->z-((long)random_limit(0x15c)<<16)-0xd80000,0x34);
        } else {
            if(!(g_focus->velocity_x&0xffff0000)) offset=(long)random_limit(0x40)-0x20;
            else {
                offset=random_limit(0x20)+0x78;
                if(g_focus->direction_x) offset=-offset;
            }
            child=spawn_normal(g_focus->x+(offset<<16),g_focus->y-0x1180000,g_focus->z,0x34);
        }
        if(child) g_callbacks[52].initialize(child);
    }
    child=spawn_kind((long)random_limit(0x1000)<<16,p->y-0x1680000,
        p->z-((long)random_limit(0x15c)<<16)-0xd80000,0x202a);
    if(child) g_alt_callbacks[42].initialize(child);
    g_effect_speed=0x40000;
    if(g_countdown<0) {
        g_effect_speed=0;
        finish_actor(p);
    }
}
void initialize_spinning_particle(Actor *p)
{
    p->render_flags=0x90000000;
    attach_actor(p,0x202f);
    p->owner->parameter=28;
    switch(random_limit(4)) {
    case 0: p->frame=2;break;
    case 1: p->frame=3;break;
    case 2: p->frame=1;break;
    case 3: p->frame=12;p->owner->parameter=12;break;
    }
    p->velocity_x=(long)random_limit(10)-5;
    p->velocity_y=0;
    p->velocity_z=(long)random_limit(10)-5;
    p->angle_x=(short)random_limit(4096);
    p->angle_y=(short)random_limit(4096);
    p->angle_z=(short)random_limit(4096);
    p->spin_x.value=(long)random_limit(64)-32;
    p->spin_y.value=(long)random_limit(64)-32;
    p->spin_z.value=(long)random_limit(64)-32;
}
void tick_spinning_particle(Actor *p)
{
    Actor *child;
    unsigned int n=0;
    int collision;
    p->x+=p->velocity_x<<16;
    p->y+=p->velocity_y<<16;
    p->z+=p->velocity_z<<16;
    ++p->velocity_y;
    p->angle_x=(p->angle_x+p->spin_x.step)&4095;
    p->angle_y=(p->angle_y+p->spin_y.step)&4095;
    p->angle_z=(p->angle_z+p->spin_z.step)&4095;
    collision=query_collision(g_collision_context+0x88,p);
    if(p->y>=0x2d80000 || collision) {
        if(collision) resolve_collision(p,0,0);
        random_limit(4);
        do {
            child=spawn_kind(p->x,p->y,p->z,0x202a);
            if(child) g_alt_callbacks[42].initialize(child);
            ++n;
        } while(n<=random_limit(4));
        child=spawn_kind(p->x,p->y+0x100000,p->z,0x202b);
        if(child) g_alt_callbacks[43].initialize(child);
        if(!random_limit(2)) switch(random_limit(10)) {
        case 0:case 1:case 2:play_actor_sound(0xd1cf,child,p);break;
        case 3:case 4:case 5:play_actor_sound(0xd1d0,child,p);break;
        case 6:case 7:case 8:play_actor_sound(0xd1d2,child,p);break;
        case 9:play_actor_sound(0xd1d1,child,p);break;
        }
        remove_actor(p);
        g_effect_phase=3;
    }
}
void initialize_linked_particle(Actor *p)
{
    p->field_4a=0;
    p->flags|=0x10000;
    p->frame=25;
    p->render_flags|=0x684;
}
void follow_owner_scale(Actor *p)
{
    long scale;
    p->y=0x2e00000;
    p->x=p->owner->x;
    p->z=p->owner->z;
    scale=((0x2e00000-p->owner->y)>>19)+((p->parameter<<8)/20);
    if(scale>1023) scale=1023;
    p->scale=(short)scale;
}
