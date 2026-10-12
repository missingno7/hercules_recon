#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/behavioral/query_2a170.h"
#include "../src/eng1/behavioral/resource_runtime_data.h"
#include "../src/eng1/behavioral/actor_bounds_2a880.h"
#include "../src/eng1/behavioral/extent_producer_30e40.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef int (__cdecl *ActorBounds2a880)(void *, Query2a170Bounds *, u32);
typedef int (__cdecl *ExtentProvider30e40)(void *, void *, u32);

static HMODULE oracle;
static unsigned int cases, checks, failures;
static u8 actor_bytes[256];
static u16 helper_extents[6];
static int helper_result;
static int helper_calls;
static int helper_mutates_actor;
static void *helper_actor_seen;
static u32 helper_mode_seen;
static int use_real_extent_provider;
static u8 controlled_metadata[4096];
extern ActorResource g_actor_resources[81];

void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    printf("UNEXPECTED incomplete path %08lx: %s\n", rva, reason);
    ExitProcess(97U);
}

int __cdecl ensure_actor_resource_ready(unsigned long resource_id,
                                        int metadata_index)
{
    printf("UNEXPECTED resource readiness call id=%lu metadata=%d\n",
           resource_id, metadata_index);
    ExitProcess(98U);
    return 0;
}

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

static u8 *native_cell(u32 rva)
{
    return (u8 *)oracle + rva;
}

static u16 *actor_word(void *actor, unsigned int offset)
{
    return (u16 *)(void *)((u8 *)actor + offset);
}

static u32 *actor_dword(void *actor, unsigned int offset)
{
    return (u32 *)(void *)((u8 *)actor + offset);
}

static void reset_fixture(void)
{
    memset(actor_bytes, 0, sizeof(actor_bytes));
    memset(helper_extents, 0, sizeof(helper_extents));
    helper_result = 0;
    helper_calls = 0;
    helper_mutates_actor = 0;
    helper_actor_seen = 0;
    helper_mode_seen = 0xffffffffUL;
    use_real_extent_provider = 0;
}

static void seed_actor(void *actor)
{
    *actor_dword(actor, 0x00) = 0xfffe8000UL;
    *actor_dword(actor, 0x04) = 0x00028000UL;
    *actor_dword(actor, 0x08) = 0x80018000UL;
    *(u8 *)((u8 *)actor + 0x54) = 0x40U;
}

static void set_test_extents(void)
{
    helper_extents[0] = 5;
    helper_extents[1] = 3;
    helper_extents[2] = 7;
    helper_extents[3] = 4;
    helper_extents[4] = 6;
    helper_extents[5] = 9;
}

int __cdecl eng1_fixture_0030e40(void *actor, void *scratch, u32 mode)
{
    volatile u16 *words = (volatile u16 *)scratch;
    unsigned int i;
    if (use_real_extent_provider)
        return extent_producer_30e40(actor, scratch, mode);
    ++helper_calls;
    helper_actor_seen = actor;
    helper_mode_seen = mode;
    for (i = 0; i < 6U; ++i)
        words[i] = helper_extents[i];
    if (helper_mutates_actor) {
        *actor_dword(actor, 0x00) = 0x00070000UL;
        *actor_dword(actor, 0x04) = 0xffff0000UL;
        *actor_dword(actor, 0x08) = 0x00008000UL;
        *(u8 *)((u8 *)actor + 0x54) = 0x10U;
    }
    return helper_result;
}

static u16 output_word(const Query2a170Bounds *bounds, unsigned int index)
{
    return ((const u16 *)(const void *)bounds)[index];
}

static void check_six_words(const Query2a170Bounds *bounds,
                            u16 a, u16 b, u16 c, u16 d, u16 e, u16 f,
                            const char *label)
{
    const u16 expected[6] = {a, b, c, d, e, f};
    unsigned int i;
    for (i = 0; i < 6U; ++i)
        check(output_word(bounds, i) == expected[i], label);
}

