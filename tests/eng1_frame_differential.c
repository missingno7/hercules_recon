/*
 * Bounded native-vs-reconstructed ENG1 frame-command differential fixture.
 * The oracle DLL is mapped with DONT_RESOLVE_DLL_REFERENCES in this standalone
 * process. The four tested call paths use only local code and the two copied
 * interface callbacks installed below. No game process, target code patching,
 * or binary-backed implementation is involved.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/shared/engine_dispatch_externs.h"
#include "../src/eng1/data_layouts.h"

#define ORACLE_05600 0x5600UL
#define ORACLE_02CB0 0x2cb0UL
#define ORACLE_050D0 0x50d0UL
#define ORACLE_058B0 0x58b0UL
#define OT_OFFSET 0x88UL
#define OT_WORDS 0x500UL
#define OT_BYTES (OT_WORDS * 4UL)
#define FRAME_BYTES (OT_OFFSET + OT_BYTES + 16UL)
#define MAX_UPLOADS 16
#define MAX_UPLOAD_BYTES 0x4000UL
#define MAX_REGIONS 4

typedef void (__cdecl *Eng1Command)(void);
typedef struct FixtureRegion {
    const unsigned char *base;
    unsigned long bytes;
} FixtureRegion;
typedef struct UploadRecord {
    unsigned short rectangle[4];
    int region;
    long offset;
    unsigned long bytes;
    unsigned char pixels[MAX_UPLOAD_BYTES];
} UploadRecord;
typedef struct SpyLog {
    unsigned long chain_calls;
    unsigned long chain_count;
    int chain_region;
    long chain_offset;
    unsigned long upload_count;
    int error;
    UploadRecord uploads[MAX_UPLOADS];
} SpyLog;

static HMODULE g_oracle;
EngineInterface g_engine_interface;
unsigned char *g_dispatch_frame_current_71ec4;
unsigned char *g_dispatch_frame_base_71ec8;
void *g_frame_owner_70c80;
void *g_frame_owner_72870;
void *g_frame_owner_7348c;
unsigned long g_frame_global_73484;
unsigned char g_frame_global_73498;
Eng1ProfileRow28 eng1_profile_rows_5df08[12];
#define g_frame_config_5df08 (*(unsigned char (*)[sizeof(eng1_profile_rows_5df08)])eng1_profile_rows_5df08)
unsigned long g_frame_upload_pending_6ee7c;
unsigned short g_frame_upload_coordinates_734a0[32];
unsigned char *g_header_pointer_70d38;
unsigned char g_root_count_70ed1;
static ResourceCallbackContext g_candidate_context;
static ResourceCallbackContext g_oracle_context;
static SpyLog g_spy[2];
static FixtureRegion g_regions[2][MAX_REGIONS];
static int g_active_side;
static unsigned long g_checks;
static unsigned long g_failures;

static void fail(const char *case_name, const char *detail)
{
    ++g_failures;
    printf("FAIL %s: %s\n", case_name, detail);
}

static void check(int condition, const char *case_name, const char *detail)
{
    ++g_checks;
    if (!condition)
        fail(case_name, detail);
}

static unsigned long pointer_word(const void *pointer)
{
    return (unsigned long)pointer;
}

static int classify_pointer(const void *pointer, unsigned long bytes,
                            long *offset)
{
    unsigned long address = pointer_word(pointer);
    int index;

    for (index = 0; index < MAX_REGIONS; ++index) {
        unsigned long base;
        unsigned long end;

        if (g_regions[g_active_side][index].base == 0)
            continue;
        base = pointer_word(g_regions[g_active_side][index].base);
        end = base + g_regions[g_active_side][index].bytes;
        if (address >= base && address <= end && bytes <= end - address) {
            *offset = (long)(address - base);
            return index;
        }
    }
    *offset = 0;
    return -1;
}

/* Same reviewed 32-bit host writer is used as the interface spy for both sides. */
static int __cdecl spy_frame_chain_write(void *first, unsigned long raw_count)
{
    unsigned long count;
    void **cursor;
    long offset;
    int region;

    ++g_spy[g_active_side].chain_calls;
    g_spy[g_active_side].chain_count = raw_count;
    region = classify_pointer(first, OT_BYTES, &offset);
    g_spy[g_active_side].chain_region = region;
    g_spy[g_active_side].chain_offset = offset;
    if (region < 0)
        g_spy[g_active_side].error = 1;

    cursor = (void **)first;
    count = raw_count;
    --count;
    if ((int)count > 0) {
        do {
            void **next;
            next = cursor + 1;
            *cursor = next;
            cursor = next;
            --count;
        } while (count != 0);
    }
    *cursor = 0;
    return 0;
}

