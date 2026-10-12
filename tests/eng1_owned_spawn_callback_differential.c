#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/behavioral/interpreters.h"
#include "../src/eng1/behavioral/worklist_factory_types.h"

typedef struct ActorNormal ActorNormal;
typedef struct ActorCore ActorCore;

ActorNormal *g_normal_actor_pool;
ActorCore *g_alternate_actor_pool;
unsigned short g_normal_cursor_primary, g_normal_limit_primary;
unsigned short g_normal_limit_wrap, g_normal_cursor_alternate;
unsigned short g_normal_limit_wrap_start;
unsigned short g_alternate_cursor_primary, g_alternate_limit_primary;
unsigned short g_alternate_limit_wrap, g_alternate_cursor_alternate;
unsigned short g_alternate_limit_wrap_start;
void *g_normal_factory_assets;
void *g_alternate_factory_assets;
unsigned long g_configuration_dword_71370;
unsigned long g_configuration_dword_7137c;
unsigned long g_configuration_dword_71390;
unsigned long g_configuration_dword_7139c;

static union { unsigned long align; unsigned char bytes[0x134 * 2]; } normal_pool;
static union { unsigned long align; unsigned char bytes[0x134]; } owner_storage;
static unsigned char normal_assets[53 * 16];
static unsigned char action_table[53 * 12];
static void *frame_table[2];
static unsigned long image_token, steps_token;
static HMODULE native_module;
static unsigned char native_config_before[0xc4];
static unsigned long native_pool_pointer_before;
static unsigned char source_owner_after[0x134];
static unsigned char source_pool_after[0x134 * 2];
static unsigned char native_owner_after[0x134];
static unsigned char native_pool_after[0x134 * 2];
static unsigned long checks, failures;
static int current_side;
static unsigned char *current_owner;
static unsigned long source_callbacks, native_callbacks;
static unsigned long source_callback_failures, native_callback_failures;
static unsigned short source_callback_frame, native_callback_frame;
static unsigned long source_callback_actor, native_callback_actor;

extern void __cdecl eng1_native_code_target_040e20(Actor *actor);
extern void __cdecl advance_frame(Actor *actor);
extern void __cdecl spawn_actor_with_owner(Actor *owner, long x, long y, long z,
                                           unsigned short kind);
extern void __cdecl set_render_flags_3e9b0(Actor *actor);

ActorMiddle *__cdecl allocate_middle_actor(ActorKind kind)
{
    (void)kind;
    printf("unexpected middle-allocation branch in kind-0x34 fixture\n");
    ExitProcess(0xe2);
    return 0;
}

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

void __cdecl eng1_guard_worklist_table_read(const void *base,
                                             unsigned long index,
                                             unsigned long stride,
                                             unsigned long width,
                                             const char *site)
{
    (void)base; (void)index; (void)stride; (void)width; (void)site;
}

void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    printf("INCOMPLETE %06lx %s\n", rva, reason);
    ExitProcess(0xe1);
}

static void __cdecl callback_probe(void *raw_child)
{
    unsigned char *owner;
    unsigned char *child;
    unsigned short frame_index;
    unsigned long *owner_next;
    unsigned long *child_prev;
    unsigned char bad;

    owner = current_owner;
    child = (unsigned char *)raw_child;
    frame_index = *(unsigned short *)(void *)(owner + 0x114);
    owner_next = (unsigned long *)(void *)(owner + 0x130);
    child_prev = (unsigned long *)(void *)(child + 0x12c);
    bad = 0;
    if (frame_index != 1U || *owner_next != (unsigned long)child ||
        *child_prev != (unsigned long)owner ||
        child[0x94] != owner[0x94] || child[0x95] != owner[0x95] ||
        child[0x96] != owner[0x96])
        bad = 1;

    if (current_side == 0) {
        ++source_callbacks;
        source_callback_frame = frame_index;
        source_callback_actor = (unsigned long)raw_child;
        if (bad) ++source_callback_failures;
        set_render_flags_3e9b0((Actor *)raw_child);
    } else {
        ++native_callbacks;
        native_callback_frame = frame_index;
        native_callback_actor = (unsigned long)raw_child;
        if (bad) ++native_callback_failures;
        ((void (__cdecl *)(Actor *))(void *)native_cell(0x3e9b0UL))
            ((Actor *)raw_child);
    }
}

