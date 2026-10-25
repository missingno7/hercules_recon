/* Three finite native/source alternate-bootstrap comparisons. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/simulation_data_owners.h"
extern void *g_alternate_actor_pool;
extern void __cdecl eng1_call_12960(void);
extern unsigned long g_eng1_call_12960_dword_52b50[8];
/* Link-only allocator cells; the actual initializer never accesses them. */
unsigned short g_normal_cursor_primary,g_normal_limit_primary,g_normal_limit_wrap;
unsigned short g_normal_cursor_alternate,g_normal_limit_wrap_start;
unsigned short g_alternate_cursor_primary,g_alternate_limit_primary,g_alternate_limit_wrap;
unsigned short g_alternate_cursor_alternate,g_alternate_limit_wrap_start;
#define POOL_BYTES 0x5c8U
static union { unsigned long align; unsigned char bytes[POOL_BYTES+16]; } native_pool,source_pool;
static unsigned int checks,failures;
static void check(int ok,const char *name) {++checks; if(!ok) {++failures;printf("FAIL %s\n",name);}}
void __cdecl eng1_incomplete_behavior(unsigned long rva,const char *reason) {
    fprintf(stderr,"unsupported RVA 0x%lx: %s\n",rva,reason);fflush(stderr);ExitProcess(0xe1U);
}
int main(int argc,char **argv) {
    HMODULE dll;
    unsigned char *base;
    unsigned long old_pool,old_x[8],source_x[8];
    unsigned short old_word[8],source_word[8];
    unsigned int scenario,i;
    void (__cdecl *entry)(void);
    if(argc==3) {g_alternate_actor_pool=0;eng1_call_12960();return 4;}
    if(argc!=2)return 2;
    dll=LoadLibraryA(argv[1]);if(!dll)return 3;
    base=(unsigned char *)(void *)dll;
    entry=(void (__cdecl *)(void))(base+0x12960UL);
    old_pool=*(unsigned long *)(void *)(base+0x734e0UL);
    for(i=0;i<8U;++i) {
        old_x[i]=*(unsigned long *)(void *)(base+0x52b50UL+i*4U);
        source_x[i]=g_eng1_call_12960_dword_52b50[i];
        old_word[i]=*(unsigned short *)(void *)(base+0x52b70UL+i*4U);
        source_word[i]=eng1_native_data_target_0052b70[i].value;
    }
    for(scenario=0;scenario<3U;++scenario) {
        for(i=0;i<POOL_BYTES+16U;++i)
            native_pool.bytes[i]=source_pool.bytes[i]=(unsigned char)(0xa5U+scenario*17U+i*scenario);
        for(i=0;i<8U;++i) {
            unsigned long x=scenario==0 ? old_x[i] : scenario==1 ? 0xdeadbeefUL^(i*0x10001UL) : 0UL;
            unsigned short word=scenario==0 ? old_word[i] : scenario==1 ? (unsigned short)(0x8001U+i) : 0U;
            *(unsigned long *)(void *)(base+0x52b50UL+i*4U)=g_eng1_call_12960_dword_52b50[i]=x;
            *(unsigned short *)(void *)(base+0x52b70UL+i*4U)=eng1_native_data_target_0052b70[i].value=word;
        }
        *(unsigned long *)(void *)(base+0x734e0UL)=(unsigned long)(void *)native_pool.bytes;
        g_alternate_actor_pool=source_pool.bytes;
        entry();eng1_call_12960();
        check(!memcmp(native_pool.bytes,source_pool.bytes,POOL_BYTES+16U),"full pool and canaries");
        for(i=0;i<0x128U;++i)
            check(source_pool.bytes[i]==(unsigned char)(0xa5U+scenario*17U+i*scenario),"leading two records unchanged");
        for(i=POOL_BYTES;i<POOL_BYTES+16U;++i)
            check(source_pool.bytes[i]==(unsigned char)(0xa5U+scenario*17U+i*scenario),"trailing canary");
        for(i=0;i<8U;++i) {
            unsigned char *actor=source_pool.bytes+0x128U+i*0x94U;
            check(*(unsigned long *)(void *)actor==g_eng1_call_12960_dword_52b50[i],"eight x values");
            check(*(unsigned short *)(void *)(actor+0x34U)==eng1_native_data_target_0052b70[i].value,"eight WORD values");
        }
    }
    *(unsigned long *)(void *)(base+0x734e0UL)=old_pool;
    for(i=0;i<8U;++i) {
        *(unsigned long *)(void *)(base+0x52b50UL+i*4U)=old_x[i];
        *(unsigned short *)(void *)(base+0x52b70UL+i*4U)=old_word[i];
        g_eng1_call_12960_dword_52b50[i]=source_x[i];
        eng1_native_data_target_0052b70[i].value=source_word[i];
    }
    FreeLibrary(dll);
    printf("alternate_bootstrap cases=3 checks=%u failures=%u\n",checks,failures);
    return failures ? 1 : 0;
}
