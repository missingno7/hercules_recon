#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "../src/eng1/interpreters.h"
#include "../src/eng1/resource_runtime_data.h"
#include "../src/eng1/simulation_data_owners.h"
#include "../src/shared/engine_interface.h"
#undef g_special_kind

extern void __cdecl eng1_native_code_target_012c00(Actor *actor);
extern unsigned short g_eng1_config_word_pool_533c8[282];

static HMODULE g_oracle;
static void (__cdecl *g_native_entry)(Actor *);
static unsigned long g_checks;
static unsigned long g_failures;
static unsigned char g_saved_context_cell[4];
static unsigned char g_saved_kind_info[8];
static unsigned char g_saved_active_row;
static ResourceCallbackContext *g_saved_source_context;
static ResourceKindInfo g_saved_source_kind_info;
static signed char g_saved_source_active_row;
static int g_native_snapshot_ready;
static int g_source_snapshot_ready;
static unsigned long g_spy_spawn_calls;
static unsigned long g_spy_detach_calls;
static unsigned long g_spy_cleanup_calls;
static unsigned long g_unresolved_ef10_calls;
static Actor *g_last_ef10_actor;
static unsigned char g_ef10_observed_update_state;

#define CHECK(condition, message) do { \
    ++g_checks; \
    if (!(condition)) { \
        ++g_failures; \
        printf("FAIL line %d: %s\n", __LINE__, message); \
        goto cleanup; \
    } \
} while (0)

static unsigned char *native_cell(unsigned long rva)
{
    return (unsigned char *)g_oracle + rva;
}

static void save_cells(void)
{
    memcpy(g_saved_context_cell, native_cell(0x70f24UL), 4UL);
    memcpy(g_saved_kind_info, native_cell(0x6fee8UL), 8UL);
    g_saved_active_row = *(signed char *)native_cell(0x70d01UL);
    g_saved_source_context = g_engine_interface.context_004;
    g_saved_source_kind_info = g_kind_info[0];
    g_saved_source_active_row = g_active_row_70d01;
    g_native_snapshot_ready = 1;
    g_source_snapshot_ready = 1;
}

static void restore_cells(void)
{
    if (g_oracle != NULL && g_native_snapshot_ready) {
        memcpy(native_cell(0x70f24UL), g_saved_context_cell, 4UL);
        memcpy(native_cell(0x6fee8UL), g_saved_kind_info, 8UL);
        *(signed char *)native_cell(0x70d01UL) = g_saved_active_row;
    }
    if (g_source_snapshot_ready) {
        g_engine_interface.context_004 = g_saved_source_context;
        g_kind_info[0] = g_saved_source_kind_info;
        g_active_row_70d01 = g_saved_source_active_row;
    }
}

static void make_actor(Actor *actor, unsigned short kind, unsigned long offset)
{
    memset(actor, 0xa5, sizeof(*actor));
    actor->x = 0x00010000L;
    actor->y = 0x00020000L;
    actor->z = 0x00030000L;
    actor->unknown_02a = 0x5151U;
    actor->kind = kind;
    actor->frame_ticks = 0x1234;
    actor->frame_state = 0x2345;
    actor->frame_group = 0U;
    actor->flags = 0x00000000UL;
    actor->render_flags = 0x12345600UL;
    actor->unknown_058 = (void *)offset;
    actor->image_or_program = (void *)0x22222222UL;
    *((unsigned char *)(void *)actor + 0x25) = 0x7fU;
}

static void set_context(unsigned char *native_context,
                        unsigned char *source_context,
                        unsigned char mode, unsigned char transition,
                        unsigned short pending)
{
    memset(native_context, 0, 0x80UL);
    memset(source_context, 0, 0x80UL);
    native_context[0] = mode;
    source_context[0] = mode;
    native_context[0x0c] = transition;
    source_context[0x0c] = transition;
    *(unsigned short *)(void *)(native_context + 0x1e) = pending;
    *(unsigned short *)(void *)(source_context + 0x1e) = pending;
    *(unsigned long *)native_cell(0x70f24UL) =
        (unsigned long)native_context;
    g_engine_interface.context_004 =
        (ResourceCallbackContext *)source_context;
}

