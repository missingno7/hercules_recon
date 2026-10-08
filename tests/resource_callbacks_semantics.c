#include <stdio.h>
#include <string.h>
#include "../calibration/resource_callbacks.c"
ActorResource g_actor_resources[81];
int g_loading_resource;
ResourceCallbackContext context;
ResourceCallbackContext *g_resource_context=&context;
static unsigned long checks, failures;
static int cancelled, destroyed, mutation;
static void marker(void) {}
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; printf("line %d\n",__LINE__); } } while (0)
void __cdecl host_cancel_load(void) {
    ++cancelled;
    CHECK(context.load_complete==0 && context.load_failed==0);
    CHECK(g_actor_resources[0].residency_state==2);
    if(mutation) { g_loading_resource=1;g_actor_resources[1].residency_state=4; }
}
void __cdecl resource_destroy(int id) {
    destroyed=id;
    if(mutation) g_loading_resource=2;
}
int main(void) {
    unsigned long state;
    for(state=0;state<65536UL;++state) {
        g_actor_resources[0].residency_state=(unsigned short)state;
        resource_reset(0);
        CHECK(g_actor_resources[0].residency_state==(state==0?1:state));
        g_actor_resources[0].residency_state=(unsigned short)state;
        g_loading_resource=0;
        resource_load_complete();
        CHECK(g_actor_resources[0].residency_state==(state==2?3:state));
        g_actor_resources[0].residency_state=(unsigned short)state;
        cancelled=0;
        context.load_complete=context.load_failed=marker;
        resource_cancel(0);
        CHECK(g_actor_resources[0].residency_state==((state>=1&&state<=3)?4:state));
        CHECK(cancelled==(state==2));
        CHECK((context.load_complete==0)==(state==2));
        CHECK((context.load_failed==0)==(state==2));
    }
    mutation=1;cancelled=0;destroyed=-1;
    g_actor_resources[2].residency_state=0;
    g_loading_resource=0;g_actor_resources[0].residency_state=2;
    resource_load_failed();
    CHECK(cancelled==1);
    CHECK(g_actor_resources[0].residency_state==4);
    CHECK(destroyed==1);
    CHECK(g_actor_resources[2].residency_state==1);
    CHECK(g_actor_resources[1].residency_state==4);
    mutation=0;destroyed=-1;
    g_loading_resource=3;g_actor_resources[3].residency_state=6;
    resource_load_failed();
    CHECK(destroyed==-1 && g_actor_resources[3].residency_state==6);
    printf("resource_callbacks: %lu checks, %lu failures\n",checks,failures);
    return failures?1:0;
}
