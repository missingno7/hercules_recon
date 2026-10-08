#include <stdio.h>

/* The row shape and leading positions are observed in the archive initializer.
 * These views do not assert the original declarations or full capacities. */
typedef struct ArchiveDirectoryEntry {
    unsigned long key;
    unsigned long offset;
    unsigned long length;
} ArchiveDirectoryEntry;

extern ArchiveDirectoryEntry g_archive_directory[];
extern unsigned long g_archive_directory_count;
extern int g_archive_logical_positions[];
extern FILE *g_archive_stream;
extern int g_archive_last_stream_handle;
extern char * __cdecl archive_normalize_name(const char *path);
extern unsigned long __cdecl archive_name_hash(const char *path);
extern const char g_archive_open_error_format[];
extern void __cdecl archive_fatal(const char *message);

int __cdecl archive_open(const char *path, const char *ignored_mode)
{
    char message[1024];
    unsigned long key;
    unsigned long row_key;
    int lower;
    int upper;
    int row;
    int old_row;

    (void)ignored_mode;
    key = archive_name_hash(archive_normalize_name(path));
    lower = 0;
    upper = (int)g_archive_directory_count;
    row = upper >> 1;

    while (key != g_archive_directory[row].key) {
        if (lower >= upper - 1) {
            sprintf(message, g_archive_open_error_format, path);
            archive_fatal(message);
            break;
        }

        row_key = g_archive_directory[row].key;
        if (key >= row_key) {
            old_row = row;
            row = ((upper - lower) >> 1) + lower;
            lower = old_row;
        } else {
            upper = row;
            row = ((upper - lower) >> 1) + lower;
        }
    }

    g_archive_logical_positions[row] = 0;
    return row + 1;
}

size_t __cdecl archive_read(
    void *destination, size_t item_size, size_t item_count, int handle)
{
    unsigned long position;
    unsigned long file_length;
    size_t read_count;
    size_t divisor;
    size_t items_read;

    position = (unsigned long)g_archive_logical_positions[handle - 1];
    if (g_archive_last_stream_handle != handle || position == 0) {
        g_archive_last_stream_handle = handle;
        (void)fseek(g_archive_stream,
                    (long)(g_archive_directory[handle - 1].offset + position), 0);
    }

    position = (unsigned long)g_archive_logical_positions[handle - 1];
    file_length = g_archive_directory[handle - 1].length;
    read_count = item_count;
    if (position + item_size * item_count >= file_length) {
        divisor = item_size;
        if (divisor == 0)
            divisor = 1;
        read_count = (size_t)((file_length - position) / divisor);
    }

    items_read = fread(destination, item_size, read_count, g_archive_stream);
    g_archive_logical_positions[handle - 1] = (int)(
        (unsigned long)g_archive_logical_positions[handle - 1] +
        (unsigned long)(items_read * item_size));
    return items_read;
}

int __cdecl archive_seek(int handle, long offset, int origin)
{
    unsigned long position;
    unsigned long file_length;
    unsigned long file_offset;

    if (origin == 1) {
        position = (unsigned long)g_archive_logical_positions[handle - 1] +
                   (unsigned long)offset;
    } else if (origin == 2) {
        position = g_archive_directory[handle - 1].length + (unsigned long)offset;
    } else {
        position = (unsigned long)offset;
    }

    g_archive_logical_positions[handle - 1] = (int)position;
    file_length = g_archive_directory[handle - 1].length;
    if ((unsigned long)g_archive_logical_positions[handle - 1] > file_length)
        g_archive_logical_positions[handle - 1] = (int)file_length;

    file_offset = g_archive_directory[handle - 1].offset +
                  (unsigned long)g_archive_logical_positions[handle - 1];
    return fseek(g_archive_stream, (long)file_offset, 0);
}