static void seed_world(int allocation_enabled)
{
    unsigned char *owner;
    unsigned char *asset;

    memset(normal_pool.bytes, 0xa5, sizeof(normal_pool.bytes));
    *(unsigned short *)(void *)(normal_pool.bytes + 0x2e) = 0;
    *(unsigned short *)(void *)(normal_pool.bytes + 0x134 + 0x2e) = 0;
    memset(owner_storage.bytes, 0xa5, sizeof(owner_storage.bytes));
    owner = owner_storage.bytes;
    *(long *)(void *)(owner + 0x00) = 0x00123456L;
    *(long *)(void *)(owner + 0x04) = -0x00034567L;
    *(long *)(void *)(owner + 0x08) = 0x00000789L;
    *(unsigned short *)(void *)(owner + 0x2e) = 0x002fU;
    *(unsigned short *)(void *)(owner + 0x114) = 0;
    *(unsigned long *)(void *)(owner + 0x12c) = 0;
    *(unsigned long *)(void *)(owner + 0x130) = 0;
    owner[0x94] = 0x91U;
    owner[0x95] = 0x72U;
    owner[0x96] = 0x53U;
    *(void ***)(void *)(owner + 0x110) = frame_table;
    *(void **)(void *)(owner + 0x64) = 0;

    frame_table[0] = 0;
    frame_table[1] = &image_token;
    memset(normal_assets, 0, sizeof(normal_assets));
    asset = normal_assets + 0x34 * 16;
    *(unsigned long *)(void *)(asset + 0) = (unsigned long)&image_token;
    *(unsigned long *)(void *)(asset + 4) = (unsigned long)frame_table;
    *(unsigned long *)(void *)(asset + 8) = (unsigned long)&steps_token;
    asset[12] = 0x42U;

    memset(action_table, 0, sizeof(action_table));
    *(void (__cdecl **)(void *))(void *)(action_table + 0x34 * 12) =
        callback_probe;

    g_normal_actor_pool = (ActorNormal *)normal_pool.bytes;
    g_alternate_actor_pool = (ActorCore *)normal_pool.bytes;
    g_normal_cursor_primary = 0;
    g_normal_limit_primary = 2;
    g_normal_cursor_alternate = 0;
    g_normal_limit_wrap = allocation_enabled ? 2 : 0;
    g_normal_limit_wrap_start = 0;
    g_alternate_cursor_primary = 0;
    g_alternate_limit_primary = 2;
    g_alternate_cursor_alternate = 0;
    g_alternate_limit_wrap = 2;
    g_alternate_limit_wrap_start = 0;
    g_normal_factory_assets = normal_assets;
    g_alternate_factory_assets = normal_assets;
    g_configuration_dword_71370 = (unsigned long)action_table;
    g_configuration_dword_7137c = 0;
    g_configuration_dword_71390 = 0;
    g_configuration_dword_7139c = 0;
}

static void seed_native(int allocation_enabled)
{
    set_native_pointer(0x72860UL, normal_pool.bytes);
    set_native_pointer(0x71388UL, normal_assets);
    set_native_pointer(0x71370UL, action_table);
    set_native_word(0x713c4UL, 0);
    set_native_word(0x713a8UL,
                    (unsigned short)(allocation_enabled ? 2 : 0));
    set_native_word(0x71378UL, 0);
}

