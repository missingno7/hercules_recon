#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/interpreters.h"
#include "../src/eng1/worklist_factory_types.h"
#include "../src/shared/engine_interface.h"
#include "../src/shared/engine_fx_record.h"

typedef struct NormalFactoryAsset NormalFactoryAsset;
struct NormalFactoryAsset {
    void *image;
    void *field_004;
    void *field_008;
    unsigned char category;
    unsigned char unknown_00d[3];
};
typedef struct AlternateFactoryAsset AlternateFactoryAsset;
struct AlternateFactoryAsset {
    void *image;
    unsigned short unknown_004;
    unsigned char category;
    unsigned char unknown_007;
};
typedef struct FixtureAltCallbackRecord FixtureAltCallbackRecord;
struct FixtureAltCallbackRecord {
    void (__cdecl *initialize)(void *actor);
    void *unknown_004;
};
typedef struct ActorLinkProbe ActorLinkProbe;
struct ActorLinkProbe {
    long x, y, z;
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category, mode;
    unsigned char unknown_024[6];
    short state_02a;
    unsigned char unknown_02c[0x38 - 0x2c];
    unsigned short word_038;
    unsigned char unknown_03a[0x50 - 0x3a];
    unsigned long flags_050, render_flags_054;
    unsigned char unknown_058[4];
    ActorLinkProbe *attachment_05c;
};

ActorNormal *g_normal_actor_pool;
ActorCore *g_alternate_actor_pool;
unsigned short g_normal_cursor_primary, g_normal_limit_primary;
unsigned short g_normal_limit_wrap, g_normal_cursor_alternate;
unsigned short g_normal_limit_wrap_start;
unsigned short g_alternate_cursor_primary, g_alternate_limit_primary;
unsigned short g_alternate_limit_wrap, g_alternate_cursor_alternate;
unsigned short g_alternate_limit_wrap_start;
NormalFactoryAsset *g_normal_factory_assets;
AlternateFactoryAsset *g_alternate_factory_assets;
FixtureAltCallbackRecord *g_alt_callbacks;
long g_busy, g_countdown;
EngineInterface g_engine_interface;
FxSoundRecordView g_engine_fx_records[24];

static union { unsigned long align; unsigned char bytes[0x94 * 2]; } shared_pool;
static union { unsigned long align; unsigned char bytes[0x134]; } shared_actor;
#define source_pool shared_pool
#define native_pool shared_pool
#define source_actor shared_actor
#define native_actor shared_actor
static AlternateFactoryAsset assets[35];
static FixtureAltCallbackRecord callbacks[35];
static unsigned long image_token;
static unsigned char *native_module;
static unsigned char config_before[0xc4];
static unsigned long alt_pool_pointer_before;
static unsigned char source_actor_after[0x134];
static unsigned char native_actor_after[0x134];
static unsigned char source_pool_after[0x94 * 2];
static unsigned char native_pool_after[0x94 * 2];
static unsigned long checks, failures;
static unsigned long source_callback_count, native_callback_count;
static unsigned long source_callback_bad, native_callback_bad;
static unsigned long source_callback_mask, native_callback_mask;
static unsigned long source_child_address, native_child_address;
static int current_side;
static unsigned char *current_parent;
static unsigned short current_saved_state;

extern void __cdecl eng1_native_code_target_03e220(Actor *actor);

static unsigned char *native_cell(unsigned long rva)
{
    return native_module + rva;
}

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

static void set_native_pointer(unsigned long rva, const void *value)
{
    *(unsigned long *)(void *)native_cell(rva) = (unsigned long)value;
}

static void set_native_word(unsigned long rva, unsigned short value)
{
    *(unsigned short *)(void *)native_cell(rva) = value;
}

void __cdecl detach_attachment(ActorLinkProbe *actor)
{
    (void)actor;
    printf("unexpected detach_attachment branch in 0x3e220 fixture\n");
    ExitProcess(0xe1);
}

void __cdecl dispatch_local_2c650(void)
{
    printf("unexpected FX dispatch in 0x3e220 fixture\n");
    ExitProcess(0xe2);
}

void __cdecl dispatch_local_2c660(void)
{
    printf("unexpected FX dispatch in 0x3e220 fixture\n");
    ExitProcess(0xe3);
}

