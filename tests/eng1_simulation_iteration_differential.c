#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/simulation_iteration.c"

#define CONTEXT_BYTES 0x1b34
#define NORMAL_BYTES 0x200
#define ALTERNATE_BYTES 0x600
#define AUX_BYTES 0x200

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef void (__cdecl *NoArg)(void);

EngineInterface g_engine_interface;
void *g_normal_actor_pool;

static union { u32 align; u8 bytes[CONTEXT_BYTES]; } context_memory;
static union { u32 align; u8 bytes[NORMAL_BYTES]; } normal_memory;
static union { u32 align; u8 bytes[ALTERNATE_BYTES]; } alternate_memory;
static union { u32 align; u8 bytes[AUX_BYTES]; } auxiliary_memory;
static union { u32 align; u8 bytes[0x40]; } state_72020_memory;
static u16 zeroed_motion_words[128];

typedef struct ImageSnapshot {
    u32 rva;
    u32 size;
    u8 *initial;
    u8 *expected;
} ImageSnapshot;

static HMODULE oracle_module;
static u8 *oracle_base;
static ImageSnapshot snapshots[16];
static int snapshot_count;
static int failures;
static int call_order[4];
static int call_count;

static LONG WINAPI fixture_exception_filter(EXCEPTION_POINTERS *information)
{
    printf("fixture:exception code=%08lx eip=%08lx address=%p\n",
        information->ExceptionRecord->ExceptionCode,
        information->ContextRecord->Eip,
        information->ExceptionRecord->ExceptionAddress);
    fflush(stdout);
    return EXCEPTION_EXECUTE_HANDLER;
}

static u8 *rva_bytes(u32 rva)
{
    return oracle_base + rva;
}

static u32 *rva_u32(u32 rva)
{
    return (u32 *)rva_bytes(rva);
}

static u16 *rva_u16(u32 rva)
{
    return (u16 *)rva_bytes(rva);
}

static void check(int condition, const char *message, int scenario)
{
    if (!condition) {
        ++failures;
        printf("FAIL scenario=%d %s\n", scenario, message);
    }
}

static int take_snapshot(void)
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *section;
    int i;

    dos = (IMAGE_DOS_HEADER *)oracle_base;
    nt = (IMAGE_NT_HEADERS *)rva_bytes((u32)dos->e_lfanew);
    section = IMAGE_FIRST_SECTION(nt);
    for (i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if ((section[i].Characteristics & IMAGE_SCN_MEM_WRITE) == 0)
            continue;
        if (snapshot_count == 16)
            return 0;
        snapshots[snapshot_count].rva = section[i].VirtualAddress;
        snapshots[snapshot_count].size = section[i].Misc.VirtualSize;
        snapshots[snapshot_count].initial = (u8 *)HeapAlloc(
            GetProcessHeap(), 0, snapshots[snapshot_count].size);
        snapshots[snapshot_count].expected = (u8 *)HeapAlloc(
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
                snapshots[i].size) != 0)
            return 0;
    }
    return 1;
}

static void seed_context(u8 control, u32 flags, u16 state_word,
    u8 state_byte, u8 state_10, u32 actor_position)
{
    memset(context_memory.bytes, 0, sizeof(context_memory.bytes));
    memset(normal_memory.bytes, 0, sizeof(normal_memory.bytes));
    memset(alternate_memory.bytes, 0, sizeof(alternate_memory.bytes));
    memset(auxiliary_memory.bytes, 0, sizeof(auxiliary_memory.bytes));
    memset(state_72020_memory.bytes, 0, sizeof(state_72020_memory.bytes));
    memset(zeroed_motion_words, 0, sizeof(zeroed_motion_words));

    context_memory.bytes[0] = state_byte;
    *(u32 *)(context_memory.bytes + 0x30) = flags;
    context_memory.bytes[0x65] = control;
    *(u16 *)(context_memory.bytes + 0x5c) = 0xa303U;
    context_memory.bytes[0x10] = state_10;
    *(u32 *)(normal_memory.bytes + 8) = actor_position;

    g_engine_interface.context_004 =
        (ResourceCallbackContext *)context_memory.bytes;
    g_normal_actor_pool = normal_memory.bytes;

    *rva_u32(0x70f24) = (u32)context_memory.bytes;
    *rva_u32(0x72860) = (u32)normal_memory.bytes;
    *rva_u32(0x734e0) = (u32)alternate_memory.bytes;
    *rva_u32(0x7217c) = (u32)auxiliary_memory.bytes;
    *rva_u32(0x72020) = (u32)state_72020_memory.bytes;
    *rva_u32(0x713bc) = (u32)zeroed_motion_words;
    *rva_u32(0x72110) = 0;
    *rva_u32(0x72170) = 0;
    *rva_u16(0x72108) = state_word;
    *rva_u16(0x6feb4) = 0x1234U;
    *rva_u32(0x72104) = 0xffffffffUL;
    *rva_u32(0x72174) = 5UL;
}

static void sync_source_to_oracle(void)
{
    *rva_u32(0x72110) = eng1_sim_state_72110;
    *rva_u32(0x72170) = eng1_sim_state_72170;
    *rva_u16(0x72108) = eng1_sim_state_72108;
    *rva_u16(0x6feb4) = eng1_sim_state_6feb4;
}