static void run_case(int allocation_enabled)
{
    void (__cdecl *native_callback)(Actor *);
    unsigned long source_calls_before;
    unsigned long native_calls_before;
    unsigned long source_failures_before;
    unsigned long native_failures_before;
    unsigned long native_owner_ptr;

    seed_world(allocation_enabled);
    current_owner = owner_storage.bytes;
    current_side = 0;
    source_calls_before = source_callbacks;
    source_failures_before = source_callback_failures;
    eng1_native_code_target_040e20((Actor *)owner_storage.bytes);
    memcpy(source_owner_after, owner_storage.bytes, sizeof(source_owner_after));
    memcpy(source_pool_after, normal_pool.bytes, sizeof(source_pool_after));

    seed_world(allocation_enabled);
    seed_native(allocation_enabled);
    current_owner = owner_storage.bytes;
    current_side = 1;
    native_calls_before = native_callbacks;
    native_failures_before = native_callback_failures;
    native_callback = (void (__cdecl *)(Actor *))(void *)native_cell(0x40e20UL);
    native_callback((Actor *)owner_storage.bytes);
    memcpy(native_owner_after, owner_storage.bytes, sizeof(native_owner_after));
    memcpy(native_pool_after, normal_pool.bytes, sizeof(native_pool_after));
    native_owner_ptr = (unsigned long)owner_storage.bytes;

    check(memcmp(source_owner_after, native_owner_after,
                 sizeof(source_owner_after)) == 0,
          "owner bytes match, including frame advancement and child link");
    check(memcmp(source_pool_after, native_pool_after,
                 sizeof(source_pool_after)) == 0,
          "complete two-slot normal pool output matches");
    check(source_callbacks - source_calls_before ==
              (allocation_enabled ? 1UL : 0UL) &&
          native_callbacks - native_calls_before ==
              (allocation_enabled ? 1UL : 0UL),
          "kind-0x34 action is called iff normal allocation succeeds");
    check(source_callback_failures == source_failures_before &&
          native_callback_failures == native_failures_before,
          "action sees advanced owner, copied owner bytes, and linked child");
    check(*(unsigned short *)(void *)(source_owner_after + 0x114) == 1U &&
          *(unsigned short *)(void *)(native_owner_after + 0x114) == 1U &&
          *(unsigned long *)(void *)(source_owner_after + 0x64) ==
              (unsigned long)&image_token,
          "frame provider runs before both successful and exhausted spawn");
    if (allocation_enabled) {
        check(*(unsigned long *)(void *)(source_pool_after + 0x00) == 0x00123456UL &&
              *(unsigned long *)(void *)(source_pool_after + 0x04) == 0xfffcba99UL,
              "spawn uses owner coordinates after frame advancement");
        check(source_callback_frame == 1U && native_callback_frame == 1U &&
              source_callback_actor == (unsigned long)normal_pool.bytes &&
              native_callback_actor == (unsigned long)normal_pool.bytes,
              "callback receives the created child after owner link writes");
        check(*(unsigned short *)(void *)(source_pool_after + 0x34) == 0x7fU &&
              (*(unsigned long *)(void *)(source_pool_after + 0x54) & 0x10080UL)
                  == 0x10080UL &&
              *(unsigned short *)(void *)(source_pool_after + 0x3c) == 0x200U,
              "existing kind-0x34 render callback is applied");
        check(*(unsigned long *)(void *)(source_owner_after + 0x130) ==
                  (unsigned long)normal_pool.bytes &&
              *(unsigned long *)(void *)(source_pool_after + 0x12c) == native_owner_ptr,
              "normal child is appended to the same owner chain");
    } else {
        check(*(unsigned long *)(void *)(source_owner_after + 0x130) == 0 &&
              *(unsigned long *)(void *)(native_owner_after + 0x130) == 0,
              "allocation exhaustion does not append an owner child");
    }
    printf("PASS 40e20 allocation=%s\n",
           allocation_enabled ? "success" : "exhausted");
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage: eng1_call_40e20_fixture <ENG1.DLL>\n");
        return 2;
    }
    native_module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (native_module == 0) {
        printf("LoadLibraryExA failed %lu\n", GetLastError());
        return 3;
    }
    memcpy(native_config_before, native_cell(0x71370UL),
           sizeof(native_config_before));
    native_pool_pointer_before = *(unsigned long *)(void *)native_cell(0x72860UL);
    run_case(1);
    run_case(0);

    memcpy(native_cell(0x71370UL), native_config_before,
           sizeof(native_config_before));
    *(unsigned long *)(void *)native_cell(0x72860UL) = native_pool_pointer_before;
    check(memcmp(native_cell(0x71370UL), native_config_before,
                 sizeof(native_config_before)) == 0,
          "all modified native configuration and pool-cursor cells are restored");
    check(*(unsigned long *)(void *)native_cell(0x72860UL) ==
              native_pool_pointer_before,
          "native normal-pool pointer cell is restored");
    if (!FreeLibrary(native_module)) return 4;
    printf("eng1_call_40e20 cases=2 checks=%lu failures=%lu\n",
           checks, failures);
    return failures ? 1 : 0;
}