static int compare_actor(Actor *native_actor, Actor *source_actor,
                         unsigned long expected_native_pointer,
                         const void *expected_source_pointer,
                         const char *case_name)
{
    unsigned char native_copy[sizeof(Actor)];
    unsigned char source_copy[sizeof(Actor)];
    memcpy(native_copy, native_actor, sizeof(Actor));
    memcpy(source_copy, source_actor, sizeof(Actor));
    if (expected_native_pointer != 0UL) {
        if (*(unsigned long *)(void *)(native_copy + 0x64) !=
            expected_native_pointer) {
            printf("FAIL %s native pointer-valued field\n", case_name);
            ++g_failures;
            return 0;
        }
        ++g_checks;
        if (*(void **)(void *)(source_copy + 0x64) !=
            expected_source_pointer) {
            printf("FAIL %s source pointer-valued field\n", case_name);
            ++g_failures;
            return 0;
        }
        ++g_checks;
        memset(native_copy + 0x64, 0, 4UL);
        memset(source_copy + 0x64, 0, 4UL);
    }
    if (memcmp(native_copy, source_copy, sizeof(Actor)) != 0) {
        printf("FAIL %s actor differs: native=%p source=%p\n",
               case_name, (void *)native_actor, (void *)source_actor);
        ++g_failures;
        return 0;
    }
    ++g_checks;
    return 1;
}

static int run_pair(const char *name, unsigned short kind,
                    unsigned long offset, unsigned char mode,
                    unsigned char transition, unsigned short pending,
                    unsigned long flags, unsigned long render_flags,
                    unsigned long expected_native_pointer,
                    const void *expected_source_pointer)
{
    unsigned char native_context[0x80];
    unsigned char source_context[0x80];
    Actor native_actor, source_actor;

    set_context(native_context, source_context, mode, transition, pending);
    make_actor(&native_actor, kind, offset);
    make_actor(&source_actor, kind, offset);
    native_actor.flags = source_actor.flags = flags;
    native_actor.render_flags = source_actor.render_flags = render_flags;
    if (kind == 0x2002U)
        *(unsigned short *)(void *)((unsigned char *)&native_actor + 0x38) = 0U;
    if (kind == 0x2002U)
        *(unsigned short *)(void *)((unsigned char *)&source_actor + 0x38) = 0U;

    g_native_entry(&native_actor);
    eng1_native_code_target_012c00(&source_actor);
    if (!compare_actor(&native_actor, &source_actor,
                       expected_native_pointer, expected_source_pointer, name))
        return 0;
    printf("case=%s native/source actor bytes agree\n", name);
    return 1;
}

/* Link-only spies for paths excluded by the measured controls below. */
void __cdecl detach_child(void *actor)
{
    (void)actor;
    ++g_spy_detach_calls;
}
void __cdecl detach_buffer(void *actor) { (void)actor; ++g_spy_cleanup_calls; }
void __cdecl detach_attachment(void *actor) { (void)actor; ++g_spy_cleanup_calls; }
void __cdecl release_actor_resource_state(void *actor)
{ (void)actor; ++g_spy_cleanup_calls; }
void __cdecl release_actor_token(void *actor)
{ (void)actor; ++g_spy_cleanup_calls; }
int __cdecl remove_actor(Actor *actor)
{ (void)actor; ++g_spy_cleanup_calls; return 0; }
Actor *__cdecl spawn_kind(long x, long y, long z, unsigned short kind)
{
    (void)x; (void)y; (void)z; (void)kind;
    ++g_spy_spawn_calls;
    return NULL;
}
void __cdecl eng1_native_code_target_000ef10(Actor *actor)
{
    g_last_ef10_actor = actor;
    g_ef10_observed_update_state =
        *((unsigned char *)(void *)actor + 0x25);
    ++g_unresolved_ef10_calls;
}
signed char g_special_kind;

