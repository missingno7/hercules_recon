#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "../src/eng1/behavioral/simulation_dependency.c"
#include "../calibration/resource_context.h"
#include "../src/eng1/behavioral/read_guard.h"

#define SIM_CONTEXT_BYTES 0x1b34
#define SIM_POOL_BYTES 0x600
#define SIM_ROWS 8
#define SIM_ROW_BASE 0x128UL
#define SIM_ROW_STRIDE 0x94UL
#define SIM_RESOURCE_ROWS 81
#define SIM_RESOURCE_BASE 0x5cee0UL

typedef void (__cdecl *NoArg)(void);

typedef struct SimSnapshot {
    sim_u32 rva;
    sim_u32 size;
    unsigned char *initial;
    unsigned char *expected;
} SimSnapshot;

typedef struct SimCase {
    const char *name;
    sim_u32 mode;
    sim_u16 timer;
    sim_u32 selector;
    sim_u16 resource_state;
    sim_u16 context_mask;
    int allow_source_cancel;
    int seed_special_row;
    int expected_source_interpreters;
} SimCase;

extern ActorResource g_actor_resources[SIM_RESOURCE_ROWS];
unsigned long g_configuration_dword_71384;

static unsigned char context_memory[SIM_CONTEXT_BYTES];
static unsigned char alternate_memory[SIM_POOL_BYTES];
static unsigned short source_resource_states[SIM_RESOURCE_ROWS];
static unsigned char *oracle_base;
static HMODULE oracle_module;
static SimSnapshot snapshots[16];
static int snapshot_count;
static int failures;
static int total_checks;
static int source_cancel_allowed;
static int unexpected_helpers;
static int unexpected_interpreter_helpers;
static int guard_reads;
static int guard_failures;
static const void *guarded_program;
static int interface_cancel_calls;
static int interface_cancel_violations;
ResourceCallbackContext *g_resource_context;

extern int __cdecl engine_file_cancel(void);

/* resource_callbacks.c's host_cancel_load alias is the existing 0x2c260
 * engine-file bridge, which reaches the copied interface cancel slot. */
void __cdecl host_cancel_load(void)
{
    if (!source_cancel_allowed)
        ++unexpected_helpers;
    (void)engine_file_cancel();
}

void __cdecl resource_destroy(int resource_id)
{
    (void)resource_id;
    ++unexpected_helpers;
}

static unsigned char *rva_bytes(sim_u32 rva)
{
    return oracle_base + rva;
}

static sim_u32 *rva_u32(sim_u32 rva)
{
    return (sim_u32 *)rva_bytes(rva);
}

static sim_u16 *rva_u16(sim_u32 rva)
{
    return (sim_u16 *)rva_bytes(rva);
}

static int __cdecl fixture_cancel_callback(void)
{
    ++interface_cancel_calls;
    if (*(sim_u32 *)(context_memory + 0xb0) != 0 ||
        *(sim_u32 *)(context_memory + 0xb4) != 0 ||
        *rva_u16(SIM_RESOURCE_BASE + 7UL * 20UL + 0x10UL) != 2)
        ++interface_cancel_violations;
    return 0;
}

static void check(int condition, const char *message, const char *scenario)
{
    ++total_checks;
    if (!condition) {
        ++failures;
        printf("FAIL scenario=%s %s\n", scenario, message);
    }
}

static int take_snapshot(void)
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *section;
    int i;

    dos = (IMAGE_DOS_HEADER *)oracle_base;
    nt = (IMAGE_NT_HEADERS *)rva_bytes((sim_u32)dos->e_lfanew);
    section = IMAGE_FIRST_SECTION(nt);
    for (i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if ((section[i].Characteristics & IMAGE_SCN_MEM_WRITE) == 0)
            continue;
        if (snapshot_count == 16)
            return 0;
        snapshots[snapshot_count].rva = section[i].VirtualAddress;
        snapshots[snapshot_count].size = section[i].Misc.VirtualSize;
        snapshots[snapshot_count].initial = (unsigned char *)HeapAlloc(
            GetProcessHeap(), 0, snapshots[snapshot_count].size);
        snapshots[snapshot_count].expected = (unsigned char *)HeapAlloc(
            GetProcessHeap(), 0, snapshots[snapshot_count].size);
        if (!snapshots[snapshot_count].initial ||
            !snapshots[snapshot_count].expected)
            return 0;
        memcpy(snapshots[snapshot_count].initial,
            rva_bytes(snapshots[snapshot_count].rva),
            snapshots[snapshot_count].size);
        ++snapshot_count;
    }
    return snapshot_count != 0;
}

