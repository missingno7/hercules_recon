/*
 * Successful-path extension for ENG1 RVA 0x2b130.
 *
 * Native calls enter the read-only oracle DLL and use its real allocator,
 * initializer, factory and acquire helpers. Source calls the canonical 2b130
 * producer plus the existing registry, pool/init, factory, middle factory and
 * bounded table-guard sources. Only the callback body and table/asset rows are
 * controlled fixture state. The callback mutation case is therefore a
 * native call-site comparison with a test callback, not production callback
 * behavior or a production table/capacity claim.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreters.h"
#include "secondary_records.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

int __cdecl eng1_native_code_target_002b130(Actor *actor);

typedef struct NormalAsset NormalAsset;
struct NormalAsset {
    void *image;
    void *frames;
    void *steps;
    u8 category;
    u8 padding[3];
};

typedef struct AlternateAsset AlternateAsset;
struct AlternateAsset {
    void *image;
    u16 unknown;
    u8 category;
    u8 padding;
};

typedef struct NormalCallbackRow NormalCallbackRow;
struct NormalCallbackRow { void (__cdecl *callback)(void *); u8 padding[8]; };
typedef struct SmallCallbackRow SmallCallbackRow;
struct SmallCallbackRow { void (__cdecl *callback)(void *); u8 padding[4]; };

static HMODULE g_native_module;
static u8 g_parent[0x124];
static Eng1SpawnRecord g_records_a[2];
static Eng1SpawnRecord g_records_b[2];
static union { unsigned long align; u8 bytes[0x40]; } g_group_storage;
static union { unsigned long align; u8 bytes[0x134]; } g_normal_pool_storage;
static union { unsigned long align; u8 bytes[0xec]; } g_middle_pool_storage;
static union { unsigned long align; u8 bytes[0x94]; } g_alternate_pool_storage;
static u8 g_marker_image[16];
static u8 g_marker_frames[16];
static u8 g_marker_steps[16];
typedef struct NativeCellSnapshot NativeCellSnapshot;
struct NativeCellSnapshot { u32 rva; unsigned int size; u8 bytes[4]; };
static NativeCellSnapshot g_native_snapshots[40];
static unsigned int g_native_snapshot_count;
static int g_callback_calls;
static int g_remove_calls;
static int g_reset_endpoint_calls;
static int g_chain_endpoint_calls;
static int g_callback_mutates;
static void *g_live_actor;
static PendingActionGroup *g_live_group;
static Eng1SpawnRecord *g_live_records;
static u16 g_state_seen;
static u16 g_factory_step_seen;

/* Existing source providers bind these names to their measured scalar/pointer
 * cells. In this fixture, their storage is explicitly test-owned. */
unsigned short pending_action_group_count;
PendingActionGroup *pending_action_group_table[20];
unsigned short g_configuration_word_71394;
unsigned long g_configuration_dword_71370;
unsigned long g_configuration_dword_7137c;
unsigned long g_configuration_dword_71390;
unsigned long g_configuration_dword_7139c;
void *g_normal_factory_assets;
void *g_alternate_factory_assets;
void *g_normal_actor_pool;
void *g_middle_actor_pool;
void *g_alternate_actor_pool;
unsigned short g_normal_cursor_primary, g_normal_limit_primary;
unsigned short g_normal_limit_wrap, g_normal_cursor_alternate;
unsigned short g_normal_limit_wrap_start;
unsigned short g_alternate_cursor_primary, g_alternate_limit_primary;
unsigned short g_alternate_limit_wrap, g_alternate_cursor_alternate;
unsigned short g_alternate_limit_wrap_start;
unsigned short g_middle_cursor_primary, g_middle_limit_primary;
unsigned short g_middle_wrap_start_primary, g_middle_cursor_alternate;
unsigned short g_middle_wrap_start_alternate;

