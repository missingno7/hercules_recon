#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../calibration/engine_interface.h"
#include "../src/eng1/behavioral/setup_shared_state.h"
#include "../src/eng1/behavioral/simulation_data_owners.h"
#include "../src/eng1/behavioral/script_54558_owner.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef u32 (__cdecl *NativeContinuation)(void);

extern void *g_normal_actor_pool;
extern u8 *g_collision_context;
extern unsigned short eng1_sim_state_72108;
extern u32 __cdecl dispatch_local_15d00(void);

static HMODULE oracle;
static int cases, checks, failures;
static int event_count;
static int query_calls, random_calls;
static int allow_query_rng_spy;
static u32 query_args[4];
static void *query_actor_replacement;
static void *random_actor_replacement;

static u8 *native_cell(u32 rva)
{ return (u8 *)oracle + rva; }

static void check(int ok, const char *label)
{
    ++checks;
    if (!ok) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

static void unexpected_fixture_service(const char *name)
{
    printf("unexpected candidate fixture service: %s\n", name);
    ExitProcess(0xe0484732UL);
}

static void fill(u8 *p, u32 n, u8 seed)
{
    u32 i;
    for (i = 0; i < n; ++i)
        p[i] = (u8)(seed + i * 13UL + (i >> 2));
}

static void set_record(u8 *p, u16 opcode, u16 operand)
{
    *(u16 *)(void *)p = opcode;
    *(u16 *)(void *)(p + 2) = operand;
}

/* Fixture input mutation is restricted to this measured writable .data pair.
 * Refuse executable/read-only pages; never change code or page permissions. */
static u8 *native_fixture_record(u32 rva)
{
    MEMORY_BASIC_INFORMATION info;
    u8 *p = native_cell(rva);
    DWORD protection;
    if (rva != 0x53830UL || VirtualQuery(p, &info, sizeof(info)) != sizeof(info))
        ExitProcess(101);
    protection = info.Protect & 0xff;
    if (protection != PAGE_READWRITE && protection != PAGE_WRITECOPY)
        ExitProcess(101);
    return p;
}
static void set_native_record(u32 rva, u16 opcode, u16 operand, u8 saved[4])
{
    u8 *p = native_fixture_record(rva);
    memcpy(saved, p, 4);
    set_record(p, opcode, operand);
}
static void restore_native_record(u32 rva, const u8 saved[4])
{ memcpy(native_fixture_record(rva), saved, 4); }

static u8 *source_program_storage(void)
{
    return (u8 *)(void *)g_eng1_simulation_lookup_owner_537e0.value_view_53800;
}

static void normalize_pointer(u8 *bytes, u32 offset, u32 value)
{ *(u32 *)(void *)(bytes + offset) = value; }

u32 __cdecl eng1_native_code_target_0030010(u32 x, u32 y, u32 z, u32 flags)
{
    if (!allow_query_rng_spy)
        unexpected_fixture_service("0x30010 query");
    ++query_calls;
    query_args[0] = x; query_args[1] = y;
    query_args[2] = z; query_args[3] = flags;
    ++event_count;
    if (query_actor_replacement != 0)
        g_normal_actor_pool = query_actor_replacement;
    return 0;
}
void *__cdecl eng1_native_dep_0300c0(u32 x, u32 y, u32 z, u32 operand, u32 zero)
{ (void)x; (void)y; (void)z; (void)operand; (void)zero;
  unexpected_fixture_service("0x300c0 actor operation"); return 0; }
void __cdecl spawn_actor_with_owner(void *p, long x, long y, long z, unsigned short operand)
{ (void)p; (void)x; (void)y; (void)z; (void)operand;
  unexpected_fixture_service("0x30180 actor operation"); }
void __cdecl eng1_native_code_target_001a220(void)
{ unexpected_fixture_service("0x1a220"); }
void __cdecl eng1_native_code_target_001a280(void)
{ unexpected_fixture_service("0x1a280"); }
void __cdecl eng1_native_code_target_001a3e0(void)
{ unexpected_fixture_service("0x1a3e0"); }
void __cdecl eng1_native_code_target_001a410(void)
{ unexpected_fixture_service("0x1a410"); }
void __cdecl eng1_native_code_target_0007f8e(int id, void *p)
{ (void)id; (void)p; unexpected_fixture_service("0x7f8e event"); }

unsigned int __cdecl dispatch_engine_2c460(int limit)
{
    if (!allow_query_rng_spy)
        unexpected_fixture_service("2c460 RNG");
    (void)limit;
    ++random_calls;
    ++event_count;
    if (random_actor_replacement != 0)
        g_normal_actor_pool = random_actor_replacement;
    return 2U;
}
void __cdecl dispatch_engine_2c4e0(unsigned long value)
{ (void)value; unexpected_fixture_service("2c4e0 callback"); }
void __cdecl play_sound(unsigned short id, void *p)
{ (void)id; (void)p; unexpected_fixture_service("play_sound"); }
void __cdecl dispatch_local_16790(void)
{ unexpected_fixture_service("0x16790 setup caller"); }
void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    printf("unexpected incomplete behavior at %08lx: %s\n", rva, reason);
    ExitProcess(0xe0484731UL);
}

