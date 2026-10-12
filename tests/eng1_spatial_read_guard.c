#include <setjmp.h>
#include <stdio.h>
#include "../src/eng1/behavioral/initial_spatial_roots.h"
#include "../src/eng1/behavioral/spatial_read_guard.h"
static jmp_buf escape;
static unsigned long reported;
void eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    reported=rva;
    if (!reason || !*reason) reported=0;
    longjmp(escape,1);
}
typedef struct FixtureLimit { const void *base; unsigned long rva,rows,stride; } FixtureLimit;
static const FixtureLimit limits[] = {
    {g_eng1_spatial_types_068170,0x68170,97,10},
    {g_eng1_spatial_types_04ede8,0x4ede8,101,10},
    {g_eng1_spatial_types_061130,0x61130,148,10},
    {g_eng1_spatial_types_0587a0,0x587a0,13,10},
    {g_eng1_spatial_types_06fe60,0x6fe60,7,10},
    {g_eng1_spatial_types_062bf8,0x62bf8,82,10},
    {g_eng1_collision_map_04f1e0,0x4f1e0,32,2},
    {g_eng1_collision_map_058830,0x58830,24,2},
    {g_eng1_collision_map_059fd8,0x59fd8,32,2},
    {g_eng1_collision_map_0616f8,0x616f8,32,2},
    {g_eng1_collision_map_0630a8,0x630a8,30,2},
    {g_eng1_collision_map_068590,0x68590,36,2},
    {g_eng1_collision_map_06b118,0x6b118,32,2},
    {&g_eng1_spatial_identity_06ff90,0x6ff90,0,10},
    {&g_eng1_spatial_identity_0702a8,0x702a8,0,10}
};
static int reject(const FixtureLimit *p,unsigned long index,unsigned long stride,unsigned long width)
{
    reported=0;
    if (setjmp(escape)==0) {
        eng1_guard_spatial_read(p->base,index,stride,width,"fixture");
        return 0;
    }
    return reported==p->rva;
}
int main(void)
{
    unsigned int i;
    unsigned short dynamic_words[2];
    for (i=0;i<sizeof(limits)/sizeof(limits[0]);++i) {
        const FixtureLimit *p=&limits[i];
        if (p->rows) {
            if (setjmp(escape)) return 1;
            eng1_guard_spatial_read(p->base,0,p->stride,p->stride,"first");
            eng1_guard_spatial_read(p->base,p->rows-1,p->stride,p->stride,"last");
        }
        if (!reject(p,p->rows,p->stride,p->stride)) return 2;
        if (!reject(p,0xffffffffUL,p->stride,p->stride)) return 3;
        if (!reject(p,0,0,p->stride)) return 4;
        if (!reject(p,0,p->stride,p->stride+1)) return 5;
    }
    if (setjmp(escape)) return 6;
    eng1_guard_spatial_read(dynamic_words,0xffffffffUL,2,2,"unknown runtime owner");
    puts("15 spatial owners: first/last bounds, unknown extents and overflow checks passed");
    return 0;
}