static void __cdecl spy_texture_upload(void *rectangle, const void *texels)
{
    UploadRecord *record;
    unsigned long byte_count;
    long offset;
    int region;
    unsigned long index;

    if (g_spy[g_active_side].upload_count >= MAX_UPLOADS) {
        g_spy[g_active_side].error = 2;
        return;
    }
    record = &g_spy[g_active_side].uploads[
        g_spy[g_active_side].upload_count++];
    memcpy(record->rectangle, rectangle, sizeof(record->rectangle));
    byte_count = (unsigned long)record->rectangle[2] *
                 (unsigned long)record->rectangle[3] * 2UL;
    if (byte_count > MAX_UPLOAD_BYTES) {
        g_spy[g_active_side].error = 3;
        record->bytes = 0;
        return;
    }
    region = classify_pointer(texels, byte_count, &offset);
    record->region = region;
    record->offset = offset;
    record->bytes = byte_count;
    if (region < 0) {
        g_spy[g_active_side].error = 4;
        return;
    }
    for (index = 0; index < byte_count; ++index)
        record->pixels[index] = ((const unsigned char *)texels)[index];
}

static void reset_spies(void)
{
    memset(g_spy, 0, sizeof(g_spy));
    memset(g_regions, 0, sizeof(g_regions));
}

static void set_region(int side, int index, const void *base,
                       unsigned long bytes)
{
    g_regions[side][index].base = (const unsigned char *)base;
    g_regions[side][index].bytes = bytes;
}

static void bind_interface(EngineInterface *interface_value,
                           ResourceCallbackContext *context)
{
    interface_value->context_004 = context;
    interface_value->frame_chain_write_144 = spy_frame_chain_write;
    interface_value->opaque_1fc[2] =
        pointer_word((const void *)spy_texture_upload);
}

static void bind_oracle_interface(void)
{
    EngineInterface *interface_value =
        (EngineInterface *)((unsigned char *)g_oracle + 0x70f20);
    bind_interface(interface_value, &g_oracle_context);
}

static void fill_bytes(unsigned char *buffer, unsigned long bytes,
                       unsigned char seed)
{
    unsigned long index;
    for (index = 0; index < bytes; ++index)
        buffer[index] = (unsigned char)(seed + index * 37UL + index / 7UL);
}