/* Registered synthetic prefixes for the actual production source guard. */
unsigned char eng1_fixture_normal_assets[128];
unsigned char eng1_fixture_middle_assets[64];
unsigned char eng1_fixture_alternate_assets[64];
unsigned char eng1_fixture_normal_actions[96];
unsigned char eng1_fixture_middle_actions[64];
unsigned char eng1_fixture_alternate_actions[64];

static u8 *native_rva(u32 rva)
{
    return (u8 *)(void *)g_native_module + rva;
}

static void set_native_bytes(u32 rva, const void *value, unsigned int size)
{
    DWORD old_protect;
    void *address = native_rva(rva);
    unsigned int i;
    for (i = 0; i < g_native_snapshot_count; ++i)
        if (g_native_snapshots[i].rva == rva) break;
    if (i == g_native_snapshot_count) {
        if (g_native_snapshot_count >=
            sizeof(g_native_snapshots) / sizeof(g_native_snapshots[0]) ||
            size > sizeof(g_native_snapshots[0].bytes)) {
            fprintf(stderr, "native cell snapshot capacity exceeded\n");
            ExitProcess(4);
        }
        g_native_snapshots[i].rva = rva;
        g_native_snapshots[i].size = size;
        memcpy(g_native_snapshots[i].bytes, address, size);
        ++g_native_snapshot_count;
    }
    if (!VirtualProtect(address, size, PAGE_READWRITE, &old_protect)) {
        fprintf(stderr, "VirtualProtect failed for native RVA 0x%lx: %lu\n",
                rva, GetLastError());
        ExitProcess(4);
    }
    memcpy(address, value, size);
    (void)VirtualProtect(address, size, old_protect, &old_protect);
}

static void restore_native_cells(void)
{
    unsigned int i = g_native_snapshot_count;
    while (i != 0U) {
        DWORD old_protect;
        NativeCellSnapshot *snapshot = &g_native_snapshots[--i];
        void *address = native_rva(snapshot->rva);
        if (VirtualProtect(address, snapshot->size, PAGE_READWRITE,
                           &old_protect)) {
            memcpy(address, snapshot->bytes, snapshot->size);
            (void)VirtualProtect(address, snapshot->size, old_protect,
                                 &old_protect);
        }
    }
}

static void set_native_word(u32 rva, u16 value)
{
    set_native_bytes(rva, &value, sizeof(value));
}

static void set_native_dword(u32 rva, u32 value)
{
    set_native_bytes(rva, &value, sizeof(value));
}

static u32 pointer_bits(const void *pointer)
{
    return (u32)(unsigned long)pointer;
}

static void __cdecl fixture_action(void *child)
{
    ++g_callback_calls;
    g_state_seen = *(u16 *)(void *)((u8 *)child + 0x2a);
    g_factory_step_seen = *(u16 *)(void *)((u8 *)child + 0x34);
    *(u16 *)(void *)((u8 *)child + 0x34) = 0xfedcU;
    if (g_live_records != 0)
        g_live_records[0].sprite = 0x1357U;
    if (g_callback_mutates) {
        g_live_group->count = 2L;
        *(u32 *)(void *)((u8 *)g_live_actor + 0x118) =
            pointer_bits(g_records_b);
    }
}

int __cdecl remove_actor_variant(void *actor)
{
    (void)actor;
    ++g_remove_calls;
    return 0;
}

int __cdecl remove_actor(void *actor) { (void)actor; ++g_reset_endpoint_calls; return 0; }
int __cdecl remove_middle_actor(void *actor) { (void)actor; ++g_reset_endpoint_calls; return 0; }
int __cdecl release_actor(void *actor) { (void)actor; ++g_reset_endpoint_calls; return 0; }
void __cdecl clear_actor(void *actor) { (void)actor; ++g_reset_endpoint_calls; }
void __cdecl append_chain(void *owner, void *child)
{ (void)owner; (void)child; ++g_chain_endpoint_calls; }