static void normalize_program_pointer(u8 *actor, u32 expected_rva)
{ *(u32 *)(void *)(actor + 0x64) = expected_rva; }

static void differential_default_prefix(void)
{
    u8 native_actor[0x200], source_actor[0x200];
    u8 native_collision[0x100], source_collision[0x100];
    u8 native_context[0x200], source_context[0x200];
    u8 native_actor_after[0x200], source_actor_after[0x200];
    NativeContinuation native_fn;
    u32 native_result, source_result;

    fill(native_actor, sizeof(native_actor), 0x31);
    memcpy(source_actor, native_actor, sizeof(native_actor));
    fill(native_collision, sizeof(native_collision), 0x52);
    memcpy(source_collision, native_collision, sizeof(native_collision));
    fill(native_context, sizeof(native_context), 0x73);
    memcpy(source_context, native_context, sizeof(native_context));
    *(u32 *)(void *)(native_actor + 0x4c) = 1;
    *(u32 *)(void *)(source_actor + 0x4c) = 1;
    *(u16 *)(void *)(native_actor + 0x30) = 0xffffU;
    *(u16 *)(void *)(source_actor + 0x30) = 0xffffU;
    *(u16 *)(void *)(native_actor + 0x32) = 0xffffU;
    *(u16 *)(void *)(source_actor + 0x32) = 0xffffU;
    *(u32 *)(void *)(native_actor + 0x64) = (u32)(native_cell(0x53820));
    *(void **)(void *)(source_actor + 0x64) =
        (u8 *)&g_eng1_simulation_lookup_owner_537e0 + 0x40;

    *(void **)(void *)native_cell(0x72860) = native_actor;
    *(void **)(void *)native_cell(0x7217c) = native_collision;
    *(void **)(void *)native_cell(0x70f24) = native_context;
    g_normal_actor_pool = source_actor;
    g_collision_context = source_collision;
    g_engine_interface.context_004 = (ResourceCallbackContext *)(void *)source_context;
    *(u32 *)(void *)native_cell(0x7201c) = 0;
    g_eng1_setup_dword_7201c = 0;
    *(u16 *)(void *)native_cell(0x72108) = 0;
    eng1_sim_state_72108 = 0;

    native_fn = (NativeContinuation)(void *)native_cell(0x15d00);
    native_result = native_fn();
    source_result = dispatch_local_15d00();
    memcpy(native_actor_after, native_actor, sizeof(native_actor_after));
    memcpy(source_actor_after, source_actor, sizeof(source_actor_after));
    normalize_program_pointer(native_actor_after, 0x53830UL);
    normalize_program_pointer(source_actor_after, 0x53830UL);

    check(native_result == source_result && source_result == 0,
          "no-argument entry result for bounded default setup continuation");
    check(!memcmp(native_actor_after, source_actor_after, sizeof(native_actor_after)),
          "full normal actor bytes including timer/index and normalized program RVA");
    check(!memcmp(native_collision, source_collision, sizeof(native_collision)),
          "full collision actor bytes and opcode -13 context-byte write");
    check(!memcmp(native_context, source_context, sizeof(native_context)),
          "interface context remains untouched on this path");
    check(eng1_sim_state_72108 == 0 && *(u16 *)(void *)native_cell(0x72108) == 0,
          "first setup fixture state remains 0x72108 == 0");
    check(*(u16 *)(void *)(source_actor + 0x34) == 1 &&
          *(u16 *)(void *)(source_actor + 0x30) == 6 &&
          *(u16 *)(void *)(source_actor + 0x32) == 1,
          "53820 to 53830 transition reaches opcode 1 operand 6");
    check(source_collision[0x22] == 4,
          "second script's opcode -13 overwrites collision context byte +0x22");
    ++cases;
}