static int compare_spies(const char *case_name)
{
    unsigned long index;
    int equal = 1;

    check(g_spy[0].error == 0 && g_spy[1].error == 0,
          case_name, "spy reported a pointer/extent error");
    check(g_spy[0].chain_calls == g_spy[1].chain_calls,
          case_name, "chain callback count differs");
    check(g_spy[0].chain_count == g_spy[1].chain_count,
          case_name, "chain callback count argument differs");
    check(g_spy[0].chain_region == g_spy[1].chain_region &&
          g_spy[0].chain_offset == g_spy[1].chain_offset,
          case_name, "chain callback target offset differs");
    check(g_spy[0].upload_count == g_spy[1].upload_count,
          case_name, "upload callback count/order length differs");
    if (g_spy[0].upload_count != g_spy[1].upload_count)
        equal = 0;

    for (index = 0; index < g_spy[0].upload_count &&
                    index < g_spy[1].upload_count; ++index) {
        UploadRecord *native_record = &g_spy[0].uploads[index];
        UploadRecord *source_record = &g_spy[1].uploads[index];
        int match = 1;

        if (memcmp(native_record->rectangle, source_record->rectangle,
                   sizeof(native_record->rectangle)) != 0 ||
            native_record->region != source_record->region ||
            native_record->offset != source_record->offset ||
            native_record->bytes != source_record->bytes ||
            memcmp(native_record->pixels, source_record->pixels,
                   native_record->bytes) != 0)
            match = 0;
        ++g_checks;
        if (!match) {
            char message[224];
            sprintf(message,
                    "upload %lu differs: native rect=%u,%u,%u,%u @%d:%ld/%lu source rect=%u,%u,%u,%u @%d:%ld/%lu",
                    index,
                    native_record->rectangle[0], native_record->rectangle[1],
                    native_record->rectangle[2], native_record->rectangle[3],
                    native_record->region, native_record->offset,
                    native_record->bytes,
                    source_record->rectangle[0], source_record->rectangle[1],
                    source_record->rectangle[2], source_record->rectangle[3],
                    source_record->region, source_record->offset,
                    source_record->bytes);
            fail(case_name, message);
            equal = 0;
        }
    }
    return equal;
}

static void prepare_candidate(void)
{
    bind_interface(&g_engine_interface, &g_candidate_context);
}

static void prepare_oracle(void)
{
    bind_oracle_interface();
}

static unsigned char g_native_frame_current[FRAME_BYTES];
static unsigned char g_native_frame_base[FRAME_BYTES];
static unsigned char g_source_frame_current[FRAME_BYTES];
static unsigned char g_source_frame_base[FRAME_BYTES];
static unsigned char g_native_pool[4096];
static unsigned char g_source_pool[4096];
static unsigned char g_native_palette[0x4000];
static unsigned char g_source_palette[0x4000];
static unsigned char g_native_updates[0x8000];
static unsigned char g_source_updates[0x8000];
static unsigned char g_native_descriptors[2][128];
static unsigned char g_source_descriptors[2][128];
static unsigned char g_native_table[8];
static unsigned char g_source_table[8];

static void configure_frame_globals(int same_frame, int level)
{
    unsigned char *native_base = same_frame ? g_native_frame_current :
                                               g_native_frame_base;
    unsigned char *source_base = same_frame ? g_source_frame_current :
                                               g_source_frame_base;

    *(unsigned char **)((unsigned char *)g_oracle + 0x71ec4) =
        g_native_frame_current;
    *(unsigned char **)((unsigned char *)g_oracle + 0x71ec8) = native_base;
    g_dispatch_frame_current_71ec4 = g_source_frame_current;
    g_dispatch_frame_base_71ec8 = source_base;
    *(void **)((unsigned char *)g_oracle + 0x70c80) = g_native_pool;
    g_frame_owner_70c80 = g_source_pool;
    g_oracle_context.unknown_000[0] = (unsigned char)level;
    g_candidate_context.unknown_000[0] = (unsigned char)level;
    *(unsigned long *)((unsigned char *)g_oracle + 0x73484) = 0;
    g_frame_global_73484 = 0;
    fill_bytes(g_native_frame_current, FRAME_BYTES, 0xa5);
    fill_bytes(g_source_frame_current, FRAME_BYTES, 0xa5);
    fill_bytes(g_native_frame_base, FRAME_BYTES, 0xa5);
    fill_bytes(g_source_frame_base, FRAME_BYTES, 0xa5);
    reset_spies();
    set_region(0, 0, g_native_frame_current, FRAME_BYTES);
    set_region(1, 0, g_source_frame_current, FRAME_BYTES);
    prepare_oracle();
    prepare_candidate();
}

