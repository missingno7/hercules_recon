#include <stdio.h>
#include <string.h>

typedef struct ArchiveDirectoryEntry {
    unsigned long key;
    unsigned long offset;
    unsigned long length;
} ArchiveDirectoryEntry;

/* Observed leading views only; these declarations do not prove full capacity. */
extern ArchiveDirectoryEntry g_archive_directory[];
extern int g_archive_logical_positions[];
extern FILE *g_archive_stream;
extern unsigned long g_archive_directory_count;

/* The byte strings are read directly from the verified PC image. */
extern const char g_archive_mode[];
extern const char g_archive_filename[];
extern const char g_archive_init_open_failure[];
extern const char g_archive_init_read_failure[];
extern const char g_archive_init_sort_failure[];
extern void __cdecl archive_fatal(const char *message);

void __cdecl archive_initialize(void)
{
    ArchiveDirectoryEntry *entry;
    unsigned long next_key;
    int count;
    int outer;
    int inner;

    memset(g_archive_directory, 0, 0x3000UL);
    memset(g_archive_logical_positions, 0xff, 0x1000UL);

    g_archive_stream = fopen(g_archive_filename, g_archive_mode);
    if (g_archive_stream == NULL)
        archive_fatal(g_archive_init_open_failure);

    if (fread(g_archive_directory, 0x3000UL, 1UL, g_archive_stream) != 1UL)
        archive_fatal(g_archive_init_read_failure);

    count = 0;
    g_archive_directory_count = 0UL;
    if (g_archive_directory[0].key != 0UL) {
        entry = g_archive_directory;
        while (entry < g_archive_directory + 1024) {
            next_key = (entry + 1)->key;
            ++entry;
            ++count;
            if (next_key == 0UL)
                break;
        }
    }
    g_archive_directory_count = (unsigned long)count;

    if ((long)g_archive_directory_count <= 0L)
        return;

    for (outer = 0; outer < (int)g_archive_directory_count; ++outer) {
        for (inner = outer + 1; inner < (int)g_archive_directory_count; ++inner) {
            if (g_archive_directory[inner].key <= g_archive_directory[outer].key)
                archive_fatal(g_archive_init_sort_failure);
        }
    }
}

int __cdecl archive_shutdown(void)
{
    int result;

    result = 0;
    if (g_archive_stream != NULL)
        result = fclose(g_archive_stream);
    g_archive_stream = NULL;
    return result;
}
