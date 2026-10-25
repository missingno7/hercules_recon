/*
 * Source-closure differential for native ENG1 RVA 0x13990.
 *
 * NATIVE enters the oracle DLL, whose original nested helpers run unchanged.
 * SOURCE enters the ordinary-C 0x13990 candidate, actual reset_frame_state,
 * actual eng1_interpret_word_stream, actual actor_update_2a290, and actual
 * grid_scan_20b50/query_decode_20990 sources with their existing data owners.
 * Only the interpreter's registered-read boundary and unrelated interpreter
 * service arms are test shims; the selected measured program never reaches
 * those service arms.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/shared/engine_interface.h"
#include "../src/eng1/interpreters.h"
#include "../src/eng1/grid_query_20b50.h"
#include "../src/eng1/incomplete.h"

typedef unsigned char fixture_u8;
typedef unsigned short fixture_u16;
typedef unsigned long fixture_u32;
typedef void (__cdecl *NativeActorCallback)(Actor *);

extern void __cdecl eng1_native_code_target_013990(Actor *actor);
extern void __cdecl eng1_native_code_target_002a290(Actor *actor);
extern fixture_u16 g_eng1_config_word_prefix_53370[44];
extern fixture_u16 g_eng1_config_word_pool_533c8[282];
extern signed char g_active_row_70d01;

static HMODULE native_module;
static unsigned int guard_calls;
static long guard_indices[2];
static unsigned int query_calls;
static unsigned int forbidden_service_calls;

static void fail_fixture(const char *message)
{
    fprintf(stderr, "fixture failure: %s\n", message);
    ExitProcess(0xe0484731UL);
}

static fixture_u8 *native_rva(fixture_u32 rva)
{
    return (fixture_u8 *)(void *)native_module + rva;
}

/* Exact-base, bounded private registry row for the 0x53370 source prefix.
 * The adjacent 0x533c8 owner is intentionally a separate row/owner. */
void eng1_guard_program_read(const void *program, long index,
                             unsigned int stride, unsigned int width,
                             const char *site)
{
    (void)site;
    if (guard_calls >= 2U)
        fail_fixture("interpreter read count exceeded the measured two-WORD path");
    guard_indices[guard_calls++] = index;
    if (program != (const void *)&g_eng1_config_word_prefix_53370[0] ||
        stride != 2U || width != 2U || index < 0 || index >= 44L)
        fail_fixture("interpreter read escaped the exact 0x53370 88-byte source prefix");
}

void eng1_guard_frame_slot(const void *frames, long index, const char *site)
{
    (void)frames; (void)index; (void)site;
    ++forbidden_service_calls;
    fail_fixture("unexpected frame-slot guard path");
}

void __cdecl eng1_guarded_advance_frame(Actor *actor)
{
    (void)actor; ++forbidden_service_calls;
    fail_fixture("unexpected guarded advance-frame service");
}

void __cdecl eng1_guarded_select_frame(Actor *actor, int index)
{
    (void)actor; (void)index; ++forbidden_service_calls;
    fail_fixture("unexpected guarded select-frame service");
}

/* The exact source grid scan is the 0x2a290 dependency in this fixture. */
long __cdecl eng1_native_dep_020b50(long x, long y, long z, int parameter)
{
    ++query_calls;
    return (long)grid_scan_20b50(x, y, z, (fixture_u32)parameter);
}

/* Explicit fail-fast shims for interpreter arms absent from the selected
 * measured program. They are link support, not production implementations. */
int __cdecl remove_actor(Actor *actor)
{ (void)actor; ++forbidden_service_calls; fail_fixture("unexpected remove_actor"); return 0; }
int __cdecl wrapped_ratio(int left, int right, int divisor)
{ (void)left; (void)right; (void)divisor; ++forbidden_service_calls; fail_fixture("unexpected wrapped_ratio"); return 0; }
void __cdecl play_sound(unsigned short effect_id, Actor *actor)
{ (void)effect_id; (void)actor; ++forbidden_service_calls; fail_fixture("unexpected play_sound"); }
unsigned int __cdecl dispatch_engine_2c460(int selector)
{ (void)selector; ++forbidden_service_calls; fail_fixture("unexpected dispatch_engine_2c460"); return 0; }
unsigned long g_configuration_dword_71384;
void __cdecl remove_actor_preserving_resource(Actor *actor)
{ (void)actor; ++forbidden_service_calls; fail_fixture("unexpected remove_actor_preserving_resource"); }
void __cdecl mark_actor_resource_for_release(Actor *actor)
{ (void)actor; ++forbidden_service_calls; fail_fixture("unexpected mark_actor_resource_for_release"); }
void __cdecl spawn_actor_with_owner(Actor *actor, long x, long y, long z,
                                    unsigned short kind)
{ (void)actor; (void)x; (void)y; (void)z; (void)kind; ++forbidden_service_calls; fail_fixture("unexpected spawn_actor_with_owner"); }
void __cdecl spawn_actor_at_group(long x, long y, long z,
                                  unsigned short kind, unsigned short group)
{ (void)x; (void)y; (void)z; (void)kind; (void)group; ++forbidden_service_calls; fail_fixture("unexpected spawn_actor_at_group"); }
void __cdecl stop_actor_sounds(Actor *actor)
{ (void)actor; ++forbidden_service_calls; fail_fixture("unexpected stop_actor_sounds"); }
void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{ (void)rva; (void)reason; fail_fixture("source path reached an explicit unsupported grid branch"); }