static void compare_ot_words(const char *case_name,
                             const unsigned char *native_frame,
                             const unsigned char *source_frame)
{
    unsigned long index;
    int matched = 1;

    for (index = 0; index < OT_WORDS; ++index) {
        unsigned long native_value =
            *(const unsigned long *)(native_frame + OT_OFFSET + index * 4UL);
        unsigned long source_value =
            *(const unsigned long *)(source_frame + OT_OFFSET + index * 4UL);
        unsigned long native_normalized;
        unsigned long source_normalized;

        native_normalized = native_value == 0 ? 0 :
            native_value - pointer_word(native_frame);
        source_normalized = source_value == 0 ? 0 :
            source_value - pointer_word(source_frame);
        if (native_normalized != source_normalized ||
            native_normalized != (index + 1 < OT_WORDS ?
                OT_OFFSET + (index + 1) * 4UL : 0))
            matched = 0;
    }
    check(matched, case_name, "full OT pointer/terminator sequence differs");
    check(memcmp(native_frame, source_frame, OT_OFFSET) == 0,
          case_name, "frame bytes before OT were changed differently");
    check(memcmp(native_frame + OT_OFFSET + OT_BYTES,
                 source_frame + OT_OFFSET + OT_BYTES,
                 FRAME_BYTES - OT_OFFSET - OT_BYTES) == 0,
          case_name, "frame bytes after OT were changed differently");
}

static void run_05600_case(const char *case_name, int same_frame, int level)
{
    unsigned long native_cursor;
    unsigned long source_cursor;
    unsigned long expected_offset;
    unsigned short primitive_index;
    unsigned char *native_config;
    unsigned char *source_config;
    int equal;

    configure_frame_globals(same_frame, level);
    native_config = (unsigned char *)g_oracle + 0x5df08 + level * 28 + 6;
    source_config = g_frame_config_5df08 + level * 28 + 6;
    primitive_index = *(unsigned short *)native_config;
    check(primitive_index == *(unsigned short *)source_config,
          case_name, "test config rows are not equivalent");

    g_active_side = 0;
    ((Eng1Command)((unsigned char *)g_oracle + ORACLE_05600))();
    g_active_side = 1;
    dispatch_local_05600();

    equal = compare_spies(case_name);
    check(g_spy[0].chain_calls == 1 && g_spy[0].chain_count == 0x500UL &&
          g_spy[0].chain_region == 0 && g_spy[0].chain_offset == OT_OFFSET,
          case_name, "native chain callback contract differs");
    check(g_spy[1].chain_calls == 1 && g_spy[1].chain_count == 0x500UL &&
          g_spy[1].chain_region == 0 && g_spy[1].chain_offset == OT_OFFSET,
          case_name, "source chain callback contract differs");
    compare_ot_words(case_name, g_native_frame_current,
                     g_source_frame_current);
    if (!same_frame)
        check(memcmp(g_native_frame_base, g_source_frame_base,
                     FRAME_BYTES) == 0,
              case_name, "separate base frame was modified");
    native_cursor = *(unsigned long *)((unsigned char *)g_oracle + 0x73484);
    source_cursor = g_frame_global_73484;
    expected_offset = same_frame ? (unsigned long)primitive_index * 40UL : 0;
    check(native_cursor - pointer_word(g_native_pool) == expected_offset,
          case_name, "native primitive cursor disagrees with observed contract");
    check(source_cursor - pointer_word(g_source_pool) == expected_offset,
          case_name, "source primitive cursor disagrees with observed contract");
    check(native_cursor - pointer_word(g_native_pool) ==
          source_cursor - pointer_word(g_source_pool),
          case_name, "primitive cursor relative offset differs");
    if (!equal)
        printf("  chain calls native/source: %lu/%lu\n",
               g_spy[0].chain_calls, g_spy[1].chain_calls);
    printf("PASS %s\n", case_name);
}

