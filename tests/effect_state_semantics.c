/* Canonical-only tests; original binaries are never loaded. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../src/shared/effect_state.c"
typedef char layout[(offsetof(Effect,render_flags)==0x54 && offsetof(Effect,scale)==0x3a && offsetof(Effect,ticks)==0x30 && offsetof(Effect,step)==0x72)?1:-1];
static unsigned checks,failures;
#define CHECK(c) do {++checks; if(!(c)){++failures;printf("FAIL line %d\n",__LINE__);}}while(0)
int main(void)
{
    Effect p,expected;int i,j,d;long x,y;
    for(i=0;i<256;++i) {
        memset(&p,i,sizeof(p));p.z=(i-128)*65536L;expected=p;
        expected.render_flags=0x90000000;expected.flags|=0x20000;
        expected.step=512;expected.scale=1023;expected.sprite=14;
        expected.x=0x4000000;expected.z+=0x100000;expected.mode=4;
        initialize_backdrop(&p);CHECK(memcmp(&p,&expected,sizeof(p))==0);
        expected=p;expected.flags=2;expected.category=1;expected.ticks=0;
        initialize_particle(&p);CHECK(memcmp(&p,&expected,sizeof(p))==0);
        p.scale=(short)(i*257-32768);expected=p;expected.render_flags|=5;
        expected.scale=(short)((long)p.scale*3);expected.ticks=0;expected.sprite=0;
        initialize_burst(&p);CHECK(memcmp(&p,&expected,sizeof(p))==0);
    }
    /* Wrapped world coordinates including the exact half-world boundaries. */
    for(i=0;i<4096;++i)for(j=0;j<4;++j) {
        x=i*65536L+123;y=((i+2047+j)&4095)*65536L+456;
        d=(y>>16)-(x>>16);if(d<0)d+=4096;if(d>=2048)d-=4096;
        CHECK(wrapped_delta(x,y)==d);
    }
    CHECK(wrapped_delta(0,0)==0);
    printf("%u semantic checks; %u failures\n",checks,failures);return failures?1:0;
}
