/* Experimental source with synthetic local data/callbacks only. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../calibration/macro_actor.c"

#define LAYOUT(name, expression) typedef char name[(expression) ? 1 : -1]
LAYOUT(actor_kind, offsetof(Actor,kind)==0x2e);
LAYOUT(actor_flags, offsetof(Actor,flags)==0x50);
LAYOUT(actor_attachment, offsetof(Actor,attachment)==0x5c);
LAYOUT(actor_steps, offsetof(Actor,steps)==0xf0);
LAYOUT(actor_frames, offsetof(Actor,frames)==0x110);
LAYOUT(actor_initial_steps, offsetof(Actor,initial_steps)==0x118);
LAYOUT(actor_active_step, offsetof(Actor,active_step)==0x11e);
LAYOUT(actor_command, offsetof(Actor,command)==0x128);
LAYOUT(actor_next_chain, offsetof(Actor,next_chain)==0x130);
LAYOUT(asset_stride8, sizeof(Asset8)==8);
LAYOUT(asset_stride16, sizeof(Asset16)==16);
LAYOUT(callback_stride8, sizeof(Callback8)==8);
LAYOUT(callback_stride12, sizeof(Callback12)==12);

Asset8 *g_special_assets, *g_alternate_assets;
Asset16 *g_normal_assets;
Callback8 *g_special_callbacks, *g_alternate_callbacks;
Callback12 *g_normal_callbacks;
Actor *g_scene_anchor;
unsigned long g_tag_list[4];
static unsigned short words[10], *words_pointer=words;
static int allocation_fails, route, refreshed;
static Actor *released, *buffer_released;
static unsigned checks, failures;
#define CHECK(test) do { ++checks; if (!(test)) { ++failures; \
    printf("FAIL line %d: %s\n",__LINE__,#test); } } while (0)

unsigned short **allocate_command(unsigned int opcode)
{ CHECK(opcode==0x3781); return allocation_fails ? 0 : &words_pointer; }
void release_actor(Actor *p) { released=p; }
void release_buffer(Actor *p) { buffer_released=p; }
void refresh_actor(Actor *p) { CHECK(p->frame_ticks==0 && p->frame_state==0); ++refreshed; }
static void special(Actor *p) { route=1; p->frame_ticks=22; }
static void alternate(Actor *p) { route=2; p->frame_ticks=23; }
static void normal(Actor *p) { route=3; p->frame_ticks=24; }

int main(void)
{
    Actor p,a,b,c,before;
    Asset8 special_assets[2],alternate_assets[2];
    Asset16 normal_assets[2];
    Callback8 special_callbacks[2],alternate_callbacks[2];
    Callback12 normal_callbacks[2];
    void *frames[3];
    int i;
    memset(&p,0,sizeof(p)); memset(&a,0,sizeof(a));
    memset(&b,0,sizeof(b)); memset(&c,0,sizeof(c));
    frames[0]=&a; frames[1]=0; frames[2]=0; /* null then index-zero redirect */
    p.frames=frames; p.frame_index=0;
    advance_frame(&p); CHECK(p.frame_index==0 && p.image==&a);
    p.frame_ticks=8; select_frame(&p,0);
    CHECK(p.image==&a && p.frame_state==-1 && p.frame_ticks==0);
    p.initial_steps=&b; p.step_index=42; restart_steps(&p);
    CHECK(p.steps==&b && p.step_index==0 && p.active_step==1);
    g_special_assets=special_assets; g_alternate_assets=alternate_assets;
    g_normal_assets=normal_assets; g_special_callbacks=special_callbacks;
    g_alternate_callbacks=alternate_callbacks; g_normal_callbacks=normal_callbacks;
    special_assets[1].image=&a; special_assets[1].category=11; special_callbacks[1].init=special;
    alternate_assets[1].image=&b; alternate_assets[1].category=12; alternate_callbacks[1].init=alternate;
    normal_assets[1].image=&c; normal_assets[1].category=13; normal_callbacks[1].init=normal;
    p.kind=0x4000; change_kind(&p,1); CHECK(route==1 && p.kind==1 && p.image==&a && p.category==11);
    p.kind=0x2000; change_kind(&p,1); CHECK(route==2 && p.image==&b && p.category==12);
    p.kind=0; change_kind(&p,1); CHECK(route==3 && p.image==&c && p.category==13 && refreshed==3);
    for (i=0;i<10;++i) words[i]=0x3781;
    CHECK(create_command(&p)==1 && p.command==&words_pointer && (p.flags&0x20000000));
    CHECK(words[0]==0xb781);
    for (i=1;i<10;++i) CHECK(words[i]==0);
    allocation_fails=1; before=p; CHECK(create_command(&p)==0);
    CHECK(memcmp(&p,&before,sizeof(p))==0);
    p.next_chain=0; p.kind=0; append_chain(&p,&a); append_chain(&p,&b);
    CHECK(count_chain(&p)==2 && p.next_chain==&a && a.next_chain==&b && b.prev_chain==&a);
    c.kind=0x2000; append_chain(&p,&c); CHECK(count_chain(&p)==2);
    hide_and_unlink(&a); CHECK(p.next_chain==&b && b.prev_chain==&p && !a.prev_chain && !a.next_chain);
    p.attachment=&a; p.flags=0; a.flags=0x10000; detach_attachment(&p);
    CHECK(!p.attachment && released==&a);
    p.attachment=&a; a.attachment=&p; p.flags=0x10000; detach_attachment(&p);
    CHECK(p.attachment==&a && !a.attachment);
    p.flags=0x20000; before=p; detach_attachment(&p); CHECK(memcmp(&p,&before,sizeof(p))==0);
    p.flags=0; p.child=&b; p.render_flags=0xffffffff; detach_child(&p);
    CHECK(!p.child && released==&b && p.render_flags==0xff7fffff);
    p.buffer=&c; detach_buffer(&p); CHECK(!p.buffer && buffer_released==&p);
    p.flags=0x10000000; p.buffer=&c; before=p; detach_buffer(&p); CHECK(memcmp(&p,&before,sizeof(p))==0);
    a.next_scene=&b; b.prev_scene=&a; b.next_scene=0;
    p.flags=0; g_scene_anchor=&a; CHECK(insert_scene(&p,0)==&a);
    CHECK(a.next_scene==&p && p.prev_scene==&a && p.next_scene==&b && b.prev_scene==&p);
    unlink_0e4(&p); CHECK(a.next_scene==&b && b.prev_scene==&a && !p.next_scene && !p.prev_scene);
    p.flags=0x80000; CHECK(insert_scene(&p,&a)==0);
    a.tag=17; b.tag=255; c.tag=42;
    g_tag_list[0]=3; g_tag_list[1]=(unsigned long)&a; g_tag_list[2]=(unsigned long)&b; g_tag_list[3]=(unsigned long)&c;
    CHECK(find_tag(17)==&a && find_tag(255)==&b && find_tag(42)==&c);
    CHECK(find_tag(256)==0 && find_tag(-1)==0);
    g_tag_list[0]=0; CHECK(find_tag(17)==0);
    printf("%u semantic checks; %u failures\n",checks,failures);
    return failures ? 1 : 0;
}
