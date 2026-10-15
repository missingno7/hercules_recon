/* Test-only native differential. The replacement never calls original code.
 * Original ENG1 is mapped in an isolated test process with no instruction patch.
 * Fade callback comparisons model the actual host's low-byte argument use;
 * raw DWORD residue differences are printed separately. Direct bridge probes
 * require full DWORD forwarding. All outputs and module neighbors are observed. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../calibration/engine_interface.h"

#ifdef LEVEL6_COLOR_TEST
#include "../src/eng1/behavioral/actor_pipeline_storage.h"
#include "../src/eng1/behavioral/initial_spatial_roots.h"
static int level6_active;
#endif

typedef unsigned char eng1_u8;
typedef unsigned short eng1_u16;
typedef unsigned long eng1_u32;
extern void __cdecl eng1_color_grid_update_5a60(int,int,int,int,int,int);
extern void __cdecl eng1_color_fade_step_5d50(void);
extern void __cdecl eng1_color_profile_5dc0(int);
extern void __cdecl eng1_frame_color_write_5e00(int);
extern void __cdecl dispatch_engine_2c830(eng1_u32);
extern eng1_u32 g_mesh_colors_6f4e8[512],g_frame_global_70cf8,g_frame_global_73488;
extern eng1_u8 g_frame_global_70cfc;

EngineInterface g_engine_interface;
eng1_u16 g_frame_word_6ee84,g_dispatch_setting_4d858,g_dispatch_word_6ee80;
eng1_u8 g_dispatch_key_7286c,g_packed_byte_70cf0,g_packed_byte_70cf1,g_packed_byte_70cf2;
eng1_u32 g_frame_global_70cf4;
eng1_u8 g_frame_global_734f0,g_frame_global_734f1,g_frame_global_734f2;
eng1_u8 *g_dispatch_frame_base_71ec8;

typedef void (__cdecl *Command0)(void);
typedef void (__cdecl *Command1)(int);
typedef void (__cdecl *Command6)(int,int,int,int,int,int);
typedef void (__cdecl *WordCallback)(eng1_u32);
typedef struct CallbackState {
    eng1_u32 value,current,scaled,packet,lut_hash,frame_hash;
    eng1_u16 word;
    eng1_u8 step,average;
    eng1_u32 lut[512];
    eng1_u8 base_frame[0x2910+32],current_frame[0x2910+32];
} CallbackState;
typedef struct Snapshot {
    eng1_u16 word,key_setting,lower;
    eng1_u8 step,weight[3],frame_rgb[3],average;
    eng1_u32 current,scaled,packet,lut[512];
    eng1_u8 frame[0x2910+32];
    eng1_u8 current_frame[0x2910+32];
    eng1_u32 raw_callback[8];
    CallbackState callbacks[8];
    int callback_count,trap_calls;
} Snapshot;
static HMODULE oracle;
static eng1_u8 source_frame[0x2910+32],native_frame[0x2910+32];
static eng1_u8 source_current_frame[0x2910+32],native_current_frame[0x2910+32];
static int side,case_index,checks,failures;
static int states,raw_callback_differences;
static eng1_u8 frame_before[0x2910+32];
static eng1_u32 random_state=0x97c097c0UL;
static Snapshot logs[2];
static eng1_u8 *cell(eng1_u32 rva){return (eng1_u8 *)oracle+rva;}
static eng1_u32 hash_bytes(const void *memory,int bytes)
{
    const eng1_u8 *p=(const eng1_u8 *)memory;
    eng1_u32 value=2166136261UL;int i;
    for(i=0;i<bytes;++i)value=(value^p[i])*16777619UL;
    return value;
}
static void check(int ok,const char *family,const char *detail)
{
    ++checks;if(!ok){++failures;printf("FAIL %s case=%d %s\n",family,case_index,detail);}
}
static void __cdecl observer(eng1_u32 value)
{
    if(logs[side].callback_count<8){
        CallbackState *p=&logs[side].callbacks[logs[side].callback_count];
        logs[side].raw_callback[logs[side].callback_count]=value;
        p->value=value;
        p->word=side?g_frame_word_6ee84:*(eng1_u16 *)cell(0x6ee84);
        p->step=side?g_dispatch_key_7286c:*cell(0x7286c);
        p->current=side?g_frame_global_70cf4:*(eng1_u32 *)cell(0x70cf4);
        p->scaled=side?g_frame_global_70cf8:*(eng1_u32 *)cell(0x70cf8);
        p->packet=side?g_frame_global_73488:*(eng1_u32 *)cell(0x73488);
        p->average=side?g_frame_global_70cfc:*cell(0x70cfc);
        p->lut_hash=hash_bytes(side?(void *)g_mesh_colors_6f4e8:(void *)cell(0x6f4e8),2048);
        p->frame_hash=hash_bytes(side?source_frame:native_frame,sizeof source_frame);
        memcpy(p->lut,side?(void *)g_mesh_colors_6f4e8:(void *)cell(0x6f4e8),sizeof p->lut);
        memcpy(p->base_frame,side?source_frame:native_frame,sizeof p->base_frame);
        memcpy(p->current_frame,side?source_current_frame:native_current_frame,sizeof p->current_frame);
    }
    ++logs[side].callback_count;
}
static void __cdecl wrong_slot_observer(eng1_u32 first,eng1_u32 second)
{(void)first;(void)second;++logs[side].trap_calls;}
/* Snapshot every mapped section. Restore only writable sections between cases;
 * compare every section after native execution except the seven output ranges.
 * Code is observed and never made writable or patched. */