static void source_failure_and_horizontal_guard(void)
{
    Query2a170Bounds bounds;
    u8 before[12];
    int result;

    reset_fixture();
    seed_actor(actor_bytes);
    set_test_extents();
    helper_result = 0;
    memset(&bounds, 0xa5, sizeof(bounds));
    memcpy(before, &bounds, sizeof(before));
    result = actor_bounds_2a880(actor_bytes, &bounds, 0x12345678UL);
    check(result == 0, "false extent provider returns zero");
    check(memcmp(&bounds, before, sizeof(before)) == 0,
          "false extent provider leaves all caller bounds bytes untouched");
    check(helper_calls == 1 && helper_actor_seen == actor_bytes &&
          helper_mode_seen == 0x5678UL,
          "provider receives actor and only the low WORD of mode");
    ++cases;

    reset_fixture();
    seed_actor(actor_bytes);
    set_test_extents();
    helper_extents[3] = 0;
    helper_extents[4] = 0;
    helper_result = 1;
    memset(&bounds, 0x6d, sizeof(bounds));
    memcpy(before, &bounds, sizeof(before));
    result = actor_bounds_2a880(actor_bytes, &bounds, 0xabcduL);
    check(result == 0,
          "successful provider with both horizontal sentinel words zero returns zero");
    check(memcmp(&bounds, before, sizeof(before)) == 0,
          "horizontal sentinel path performs no output WORD stores");
    check(helper_calls == 1 && helper_mode_seen == 0xabcduL,
          "horizontal sentinel path still invokes provider exactly once");
    ++cases;
}

static void source_axis_flag_cases(void)
{
    static const struct Case {
        u8 flags;
        u16 expected[6];
        const char *label;
    } table[] = {
        {0x00, {0x0003,0x0005,0x8008,0x0007,0x000b,0x8011},
         "unflagged axes add their two extents"},
        {0x10, {0xfff5,0x0005,0x8008,0xfff9,0x000b,0x8011},
         "bit 0x10 reverses only the x extent direction"},
        {0x20, {0x0003,0xfff9,0x8008,0x0007,0xffff,0x8011},
         "bit 0x20 reverses only the y extent direction"},
        {0x30, {0xfff5,0xfff9,0x8008,0xfff9,0xffff,0x8011},
         "both direction bits reverse x and y while z still adds"}
    };
    unsigned int case_index;

    for (case_index = 0; case_index < sizeof(table) / sizeof(table[0]);
         ++case_index) {
        Query2a170Bounds bounds;
        unsigned int i;
        int result;
        reset_fixture();
        seed_actor(actor_bytes);
        *(u8 *)(actor_bytes + 0x54) = (u8)(0x40U | table[case_index].flags);
        set_test_extents();
        helper_result = 1;
        memset(&bounds, 0xa5, sizeof(bounds));
        result = actor_bounds_2a880(
            actor_bytes, &bounds, 0xabcd1234UL + case_index);
        check(result == 1, "extent provider plus visible horizontal extent returns one");
        check(helper_calls == 1 && helper_actor_seen == actor_bytes &&
              helper_mode_seen == (0x1234UL + case_index),
              "success path preserves actor identity and low-WORD mode forwarding");
        for (i = 0; i < 6U; ++i)
            check(output_word(&bounds, i) == table[case_index].expected[i],
                  table[case_index].label);
        ++cases;
    }
}

static void source_reads_actor_after_provider(void)
{
    Query2a170Bounds bounds;
    int result;
    reset_fixture();
    seed_actor(actor_bytes);
    set_test_extents();
    helper_result = 1;
    helper_mutates_actor = 1;
    result = actor_bounds_2a880(actor_bytes, &bounds, 1UL);
    check(result == 1, "mutating extent provider's success reaches output builder");
    check_six_words(&bounds, 0xfffe, 0x0002, 0x0007,
                    0x0002, 0x0008, 0x0010,
                    "coordinates and flags are read after the provider returns");
    ++cases;
}

static void source_output_may_alias_actor(void)
{
    Query2a170Bounds *bounds = (Query2a170Bounds *)(void *)actor_bytes;
    int result;
    reset_fixture();
    seed_actor(actor_bytes);
    set_test_extents();
    helper_result = 1;
    result = actor_bounds_2a880(actor_bytes, bounds, 0UL);
    check(result == 1, "builder supports an output record overlapping the actor prefix");
    check_six_words(bounds, 0x0003, 0x0005, 0x8008,
                    0x0007, 0x000b, 0x8011,
                    "coordinate fields and initial flags precede output stores for actor-base alias");
    ++cases;
}

static void native_helper_failure_controls(void)
{
    ActorBounds2a880 native_bounds =
        (ActorBounds2a880)(void *)native_cell(0x2a880UL);
    ExtentProvider30e40 native_provider =
        (ExtentProvider30e40)(void *)native_cell(0x30e40UL);
    Query2a170Bounds bounds;
    u8 before[12];
    u8 native_scratch[16];
    u16 *type_word = actor_word(actor_bytes, 0x34);
    int provider_result;
    int bounds_result;

    reset_fixture();
    memset(actor_bytes, 0x55, sizeof(actor_bytes));
    *type_word = 0;
    memset(&bounds, 0x3c, sizeof(bounds));
    memcpy(before, &bounds, sizeof(before));
    memset(native_scratch, 0xa6, sizeof(native_scratch));
    provider_result = native_provider(actor_bytes, native_scratch, 0x56781234UL);
    bounds_result = native_bounds(actor_bytes, &bounds, 0x56781234UL);
    check(provider_result == 0 && bounds_result == 0,
          "native extent provider and 2a880 both take the actor-type-zero failure path");
    check(memcmp(&bounds, before, sizeof(before)) == 0,
          "native 2a880 leaves caller output untouched when 30e40 rejects the actor");
    use_real_extent_provider=1;
    bounds_result=actor_bounds_2a880(actor_bytes,&bounds,0x56781234UL);
    check(bounds_result==0 && memcmp(&bounds,before,sizeof(before))==0,
          "source bounds real extent failure preserves output like native");
    ++cases;
}

