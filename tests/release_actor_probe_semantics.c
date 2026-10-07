/* Historical twelve-case probe. It missed the PC's independent sidecar flag
 * paths and is excluded from registered suites. Use release_actor_semantics.c. */
#include <stdio.h>
#include <string.h>
#include "../calibration/release_actor.c"

KindInfo g_kind_info[16];
signed char g_special_kind;
static int removed, cleared, detach_count, detach_order[4], remove_result;
static unsigned checks, failures;
#define CHECK(test) do { ++checks; if (!(test)) { ++failures; printf("FAIL line %d: %s\n", __LINE__, #test); } } while (0)

void detach_child(Actor *p) { (void)p; detach_order[detach_count++]=1; }
void detach_buffer(Actor *p) { (void)p; detach_order[detach_count++]=2; }
void detach_attachment(Actor *p) { (void)p; detach_order[detach_count++]=3; }
void clear_actor(Actor *p) { (void)p; ++cleared; }
int remove_actor(Actor *p) { (void)p; ++removed; return remove_result; }
static void reset_calls(void)
{ removed=0; cleared=0; detach_count=0; memset(detach_order,0,sizeof(detach_order)); }

int main(void)
{
    Actor p;
    Sidecar side;
    int result;
    memset(&p,0,sizeof(p)); reset_calls();
    result=release_actor(&p);
    CHECK(result==1 && removed==0 && cleared==0 && detach_count==0);

    memset(&p,0,sizeof(p)); p.kind=0x1000; reset_calls(); remove_result=7;
    result=release_actor(&p);
    CHECK(result==7 && removed==1 && cleared==0 && detach_count==0);

    memset(&p,0,sizeof(p)); p.kind=0x2000; p.flags=0x10000000; reset_calls();
    result=release_actor(&p);
    CHECK(result==1 && p.kind==0 && cleared==1 && removed==0 && detach_count==0);

    memset(&p,0,sizeof(p)); p.kind=0x2000; reset_calls();
    result=release_actor(&p);
    CHECK(result==1 && p.kind==0 && cleared==1 && removed==0 && detach_count==3);
    CHECK(detach_order[0]==1 && detach_order[1]==2 && detach_order[2]==3);

    memset(&p,0,sizeof(p)); memset(&side,0,sizeof(side)); p.kind=0x2000; p.owner_context=&side; side.flags=0x2000; reset_calls();
    result=release_actor(&p);
    CHECK(result==0 && removed==0 && cleared==0 && detach_count==0);

    memset(&p,0,sizeof(p)); memset(&side,0,sizeof(side)); p.kind=0x2000; p.profile=5; p.owner_context=&side; p.x=11; p.y=22; p.z=33; p.field_02a=0x345; side.flags=0x0100; g_special_kind=5; reset_calls(); remove_result=7;
    result=release_actor(&p);
    CHECK(result==7 && side.x==11 && side.y==22 && side.z==33 && side.field_00e==0x345);
    CHECK(removed==1 && cleared==0 && detach_count==0);

    memset(&p,0,sizeof(p)); memset(&side,0,sizeof(side)); p.kind=0x2000; p.profile=5; p.owner_context=&side; p.x=44; p.y=55; p.z=66; p.field_02a=0x234; side.flags=0x8180; g_special_kind=5; reset_calls();
    result=release_actor(&p);
    CHECK(result==1 && p.kind==0 && cleared==1 && removed==0 && detach_count==3);
    CHECK(side.x==44 && side.y==55 && side.z==66 && side.field_00e==0x234 && (side.flags&0x8000)==0);

    memset(&p,0,sizeof(p)); memset(&side,0,sizeof(side)); p.kind=0x2000; p.profile=6; p.owner_context=&side; p.x=77; side.flags=0x0100; g_special_kind=5; reset_calls(); remove_result=7;
    result=release_actor(&p);
    CHECK(result==7 && removed==1 && side.x==0 && cleared==0 && detach_count==0);

    memset(&p,0,sizeof(p)); memset(&side,0,sizeof(side)); p.kind=0x2000; p.profile=5; p.owner_context=&side; side.flags=0x0080; g_special_kind=5; reset_calls(); remove_result=7;
    result=release_actor(&p);
    CHECK(result==7 && removed==1 && cleared==0 && detach_count==0);

    printf("%u semantic checks; %u failures\n",checks,failures);
    return failures ? 1 : 0;
}
