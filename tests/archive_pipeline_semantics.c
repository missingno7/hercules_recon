/* Separate TUs exercise the real reconstructed adapters/backends and CRT I/O.
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
int main(void) {
    FILE *created;
    int first,second;
    unsigned char bytes[4],payload[8]={1,2,3,4,10,11,12,13};
    memset(g_archive_directory,0,sizeof(g_archive_directory));
    g_archive_directory[0].key=archive_name_hash(archive_normalize_name("A"));
    g_archive_directory[0].offset=0x3000;g_archive_directory[0].length=4;
    g_archive_directory[1].key=archive_name_hash(archive_normalize_name("B"));
    g_archive_directory[1].offset=0x3004;g_archive_directory[1].length=4;
    /* test_pilot executes in a fresh ignored build/tests directory. */
    created=fopen(g_archive_filename,"wb");
    if(!created)return 2;
    EXPECT(fwrite(g_archive_directory,0x3000,1,created)==1);
    EXPECT(fwrite(payload,1,8,created)==8);
    EXPECT(fclose(created)==0);
    if(setjmp(fatal_jump)) {printf("unexpected fatal: %s\n",fatal_seen);return 3;}
    archive_initialize();
    EXPECT(g_archive_directory_count==2);
    EXPECT(g_archive_logical_positions[0]==-1 && g_archive_logical_positions[1023]==-1);
    first=virtual_file_open("A",0,0);second=virtual_file_open("B",0,0);
    EXPECT(first==1 && second==2 && g_public_file_count==2);
    EXPECT(g_public_file_handles[0]==1 && g_public_file_handles[1]==2);
    EXPECT(virtual_file_read(first,bytes,2)==2);
    EXPECT(bytes[0]==1 && bytes[1]==2 && archive_position(1)==2);
    EXPECT(g_archive_last_stream_handle==1);
    EXPECT(virtual_file_seek(second,1,0)==1);
    EXPECT(g_archive_last_stream_handle==1);
    /* Seek does not update the read cache. Same-handle read uses the shared
     * physical stream now pointing into B, while advancing A's logical position. */
    EXPECT(virtual_file_read(first,bytes,1)==1);
    EXPECT(bytes[0]==11 && archive_position(1)==3 && archive_position(2)==1);
    EXPECT(virtual_file_close(first)==0);
    EXPECT(g_public_file_count==1 && g_public_file_handles[0]==0 && g_public_file_handles[1]==2);
    if(setjmp(fatal_jump)==0) {
        virtual_file_read(second,bytes,1);
        EXPECT(0);
    } else EXPECT(fatal_seen==g_read_handle_error);
    EXPECT(virtual_file_close(second)==-1);
    EXPECT(archive_shutdown()==0 && g_archive_stream==0);
    EXPECT(archive_shutdown()==0);
    EXPECT(remove(g_archive_filename)==0);
    printf("archive pipeline: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
