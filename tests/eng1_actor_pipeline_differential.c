/* Test-only original/source258e0 and four bounded25d00 paths. Original code is
 * mapped without initialization and never patched. Native26480 owner-null and
 * native2a880 type-zero paths execute naturally; source helpers are explicit
 * spies. No general simulation or gameplay equivalence claim. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/actor_pipeline.c"

EngineInterface g_engine_interface;
ENG1ActorPipelineStorage71440 g_actor_pipeline_storage_71440;
ActorResource g_actor_resources[81];
void *g_normal_actor_pool,*g_middle_actor_pool,*g_alternate_actor_pool;
ActorPipelineCallback12 *g_configuration_dword_71370;
ActorPipelineCallback8 *g_configuration_dword_7137c,*g_configuration_dword_7139c;
unsigned short g_configuration_word_71378,g_configuration_word_713a8;
unsigned short g_configuration_word_713aa,g_configuration_word_713a0;
unsigned short g_configuration_word_7138c,g_configuration_word_713b6;
unsigned short g_configuration_word_713c4,g_configuration_word_713b8;
unsigned short g_configuration_word_71430,g_configuration_word_713b4;
unsigned short g_configuration_word_713ba,g_configuration_word_71432;
unsigned short g_pipeline_word_713ae,g_pipeline_word_71376;
unsigned short g_pipeline_word_6fec0,g_pipeline_word_6fec4;
ActorPipelineView *g_pipeline_actor_7200c;
unsigned char *g_collision_context;
unsigned long g_frame_global_734f4;
long g_camera_center_x_70cdc,g_camera_x_70ca0;
Eng1ProfileRow28 eng1_profile_rows_5df08[12];
ResourceKindInfo g_kind_info[1];

static HMODULE oracle;
static int checking25d00,bounds_calls;
static unsigned char collision_actor[0x134];
static int checks,failures,cases,helper_calls,unexpected_calls;
static unsigned char source_pool[0x134+32],native_pool[0x134+32];
static unsigned char source_context[0x100+32],native_context[0x100+32];
static unsigned long helper_first,helper_second;
static unsigned short helper_clears[5];
static unsigned long helper_clear_dword;
static unsigned char *cell(unsigned long rva){return (unsigned char *)oracle+rva;}
static void check(int ok,const char *what){++checks;if(!ok){++failures;printf("FAIL case=%d %s\n",cases,what);}}

void __cdecl eng1_call_26480(void)
{
    ++helper_calls;
    helper_first=g_pipeline_dword_716a8;helper_second=g_pipeline_dword_71718;
    helper_clears[0]=g_pipeline_word_71ca0;helper_clears[1]=g_pipeline_word_71740;
    helper_clears[2]=g_pipeline_word_71720;helper_clears[3]=g_pipeline_word_71cc0;
    helper_clears[4]=g_pipeline_word_71c60;helper_clear_dword=g_pipeline_dword_71680;
}
int __cdecl remove_actor(ActorPipelineView *p){(void)p;++unexpected_calls;return 0;}
int __cdecl remove_actor_variant(ActorPipelineView *p){(void)p;++unexpected_calls;return 0;}
int __cdecl release_actor(ActorPipelineView *p){(void)p;++unexpected_calls;return 0;}
/* The historical linker retains sibling-function dependencies in this unit.
 * These explicit test spies must remain uncalled by the selected258e0 path. */
