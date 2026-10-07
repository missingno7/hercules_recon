/* Reconstructed linked-particle state: ENG1 21ed0 and 21f00.
 * Names/layout are provisional; full contribution equality is recorded in recovery.json. */
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
    p->x=p->owner->x;
    p->y=0x2e00000;
    p->z=p->owner->z;
    scale=((0x2e00000-p->owner->y)>>19)+((p->parameter<<8)/20);
    if(scale>1023) scale=1023;
    p->scale=(short)scale;
}