typedef struct ModuleSection {eng1_u32 rva,bytes,flags;eng1_u8 *initial,*before;} ModuleSection;
static ModuleSection module_sections[16];static int module_section_count;
static int module_begin(void)
{
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)oracle;
    IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS *)cell(dos->e_lfanew);
    IMAGE_SECTION_HEADER *sections=IMAGE_FIRST_SECTION(nt);int i;
    module_section_count=nt->FileHeader.NumberOfSections;
    if(module_section_count>16)return 0;
    for(i=0;i<module_section_count;++i){
        ModuleSection *s=&module_sections[i];
        s->rva=sections[i].VirtualAddress;s->bytes=sections[i].Misc.VirtualSize;
        s->flags=sections[i].Characteristics;
        s->initial=(eng1_u8 *)HeapAlloc(GetProcessHeap(),0,s->bytes);
        s->before=(eng1_u8 *)HeapAlloc(GetProcessHeap(),0,s->bytes);
        if(!s->initial || !s->before)return 0;
        memcpy(s->initial,cell(s->rva),s->bytes);
    }
    return 1;
}
static void module_restore(void)
{
    int i;for(i=0;i<module_section_count;++i){
        ModuleSection *s=&module_sections[i];
        if(s->flags&IMAGE_SCN_MEM_WRITE)memcpy(cell(s->rva),s->initial,s->bytes);
    }
}
static void module_before(void)
{
    int i;for(i=0;i<module_section_count;++i){
        ModuleSection *s=&module_sections[i];memcpy(s->before,cell(s->rva),s->bytes);
    }
}
static int permitted_output(eng1_u32 rva)
{
    #ifdef LEVEL6_COLOR_TEST
    if(level6_active && ((rva>=0x5a7b0 && rva<0x5a7b6) || (rva>=0x5a7b8 && rva<0x5a7be) || (rva>=0x6fea0 && rva<0x6fea4) || (rva>=0x70cbc && rva<0x70cc0)))return 1;
#endif
    return (rva>=0x6f4e8 && rva<0x6fce8) ||
        (rva>=0x6ee84 && rva<0x6ee86) || rva==0x7286c ||
        (rva>=0x70cf4 && rva<0x70cfd) ||
        (rva>=0x73488 && rva<0x7348c);
}
static void module_compare(const char *family)
{
    int i,ok=1;eng1_u32 j;
    for(i=0;i<module_section_count && ok;++i){
        ModuleSection *s=&module_sections[i];
        for(j=0;j<s->bytes;++j)if(s->before[j]!=cell(s->rva)[j] && !permitted_output(s->rva+j)){
            printf("Unexpected native module write RVA=%lx\n",s->rva+j);ok=0;break;
        }
    }
    check(ok,family,"module sections and neighboring bytes preserved");
}
static eng1_u32 next_random(void)
{random_state=random_state*1664525UL+1013904223UL;return random_state;}
static void initial(Snapshot *p,int n)
{
    static const eng1_u16 words[8]={0,1,128,256,32767,32768,65535,64};
    int i;
    memset(p,0,sizeof *p);
    p->word=n<8?words[n]:(eng1_u16)next_random();
    p->step=(eng1_u8)next_random();p->key_setting=(eng1_u16)next_random();p->lower=(eng1_u16)next_random();
    for(i=0;i<3;++i){p->weight[i]=(eng1_u8)next_random();p->frame_rgb[i]=(eng1_u8)next_random();}
    p->current=0xdeadbeefUL;p->scaled=0x11223344UL;p->packet=0x55667788UL;p->average=0x99;
    for(i=0;i<512;++i)p->lut[i]=next_random();
    for(i=0;i<(int)sizeof p->frame;++i)p->frame[i]=(eng1_u8)(i*37+n);
}
static void bind_fixture(const Snapshot *p)
{
    EngineInterface *native_interface=(EngineInterface *)cell(0x70f20);
    module_restore();
    memset(logs,0,sizeof logs);
    g_engine_interface.opaque_1fc[16]=(eng1_u32)observer;
    g_engine_interface.opaque_1fc[17]=(eng1_u32)wrong_slot_observer;
    native_interface->opaque_1fc[16]=(eng1_u32)observer;
    native_interface->opaque_1fc[17]=(eng1_u32)wrong_slot_observer;
    g_frame_word_6ee84=p->word;*(eng1_u16 *)cell(0x6ee84)=p->word;
    g_dispatch_key_7286c=p->step;*cell(0x7286c)=p->step;
    g_dispatch_setting_4d858=p->key_setting;*(eng1_u16 *)cell(0x4d858)=p->key_setting;
    g_dispatch_word_6ee80=p->lower;*(eng1_u16 *)cell(0x6ee80)=p->lower;
    g_packed_byte_70cf0=p->weight[0];*cell(0x70cf0)=p->weight[0];
    g_packed_byte_70cf1=p->weight[1];*cell(0x70cf1)=p->weight[1];
    g_packed_byte_70cf2=p->weight[2];*cell(0x70cf2)=p->weight[2];
    g_frame_global_734f0=p->frame_rgb[0];*cell(0x734f0)=p->frame_rgb[0];
    g_frame_global_734f1=p->frame_rgb[1];*cell(0x734f1)=p->frame_rgb[1];
    g_frame_global_734f2=p->frame_rgb[2];*cell(0x734f2)=p->frame_rgb[2];
    g_frame_global_70cf4=p->current;*(eng1_u32 *)cell(0x70cf4)=p->current;
    g_frame_global_70cf8=p->scaled;*(eng1_u32 *)cell(0x70cf8)=p->scaled;
    g_frame_global_73488=p->packet;*(eng1_u32 *)cell(0x73488)=p->packet;
    g_frame_global_70cfc=p->average;*cell(0x70cfc)=p->average;
    memcpy(g_mesh_colors_6f4e8,p->lut,sizeof p->lut);memcpy(cell(0x6f4e8),p->lut,sizeof p->lut);
    memcpy(source_frame,p->frame,sizeof p->frame);memcpy(native_frame,p->frame,sizeof p->frame);
    memcpy(frame_before,p->frame,sizeof p->frame);
    memcpy(source_current_frame,p->frame,sizeof p->frame);memcpy(native_current_frame,p->frame,sizeof p->frame);
    g_dispatch_frame_base_71ec8=source_frame+16;
    *(eng1_u8 **)cell(0x71ec8)=native_frame+16;
    *(eng1_u8 **)cell(0x71ec4)=native_current_frame+16;
    module_before();
}
static void capture(int n)
{
    Snapshot *p=&logs[n];
    if(n==0){
        p->word=*(eng1_u16 *)cell(0x6ee84);p->step=*cell(0x7286c);
        p->current=*(eng1_u32 *)cell(0x70cf4);p->scaled=*(eng1_u32 *)cell(0x70cf8);
        p->packet=*(eng1_u32 *)cell(0x73488);p->average=*cell(0x70cfc);
        memcpy(p->lut,cell(0x6f4e8),sizeof p->lut);memcpy(p->frame,native_frame,sizeof p->frame);
        memcpy(p->current_frame,native_current_frame,sizeof p->current_frame);
    }else{
        p->word=g_frame_word_6ee84;p->step=g_dispatch_key_7286c;
        p->current=g_frame_global_70cf4;p->scaled=g_frame_global_70cf8;
        p->packet=g_frame_global_73488;p->average=g_frame_global_70cfc;
        memcpy(p->lut,g_mesh_colors_6f4e8,sizeof p->lut);memcpy(p->frame,source_frame,sizeof p->frame);
        memcpy(p->current_frame,source_current_frame,sizeof p->current_frame);
    }
}
static void compare(const char *family)
{
    int i,before_failures=failures;
    int frame_neighbors_ok=1;
    ++states;module_compare(family);
    check(logs[0].word==logs[1].word,family,"fade word");
    check(logs[0].step==logs[1].step,family,"fade step");
    check(logs[0].current==logs[1].current,family,"weighted current RGB");
    check(logs[0].scaled==logs[1].scaled,family,"scaled RGB");
    check(logs[0].packet==logs[1].packet,family,"base packet");
    check(logs[0].average==logs[1].average,family,"average byte");
    check(memcmp(logs[0].lut,logs[1].lut,sizeof logs[0].lut)==0,family,"512-word LUT");
    check(memcmp(logs[0].frame,logs[1].frame,sizeof logs[0].frame)==0,family,"full guarded frame buffers");
    check(memcmp(logs[0].current_frame,logs[1].current_frame,sizeof logs[0].current_frame)==0,family,"independent current frame preserved");
    check(memcmp(logs[0].current_frame,frame_before,sizeof frame_before)==0,family,"native independent current frame untouched");
    for(i=0;i<(int)sizeof frame_before;++i)
        if(i!=16+0x19 && i!=16+0x1a && i!=16+0x1b &&
           i!=16+0x14a1 && i!=16+0x14a2 && i!=16+0x14a3 &&
           logs[0].frame[i]!=frame_before[i])frame_neighbors_ok=0;
    check(frame_neighbors_ok,family,"native base frame canaries and unobserved bytes untouched");
    check(logs[0].callback_count==logs[1].callback_count,family,"callback count");
    check(logs[0].trap_calls==0 && logs[1].trap_calls==0,family,"wrong +240 callback selected");
    for(i=0;i<logs[0].callback_count && i<logs[1].callback_count && i<8;++i){
        CallbackState a=logs[0].callbacks[i],b=logs[1].callbacks[i];
        check((eng1_u8)logs[0].raw_callback[i]==(eng1_u8)logs[1].raw_callback[i],family,"meaningful low-byte callback value");
        if(a.value!=b.value){
            if(raw_callback_differences<8)printf("RAW_CALLBACK %s case=%d ordinal=%d native=%08lx source=%08lx\n",family,case_index,i,a.value,b.value);
            ++raw_callback_differences;
        }
        a.value=0;b.value=0;
        check(memcmp(&a,&b,sizeof a)==0,family,"state and callback order at invocation");
    }
    if(failures!=before_failures)printf("RESULT %s case=%d word=%04x step=%02x current=%08lx/%08lx scaled=%08lx/%08lx average=%02x/%02x\n",family,case_index,logs[0].word,logs[0].step,logs[0].current,logs[1].current,logs[0].scaled,logs[1].scaled,logs[0].average,logs[1].average);
}
static void run_grid(const char *family,Snapshot *start,const int *a)
{
    int before_failures=failures;
    bind_fixture(start);side=0;
    ((Command6)cell(0x5a60))(a[0],a[1],a[2],a[3],a[4],a[5]);capture(0);
    side=1;eng1_color_grid_update_5a60(a[0],a[1],a[2],a[3],a[4],a[5]);capture(1);compare(family);
    if(failures!=before_failures)printf("INPUT word=%04x weights=%u,%u,%u args=%d,%d,%d,%d,%d,%d\n",start->word,start->weight[0],start->weight[1],start->weight[2],a[0],a[1],a[2],a[3],a[4],a[5]);
}
#ifdef LEVEL6_COLOR_TEST
/* Reuse this fixture's complete color, frame and native-module comparisons. */
extern void __cdecl eng1_call_26730(void);
extern unsigned char g_level6_color_target_5a7b0[6],g_level6_color_current_5a7b8[6];
extern long g_camera_offset_y_70cbc,g_camera_z_offset_70d18;
extern long g_camera_angle_x_70cd0,g_camera_angle_y_70cd4;
extern short g_camera_adjust_y_70d04;
static unsigned char color_context[0x100],pending_bytes[32],row_bytes[135][8];
static unsigned char pending_before[32],rows_before[135][8];
static int level6_cases;
static unsigned long level6_invoke(Command0 fn)
{
    unsigned long fault=0,a,b;
    __asm mov a,esp
    __try {fn();} __except(fault=GetExceptionCode(),EXCEPTION_EXECUTE_HANDLER) {}
    __asm mov b,esp
    check(a==b,"26730","ESP preserved");return fault;
}
static void level6_bind(Snapshot *start,int lane,int delta,int high,int count,int pending)
{
    int i,j;unsigned long *d=g_actor_pipeline_storage_71440.dword_cells;
    static long offsets[]={0,16384,16385,-0x560001L,(-2147483647L-1)};
    bind_fixture(start);level6_active=1;
    memset(color_context,0,sizeof color_context);color_context[0]=6;
    g_engine_interface.context_004=(ResourceCallbackContext *)color_context;
    *(void **)cell(0x70f24)=color_context;
    memset(&g_actor_pipeline_storage_71440,0,sizeof g_actor_pipeline_storage_71440);
    d[0x6a0/4]=(unsigned long)count;
    memset(pending_bytes,0xa7,sizeof pending_bytes);
    memset(row_bytes,0xb6,sizeof row_bytes);
    for(i=0;i<6;++i){
        g_level6_color_target_5a7b0[i]=g_level6_color_current_5a7b8[i]=(eng1_u8)high;
        pending_bytes[8+i]=(eng1_u8)(255-high-i*13);
    }
    g_level6_color_target_5a7b0[lane]=(eng1_u8)(high?high-delta:delta);
    for(j=0;j<count && j<135;++j){
        for(i=0;i<6;++i)row_bytes[j][i]=(eng1_u8)(j*19+i*31+level6_cases);
        d[0x6a4/4+j]=(unsigned long)row_bytes[j];
    }
    g_level6_color_pending_6fea0=pending?pending_bytes+8:0;
    memcpy(cell(0x5a7b0),g_level6_color_target_5a7b0,6);
    memcpy(cell(0x5a7b8),g_level6_color_current_5a7b8,6);
    memcpy(cell(0x6fea0),&g_level6_color_pending_6fea0,4);
    memcpy(cell(0x71440),&g_actor_pipeline_storage_71440,sizeof g_actor_pipeline_storage_71440);
    g_camera_offset_y_70cbc=offsets[level6_cases%5];
    memcpy(cell(0x70cbc),&g_camera_offset_y_70cbc,4);
    g_camera_angle_x_70cd0=111;g_camera_angle_y_70cd4=-222;g_camera_z_offset_70d18=333;
    g_camera_adjust_y_70d04=0x1234;
    *(long *)cell(0x70cd0)=111;*(long *)cell(0x70cd4)=-222;*(long *)cell(0x70d18)=333;
    *(short *)cell(0x70d04)=0x1234;
    memcpy(pending_before,pending_bytes,sizeof pending_bytes);memcpy(rows_before,row_bytes,sizeof row_bytes);
    module_before();
}
static void run_level6(int lane,int delta,int high,int count,int pending,int cache)
{
    Snapshot start;unsigned long a,b;unsigned char current[6];
    initial(&start,level6_cases);if(cache)start.current=0;
    level6_bind(&start,lane,delta,high,count,pending);
    if(cache){memset(row_bytes,0,sizeof row_bytes);memcpy(rows_before,row_bytes,sizeof row_bytes);}
    if(pending==2){
        g_level6_color_pending_6fea0=g_level6_color_current_5a7b8;
        *(void **)cell(0x6fea0)=cell(0x5a7b8);module_before();
    }
    memcpy(current,g_level6_color_current_5a7b8,6);
    case_index=level6_cases++;side=0;a=level6_invoke((Command0)cell(0x26730));capture(0);
    side=1;b=level6_invoke(eng1_call_26730);capture(1);compare("26730-level6");
    check(a==0 && b==0,"26730-level6","no exception on supported path");
    check(memcmp(cell(0x5a7b0),g_level6_color_target_5a7b0,6)==0,"26730-level6","target six bytes");
    check(memcmp(cell(0x5a7b8),g_level6_color_current_5a7b8,6)==0,"26730-level6","current six bytes");
    check(*(void **)cell(0x6fea0)==0 && !g_level6_color_pending_6fea0,"26730-level6","pending target consumed");
    check(memcmp(cell(0x6fe60),g_eng1_spatial_types_06fe60,sizeof g_eng1_spatial_types_06fe60)==0,"26730-level6","shared spatial prefix and pending pointer alias");
    check(*(long *)cell(0x70cbc)==g_camera_offset_y_70cbc,"26730-level6","common offset smoothing");
    check(g_camera_angle_x_70cd0==111 && g_camera_angle_y_70cd4==-222 && g_camera_z_offset_70d18==333 && g_camera_adjust_y_70d04==0x1234,"26730-level6","camera angle stages skipped");
    check(memcmp(cell(0x71440),&g_actor_pipeline_storage_71440,sizeof g_actor_pipeline_storage_71440)==0,"26730-level6","whole shared pipeline state");
    check(memcmp(pending_bytes,pending_before,sizeof pending_bytes)==0 && memcmp(row_bytes,rows_before,sizeof row_bytes)==0,"26730-level6","pending and row guards");
    if(count)check(memcmp(current,g_level6_color_current_5a7b8,6)==0,"26730-level6","list leaves current color record unchanged");
    if(cache)check(memcmp(logs[0].lut,start.lut,sizeof start.lut)==0,"26730-level6","native cache hit retains poisoned LUT");
    level6_active=0;
}
static void level6_suite(void)
{
    static int deltas[]={1,2,3,16,127,254,255};
    static int counts[]={1,3,135};
    int lane,d,high,count,pending,old_checks=checks;Snapshot start;
    for(lane=0;lane<6;++lane)for(d=0;d<7;++d)for(high=0;high<2;++high)
        run_level6(lane,deltas[d],high?255:0,0,0,0);
    for(count=0;count<3;++count)for(pending=0;pending<3;++pending)
        run_level6(0,0,0,counts[count],pending,0);
    for(lane=0;lane<6;++lane)run_level6(lane,1,0,0,1,0);
    run_level6(0,0,0,1,0,1);run_level6(0,0,0,3,1,1);
    printf("level6_native_cases=%d checks=%d failures=%d\n",level6_cases,checks-old_checks,failures);
    old_checks=checks;
    for(d=0;d<6;++d){
        unsigned long fault;Snapshot saved;unsigned char target[6],current[6],pipeline[0x8c0];
        unsigned char *saved_pending;long offset;
        initial(&start,0);level6_bind(&start,0,0,0,0,0);
        if(d==1)g_level6_color_pending_6fea0=g_level6_color_current_5a7b8;
        if(d==2)g_actor_pipeline_storage_71440.dword_cells[0x6a0/4]=0xffffffffUL;
        if(d==3)g_actor_pipeline_storage_71440.dword_cells[0x6a0/4]=136;
        if(d==4)g_level6_color_pending_6fea0=g_level6_color_target_5a7b0+1;
        if(d==5){g_actor_pipeline_storage_71440.dword_cells[0x6a0/4]=1;g_actor_pipeline_storage_71440.dword_cells[0x240/4]=416;}
        capture(1);saved=logs[1];memcpy(target,g_level6_color_target_5a7b0,6);memcpy(current,g_level6_color_current_5a7b8,6);
        memcpy(pipeline,&g_actor_pipeline_storage_71440,sizeof pipeline);
        saved_pending=g_level6_color_pending_6fea0;offset=g_camera_offset_y_70cbc;
        fault=level6_invoke(eng1_call_26730);capture(1);
        check(fault==0xe0484731UL,"26730-guard","explicit unsupported diagnostic");
        check(memcmp(&saved,&logs[1],sizeof saved)==0,"26730-guard","color/LUT/frame state unchanged");
        check(memcmp(target,g_level6_color_target_5a7b0,6)==0 && memcmp(current,g_level6_color_current_5a7b8,6)==0 && saved_pending==g_level6_color_pending_6fea0,"26730-guard","pending/color records unchanged");
        check(memcmp(pipeline,&g_actor_pipeline_storage_71440,sizeof pipeline)==0 && offset==g_camera_offset_y_70cbc,"26730-guard","pipeline and camera unchanged");
    }
    level6_active=0;
    printf("level6_source_guard_cases=6 checks=%d failures=%d\n",checks-old_checks,failures);
}
#endif

