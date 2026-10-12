#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/behavioral/action_spawn_27d40.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef struct ActorDetachView ActorDetachView;
struct ActorDetachView {
    u8 reserved_000[0x50];
    u32 flags_050;
    u32 render_flags_054;
    u8 reserved_058[4];
    ActorDetachView *attachment_05c;
    ActorDetachView *child_060;
    u8 reserved_064[8];
    u32 fx_linked_06c;
};

/* Test-only mirrors of frame_data.c cells used by the existing pool/factory
 * providers. The actor pointer itself comes from the real actor_storage.c. */
u16 g_configuration_word_71374;
u16 g_configuration_word_713ba;
u16 g_configuration_word_713b6;
u16 g_configuration_word_7138c;
u16 g_configuration_word_71430;
void *g_configuration_dword_713a4;
u16 g_configuration_word_71308;
u16 g_configuration_word_713b4;
u16 g_configuration_word_713a8;
u16 g_configuration_word_713c4;
u16 g_configuration_word_71378;
void *g_configuration_dword_71388;
extern void *g_alternate_actor_pool;

typedef void *(__cdecl *NativeSpawn)(void *actor, unsigned long action);
typedef struct NativeAsset NativeAsset;
struct NativeAsset {
    void *image;
    u16 unknown_004;
    u8 category;
    u8 unknown_007;
};

static HMODULE oracle;
static int cases;
static int checks;
static int failures;
static int release_calls;
static int bad_release_kind;
static int remove_fx_calls;
static unsigned char *native_module;
static u8 native_pool[0x94 * 2];
static u8 source_pool[0x94 * 2];
static u8 native_parent_storage[0x100];
static u8 source_parent_storage[0x100];
static u8 native_old_child[0x94];
static u8 source_old_child[0x94];
static u8 image_tokens[8][8];
static NativeAsset assets[8];

static u8 saved_cells[6][4];
static const u32 native_cell_rvas[6] = {
    0x71374, 0x713ba, 0x713b6, 0x7138c, 0x713a4, 0x734e0
};

int __cdecl release_actor(ActorDetachView *actor)
{
    ++release_calls;
    if (*(volatile u16 *)(volatile void *)((volatile u8 *)actor + 0x2e) != 0) {
        bad_release_kind = 1;
        return 0;
    }
    return 1;
}

void __cdecl remove_fx_links(ActorDetachView *actor)
{
    (void)actor;
    ++remove_fx_calls;
}

static u8 *native_cell(u32 rva)
{
    return native_module + rva;
}

static void check(int ok, const char *description)
{
    ++checks;
    if (!ok) {
        ++failures;
        printf("FAIL %s\n", description);
    }
}

static void fill_pattern(u8 *bytes, unsigned long count, u8 seed)
{
    unsigned long i;
    for (i = 0; i != count; ++i)
        bytes[i] = (u8)(seed + i * 17UL + (i >> 1));
}

static void put_u16(u8 *bytes, unsigned long offset, u16 value)
{
    memcpy(bytes + offset, &value, sizeof(value));
}

static void put_u32(u8 *bytes, unsigned long offset, u32 value)
{
    memcpy(bytes + offset, &value, sizeof(value));
}

