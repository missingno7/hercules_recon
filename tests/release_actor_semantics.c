#include <stdio.h>
#include <string.h>
#include "../calibration/release_actor_repaired.c"
KindInfo g_kind_info[65536];
signed char g_special_kind;
static unsigned checks, failures;
static int calls, order[4], mutate;
static Actor snapshots[4];
static Sidecar side_snapshots[4], *active_side;
static Actor alternate;
#define CHECK(c) do { ++checks; if (!(c)) { if(failures<10) printf("FAIL line %d\n",__LINE__); ++failures; } } while(0)
static void event(Actor *p, int id) {
    order[calls]=id; snapshots[calls]=*p;
    if(active_side) side_snapshots[calls]=*active_side;
    ++calls;
}
void detach_child(Actor *p) {event(p,1); if(mutate){p->kind=0x7777; p->owner_context=&alternate; active_side->flags=0xffff; p->x=99;}}
void detach_buffer(Actor *p) {event(p,2); if(mutate) active_side->flags&=~0x100;}
void detach_attachment(Actor *p) {event(p,3); if(mutate){p->kind=0x8888; active_side->flags|=0x8000;}}
void clear_actor(Actor *p) {event(p,4);}
int remove_actor(Actor *p) {event(p,5); return -17;}
static void reset(void){calls=mutate=0;active_side=0; memset(order,0,sizeof(order));memset(snapshots,0,sizeof(snapshots));memset(side_snapshots,0,sizeof(side_snapshots));}
static void setup(Actor *p,Sidecar *side,unsigned profile,unsigned flags,int special) {
    memset(p,0,sizeof(*p));memset(side,0,sizeof(*side));reset();
    p->kind=0x2000;p->profile=(unsigned short)profile;p->x=11;p->y=22;p->z=33;p->field_02a=0x234;p->owner_context=side;
    side->x=-1;side->y=-2;side->z=-3;side->field_00e=-4;side->flags=(unsigned short)flags;
    g_special_kind=(signed char)special;active_side=side;
}
static void check_case(unsigned profile,unsigned flags,int special) {
    Actor p; Sidecar side; int r,eq,copyxyz,copyword;
    setup(&p,&side,profile,flags,special);
    eq=profile==(unsigned short)(signed char)special;copyxyz=eq && (flags&0x100);copyword=eq && (flags&0x180);
    r=release_actor(&p);
    if(flags&0x2000) {CHECK(r==0&&calls==0&&p.kind==0x2000&&side.flags==flags);CHECK(side.x==-1&&side.y==-2&&side.z==-3&&side.field_00e==-4);return;}
    CHECK(r==1&&calls==4&&order[0]==1&&order[1]==2&&order[2]==3&&order[3]==4);
    CHECK(side.x==(copyxyz?11:-1)&&side.y==(copyxyz?22:-2)&&side.z==(copyxyz?33:-3)&&side.field_00e==(copyword?0x234:-4));
    CHECK(p.kind==0&&side.flags==(flags&0x7fff));
    CHECK(snapshots[0].kind==0x2000&&snapshots[1].kind==0x2000&&snapshots[2].kind==0x2000&&snapshots[3].kind==0);
    CHECK(side_snapshots[0].flags==flags&&side_snapshots[1].flags==flags&&side_snapshots[2].flags==flags&&side_snapshots[3].flags==(flags&0x7fff));
    CHECK(side_snapshots[0].x==side.x&&side_snapshots[0].field_00e==side.field_00e);
}
static void alias_case(unsigned offset,unsigned profile,int special) {
    Actor p,expected; Sidecar *side,*eside;unsigned short sf,cached_profile;int r; long v; short w;
    memset(&p,0,sizeof(p));p.x=11;p.y=22;p.z=33;p.kind=0x2000;p.profile=(unsigned short)profile;p.field_02a=0x234;
    side=(Sidecar *)((unsigned char *)&p+offset);side->flags=0x8180;
    p.owner_context=side; cached_profile=p.profile;sf=side->flags;
    expected=p;eside=(Sidecar *)((unsigned char *)&expected+offset);
    if((sf&0x100)&&cached_profile==(unsigned short)(signed char)special){v=expected.x;eside->x=v;v=expected.y;eside->y=v;v=expected.z;eside->z=v;w=expected.field_02a;eside->field_00e=w;}
    if((sf&0x80)&&expected.profile==(unsigned short)(signed char)special){w=expected.field_02a;eside->field_00e=w;}
    expected.kind=0;eside->flags&=0x7fff;
    reset();g_special_kind=(signed char)special;active_side=side;r=release_actor(&p);
    CHECK(r==1&&calls==4&&memcmp(&p,&expected,sizeof(p))==0);
    CHECK(snapshots[3].kind==0&&order[0]==1&&order[1]==2&&order[2]==3&&order[3]==4);
}
int main(void) {
    unsigned profile,n,flags;int special,r;Actor p;Sidecar side;
    struct { unsigned char prefix[16]; Sidecar side; } shifted;
    unsigned profiles[]={0,5,6,127,128,0xff7f,0xff80,0xfffe,0xffff};
    for(profile=0;profile<65536;++profile)for(n=0;n<8;++n){flags=(n&1?0x100:0)|(n&2?0x80:0)|(n&4?0x2000:0)|0x8000;check_case(profile,flags,-128);}
    for(profile=0;profile<sizeof(profiles)/sizeof(profiles[0]);++profile)for(special=-128;special<128;++special)for(n=0;n<16;++n){flags=(n&1?0x100:0)|(n&2?0x80:0)|(n&4?0x2000:0)|(n&8?0x8000:0);check_case(profiles[profile],flags,special);}
    setup(&p,&side,5,0x8180,5);mutate=1;r=release_actor(&p);
    CHECK(r==1&&calls==4&&side.x==11&&p.x==99&&p.kind==0&&p.owner_context==&alternate&&side.flags==0x7eff);
    CHECK(snapshots[0].kind==0x2000&&snapshots[1].kind==0x7777&&snapshots[2].kind==0x7777&&snapshots[3].kind==0);
    CHECK(side_snapshots[0].flags==0x8180&&side_snapshots[1].flags==0xffff&&side_snapshots[2].flags==0xfeff&&side_snapshots[3].flags==0x7eff);
    setup(&p,&side,5,0x8180,5);p.kind=0;r=release_actor(&p);CHECK(r==1&&calls==0&&side.flags==0x8180);
    setup(&p,&side,5,0x8180,5);p.kind=0x1000;r=release_actor(&p);CHECK(r==-17&&calls==1&&order[0]==5&&p.kind==0x1000);
    setup(&p,&side,5,0x8180,5);p.flags=0x10000000;r=release_actor(&p);CHECK(r==1&&calls==1&&order[0]==4&&snapshots[0].kind==0&&side.flags==0x8180);
    setup(&p,&side,5,0x8180,5);p.owner_context=0;r=release_actor(&p);CHECK(r==1&&calls==4&&snapshots[0].kind==0x2000&&snapshots[3].kind==0&&side.flags==0x8180);
    setup(&p,&shifted.side,5,0x8080,5); memset(shifted.prefix,0x55,sizeof(shifted.prefix));p.owner_context=&shifted;g_kind_info[5].sidecar_offset=16;r=release_actor(&p);
    CHECK(r==1&&calls==4&&shifted.side.x==-1&&shifted.side.field_00e==0x234&&shifted.side.flags==0x80);
    CHECK(shifted.prefix[0]==0x55&&shifted.prefix[15]==0x55);g_kind_info[5].sidecar_offset=0;
    alias_case(0,5,5);alias_case(4,5,5);alias_case(0x20,5,5);alias_case(0x34,5,5);alias_case(0,0xff80,-128);alias_case(0x34,0xffff,-1);
    printf("%u checks; %u failures\n",checks,failures);return failures?1:0;
}