void __cdecl eng1_call_27fa0(void){++unexpected_calls;}
void __cdecl eng1_call_279f0(void){++unexpected_calls;}
void __cdecl initialize_pending_action_groups(void){++unexpected_calls;}
void __cdecl reset_command_table(void){++unexpected_calls;}
void __cdecl eng1_call_1fdf0(void){++unexpected_calls;}
void __cdecl eng1_call_274e0(void){++unexpected_calls;}
void __cdecl dispatch_local_1fd20(void){++unexpected_calls;}
void __cdecl eng1_call_26040(void){++unexpected_calls;}
void __cdecl eng1_call_12960(void){++unexpected_calls;}
void __cdecl eng1_call_01e50(void){++unexpected_calls;}
void __cdecl eng1_call_26730(void){++unexpected_calls;}
void __cdecl reset_alternate_actor_pool(void){++unexpected_calls;}
void __cdecl reset_middle_actor_pool(void){++unexpected_calls;}
void __cdecl reset_normal_actor_pool(void){++unexpected_calls;}
unsigned short __cdecl eng1_call_2a880(ActorPipelineView *p,void *q,unsigned long n)
{
 if(checking25d00){check((void *)p==collision_actor,"source collision actor reloaded");check(q==(bounds_calls?g_pipeline_actor_slots_71440:collision_actor+0x88),"correct query output");check(n==(unsigned long)bounds_calls+1,"query mode");++bounds_calls;}
 else ++unexpected_calls;return 0;
}
ActorCore *__cdecl eng1_call_27c40(Eng1ForestRecord20 *p,unsigned long n){(void)p;(void)n;++unexpected_calls;return 0;}
ActorMiddle *__cdecl eng1_call_27720(Eng1ForestRecord20 *p,unsigned long n){(void)p;(void)n;++unexpected_calls;return 0;}
ActorNormal *__cdecl eng1_call_270c0(Eng1ForestRecord20 *p,unsigned long n){(void)p;(void)n;++unexpected_calls;return 0;}

