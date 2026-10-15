/* Test-only composition: the native caller reaches its native bounds helper;
 * the source caller reaches the actual source helper and resource providers. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../calibration/engine_interface.h"
#include "../src/eng1/behavioral/resource_runtime_data.h"
#include "../src/eng1/behavioral/actor_pipeline_storage.h"

extern void __cdecl dispatch_local_2fb70(void);
extern long g_camera_x_70ca0, g_camera_y_70ca4, g_camera_z_70ca8;
extern long g_camera_grid_x_70cac, g_camera_grid_y_70cb0, g_camera_grid_z_70cb4;
extern long g_camera_prior_x_70cc4, g_camera_prior_y_70cc8;
extern long g_camera_center_x_70cdc, g_camera_center_y_70ce0, g_camera_focal_70ce4;
extern long g_camera_z_offset_70d18;
extern long g_camera_angle_x_70cd0,g_camera_angle_y_70cd4,g_camera_offset_y_70cbc;
extern short g_camera_adjust_y_70d04;
extern void __cdecl eng1_call_26730(void);
extern unsigned char g_camera_iterations_70cfd, g_camera_bounds_id_70cff;
extern unsigned short g_camera_flags_70d10;
EngineInterface g_engine_interface;
unsigned long g_frame_global_734f4, g_frame_global_70ce8, g_frame_global_70cec;
static HMODULE oracle;
static unsigned long cases, checks, failures;
static unsigned char actor[0x100], actor_before[0x100];
static unsigned long packed[64], packed_before[64];
static unsigned char *initial[16], *before[16];
static unsigned long section_rva[16], section_size[16], section_flags[16];
static unsigned sections;
static unsigned char context[0x100];
static unsigned char pipeline_before[0x8c0];
static int smoothing;
typedef struct Field { unsigned long rva; void *source; unsigned size; } Field;
#define F(r,n) {0x##r, &n, sizeof(n)}
static Field outputs[] = {
    F(70ca0,g_camera_x_70ca0), F(70ca4,g_camera_y_70ca4), F(70ca8,g_camera_z_70ca8),
    F(70cac,g_camera_grid_x_70cac), F(70cb0,g_camera_grid_y_70cb0),
    F(70cb4,g_camera_grid_z_70cb4), F(70cc4,g_camera_prior_x_70cc4),
    F(70cc8,g_camera_prior_y_70cc8),F(70cd0,g_camera_angle_x_70cd0),
    F(70cd4,g_camera_angle_y_70cd4),F(70cbc,g_camera_offset_y_70cbc),
    F(70d18,g_camera_z_offset_70d18),F(70d04,g_camera_adjust_y_70d04)
};
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; printf("FAIL case=%lu line=%d\n",cases,__LINE__); } } while(0)
static unsigned char *cell(unsigned long rva) { return (unsigned char *)oracle+rva; }
/* These sibling 2f840 callbacks must never execute in this selected path. */
void __cdecl dispatch_engine_2c9d0(unsigned long x,unsigned long y)
{ (void)x;(void)y;CHECK(0); }
void __cdecl dispatch_engine_2c9f0(unsigned long x) { (void)x;CHECK(0); }
static void seed(unsigned long rva,const void *value,unsigned size)
{ memcpy(cell(rva),value,size); }
static void setup(unsigned flags,int scenario,long position,unsigned iterations)
{
    unsigned i,j; unsigned short *row; unsigned char *base=(unsigned char *)packed;
    for(i=0;i<sections;++i) if(section_flags[i]&IMAGE_SCN_MEM_WRITE)
        memcpy(cell(section_rva[i]),initial[i],section_size[i]);
    for(i=0;i<sizeof(outputs)/sizeof(outputs[0]);++i)
        memcpy(outputs[i].source,cell(outputs[i].rva),outputs[i].size);
    memset(actor,0,sizeof actor);memset(packed,0xa5,sizeof packed);
    *(long *)actor=(scenario&1)?-32768L*65536L:1000L*65536L;
    *(long *)(actor+4)=(scenario&2)?-32768L*65536L:500L*65536L;
    *(long *)(actor+8)=300L*65536L;actor[0x94]=1;
    packed[3]=52;packed[4]=48;
    *(unsigned short *)(base+64)=scenario==4?1:3;
    for(i=0;i<3;++i){
        row=(unsigned short *)(base+68+i*16);row[0]=(unsigned short)(i*100);
        for(j=1;j<8;++j)row[j]=(unsigned short)(100+i*20+j*5);
    }
    if(scenario==4)*(unsigned short *)(base+68)=23;
    g_header_pointer_70d4c=base+16;g_root_count_70ed3=1;
    memcpy(cell(0x70d4c),&g_header_pointer_70d4c,4);
    memcpy(cell(0x70ed3),&g_root_count_70ed3,1);
    g_frame_global_734f4=(unsigned long)actor;
    g_frame_global_70ce8=0;g_frame_global_70cec=600;
    g_camera_x_70ca0=position;g_camera_y_70ca4=150;g_camera_z_70ca8=-64;
    g_camera_prior_x_70cc4=-123;g_camera_prior_y_70cc8=-456;
    g_camera_flags_70d10=(unsigned short)flags;
    g_camera_iterations_70cfd=(unsigned char)iterations;
    g_camera_bounds_id_70cff=scenario==3?1:scenario==2?255:0;
    g_element_width_70d14=4;g_camera_center_x_70cdc=160;
    g_camera_center_y_70ce0=180;g_camera_focal_70ce4=100;g_camera_z_offset_70d18=10;
#define SEED(r,n) seed(0x##r,&n,sizeof(n))
    SEED(734f4,g_frame_global_734f4);SEED(70ce8,g_frame_global_70ce8);SEED(70cec,g_frame_global_70cec);
    SEED(70d10,g_camera_flags_70d10);SEED(70cfd,g_camera_iterations_70cfd);SEED(70cff,g_camera_bounds_id_70cff);
    SEED(70d14,g_element_width_70d14);SEED(70cdc,g_camera_center_x_70cdc);
    SEED(70ce0,g_camera_center_y_70ce0);SEED(70ce4,g_camera_focal_70ce4);SEED(70d18,g_camera_z_offset_70d18);
    for(i=0;i<sizeof(outputs)/sizeof(outputs[0]);++i)
        memcpy(cell(outputs[i].rva),outputs[i].source,outputs[i].size);
    memcpy(actor_before,actor,sizeof actor);memcpy(packed_before,packed,sizeof packed);
    for(i=0;i<sections;++i)memcpy(before[i],cell(section_rva[i]),section_size[i]);
}
static unsigned long invoke(void (__cdecl *fn)(void))
{
    unsigned long fault=0,a,b;
    __asm mov a,esp
    __try { fn(); } __except(fault=GetExceptionCode(),EXCEPTION_EXECUTE_HANDLER) {}
    __asm mov b,esp
    CHECK(a==b);return fault;
}
static void __cdecl native_sequence(void)
{
    ((void (__cdecl *)(void))cell(0x26730))();
    ((void (__cdecl *)(void))cell(0x2fb70))();
}
static void __cdecl source_sequence(void)
{ eng1_call_26730();dispatch_local_2fb70(); }
static void compare(int fault_expected)
{
    unsigned i,j,k;int allowed,clean=1;unsigned long a,b;
    ++cases;a=invoke(smoothing==2?native_sequence:(void (__cdecl *)(void))cell(smoothing?0x26730:0x2fb70));
    b=invoke(smoothing==2?source_sequence:smoothing?eng1_call_26730:dispatch_local_2fb70);
    CHECK(a==b);CHECK((a!=0)==fault_expected);
    for(i=0;i<sizeof(outputs)/sizeof(outputs[0]);++i) {
        if(memcmp(cell(outputs[i].rva),outputs[i].source,outputs[i].size)!=0)
            printf("DIFF rva=%lx native=%ld source=%ld\n",outputs[i].rva,
                outputs[i].size==2?*(short *)cell(outputs[i].rva):*(long *)cell(outputs[i].rva),
                outputs[i].size==2?*(short *)outputs[i].source:*(long *)outputs[i].source);
        CHECK(memcmp(cell(outputs[i].rva),outputs[i].source,outputs[i].size)==0);
    }
    CHECK(memcmp(actor,actor_before,sizeof actor)==0);
    CHECK(memcmp(packed,packed_before,sizeof packed)==0);
    if(smoothing)CHECK(memcmp(&g_actor_pipeline_storage_71440,pipeline_before,sizeof pipeline_before)==0);
    for(i=0;i<sections&&clean;++i)for(j=0;j<section_size[i];++j)
        if(before[i][j]!=cell(section_rva[i])[j]){
            allowed=0;for(k=0;k<sizeof(outputs)/sizeof(outputs[0]);++k)
                if(section_rva[i]+j>=outputs[k].rva && section_rva[i]+j<outputs[k].rva+outputs[k].size)allowed=1;
            if(!allowed){printf("UNEXPECTED native write rva=%lx\n",section_rva[i]+j);clean=0;break;}
        }
    CHECK(clean);
}
static void smoothing_setup(unsigned level,long value,int count)
{
    unsigned i;unsigned short *w=g_actor_pipeline_storage_71440.word_cells;
    unsigned long *d=g_actor_pipeline_storage_71440.dword_cells;
    setup(0,0,0,0);memset(context,0,sizeof context);context[0]=(unsigned char)level;
    g_engine_interface.context_004=(ResourceCallbackContext *)context;
    memcpy(cell(0x70f24),&g_engine_interface.context_004,4);
    memset(&g_actor_pipeline_storage_71440,0xa5,sizeof g_actor_pipeline_storage_71440);
    w[0x860/2]=w[0x300/2]=w[0x2e0/2]=(unsigned short)count;
    d[0x240/4]=(unsigned long)(long)count;
    w[0x408/2]=30000;w[0x5d4/2]=65530;w[0x5d2/2]=30001;
    w[0x3fc/2]=0;d[0x374/4]=0xfffffff0UL;
    if(count==0){w[0x408/2]=w[0x5d4/2]=w[0x5d2/2]=0;d[0x374/4]=0;}
    if(count>0 && count<=15)for(i=1;i<=(unsigned)count;++i){
        w[0x860/2+i]=(unsigned short)(60000+i*321);
        w[0x300/2+i]=(unsigned short)(61000+i*333);
        w[0x2e0/2+i]=(unsigned short)(62000+i*111);
        d[0x240/4+i]=0x80000000UL+i*12345;
    }
    g_camera_z_offset_70d18=g_camera_angle_x_70cd0=g_camera_angle_y_70cd4=value;
    g_camera_offset_y_70cbc=value;g_camera_adjust_y_70d04=(short)value;
    for(i=0;i<sizeof(outputs)/sizeof(outputs[0]);++i)memcpy(cell(outputs[i].rva),outputs[i].source,outputs[i].size);
    memcpy(cell(0x71440),&g_actor_pipeline_storage_71440,sizeof g_actor_pipeline_storage_71440);
    memcpy(pipeline_before,&g_actor_pipeline_storage_71440,sizeof pipeline_before);
    for(i=0;i<sections;++i)memcpy(before[i],cell(section_rva[i]),section_size[i]);
}
static void guards(void)
{
    unsigned i,j;unsigned long fault;unsigned char saved[13][4];
    unsigned long previous=checks;
    for(j=0;j<5;++j){
        smoothing_setup(j==0?6:1,0,0);
        if(j==1)g_actor_pipeline_storage_71440.word_cells[0x860/2]=48;
        if(j==2)g_actor_pipeline_storage_71440.word_cells[0x300/2]=736;
        if(j==3)g_actor_pipeline_storage_71440.word_cells[0x2e0/2]=752;
        if(j==4)g_actor_pipeline_storage_71440.dword_cells[0x240/4]=416;
        for(i=0;i<13;++i)memcpy(saved[i],outputs[i].source,outputs[i].size);
        fault=invoke(eng1_call_26730);CHECK(fault==0xe0484731UL);
        for(i=0;i<13;++i)CHECK(memcmp(saved[i],outputs[i].source,outputs[i].size)==0);
    }
    printf("camera_source_guard_cases=5 checks=%lu failures=%lu\n",checks-previous,failures);
}
int main(int argc,char **argv)
{
    IMAGE_NT_HEADERS *nt;IMAGE_SECTION_HEADER *table;unsigned i,f,s,p;
    static unsigned flags[]={0x80,0x180,0x380,0x83,0x18c,0};
    static long positions[]={-1,0,50,100,200,65535};
    static long deltas[]={0,1,19,20,21,39,40,295,296,297,65535,65536,
        -1,-19,-20,-21,-296,-297,-65535,-65536,(-2147483647L-1)};
    if(argc!=2)return 2;oracle=LoadLibraryExA(argv[1],0,DONT_RESOLVE_DLL_REFERENCES);if(!oracle)return 3;
    nt=(IMAGE_NT_HEADERS *)cell(((IMAGE_DOS_HEADER *)oracle)->e_lfanew);
    sections=nt->FileHeader.NumberOfSections;if(sections>16)return 4;table=IMAGE_FIRST_SECTION(nt);
    for(i=0;i<sections;++i){
        section_rva[i]=table[i].VirtualAddress;section_size[i]=table[i].Misc.VirtualSize;section_flags[i]=table[i].Characteristics;
        initial[i]=(unsigned char *)HeapAlloc(GetProcessHeap(),0,section_size[i]);
        before[i]=(unsigned char *)HeapAlloc(GetProcessHeap(),0,section_size[i]);
        if(!initial[i]||!before[i])return 5;memcpy(initial[i],cell(section_rva[i]),section_size[i]);
    }
    for(f=0;f<6;++f)for(s=0;s<4;++s)for(p=0;p<6;++p){setup(flags[f],s,positions[p],1);compare(0);}
    setup(0x80,0,50,0);compare(0);setup(0x80,0,100,128);compare(0);
    setup(0x18c,0,100,2);compare(0);
    setup(0x80,4,32791,1);compare(1);
    for(f=0;f<2;++f)for(p=0;p<sizeof(deltas)/sizeof(deltas[0]);++p){
        setup(0x82+f,0,0,1);*(long *)actor=0;
        g_camera_center_x_70cdc=(long)((unsigned long)deltas[p]+160UL);
        memcpy(cell(0x70cdc),&g_camera_center_x_70cdc,4);
        memcpy(actor_before,actor,sizeof actor);
        for(i=0;i<sections;++i)memcpy(before[i],cell(section_rva[i]),section_size[i]);
        compare(0);
    }
    smoothing=1;
    { static long values[]={0,1,-1,8,-8,16,-16,17,-17,2047,2048,-2047,-2048,
          32767,-32768,65535,65536,0x4000,0x4001,0x80000,0x80001,
          -0x560000L,-0x560001L,(-2147483647L-1),2147483647L};
      static int counts[]={-32768,-1,0,1,2,3,15};
      static unsigned levels[]={0,1,5,255};
      for(f=0;f<4;++f)for(p=0;p<25;++p){smoothing_setup(levels[f],values[p],0);compare(0);}
      for(f=0;f<4;++f)for(p=0;p<7;++p){smoothing_setup(levels[f],32767,counts[p]);compare(0);}
      smoothing_setup(1,0,0);
      g_actor_pipeline_storage_71440.word_cells[0x860/2]=47;
      for(p=1;p<=47;++p)g_actor_pipeline_storage_71440.word_cells[0x860/2+p]=(unsigned short)(p*2000);
      memcpy(cell(0x71440),&g_actor_pipeline_storage_71440,sizeof g_actor_pipeline_storage_71440);
      memcpy(pipeline_before,&g_actor_pipeline_storage_71440,sizeof pipeline_before);
      for(i=0;i<sections;++i)memcpy(before[i],cell(section_rva[i]),section_size[i]);
      compare(0);
      smoothing=2;
      for(f=0;f<4;++f){
          smoothing_setup(levels[f],65535,3);
          g_camera_iterations_70cfd=1;memcpy(cell(0x70cfd),&g_camera_iterations_70cfd,1);
          g_camera_flags_70d10=0x18c;memcpy(cell(0x70d10),&g_camera_flags_70d10,2);
          for(i=0;i<sections;++i)memcpy(before[i],cell(section_rva[i]),section_size[i]);
          compare(0);
      }
    }
    smoothing=1;
    smoothing_setup(1,0,0);g_engine_interface.context_004=0;
    memset(cell(0x70f24),0,4);
    for(i=0;i<sections;++i)memcpy(before[i],cell(section_rva[i]),section_size[i]);
    compare(1);
    printf("camera_chain_native_cases=%lu checks=%lu failures=%lu\n",cases,checks,failures);
    guards();
    FreeLibrary(oracle);return failures?1:0;
}
