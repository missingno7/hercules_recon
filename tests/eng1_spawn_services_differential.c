#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/worklist_factory_types.h"

typedef struct ActorNormal ActorNormal;
typedef struct ActorMiddle ActorMiddle;
typedef struct ActorCore ActorCore;

ActorNormal *g_normal_actor_pool;
ActorMiddle *g_middle_actor_pool;
ActorCore *g_alternate_actor_pool;
unsigned short g_normal_cursor_primary, g_normal_limit_primary;
unsigned short g_normal_limit_wrap, g_normal_cursor_alternate;
unsigned short g_normal_limit_wrap_start;
unsigned short g_alternate_cursor_primary, g_alternate_limit_primary;
unsigned short g_alternate_limit_wrap, g_alternate_cursor_alternate;
unsigned short g_alternate_limit_wrap_start;
unsigned short g_middle_cursor_primary, g_middle_limit_primary;
unsigned short g_middle_wrap_start_primary, g_middle_cursor_alternate;
unsigned short g_middle_wrap_start_alternate;
void *g_normal_factory_assets;
void *g_alternate_factory_assets;
unsigned long g_configuration_dword_71370;
unsigned long g_configuration_dword_7137c;
unsigned long g_configuration_dword_71390;
unsigned long g_configuration_dword_7139c;

unsigned char eng1_fixture_normal_assets[128];
unsigned char eng1_fixture_middle_assets[64];
unsigned char eng1_fixture_alternate_assets[64];
unsigned char eng1_fixture_normal_actions[96];
unsigned char eng1_fixture_middle_actions[64];
unsigned char eng1_fixture_alternate_actions[64];

static union { unsigned long align; unsigned char bytes[0x134 * 2]; } source_normal;
static union { unsigned long align; unsigned char bytes[0x0ec * 2]; } source_middle;
static union { unsigned long align; unsigned char bytes[0x094 * 2]; } source_alternate;
static union { unsigned long align; unsigned char bytes[0x134 * 2]; } native_normal;
static union { unsigned long align; unsigned char bytes[0x0ec * 2]; } native_middle;
static union { unsigned long align; unsigned char bytes[0x094 * 2]; } native_alternate;
static union { unsigned long align; unsigned char bytes[0x134]; } source_owner;
static union { unsigned long align; unsigned char bytes[0x134]; } native_owner;
static HMODULE native_module;
static unsigned long checks, failures;
static int callback_side;
static int callback_expect_owner;
static unsigned long source_callbacks, native_callbacks;
static unsigned long callback_actor_stride;
static void *source_callback_actor, *native_callback_actor;
static unsigned char *source_expected_owner, *native_expected_owner;
static unsigned long callback_order_failures;
static int callback_expect_chain;
static unsigned char source_callback_copy[0x134];
static unsigned char native_callback_copy[0x134];
static const unsigned long raw_callback_result = 0x12345678UL;

extern void *__cdecl spawn_at_group_300c0(long, long, long,
                                            unsigned short, unsigned short);
extern void __cdecl spawn_actor_with_owner(void *, long, long, long,
                                           unsigned short);
extern ActorMiddle *__cdecl spawn_middle_277e0(long, long, long,
                                                   unsigned short);

typedef void (__cdecl *Action)(void *);
typedef unsigned long (__cdecl *RawAction)(void *);

static unsigned char *native_cell(unsigned long rva)
{
    return (unsigned char *)native_module + rva;
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

void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    printf("INCOMPLETE %06lx %s\n", rva, reason);
    ExitProcess(0xe1);
}

/* The pool provider TU carries reset routines that are outside this fixture.
 * These definitions satisfy those unentered routines only. */
int __cdecl remove_actor(void *actor) { (void)actor; return 0; }
int __cdecl remove_middle_actor(void *actor) { (void)actor; return 0; }
int __cdecl release_actor(void *actor) { (void)actor; return 0; }
void __cdecl clear_actor(void *actor) { (void)actor; }