static void write_u32(u8 *destination, u32 value)
{
    *(u32 *)(void *)destination = value;
}

static void seed_alias_actor(u8 *actor)
{
    memset(actor, 0, 256U);
    *actor_dword(actor, 0x00) = 0x00000000UL;
    *actor_dword(actor, 0x04) = 0x01000000UL;
    *actor_dword(actor, 0x08) = 0x02000000UL;
    actor[0x22] = 2U;
    *actor_word(actor, 0x34) = 1U;
    actor[0x54] = 0U;
    *actor_word(actor, 0x3a) = 0x0100U;
    *actor_word(actor, 0x3c) = 0x0100U;
    *actor_word(actor, 0x3e) = 0x0100U;
}

static void source_native_output_alias_reloads_y_flags(void)
{
    ActorBounds2a880 native_bounds_fn =
        (ActorBounds2a880)(void *)native_cell(0x2a880UL);
    ActorResource *native_resources =
        (ActorResource *)(void *)native_cell(0x5cee0UL);
    ActorResource saved_native_row;
    ActorResource saved_source_row;
    u8 source_actor[256];
    u8 native_actor[256];
    u8 *metadata;
    u8 *record;
    unsigned long metadata_base;
    DWORD old_protect;
    int native_result;
    int source_result;

    reset_fixture();
    saved_native_row = native_resources[2];
    saved_source_row = g_actor_resources[2];
    if (!VirtualProtect(&native_resources[2], sizeof(ActorResource),
                        PAGE_READWRITE, &old_protect)) {
        printf("FAIL could not make loaded native resource row writable\n");
        ++failures;
        ++cases;
        return;
    }

    check(native_resources[2].entry_count == 0x011aU &&
          g_actor_resources[2].entry_count == 0x011aU,
          "native and source resource row 2 agree on the metadata-table stride base");
    memset(controlled_metadata, 0, sizeof(controlled_metadata));
    metadata_base = (unsigned long)g_actor_resources[2].entry_count * 4UL + 8UL;
    write_u32(controlled_metadata + metadata_base + 4UL, 0x1000UL);
    metadata = controlled_metadata + metadata_base + 16UL;
    metadata[1] = 1U;
    record = metadata + 2;
    record[0] = 0U;
    record[1] = 1U;
    record[2] = 0U;
    record[3] = 32U;
    record[4] = 3U;
    record[5] = 0U;

    g_actor_resources[2].references = (ActorReference *)(void *)controlled_metadata;
    g_actor_resources[2].residency_state = 3U;
    native_resources[2].references = (ActorReference *)(void *)controlled_metadata;
    native_resources[2].residency_state = 3U;

    seed_alias_actor(source_actor);
    seed_alias_actor(native_actor);
    use_real_extent_provider = 1;
    source_result = actor_bounds_2a880(
        source_actor, (Query2a170Bounds *)(void *)(source_actor + 0x4e), 0UL);
    use_real_extent_provider = 0;
    native_result = native_bounds_fn(
        native_actor, (Query2a170Bounds *)(void *)(native_actor + 0x4e), 0UL);

    check(source_result == native_result,
          "real 30e40-backed aliased-output call returns the native result");
    if (memcmp(source_actor, native_actor, sizeof(source_actor)) != 0) {
        printf("ALIAS-DIFF source-y=(%04x,%04x) native-y=(%04x,%04x) flags=%02x\n",
               *actor_word(source_actor, 0x50), *actor_word(source_actor, 0x56),
               *actor_word(native_actor, 0x50), *actor_word(native_actor, 0x56),
               native_actor[0x54]);
    }
    check(memcmp(source_actor, native_actor, sizeof(source_actor)) == 0,
          "source reloads y direction after aliased x-max store, matching native actor bytes");
    check(native_result == 1 && *actor_word(native_actor, 0x54) == 0x0020U,
          "x-max WORD store changes actor flags byte to 0x20 before native y branch");
    check(*actor_word(native_actor, 0x50) == 0x00fdU &&
          *actor_word(native_actor, 0x56) == 0x00ffU,
          "native y branch observes the post-x-store 0x20 flag and reverses extents");

    native_resources[2] = saved_native_row;
    g_actor_resources[2] = saved_source_row;
    (void)VirtualProtect(&native_resources[2], sizeof(ActorResource),
                         old_protect, &old_protect);
    ++cases;
}

