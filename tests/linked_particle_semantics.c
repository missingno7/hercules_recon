#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "../src/shared/linked_particle.c"

static unsigned checks, failures;
#define CHECK(c) do { ++checks; if(!(c)) { ++failures; if(failures<10) printf("FAIL %d\n",__LINE__); } } while(0)
typedef char layout[(offsetof(Actor,frame)==0x34 && offsetof(Actor,scale)==0x3a &&
    offsetof(Actor,field_4a)==0x4a && offsetof(Actor,parameter)==0x4c &&
    offsetof(Actor,flags)==0x50 && offsetof(Actor,render_flags)==0x54 && offsetof(Actor,owner)==0x5c)?1:-1];
int main(void)
{
    Actor p,owner,expected;
    long parameters[]={-100000,-81,-21,-1,0,1,21,79,80,81,100000};
    long offsets[]={-1025,-1024,-1,0,1,1023,1024,1025};
    long scale; int fill,i,j,k;
    for(fill=0;fill<256;++fill) {
        memset(&p,fill,sizeof(p)); expected=p;
        expected.field_4a=0;expected.flags|=0x10000;
        expected.frame=25;expected.render_flags|=0x684;
        initialize_linked_particle(&p);
        CHECK(memcmp(&p,&expected,sizeof(p))==0);
    }
    for(i=0;i<11;++i)for(j=0;j<8;++j)for(k=0;k<3;++k) {
        memset(&p,0x55,sizeof(p));memset(&owner,0,sizeof(owner));
        owner.x=-1234567;owner.z=7654321;owner.y=0x2e00000-offsets[j]*524288L+k-1;
        p.owner=&owner;p.parameter=parameters[i];expected=p;
        expected.x=owner.x;expected.y=0x2e00000;expected.z=owner.z;
        scale=((0x2e00000-owner.y)>>19)+(parameters[i]*256/20);
        if(scale>1023)scale=1023;expected.scale=(short)scale;
        follow_owner_scale(&p);CHECK(memcmp(&p,&expected,sizeof(p))==0);
    }
    /* Self-owner reads see the y store, as in the original instruction order. */
    memset(&p,0x33,sizeof(p));p.owner=&p;p.parameter=-21;expected=p;
    expected.y=0x2e00000;expected.scale=(short)(-21*256/20);
    follow_owner_scale(&p);CHECK(memcmp(&p,&expected,sizeof(p))==0);
    printf("%u semantic checks; %u failures\n",checks,failures);
    return failures?1:0;
}