static void fill_tables(void)
{
    NormalAsset *normal_assets = (NormalAsset *)(void *)eng1_fixture_normal_assets;
    AlternateAsset *alternate_assets =
        (AlternateAsset *)(void *)eng1_fixture_alternate_assets;
    SmallCallbackRow *middle_callback_rows =
        (SmallCallbackRow *)(void *)eng1_fixture_middle_actions;
    SmallCallbackRow *alternate_callback_rows =
        (SmallCallbackRow *)(void *)eng1_fixture_alternate_actions;
    NormalCallbackRow *normal_callback_rows =
        (NormalCallbackRow *)(void *)eng1_fixture_normal_actions;
    unsigned int i;

    memset(g_marker_image, 0x31, sizeof(g_marker_image));
    memset(g_marker_frames, 0x42, sizeof(g_marker_frames));
    memset(g_marker_steps, 0x53, sizeof(g_marker_steps));
    memset(eng1_fixture_normal_assets, 0, sizeof(eng1_fixture_normal_assets));
    memset(eng1_fixture_middle_assets, 0, sizeof(eng1_fixture_middle_assets));
    memset(eng1_fixture_alternate_assets, 0,
           sizeof(eng1_fixture_alternate_assets));
    memset(eng1_fixture_normal_actions, 0,
           sizeof(eng1_fixture_normal_actions));
    memset(eng1_fixture_middle_actions, 0,
           sizeof(eng1_fixture_middle_actions));
    memset(eng1_fixture_alternate_actions, 0,
           sizeof(eng1_fixture_alternate_actions));

    normal_assets[1].image = g_marker_image;
    normal_assets[1].frames = g_marker_frames;
    normal_assets[1].steps = g_marker_steps;
    normal_assets[1].category = 3U;
    *(void **)(void *)(eng1_fixture_middle_assets + 8) = g_marker_image;
    eng1_fixture_middle_assets[8 + 6] = 4U;
    alternate_assets[1].image = g_marker_image;
    alternate_assets[1].unknown = 0x8877U;
    alternate_assets[1].category = 5U;

    normal_callback_rows[1].callback = fixture_action;
    middle_callback_rows[1].callback = fixture_action;
    alternate_callback_rows[1].callback = fixture_action;
    for (i = 0; i < 8; ++i) {
        /* Keep deliberately nonzero tails to make table stride observable. */
        normal_callback_rows[i].padding[0] = (u8)(0x90U + i);
        middle_callback_rows[i].padding[0] = (u8)(0xa0U + i);
        alternate_callback_rows[i].padding[0] = (u8)(0xb0U + i);
    }
}