static void restore_image(void)
{
    int i;
    for (i = 0; i < snapshot_count; ++i)
        memcpy(rva_bytes(snapshots[i].rva), snapshots[i].initial,
            snapshots[i].size);
}

static void save_expected_image(void)
{
    int i;
    for (i = 0; i < snapshot_count; ++i)
        memcpy(snapshots[i].expected, rva_bytes(snapshots[i].rva),
            snapshots[i].size);
}

static int compare_expected_image(void)
{
    int i;
    for (i = 0; i < snapshot_count; ++i) {
        if (memcmp(snapshots[i].expected, rva_bytes(snapshots[i].rva),
                snapshots[i].size) != 0) {
            unsigned int j;
            for (j = 0; j < snapshots[i].size; ++j) {
                if (snapshots[i].expected[j] !=
                    rva_bytes(snapshots[i].rva)[j]) {
                    printf("image-diff rva=%08lx expected=%02x actual=%02x\n",
                        (unsigned long)(snapshots[i].rva + j),
                        snapshots[i].expected[j],
                        rva_bytes(snapshots[i].rva)[j]);
                    break;
                }
            }
            return 0;
        }
    }
    return 1;
}

static void capture_source_resource_states(void)
{
    int i;
    for (i = 0; i < SIM_RESOURCE_ROWS; ++i)
        source_resource_states[i] = g_actor_resources[i].residency_state;
}

static void reset_source_resource_states(void)
{
    int i;
    for (i = 0; i < SIM_RESOURCE_ROWS; ++i)
        g_actor_resources[i].residency_state = source_resource_states[i];
}

static void set_resource_state(sim_u32 index, sim_u16 value)
{
    g_actor_resources[index].residency_state = value;
    *rva_u16(SIM_RESOURCE_BASE + index * 20UL + 0x10UL) = value;
}

static void reset_resource_state_in_image(void)
{
    int i;
    for (i = 0; i < SIM_RESOURCE_ROWS; ++i)
        *rva_u16(SIM_RESOURCE_BASE + (sim_u32)i * 20UL + 0x10UL) =
            source_resource_states[i];
}

static unsigned char *actor_row(unsigned char *pool, int index)
{
    return pool + SIM_ROW_BASE + (sim_u32)index * SIM_ROW_STRIDE;
}

/* Native pointers are VAs in the loaded DLL while the candidate's ordinary-C
 * table relocates to its own source-owned script bytes. Normalize only those
 * observed pointer identities to their shared original RVA for comparison. */
static int program_pointer_rva(sim_u32 pointer, sim_u32 *rva)
{
    sim_u32 native_base;
    sim_u32 source_base;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    sim_u32 image_size;

    if (pointer == 0) {
        *rva = 0;
        return 1;
    }
    native_base = (sim_u32)oracle_base;
    dos = (IMAGE_DOS_HEADER *)oracle_base;
    nt = (IMAGE_NT_HEADERS *)rva_bytes((sim_u32)dos->e_lfanew);
    image_size = nt->OptionalHeader.SizeOfImage;
    if (pointer >= native_base && pointer - native_base < image_size) {
        *rva = pointer - native_base;
        return 1;
    }
    source_base = (sim_u32)g_eng1_simulation_data_bytes_53620;
    if (pointer >= source_base &&
        pointer - source_base < sizeof(g_eng1_simulation_data_bytes_53620)) {
        *rva = 0x53620UL + (pointer - source_base);
        return 1;
    }
    return 0;
}