static void source_native_profile_alias_reloads_y_flags(void)
{
    ActorBounds2a880 native_bounds_fn =
        (ActorBounds2a880)(void *)native_cell(0x2a880UL);
    unsigned char **native_profile_slot =
        (unsigned char **)(void *)native_cell(0x70d2cUL);
    unsigned char *saved_native_profile = *native_profile_slot;
    unsigned char *saved_source_profile = g_header_pointer_70d2c;
    u8 profile[1024];
    u8 source_actor[256];
    u8 native_actor[256];
    DWORD old_protect;
    int native_result;
    int source_result;

    reset_fixture();
    if (!VirtualProtect(native_profile_slot, sizeof(void *),
                        PAGE_READWRITE, &old_protect)) {
        printf("FAIL could not make loaded native profile-root cell writable\n");
        ++failures;
        ++cases;
        return;
    }

    memset(profile, 0, sizeof(profile));
    write_u32(profile + 4, 0x20UL);
    profile[0x26] = 1U;
    write_u32(profile + 0x28, 0x00000000UL);
    profile[0x2c] = 0U;
    profile[0x2d] = 0U;
    profile[0x2e] = 0U;
    profile[0x2f] = 1U;
    profile[0x30] = 0U;
    profile[0x31] = 32U;
    profile[0x32] = 0U;
    profile[0x33] = 2U;
    g_header_pointer_70d2c = profile;
    *native_profile_slot = profile;

    seed_alias_actor(source_actor);
    seed_alias_actor(native_actor);
    source_actor[0x22] = 0U;
    native_actor[0x22] = 0U;
    use_real_extent_provider = 1;
    source_result = actor_bounds_2a880(
        source_actor, (Query2a170Bounds *)(void *)(source_actor + 0x4e), 0UL);
    use_real_extent_provider = 0;
    native_result = native_bounds_fn(
        native_actor, (Query2a170Bounds *)(void *)(native_actor + 0x4e), 0UL);

    check(source_result == native_result,
          "real 30e40-to-31ec0 profile path returns the native result");
    if (memcmp(source_actor, native_actor, sizeof(source_actor)) != 0) {
        printf("PROFILE-ALIAS-DIFF source-y=(%04x,%04x) native-y=(%04x,%04x) flags=%02x\n",
               *actor_word(source_actor, 0x50), *actor_word(source_actor, 0x56),
               *actor_word(native_actor, 0x50), *actor_word(native_actor, 0x56),
               native_actor[0x54]);
    }
    check(memcmp(source_actor, native_actor, sizeof(source_actor)) == 0,
          "source 31ec0 profile extents preserve the post-x-store y-flag behavior");
    check(native_result == 1 && *actor_word(native_actor, 0x54) == 0x0020U,
          "profile-backed x-max WORD store changes actor+0x54 before native y branch");
    check(*actor_word(native_actor, 0x50) == 0x00fdU &&
          *actor_word(native_actor, 0x56) == 0x00ffU,
          "native 31ec0 extents select reversed y bounds after the aliased x store");

    g_header_pointer_70d2c = saved_source_profile;
    *native_profile_slot = saved_native_profile;
    (void)VirtualProtect(native_profile_slot, sizeof(void *),
                         old_protect, &old_protect);
    ++cases;
}

int main(int argc, char **argv)
{
    unsigned int spy_cases, spy_checks;
    if (argc != 2)
        return 2;
    oracle = LoadLibraryA(argv[1]);
    if (oracle == 0) {
        printf("LoadLibrary failed: %lu\n", (u32)GetLastError());
        return 3;
    }

    source_failure_and_horizontal_guard();
    source_axis_flag_cases();
    source_reads_actor_after_provider();
    source_output_may_alias_actor();
    spy_cases=cases; spy_checks=checks;
    native_helper_failure_controls();
    source_native_output_alias_reloads_y_flags();
    source_native_profile_alias_reloads_y_flags();

    FreeLibrary(oracle);
    printf("actor_bounds_native_cases=%u checks=%u failures=%u\n",cases-spy_cases,checks-spy_checks,failures);
    printf("actor_bounds_source_spy_cases=%u checks=%u failures=%u\n",spy_cases,spy_checks,failures);
    return failures == 0 ? 0 : 1;
}
