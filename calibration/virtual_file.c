/* PC-derived virtual-handle adapters; labels and original prototypes unproved. */
#include <stdio.h>
extern int g_public_file_count;
extern int g_public_file_handles[16];
extern const char g_open_capacity_error[], g_open_mode_error[];
extern const char g_seek_handle_error[], g_read_handle_error[];
extern const char g_archive_read_mode[];
extern void __cdecl archive_fatal(const char *message);
extern int __cdecl archive_open(const char *path, const char *mode);
extern int __cdecl archive_close(int handle);
extern long __cdecl archive_position(int handle);
extern int __cdecl archive_seek(int handle, long offset, int origin);
extern size_t __cdecl archive_read(void *destination, size_t size, size_t count, int handle);

int __cdecl virtual_file_open(const char *path, int mode, int ignored)
{
    int slot;
    if (g_public_file_count >= 16)
        archive_fatal(g_open_capacity_error);
    if (mode != 0)
        archive_fatal(g_open_mode_error);
    for (slot=0;slot<16;++slot)
        if (g_public_file_handles[slot]==0)
            break;
    g_public_file_handles[slot]=archive_open(path,g_archive_read_mode);
    ++g_public_file_count;
    return slot+1;
}

long __cdecl virtual_file_seek(int handle, long offset, int mode)
{
    int origin;
    if (handle>g_public_file_count || handle<1 || g_public_file_handles[handle-1]==0)
        archive_fatal(g_seek_handle_error);
    origin = mode==0 ? 0 : mode==1 ? 1 : 2;
    archive_seek(g_public_file_handles[handle-1],offset,origin);
    return archive_position(g_public_file_handles[handle-1]);
}

size_t __cdecl virtual_file_read(int handle, void *destination, size_t bytes)
{
    if (handle>g_public_file_count || handle<1 || g_public_file_handles[handle-1]==0)
        archive_fatal(g_read_handle_error);
    return archive_read(destination,1,bytes,g_public_file_handles[handle-1]);
}

int __cdecl virtual_file_close(int handle)
{
    int result;
    if (handle>g_public_file_count || handle<1 || g_public_file_handles[handle-1]==0)
        return -1;
    result=archive_close(g_public_file_handles[handle-1]);
    if (g_public_file_count>0)
        --g_public_file_count;
    g_public_file_handles[handle-1]=0;
    return result;
}