static unsigned long __cdecl callback_probe(void *actor)
{
    unsigned char *a = (unsigned char *)actor;
    unsigned char *owner = callback_side == 0 ? source_expected_owner :
                                                native_expected_owner;
    if (callback_side == 0) {
        ++source_callbacks;
        source_callback_actor = actor;
        memcpy(source_callback_copy, actor, callback_actor_stride);
    } else {
        ++native_callbacks;
        native_callback_actor = actor;
        memcpy(native_callback_copy, actor, callback_actor_stride);
    }
    if (callback_expect_owner &&
        (a[0x94] != 0x91U || a[0x95] != 0x72U || a[0x96] != 0x53U))
        ++callback_order_failures;
    if (callback_expect_chain && owner != 0 &&
        (*(unsigned long *)(void *)(owner + 0x130) != (unsigned long)actor ||
         *(unsigned long *)(void *)(a + 0x12c) != (unsigned long)owner))
        ++callback_order_failures;
    return raw_callback_result;
}

static void set_action(unsigned char *table, unsigned long index,
                       unsigned long stride)
{
    union {
        Action void_action;
        RawAction raw_action;
    } entry;

    entry.raw_action = callback_probe;
    *(Action *)(void *)(table + index * stride) = entry.void_action;
}

static void seed_source(int family, unsigned long kind)
{
    memset(source_normal.bytes, 0xa5, sizeof(source_normal.bytes));
    memset(source_middle.bytes, 0xa5, sizeof(source_middle.bytes));
    memset(source_alternate.bytes, 0xa5, sizeof(source_alternate.bytes));
    memset(source_owner.bytes, 0xa5, sizeof(source_owner.bytes));
    memset(eng1_fixture_normal_assets, 0, sizeof(eng1_fixture_normal_assets));
    memset(eng1_fixture_middle_assets, 0, sizeof(eng1_fixture_middle_assets));
    memset(eng1_fixture_alternate_assets, 0, sizeof(eng1_fixture_alternate_assets));
    memset(eng1_fixture_normal_actions, 0, sizeof(eng1_fixture_normal_actions));
    memset(eng1_fixture_middle_actions, 0, sizeof(eng1_fixture_middle_actions));
    memset(eng1_fixture_alternate_actions, 0, sizeof(eng1_fixture_alternate_actions));
    g_normal_actor_pool = (ActorNormal *)source_normal.bytes;
    g_middle_actor_pool = (ActorMiddle *)source_middle.bytes;
    g_alternate_actor_pool = (ActorCore *)source_alternate.bytes;
    g_normal_cursor_primary = g_normal_cursor_alternate = 0;
    g_normal_limit_primary = g_normal_limit_wrap = 2;
    g_normal_limit_wrap_start = 0;
    g_alternate_cursor_primary = g_alternate_cursor_alternate = 0;
    g_alternate_limit_primary = g_alternate_limit_wrap = 2;
    g_alternate_limit_wrap_start = 0;
    g_middle_cursor_primary = g_middle_cursor_alternate = 0;
    g_middle_limit_primary = g_middle_wrap_start_primary = 2;
    g_middle_wrap_start_alternate = 0;

    *(unsigned short *)(void *)(source_normal.bytes + 0x2e) = 0;
    *(unsigned short *)(void *)(source_middle.bytes + 0x2e) = 0;
    *(unsigned short *)(void *)(source_alternate.bytes + 0x2e) = 0;
    *(unsigned short *)(void *)(source_owner.bytes + 0x2e) = (unsigned short)kind;
    source_owner.bytes[0x94] = 0x91U;
    source_owner.bytes[0x95] = 0x72U;
    source_owner.bytes[0x96] = 0x53U;
    *(unsigned long *)(void *)(source_owner.bytes + 0x12c) = 0;
    *(unsigned long *)(void *)(source_owner.bytes + 0x130) = 0;

    memset(eng1_fixture_normal_assets, 0x4c, sizeof(eng1_fixture_normal_assets));
    memset(eng1_fixture_alternate_assets, 0x7a, sizeof(eng1_fixture_alternate_assets));
    *(unsigned long *)(void *)(eng1_fixture_normal_assets + 3 * 16) = 0x11223344UL;
    *(unsigned long *)(void *)(eng1_fixture_normal_assets + 3 * 16 + 4) = 0x22334455UL;
    *(unsigned long *)(void *)(eng1_fixture_normal_assets + 3 * 16 + 8) = 0x33445566UL;
    eng1_fixture_normal_assets[3 * 16 + 12] = 0x42U;
    *(unsigned long *)(void *)(eng1_fixture_alternate_assets + 3 * 8) = 0x44556677UL;
    eng1_fixture_alternate_assets[3 * 8 + 6] = 0x64U;
    *(unsigned long *)(void *)(eng1_fixture_middle_assets + 3 * 8) = 0x55667788UL;
    eng1_fixture_middle_assets[3 * 8 + 6] = 0x36U;
    g_normal_factory_assets = eng1_fixture_normal_assets;
    g_alternate_factory_assets = eng1_fixture_alternate_assets;
    g_configuration_dword_71370 = (unsigned long)eng1_fixture_normal_actions;
    g_configuration_dword_7137c = (unsigned long)eng1_fixture_middle_actions;
    g_configuration_dword_71390 = (unsigned long)eng1_fixture_middle_assets;
    g_configuration_dword_7139c = (unsigned long)eng1_fixture_alternate_actions;
    if (family == 2)
        set_action(eng1_fixture_alternate_actions, kind & 0x0fffUL, 8UL);
    else if (family == 1)
        set_action(eng1_fixture_middle_actions, kind & 0x0fffUL, 8UL);
    else
        set_action(eng1_fixture_normal_actions, kind & 0x0fffUL, 12UL);
}

