/* Synthetic callbacks are test fixtures only; this never loads the PC oracle. */
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "../calibration/actor_continuation.c"
ScriptWord g_frame_script[8];
unsigned char g_spawn_gate;
long g_countdown,g_effect_speed,g_effect_phase;
Actor *g_focus;
unsigned char *g_collision_context;
Callback8 *g_alt_callbacks;
Callback12 *g_callbacks;
static Callback8 alt[60];
static Callback12 normal[60];
static Actor subject,owner,focus,children[8],observed;
static unsigned char collision_context[256];
static unsigned checks,failures;
static int released,removed,finished,sounds,spawn_count,callback_count,collision,resolve_count;
static int last_sound,last_channel,actor_sound,spawn_fail_mask;
static unsigned rv[20],limits[20],rng_count,rng_size;
static struct Spawn { long x,y,z; int kind,normal; } spawns[8];
#define CHECK(c) do {++checks;if(!(c)){++failures;if(failures<20)printf("FAIL %d\n",__LINE__);}}while(0)
typedef char layout[(offsetof(Actor,ticks)==0x30 && offsetof(Actor,frame)==0x34 && offsetof(Actor,field_4a)==0x4a && offsetof(Actor,owner)==0x5c && offsetof(Actor,angle_z)==0x74 && offsetof(Actor,velocity_x)==0xa8 && offsetof(Actor,velocity_y)==0xb8 && offsetof(Actor,velocity_z)==0xc8 && offsetof(Actor,spin_z)==0xe0)?1:-1];
static void reset(void)
{
    memset(&subject,0x55,sizeof(subject));memset(&owner,0,sizeof(owner));
    memset(&focus,0,sizeof(focus));subject.owner=&owner;g_focus=&focus;
    subject.x=12345;subject.y=20000000;subject.z=99999;
    released=removed=finished=sounds=spawn_count=callback_count=resolve_count=0;
    rng_count=rng_size=0;actor_sound=0;spawn_fail_mask=0;collision=0;
    g_collision_context=collision_context;
}
static void rng(unsigned bound,unsigned value){limits[rng_size]=bound;rv[rng_size++]=value;}
unsigned int random_limit(int n)
{
    CHECK(rng_count<rng_size);CHECK((unsigned)n==limits[rng_count]);
    CHECK(rv[rng_count]<(unsigned)n);return rv[rng_count++];
}
void release_actor(Actor *p){CHECK(p==&subject);observed=*p;++released;}
void remove_actor(Actor *p){CHECK(p==&subject);observed=*p;++removed;}
void finish_actor(Actor *p){CHECK(p==&subject);CHECK(g_effect_speed==0);++finished;}
void play_sound(int s,int channel){last_sound=s;last_channel=channel;observed=subject;++sounds;}
void attach_actor(Actor *p,int kind){CHECK(p==&subject && kind==0x202f);CHECK(p->render_flags==0x90000000);}
int query_collision(void *context,Actor *p){CHECK(context==collision_context+0x88 && p==&subject);observed=*p;return collision;}
void resolve_collision(Actor *p,int a,int b){CHECK(p==&subject && !a && !b);++resolve_count;}
void play_actor_sound(int s,Actor *child,Actor *p){CHECK(p==&subject);CHECK(child==((spawn_fail_mask&(1<<(spawn_count-1)))?0:&children[spawn_count-1]));actor_sound=s;}
static Actor *spawn(long x,long y,long z,int kind,int is_normal)
{
    int index=spawn_count++;CHECK(index<8);spawns[index].x=x;spawns[index].y=y;
    spawns[index].z=z;spawns[index].kind=kind;spawns[index].normal=is_normal;
    return (spawn_fail_mask&(1<<index))?0:&children[index];
}
Actor *spawn_kind(long x,long y,long z,int kind){return spawn(x,y,z,kind,0);}
Actor *spawn_normal(long x,long y,long z,int kind){return spawn(x,y,z,kind,1);}
static void initialize_child(Actor *p){CHECK(p>=children && p<children+8);++callback_count;}
int main(void)
{
    Actor expected;int i,j,k,gate,path,speed,direction,negative,mask,count,hit,selection,sound;
    long s,values[7]={-1000,-21,-1,0,1,21,1000};
    for(i=0;i<60;++i){alt[i].initialize=initialize_child;normal[i].initialize=initialize_child;}
    g_alt_callbacks=alt;g_callbacks=normal;
    for(i=-32768;i<=32767;++i)for(j=-1;j<2;++j){
        reset();subject.ticks=(short)i;subject.phase=2;
        g_frame_script[2].value=(short)j;g_frame_script[3].value=-32768;
        expected=subject;expected.ticks=(short)(i-1);
        if(expected.ticks<=0){expected.ticks=(short)j;expected.phase=3;
            if(j>=0){expected.frame=(unsigned short)(-32768+118);expected.phase=4;}}
        tick_scripted_actor(&subject);
        CHECK(memcmp(&subject,&expected,sizeof(subject))==0);
        CHECK(released==(expected.ticks<=0 && j<0 && (short)(i-1)<=0));
        if(released)CHECK(memcmp(&observed,&expected,sizeof(subject))==0);
    }
    for(mask=0;mask<2;++mask){reset();spawn_fail_mask=mask;expected=subject;expected.state=2;
        start_spawn_phase(&subject);CHECK(memcmp(&subject,&expected,sizeof(subject))==0);
        CHECK(spawn_count==1 && callback_count==!mask);CHECK(spawns[0].kind==0x2033);
        CHECK(spawns[0].x==expected.x && spawns[0].y==expected.y && spawns[0].z==expected.z);
        CHECK(g_countdown==180 && sounds==1 && last_sound==461 && !last_channel);
        CHECK(memcmp(&observed,&expected,sizeof(subject))==0);
    }
    for(gate=0;gate<4;++gate)for(path=0;path<2;++path)for(speed=0;speed<2;++speed)
    for(direction=0;direction<3;++direction)for(negative=0;negative<2;++negative)for(mask=0;mask<4;++mask){
        reset();g_spawn_gate=(unsigned char)gate;g_countdown=negative?-1:0;spawn_fail_mask=mask;
        focus.x=100000;focus.y=30000000;focus.z=44444;
        focus.velocity_x=speed?0x10000:65535;focus.direction_x=(unsigned char)direction;
        expected=subject;
        if(!gate){rng(8,path);if(!path){rng(348,17);rng(4096,31);}else rng(speed?32:64,7);}
        rng(348,11);rng(4096,19);tick_spawn_phase(&subject);
        CHECK(rng_count==rng_size);CHECK(memcmp(&subject,&expected,sizeof(subject))==0);
        CHECK(spawn_count==(gate?1:2));CHECK(finished==negative);CHECK(g_effect_speed==(negative?0:0x40000));
        k=0;
        if(!gate){CHECK(spawns[0].normal && spawns[0].kind==0x34);
            if(!path){CHECK(spawns[0].x==(31L<<16));CHECK(spawns[0].y==expected.y-0x1680000);CHECK(spawns[0].z==expected.z-(17L<<16)-0xd80000);}
            else{ s=speed?127:-25;if(speed && direction)s=-s;
                CHECK(spawns[0].x==focus.x+(s<<16));CHECK(spawns[0].y==focus.y-0x1180000);CHECK(spawns[0].z==focus.z);}
            k=1;}
        CHECK(!spawns[k].normal && spawns[k].kind==0x202a && spawns[k].x==(19L<<16));
        CHECK(spawns[k].y==expected.y-0x1680000 && spawns[k].z==expected.z-(11L<<16)-0xd80000);
        CHECK(callback_count==((gate?0:!(mask&1))+!(mask&(1<<k))));
    }
    for(i=0;i<4;++i){reset();expected=subject;
        rng(4,i);rng(10,2);rng(10,9);rng(4096,0);rng(4096,4095);rng(4096,1);
        rng(64,0);rng(64,32);rng(64,63);
        expected.render_flags=0x90000000;expected.frame=(unsigned short)(i==0?2:i==1?3:i==2?1:12);
        expected.velocity_x=-3;expected.velocity_y=0;expected.velocity_z=4;
        expected.angle_x=0;expected.angle_y=4095;expected.angle_z=1;
        expected.spin_x.value=-32;expected.spin_y.value=0;expected.spin_z.value=31;
        initialize_spinning_particle(&subject);CHECK(memcmp(&subject,&expected,sizeof(subject))==0);
        CHECK(owner.parameter==(i==3?12:28));CHECK(rng_count==rng_size);
    }
    for(count=1;count<=4;++count)for(hit=0;hit<3;++hit)for(selection=0;selection<10;++selection)
    for(sound=0;sound<2;++sound)for(mask=0;mask<2;++mask){
        reset();collision=hit==2;subject.y=hit==1?0x2d80000:1000000;
        subject.velocity_x=-3;subject.velocity_y=2;subject.velocity_z=5;
        subject.angle_x=-32768;subject.angle_y=4095;subject.angle_z=1;
        subject.spin_x.value=0x1234ffff;subject.spin_y.value=2;subject.spin_z.value=-3;
        expected=subject;expected.x-=3L<<16;expected.y+=2L<<16;expected.z+=5L<<16;
        expected.velocity_y=3;expected.angle_x=4095;expected.angle_y=1;expected.angle_z=4094;
        if(hit){rng(4,2);for(i=0;i<count;++i)rng(4,count-1);rng(2,sound);if(!sound)rng(10,selection);
            if(mask)spawn_fail_mask=(1<<(count+1))-1;}
        tick_spinning_particle(&subject);CHECK(memcmp(&subject,&expected,sizeof(subject))==0);
        CHECK(memcmp(&observed,&expected,sizeof(subject))==0);CHECK(rng_count==rng_size);
        CHECK(removed==!!hit && resolve_count==(hit==2));CHECK(spawn_count==(hit?count+1:0));
        if(hit){CHECK(g_effect_phase==3);CHECK(callback_count==(mask?0:count+1));
            CHECK(spawns[count].y==expected.y+0x100000 && spawns[count].kind==0x202b);
            for(i=0;i<count;++i)CHECK(spawns[i].kind==0x202a && spawns[i].x==expected.x && spawns[i].y==expected.y && spawns[i].z==expected.z);
            CHECK(actor_sound==(sound?0:selection<3?0xd1cf:selection<6?0xd1d0:selection<9?0xd1d2:0xd1d1));}
    }
    for(i=0;i<256;++i){reset();memset(&subject,i,sizeof(subject));expected=subject;
        expected.field_4a=0;expected.flags|=0x10000;expected.frame=25;expected.render_flags|=0x684;
        initialize_linked_particle(&subject);CHECK(memcmp(&subject,&expected,sizeof(subject))==0);
    }
    for(i=0;i<7;++i)for(j=-3;j<=3;++j){reset();owner.x=-1234567;owner.y=0x2e00000+j*524288L+123;owner.z=7654321;
        subject.parameter=values[i];expected=subject;expected.x=owner.x;expected.y=0x2e00000;expected.z=owner.z;
        s=((0x2e00000-owner.y)>>19)+(values[i]*256/20);if(s>1023)s=1023;expected.scale=(short)s;
        follow_owner_scale(&subject);CHECK(memcmp(&subject,&expected,sizeof(subject))==0);
    }
    printf("%u semantic checks; %u failures\n",checks,failures);return failures?1:0;
}