static int compare_actor_pool_normalized(const unsigned char *native_pool,
    const unsigned char *source_pool, const char *scenario)
{
    unsigned int offset;

    for (offset = 0; offset < SIM_POOL_BYTES; ++offset) {
        int row;
        int is_program_pointer;
        row = -1;
        is_program_pointer = 0;
        if (offset >= SIM_ROW_BASE) {
            unsigned int relative;
            relative = offset - SIM_ROW_BASE;
            row = (int)(relative / SIM_ROW_STRIDE);
            if (row < SIM_ROWS &&
                (relative % SIM_ROW_STRIDE) >= 0x64 &&
                (relative % SIM_ROW_STRIDE) < 0x68)
                is_program_pointer = 1;
        }
        if (is_program_pointer) {
            sim_u32 native_pointer;
            sim_u32 source_pointer;
            sim_u32 native_rva;
            sim_u32 source_rva;
            unsigned int row_base;
            row_base = (unsigned int)SIM_ROW_BASE +
                (unsigned int)row * SIM_ROW_STRIDE;
            native_pointer = *(const sim_u32 *)(native_pool + row_base + 0x64);
            source_pointer = *(const sim_u32 *)(source_pool + row_base + 0x64);
            if (!program_pointer_rva(native_pointer, &native_rva) ||
                !program_pointer_rva(source_pointer, &source_rva) ||
                native_rva != source_rva) {
                printf("pointer-identity-diff scenario=%s row=%d native=%08lx source=%08lx\n",
                    scenario, row, (unsigned long)native_pointer,
                    (unsigned long)source_pointer);
                return 0;
            }
            offset = row_base + 0x67;
            continue;
        }
        if (native_pool[offset] != source_pool[offset]) {
            printf("pool-diff scenario=%s offset=%04x native=%02x source=%02x\n",
                scenario, offset, native_pool[offset], source_pool[offset]);
            return 0;
        }
    }
    return 1;
}

static void verify_source_data_views(void)
{
    sim_u32 i;
    for (i = 0; i < 8; ++i) {
        sim_u32 source_pointer;
        sim_u32 native_pointer;
        sim_u32 source_rva;
        sim_u32 native_rva;
        source_pointer = ENG1_NATIVE_DATA_TARGET_00537E0_DWORD(i);
        native_pointer = *rva_u32(0x537e0UL + i * 4UL);
        check(program_pointer_rva(source_pointer, &source_rva) &&
              program_pointer_rva(native_pointer, &native_rva) &&
              source_rva == native_rva,
            "source pointer-prefix relocation names the same native RVA",
            "provider-ownership");
        if (program_pointer_rva(source_pointer, &source_rva) &&
            program_pointer_rva(native_pointer, &native_rva) &&
            source_rva == native_rva)
            check(memcmp((const void *)source_pointer,
                    rva_bytes(native_rva), 0x40) == 0,
                "source byte block matches its native RVA block",
                "provider-ownership");
    }
    check(eng1_native_data_target_0052b70[0].value ==
              *rva_u16(0x52b70UL) &&
          eng1_native_data_target_0052b70[7].value ==
              *rva_u16(0x52b70UL + 7UL * 4UL),
        "ordinary-C WORD-slot owner matches native first/last consumed cells",
        "provider-ownership");
}

