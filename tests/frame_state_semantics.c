#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "../src/shared/frame_state.c"
typedef char layout[(offsetof(Actor,next_chain)==0x130 && offsetof(Actor,frames)==0x110)?1:-1];
static unsigned checks,failures;
#define CHECK(c) do { ++checks; if(!(c)) {++failures;printf("FAIL %d\n",__LINE__);} }while(0)
int main(void)
{
    Actor p,nodes[20]; void *frames[5]; int i,j;
    memset(&p,0,sizeof(p));memset(nodes,0,sizeof(nodes));
    for(i=-128;i<128;++i) {
        p.frame_ticks=i;p.frame_state=-i;reset_frame_state(&p);
        CHECK(!p.frame_ticks && !p.frame_state);
        p.initial_steps=nodes+(i&15);p.step_index=i;p.active_step=-1;
        restart_steps(&p);CHECK(p.steps==p.initial_steps && !p.step_index && p.active_step==1);
    }
    for(i=0;i<5;++i)frames[i]=nodes+i;
    p.frames=frames;
    for(i=0;i<5;++i){select_frame(&p,i);CHECK(p.frame_index==i && p.image==frames[i] && p.frame_state==-1 && !p.frame_ticks);}
    p.frame_index=0;advance_frame(&p);CHECK(p.frame_index==1 && p.image==nodes+1);
    frames[2]=0;frames[3]=(void *)4;p.frame_index=1;advance_frame(&p);
    CHECK(p.frame_index==4 && p.image==nodes+4);
    p.next_chain=0;p.kind=0;
    for(i=0;i<20;++i){nodes[i].kind=0;append_chain(&p,nodes+i);CHECK(count_chain(&p)==i+1);}
    for(i=0;i<20;++i){CHECK(nodes[i].prev_chain==(i?nodes+i-1:&p));CHECK(nodes[i].next_chain==(i==19?0:nodes+i+1));}
    for(i=0;i<4;++i)for(j=0;j<4;++j){p.kind=i<<13;nodes[0].kind=j<<13;p.next_chain=0;nodes[0].next_chain=0;append_chain(&p,nodes);CHECK(!!p.next_chain==(!i && !j));}
    printf("%u semantic checks; %u failures\n",checks,failures);return failures?1:0;
}