static void run_058b0_case(const char *case_name, unsigned char dirty)
{
    unsigned char native_after;
    unsigned char source_after;

    fill_bytes(g_native_palette, sizeof(g_native_palette), 0x31);
    fill_bytes(g_source_palette, sizeof(g_source_palette), 0x31);
    *(void **)((unsigned char *)g_oracle + 0x7348c) = g_native_palette;
    *(unsigned char *)((unsigned char *)g_oracle + 0x73498) = dirty;
    g_frame_owner_7348c = g_source_palette;
    g_frame_global_73498 = dirty;
    reset_spies();
    set_region(0, 0, g_native_palette, sizeof(g_native_palette));
    set_region(1, 0, g_source_palette, sizeof(g_source_palette));
    prepare_oracle();
    prepare_candidate();

    g_active_side = 0;
    ((Eng1Command)((unsigned char *)g_oracle + ORACLE_058B0))();
    g_active_side = 1;
    dispatch_local_058b0();
    compare_spies(case_name);
    check(g_spy[0].upload_count == (dirty != 0 ? 1UL : 0UL),
          case_name, "native upload count differs from dirty state");
    check(g_spy[1].upload_count == (dirty != 0 ? 1UL : 0UL),
          case_name, "source upload count differs from dirty state");
    native_after = *(unsigned char *)((unsigned char *)g_oracle + 0x73498);
    source_after = g_frame_global_73498;
    check(native_after == source_after, case_name, "dirty flag differs after call");
    check(native_after == 0, case_name, "native dirty flag final value is unexpected");
    check(memcmp(g_native_palette, g_source_palette,
                 sizeof(g_native_palette)) == 0,
          case_name, "palette upload source buffer changed");
    printf("PASS %s\n", case_name);
}

static void write_descriptor(unsigned char *storage, unsigned short frames,
                             unsigned short period, unsigned short x,
                             unsigned short y, unsigned short width,
                             unsigned short height, unsigned char seed)
{
    unsigned char *descriptor = storage + 32;
    unsigned long index;

    memset(storage, 0xee, 128);
    *(unsigned short *)(descriptor + 0) = frames;
    *(unsigned short *)(descriptor + 2) = period;
    *(unsigned short *)(descriptor + 4) = x;
    *(unsigned short *)(descriptor + 6) = y;
    *(unsigned short *)(descriptor + 8) = width;
    *(unsigned short *)(descriptor + 10) = height;
    for (index = 0; index < 116; ++index)
        descriptor[12 + index] = (unsigned char)(seed + index * 13UL);
}

static void configure_descriptor_table(unsigned char count,
                                       unsigned long tick,
                                       unsigned short frame_count0,
                                       unsigned short period0,
                                       unsigned short frame_count1,
                                       unsigned short period1)
{
    unsigned char *native0 = g_native_descriptors[0] + 32;
    unsigned char *native1 = g_native_descriptors[1] + 32;
    unsigned char *source0 = g_source_descriptors[0] + 32;
    unsigned char *source1 = g_source_descriptors[1] + 32;

    write_descriptor(g_native_descriptors[0], frame_count0, period0,
                     0x31, 0x42, 2, 2, 0x10);
    write_descriptor(g_source_descriptors[0], frame_count0, period0,
                     0x31, 0x42, 2, 2, 0x10);
    write_descriptor(g_native_descriptors[1], frame_count1, period1,
                     0x51, 0x62, 1, 2, 0x80);
    write_descriptor(g_source_descriptors[1], frame_count1, period1,
                     0x51, 0x62, 1, 2, 0x80);
    memset(g_native_table, 0, sizeof(g_native_table));
    memset(g_source_table, 0, sizeof(g_source_table));
    *(unsigned long *)(g_native_table + 0) =
        pointer_word(native0) - pointer_word(g_native_table + 0);
    *(unsigned long *)(g_native_table + 4) =
        pointer_word(native1) - pointer_word(g_native_table + 4);
    *(unsigned long *)(g_source_table + 0) =
        pointer_word(source0) - pointer_word(g_source_table + 0);
    *(unsigned long *)(g_source_table + 4) =
        pointer_word(source1) - pointer_word(g_source_table + 4);
    *(unsigned char **)((unsigned char *)g_oracle + 0x70d38) =
        g_native_table;
    *(unsigned char *)((unsigned char *)g_oracle + 0x70ed1) = count;
    *(unsigned char **)((unsigned char *)g_oracle + 0x70f24) =
        (unsigned char *)&g_oracle_context;
    g_header_pointer_70d38 = g_source_table;
    g_root_count_70ed1 = count;
    g_candidate_context.tick_038 = tick;
    g_oracle_context.tick_038 = tick;
}