static void seed_memory(const SimCase *test_case)
{
    int i;
    unsigned char *actor;
    sim_u32 position;
    sim_u32 target;

    memset(context_memory, 0, sizeof(context_memory));
    memset(alternate_memory, 0xa5, sizeof(alternate_memory));
    *(sim_u16 *)(context_memory + 0x1c) = test_case->context_mask;
    for (i = 0; i < SIM_ROWS; ++i) {
        actor = actor_row(alternate_memory, i);
        position = (i & 1) ? 0xfff90000UL : 0x00100000UL;
        target = (i & 1) ? 0x00010000UL : 0;
        *(sim_u32 *)(actor + 4) = position;
        *(sim_u16 *)(actor + 0x2a) = 0;
        *(sim_u16 *)(actor + 0x30) = 0;
        *(sim_u16 *)(actor + 0x32) = 0;
        *(sim_u32 *)(actor + 0x4c) = target;
        *(sim_u32 *)(actor + 0x54) = 0x12340005UL + (sim_u32)i;
        *(sim_u8 *)(actor + 0x21) = 0;
        *(sim_u8 *)(actor + 0x22) = 0;
        *(sim_u32 *)(actor + 0x64) = 0;
    }

    if (test_case->seed_special_row) {
        actor = actor_row(alternate_memory, 1);
        *(sim_u16 *)(actor + 0x2a) = 0;
        *(sim_u32 *)(actor + 0x64) =
            ENG1_NATIVE_DATA_TARGET_00537E0_DWORD(1UL);
        *(sim_u8 *)(actor + 0x21) = 0;
        *(sim_u8 *)(actor + 0x22) = 2;
    }
    if (test_case->mode == 5 && test_case->seed_special_row &&
        test_case->resource_state == 2) {
        *(sim_u32 *)(context_memory + 0xb0) = 0x12345678UL;
        *(sim_u32 *)(context_memory + 0xb4) = 0x87654321UL;
    }

    g_engine_interface.context_004 =
        (ResourceCallbackContext *)context_memory;
    g_engine_interface.cancel_load_024 = fixture_cancel_callback;
    g_resource_context = (ResourceCallbackContext *)context_memory;
    g_alternate_actor_pool = alternate_memory;
    eng1_sim_state_72174 = test_case->mode;
    eng1_sim_state_72104 = test_case->selector;
    eng1_sim_state_6ef40 = test_case->timer;
    reset_source_resource_states();
    set_resource_state(7, test_case->resource_state);

    *rva_u32(0x70f24) = (sim_u32)context_memory;
    *rva_u32(0x70f44) = (sim_u32)fixture_cancel_callback;
    *rva_u32(0x734e0) = (sim_u32)alternate_memory;
    *rva_u32(0x72174) = test_case->mode;
    *rva_u32(0x72104) = test_case->selector;
    *rva_u16(0x6ef40) = test_case->timer;
}

static void sync_source_state_to_image(void)
{
    *rva_u32(0x72174) = eng1_sim_state_72174;
    *rva_u32(0x72104) = eng1_sim_state_72104;
    *rva_u16(0x6ef40) = eng1_sim_state_6ef40;
}

static void sync_source_resource_state_to_image(void)
{
    sim_u32 i;
    for (i = 0; i < SIM_RESOURCE_ROWS; ++i)
        *rva_u16(SIM_RESOURCE_BASE + i * 20UL + 0x10UL) =
            g_actor_resources[i].residency_state;
}

int __cdecl remove_actor(Actor *actor)
{
    (void)actor;
    ++unexpected_interpreter_helpers;
    return 0;
}

void __cdecl remove_actor_preserving_resource(Actor *actor)
{
    (void)actor;
    ++unexpected_interpreter_helpers;
}

void __cdecl mark_actor_resource_for_release(Actor *actor)
{
    (void)actor;
    ++unexpected_interpreter_helpers;
}

void __cdecl spawn_actor_at_group(long x, long y, long z,
    unsigned short resource, unsigned short group)
{
    (void)x; (void)y; (void)z; (void)resource; (void)group;
    ++unexpected_interpreter_helpers;
}

void __cdecl play_sound(unsigned short sound, Actor *actor)
{
    (void)sound; (void)actor;
    ++unexpected_interpreter_helpers;
}

void __cdecl stop_actor_sounds(Actor *actor)
{
    (void)actor;
    ++unexpected_interpreter_helpers;
}

/* The custom test program is not one of the registered global programs. The
 * existing source guard therefore allows these proven in-range words; this
 * provider additionally makes any unexpected interpreter read fail visible. */
void eng1_guard_program_read(const void *program, long index,
    unsigned int stride, unsigned int width, const char *site)
{
    (void)site;
    ++guard_reads;
    if (program != guarded_program || index < 0 || index > 3 ||
        stride != 2 || width != 2)
        ++guard_failures;
}

void eng1_guard_frame_slot(const void *frames, long index, const char *site)
{
    (void)frames; (void)index; (void)site;
    ++unexpected_interpreter_helpers;
}

void __cdecl eng1_guarded_advance_frame(Actor *actor)
{
    (void)actor;
    ++unexpected_interpreter_helpers;
}

void __cdecl eng1_guarded_select_frame(Actor *actor, int index)
{
    (void)actor; (void)index;
    ++unexpected_interpreter_helpers;
}

void __cdecl spawn_actor_with_owner(Actor *actor, long x, long y, long z,
    unsigned short resource)
{
    (void)actor; (void)x; (void)y; (void)z; (void)resource;
    ++unexpected_interpreter_helpers;
}