typedef struct SectionSnapshot {unsigned long rva,size,flags;unsigned char *initial,*before;} SectionSnapshot;
static SectionSnapshot sections[16];static int section_count;
static int begin_module(void)
{
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)oracle;
    IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS *)cell(dos->e_lfanew);
    IMAGE_SECTION_HEADER *table=IMAGE_FIRST_SECTION(nt);int i;
    section_count=nt->FileHeader.NumberOfSections;if(section_count>16)return 0;
    for(i=0;i<section_count;++i){
        SectionSnapshot *s=&sections[i];s->rva=table[i].VirtualAddress;
        s->size=table[i].Misc.VirtualSize;s->flags=table[i].Characteristics;
        s->initial=(unsigned char *)HeapAlloc(GetProcessHeap(),0,s->size);
        s->before=(unsigned char *)HeapAlloc(GetProcessHeap(),0,s->size);
        if(!s->initial||!s->before)return 0;
        memcpy(s->initial,cell(s->rva),s->size);
    }
    return 1;
}
static void restore_module(void)
{int i;for(i=0;i<section_count;++i)if(sections[i].flags&IMAGE_SCN_MEM_WRITE)memcpy(cell(sections[i].rva),sections[i].initial,sections[i].size);}
static void before_module(void)
{int i;for(i=0;i<section_count;++i)memcpy(sections[i].before,cell(sections[i].rva),sections[i].size);}
static int output_byte(unsigned long rva)
{
    static const unsigned long dwords[]={0x71840,0x716a8,0x71718,0x71680,0x714c0,0x71760,0x717c0,0x716c0,0x71580,0x71a20,0x71be0,0x71b20};
    static const unsigned long words[]={0x71bc4,0x71c5c,0x71ca0,0x71740,0x71720,0x71cc0,0x71c60};int i;
    if(checking25d00 && ((rva>=0x71460&&rva<0x71464)||(rva>=0x71ce0&&rva<0x71ce4)||(rva>=0x71900&&rva<0x71904)||(rva>=0x71ae0&&rva<0x71ae4)||(rva>=0x6fec0&&rva<0x6fec2)||(rva>=0x6fec4&&rva<0x6fec6)||(rva>=0x71376&&rva<0x71378)))return 1;
    if(rva>=0x719c0&&rva<0x719c0+81)return 1;
    for(i=0;i<sizeof(dwords)/sizeof(dwords[0]);++i)if(rva>=dwords[i]&&rva<dwords[i]+4)return 1;
    for(i=0;i<sizeof(words)/sizeof(words[0]);++i)if(rva>=words[i]&&rva<words[i]+2)return 1;
    return 0;
}
static void check_module(void)
{
    int i,ok=1;unsigned long j;
    for(i=0;i<section_count&&ok;++i)for(j=0;j<sections[i].size;++j){
        unsigned long r=sections[i].rva+j;
        if(sections[i].before[j]!=*cell(r)&&!output_byte(r)){printf("UNEXPECTED_NATIVE_WRITE rva=%lx\n",r);ok=0;break;}
    }
    check(ok,"all original code/table/input/neighbour bytes preserved outside explicit outputs");
}
static void bind_case(unsigned long tick,long z)
{
    int i;unsigned char *bytes=(unsigned char *)&g_actor_pipeline_storage_71440;
    ResourceCallbackContext *scontext=(ResourceCallbackContext *)(source_context+16);
    ResourceCallbackContext *ncontext=(ResourceCallbackContext *)(native_context+16);
    restore_module();memset(&g_engine_interface,0,sizeof g_engine_interface);
    memset(source_pool,0x5a,sizeof source_pool);memcpy(native_pool,source_pool,sizeof source_pool);
    *(long *)(source_pool+16+8)=z;*(long *)(native_pool+16+8)=z;
    memset(source_context,0x3c,sizeof source_context);memcpy(native_context,source_context,sizeof source_context);
    scontext->tick_038=tick;ncontext->tick_038=tick;
    g_engine_interface.context_004=scontext;*(void **)cell(0x70f24)=ncontext;
    g_normal_actor_pool=source_pool+16;*(void **)cell(0x72860)=native_pool+16;
    g_middle_actor_pool=g_alternate_actor_pool=0;*(unsigned long *)cell(0x73494)=0;*(unsigned long *)cell(0x734e0)=0;
    g_configuration_word_71378=g_configuration_word_713a8=g_configuration_word_713aa=g_configuration_word_713a0=g_configuration_word_7138c=g_configuration_word_713b6=0;
    *(unsigned short *)cell(0x71378)=0;*(unsigned short *)cell(0x713a8)=0;
    *(unsigned short *)cell(0x713aa)=0;*(unsigned short *)cell(0x713a0)=0;
    *(unsigned short *)cell(0x7138c)=0;*(unsigned short *)cell(0x713b6)=0;
    for(i=0;i<sizeof g_actor_pipeline_storage_71440;++i)bytes[i]=(unsigned char)(i*29+tick*7+0xa5);
    memcpy(cell(0x71440),bytes,sizeof g_actor_pipeline_storage_71440);
    for(i=0;i<81;++i){
        memset(&g_actor_resources[i],(unsigned char)(i*13+tick),sizeof g_actor_resources[i]);
        g_actor_resources[i].residency_state=(unsigned long)(i+tick)&7;
    }
    /* Native snapshot starts at record+16=5cef0; full owner starts5cee0. */
    memcpy(cell(0x5cee0),g_actor_resources,sizeof g_actor_resources);
    /* Natural26480 returns at its genuine owner-null branch; no imports,
     * callbacks or code patches are needed. Full module observation checks it. */
    *cell(0x70cfe)=0;*(unsigned long *)cell(0x6fee8)=0;*(unsigned long *)cell(0x6feec)=0;
    helper_calls=unexpected_calls=0;helper_first=helper_second=0;
    memset(helper_clears,0xcc,sizeof helper_clears);helper_clear_dword=0xccccccccUL;
    before_module();
}
static void run_case(unsigned long tick,long z)
{
    int i;void (__cdecl *native_command)(void)=(void (__cdecl *)(void))cell(0x258e0);
    bind_case(tick,z);native_command();check_module();dispatch_local_258e0();
    check(!memcmp(&g_actor_pipeline_storage_71440,cell(0x71440),sizeof g_actor_pipeline_storage_71440),"full0x8c0-byte source aggregate matches native including clears/snapshot/preserved neighbours");
    check(!memcmp(source_pool,native_pool,sizeof source_pool),"full guarded normal actor buffer preserved");
    check(!memcmp(source_context,native_context,sizeof source_context),"full guarded context preserved");
    check(!memcmp(g_actor_resources,cell(0x5cee0),sizeof g_actor_resources),"all81 complete resource records preserved");
    check(helper_calls==1&&unexpected_calls==0,"source helper call once and actor callbacks skipped with empty pool bounds");
    check(helper_first==*(unsigned long *)cell(0x716a8)&&helper_second==*(unsigned long *)cell(0x71718),"source helper observes actual native sine output values");
    check(helper_clear_dword==0,"DWORD reset visible before source helper");
    for(i=0;i<5;++i)check(helper_clears[i]==0,"WORD reset visible before source helper");
    ++cases;
}
static void run_reset(unsigned long count)
{
    unsigned long a,b;int prior=checks;
    bind_case(0,0);checking25d00=1;bounds_calls=0;
    memset(collision_actor,0,sizeof collision_actor);
    memset(source_pool+16,0,0x134);memcpy(native_pool,source_pool,sizeof source_pool);
    source_context[16]=native_context[16]=9;
    g_collision_context=collision_actor;*(void **)cell(0x7217c)=collision_actor;
    g_pipeline_actor_7200c=(ActorPipelineView *)(source_pool+16);*(void **)cell(0x7200c)=native_pool+16;
    g_actor_pipeline_storage_71440.dword_cells[0x6a0/4]=count;
    memcpy(cell(0x71440),&g_actor_pipeline_storage_71440,sizeof g_actor_pipeline_storage_71440);
    g_configuration_word_713b4=g_configuration_word_71432=g_configuration_word_713ba=0;
    *(unsigned short *)cell(0x713b4)=*(unsigned short *)cell(0x71432)=*(unsigned short *)cell(0x713ba)=0;
    g_pipeline_word_71376=17;*(unsigned short *)cell(0x71376)=17;
    g_pipeline_word_6fec0=g_pipeline_word_6fec4=0xffff;
    *(unsigned short *)cell(0x6fec0)=*(unsigned short *)cell(0x6fec4)=0xffff;
    before_module();
    __asm mov a,esp
    ((void (__cdecl *)(void))cell(0x25d00))();
    __asm mov b,esp
    check(a==b,"native25d00 ESP");check_module();
    __asm mov a,esp
    dispatch_local_25d00();
    __asm mov b,esp
    check(a==b,"source25d00 ESP");
    check(*(unsigned long *)cell(0x71ae0)==0,"native consumed color list reset");
    check(!memcmp(&g_actor_pipeline_storage_71440,cell(0x71440),sizeof g_actor_pipeline_storage_71440),"complete pipeline including color list reset");
    check(bounds_calls==2&&unexpected_calls==0,"only two bounds spies, no callbacks or26730 at level9");
    check(g_pipeline_word_6fec0==0&&g_pipeline_word_6fec4==0&&g_pipeline_word_71376==16,"independent scalar outputs");
    check(!memcmp(source_pool,native_pool,sizeof source_pool)&&!memcmp(source_context,native_context,sizeof source_context),"guarded actor/context buffers");
    printf("reset_case=%lu checks=%d failures=%d\n",count,checks-prior,failures);
    checking25d00=0;
}
int main(int argc,char **argv)
{
    static const long zs[]={0,0x12345678L,(long)0x87654321UL,-1L};int tick,z,i;
    if(argc!=2)return 2;
    oracle=LoadLibraryExA(argv[1],0,DONT_RESOLVE_DLL_REFERENCES);
    if(!oracle){printf("LOAD_ERROR %lu\n",GetLastError());return 2;}
    if(!begin_module())return 2;
    check(!memcmp(cell(0x5a848),g_actor_stage_table_5a848,2048),"full source sine table equals naturally relocated native table");
    for(tick=0;tick<128;++tick)for(z=0;z<4;++z)run_case((unsigned long)tick,zs[z]);
    {int prior=checks;run_reset(0);run_reset(1);run_reset(17);run_reset(0xffffffffUL);
     printf("color_reset_native_cases=4 checks=%d failures=%d\n",checks-prior,failures);}
    restore_module();
    for(i=0;i<section_count;++i){check(!memcmp(cell(sections[i].rva),sections[i].initial,sections[i].size),"native section restored before unload");HeapFree(GetProcessHeap(),0,sections[i].initial);HeapFree(GetProcessHeap(),0,sections[i].before);}
    FreeLibrary(oracle);printf("sine_native_cases=%d checks=%d failures=%d\n",cases,checks,failures);
    return failures?1:0;
}