static void seed_native(int family, unsigned long kind)
{
    memset(native_normal.bytes, 0xa5, sizeof(native_normal.bytes));
    memset(native_middle.bytes, 0xa5, sizeof(native_middle.bytes));
    memset(native_alternate.bytes, 0xa5, sizeof(native_alternate.bytes));
    memset(native_owner.bytes, 0xa5, sizeof(native_owner.bytes));
    *(unsigned short *)(void *)(native_normal.bytes + 0x2e) = 0;
    *(unsigned short *)(void *)(native_middle.bytes + 0x2e) = 0;
    *(unsigned short *)(void *)(native_alternate.bytes + 0x2e) = 0;
    *(unsigned short *)(void *)(native_owner.bytes + 0x2e) = (unsigned short)kind;
    native_owner.bytes[0x94] = 0x91U;
    native_owner.bytes[0x95] = 0x72U;
    native_owner.bytes[0x96] = 0x53U;
    *(unsigned long *)(void *)(native_owner.bytes + 0x12c) = 0;
    *(unsigned long *)(void *)(native_owner.bytes + 0x130) = 0;

    set_native_pointer(0x72860UL, native_normal.bytes);
    set_native_pointer(0x73494UL, native_middle.bytes);
    set_native_pointer(0x734e0UL, native_alternate.bytes);
    set_native_pointer(0x71388UL, eng1_fixture_normal_assets);
    set_native_pointer(0x71390UL, eng1_fixture_middle_assets);
    set_native_pointer(0x713a4UL, eng1_fixture_alternate_assets);
    set_native_pointer(0x71370UL, eng1_fixture_normal_actions);
    set_native_pointer(0x7137cUL, eng1_fixture_middle_actions);
    set_native_pointer(0x7139cUL, eng1_fixture_alternate_actions);
    set_native_word(0x713c4UL, 0);
    set_native_word(0x713a8UL, 2);
    set_native_word(0x71378UL, 0);
    set_native_word(0x71430UL, 0);
    set_native_word(0x713b6UL, 2);
    set_native_word(0x7138cUL, 0);
    set_native_word(0x713b8UL, 0);
    set_native_word(0x713a0UL, 2);
    set_native_word(0x713aaUL, 0);
    (void)family;
    (void)kind;
}