static void sync_oracle_to_source(void)
{
    eng1_sim_state_72110 = *rva_u32(0x72110);
    eng1_sim_state_72170 = *rva_u32(0x72170);
    eng1_sim_state_72108 = *rva_u16(0x72108);
    eng1_sim_state_6feb4 = *rva_u16(0x6feb4);
}

void __cdecl eng1_native_code_target_0015780(void)
{
    NoArg target = (NoArg)rva_bytes(0x15780);
    call_order[call_count++] = 0x15780;
    sync_source_to_oracle();
    target();
    sync_oracle_to_source();
}

void __cdecl eng1_native_code_target_0012620(void)
{
    NoArg target = (NoArg)rva_bytes(0x12620);
    call_order[call_count++] = 0x12620;
    sync_source_to_oracle();
    target();
    sync_oracle_to_source();
}

static int run_case(int scenario, u8 control, u32 flags, u16 state_word,
    u8 state_byte, u8 state_10, u32 actor_position)
{
    NoArg native_target;
    u8 native_context[CONTEXT_BYTES];
    u8 native_normal[NORMAL_BYTES];
    u8 native_alternate[ALTERNATE_BYTES];

    restore_image();
    seed_context(control, flags, state_word, state_byte, state_10, actor_position);
    native_target = (NoArg)rva_bytes(0x156a0);
    printf("fixture:case %d enter native156a0\n", scenario);
    fflush(stdout);
    native_target();
    printf("fixture:case %d leave native156a0\n", scenario);
    fflush(stdout);
    save_expected_image();
    memcpy(native_context, context_memory.bytes, sizeof(native_context));
    memcpy(native_normal, normal_memory.bytes, sizeof(native_normal));
    memcpy(native_alternate, alternate_memory.bytes, sizeof(native_alternate));

    restore_image();
    seed_context(control, flags, state_word, state_byte, state_10, actor_position);
    sync_oracle_to_source();
    call_count = 0;
    printf("fixture:case %d enter source156a0\n", scenario);
    fflush(stdout);
    dispatch_local_156a0();
    printf("fixture:case %d leave source156a0\n", scenario);
    fflush(stdout);
    sync_source_to_oracle();

    check(call_count == 2, "source made exactly two direct dependency calls", scenario);
    check(call_order[0] == 0x15780 && call_order[1] == 0x12620,
        "direct dependency order", scenario);
    check(compare_expected_image(), "all writable mapped module bytes match native", scenario);
    check(memcmp(native_context, context_memory.bytes, sizeof(native_context)) == 0,
        "complete observed context prefix matches native", scenario);
    check(memcmp(native_normal, normal_memory.bytes, sizeof(native_normal)) == 0,
        "guarded normal-pool access window matches native", scenario);
    check(memcmp(native_alternate, alternate_memory.bytes,
            sizeof(native_alternate)) == 0,
        "guarded alternate-pool access window matches native", scenario);
    return failures == 0;
}

int main(int argc, char **argv)
{
    int i;
    struct Scenario {
        u8 control;
        u32 flags;
        u16 state_word;
        u8 state_byte;
        u8 state_10;
        u32 actor_position;
    } scenarios[] = {
        { 0x00U, 0x00008000UL, 0x0020U, 0x00U, 0x01U, 0x00000000UL },
        { 0x01U, 0x0000c000UL, 0x0020U, 0x00U, 0x00U, 0x007fffffUL },
        { 0x02U, 0x0000c000UL, 0x0020U, 0x00U, 0x01U, 0x01345678UL },
        { 0x04U, 0x0000c000UL, 0x0020U, 0x00U, 0x01U, 0xff7fffffUL },
        { 0x08U, 0x0000a000UL, 0x0020U, 0x00U, 0x01U, 0x7fffffffUL },
        { 0x0fU, 0x0000e000UL, 0x0019U, 0x09U, 0x01U, 0x80800000UL }
    };

    if (argc != 2)
        return 2;
    SetUnhandledExceptionFilter(fixture_exception_filter);
    printf("fixture:load-oracle\n");
    fflush(stdout);
    oracle_module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (!oracle_module)
        return 3;
    oracle_base = (u8 *)oracle_module;
    printf("fixture:loaded base=%p\n", oracle_module);
    fflush(stdout);
    if (!take_snapshot())
        return 4;
    printf("fixture:snapshot sections=%d\n", snapshot_count);
    fflush(stdout);

    for (i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); ++i) {
        printf("fixture:case %d native\n", i);
        fflush(stdout);
        run_case(i, scenarios[i].control, scenarios[i].flags,
            scenarios[i].state_word, scenarios[i].state_byte,
            scenarios[i].state_10,
            scenarios[i].actor_position);
    }

    printf("simulation_cases=%d checks=%d failures=%d\n",
        (int)(sizeof(scenarios) / sizeof(scenarios[0])),
        (int)(sizeof(scenarios) / sizeof(scenarios[0])) * 6, failures);
    return failures ? 1 : 0;
}
