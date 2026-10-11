#include <stdio.h>

typedef struct ArchiveDirectoryEntry {
    unsigned long key;
    unsigned long offset;
    unsigned long length;
} ArchiveDirectoryEntry;

extern ArchiveDirectoryEntry g_archive_directory[];
extern int g_archive_logical_positions[];
extern FILE *g_archive_stream;
extern int g_archive_last_stream_handle;

size_t __cdecl archive_read(
    void *destination, size_t item_size, size_t item_count, int handle)
{
    unsigned long position;
    unsigned long file_length;
    size_t divisor;
    size_t items_read;

    if (g_archive_last_stream_handle != handle ||
        (unsigned long)g_archive_logical_positions[handle - 1] == 0) {
        position = (unsigned long)g_archive_logical_positions[handle - 1];
        g_archive_last_stream_handle = handle;
        (void)fseek(g_archive_stream,
                    (long)(g_archive_directory[handle - 1].offset + position), 0);
    }

    position = (unsigned long)g_archive_logical_positions[handle - 1];
    file_length = g_archive_directory[handle - 1].length;
    if (position + item_size * item_count >= file_length) {
        divisor = item_size;
        if (divisor == 0)
            divisor = 1;
        item_count = (size_t)((file_length - position) / divisor);
    }

    items_read = fread(destination, item_size, item_count, g_archive_stream);
    g_archive_logical_positions[handle - 1] = (int)(
        (unsigned long)g_archive_logical_positions[handle - 1] +
        (unsigned long)(items_read * item_size));
    return items_read;
}