static u16 get_u16(const u8 *bytes, unsigned long offset)
{
    u16 value;
    memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

static u32 get_u32(const u8 *bytes, unsigned long offset)
{
    u32 value;
    memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

static int same_bytes_except_pointer(const u8 *left, const u8 *right,
                                    unsigned long count, unsigned long pointer_offset)
{
    u8 a[0x200];
    u8 b[0x200];
    if (count > sizeof(a))
        return 0;
    memcpy(a, left, count);
    memcpy(b, right, count);
    memset(a + pointer_offset, 0, 4);
    memset(b + pointer_offset, 0, 4);
    return memcmp(a, b, count) == 0;
}

static void initialize_assets(void)
{
    unsigned int i;
    for (i = 0; i != 8; ++i) {
        memset(image_tokens[i], (int)(0x41 + i), sizeof(image_tokens[i]));
        assets[i].image = image_tokens[i];
        assets[i].unknown_004 = (u16)(0x1200 + i);
        assets[i].category = (u8)(0x70 + i);
        assets[i].unknown_007 = (u8)(0x30 + i);
    }
}

static void save_native_cells(void)
{
    unsigned int i;
    for (i = 0; i != 6; ++i)
        memcpy(saved_cells[i], native_cell(native_cell_rvas[i]), 4);
}

static void restore_native_cells(void)
{
    unsigned int i;
    for (i = 0; i != 6; ++i)
        memcpy(native_cell(native_cell_rvas[i]), saved_cells[i], 4);
}

static void reset_native_state(u8 *pool, NativeAsset *table,
                               unsigned short limit, int exhausted)
{
    u16 word;
    u32 pointer;
    word = 0;
    memcpy(native_cell(0x71374), &word, 2);
    word = limit;
    memcpy(native_cell(0x713ba), &word, 2);
    word = 0;
    memcpy(native_cell(0x713b6), &word, 2);
    memcpy(native_cell(0x7138c), &word, 2);
    memcpy(native_cell(0x71430), &word, 2);
    pointer = (u32)(unsigned long)table;
    memcpy(native_cell(0x713a4), &pointer, 4);
    pointer = (u32)(unsigned long)pool;
    memcpy(native_cell(0x734e0), &pointer, 4);
    if (exhausted)
        put_u16(pool, 0x2e, 0x8000);
}

static void reset_source_state(u8 *pool, NativeAsset *table,
                               unsigned short limit, int exhausted)
{
    g_configuration_word_71374 = 0;
    g_configuration_word_713ba = limit;
    g_configuration_word_713b6 = 0;
    g_configuration_word_7138c = 0;
    g_configuration_word_71430 = 0;
    g_configuration_dword_713a4 = table;
    g_alternate_actor_pool = pool;
    if (exhausted)
        put_u16(pool, 0x2e, 0x8000);
}

static void prepare_parent(u8 *parent, u8 *old_child, int has_child,
                           u32 flags, u32 render_flags)
{
    fill_pattern(parent, 0x100, 0x23);
    put_u32(parent, 0x00, 1234567UL);
    put_u32(parent, 0x04, (u32)-7654321L);
    put_u32(parent, 0x08, 0x13572468UL);
    put_u16(parent, 0x2e, 0x1234);
    put_u16(parent, 0x38, 0x6a2d);
    parent[0x22] = 0x5e;
    put_u32(parent, 0x50, flags);
    put_u32(parent, 0x54, render_flags);
    put_u32(parent, 0x60, has_child ? (u32)(unsigned long)old_child : 0);
}

static void prepare_pool(int alias_parent, u8 *parent, u8 *pool)
{
    fill_pattern(pool, sizeof(native_pool), 0x81);
    put_u16(pool, 0x2e, 0);
    if (alias_parent)
        memcpy(pool, parent, 0x94);
    put_u16(pool, 0x2e, 0);
}

static void run_case(const char *name, unsigned long action,
                     int early_guard, int has_child, int exhausted,
                     int alias_parent)
{
    NativeSpawn native_fn;
    Actor *source_result;
    void *native_result;
    u8 *native_parent;
    u8 *source_parent;
    u8 native_parent_before[0x100];
    u8 source_parent_before[0x100];
    u8 native_old_before[0x94];
    u8 source_old_before[0x94];
    u8 native_pool_before[sizeof(native_pool)];
    u8 source_pool_before[sizeof(source_pool)];
    int before_release_calls;
    int expected_success;
    u8 *native_view;
    u8 *source_view;

    fill_pattern(native_old_child, sizeof(native_old_child), 0x5a);
    memcpy(source_old_child, native_old_child, sizeof(native_old_child));
    put_u16(native_old_child, 0x2e, 0);
    put_u16(source_old_child, 0x2e, 0);

    prepare_pool(alias_parent, native_parent_storage, native_pool);
    prepare_pool(alias_parent, source_parent_storage, source_pool);
    native_parent = alias_parent ? native_pool : native_parent_storage;
    source_parent = alias_parent ? source_pool : source_parent_storage;
    prepare_parent(native_parent, native_old_child, has_child,
                   early_guard ? 0x40000000UL : 0x00123456UL,
                   0x00000124UL);
    prepare_parent(source_parent, source_old_child, has_child,
                   early_guard ? 0x40000000UL : 0x00123456UL,
                   0x00000124UL);
    if (alias_parent) {
        put_u16(native_parent, 0x2e, 0);
        put_u16(source_parent, 0x2e, 0);
        if (has_child) {
            put_u32(native_parent, 0x60, (u32)(unsigned long)native_parent);
            put_u32(source_parent, 0x60, (u32)(unsigned long)source_parent);
        }
    }

    reset_native_state(native_pool, assets, 1, exhausted);
    reset_source_state(source_pool, assets, 1, exhausted);
    if (alias_parent && has_child) {
        put_u32(native_parent, 0x60, (u32)(unsigned long)native_parent);
        put_u32(source_parent, 0x60, (u32)(unsigned long)source_parent);
    }

    memcpy(native_parent_before, native_parent, sizeof(native_parent_before));
    memcpy(source_parent_before, source_parent, sizeof(source_parent_before));
    memcpy(native_old_before, native_old_child, sizeof(native_old_before));
    memcpy(source_old_before, source_old_child, sizeof(source_old_before));
    memcpy(native_pool_before, native_pool, sizeof(native_pool_before));
    memcpy(source_pool_before, source_pool, sizeof(source_pool_before));
    before_release_calls = release_calls;

    native_fn = (NativeSpawn)(void *)(native_module + 0x27d40);
    native_result = native_fn(native_parent, action);
    source_result = action_spawn_27d40((Actor *)source_parent, action);
    expected_success = !early_guard && !exhausted;
    native_view = alias_parent ? native_pool : native_parent;
    source_view = alias_parent ? source_pool : source_parent;

    check((native_result != NULL) == expected_success,
          "native return agrees with fixture branch");
    check((source_result != 0) == expected_success,
          "candidate return agrees with fixture branch");
    if (expected_success) {
        check(native_result == (void *)native_pool,
              "native returns the selected alternate-pool slot");
        check((void *)source_result == (void *)source_pool,
              "candidate returns the selected alternate-pool slot");
        check((u32)(unsigned long)native_result == get_u32(native_view, 0x60),
              "native parent stores the returned child pointer");
        check((u32)(unsigned long)source_result == get_u32(source_view, 0x60),
              "candidate parent stores the returned child pointer");
        check(get_u32(native_pool, 0x50) == 0x10000000UL,
              "native child flag store");
        check(get_u32(source_pool, 0x50) == 0x10000000UL,
              "candidate child flag store");
        check(get_u32(native_view, 0x54) ==
                  (alias_parent ? 0x80800000UL : 0x00800124UL),
              "native parent render flag OR");
        check(get_u32(source_view, 0x54) ==
                  (alias_parent ? 0x80800000UL : 0x00800124UL),
              "candidate parent render flag OR");
        check(get_u16(native_pool, 0x2e) == (u16)(action | 0x2000UL),
              "native factory kind uses the action low WORD");
        check(get_u16(source_pool, 0x2e) == (u16)(action | 0x2000UL),
              "source factory kind uses the action low WORD");
        check(get_u32(native_pool, 0x64) ==
                  (u32)(unsigned long)assets[action & 0x0fffUL].image,
              "native factory selects the measured fixture asset row");
        check(get_u32(source_pool, 0x64) ==
                  (u32)(unsigned long)assets[action & 0x0fffUL].image,
              "source factory selects the measured fixture asset row");
        check(get_u32(native_pool, 0x00) == 1234567UL &&
              get_u32(source_pool, 0x00) == 1234567UL,
              "factory receives parent x after detach");
        check(get_u32(native_pool, 0x04) == (u32)-7654321L &&
              get_u32(source_pool, 0x04) == (u32)-7654321L,
              "factory receives parent y after detach");
        check(get_u32(native_pool, 0x08) == 0x13572468UL &&
              get_u32(source_pool, 0x08) == 0x13572468UL,
              "factory receives parent z after detach");
        check(native_pool[0x22] == native_view[0x22] &&
              source_pool[0x22] == source_view[0x22],
              "late parent category byte is copied to child");
        check(get_u16(native_pool, 0x38) == get_u16(native_view, 0x38) &&
              get_u16(source_pool, 0x38) == get_u16(source_view, 0x38),
              "late parent WORD is copied to child");
        check(same_bytes_except_pointer(native_view, source_view, 0x94, 0x60),
              "native/source parent bytes agree after pointer normalization");
        check(same_bytes_except_pointer(native_pool, source_pool, 0x94, 0x60),
              "native/source alternate actor bytes agree after pointer normalization");
    } else if (early_guard) {
        check(!memcmp(native_parent, native_parent_before, 0x100),
              "native early guard leaves parent unchanged");
        check(!memcmp(source_parent, source_parent_before, 0x100),
              "candidate early guard leaves parent unchanged");
        check(!memcmp(native_pool, native_pool_before, sizeof(native_pool)),
              "native early guard leaves alternate pool unchanged");
        check(!memcmp(source_pool, source_pool_before, sizeof(source_pool)),
              "candidate early guard leaves alternate pool unchanged");
        check(same_bytes_except_pointer(native_parent, source_parent, 0x94, 0x60),
              "native/source blocked parent bytes agree after pointer normalization");
    } else {
        check(get_u32(native_view, 0x60) == 0 && get_u32(source_view, 0x60) == 0,
              "failed creation leaves the old child detached");
        check((get_u32(native_view, 0x54) & 0x00800000UL) == 0 &&
              (get_u32(source_view, 0x54) & 0x00800000UL) == 0,
              "failed creation keeps the detach render-bit clear");
        check(same_bytes_except_pointer(native_view, source_view, 0x94, 0x60),
              "native/source detached parent bytes agree");
    }

    if (has_child && !early_guard)
        check(release_calls == before_release_calls + 1,
              "existing typed detach provider releases the zero-kind child once");
    else
        check(release_calls == before_release_calls,
              "no child release on early guard or no-child path");
    check(!bad_release_kind, "release spy stayed within its measured zero-kind branch");
    check(remove_fx_calls == 0, "unrelated detach-buffer spy was not invoked");
    check(!memcmp(native_old_child, native_old_before, sizeof(native_old_before)) &&
          !memcmp(source_old_child, source_old_before, sizeof(source_old_before)),
          "measured zero-kind release branch leaves old child bytes unchanged");
    if (exhausted)
        check(!memcmp(native_pool, source_pool, sizeof(native_pool)),
              "exhausted native/source pool prefix agrees");
    ++cases;
    printf("case=%s native=%s source=%s\n", name,
           native_result ? "child" : "null", source_result ? "child" : "null");
}

int main(int argc, char **argv)
{
    unsigned int i;
    if (argc != 2)
        return 2;
    if (sizeof(void *) != 4 || sizeof(unsigned long) != 4)
        return 3;
    oracle = LoadLibraryExA(argv[1], NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!oracle) {
        printf("LoadLibraryExA failed %lu\n", GetLastError());
        return 4;
    }
    native_module = (u8 *)oracle;
    save_native_cells();
    initialize_assets();

    run_case("blocked_existing_child", 0xa007UL, 1, 1, 0, 0);
    run_case("normal_0xa007", 0xa007UL, 0, 0, 0, 0);
    run_case("detach_then_0xa001", 0xa001UL, 0, 1, 0, 0);
    run_case("detach_then_exhausted", 0xa007UL, 0, 1, 1, 0);
    run_case("parent_pool_alias", 0xa007UL, 0, 1, 0, 1);

    check(release_calls == 3, "three non-blocked existing-child detach cases ran");
    check(!bad_release_kind && remove_fx_calls == 0,
          "test-only external helpers remained on declared branches");
    restore_native_cells();
    for (i = 0; i != 6; ++i)
        check(memcmp(saved_cells[i], native_cell(native_cell_rvas[i]), 4) == 0,
              "native module data cell restored before unload");
    printf("setup_source_27d40_cases=%d checks=%d failures=%d\n",
           cases, checks, failures);
    FreeLibrary(oracle);
    return failures ? 1 : 0;
}
