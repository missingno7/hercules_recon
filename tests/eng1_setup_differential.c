#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/setup_dispatch.c"
EngineInterface g_engine_interface;
extern void __cdecl eng1_empty_c720(void);
static int cases,checks,failures,called_id,called_count;
static unsigned char source_context[256],native_context[256];
static HMODULE oracle;
static void check(int ok,const char *why){++checks;if(!ok){++failures;printf("FAIL case=%d %s\n",cases,why);}}
void __cdecl eng1_native_code_target_00c720(void){called_id=1;++called_count;eng1_empty_c720();}
void __cdecl eng1_native_code_target_0337b0(void){called_id=5;++called_count;}
void __cdecl eng1_native_code_target_01abf0(void){called_id=6;++called_count;}
void __cdecl eng1_native_code_target_03c130(void){called_id=9;++called_count;}
typedef struct Snapshot {unsigned long rva,size;unsigned char *before;} Snapshot;
static Snapshot snapshots[16];static int snapshot_count;
static unsigned char *cell(unsigned long rva){return (unsigned char *)oracle+rva;}
static int begin(void)
{
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)oracle;
    IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS *)cell(dos->e_lfanew);
    IMAGE_SECTION_HEADER *s=IMAGE_FIRST_SECTION(nt);int i;
    snapshot_count=nt->FileHeader.NumberOfSections;if(snapshot_count>16)return 0;
    for(i=0;i<snapshot_count;++i){snapshots[i].rva=s[i].VirtualAddress;snapshots[i].size=s[i].Misc.VirtualSize;
        snapshots[i].before=(unsigned char *)HeapAlloc(GetProcessHeap(),0,snapshots[i].size);
        if(!snapshots[i].before)return 0;}
    return 1;
}
static void before(void)
{int i;for(i=0;i<snapshot_count;++i)memcpy(snapshots[i].before,cell(snapshots[i].rva),snapshots[i].size);}
static void check_sections(void)
{int i,ok=1;for(i=0;i<snapshot_count;++i)if(memcmp(snapshots[i].before,cell(snapshots[i].rva),snapshots[i].size))ok=0;check(ok,"all native mapped section bytes preserved");}
int main(int argc,char **argv)
{
    int level,expected,native_cases=0;unsigned char expected_context[256];
    void (__cdecl *native_setup)(void);
    if(argc!=2)return 2;oracle=LoadLibraryExA(argv[1],0,DONT_RESOLVE_DLL_REFERENCES);
    if(!oracle||!begin())return 3;native_setup=(void (__cdecl *)(void))cell(0x1fdf0);
    for(level=-128;level<=127;++level){
        memset(source_context,0x5a,sizeof source_context);source_context[16]=(unsigned char)level;
        memcpy(native_context,source_context,sizeof source_context);memcpy(expected_context,source_context,sizeof source_context);
        g_engine_interface.context_004=(ResourceCallbackContext *)(source_context+16);
        called_id=called_count=0;eng1_call_1fdf0();
        expected=(level==1||level==5||level==6||level==9)?level:0;
        check(called_count==(expected?1:0)&&called_id==expected,"complete signed-byte switch dispatch");
        check(!memcmp(source_context,expected_context,sizeof source_context),"full guarded source context preserved");
        if(level!=5&&level!=6&&level!=9){
            *(void **)cell(0x70f24)=native_context+16;before();native_setup();++native_cases;
            check(!memcmp(source_context,native_context,sizeof source_context),"native and source full guarded contexts equal");check_sections();
        }
        ++cases;
    }
    printf("setup_cases=%d native_cases=%d checks=%d failures=%d\n",cases,native_cases,checks,failures);
    return failures?1:0;
}
