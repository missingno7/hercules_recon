/* Test-only semantic spies and unmodified PC26040 oracle execution. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "../src/eng1/behavioral/forest_actor_init.c"

EngineInterface g_engine_interface;
ResourceKindInfo g_kind_info[1];
unsigned char *g_root_group_rows_70d5c[4];
signed char g_root_count_70ed0,g_active_category_70cfe;
unsigned long g_configuration_dword_71398,g_frame_global_734f4;
unsigned short g_pipeline_word_713ae;
long g_camera_x_70ca0,g_camera_center_x_70cdc;
void *g_pipeline_actor_7200c;

static int checks,failures,cases,alloc_calls,alloc_fail,alloc_mutate;
static unsigned long alloc_bytes,alloc_flags;
static unsigned char source_alloc[16384],native_alloc[16384];
static unsigned char source_context[256],native_context[256];
static unsigned char source_block[4096],native_block[4096];
static Eng1SpatialType10 test_types[4];
static HMODULE oracle;
static int native_mode,callback_count,callback_id[32],callback_index[32],callback_mutate;
static unsigned long callback_argument[32],incomplete_rva;
static jmp_buf incomplete_jump;
static unsigned char *cell(unsigned long r){return (unsigned char *)oracle+r;}
static void check(int ok,const char *what){++checks;if(!ok){++failures;printf("FAIL case=%d %s\n",cases,what);}}
void __cdecl eng1_incomplete_behavior(unsigned long r,const char *why)
{(void)why;incomplete_rva=r;longjmp(incomplete_jump,1);}
static void *__cdecl allocation(void **slot,unsigned long bytes,unsigned long flags)
{
    unsigned char *buffer=native_mode?native_alloc:source_alloc;
    ++alloc_calls;alloc_bytes=bytes;alloc_flags=flags;
    if(alloc_fail)return 0;
    *slot=buffer+16;
    if(alloc_mutate) {
        if(alloc_mutate==1) {
            if(native_mode)*(unsigned long *)cell(0x6feec)=1;
            else g_kind_info[0].count=1;
        } else if(alloc_mutate==2) {
            if(native_mode)*(unsigned short *)cell(0x713ae)=0;
            else g_pipeline_word_713ae=0;
        } else if(alloc_mutate==3)g_pipeline_word_713ae=2;
        else if(alloc_mutate==4) {
            if(native_mode)*(unsigned long *)cell(0x71398)=0;
            else g_configuration_dword_71398=0;
        } else if(alloc_mutate==5) {
            if(native_mode)*(void **)cell(0x70d5c)=native_block+96;
            else g_root_group_rows_70d5c[0]=source_block+96;
        }
    }
    return buffer+16;
}
static void factory(int id,void *p,unsigned long group)
{
    int n=callback_count++;
    callback_id[n]=id;callback_index[n]=(int)((Eng1ForestRecord20 *)p-(Eng1ForestRecord20 *)((unsigned char *)g_kind_info[0].owner+20));
    callback_argument[n]=group;
    if(callback_mutate==1)g_kind_info[0].count=1;
    if(callback_mutate==2){g_kind_info[0].count=3;g_kind_info[0].owner=0;}
}
void __cdecl eng1_call_270c0(void *p,unsigned long n){factory(0,p,n);}
void __cdecl eng1_call_27720(void *p,unsigned long n){factory(4,p,n);}
void __cdecl eng1_call_27c40(void *p,unsigned long n){factory(2,p,n);}
void __cdecl eng1_native_code_target_028290(unsigned long n)
{int i=callback_count++;callback_id[i]=1;callback_index[i]=-1;callback_argument[i]=n;}

typedef struct Snapshot {unsigned long rva,size;unsigned char *before;} Snapshot;
static Snapshot snapshot[16];static int snapshot_count;
static void before_native(void)
{
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)oracle;
    IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS *)cell(dos->e_lfanew);
    IMAGE_SECTION_HEADER *s=IMAGE_FIRST_SECTION(nt);int i;
    snapshot_count=nt->FileHeader.NumberOfSections;
    for(i=0;i<snapshot_count;++i){snapshot[i].rva=s[i].VirtualAddress;snapshot[i].size=s[i].Misc.VirtualSize;
        if(!snapshot[i].before)snapshot[i].before=(unsigned char *)HeapAlloc(GetProcessHeap(),0,snapshot[i].size);
        memcpy(snapshot[i].before,cell(snapshot[i].rva),snapshot[i].size);}
}
static void check_native_writes(void)
{
    int i,ok=1;unsigned long j,r;
    for(i=0;i<snapshot_count&&ok;++i)for(j=0;j<snapshot[i].size;++j){r=snapshot[i].rva+j;
        if(snapshot[i].before[j]!=*cell(r)&&!(r>=0x6fee8&&r<0x6fef0)&&!(r>=0x713ae&&r<0x713b0)&&
           !(alloc_mutate==4&&r>=0x71398&&r<0x7139c)&&!(alloc_mutate==5&&r>=0x70d5c&&r<0x70d60)){
            printf("unexpected_native_write=%lx\n",r);ok=0;break;}}
    check(ok,"all mapped native sections preserved outside actual kind-info and cached-group outputs");
}
static void reset(void)
{
    memset(source_alloc,0xa5,sizeof source_alloc);memcpy(native_alloc,source_alloc,sizeof source_alloc);
    memset(source_context,0x3c,sizeof source_context);memcpy(native_context,source_context,sizeof source_context);
    memset(source_block,0,sizeof source_block);memset(test_types,0,sizeof test_types);
    memset(&g_engine_interface,0,sizeof g_engine_interface);
    g_engine_interface.context_004=(ResourceCallbackContext *)(source_context+16);
    g_engine_interface.allocate_078=allocation;
    g_kind_info[0].owner=0;g_kind_info[0].count=0;
    g_root_group_rows_70d5c[0]=source_block+64;g_root_count_70ed0=1;
    g_configuration_dword_71398=(unsigned long)test_types;g_pipeline_word_713ae=0;
    g_active_category_70cfe=0;g_camera_x_70ca0=0;g_camera_center_x_70cdc=0;
    g_frame_global_734f4=1;g_pipeline_actor_7200c=0;
    alloc_calls=0;alloc_bytes=alloc_flags=0;alloc_fail=alloc_mutate=0;native_mode=0;
    callback_count=callback_mutate=0;incomplete_rva=0;
}
static void synthetic(void)
{
    Eng1ForestPlacement8 *p=(Eng1ForestPlacement8 *)(source_block+16);int i;
    *(unsigned short *)(source_block+64+8)=3;*(short *)(source_block+64+10)=-58;
    for(i=0;i<3;++i){p[i].x=(unsigned short)(65535-i);p[i].y=(unsigned short)(2+i);p[i].z=32768;p[i].spatial_index=(unsigned short)i;}
    test_types[0].x=-2;test_types[0].y=-4;test_types[0].z=-32768;test_types[0].flags=0x2200;test_types[0].kind=0x2018;
    test_types[1].x=3;test_types[1].y=5;test_types[1].z=32767;test_types[1].flags=0x0080;test_types[1].kind=0x2019;
    test_types[2].x=1;test_types[2].y=1;test_types[2].z=1;test_types[2].flags=0x0200;test_types[2].kind=0x2018;
}
static void compare_native26040(int warm)
{
    int source_calls;unsigned long source_bytes,source_flags;
    void (__cdecl *native26040)(void)=(void (__cdecl *)(void))cell(0x26040);
    memcpy(native_block,source_block,sizeof source_block);
    memcpy(native_alloc,source_alloc,sizeof source_alloc);memcpy(native_context,source_context,sizeof source_context);
    *(void **)cell(0x70f24)=native_context+16;*(void **)cell(0x70f98)=allocation;
    *(void **)cell(0x71398)=g_configuration_dword_71398==(unsigned long)g_eng1_spatial_types_04ede8?cell(0x4ede8):(void *)g_configuration_dword_71398;
    *(unsigned short *)cell(0x713ae)=g_pipeline_word_713ae;
    *(signed char *)cell(0x70ed0)=g_root_count_70ed0;
    *(void **)cell(0x70d5c)=native_block+(g_root_group_rows_70d5c[0]-source_block);
    *(void **)cell(0x6fee8)=warm&&g_kind_info[0].owner?native_alloc+16:0;
    *(unsigned long *)cell(0x6feec)=g_kind_info[0].count;
    before_native();eng1_call_26040();source_calls=alloc_calls;source_bytes=alloc_bytes;source_flags=alloc_flags;
    alloc_calls=0;native_mode=1;native26040();native_mode=0;
    check(alloc_calls==source_calls&&alloc_bytes==source_bytes&&alloc_flags==source_flags,"native allocator event bytes flags and call count");
    check(!memcmp(source_alloc,native_alloc,sizeof source_alloc),"full guarded worklist allocation including header tail and unused bytes");
    check(!memcmp(source_context,native_context,sizeof source_context),"full guarded callback context");
    check(!memcmp(source_block,native_block,sizeof source_block),"full input MAP block preservation");
    check(g_pipeline_word_713ae==*(unsigned short *)cell(0x713ae),"cached group count");
    check(g_kind_info[0].count==*(unsigned long *)cell(0x6feec),"live kind-info count");
    check((g_kind_info[0].owner==0)==(*(void **)cell(0x6fee8)==0),"owner null status; successful pointer identity normalized");
    check((!g_kind_info[0].owner||g_kind_info[0].owner==source_alloc+16)&&(!*(void **)cell(0x6fee8)||*(void **)cell(0x6fee8)==native_alloc+16),"successful owner pointers name corresponding allocation headers");
    check_native_writes();++cases;
}
static void init_tests(const char *forest)
{
    Eng1ForestRecord20 *r;unsigned char saved[80];FILE *f;size_t n;
    reset();synthetic();compare_native26040(0);
    r=(Eng1ForestRecord20 *)(source_alloc+36);
    check(alloc_bytes==80&&alloc_calls==1&&alloc_flags==0,"three placements allocate four20-byte records");
    check((unsigned long)r[0].x==0xfffd0000UL&&(unsigned long)r[0].y==0xfffe0000UL&&r[0].z==0,"zero-extended placement plus signed offsets with fixedpoint wrap");
    check(r[0].untouched_012==0xa5a5&&r[0].state==0,"reserved final WORD preserved and state cleared");
    check(*(unsigned short *)(source_context+16+0x16)==1&&*(unsigned short *)(source_context+16+0x1a)==2,"2019 and2018 context counters at distinct offsets");
    memcpy(saved,source_alloc+16,80);r[0].state=0x4567;r[1].state=0x8910;r[2].state=0x2345;
    test_types[0].x=100;test_types[1].x=100;test_types[2].x=-100;alloc_calls=0;compare_native26040(1);
    check(r[0].state==0&&r[2].state==0&&r[1].state==0x8910,"warm refresh only metadata flag0200 records");
    check(!memcmp(source_alloc+16,saved,20)&&r[1].x==((Eng1ForestRecord20 *)(saved+20))[1].x,"warm header and inactive record preservation");
    reset();synthetic();alloc_fail=1;compare_native26040(0);check(g_kind_info[0].count==3&&g_kind_info[0].owner==0,"failed allocation retains count and zero owner");
    reset();synthetic();alloc_mutate=1;compare_native26040(0);check(g_kind_info[0].count==1&&((Eng1ForestRecord20 *)(source_alloc+36))[1].state==0xa5a5,"allocator callback shrinks live count before initialization");
    reset();synthetic();alloc_mutate=2;compare_native26040(0);check(g_pipeline_word_713ae==0,"cached group count remains host-mutated and is reloaded");
    reset();synthetic();alloc_mutate=4;compare_native26040(0);check(g_configuration_dword_71398==0&&((Eng1ForestRecord20 *)(source_alloc+36))[2].kind==0x2018,"selected metadata pointer captured before allocation callback");
    reset();synthetic();alloc_mutate=5;*(short *)(source_block+96+10)=-90;compare_native26040(0);
    check(g_root_group_rows_70d5c[0]==source_block+96&&((Eng1ForestRecord20 *)(source_alloc+36))[2].kind==0x2018,"placement block pointer reloaded after allocation callback");
    reset();compare_native26040(0);check(alloc_calls==0,"empty MAP group makes no allocation");
    reset();g_configuration_dword_71398=0;compare_native26040(0);check(g_pipeline_word_713ae==0,"null spatial selector does not cache groups or clear context");
    reset();synthetic();g_pipeline_word_713ae=1;g_kind_info[0].count=3;compare_native26040(1);check(alloc_calls==0,"warm null owner skips allocation and records");
    reset();f=fopen(forest,"rb");if(!f){check(0,"forest block opens");return;}
    n=fread(source_block,1,sizeof source_block,f);fclose(f);
    check(n==2286,"real forest block bytes complete");
    g_root_group_rows_70d5c[0]=source_block+2266;
    g_configuration_dword_71398=(unsigned long)g_eng1_spatial_types_04ede8;
    compare_native26040(0);check(g_kind_info[0].count==283&&alloc_bytes==5680,"actual FOREST283 placements initialize live worklist");
    alloc_calls=0;compare_native26040(1);
    reset();synthetic();g_root_count_70ed0=2;
    if(setjmp(incomplete_jump)==0){eng1_call_26040();check(0,"unproved group count trapped");}
    else check(incomplete_rva==0x6fee8&&alloc_calls==0,"unproved group count fails before table access");++cases;
    reset();synthetic();alloc_mutate=3;
    if(setjmp(incomplete_jump)==0){eng1_call_26040();check(0,"host-expanded unsupported group count trapped");}
    else check(incomplete_rva==0x6fee8&&alloc_calls==1,"live host-expanded group count fails before second table slot");++cases;
}
typedef struct WindowCase {short x;unsigned short flags,kind;int expected;} WindowCase;
static const WindowCase windows[]={
    {-501,0x40,0, -1},{-500,0x40,0,0},{-32,0x40,0,0},{-31,0x40,0,-1},
    {351,0x40,0,-1},{352,0x40,0x4001,4},{820,0x40,0x2007,2},{821,0x40,0,-1},
    {-257,0,0,-1},{-256,0,0,0},{-32,0,0,0},{-31,0,0,-1},
    {351,0,0,-1},{352,0,0x4001,4},{576,0,0x2007,2},{577,0,0,-1},
    {119,0,0x1234,-1},{120,0,0x1234,1},{180,0,0x1234,1},{181,0,0x1234,-1},
    {150,0,0x2019,-1},{-100,0x8000,0,-1},{-100,0x4000,0,-1},
    {-100,0,0x6000,-1},{-100,0x0400,0,0}};
static void walk_tests(void)
{
    unsigned int i;Eng1ForestRecord20 *r;unsigned char before[16384];
    for(i=0;i<sizeof windows/sizeof windows[0];++i){
        reset();g_kind_info[0].owner=source_alloc+16;g_kind_info[0].count=1;
        r=(Eng1ForestRecord20 *)(source_alloc+36);memset(r,0,20);
        r->x=(long)((unsigned long)(long)windows[i].x<<16);r->flags=windows[i].flags;r->kind=windows[i].kind;
        memcpy(before,source_alloc,sizeof before);eng1_call_26480();
        check(callback_count==(windows[i].expected>=0?1:0),"explicit native boundary dispatch count");
        if(callback_count){check(callback_id[0]==windows[i].expected,"expected normal middle alternate or action dispatch");
            check(callback_argument[0]==(windows[i].expected==1?windows[i].kind:0),"factory zero group or complete action kind");}
        check(!memcmp(before,source_alloc,sizeof before),"walker preserves all worklist bytes with observational spies");++cases;
    }
    reset();g_kind_info[0].owner=source_alloc+16;g_kind_info[0].count=3;r=(Eng1ForestRecord20 *)(source_alloc+36);
    for(i=0;i<3;++i){memset(r+i,0,20);r[i].x=-100L*65536;r[i].kind=(unsigned short)(i==0?0:i==1?0x4000:0x2000);}
    callback_mutate=1;eng1_call_26480();check(callback_count==1,"factory reduces count and cancels later records");++cases;
    g_kind_info[0].owner=source_alloc+16;g_kind_info[0].count=1;callback_count=0;callback_mutate=2;
    eng1_call_26480();check(callback_count==3&&callback_id[0]==0&&callback_id[1]==4&&callback_id[2]==2,"factory grows count; cached owner survives owner slot clear");++cases;
    reset();g_kind_info[0].owner=source_alloc+16;g_kind_info[0].count=1;r=(Eng1ForestRecord20 *)(source_alloc+36);
    memset(r,0,20);r->x=-100L*65536;r->flags=0x400;g_frame_global_734f4=0;
    eng1_call_26480();check(callback_count==0,"frame-current actor equality suppresses0400 flagged record");++cases;
    r->flags=0;r->x=30L*65536;g_camera_x_70ca0=130;g_camera_center_x_70cdc=0;g_frame_global_734f4=1;
    eng1_call_26480();check(callback_count==1,"camera delta moves interior record into spawn window");++cases;
    reset();g_active_category_70cfe=-1;
    if(setjmp(incomplete_jump)==0){eng1_call_26480();check(0,"unsupported signed category traps");}
    else check(incomplete_rva==0x6fee8&&callback_count==0,"negative category fails before unproved owner read");++cases;
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;oracle=LoadLibraryExA(argv[1],0,DONT_RESOLVE_DLL_REFERENCES);
    if(!oracle){printf("load_error=%lu\n",GetLastError());return 3;}
    init_tests(argv[2]);walk_tests();
    printf("forest_cases=%d checks=%d failures=%d\n",cases,checks,failures);
    return failures?1:0;
}