static void initialize_case(unsigned short kind)
{
    unsigned int i;
    for (i = 0; i < sizeof(g_parent); ++i)
        g_parent[i] = (u8)(0x27U + i * 13U);
    for (i = 0; i < sizeof(g_records_a); ++i)
        ((u8 *)g_records_a)[i] = (u8)(0x4bU + i * 7U);
    for (i = 0; i < sizeof(g_records_b); ++i)
        ((u8 *)g_records_b)[i] = 0U;
    memset(g_group_storage.bytes, 0xcd, sizeof(g_group_storage.bytes));
    *(long *)(void *)g_group_storage.bytes = -1L;
    memset(g_normal_pool_storage.bytes, 0xa5,
           sizeof(g_normal_pool_storage.bytes));
    memset(g_middle_pool_storage.bytes, 0xa5,
           sizeof(g_middle_pool_storage.bytes));
    memset(g_alternate_pool_storage.bytes, 0xa5,
           sizeof(g_alternate_pool_storage.bytes));

    /* A zero kind at the pool's +0x2e must make each allocator select row 0. */
    *(u16 *)(void *)(g_normal_pool_storage.bytes + 0x2e) = 0U;
    *(u16 *)(void *)(g_middle_pool_storage.bytes + 0x2e) = 0U;
    *(u16 *)(void *)(g_alternate_pool_storage.bytes + 0x2e) = 0U;
    *(long *)(void *)(g_parent + 0x00) = (long)0x7fff8001UL;
    *(long *)(void *)(g_parent + 0x04) = (long)0x00010003UL;
    *(long *)(void *)(g_parent + 0x08) = (long)0xfffffff0UL;
    g_records_a[0].x = 2;
    g_records_a[0].y = -2;
    g_records_a[0].z = 3;
    g_records_a[0].kind = kind;
    g_records_a[0].sprite = 0x2468U;
    g_records_a[0].state = 0x5a3cU;
    /* A stale cached record would spawn this alternate child. The callback
     * swaps +0x118 so the live second record is instead the zero sentinel. */
    g_records_a[1].kind = 0x2002U;
    g_records_b[1].kind = 0U;
    *(u32 *)(void *)(g_parent + 0x118) = pointer_bits(g_records_a);
    *(u32 *)(void *)(g_parent + 0x120) = 0x12345678UL;

    g_live_actor = g_parent;
    g_live_group = (PendingActionGroup *)(void *)g_group_storage.bytes;
    g_live_records = g_records_a;
    g_callback_calls = 0;
    g_remove_calls = 0;
    g_reset_endpoint_calls = 0;
    g_chain_endpoint_calls = 0;
    g_state_seen = 0U;
    g_factory_step_seen = 0xffffU;
    g_callback_mutates = 1;

    g_normal_actor_pool = g_normal_pool_storage.bytes;
    g_middle_actor_pool = g_middle_pool_storage.bytes;
    g_alternate_actor_pool = g_alternate_pool_storage.bytes;
    g_normal_cursor_primary = 0U;
    g_normal_limit_primary = 0U;
    g_normal_limit_wrap = 1U;
    g_normal_cursor_alternate = 0U;
    g_normal_limit_wrap_start = 0U;
    g_alternate_cursor_primary = 0U;
    g_alternate_limit_primary = 0U;
    g_alternate_limit_wrap = 1U;
    g_alternate_cursor_alternate = 0U;
    g_alternate_limit_wrap_start = 0U;
    g_middle_cursor_primary = 0U;
    g_middle_limit_primary = 0U;
    g_middle_wrap_start_primary = 1U;
    g_middle_cursor_alternate = 0U;
    g_middle_wrap_start_alternate = 0U;

    pending_action_group_count = 1U;
    pending_action_group_table[0] = g_live_group;
    g_configuration_word_71394 = 8U;
    g_configuration_dword_71370 = pointer_bits(eng1_fixture_normal_actions);
    g_configuration_dword_7137c = pointer_bits(eng1_fixture_middle_actions);
    g_configuration_dword_71390 = pointer_bits(eng1_fixture_middle_assets);
    g_configuration_dword_7139c = pointer_bits(eng1_fixture_alternate_actions);
    g_normal_factory_assets = eng1_fixture_normal_assets;
    g_alternate_factory_assets = eng1_fixture_alternate_assets;
}