static void initialize_actor(fixture_u8 *actor, fixture_u16 kind)
{
    fixture_u32 i;
    for (i = 0; i < 0x134UL; ++i)
        actor[i] = (fixture_u8)(0x37U + i * 13UL + (i >> 2));

    *(fixture_u32 *)(void *)(actor + 0x00) = 0x00a00000UL;
    *(fixture_u32 *)(void *)(actor + 0x04) = 0x00800000UL;
    *(fixture_u32 *)(void *)(actor + 0x08) = 0x01400000UL;
    *(fixture_u16 *)(void *)(actor + 0x2e) = kind;
    *(short *)(void *)(actor + 0x30) = 17;
    *(short *)(void *)(actor + 0x32) = 9;
    *(fixture_u16 *)(void *)(actor + 0x38) = 0x0100U;
    *(fixture_u32 *)(void *)(actor + 0x54) = 0;
}

static void print_actor(const fixture_u8 *actor)
{
    fixture_u32 i;
    for (i = 0; i < 0x134UL; ++i)
        printf("%02x", (unsigned int)actor[i]);
}

int main(int argc, char **argv)
{
    fixture_u8 actor[0x134];
    fixture_u8 context[0x40];
    fixture_u16 kind;
    fixture_u8 selector_byte;
    fixture_u32 program_tag;
    fixture_u32 program_pointer;
    const char *mode;

    if (argc != 5) {
        fprintf(stderr, "usage: neutralfixture.exe ENG1.DLL native|source kind context_byte2\n");
        return 2;
    }
    native_module = LoadLibraryA(argv[1]);
    if (native_module == 0) {
        fprintf(stderr, "LoadLibraryA failed: %lu\n", GetLastError());
        return 3;
    }
    mode = argv[2];
    kind = (fixture_u16)strtoul(argv[3], 0, 0);
    selector_byte = (fixture_u8)strtoul(argv[4], 0, 0);

    memset(context, 0, sizeof(context));
    context[2] = selector_byte;
    initialize_actor(actor, kind);
    guard_calls = query_calls = forbidden_service_calls = 0;
    guard_indices[0] = guard_indices[1] = -999L;

    if (strcmp(mode, "native") == 0) {
        NativeActorCallback callback;
        *(fixture_u8 *)(void *)native_rva(0x70d01UL) = 0xffU;
        *(fixture_u8 **)(void *)native_rva(0x70f24UL) = context;
        callback = (NativeActorCallback)(void *)native_rva(0x13990UL);
        callback((Actor *)(void *)actor);
        if (*(fixture_u32 *)(void *)(actor + 0x64) ==
            (fixture_u32)(unsigned long)native_module + 0x53370UL)
            program_tag = 0x53370UL;
        else if (*(fixture_u32 *)(void *)(actor + 0x64) ==
                 (fixture_u32)(unsigned long)native_module + 0x533c8UL)
            program_tag = 0x533c8UL;
        else
            program_tag = 0xffffffffUL;
    } else if (strcmp(mode, "source") == 0) {
        g_engine_interface.context_004 = (ResourceCallbackContext *)(void *)context;
        g_active_row_70d01 = (signed char)-1;
        eng1_native_code_target_013990((Actor *)(void *)actor);
        program_pointer = *(fixture_u32 *)(void *)(actor + 0x64);
        if (program_pointer == (fixture_u32)(unsigned long)&g_eng1_config_word_prefix_53370[0])
            program_tag = 0x53370UL;
        else if (program_pointer == (fixture_u32)(unsigned long)&g_eng1_config_word_pool_533c8[0])
            program_tag = 0x533c8UL;
        else
            program_tag = 0xffffffffUL;
        if (guard_calls != 2U)
            fail_fixture("expected actual interpreter to make exactly two guarded WORD reads");
        if (guard_indices[0] != 1L || guard_indices[1] != 0L)
            fail_fixture("actual interpreter read order was not opcode index 1 then default-duration index 0");
        if (query_calls != 1U)
            fail_fixture("expected actual actor update to execute the source grid query once");
        if (forbidden_service_calls != 0U)
            fail_fixture("an unreachable interpreter service spy was entered");
        if (*(short *)(void *)(actor + 0x30) != 0 ||
            *(short *)(void *)(actor + 0x32) != 1 ||
            *(short *)(void *)(actor + 0x34) != (short)0x00e8 ||
            *(fixture_u32 *)(void *)(actor + 0x4c) != 1UL ||
            *(fixture_u32 *)(void *)(actor + 0x50) != 0x802UL)
            fail_fixture("source reset/interpreter/update field effects did not match the measured path");
    } else {
        fprintf(stderr, "unknown mode\n");
        return 2;
    }

    printf("mode=%s guard=%u query=%u forbidden=%u program_tag=%08lx actor=",
           mode, guard_calls, query_calls, forbidden_service_calls, program_tag);
    print_actor(actor);
    printf("\n");
    return 0;
}
