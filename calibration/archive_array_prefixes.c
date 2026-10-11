/* Observed working storage prefixes only. Original complete declarations,
 * aggregate/TU ownership and placement remain unproved. No scratch is supplied.
 * The native full-table next-key probe crosses the directory prefix into the
 * first position word. These declarations do not prove that terminal source
 * expression, or general initializer behavior for a full nonzero directory. */
#include <stddef.h>

typedef struct ArchiveDirectoryEntry {
    unsigned long key;
    unsigned long offset;
    unsigned long length;
} ArchiveDirectoryEntry;

typedef char archive_row_is_12[sizeof(ArchiveDirectoryEntry)==12 ? 1:-1];
typedef char archive_row_offsets[
    offsetof(ArchiveDirectoryEntry,offset)==4 &&
    offsetof(ArchiveDirectoryEntry,length)==8 ? 1:-1];
typedef char archive_position_is_4[sizeof(int)==4 ? 1:-1];

ArchiveDirectoryEntry g_archive_directory[1024] = {0};
int g_archive_logical_positions[1024] = {0};
