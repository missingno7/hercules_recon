/* Execute reconstructed callbacks against synthetic state and bounded references. */
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "../calibration/refinery_blind.c"
typedef char layout[(offsetof(Effect,flags)==0x50 && offsetof(Effect,step)==0x72 && offsetof(Effect,sprite)==0x34)?1:-1];
Effect *g_focus;
unsigned long g_clock;
long g_delay,g_busy,g_blocked,g_cooldown;
long *g_script_cursor,*g_script_start;
static unsigned int random_values[2],random_calls;
static int selected,sound,channel,freed,finished;
static Effect *subject;
static unsigned checks,failures;
#define CHECK(c) do { ++checks; if(!(c)) {++failures; printf("FAIL %d\n",__LINE__);} }while(0)
void release_effect(Effect *p) {CHECK(p==subject); ++freed;}
void finish_effect(Effect *p) {CHECK(p==subject); ++finished;}
unsigned int random_limit(int n) { CHECK(n==2 || n==4); return random_values[random_calls++ % 2]; }
void select_effect_frame(Effect *p,int frame) { CHECK(p==subject);selected=frame; }
void play_effect_sound(int s,int c) {sound=s;channel=c;}
int main(void)
{
    Effect p,q,before; int i,j,expected,d; long script[3]={0,1,2},end=-1;
    subject=&p; g_focus=&q;
    memset(&p,0,sizeof(p)); p.z=0x12340000; initialize_backdrop(&p);
    CHECK(p.render_flags==0x90000000 && p.flags==0x20000 && p.step==512);
    CHECK(p.scale==1023 && p.sprite==14 && p.x==0x4000000 && p.z==0x12440000 && p.mode==4);
    initialize_particle(&p); CHECK(p.flags==2 && p.category==1 && p.ticks==0);
    p.y=0x30000;tick_particle(&p);CHECK(p.y==0x20000 && p.ticks==1 && freed==1);
    tick_particle(&p);CHECK(p.y==0x10000 && p.ticks==0 && freed==1);
    random_values[0]=1;random_calls=0;initialize_flicker(&p);
    CHECK(p.sprite==93 && p.phase==1 && (p.render_flags&0x80000005)==0x80000005);
    for(i=0;i<4096;i+=127) for(j=0;j<4096;j+=113) {
        expected=j-i; if(expected<0)expected+=4096; if(expected>=2048)expected-=4096;
        CHECK(wrapped_delta(i*65536L,j*65536L)==expected);
    }
    for(i=-1025;i<=1025;++i) {
        p.x=2048L*65536; q.x=(2048L+i)*65536;p.phase=0;g_clock=1;
        tick_flicker(&p);d=wrapped_delta(p.x,q.x);
        CHECK(!!(p.render_flags&0x80000000)==!(d<1024 && d>=-1024));
    }
    p.sprite=100;p.phase=1;g_clock=1;tick_flicker(&p);CHECK(p.sprite==93);
    for(i=0;i<4;++i) for(j=0;j<4;++j) {
        g_delay=g_busy=g_blocked=0; random_calls=0;random_values[0]=i;random_values[1]=j;selected=sound=0;
        maybe_start_effect(&p);CHECK(g_busy==(i<3));CHECK(selected==(i<3?3:0));CHECK(sound==(i<3 && j==0?450:0));
    }
    g_delay=2;g_busy=0;random_calls=0;maybe_start_effect(&p);CHECK(g_delay==1 && random_calls==0);
    g_delay=0;g_busy=1;maybe_start_effect(&p);CHECK(random_calls==0);
    g_busy=0;g_blocked=1;random_values[0]=0;maybe_start_effect(&p);CHECK(g_busy==0);
    for(i=0;i<3;++i) {g_script_cursor=script+i;next_script_frame(&p);CHECK(selected==(i==0?2:i==1?6:5));CHECK(g_script_cursor==script+i+1);}
    g_script_start=script;g_script_cursor=&end;next_script_frame(&p);CHECK(selected==2 && g_script_cursor==script+1);
    p.state=8;g_busy=1;reset_effect(&p);CHECK(p.state==0 && !g_busy && g_cooldown==30);
    p.scale=500;initialize_burst(&p);CHECK(p.scale==1500 && p.ticks==0 && p.sprite==0);
    tick_burst(&p);CHECK(p.ticks==2 && p.sprite==134 && !finished);
    before=p;tick_burst(&p);CHECK(p.ticks==1 && p.sprite==before.sprite);
    p.ticks=0;p.sprite=147;tick_burst(&p);CHECK(p.ticks==2 && p.sprite==148 && finished==1);
    p.scale=12;initialize_sound_effect(&p);
    CHECK(p.mode==5 && p.scale==36 && p.phase==0 && p.ticks==0 && p.speed==128);
    CHECK(p.x==0x4000000 && p.y==0x2e00000 && p.z==0x2900000 && sound==467 && channel==0);
    printf("%u semantic checks; %u failures\n",checks,failures);return failures?1:0;
}