static void source_spy_callback_order(void)
{
    u8 actor[0x200], after_query[0x200], after_random[0x200];
    u8 collision[0x100], context[0x200];
    u32 result;
    fill(actor, sizeof(actor), 0x19);
    fill(after_query, sizeof(after_query), 0x29);
    fill(after_random, sizeof(after_random), 0x39);
    *(u32 *)(void *)(after_query + 0x64) = 0;
    fill(collision, sizeof(collision), 0x2a);
    fill(context, sizeof(context), 0x3b);
    *(u16 *)(void *)(actor + 0x30) = 0xffffU;
    *(u16 *)(void *)(actor + 0x32) = 0xffffU;
    *(u32 *)(void *)(actor + 0x4c) = 0;
    actor[0x94] = 0;
    *(void **)(void *)(actor + 0x64) =
        (u8 *)&g_eng1_simulation_lookup_owner_537e0 + 0x40;
    g_normal_actor_pool = actor;
    g_collision_context = collision;
    g_engine_interface.context_004 = (ResourceCallbackContext *)(void *)context;
    eng1_sim_state_72108 = 0;
    g_eng1_setup_dword_7201c = 0;
    query_calls = random_calls = event_count = 0;
    query_actor_replacement = after_query;
    random_actor_replacement = after_random;
    allow_query_rng_spy = 1;
    result = dispatch_local_15d00();
    allow_query_rng_spy = 0;
    query_actor_replacement = 0;
    random_actor_replacement = 0;

    check(result == 0, "source-spy callback path returns ordinary 0");
    check(query_calls == 1 && random_calls == 1 && event_count == 2,
          "query service then one RNG callback");
    check(query_args[0] == (*(u32 *)(void *)actor + 0x380000UL) &&
          query_args[1] == (*(u32 *)(void *)(actor + 4) - 0x10000UL) &&
          query_args[2] == *(u32 *)(void *)(collision + 8) && query_args[3] == 1,
          "0x30010 physical four-DWORD arguments including modular coordinate math");
    check(*(u32 *)(void *)(after_random + 0x64) == (u32)(void *)
          ((u8 *)&g_eng1_simulation_lookup_owner_537e0 + 0x98),
          "post-RNG live actor receives source-backed 0x53878 program");
    check(*(u16 *)(void *)(after_random + 0x34) == 0x110 &&
          *(u16 *)(void *)(after_random + 0x30) == 2 &&
          *(u16 *)(void *)(after_random + 0x32) == 1,
          "selected 0x53878 executes -13 then opcode 0x110 operand 2");
    check(*(u32 *)(void *)(actor + 0x64) == (u32)(void *)
          ((u8 *)&g_eng1_simulation_lookup_owner_537e0 + 0x40) &&
          *(u16 *)(void *)(actor + 0x32) == 1 &&
          *(u32 *)(void *)(after_query + 0x64) == 0,
          "actor pointer is reloaded after query and RNG callbacks");
    check(collision[0x22] == 1,
          "first 0x53820 command stores byte 1 before the query callback");
    ++cases;
}