static void run_02cb0_case(const char *case_name, unsigned char count,
                           unsigned long tick, unsigned short frames0,
                           unsigned short period0, unsigned short frames1,
                           unsigned short period1,
                           unsigned long expected_uploads,
                           int expect_exception)
{
    unsigned long native_exception = 0;
    unsigned long source_exception = 0;
    int native_safe = 1;
    int source_safe = 1;

    configure_descriptor_table(count, tick, frames0, period0,
                               frames1, period1);
    reset_spies();
    set_region(0, 0, g_native_descriptors, sizeof(g_native_descriptors));
    set_region(1, 0, g_source_descriptors, sizeof(g_source_descriptors));
    prepare_oracle();
    prepare_candidate();

    g_active_side = 0;
    __try {
        ((Eng1Command)((unsigned char *)g_oracle + ORACLE_02CB0))();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        native_exception = GetExceptionCode();
        native_safe = 0;
    }
    g_active_side = 1;
    __try {
        dispatch_local_02cb0();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        source_exception = GetExceptionCode();
        source_safe = 0;
    }
    compare_spies(case_name);
    check(g_spy[0].upload_count == expected_uploads,
          case_name, "native upload count differs from fixture expectation");
    check(g_spy[1].upload_count == expected_uploads,
          case_name, "source upload count differs from fixture expectation");
    check(native_safe == source_safe, case_name, "exception vs return differs");
    if (!native_safe || !source_safe) {
        check(native_exception == source_exception,
              case_name, "exception code differs");
        if (expect_exception)
            check(native_exception == EXCEPTION_INT_DIVIDE_BY_ZERO,
                  case_name, "native exception is not integer divide-by-zero");
    } else if (expect_exception) {
        fail(case_name, "expected native integer divide-by-zero was not raised");
    }
    check(*(unsigned char *)((unsigned char *)g_oracle + 0x70ed1) == count,
          case_name, "native signed count byte changed");
    check(g_root_count_70ed1 == count,
          case_name, "source signed count byte changed");
    printf("PASS %s\n", case_name);
}