static void reset_observers(int owner, unsigned long actor_bytes)
{
    callback_expect_owner = owner;
    callback_expect_chain = owner && actor_bytes == 0x134UL;
    callback_actor_stride = actor_bytes;
    source_callbacks = native_callbacks = 0;
    callback_order_failures = 0;
    source_callback_actor = native_callback_actor = 0;
    memset(source_callback_copy, 0, sizeof(source_callback_copy));
    memset(native_callback_copy, 0, sizeof(native_callback_copy));
}

static void run_case(unsigned long rva, unsigned long kind,
                     int owner_mode, int alternate)
{
    long x = 0x00123456L, y = -0x00034567L, z = 0x00000789L;
    void *source_result, *native_result;
    unsigned char *source_pool = alternate ? source_alternate.bytes : source_normal.bytes;
    unsigned char *native_pool = alternate ? native_alternate.bytes : native_normal.bytes;
    unsigned long stride = alternate ? 0x94UL : 0x134UL;
    unsigned char *source_owner_ptr = source_owner.bytes;
    unsigned char *native_owner_ptr = native_owner.bytes;
    unsigned long source_before_callbacks, native_before_callbacks;
    unsigned long native_raw_result = 0;
    unsigned char source_actor_compare[0x134];
    unsigned char native_actor_compare[0x134];

    seed_source(alternate ? 2 : 0, kind);
    reset_observers(owner_mode == 1 && !alternate, stride);
    seed_native(alternate ? 2 : 0, kind);
    source_expected_owner = source_owner.bytes;
    native_expected_owner = native_owner.bytes;
    if (owner_mode == 1) {
        spawn_actor_with_owner(source_owner_ptr, x, y, z, kind);
        source_result = source_pool;
        source_before_callbacks = source_callbacks;
        callback_side = 1;
        native_raw_result = ((unsigned long (__cdecl *)(void *, long, long, long,
                                                         unsigned long))
            (void *)native_cell(0x30180UL))(native_owner_ptr, x, y, z, kind);
        native_result = native_pool;
        native_before_callbacks = native_callbacks;
    } else {
        source_result = spawn_at_group_300c0(x, y, z, kind, 0xdecafbadUL);
        source_before_callbacks = source_callbacks;
        callback_side = 1;
        native_result = ((void *(__cdecl *)(long, long, long, unsigned long, unsigned long))
            (void *)native_cell(0x300c0UL))(x, y, z, kind, 0xdecafbadUL);
        native_before_callbacks = native_callbacks;
    }
    callback_side = 0;

    if (owner_mode == 1) {
        check(native_raw_result == raw_callback_result &&
              native_raw_result != (unsigned long)native_pool,
              "30180 preserves raw callback EAX, not the created actor pointer");
    } else {
        check(source_result == source_pool && native_result == native_pool,
              "300c0 returns the first selected pool slot");
    }
    check(source_callbacks == 1 && native_callbacks == 1 &&
          source_before_callbacks == 1 && native_before_callbacks == 1,
          "one callback is invoked after successful creation");
    check(source_callback_actor == source_pool &&
          native_callback_actor == native_pool,
          "callback receives the created actor independently of return EAX");
    check(callback_order_failures == 0,
          "owner-copy and chain updates are already visible to callback");
    memcpy(source_actor_compare, source_pool, stride);
    memcpy(native_actor_compare, native_pool, stride);
    if (owner_mode == 1 && !alternate) {
        /* These fields contain caller-side owner pointers, which necessarily
         * differ between the two controlled pool instances. */
        memset(source_actor_compare + 0x12c, 0, 8);
        memset(native_actor_compare + 0x12c, 0, 8);
    }
    check(memcmp(source_actor_compare, native_actor_compare, stride) == 0,
          "created actor bytes match after normalizing owner pointers");
    if (owner_mode == 1 && !alternate) {
        check(*(unsigned long *)(void *)(source_owner_ptr + 0x130) ==
              (unsigned long)source_result &&
              *(unsigned long *)(void *)(native_owner_ptr + 0x130) ==
              (unsigned long)native_result,
              "normal owner chain appends child");
        check(*(unsigned long *)(void *)(source_pool + 0x12c) ==
              (unsigned long)source_owner_ptr &&
              *(unsigned long *)(void *)(native_pool + 0x12c) ==
              (unsigned long)native_owner_ptr,
              "normal child links back to owner");
        check(memcmp(source_pool + 0x94, native_pool + 0x94, 3) == 0 &&
              source_pool[0x94] == 0x91U && source_pool[0x95] == 0x72U &&
              source_pool[0x96] == 0x53U,
              "owner status bytes copied before callback");
    }
    printf("PASS %06lx kind=%04lx\n", rva, kind);
}