int __cdecl wrapped_ratio(int numerator, int denominator, int divisor)
{
    (void)numerator; (void)denominator; (void)divisor;
    ++unexpected_interpreter_helpers;
    return 0;
}

unsigned int __cdecl dispatch_engine_2c460(int value)
{
    (void)value;
    ++unexpected_interpreter_helpers;
    return 0;
}

static void reset_trace(void)
{
    unexpected_helpers = 0;
    unexpected_interpreter_helpers = 0;
    guard_reads = 0;
    guard_failures = 0;
    interface_cancel_calls = 0;
    interface_cancel_violations = 0;
}

static void run_case(const SimCase *test_case)
{
    NoArg native_target;
    unsigned char native_context[SIM_CONTEXT_BYTES];
    unsigned char native_pool[SIM_POOL_BYTES];
    int i;
    int native_cancel_calls;
    int expected_cancel_calls;

    restore_image();
    reset_source_resource_states();
    reset_resource_state_in_image();
    seed_memory(test_case);
    source_cancel_allowed = 0;
    reset_trace();
    native_target = (NoArg)rva_bytes(0x12620);
    native_target();
    native_cancel_calls = interface_cancel_calls;
    save_expected_image();
    memcpy(native_context, context_memory, sizeof(native_context));
    memcpy(native_pool, alternate_memory, sizeof(native_pool));

    restore_image();
    reset_source_resource_states();
    reset_resource_state_in_image();
    seed_memory(test_case);
    source_cancel_allowed = test_case->allow_source_cancel;
    reset_trace();
    dispatch_local_12620();
    sync_source_state_to_image();
    sync_source_resource_state_to_image();

    check(compare_expected_image(),
        "all writable native module bytes match", test_case->name);
    check(memcmp(native_context, context_memory, sizeof(native_context)) == 0,
        "complete observed context prefix matches native", test_case->name);
    check(compare_actor_pool_normalized(native_pool, alternate_memory,
            test_case->name),
        "alternate actor bytes match after program-pointer RVA normalization",
        test_case->name);
    for (i = 0; i < SIM_RESOURCE_ROWS; ++i) {
        check(g_actor_resources[i].residency_state ==
                *rva_u16(SIM_RESOURCE_BASE + (sim_u32)i * 20UL + 0x10UL),
            "existing resource-row state matches native image",
            test_case->name);
    }
    if (test_case->expected_source_interpreters != 0)
        check(guard_reads >= 4,
            "source word interpreter read the provider-backed script",
            test_case->name);
    else
        check(guard_reads == 0,
            "source word interpreter was not reached on this row path",
            test_case->name);
    check(unexpected_helpers == 0,
        "no unresolved helper was reached in this test path",
        test_case->name);
    check(unexpected_interpreter_helpers == 0,
        "source interpreter stayed within selected positive-opcode path",
        test_case->name);
    check(guard_failures == 0,
        "source interpreter guarded only valid test program words",
        test_case->name);
    expected_cancel_calls =
        (test_case->mode == 5 && test_case->seed_special_row &&
         test_case->resource_state == 2) ? 1 : 0;
    check(native_cancel_calls == expected_cancel_calls,
        "native cancel bridge forwards through the copied interface slot once",
        test_case->name);
    check(interface_cancel_calls == native_cancel_calls,
        "source cancel bridge forwards through the same copied interface slot",
        test_case->name);
    check(interface_cancel_violations == 0,
        "cancel endpoint sees cleared callbacks before residency state changes",
        test_case->name);
    printf("case=%s source_word_reads=%d\n",
        test_case->name, guard_reads);
}

static jmp_buf prefix_guard_jump;
static int prefix_guard_armed;
static unsigned long prefix_guard_rva;

void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    if (prefix_guard_armed) {
        prefix_guard_rva = rva;
        longjmp(prefix_guard_jump, 1);
    }
    fprintf(stderr, "unexpected simulation prefix failure %08lx: %s\n", rva, reason);
    ExitProcess(97);
}