int main(int argc,char **argv)
{
    static const int directed_grid[][7]={
        {100,10,20,30,50,80,110}, {128,0,0,0,128,128,128},
        {1,-1,-2,-4,0,0,0}, {1,-255,256,-257,-1,1,-128},
        {32767,-12,15,-18,255,-256,257}, {32768,1,-1,254,255,-256,257},
        {65535,-13,-14,-15,32767,-32768,65536}, {128,254,255,256,1,1,1},
        {65535,0,0,0,1073741824,-1073741824,2147483647},
        {32767,-129,128,-1,65535,65536,65537},
        {1,-1,-1,-1,1,-1,0}, {0,0,0,0,-32768,32767,-1}
    };
    static const unsigned long directed_fade[][4]={
        {100,0,128,0},{100,8,128,0},{125,8,128,0},{128,1,128,0},
        {0,255,128,0},{5,246,128,0},{0,255,128,32768},
        {65535,1,128,0},{32767,1,65535,32768},{32767,1,128,32768},
        {32768,128,65535,32768},{32768,127,65535,32768},
        {0,1,65535,2},{1,255,128,0},{100,1,101,0},{101,255,101,100}
    };
    static const eng1_u32 callback_words[]={0,1,255,256,0x12345678UL,0x80000000UL,0xffffffffUL};
    static const int profile_levels[]={(-2147483647-1),2147483647,-65536,-32768,-257,-256,-1,0,1,255,256,32767,65535,65536};
    Snapshot start;
    Command6 native6;Command0 native0;Command1 native1;
    int i,a[6],level;
    if(argc!=2)return 2;
    oracle=LoadLibraryExA(argv[1],0,DONT_RESOLVE_DLL_REFERENCES);
    if(!oracle){printf("LOAD_FAILED %lu\n",GetLastError());return 3;}
    if(!module_begin()){printf("SNAPSHOT_FAILED\n");return 4;}
    native6=(Command6)cell(0x5a60);native0=(Command0)cell(0x5d50);
    for(i=0;i<(int)(sizeof callback_words/sizeof callback_words[0]);++i){
        case_index=i;initial(&start,i);bind_fixture(&start);
        side=0;((WordCallback)cell(0x2c830))(callback_words[i]);capture(0);
        side=1;dispatch_engine_2c830(callback_words[i]);capture(1);compare("2c830-full-DWORD-forwarding");
        check(logs[0].callback_count==1 && logs[1].callback_count==1,"2c830-full-DWORD-forwarding","one callback per direct wrapper call");
        check(logs[0].raw_callback[0]==callback_words[i] && logs[1].raw_callback[0]==callback_words[i],"2c830-full-DWORD-forwarding","entire DWORD forwarded unchanged");
    }
    for(i=0;i<(int)(sizeof directed_grid/sizeof directed_grid[0]);++i){
        case_index=i;initial(&start,i);start.word=(eng1_u16)directed_grid[i][0];
        start.weight[0]=64;start.weight[1]=128;start.weight[2]=192;
        run_grid("5a60-directed",&start,directed_grid[i]+1);
    }
    for(i=0;i<256;++i){
        case_index=i;initial(&start,i);
        a[0]=(int)(next_random()&1023)-512;a[1]=(int)(next_random()&1023)-512;a[2]=(int)(next_random()&1023)-512;
        a[3]=(int)(short)next_random();a[4]=(int)(short)next_random();a[5]=(int)(short)next_random();
        run_grid("5a60-mixed-sign",&start,a);
    }
    for(i=0;i<(int)(sizeof directed_fade/sizeof directed_fade[0]);++i){
        case_index=i;initial(&start,i);
        start.word=(eng1_u16)directed_fade[i][0];start.step=(eng1_u8)directed_fade[i][1];
        start.key_setting=(eng1_u16)directed_fade[i][2];start.lower=(eng1_u16)directed_fade[i][3];bind_fixture(&start);
        side=0;native0();capture(0);side=1;eng1_color_fade_step_5d50();capture(1);compare("5d50-directed");
    }
    initial(&start,0);start.word=120;start.step=3;start.key_setting=128;start.lower=0;bind_fixture(&start);
    side=0;for(i=0;i<7;++i)native0();capture(0);
    side=1;for(i=0;i<7;++i)eng1_color_fade_step_5d50();capture(1);
    case_index=0;compare("5d50-sequence-positive-clamp");
    initial(&start,0);start.word=8;start.step=253;start.key_setting=128;start.lower=0;bind_fixture(&start);
    side=0;for(i=0;i<7;++i)native0();capture(0);
    side=1;for(i=0;i<7;++i)eng1_color_fade_step_5d50();capture(1);
    case_index=0;compare("5d50-sequence-negative-clamp");
    for(i=0;i<512;++i){
        case_index=i;initial(&start,i);
        a[0]=(int)(next_random()&255);a[1]=(int)(next_random()&255);a[2]=(int)(next_random()&255);
        a[3]=(int)(next_random()&255);a[4]=(int)(next_random()&255);a[5]=(int)(next_random()&255);
        bind_fixture(&start);side=0;native6(a[0],a[1],a[2],a[3],a[4],a[5]);capture(0);
        side=1;eng1_color_grid_update_5a60(a[0],a[1],a[2],a[3],a[4],a[5]);capture(1);compare("5a60");
        if(i<128){
            start.current=logs[0].current;bind_fixture(&start);
            side=0;native6(a[0],a[1],a[2],a[3],a[4],a[5]);capture(0);
            side=1;eng1_color_grid_update_5a60(a[0],a[1],a[2],a[3],a[4],a[5]);capture(1);compare("5a60-cache");
            check(memcmp(logs[0].lut,start.lut,sizeof start.lut)==0,"5a60-cache","native preserves poisoned LUT on cache hit");
        }
    }
    for(i=0;i<512;++i){
        case_index=i;initial(&start,i);bind_fixture(&start);
        side=0;native0();capture(0);side=1;eng1_color_fade_step_5d50();capture(1);compare("5d50");
    }
    native1=(Command1)cell(0x5dc0);
    for(i=0;i<(int)(sizeof profile_levels/sizeof profile_levels[0]);++i){
        case_index=i;initial(&start,i);bind_fixture(&start);
        side=0;native1(profile_levels[i]);capture(0);
        side=1;eng1_color_profile_5dc0(profile_levels[i]);capture(1);compare("5dc0-directed-level-width");
    }
    /* Cache return leaves the original ECX upper bytes from packed current RGB.
     * The actual host observes only CL. Capture the complete pushed DWORD too. */
    initial(&start,0);start.word=101;start.weight[0]=64;start.weight[1]=128;start.weight[2]=192;bind_fixture(&start);
    side=0;native1(101);capture(0);
    start.current=logs[0].current;start.word=100;start.step=1;start.key_setting=128;start.lower=0;
    bind_fixture(&start);side=0;native0();capture(0);
    side=1;eng1_color_fade_step_5d50();capture(1);case_index=0;compare("5d50-profile-cache-hit-callback-residue");
    check(memcmp(logs[0].lut,start.lut,sizeof start.lut)==0,"5d50-profile-cache-hit-callback-residue","native cached profile preserves poisoned LUT");
    for(i=0;i<512;++i){
        case_index=i;initial(&start,i);level=(int)(short)next_random();bind_fixture(&start);
        side=0;native1(level);capture(0);side=1;eng1_color_profile_5dc0(level);capture(1);compare("5dc0");
    }
    native1=(Command1)cell(0x5e00);
    for(i=0;i<512;++i){
        case_index=i;initial(&start,i);bind_fixture(&start);
        side=0;native1(-123);capture(0);side=1;eng1_frame_color_write_5e00(-123);capture(1);compare("5e00");
    }
    #ifdef LEVEL6_COLOR_TEST
    level6_suite();
    #endif
    module_restore();
    printf("native_states=%d checks=%d failures=%d raw_callback_differences=%d\n",states,checks,failures,raw_callback_differences);
    FreeLibrary(oracle);return failures?1:0;
}
