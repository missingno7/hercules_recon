#include <stdio.h>
#include <string.h>
#include "../calibration/virtual_file.c"
int g_public_file_count;
int g_public_file_handles[16];
const char g_open_capacity_error[]="PCopen(1)",g_open_mode_error[]="PCopen(2)";
const char g_seek_handle_error[]="PCseek",g_read_handle_error[]="PCread";
const char g_archive_read_mode[]="rb";
static unsigned long checks,failures;
static int fatal_calls,open_calls,seek_calls,position_calls,read_calls,close_calls;
static const char *fatal_message;
static int redirect_slot,redirect_value,count_after_close,mutate_count;
static int backend_handle,backend_origin,open_result,close_result;
static long backend_offset,position_result;
static size_t read_result,read_size,read_count;
static void *read_destination;
static const char *open_path,*open_mode;
#define EXPECT(x) do{++checks;if(!(x)){++failures;printf("line %d\n",__LINE__);}}while(0)
void __cdecl archive_fatal(const char *message) {
    ++fatal_calls;fatal_message=message;
    if(redirect_slot>=0)g_public_file_handles[redirect_slot]=redirect_value;
}
int __cdecl archive_open(const char *path,const char *mode) {
    ++open_calls;open_path=path;open_mode=mode;return open_result;
}
int __cdecl archive_seek(int handle,long offset,int origin) {
    ++seek_calls;backend_handle=handle;backend_offset=offset;backend_origin=origin;
    if(redirect_slot>=0)g_public_file_handles[redirect_slot]=redirect_value;
    return -123;
}
long __cdecl archive_position(int handle) {
    ++position_calls;backend_handle=handle;return position_result;
}
size_t __cdecl archive_read(void *destination,size_t size,size_t count,int handle) {
    ++read_calls;read_destination=destination;read_size=size;read_count=count;backend_handle=handle;
    return read_result;
}
int __cdecl archive_close(int handle) {
    ++close_calls;backend_handle=handle;
    if(mutate_count)g_public_file_count=count_after_close;
    if(redirect_slot>=0)g_public_file_handles[redirect_slot]=redirect_value;
    return close_result;
}
static void reset(void) {
    memset(g_public_file_handles,0,sizeof(g_public_file_handles));g_public_file_count=0;
    fatal_calls=open_calls=seek_calls=position_calls=read_calls=close_calls=0;
    redirect_slot=-1;mutate_count=0;fatal_message=0;open_result=77;close_result=-42;
    position_result=-2147483647L;read_result=0x87654321U;
}
int main(void) {
    int count,handle,occupied,i,valid,result,mode;
    char destination[16],path[]="file";
    for(count=-1;count<=16;++count)for(handle=-1;handle<=18;++handle)for(occupied=0;occupied<=1;++occupied) {
        reset();g_public_file_count=count;
        for(i=0;i<16;++i)g_public_file_handles[i]=occupied ? i+100:0;
        valid=(handle>=1 && handle<=count && occupied);
        result=virtual_file_close(handle);
        EXPECT(result==(valid?close_result:-1));
        EXPECT(close_calls==valid);
        EXPECT(g_public_file_count==(valid?count-1:count));
        if(valid) { EXPECT(g_public_file_handles[handle-1]==0);EXPECT(backend_handle==handle+99); }
    }
    reset();g_public_file_count=2;g_public_file_handles[0]=101;g_public_file_handles[1]=102;
    EXPECT(virtual_file_close(1)==close_result);
    EXPECT(g_public_file_count==1 && g_public_file_handles[1]==102);
    EXPECT(virtual_file_read(2,destination,0xfedcba98U)==read_result);
    EXPECT(fatal_calls==1 && fatal_message==g_read_handle_error);
    EXPECT(read_destination==destination && read_size==1 && read_count==0xfedcba98U && backend_handle==102);
    for(i=0;i<3;++i) {
        reset();g_public_file_count=2;g_public_file_handles[1]=90;
        mutate_count=1;count_after_close=i==0?-3:i==1?0:8;
        redirect_slot=1;redirect_value=500;
        EXPECT(virtual_file_close(2)==-42);
        EXPECT(g_public_file_count==(count_after_close>0?count_after_close-1:count_after_close));
        EXPECT(g_public_file_handles[1]==0);
    }
    for(mode=-3;mode<=3;++mode) {
        reset();g_public_file_count=3;g_public_file_handles[1]=90;
        redirect_slot=1;redirect_value=91;
        EXPECT(virtual_file_seek(2,-2147483647L,mode)==position_result);
        EXPECT(fatal_calls==0 && seek_calls==1 && position_calls==1);
        EXPECT(backend_origin==(mode==0?0:mode==1?1:2));
        EXPECT(backend_offset==-2147483647L && backend_handle==91);
    }
    reset();g_public_file_count=1;g_public_file_handles[3]=90;
    redirect_slot=3;redirect_value=91;
    EXPECT(virtual_file_seek(4,4L,0)==position_result);
    EXPECT(fatal_calls==1 && fatal_message==g_seek_handle_error && backend_handle==91);
    for(i=0;i<16;++i) {
        reset();g_public_file_count=15;
        for(handle=0;handle<16;++handle)g_public_file_handles[handle]=100+handle;
        g_public_file_handles[i]=0;
        EXPECT(virtual_file_open(path,0,0x12345678)==i+1);
        EXPECT(g_public_file_handles[i]==77 && g_public_file_count==16);
        EXPECT(open_path==path && open_mode==g_archive_read_mode && open_calls==1 && fatal_calls==0);
    }
    reset();g_public_file_count=16;
    EXPECT(virtual_file_open(path,3,0)==1);
    EXPECT(fatal_calls==2 && fatal_message==g_open_mode_error && open_calls==1 && g_public_file_count==17);
    reset();open_result=0;
    EXPECT(virtual_file_open(path,0,0)==1);
    EXPECT(g_public_file_count==1 && g_public_file_handles[0]==0);
    printf("virtual_file: %lu checks, %lu failures\n",checks,failures);
    return failures?1:0;
}