static void differential_alternate_program_prefix(void)
{
    u8 native_actor[0x200], source_actor[0x200];
    u8 native_actor_after[0x200], source_actor_after[0x200];
    u8 *native_program = native_cell(0x54558UL + 21UL * 4UL);
    u8 *source_program = (u8 *)(void *)(g_eng1_setup_program_54558 + 21);
    NativeContinuation native_fn;
    u32 native_result, source_result;

    fill(native_actor, sizeof(native_actor), 0x41);
    memcpy(source_actor, native_actor, sizeof(native_actor));
    *(u16 *)(void *)(native_actor + 0x30) = 0xffffU;
    *(u16 *)(void *)(source_actor + 0x30) = 0xffffU;
    *(u16 *)(void *)(native_actor + 0x32) = 0xffffU;
    *(u16 *)(void *)(source_actor + 0x32) = 0xffffU;
    *(u32 *)(void *)(native_actor + 0x64) = (u32)(void *)native_program;
    *(void **)(void *)(source_actor + 0x64) = source_program;
    *(void **)(void *)native_cell(0x72860) = native_actor;
    g_normal_actor_pool = source_actor;
    *(u32 *)(void *)native_cell(0x6ef88) = 1UL;
    g_eng1_setup_dword_6ef88 = 1UL;

    native_fn = (NativeContinuation)(void *)native_cell(0x15d00);
    native_result = native_fn();
    source_result = dispatch_local_15d00();
    check(*(u32 *)(void *)(native_actor + 0x64) ==
              (u32)(void *)native_program &&
          *(u32 *)(void *)(source_actor + 0x64) ==
              (u32)(void *)source_program,
          "alternate script pointers remain at measured record-21 RVA");
    memcpy(native_actor_after, native_actor, sizeof(native_actor_after));
    memcpy(source_actor_after, source_actor, sizeof(source_actor_after));
    normalize_program_pointer(native_actor_after, 0x545acUL);
    normalize_program_pointer(source_actor_after, 0x545acUL);

    check(native_result == source_result && source_result == 0,
          "alternate bounded stream returns ordinary 0");
    check(!memcmp(native_actor_after, source_actor_after,
                  sizeof(native_actor_after)),
          "alternate stream -29/-49 dispatch actor state");
    check(*(u16 *)(void *)native_cell(0x6ef98) == g_eng1_setup_word_6ef98 &&
          g_eng1_setup_word_6ef98 == 45,
          "alternate stream writes measured global WORD through -49");
    check(*(u16 *)(void *)(source_actor + 0x34) == 48 &&
          *(u16 *)(void *)(source_actor + 0x30) == 2 &&
          *(u16 *)(void *)(source_actor + 0x32) == 2,
          "alternate stream positive command and following operand");
    ++cases;
}

static void differential_minus_45_collision_byte_alias(void)
{
    u8 native_actor[0x200], source_actor[0x200];
    u8 native_collision[0x200], source_collision[0x200];
    u8 native_actor_after[0x200], source_actor_after[0x200];
    u8 native_collision_after[0x200], source_collision_after[0x200];
    u8 native_saved[4], source_saved[4];
    u8 *source_program = source_program_storage();
    u8 *source_record = source_program + 0x30;
    u8 *native_child, *source_child;
    NativeContinuation native_fn;
    u32 native_result, source_result;
    u32 initial_global = 0x12345678UL;

    fill(native_actor, sizeof(native_actor), 0x31);
    memcpy(source_actor, native_actor, sizeof(native_actor));
    fill(native_collision, sizeof(native_collision), 0x52);
    memcpy(source_collision, native_collision, sizeof(native_collision));
    *(u16 *)(void *)(native_actor + 0x30) = 0;
    *(u16 *)(void *)(source_actor + 0x30) = 0;
    *(u16 *)(void *)(native_actor + 0x32) = 0xffffU;
    *(u16 *)(void *)(source_actor + 0x32) = 0xffffU;
    native_actor[0x94] = source_actor[0x94] = 0;
    native_child = native_collision + 0x72;
    source_child = source_collision + 0x72;
    *(void **)(void *)(native_collision + 0x60) = native_child;
    *(void **)(void *)(source_collision + 0x60) = source_child;
    native_collision[0x94] = source_collision[0x94] = 0x10;
    *(u32 *)(void *)native_cell(0x72860) = (u32)(void *)native_actor;
    *(u32 *)(void *)native_cell(0x7217c) = (u32)(void *)native_collision;
    *(u32 *)(void *)native_cell(0x6efa0) = initial_global;
    g_normal_actor_pool = source_actor;
    g_collision_context = source_collision;
    g_eng1_setup_dword_6efa0 = initial_global;
    *(void **)(void *)(native_actor + 0x64) = native_cell(0x53830);
    *(void **)(void *)(source_actor + 0x64) = source_program + 0x30;
    memcpy(source_saved, source_record, sizeof(source_saved));
    set_native_record(0x53830UL, (u16)-45, 0x1234U, native_saved);
    set_record(source_record, (u16)-45, 0x1234U);

    native_fn = (NativeContinuation)(void *)native_cell(0x15d00);
    native_result = native_fn();
    source_result = dispatch_local_15d00();
    memcpy(native_actor_after, native_actor, sizeof(native_actor_after));
    memcpy(source_actor_after, source_actor, sizeof(source_actor_after));
    memcpy(native_collision_after, native_collision, sizeof(native_collision_after));
    memcpy(source_collision_after, source_collision, sizeof(source_collision_after));
    normalize_program_pointer(native_actor_after, 0x53830UL);
    normalize_program_pointer(source_actor_after, 0x53830UL);
    normalize_pointer(native_collision_after, 0x60, 0xfacef00dUL);
    normalize_pointer(source_collision_after, 0x60, 0xfacef00dUL);

    check(native_result == source_result && source_result == 0,
          "-45 alias case returns the same ordinary result");
    check(!memcmp(native_actor_after, source_actor_after, sizeof(native_actor_after)),
          "-45 alias case preserves matching actor writes and next command");
    check(!memcmp(native_collision_after, source_collision_after,
                  sizeof(native_collision_after)),
          "-45 alias case matches child/collision bytes when child+22 aliases collision+94");
    check(*(u32 *)(void *)native_cell(0x6efa0) == g_eng1_setup_dword_6efa0 &&
          g_eng1_setup_dword_6efa0 == 5,
          "-45 alias case writes the measured global dword value 5");
    check(native_actor[0x94] == 0 && native_collision[0x94] == 1,
          "-45 test distinguishes normal actor+94 from the aliased live collision byte");
    check((*(u32 *)(void *)(source_child + 0x54) & 0x10UL) != 0,
          "-45 child flags use collision+94 bit 0x10 read before aliased child+22 store");
    memcpy(source_record, source_saved, sizeof(source_saved));
    restore_native_record(0x53830UL, native_saved);
    ++cases;
}