static void run_050d0_case(const char *case_name,
                           const unsigned short *coordinates,
                           unsigned long count,
                           unsigned long expected_uploads)
{
    unsigned long index;
    unsigned long failures_before = g_failures;

    fill_bytes(g_native_updates, sizeof(g_native_updates), 0x51);
    fill_bytes(g_source_updates, sizeof(g_source_updates), 0x51);
    for (index = 0; index < count; ++index) {
        g_frame_upload_coordinates_734a0[index] = coordinates[index];
        *(unsigned short *)((unsigned char *)g_oracle + 0x734a0 +
                            index * 2UL) = coordinates[index];
    }
    *(unsigned long *)((unsigned char *)g_oracle + 0x6ee7c) = count;
    *(void **)((unsigned char *)g_oracle + 0x72870) = g_native_updates;
    g_frame_upload_pending_6ee7c = count;
    g_frame_owner_72870 = g_source_updates;
    reset_spies();
    set_region(0, 0, g_native_updates, sizeof(g_native_updates));
    set_region(1, 0, g_source_updates, sizeof(g_source_updates));
    prepare_oracle();
    prepare_candidate();

    g_active_side = 0;
    ((Eng1Command)((unsigned char *)g_oracle + ORACLE_050D0))();
    g_active_side = 1;
    dispatch_local_050d0();
    compare_spies(case_name);
    check(g_spy[0].upload_count == expected_uploads,
          case_name, "native update upload count differs from fixture expectation");
    check(g_spy[1].upload_count == expected_uploads,
          case_name, "source update upload count differs from fixture expectation");
    check(*(unsigned long *)((unsigned char *)g_oracle + 0x6ee7c) == 0,
          case_name, "native pending DWORD was not drained");
    check(g_frame_upload_pending_6ee7c == 0,
          case_name, "source pending DWORD was not drained");
    check(memcmp(g_native_updates, g_source_updates,
                 sizeof(g_native_updates)) == 0,
          case_name, "update source buffers differ after calls");
    for (index = 0; index < count; ++index)
        check(*(unsigned short *)((unsigned char *)g_oracle + 0x734a0 +
                                  index * 2UL) ==
              g_frame_upload_coordinates_734a0[index],
              case_name, "native coordinate input changed");
    if (g_failures == failures_before)
        printf("PASS %s\n", case_name);
}

static int load_oracle(const char *path)
{
    g_oracle = LoadLibraryExA(path, 0, DONT_RESOLVE_DLL_REFERENCES);
    if (g_oracle == 0) {
        printf("Could not map ENG1.DLL: GetLastError=%lu\n", GetLastError());
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    static const unsigned short one_coordinate[] = { 0x0032 };
    static const unsigned short two_row_runs[] = {
        0x0011, 0x0012, 0x0051, 0x0052, 0x0053
    };
    unsigned char *native_configuration;

    if (argc != 2) {
        printf("usage: differential_fixture <path-to-ENG1.DLL>\n");
        return 2;
    }
    if (!load_oracle(argv[1]))
        return 3;

    native_configuration = (unsigned char *)g_oracle + 0x5df08;
    memcpy(g_frame_config_5df08, native_configuration,
           sizeof(g_frame_config_5df08));

    run_05600_case("05600/current-is-base/level-0", 1, 0);
    run_05600_case("05600/current-is-base/level-3", 1, 3);
    run_05600_case("05600/current-differs-from-base", 0, 0);

    run_058b0_case("058b0/dirty-upload", 1);
    run_058b0_case("058b0/clean-no-upload", 0);

    run_02cb0_case("02cb0/two-periodic-uploads", 2, 4, 3, 2, 2, 4, 2, 0);
    run_02cb0_case("02cb0/signed-negative-tick", 1, 0xfffffffCUL,
                   3, 2, 2, 0, 1, 0);
    run_02cb0_case("02cb0/negative-nondivisible-tick", 1, 0xffffffffUL,
                   3, 2, 2, 0, 0, 0);
    run_02cb0_case("02cb0/signed-negative-count", 0xff, 4,
                   3, 2, 2, 4, 0, 0);
    run_02cb0_case("02cb0/signed-negative-count-min", 0x80, 4,
                   3, 2, 2, 4, 0, 0);
    run_02cb0_case("02cb0/zero-period-skips", 1, 0,
                   0, 0, 0, 0, 0, 0);
    run_02cb0_case("02cb0/zero-frame-count-exception", 1, 0,
                   0, 1, 0, 0, 0, 1);

    run_050d0_case("050d0/no-pending", one_coordinate, 0, 0);
    run_050d0_case("050d0/one-tile", one_coordinate, 1, 1);
    run_050d0_case("050d0/two-row-runs", two_row_runs, 5, 2);

    if (g_oracle != 0)
        FreeLibrary(g_oracle);
    printf("checks=%lu failures=%lu\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