static int verify_prefix_guards(void)
{
    static unsigned char row[0x94], expected[0x94];
    static unsigned char resources_before[sizeof(g_actor_resources)];
    unsigned long saved_id, saved_selector;
    int case_index, bad = 0;
    int calls, words;

    saved_id = eng1_native_data_target_0053800[0];
    saved_selector = eng1_sim_state_72104;
    for (case_index = 0; case_index < 2; ++case_index) {
        memset(row, 0xa5, sizeof(row));
        *(sim_u16 *)(row + 0x2a) = 2;
        memcpy(expected, row, sizeof(row));
        *(sim_u16 *)(expected + 0x2a) = 1;
        memcpy(resources_before, g_actor_resources, sizeof(g_actor_resources));
        calls = unexpected_helpers + unexpected_interpreter_helpers + interface_cancel_calls;
        words = guard_reads;
        eng1_sim_state_72104 = case_index ? 0 : 64;
        if (case_index)
            eng1_native_data_target_0053800[0] = 81;
        prefix_guard_rva = 0;
        prefix_guard_armed = 1;
        if (setjmp(prefix_guard_jump) == 0) {
            (void)dispatch_local_12790(row, eng1_sim_state_72104);
            ++bad; /* Unknown operation must never report successful return. */
        }
        prefix_guard_armed = 0;
        if (prefix_guard_rva != (case_index ? 0x5cee0UL : 0x53800UL) ||
            memcmp(row, expected, sizeof(row)) != 0 ||
            memcmp(resources_before, g_actor_resources, sizeof(g_actor_resources)) != 0 ||
            calls != unexpected_helpers + unexpected_interpreter_helpers + interface_cancel_calls ||
            words != guard_reads)
            ++bad;
        eng1_native_data_target_0053800[0] = saved_id;
    }
    eng1_sim_state_72104 = saved_selector;
    printf("simulation_source_guard_cases=2 failures=%d\n", bad);
    return bad;
}

int main(int argc, char **argv)
{
    static const SimCase cases[] = {
        { "mode0-clear-return", 0, 0, 8, 0, 0x0055, 0, 0, 0 },
        { "mode1-source-nested", 1, 7, 1, 0, 0x0055, 0, 0, 1 },
        { "mode2-signed-positive", 2, 0x8000, 8, 0, 0x00aa, 0, 0, 0 },
        { "mode2-zero-wrap", 2, 0, 8, 0, 0x00aa, 0, 0, 0 },
        { "mode3-initialize", 3, 0xffff, 8, 0, 0x00ff, 0, 0, 0 },
        { "mode4-zero-expiry", 4, 1, 8, 0, 0x0033, 0, 0, 0 },
        { "mode4-negative-wrap", 4, 0x8001, 8, 0, 0x0033, 0, 0, 0 },
        { "mode5-common", 5, 0, 8, 0, 0x0066, 0, 0, 0 },
        { "unsigned-default", 0xffffffffUL, 0x100, 8, 0, 0x0099, 0, 0, 0 },
        { "source-mode1-reset-zero", 1, 0x008c, 1, 0, 0x0055, 0, 0, 1 },
        { "source-mode1-reset-state2", 1, 0x008c, 1, 2, 0x0055, 0, 0, 1 },
        { "source-mode1-reset-state3", 1, 0x008c, 1, 3, 0x0055, 0, 0, 1 },
        { "source-row-lifecycle-state1", 5, 0, 1, 1, 0x0055, 0, 1, 1 },
        { "source-row-lifecycle-state2", 5, 0, 1, 2, 0x0055, 1, 1, 1 },
        { "source-row-lifecycle-state3", 5, 0, 1, 3, 0x0055, 0, 1, 1 }
    };
    int i;

    setvbuf(stdout, 0, _IONBF, 0);
    if (argc != 2)
        return 2;
    SetUnhandledExceptionFilter(0);
    oracle_module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (!oracle_module)
        return 3;
    oracle_base = (unsigned char *)oracle_module;
    if (!take_snapshot())
        return 4;

    capture_source_resource_states();
    guarded_program = &g_eng1_simulation_data_bytes_53620[0x40];
    verify_source_data_views();

    for (i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); ++i)
        run_case(&cases[i]);

    failures += verify_prefix_guards();

    printf("simulation_dependency_cases=%d checks=%d failures=%d\n",
        (int)(sizeof(cases) / sizeof(cases[0])),
        total_checks,
        failures);
    return failures ? 1 : 0;
}