static void differential_collision_program_operand(
    u16 opcode, u32 initial_flags, u32 expected_flags, const char *name)
{
    u8 native_actor[0x200], source_actor[0x200];
    u8 native_collision[0x200], source_collision[0x200];
    u8 native_actor_after[0x200], source_actor_after[0x200];
    u8 native_collision_after[0x200], source_collision_after[0x200];
    u8 native_saved[4], source_saved[4];
    u8 *source_program = source_program_storage();
    u8 *source_record = source_program + 0x30;
    NativeContinuation native_fn;
    u32 native_result, source_result;

    fill(native_actor, sizeof(native_actor), 0x41);
    memcpy(source_actor, native_actor, sizeof(native_actor));
    fill(native_collision, sizeof(native_collision), 0x62);
    memcpy(source_collision, native_collision, sizeof(native_collision));
    *(u16 *)(void *)(native_actor + 0x30) = 0;
    *(u16 *)(void *)(source_actor + 0x30) = 0;
    *(u16 *)(void *)(native_actor + 0x32) = 0xffffU;
    *(u16 *)(void *)(source_actor + 0x32) = 0xffffU;
    *(u16 *)(void *)(native_collision + 0x32) = 0;
    *(u16 *)(void *)(source_collision + 0x32) = 0;
    *(u32 *)(void *)(native_collision + 0x54) = initial_flags;
    *(u32 *)(void *)(source_collision + 0x54) = initial_flags;
    *(void **)(void *)(native_actor + 0x64) = native_cell(0x53830);
    *(void **)(void *)(source_actor + 0x64) = source_program + 0x30;
    *(void **)(void *)(native_collision + 0x64) = native_cell(0x53878);
    *(void **)(void *)(source_collision + 0x64) = source_program + 0x78;
    *(u32 *)(void *)native_cell(0x72860) = (u32)(void *)native_actor;
    *(u32 *)(void *)native_cell(0x7217c) = (u32)(void *)native_collision;
    g_normal_actor_pool = source_actor;
    g_collision_context = source_collision;
    memcpy(source_saved, source_record, sizeof(source_saved));
    set_native_record(0x53830UL, opcode, 2U, native_saved);
    set_record(source_record, opcode, 2U);

    native_fn = (NativeContinuation)(void *)native_cell(0x15d00);
    native_result = native_fn();
    source_result = dispatch_local_15d00();
    memcpy(native_actor_after, native_actor, sizeof(native_actor_after));
    memcpy(source_actor_after, source_actor, sizeof(source_actor_after));
    memcpy(native_collision_after, native_collision, sizeof(native_collision_after));
    memcpy(source_collision_after, source_collision, sizeof(source_collision_after));
    normalize_program_pointer(native_actor_after, 0x53830UL);
    normalize_program_pointer(source_actor_after, 0x53830UL);
    normalize_program_pointer(native_collision_after, 0x53878UL);
    normalize_program_pointer(source_collision_after, 0x53878UL);

    check(native_result == source_result && source_result == 0,
          name);
    check(!memcmp(native_actor_after, source_actor_after, sizeof(native_actor_after)),
          "collision-program operand case matches actor bytes");
    check(!memcmp(native_collision_after, source_collision_after,
                  sizeof(native_collision_after)),
          "collision-program operand case matches collision bytes and flags");
    check(*(u32 *)(void *)(native_collision + 0x54) == expected_flags &&
          *(u32 *)(void *)(source_collision + 0x54) == expected_flags,
          "collision-program operand differs from normal program operand and selects native flag result");
    memcpy(source_record, source_saved, sizeof(source_saved));
    restore_native_record(0x53830UL, native_saved);
    ++cases;
}

