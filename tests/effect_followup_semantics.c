/* Full 16-bit state coverage and callback observations for the three follow-ups. */
#include <stdio.h>
#include <string.h>
#include "../calibration/refinery_blind.c"
Effect *g_focus;
unsigned long g_clock;
long g_delay,g_busy,g_blocked,g_cooldown;
long *g_script_cursor,*g_script_start;
static Effect *subject,observed;
static unsigned checks,failures,random_calls,values[2];
static int released,sounds,selected,last_sound,last_channel,block_on_random;
#define CHECK(c) do {++checks;if(!(c)){++failures;printf("FAIL %d\n",__LINE__);}}while(0)
void release_effect(Effect *p){CHECK(p==subject);observed=*p;++released;}
void finish_effect(Effect *p){CHECK(p==subject);}
unsigned int random_limit(int n){CHECK(n==4);if(block_on_random)g_blocked=1;return values[random_calls++ % 2];}
void select_effect_frame(Effect *p,int frame){CHECK(p==subject && frame==3 && g_busy==1);++selected;}
void play_effect_sound(int sound,int channel){last_sound=sound;last_channel=channel;observed=*subject;++sounds;}
int main(void)
{
    Effect p,expected;int i,delay,busy,blocked,first,second,mutate,starts;
    subject=&p;
    for(i=-32768;i<=32767;++i){
        memset(&p,(unsigned char)i,sizeof(p));p.ticks=(short)i;p.y=12345678;
        expected=p;expected.ticks=(short)((unsigned short)i ^ 1);expected.y-=65536;
        released=0;tick_particle(&p);
        CHECK(memcmp(&p,&expected,sizeof(p))==0);CHECK(released==!!expected.ticks);
        if(released)CHECK(memcmp(&observed,&expected,sizeof(p))==0);
        p.scale=(short)i;expected=p;
        expected.mode=5;expected.render_flags|=5;expected.phase=expected.ticks=0;
        expected.scale=(short)((long)i*3);expected.x=0x4000000;
        expected.y=0x2e00000;expected.z=0x2900000;expected.speed=128;
        sounds=0;initialize_sound_effect(&p);
        CHECK(memcmp(&p,&expected,sizeof(p))==0);
        CHECK(sounds==1 && last_sound==467 && last_channel==0);
        CHECK(memcmp(&observed,&expected,sizeof(p))==0);
    }
    for(delay=-1;delay<=1;++delay)for(busy=0;busy<2;++busy)
    for(blocked=0;blocked<2;++blocked)for(first=0;first<4;++first)
    for(second=0;second<4;++second)for(mutate=0;mutate<2;++mutate){
        g_delay=delay;g_busy=busy;g_blocked=blocked;block_on_random=mutate;
        random_calls=0;values[0]=first;values[1]=second;selected=sounds=0;
        expected=p;maybe_start_effect(&p);
        starts=!delay && !busy && first<3 && !blocked && !mutate;
        CHECK(memcmp(&p,&expected,sizeof(p))==0);
        CHECK(g_delay==(delay?delay-1:0));CHECK(g_busy==(starts?1:busy));
        CHECK(selected==starts);CHECK(sounds==(starts && !second));
        CHECK(random_calls==(delay || busy?0:starts?2:1));
        if(sounds)CHECK(last_sound==450 && last_channel==0);
    }
    printf("%u semantic checks; %u failures\n",checks,failures);return failures?1:0;
}