int main(int argc, char **argv)
{
    int result = 1;
    unsigned char *native_kind_info;
    unsigned long kind_info_pointer;
    (void)argc; (void)argv;

    g_oracle = LoadLibraryExA(argv[1], NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (g_oracle == NULL) {
        printf("LoadLibraryExA failed: %lu\n", GetLastError());
        return 2;
    }
    g_native_entry = (void (__cdecl *)(Actor *))native_cell(0x12c00UL);
    save_cells();

    /* The native and source kind-info entries point at corresponding measured
     * byte owners. The 0x200b fixture reads owner+0x40+0x0c = 0x5366c, a WORD
     * value 2, then lookup entry 2 (within the 64-DWORD 0x537e0 prefix). */
    native_kind_info = native_cell(0x6fee8UL);
    kind_info_pointer = (unsigned long)(native_cell(0x53620UL));
    *(unsigned long *)(void *)native_kind_info = kind_info_pointer;
    *(unsigned long *)(void *)(native_kind_info + 4) = 0UL;
    g_kind_info[0].owner = g_eng1_simulation_data_bytes_53620;
    g_kind_info[0].count = 0UL;

    /* 0x2012 transition: context+0x0c==9 sets kind 0x2013 and pointer 0x53550;
     * context[0]==9 makes the real local helper return after the transition. */
    if (!run_pair("transition_2012", 0x2012U, 0UL, 9U, 9U, 0U,
                  0UL, 0x12345600UL,
                  (unsigned long)(native_cell(0x53550UL)),
                  &g_eng1_config_word_pool_533c8[196]))
        goto cleanup;

    /* The independent 0x2002 gate reads a WORD at context+0x1e. */
    if (!run_pair("transition_2002", 0x2002U, 0UL, 9U, 0U, 0x00ffU,
                  0UL, 0x12345600UL,
                  (unsigned long)(native_cell(0x53550UL)),
                  &g_eng1_config_word_pool_533c8[196]))
        goto cleanup;

    /* Execute the recovered 0x27d40 wrapper under its native early-out flag;
     * the controlled link-only factory/detach spies must remain unreachable. */
    if (!run_pair("spawn_blocked_2015", 0x2015U, 0UL, 9U, 0U, 0U,
                  0x40000000UL, 0x12345600UL, 0UL, NULL))
        goto cleanup;

    /* Exercise nonzero relative offset, frame-group row zero, masked table
     * index 2, and the local helper's context-level return. */
    if (!run_pair("kind_200b_table_index_2", 0x200bU, 0x40UL, 9U, 0U, 0U,
                  0UL, 0x12345600UL,
                  (unsigned long)(native_cell(0x536a0UL)),
                  g_eng1_simulation_data_bytes_53620 + 0x80UL))
        goto cleanup;

    /* Source-only: selected 0x12d10 entries call 0x2a290 and fall through to
     * unresolved 0xef10. The test spy sees actor+0x25 after the real
     * actor-update source has set its low bit; it cannot establish 0xef10's
     * native behavior. The active-row -1 sentinel bounds the grid dependency. */
    g_active_row_70d01 = (signed char)-1;
    {
        unsigned char native_context[0x80], source_context[0x80];
        Actor source_actor;
        set_context(native_context, source_context, 0U, 0U, 0U);
        make_actor(&source_actor, 0x2002U, 0UL);
        *((unsigned char *)(void *)&source_actor + 0x25) = 0x7eU;
        eng1_native_code_target_012c00(&source_actor);
        CHECK(g_unresolved_ef10_calls == 1UL,
              "selected helper path must reach 0xef10 after actor update");
        CHECK(g_last_ef10_actor == &source_actor,
              "selected helper path must pass actor pointer to 0xef10");
        CHECK(g_ef10_observed_update_state == 0x7fU,
              "0xef10 spy must observe actor-update side effect first");
        CHECK(*((unsigned char *)(void *)&source_actor + 0x25) == 0x7fU,
              "actor-update source must set byte+0x25 before 0xef10");
        printf("case=source_only_actor_update_then_ef10_order\n");
    }

    /* The 0x200b null-offset branch calls existing 0x27e20. This fixture links
     * its calibration source only for the bounded kind-bit release case; the
     * render guard keeps clear_actor's resource/token calls unreachable. */
    if (!run_pair("release_200b_guarded_clear", 0x200bU, 0UL, 9U, 0U, 0U,
                  0x10000000UL, 0x10000800UL, 0UL, NULL))
        goto cleanup;

    /* Source-only default dispatch probe. */
    {
        unsigned char native_context[0x80], source_context[0x80];
        Actor source_actor;
        set_context(native_context, source_context, 0U, 0U, 0U);
        make_actor(&source_actor, 0x2003U, 0UL);
        eng1_native_code_target_012c00(&source_actor);
        CHECK(g_unresolved_ef10_calls == 2UL,
              "local helper must route unsupported kind 0x2003 to typed 0xef10");
        CHECK(g_last_ef10_actor == &source_actor,
              "local helper must pass actor pointer to typed 0xef10");
        printf("case=source_only_ef10_dispatch_spy\n");
    }

    CHECK(g_spy_spawn_calls == 0UL, "blocked action must not call spawn_kind spy");
    CHECK(g_spy_detach_calls == 0UL, "fixture paths must not call detach_child spy");
    CHECK(g_spy_cleanup_calls == 0UL, "guarded release must not call cleanup spies");
    CHECK(g_unresolved_ef10_calls == 2UL, "only source-only helper probes may reach 0xef10");
    printf("eng1_call_12c00 native_cases=5 source_only_cases=2 checks=%lu failures=%lu\n",
           g_checks, g_failures);
    result = g_failures == 0UL ? 0 : 1;
    goto cleanup;

cleanup:
    restore_cells();
    if (g_oracle != NULL) {
        if (!FreeLibrary(g_oracle) && result == 0)
            result = 3;
        g_oracle = NULL;
    }
    if (g_failures != 0UL)
        result = 1;
    return result;
}
