#include <stdio.h>
#include <string.h>
#include "../calibration/archive_backend.c"

ArchiveDirectoryEntry g_archive_directory[1024];
unsigned long g_archive_directory_count;
int g_archive_logical_positions[1024];
FILE *g_archive_stream;
int g_archive_last_stream_handle;
const char g_archive_open_error_format[] = "PC_fopen(\"%s\")";

static int checks, failures;
static char normalized_path[128];
static const char *normalize_argument;
static const char *hash_argument;
static unsigned long configured_hash;
static int normalize_calls, hash_calls, diagnostic_calls;
static char diagnostic_message[256];
static unsigned char archive_bytes[128];
static unsigned long stream_offset;
static int fseek_calls, last_fseek_origin, fseek_result;
static int mutate_position_on_fseek, mutation_handle, mutation_position, position_mutation_seen;
static long last_fseek_offset;
static int fread_calls;
static size_t last_fread_size, last_fread_count;
static unsigned long last_fread_offset;

#define expect(label, expression) do { \
    ++checks; \
    if (!(expression)) { ++failures; printf("FAIL: %s\n", label); } \
} while (0)

char * __cdecl archive_normalize_name(const char *path)
{
    normalize_argument = path;
    ++normalize_calls;
    strcpy(normalized_path, path);
    return normalized_path;
}
unsigned long __cdecl archive_name_hash(const char *path)
{
    hash_argument = path;
    ++hash_calls;
    return configured_hash;
}
void __cdecl archive_fatal(const char *message)
{
    ++diagnostic_calls;
    strncpy(diagnostic_message, message, sizeof(diagnostic_message) - 1);
    diagnostic_message[sizeof(diagnostic_message) - 1] = '\0';
}
int __cdecl fseek(FILE *stream, long offset, int origin)
{
    ++fseek_calls;
    last_fseek_offset = offset;
    last_fseek_origin = origin;
    if (stream != g_archive_stream || origin != 0 || offset < 0) return -1;
    stream_offset = (unsigned long)offset;
    if (mutate_position_on_fseek) {
        g_archive_logical_positions[mutation_handle - 1] = mutation_position;
        mutate_position_on_fseek = 0;
        position_mutation_seen = 1;
    }
    return fseek_result;
}
size_t __cdecl fread(void *destination, size_t item_size, size_t item_count, FILE *stream)
{
    size_t available_items, copied_items, i;
    unsigned char *out;
    ++fread_calls;
    last_fread_size = item_size;
    last_fread_count = item_count;
    last_fread_offset = stream_offset;
    if (stream != g_archive_stream || item_size == 0 || item_count == 0 ||
        stream_offset >= sizeof(archive_bytes)) return 0;
    available_items = (sizeof(archive_bytes) - stream_offset) / item_size;
    copied_items = item_count < available_items ? item_count : available_items;
    out = (unsigned char *)destination;
    for (i = 0; i < copied_items * item_size; ++i)
        out[i] = archive_bytes[stream_offset + i];
    stream_offset += (unsigned long)(copied_items * item_size);
    return copied_items;
}
static void reset_fixture(void)
{
    int i;
    memset(g_archive_directory, 0, sizeof(g_archive_directory));
    for (i = 0; i < 1024; ++i) g_archive_logical_positions[i] = -1;
    memset(normalized_path, 0, sizeof(normalized_path));
    normalize_argument = hash_argument = 0;
    configured_hash = 0;
    normalize_calls = hash_calls = diagnostic_calls = 0;
    memset(diagnostic_message, 0, sizeof(diagnostic_message));
    for (i = 0; i < (int)sizeof(archive_bytes); ++i) archive_bytes[i] = (unsigned char)i;
    stream_offset = 0;
    fseek_calls = 0;
    last_fseek_offset = 0;
    last_fseek_origin = -1;
    fseek_result = 0;
    mutate_position_on_fseek = 0;
    mutation_handle = 0;
    mutation_position = 0;
    position_mutation_seen = 0;
    fread_calls = 0;
    last_fread_size = last_fread_count = 0;
    last_fread_offset = 0;
    g_archive_directory_count = 0;
    g_archive_stream = (FILE *)1;
    g_archive_last_stream_handle = 0;
}
static void set_row(int row, unsigned long key, unsigned long offset, unsigned long length)
{
    g_archive_directory[row].key = key;
    g_archive_directory[row].offset = offset;
    g_archive_directory[row].length = length;
}
static void test_open(void)
{
    const char *path;
    int handle;
    reset_fixture();
    g_archive_directory_count = 3;
    set_row(0, 0x00000100UL, 10, 5);
    set_row(1, 0x80000000UL, 20, 7);
    set_row(2, 0xf0000000UL, 40, 4);
    path = "Alpha";
    configured_hash = 0x80000000UL;
    handle = archive_open(path, "rb");
    expect("matching unsigned middle key returns one-based handle", handle == 2);
    expect("open normalizes first argument once", normalize_calls == 1 && normalize_argument == path);
    expect("open hashes normalized result once", hash_calls == 1 && hash_argument == normalized_path);
    expect("open clears only matching row position", g_archive_logical_positions[1] == 0 &&
           g_archive_logical_positions[0] == -1 && g_archive_logical_positions[2] == -1);
    expect("successful open does not diagnose", diagnostic_calls == 0);

    reset_fixture();
    g_archive_directory_count = 3;
    set_row(0, 0x00000100UL, 10, 5);
    set_row(1, 0x80000000UL, 20, 7);
    set_row(2, 0xf0000000UL, 40, 4);
    path = "MissingBetween";
    configured_hash = 0x80000001UL;
    handle = archive_open(path, "rb");
    expect("unsigned between-key miss falls through at lower row plus one", handle == 2);
    expect("miss invokes diagnostic once", diagnostic_calls == 1);
    expect("miss message formats original path", strcmp(diagnostic_message, "PC_fopen(\"MissingBetween\")") == 0);
    expect("miss resets final search row position", g_archive_logical_positions[1] == 0);

    reset_fixture();
    g_archive_directory_count = 3;
    set_row(0, 0x00000100UL, 10, 5);
    set_row(1, 0x80000000UL, 20, 7);
    set_row(2, 0xf0000000UL, 40, 4);
    configured_hash = 0;
    handle = archive_open("BeforeFirst", "rb");
    expect("below-first miss returns first one-based row", handle == 1);
    expect("below-first miss resets row zero and diagnoses", g_archive_logical_positions[0] == 0 && diagnostic_calls == 1);

    reset_fixture();
    set_row(0, 9, 10, 4);
    configured_hash = 8;
    handle = archive_open("EmptyIndex", "rb");
    expect("zero-count miss retains target row-zero fallthrough", handle == 1 &&
           diagnostic_calls == 1 && g_archive_logical_positions[0] == 0);
}
static void test_read_and_seek(void)
{
    unsigned char destination[16];
    size_t items;
    int calls_before, status;
    reset_fixture();
    g_archive_directory_count = 3;
    set_row(0, 0x100, 10, 5);
    set_row(1, 0x200, 20, 7);
    set_row(2, 0x300, 40, 4);
    /* A successful archive_open resets handle 2's logical position to zero. */
    g_archive_logical_positions[1] = 0;
    memset(destination, 0xcc, sizeof(destination));
    items = archive_read(destination, 2, 5, 2);
    expect("first read seeks shared stream to row offset", fseek_calls == 1 &&
           last_fseek_offset == 20 && last_fseek_origin == 0 && g_archive_last_stream_handle == 2);
    expect("read clips by complete items before logical EOF", last_fread_count == 3 && items == 3);
    expect("read copies requested bytes and preserves destination tail", destination[0] == 20 &&
           destination[5] == 25 && destination[6] == 0xcc);
    expect("logical position advances by returned item bytes", g_archive_logical_positions[1] == 6);

    fseek_result = -1;
    status = archive_seek(2, -3L, 2);
    expect("END seek adds signed offset to length and returns fseek status", status == -1 &&
           g_archive_logical_positions[1] == 4 && last_fseek_offset == 24);
    calls_before = fseek_calls;
    memset(destination, 0xcc, sizeof(destination));
    items = archive_read(destination, 2, 2, 2);
    expect("same-handle nonzero read position does not reseek", fseek_calls == calls_before);
    expect("same-handle read clips by remaining complete items", last_fread_count == 1 && items == 1 &&
           last_fread_offset == 24 && destination[0] == 24 && destination[1] == 25);
    expect("partial final item is not counted", g_archive_logical_positions[1] == 6);

    calls_before = fread_calls;
    items = archive_read(destination, 2, 1, 2);
    expect("EOF read calls fread with zero items and preserves position", fread_calls == calls_before + 1 &&
           last_fread_count == 0 && items == 0 && g_archive_logical_positions[1] == 6);

    g_archive_logical_positions[2] = 0;
    memset(destination, 0xcc, sizeof(destination));
    items = archive_read(destination, 1, 3, 3);
    expect("handle change reseeks to next row offset", last_fseek_offset == 40 &&
           g_archive_last_stream_handle == 3 && last_fread_offset == 40);
    expect("next row position advances independently", items == 3 && g_archive_logical_positions[2] == 3 &&
           destination[0] == 40 && destination[2] == 42);

    g_archive_logical_positions[2] = 4;
    calls_before = fseek_calls;
    items = archive_read(destination, 0, 9, 3);
    expect("zero-size EOF read uses divisor guard and does not reseek", fseek_calls == calls_before &&
           last_fread_size == 0 && last_fread_count == 0 && items == 0 &&
           g_archive_logical_positions[2] == 4);

    fseek_result = 17;
    status = archive_seek(2, 2L, 1);
    expect("CURRENT seek adds current position and returns fseek status", status == 17 &&
           g_archive_logical_positions[1] == 7 && last_fseek_offset == 27);
    status = archive_seek(2, 20L, 0);
    expect("SET seek clamps above-end position before physical seek", status == 17 &&
           g_archive_logical_positions[1] == 7 && last_fseek_offset == 27);
    status = archive_seek(2, -1L, 0);
    expect("negative SET wraps unsigned then clamps to EOF", status == 17 &&
           g_archive_logical_positions[1] == 7 && last_fseek_offset == 27);
    status = archive_seek(2, 3L, 77);
    expect("unrecognized backend origin follows absolute path", status == 17 &&
           g_archive_logical_positions[1] == 3 && last_fseek_offset == 23);
}
static void test_post_seek_position_reload(void)
{
    unsigned char destination[8];
    size_t items;
    reset_fixture();
    g_archive_directory_count = 2;
    set_row(0, 0x100, 10, 5);
    set_row(1, 0x200, 20, 7);
    g_archive_logical_positions[1] = 0;
    g_archive_last_stream_handle = 2;
    mutate_position_on_fseek = 1;
    mutation_handle = 2;
    mutation_position = 3;
    memset(destination, 0xcc, sizeof(destination));
    items = archive_read(destination, 2, 3, 2);
    expect("fseek hook runs after captured position determines physical offset",
           position_mutation_seen && fseek_calls == 1 && last_fseek_offset == 20);
    expect("post-seek bounds use reloaded logical position",
           last_fread_count == 2 && items == 2 && g_archive_logical_positions[1] == 7);
}
int main(void)
{
    test_open();
    test_read_and_seek();
    test_post_seek_position_reload();
    printf("checks=%d failures=%d\n", checks, failures);
    return failures != 0;
}