static void configure_native(void)
{
    u32 pointer;
    pointer = pointer_bits(g_normal_pool_storage.bytes);
    set_native_dword(0x72860UL, pointer);
    pointer = pointer_bits(g_middle_pool_storage.bytes);
    set_native_dword(0x73494UL, pointer);
    pointer = pointer_bits(g_alternate_pool_storage.bytes);
    set_native_dword(0x734e0UL, pointer);

    set_native_word(0x71308UL, 0U);
    set_native_word(0x713b4UL, 0U);
    set_native_word(0x713a8UL, 1U);
    set_native_word(0x713c4UL, 0U);
    set_native_word(0x71378UL, 0U);
    set_native_word(0x71374UL, 0U);
    set_native_word(0x713baUL, 0U);
    set_native_word(0x713b6UL, 1U);
    set_native_word(0x71430UL, 0U);
    set_native_word(0x7138cUL, 0U);
    set_native_word(0x713acUL, 0U);
    set_native_word(0x71432UL, 0U);
    set_native_word(0x713a0UL, 1U);
    set_native_word(0x713b8UL, 0U);
    set_native_word(0x713aaUL, 0U);

    set_native_word(0x71380UL, 1U);
    set_native_word(0x71394UL, 8U);
    pointer = pointer_bits(g_live_group);
    set_native_dword(0x713e0UL, pointer);
    pointer = pointer_bits(eng1_fixture_normal_actions);
    set_native_dword(0x71370UL, pointer);
    pointer = pointer_bits(eng1_fixture_middle_actions);
    set_native_dword(0x7137cUL, pointer);
    pointer = pointer_bits(eng1_fixture_middle_assets);
    set_native_dword(0x71390UL, pointer);
    pointer = pointer_bits(eng1_fixture_alternate_actions);
    set_native_dword(0x7139cUL, pointer);
    pointer = pointer_bits(eng1_fixture_normal_assets);
    set_native_dword(0x71388UL, pointer);
    pointer = pointer_bits(eng1_fixture_alternate_assets);
    set_native_dword(0x713a4UL, pointer);
}

static void print_hex(const u8 *bytes, unsigned int length)
{
    unsigned int i;
    for (i = 0; i < length; ++i)
        printf("%02x", bytes[i]);
}