static void __cdecl attached_actor_probe(void *raw_child)
{
    unsigned char *child = (unsigned char *)raw_child;
    void **frames = *(void ***)(void *)(current_parent + 0x110);
    unsigned long bad = 0;

    if (*(unsigned long *)(void *)(current_parent + 0x50) != 0x00000a02UL) bad |= 1UL << 0;
    if (*(unsigned short *)(void *)(current_parent + 0x124) != 0) bad |= 1UL << 1;
    if (*(unsigned short *)(void *)(current_parent + 0xee) != 3) bad |= 1UL << 2;
    if (*(unsigned long *)(void *)(current_parent + 0xdc) !=
        ((unsigned long)current_saved_state << 16)) bad |= 1UL << 3;
    if (*(unsigned short *)(void *)(current_parent + 0x2a) != 0) bad |= 1UL << 4;
    if (*(unsigned long *)(void *)(current_parent + 0x5c) !=
        (unsigned long)raw_child) bad |= 1UL << 5;
    if (*(unsigned long *)(void *)(child + 0x5c) !=
        (unsigned long)current_parent) bad |= 1UL << 6;
    if (*(unsigned long *)(void *)(child + 0x50) != 0x00010000UL) bad |= 1UL << 7;
    if (*(unsigned short *)(void *)(child + 0x38) !=
        *(unsigned short *)(void *)(current_parent + 0x38)) bad |= 1UL << 8;
    if (*(unsigned short *)(void *)(child + 0x2e) != 0x2022U) bad |= 1UL << 9;
    if (*(unsigned char *)(void *)(child + 0x22) != 0x73U) bad |= 1UL << 10;
    if (*(unsigned long *)(void *)(child + 0x64) !=
        (unsigned long)&image_token) bad |= 1UL << 11;
    if (*(unsigned short *)(void *)(current_parent + 0x114) != 13U) bad |= 1UL << 12;
    if (*(unsigned long *)(void *)(current_parent + 0x64) !=
        (unsigned long)frames[13]) bad |= 1UL << 13;

    if (current_side == 0) {
        ++source_callback_count;
        source_child_address = (unsigned long)raw_child;
        if (bad) { ++source_callback_bad; source_callback_mask |= bad; }
    } else {
        ++native_callback_count;
        native_child_address = (unsigned long)raw_child;
        if (bad) { ++native_callback_bad; native_callback_mask |= bad; }
    }

    /* The target has no stores after attach_actor; this mutation must survive. */
    *(unsigned short *)(void *)(current_parent + 0x124) = 0x6d13U;
    *(unsigned short *)(void *)(current_parent + 0x2a) = 0x9876U;
    *(unsigned short *)(void *)(child + 0x126) = 0x4b2aU;
}

static void seed_source(int allocation_enabled, unsigned short saved_state,
                        void **frame_table)
{
    unsigned char *parent = source_actor.bytes;

    memset(source_pool.bytes, 0xa5, sizeof(source_pool.bytes));
    *(unsigned short *)(void *)(source_pool.bytes + 0x2e) = 0;
    memset(parent, 0xa5, sizeof(source_actor.bytes));
    *(long *)(void *)(parent + 0x00) = 0x00112233L;
    *(long *)(void *)(parent + 0x04) = -0x00023456L;
    *(long *)(void *)(parent + 0x08) = 0x00034567L;
    *(unsigned short *)(void *)(parent + 0x2a) = saved_state;
    *(unsigned short *)(void *)(parent + 0x38) = 0xc35aU;
    *(void **)(void *)(parent + 0x5c) = 0;
    *(void **)(void *)(parent + 0x110) = frame_table;
    *(unsigned short *)(void *)(parent + 0x114) = 0;
    *(void **)(void *)(parent + 0x64) = 0;
    memset(assets, 0, sizeof(assets));
    assets[0x22].image = &image_token;
    assets[0x22].category = 0x73U;
    memset(callbacks, 0, sizeof(callbacks));
    callbacks[0x22].initialize = attached_actor_probe;

    g_alternate_actor_pool = (ActorCore *)source_pool.bytes;
    g_alternate_cursor_alternate = 0;
    g_alternate_limit_wrap = allocation_enabled ? 2U : 0U;
    g_alternate_limit_wrap_start = 0;
    g_alternate_factory_assets = assets;
    g_alt_callbacks = callbacks;
    current_parent = parent;
    current_saved_state = saved_state;
    current_side = 0;
}

