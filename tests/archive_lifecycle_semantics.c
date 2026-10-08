#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "../calibration/archive_lifecycle.c"

ArchiveDirectoryEntry g_archive_directory[1024];
int g_archive_logical_positions[1024];
FILE *g_archive_stream;
unsigned long g_archive_directory_count;
const char g_archive_mode[] = "rb";
const char g_archive_filename[] = "hercules.fs";
const char g_archive_init_open_failure[] = "PC_InitFileSys(1)";
const char g_archive_init_read_failure[] = "PC_InitFileSys(2)";
const char g_archive_init_sort_failure[] = "PC_InitFileSys(3)";

static jmp_buf diagnostic_jump;
static int diagnostic_jump_enabled;
static int diagnostic_count;
static char diagnostic_last[64];
static int checks;
static int failures;

void __cdecl archive_fatal(const char *message)
{
    ++diagnostic_count;
    strncpy(diagnostic_last, message, sizeof(diagnostic_last) - 1);
    diagnostic_last[sizeof(diagnostic_last) - 1] = '\0';
    if (diagnostic_jump_enabled)
        longjmp(diagnostic_jump, 1);
}

static void check(const char *name, int passed)
{
    ++checks;
    if (!passed) {
        ++failures;
        printf("FAIL: %s\n", name);
    }
}

static int make_archive(const unsigned long *keys, int key_count, int complete)
{
    ArchiveDirectoryEntry rows[1024];
    FILE *file;
    int i;
    size_t bytes;

    memset(rows, 0, sizeof(rows));
    for (i = 0; i < key_count; ++i) {
        rows[i].key = keys[i];
        rows[i].offset = (unsigned long)(i * 64);
        rows[i].length = (unsigned long)(i + 1);
    }

    file = fopen("hercules.fs", "wb");
    if (file == NULL)
        return 0;
    bytes = complete ? sizeof(rows) : sizeof(rows[0]);
    if (fwrite(rows, 1, bytes, file) != bytes) {
        fclose(file);
        remove("hercules.fs");
        return 0;
    }
    fclose(file);
    return 1;
}

static void reset_diagnostics(void)
{
    diagnostic_count = 0;
    diagnostic_last[0] = '\0';
}

static int all_positions_are_minus_one(void)
{
    int i;
    for (i = 0; i < 1024; ++i) {
        if (g_archive_logical_positions[i] != -1)
            return 0;
    }
    return 1;
}

int main(void)
{
    unsigned long sorted_keys[3];
    unsigned long hole_keys[3];
    unsigned long unsorted_keys[3];
    unsigned long short_keys[1];
    int i;

    g_archive_stream = NULL;
    sorted_keys[0] = 0x80000000UL;
    sorted_keys[1] = 0x80000001UL;
    sorted_keys[2] = 0xf0000000UL;
    check("write full sorted synthetic directory", make_archive(sorted_keys, 3, 1));
    g_archive_directory_count = 777UL;
    for (i = 0; i < 1024; ++i)
        g_archive_logical_positions[i] = 0x12345678;
    reset_diagnostics();
    archive_initialize();
    check("initializer counts a three-row nonzero prefix", g_archive_directory_count == 3UL);
    check("unsigned high-bit keys remain strictly ordered", diagnostic_count == 0);
    check("initializer retains actual row fields from the file", g_archive_directory[2].key == 0xf0000000UL && g_archive_directory[2].offset == 128UL && g_archive_directory[2].length == 3UL);
    check("all observed position slots initialize to -1", all_positions_are_minus_one());
    check("a zero directory tail is retained", g_archive_directory[3].key == 0UL && g_archive_directory[1023].length == 0UL);
    check("successful initializer leaves its stream open", g_archive_stream != NULL);
    check("shutdown returns fclose status and clears stream", archive_shutdown() == 0 && g_archive_stream == NULL);

    hole_keys[0] = 5UL;
    hole_keys[1] = 0UL;
    hole_keys[2] = 99UL;
    check("write directory with a zero-key hole", make_archive(hole_keys, 3, 1));
    reset_diagnostics();
    archive_initialize();
    check("count stops at the first zero key", g_archive_directory_count == 1UL);
    check("post-hole rows are not part of the counted prefix", g_archive_directory[2].key == 99UL && diagnostic_count == 0);
    check("shutdown closes second opened stream", archive_shutdown() == 0 && g_archive_stream == NULL);

    unsorted_keys[0] = 10UL;
    unsorted_keys[1] = 12UL;
    unsorted_keys[2] = 11UL;
    check("write unsorted synthetic directory", make_archive(unsorted_keys, 3, 1));
    reset_diagnostics();
    archive_initialize();
    check("unsorted key pair is diagnosed without stopping later work", diagnostic_count == 1 && strcmp(diagnostic_last, g_archive_init_sort_failure) == 0 && g_archive_directory_count == 3UL);
    check("shutdown closes unsorted-directory stream", archive_shutdown() == 0 && g_archive_stream == NULL);

    short_keys[0] = 4UL;
    check("write one-record short file", make_archive(short_keys, 1, 0));
    reset_diagnostics();
    archive_initialize();
    check("short fread is diagnosed with the observed message", diagnostic_count >= 1 && strcmp(diagnostic_last, g_archive_init_read_failure) == 0);
    check("short read still leaves a closable stream", g_archive_stream != NULL && archive_shutdown() == 0 && g_archive_stream == NULL);

    remove("hercules.fs");
    reset_diagnostics();
    diagnostic_jump_enabled = 1;
    if (setjmp(diagnostic_jump) == 0) {
        archive_initialize();
        check("missing file takes the expected failure path", 0);
    } else {
        check("open failure diagnoses before the following fread", strcmp(diagnostic_last, g_archive_init_open_failure) == 0 && g_archive_stream == NULL);
    }
    diagnostic_jump_enabled = 0;
    check("null-stream shutdown returns zero", archive_shutdown() == 0 && g_archive_stream == NULL);

    printf("archive lifecycle fixture: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