int main(int argc, char **argv)
{
    unsigned short kind;
    const char *factory;
    unsigned int pool_size;
    int result;
    unsigned int i;
    if (argc != 4) {
        fprintf(stderr, "usage: neutralfixture.exe ENG1.DLL native|source kind\n");
        return 2;
    }
    if (strcmp(argv[2], "native") != 0 && strcmp(argv[2], "source") != 0)
        return 2;
    kind = (unsigned short)strtoul(argv[3], 0, 0);
    if (kind == 0x4001U) { factory = "middle"; pool_size = 0xecU; }
    else if (kind == 0x2001U) { factory = "alternate"; pool_size = 0x94U; }
    else if (kind == 0x0001U) { factory = "normal"; pool_size = 0x134U; }
    else return 2;

    g_native_module = LoadLibraryA(argv[1]);
    if (g_native_module == 0) {
        fprintf(stderr, "LoadLibraryA failed: %lu\n", GetLastError());
        return 3;
    }
    fill_tables();
    initialize_case(kind);
    if (strcmp(argv[2], "native") == 0) {
        typedef int (__cdecl *NativeEntry)(void *);
        NativeEntry entry;
        configure_native();
        entry = (NativeEntry)(void *)native_rva(0x2b130UL);
        result = entry(g_parent);
    } else {
        result = eng1_native_code_target_002b130((Actor *)(void *)g_parent);
    }

    printf("case=%s mode=%s result=%d callbacks=%d state=%04x factory_step=%04x count=%ld "
           "owner=%08lx pool_cursor=",
           factory, argv[2], result, g_callback_calls, g_state_seen,
           g_factory_step_seen,
           *(long *)(void *)g_group_storage.bytes,
           *(unsigned long *)(void *)(g_parent + 0x120));
    if (strcmp(factory, "normal") == 0)
        printf("%u", strcmp(argv[2], "native") == 0 ?
               *(u16 *)(void *)native_rva(0x713c4UL) : g_normal_cursor_alternate);
    else if (strcmp(factory, "middle") == 0)
        printf("%u", strcmp(argv[2], "native") == 0 ?
               *(u16 *)(void *)native_rva(0x713b8UL) : g_middle_cursor_alternate);
    else
        printf("%u", strcmp(argv[2], "native") == 0 ?
               *(u16 *)(void *)native_rva(0x71430UL) : g_alternate_cursor_alternate);
    printf(" remove=%d reset=%d chain=%d coordinates=",
           g_remove_calls, g_reset_endpoint_calls,
           g_chain_endpoint_calls);
    if (strcmp(factory, "normal") == 0) {
        printf("%08lx_%08lx_%08lx",
               *(unsigned long *)(void *)(g_normal_pool_storage.bytes + 0x00),
               *(unsigned long *)(void *)(g_normal_pool_storage.bytes + 0x04),
               *(unsigned long *)(void *)(g_normal_pool_storage.bytes + 0x08));
    } else if (strcmp(factory, "middle") == 0) {
        printf("%08lx_%08lx_%08lx",
               *(unsigned long *)(void *)(g_middle_pool_storage.bytes + 0x00),
               *(unsigned long *)(void *)(g_middle_pool_storage.bytes + 0x04),
               *(unsigned long *)(void *)(g_middle_pool_storage.bytes + 0x08));
    } else {
        printf("%08lx_%08lx_%08lx",
               *(unsigned long *)(void *)(g_alternate_pool_storage.bytes + 0x00),
               *(unsigned long *)(void *)(g_alternate_pool_storage.bytes + 0x04),
               *(unsigned long *)(void *)(g_alternate_pool_storage.bytes + 0x08));
    }
    printf(" child_state=%04x child_sprite=%04x child_kind=%04x records_ptr=%08lx\n",
           *(u16 *)(void *)((u8 *)(strcmp(factory, "normal") == 0 ?
               (void *)g_normal_pool_storage.bytes :
               strcmp(factory, "middle") == 0 ? (void *)g_middle_pool_storage.bytes :
               (void *)g_alternate_pool_storage.bytes) + 0x2a),
           *(u16 *)(void *)((u8 *)(strcmp(factory, "normal") == 0 ?
               (void *)g_normal_pool_storage.bytes :
               strcmp(factory, "middle") == 0 ? (void *)g_middle_pool_storage.bytes :
               (void *)g_alternate_pool_storage.bytes) + 0x34),
           *(u16 *)(void *)((u8 *)(strcmp(factory, "normal") == 0 ?
               (void *)g_normal_pool_storage.bytes :
               strcmp(factory, "middle") == 0 ? (void *)g_middle_pool_storage.bytes :
               (void *)g_alternate_pool_storage.bytes) + 0x2e),
           *(u32 *)(void *)(g_parent + 0x118));
    printf("actor="); print_hex(g_parent, sizeof(g_parent));
    printf(" pool=");
    if (strcmp(factory, "normal") == 0) print_hex(g_normal_pool_storage.bytes, pool_size);
    else if (strcmp(factory, "middle") == 0) print_hex(g_middle_pool_storage.bytes, pool_size);
    else print_hex(g_alternate_pool_storage.bytes, pool_size);
    printf(" group="); print_hex(g_group_storage.bytes, sizeof(g_group_storage.bytes));
    printf(" old_records="); print_hex((const u8 *)g_records_a, sizeof(g_records_a));
    printf(" new_records="); print_hex((const u8 *)g_records_b, sizeof(g_records_b));
    printf(" checks=%d\n", (int)(g_callback_calls == 1 && g_state_seen == 0x5a3cU &&
           g_factory_step_seen == 0U &&
           *(u16 *)(void *)((u8 *)(strcmp(factory, "normal") == 0 ?
               (void *)g_normal_pool_storage.bytes :
               strcmp(factory, "middle") == 0 ? (void *)g_middle_pool_storage.bytes :
               (void *)g_alternate_pool_storage.bytes) + 0x34) == 0x2468U &&
           *(u32 *)(void *)(g_parent + 0x118) == pointer_bits(g_records_b) &&
           *(long *)(void *)g_group_storage.bytes == 3L));
    (void)i;
    if (strcmp(argv[2], "native") == 0)
        restore_native_cells();
    if (!FreeLibrary(g_native_module)) return 4;
    return 0;
}
