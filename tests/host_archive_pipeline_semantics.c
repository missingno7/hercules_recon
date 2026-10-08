/* Separate TUs exercise the real host services, archive dependencies and CRT I/O.
 * Fatal UI/termination and initial data remain explicit fixture boundaries. */
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
typedef struct ArchiveDirectoryEntry {unsigned long key,offset,length;} ArchiveDirectoryEntry;
ArchiveDirectoryEntry g_archive_directory[1024];
int g_archive_logical_positions[1024];
FILE *g_archive_stream;
unsigned long g_archive_directory_count;
int g_archive_last_stream_handle=-1;
int g_public_file_count;
int g_public_file_handles[16];
const char g_archive_mode[]="rb",g_archive_read_mode[]="rb";
const char g_archive_filename[]="hercules.fs";
const char g_archive_init_open_failure[]="PC_InitFileSys(1)";
const char g_archive_init_read_failure[]="PC_InitFileSys(2)";
const char g_archive_init_sort_failure[]="PC_InitFileSys(3)";
const char g_archive_open_error_format[]="PC_fopen(\"%s\")";
const char g_open_capacity_error[]="PCopen(1)",g_open_mode_error[]="PCopen(2)";
const char g_seek_handle_error[]="PCseek",g_read_handle_error[]="PCread";
/* Synthetic backing extent; no assertion about target scratch capacity. */
char g_archive_scratch[512];
const char g_archive_prefix_maps[15]="M:\\GRAFIX\\MAPS";
const char g_archive_prefix_chop[15]="M:\\GRAFIX\\CHOP";
const char g_archive_prefix_sonix[18]="M:\\LANGUAGE\\SONIX";
extern void archive_initialize(void);
extern int archive_shutdown(void);
extern int virtual_file_open(const char*,int,int);
extern size_t virtual_file_read(int,void*,size_t);
extern long virtual_file_seek(int,long,int);
extern int virtual_file_close(int);
extern long archive_position(int);
extern char *archive_normalize_name(const char *);
extern unsigned long archive_name_hash(const char *);
static jmp_buf fatal_jump;
static const char *fatal_seen;
static int checks,failures;
#define EXPECT(x) do {++checks;if(!(x)){++failures;printf("line %d\n",__LINE__);}}while(0)
void __cdecl archive_fatal(const char *message) {fatal_seen=message;longjmp(fatal_jump,1);}

#include "../calibration/resource_context.h"
ResourceCallbackContext context;
ResourceCallbackContext *g_resource_context=&context;
/* Test-only capacities; original complete backing declarations remain unknown. */
char g_resource_filename[512];
const char g_resource_filename_format[]="%s%s";
const char g_resource_loading_format[]="Loading %s @ 0x%x,size %d ";
const char g_resource_async_open_failure_format[]="Unable to open %s (%d)\n";
const char g_resource_sync_open_failure_format[]="Unable to open %s \n";
const char g_resource_sync_loaded_format[]="- Loaded \n";
const char g_resource_close_failure_format[]="- Failed \n";
unsigned long g_resource_prepare_scalar;
extern void host_file_prepare(void);
extern unsigned long host_file_sync(const char*,const char*,void*,unsigned long);
extern int host_file_async(const char*,const char*,int,void*,unsigned long);
extern void host_cancel_load(void);
extern void host_file_progress(void);
extern unsigned long resource_file_length(const char*,const char*);
static int complete_count,failed_count;
static void __cdecl completed(int arg) {
    ++complete_count;
    EXPECT(arg==0 && context.transfer_status_0dc==0);
    EXPECT(context.mode_09c==4 && context.countdown_0bc==12);
    EXPECT(g_public_file_count==0);
}
static void __cdecl failed(int arg) {
    ++failed_count;
    EXPECT(arg==0 && context.transfer_status_0dc==0x100);
    EXPECT(context.mode_09c==1 && context.transfer_result_0d8==-1);
    /* The real failure callback has cancellation responsibilities. */
    host_cancel_load();
}
int main(void) {
    FILE *created;
    unsigned char payload[4]={1,2,3,4},bytes[2048];
    int i;
    memset(g_archive_directory,0,sizeof(g_archive_directory));
    g_archive_directory[0].key=archive_name_hash(archive_normalize_name("A"));
    g_archive_directory[0].offset=0x3000;g_archive_directory[0].length=4;
    created=fopen(g_archive_filename,"wb");if(!created)return 2;
    EXPECT(fwrite(g_archive_directory,0x3000,1,created)==1);
    EXPECT(fwrite(payload,1,4,created)==4);
    EXPECT(fclose(created)==0);
    if(setjmp(fatal_jump)){printf("unexpected fatal: %s\n",fatal_seen);return 3;}
    archive_initialize();
    EXPECT(g_archive_directory_count==1);
    EXPECT(resource_file_length("","A")==4 && g_public_file_count==0);
    context.prepare_state_1b00=9;g_resource_prepare_scalar=8;
    memset(bytes,0xee,sizeof(bytes));
    EXPECT(host_file_sync("","A",bytes,4)==4);
    EXPECT(memcmp(bytes,payload,4)==0 && g_public_file_count==0);
    EXPECT(context.prepare_state_1b00==0 && g_resource_prepare_scalar==0);
    context.load_complete=completed;context.load_failed=failed;
    memset(bytes,0xee,sizeof(bytes));
    EXPECT(host_file_async("","A",0,bytes,4)==2);
    EXPECT(context.mode_09c==1 && context.remaining_sectors_09e==1 && context.total_sectors_0a0==1);
    EXPECT(g_public_file_count==1 && context.destination_cursor_a4==bytes);
    EXPECT(host_file_async("","A",0,bytes,4)==1 && g_public_file_count==1);
    host_file_progress();
    EXPECT(complete_count==1 && failed_count==0);
    EXPECT(memcmp(bytes,payload,4)==0 && bytes[4]==0xee);
    /* Archive read clamps to four bytes, while the service advances one sector. */
    EXPECT(context.destination_cursor_a4==bytes+2048 && context.remaining_sectors_09e==0);
    EXPECT(context.mode_09c==4 && context.countdown_0bc==12);
    for(i=0;i<11;++i)host_file_progress();
    EXPECT(context.mode_09c==4 && context.countdown_0bc==1 && complete_count==1);
    host_file_progress();
    EXPECT(context.mode_09c==0 && context.countdown_0bc==0);
    EXPECT(host_file_async("","A",0,bytes,4)==2 && g_public_file_count==1);
    context.transfer_result_0d8=-1;host_file_progress();
    EXPECT(failed_count==1 && complete_count==1);
    EXPECT(context.mode_09c==0 && context.transfer_result_0d8==0 && g_public_file_count==0);
    EXPECT(archive_shutdown()==0 && g_archive_stream==0);
    EXPECT(remove(g_archive_filename)==0);
    printf("host archive pipeline: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