static void run_middle_case(unsigned long rva, int owner_mode,
                            unsigned long kind)
{
    long x = -0x00123456L, y = 0x00034567L, z = -0x00000789L;
    unsigned long stride = 0x0ecUL;
    void *source_result;
    void *native_result;
    unsigned long native_raw_result = 0;

    seed_source(1, kind);
    reset_observers(owner_mode, stride);
    seed_native(1, kind);
    source_expected_owner = source_owner.bytes;
    native_expected_owner = native_owner.bytes;
    if (owner_mode) {
        spawn_actor_with_owner(source_owner.bytes, x, y, z,
                               (unsigned short)kind);
        source_result = source_middle.bytes;
        callback_side = 1;
        native_raw_result = ((unsigned long (__cdecl *)(void *, long, long, long,
                                                         unsigned long))
            (void *)native_cell(0x30180UL))(native_owner.bytes, x, y, z, kind);
        native_result = native_middle.bytes;
    } else {
        source_result = spawn_at_group_300c0(x, y, z, (unsigned short)kind, 0x55aaU);
        callback_side = 1;
        native_result = ((void *(__cdecl *)(long, long, long, unsigned long, unsigned long))
            (void *)native_cell(0x300c0UL))(x, y, z, kind, 0x55aaU);
    }
    callback_side = 0;

    if (owner_mode) {
        check(native_raw_result == raw_callback_result &&
              native_raw_result != (unsigned long)native_middle.bytes,
              "30180 middle path preserves raw callback EAX");
    } else {
        check(source_result == source_middle.bytes &&
              native_result == native_middle.bytes,
              "300c0 middle path returns first selected middle slot");
    }
    check(source_callbacks == 1 && native_callbacks == 1 &&
          source_callback_actor == source_result &&
          native_callback_actor == native_result,
          "middle action callback runs once with the created actor");
    check(memcmp(source_middle.bytes, native_middle.bytes, stride) == 0,
          "complete middle actor bytes match native output");
    check(callback_order_failures == 0,
          "middle owner bytes are visible before callback and no chain is appended");
    if (owner_mode)
        check(*(unsigned long *)(void *)(source_owner.bytes + 0x130) == 0 &&
              *(unsigned long *)(void *)(native_owner.bytes + 0x130) == 0,
              "middle service does not append the normal owner chain");
    printf("PASS %06lx middle kind=%04lx%s\n", rva, kind,
           owner_mode ? " owner" : "");
}