static void timer_short_circuit(void)
{
    u8 native_actor[0x80], source_actor[0x80];
    u8 native_collision[0x40], source_collision[0x40];
    u8 native_context[0x80], source_context[0x80];
    NativeContinuation native_fn;
    u32 native_result, source_result;
    fill(native_actor, sizeof(native_actor), 0x91);
    memcpy(source_actor, native_actor, sizeof(native_actor));
    fill(native_collision, sizeof(native_collision), 0xa2);
    memcpy(source_collision, native_collision, sizeof(native_collision));
    fill(native_context, sizeof(native_context), 0xb3);
    memcpy(source_context, native_context, sizeof(native_context));
    *(u16 *)(void *)(native_actor + 0x30) = 1;
    *(u16 *)(void *)(source_actor + 0x30) = 1;
    *(void **)(void *)native_cell(0x72860) = native_actor;
    *(void **)(void *)native_cell(0x7217c) = native_collision;
    *(void **)(void *)native_cell(0x70f24) = native_context;
    g_normal_actor_pool = source_actor;
    g_collision_context = source_collision;
    g_engine_interface.context_004 =
        (ResourceCallbackContext *)(void *)source_context;

    native_fn = (NativeContinuation)(void *)native_cell(0x15d00);
    native_result = native_fn();
    source_result = dispatch_local_15d00();
    check(native_result == source_result && source_result == 0,
          "timer short-circuit returns native EAX 0");
    check(!memcmp(native_actor, source_actor, sizeof(native_actor)),
          "timer path performs only the same native WORD decrement");
    check(!memcmp(native_collision, source_collision,
                  sizeof(native_collision)) &&
          !memcmp(native_context, source_context, sizeof(native_context)),
          "timer path does not mutate collision/context data");
    ++cases;
}

int main(int argc, char **argv)
{
    int native_cases, native_checks;
    if (argc != 2) return 2;
    oracle = LoadLibraryExA(argv[1], NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!oracle) {
        printf("LoadLibraryExA failed %lu\n", GetLastError());
        return 3;
    }
    differential_default_prefix();
    differential_alternate_program_prefix();
    timer_short_circuit();
    differential_minus_45_collision_byte_alias();
    differential_collision_program_operand((u16)-19, 0UL, 1UL,
        "-19 uses the collision program operand");
    differential_collision_program_operand((u16)-20, 0UL, 0x00010000UL,
        "-20 uses the collision program operand");
    differential_collision_program_operand((u16)-21, 3UL, 2UL,
        "-21 uses the collision program operand");
    differential_collision_program_operand((u16)-22, 0x00030000UL, 0x00020000UL,
        "-22 uses the collision program operand");
    native_cases = cases; native_checks = checks;
    printf("player_setup_native_cases=%d checks=%d failures=%d\n",
           native_cases, native_checks, failures);
    source_spy_callback_order();
    printf("player_setup_source_spy_cases=%d checks=%d failures=%d\n",
           cases - native_cases, checks - native_checks, failures);
    FreeLibrary(oracle);
    return failures ? 1 : 0;
}