static void seed_native(int allocation_enabled)
{
    memset(native_pool.bytes, 0xa5, sizeof(native_pool.bytes));
    *(unsigned short *)(void *)(native_pool.bytes + 0x2e) = 0;
    set_native_pointer(0x734e0UL, native_pool.bytes);
    set_native_pointer(0x713a4UL, assets);
    set_native_pointer(0x7139cUL, callbacks);
    set_native_word(0x71430UL, 0);
    set_native_word(0x713b6UL, (unsigned short)(allocation_enabled ? 2U : 0U));
    set_native_word(0x7138cUL, 0);
    current_parent = native_actor.bytes;
    current_side = 1;
}

static void run_case(int allocation_enabled, unsigned short saved_state)
{
    void (__cdecl *native_target)(Actor *);
    void **frame_table = (void **)native_cell(0x65b28UL);
    unsigned long source_callbacks_before = source_callback_count;
    unsigned long native_callbacks_before = native_callback_count;

    seed_source(allocation_enabled, saved_state, frame_table);
    eng1_native_code_target_03e220((Actor *)source_actor.bytes);
    memcpy(source_actor_after, source_actor.bytes, sizeof(source_actor_after));
    memcpy(source_pool_after, source_pool.bytes, sizeof(source_pool_after));

    memset(native_actor.bytes, 0xa5, sizeof(native_actor.bytes));
    *(long *)(void *)(native_actor.bytes + 0x00) = 0x00112233L;
    *(long *)(void *)(native_actor.bytes + 0x04) = -0x00023456L;
    *(long *)(void *)(native_actor.bytes + 0x08) = 0x00034567L;
    *(unsigned short *)(void *)(native_actor.bytes + 0x2a) = saved_state;
    *(unsigned short *)(void *)(native_actor.bytes + 0x38) = 0xc35aU;
    *(void **)(void *)(native_actor.bytes + 0x5c) = 0;
    *(void **)(void *)(native_actor.bytes + 0x110) = frame_table;
    *(unsigned short *)(void *)(native_actor.bytes + 0x114) = 0;
    *(void **)(void *)(native_actor.bytes + 0x64) = 0;
    seed_native(allocation_enabled);
    current_saved_state = saved_state;
    native_target = (void (__cdecl *)(Actor *))(void *)native_cell(0x3e220UL);
    native_target((Actor *)native_actor.bytes);
    memcpy(native_actor_after, native_actor.bytes, sizeof(native_actor_after));
    memcpy(native_pool_after, native_pool.bytes, sizeof(native_pool_after));

    check(memcmp(source_actor_after, native_actor_after,
                 sizeof(source_actor_after)) == 0,
          "complete actor bytes match after actual frame and attach helpers");
    check(memcmp(source_pool_after, native_pool_after,
                 sizeof(source_pool_after)) == 0,
          "bounded two-slot alternate pool bytes match");
    check(source_callback_count - source_callbacks_before ==
              (allocation_enabled ? 1UL : 0UL) &&
          native_callback_count - native_callbacks_before ==
              (allocation_enabled ? 1UL : 0UL),
          "alternate action callback runs iff allocation succeeds");
    check(source_callback_bad == 0 && native_callback_bad == 0,
          "attach callback observes every ordered target store and child link");
    check(*(unsigned short *)(void *)(source_actor_after + 0x114) == 13U &&
          *(unsigned short *)(void *)(native_actor_after + 0x114) == 13U &&
          *(unsigned long *)(void *)(source_actor_after + 0xdc) ==
              ((unsigned long)saved_state << 16) &&
          *(unsigned long *)(void *)(native_actor_after + 0xdc) ==
              ((unsigned long)saved_state << 16),
          "selector 13 and zero-extended saved WORD shift agree");
    if (allocation_enabled) {
        check(*(unsigned short *)(void *)(source_actor_after + 0x124) == 0x6d13U &&
              *(unsigned short *)(void *)(native_actor_after + 0x124) == 0x6d13U &&
              *(unsigned short *)(void *)(source_actor_after + 0x2a) == 0x9876U &&
              *(unsigned short *)(void *)(native_actor_after + 0x2a) == 0x9876U,
              "attach callback mutations survive return with no later target stores");
    } else {
        check(*(unsigned short *)(void *)(source_actor_after + 0x124) == 0 &&
              *(unsigned short *)(void *)(native_actor_after + 0x124) == 0 &&
              *(unsigned short *)(void *)(source_actor_after + 0x2a) == 0 &&
              *(unsigned short *)(void *)(native_actor_after + 0x2a) == 0,
              "failed attachment leaves target stores intact");
    }
    check(source_actor_after[0xec] == 0xa5U &&
          source_actor_after[0xed] == 0xa5U &&
          source_actor_after[0xf0] == 0xa5U &&
          native_actor_after[0xec] == 0xa5U &&
          native_actor_after[0xed] == 0xa5U &&
          native_actor_after[0xf0] == 0xa5U,
          "interior WORD +0xee preserves neighboring +0xec/+0xed/+0xf0 bytes");
    if (allocation_enabled) {
        check(*(unsigned long *)(void *)(source_actor_after + 0x5c) ==
                  source_child_address &&
              *(unsigned long *)(void *)(native_actor_after + 0x5c) ==
                  native_child_address &&
              source_child_address == (unsigned long)source_pool.bytes &&
              native_child_address == (unsigned long)native_pool.bytes,
              "kind 0x2022 attaches the first bounded alternate slot");
    } else {
        check(*(unsigned long *)(void *)(source_actor_after + 0x5c) == 0 &&
              *(unsigned long *)(void *)(native_actor_after + 0x5c) == 0 &&
              memcmp(source_pool_after, source_pool.bytes,
                     sizeof(source_pool_after)) == 0 &&
              memcmp(native_pool_after, native_pool.bytes,
                     sizeof(native_pool_after)) == 0,
              "allocation failure leaves attachment empty and pool unchanged");
    }
    if (memcmp(source_actor_after, native_actor_after,
               sizeof(source_actor_after)) != 0) {
        unsigned long i, shown = 0;
        printf("actor diffs:");
        for (i = 0; i < sizeof(source_actor_after) && shown < 24; ++i) {
            if (source_actor_after[i] != native_actor_after[i]) {
                printf(" %02lx:%02x/%02x", i, source_actor_after[i], native_actor_after[i]);
                ++shown;
            }
        }
        printf("\n");
    }
    if (memcmp(source_pool_after, native_pool_after,
               sizeof(source_pool_after)) != 0) {
        unsigned long i, shown = 0;
        printf("pool diffs:");
        for (i = 0; i < sizeof(source_pool_after) && shown < 24; ++i) {
            if (source_pool_after[i] != native_pool_after[i]) {
                printf(" %02lx:%02x/%02x", i, source_pool_after[i], native_pool_after[i]);
                ++shown;
            }
        }
        printf("\n");
    }
    printf("callback masks source=0x%08lx native=0x%08lx\n",
           source_callback_mask, native_callback_mask);
    printf("PASS 3e220 allocation=%s saved=0x%04x\n",
           allocation_enabled ? "success" : "exhausted", saved_state);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage: eng1_call_3e220_fixture <ENG1.DLL>\n");
        return 2;
    }
    native_module = (unsigned char *)LoadLibraryExA(
        argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (native_module == 0) {
        printf("LoadLibraryExA failed %lu\n", GetLastError());
        return 3;
    }
    memcpy(config_before, native_cell(0x71370UL), sizeof(config_before));
    alt_pool_pointer_before = *(unsigned long *)(void *)native_cell(0x734e0UL);
    check(*(unsigned long *)(void *)native_cell(0x67848UL) ==
              (unsigned long)native_cell(0x3e220UL),
          "existing callback record 48 slot 0 binds native RVA 0x3e220");
    check(*(unsigned long *)(void *)native_cell(0x679a4UL) ==
              (unsigned long)native_cell(0x3e220UL),
          "existing callback record 77 slot 0 binds native RVA 0x3e220");

    run_case(1, 0x1234U);
    run_case(1, 0x8f27U);
    run_case(0, 0x00e1U);

    memcpy(native_cell(0x71370UL), config_before, sizeof(config_before));
    *(unsigned long *)(void *)native_cell(0x734e0UL) = alt_pool_pointer_before;
    check(memcmp(native_cell(0x71370UL), config_before,
                 sizeof(config_before)) == 0,
          "all changed native configuration and cursor cells are restored");
    check(*(unsigned long *)(void *)native_cell(0x734e0UL) ==
              alt_pool_pointer_before,
          "native alternate-pool pointer cell is restored");
    FreeLibrary((HMODULE)native_module);
    printf("eng1_call_3e220 cases=3 checks=%lu failures=%lu\n",
           checks, failures);
    return failures ? 1 : 0;
}