static void run_allocation_failure(int owner_case, int alternate)
{
    unsigned long kind = alternate ? 0x2003UL : 3UL;
    unsigned long stride = alternate ? 0x94UL : 0x134UL;
    unsigned char *source_pool = alternate ? source_alternate.bytes : source_normal.bytes;
    unsigned char *native_pool = alternate ? native_alternate.bytes : native_normal.bytes;
    unsigned char source_before[0x268];
    unsigned char native_before[0x268];
    unsigned char source_owner_before[0x134];
    unsigned char native_owner_before[0x134];
    void *source_result;
    void *native_result;
    unsigned long native_raw_result = 0;

    seed_source(alternate ? 2 : 0, kind);
    reset_observers(owner_case && !alternate, stride);
    seed_native(alternate ? 2 : 0, kind);
    memcpy(source_before, source_pool, stride * 2UL);
    memcpy(native_before, native_pool, stride * 2UL);
    memcpy(source_owner_before, source_owner.bytes, sizeof(source_owner_before));
    memcpy(native_owner_before, native_owner.bytes, sizeof(native_owner_before));
    if (alternate) {
        g_alternate_limit_wrap = 0;
        g_alternate_limit_wrap_start = 0;
        set_native_word(0x713b6UL, 0);
        set_native_word(0x7138cUL, 0);
    } else {
        g_normal_limit_wrap = 0;
        g_normal_limit_wrap_start = 0;
        set_native_word(0x713a8UL, 0);
        set_native_word(0x71378UL, 0);
    }

    if (owner_case) {
        spawn_actor_with_owner(source_owner.bytes, 1L, 2L, 3L, kind);
    } else {
        source_result = spawn_at_group_300c0(1L, 2L, 3L, kind, 0UL);
    }
    callback_side = 1;
    if (owner_case) {
        native_raw_result = ((unsigned long (__cdecl *)(void *, long, long, long,
                                                         unsigned long))
            (void *)native_cell(0x30180UL))(native_owner.bytes, 1L, 2L, 3L, kind);
        native_result = 0;
    } else {
        native_result = ((void *(__cdecl *)(long, long, long, unsigned long, unsigned long))
            (void *)native_cell(0x300c0UL))(1L, 2L, 3L, kind, 0UL);
    }
    callback_side = 0;

    if (owner_case) {
        check(native_raw_result == 0,
              "exhausted 30180 allocator path returns native null EAX");
    } else {
        check(source_result == 0 && native_result == 0,
              "exhausted 300c0 selected allocator returns null");
    }
    check(source_callbacks == 0 && native_callbacks == 0,
          "allocation failure skips both callbacks");
    check(memcmp(source_before, source_pool, stride * 2UL) == 0 &&
          memcmp(native_before, native_pool, stride * 2UL) == 0,
          "allocation failure preserves actor storage");
    if (owner_case)
        check(memcmp(source_owner_before, source_owner.bytes, sizeof(source_owner_before)) == 0 &&
              memcmp(native_owner_before, native_owner.bytes, sizeof(native_owner_before)) == 0,
              "allocation failure skips owner copy and chain writes");
    printf("PASS %s allocation failure\n", alternate ? "alternate" : "normal");
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage: spawn_services_fixture <ENG1.DLL>\n");
        return 2;
    }
    native_module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (native_module == 0) {
        printf("LoadLibraryExA failed %lu\n", GetLastError());
        return 3;
    }
    run_case(0x300c0UL, 3UL, 0, 0);
    run_middle_case(0x300c0UL, 0, 0x4003UL);
    run_case(0x300c0UL, 0x2003UL, 0, 1);
    run_case(0x30180UL, 3UL, 1, 0);
    run_middle_case(0x30180UL, 1, 0x6003UL);
    run_case(0x30180UL, 0x2003UL, 1, 1);
    run_allocation_failure(0, 0);
    run_allocation_failure(1, 1);
    printf("spawn_services cases=6 checks=%lu failures=%lu\n", checks, failures);
    FreeLibrary(native_module);
    return failures ? 1 : 0;
}
