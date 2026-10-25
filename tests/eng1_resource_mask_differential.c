#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "../src/shared/engine_interface.h"
#include "../src/shared/resource_record.h"
#include "../src/eng1/setup_shared_state.h"
#include "../src/eng1/simulation_data_owners.h"

extern ActorResource g_actor_resources[81];
extern int g_loading_resource;
extern void __cdecl eng1_native_code_target_028290(unsigned long action);

#define RESOURCE_COUNT 81UL
#define RESOURCE_STRIDE 20UL
#define RESOURCE_STATE_RVA 0x5cef0UL
#define TABLE_RVA 0x53800UL
#define PENDING_RVA 0x72010UL
#define CONTEXT_RVA 0x70f24UL
#define CANCEL_CALLBACK_RVA 0x70f44UL

static HMODULE g_oracle;
static unsigned char *g_image;
static unsigned char g_native_context_a[0x1b34];
static unsigned char g_native_context_b[0x1b34];
static ResourceCallbackContext g_source_context_a;
static ResourceCallbackContext g_source_context_b;
static int g_mutate_on_first_callback;
static unsigned long g_native_callback_count;
static unsigned long g_source_callback_count;
static unsigned long g_native_callback_contexts[8];
static unsigned long g_source_callback_contexts[8];
static unsigned long g_checks;
static unsigned long g_failures;
static int g_quiet_case;
int g_loading_resource;

static const unsigned long g_default_table[8] = {6, 7, 8, 9, 10, 11, 7, 12};
static void __cdecl native_cancel_hook(void);

static unsigned char *native_cell(unsigned long rva)
{
    return g_image + rva;
}

static unsigned long pointer_tag(const void *context, const void *a,
                                 const void *b)
{
    if (context == a)
        return 1UL;
    if (context == b)
        return 2UL;
    return 0UL;
}

static unsigned short context_mask(const unsigned char *context)
{
    unsigned short value;
    memcpy(&value, context + 0x1c, sizeof(value));
    return value;
}

static unsigned short context_word_neighbor(const unsigned char *context)
{
    unsigned short value;
    memcpy(&value, context + 0x1e, sizeof(value));
    return value;
}

static void set_context_mask(unsigned char *context, unsigned short value)
{
    memcpy(context + 0x1c, &value, sizeof(value));
}

static void __cdecl context_callback_sentinel(void)
{
}

static void set_context_callbacks(unsigned char *context)
{
    void (__cdecl *callback)(void) = context_callback_sentinel;
    memcpy(context + 0xb0, &callback, sizeof(callback));
    memcpy(context + 0xb4, &callback, sizeof(callback));
}

static void reset_world(void)
{
    unsigned long i;
    DWORD discarded_protect;
    DWORD old_protect;

    memset(g_native_context_a, 0, sizeof(g_native_context_a));
    memset(g_native_context_b, 0, sizeof(g_native_context_b));
    memset(&g_source_context_a, 0, sizeof(g_source_context_a));
    memset(&g_source_context_b, 0, sizeof(g_source_context_b));
    set_context_callbacks(g_native_context_a);
    set_context_callbacks(g_native_context_b);
    g_source_context_a.load_complete = context_callback_sentinel;
    g_source_context_a.load_failed = context_callback_sentinel;
    g_source_context_b.load_complete = context_callback_sentinel;
    g_source_context_b.load_failed = context_callback_sentinel;

    g_engine_interface.context_004 = &g_source_context_a;
    g_eng1_setup_byte_72010 = 0;
    g_eng1_setup_byte_72011 = 0x5a;
    g_eng1_setup_word_72012 = 0xa55a;
    *(native_cell(PENDING_RVA) + 1) = 0x5a;
    *(unsigned short *)(void *)(native_cell(PENDING_RVA) + 2) = 0xa55a;
    for (i = 0; i < 8; ++i) {
        g_eng1_simulation_lookup_owner_537e0.value_view_53800[i] =
            g_default_table[i];
        *(unsigned long *)(void *)(native_cell(TABLE_RVA) + i * 4UL) =
            g_default_table[i];
    }
    for (i = 0; i < RESOURCE_COUNT; ++i) {
        g_actor_resources[i].residency_state = 0;
        *(unsigned short *)(void *)(native_cell(RESOURCE_STATE_RVA) +
                                    i * RESOURCE_STRIDE) = 0;
    }

    *(unsigned long *)(void *)native_cell(CONTEXT_RVA) =
        (unsigned long)(void *)g_native_context_a;
    *native_cell(PENDING_RVA) = 0;
    *(unsigned long *)(void *)native_cell(CANCEL_CALLBACK_RVA) =
        (unsigned long)(void *)&native_cancel_hook;
    g_mutate_on_first_callback = 0;
    g_native_callback_count = 0;
    g_source_callback_count = 0;
    memset(g_native_callback_contexts, 0, sizeof(g_native_callback_contexts));
    memset(g_source_callback_contexts, 0, sizeof(g_source_callback_contexts));

    if (!VirtualProtect(native_cell(CANCEL_CALLBACK_RVA), 4,
                        PAGE_READWRITE, &old_protect)) {
        printf("FAIL VirtualProtect callback cell error=%lu\n", GetLastError());
        ++g_failures;
    } else {
        VirtualProtect(native_cell(CANCEL_CALLBACK_RVA), 4, old_protect,
                       &discarded_protect);
    }
}

static void __cdecl native_cancel_hook(void)
{
    unsigned long index = g_native_callback_count;
    unsigned char *current = *(unsigned char **)(void *)native_cell(CONTEXT_RVA);
    if (index < 8UL)
        g_native_callback_contexts[index] = pointer_tag(
            current, g_native_context_a, g_native_context_b);
    ++g_native_callback_count;
    if (g_mutate_on_first_callback && index == 0) {
        *native_cell(PENDING_RVA) = 0x40;
        *(unsigned long *)(void *)(native_cell(TABLE_RVA) + 4) = 12;
        *(unsigned long *)(void *)native_cell(CONTEXT_RVA) =
            (unsigned long)(void *)g_native_context_b;
    }
}

void __cdecl host_cancel_load(void)
{
    unsigned long index = g_source_callback_count;
    ResourceCallbackContext *current = g_engine_interface.context_004;
    if (index < 8UL)
        g_source_callback_contexts[index] = pointer_tag(
            current, &g_source_context_a, &g_source_context_b);
    ++g_source_callback_count;
    if (g_mutate_on_first_callback && index == 0) {
        g_eng1_setup_byte_72010 = 0x40;
        g_eng1_simulation_lookup_owner_537e0.value_view_53800[1] = 12;
        g_engine_interface.context_004 = &g_source_context_b;
    }
}

void __cdecl resource_destroy(int resource_id)
{
    (void)resource_id;
}

static void compare_case(const char *name, unsigned long action,
                         int mutate_on_callback)
{
    unsigned long i;
    unsigned char source_pending_before;
    unsigned char native_pending_before;
    unsigned char source_neighbor_before;
    unsigned char native_neighbor_before;
    unsigned short source_word_neighbor_before;
    unsigned short native_word_neighbor_before;
    unsigned long source_context_tag;
    unsigned long native_context_tag;
    unsigned long source_context_before;
    unsigned long native_context_before;
    unsigned long source_callbacks_before;
    unsigned long native_callbacks_before;

    source_pending_before = g_eng1_setup_byte_72010;
    native_pending_before = *native_cell(PENDING_RVA);
    source_neighbor_before = g_eng1_setup_byte_72011;
    native_neighbor_before = *(native_cell(PENDING_RVA) + 1);
    source_word_neighbor_before = g_eng1_setup_word_72012;
    native_word_neighbor_before = *(unsigned short *)(void *)(native_cell(PENDING_RVA) + 2);
    source_context_before = pointer_tag(g_engine_interface.context_004,
                                        &g_source_context_a,
                                        &g_source_context_b);
    native_context_before = pointer_tag(
        *(unsigned char **)(void *)native_cell(CONTEXT_RVA),
        g_native_context_a, g_native_context_b);
    source_callbacks_before = g_source_callback_count;
    native_callbacks_before = g_native_callback_count;
    if (source_pending_before != native_pending_before ||
        source_neighbor_before != native_neighbor_before ||
        source_word_neighbor_before != native_word_neighbor_before ||
        source_context_before != native_context_before) {
        printf("FAIL %s: initial state mask=%02x/%02x neighbor=%02x/%02x word=%04x/%04x context=%lu/%lu\n",
               name, source_pending_before, native_pending_before,
               source_neighbor_before, native_neighbor_before,
               source_word_neighbor_before, native_word_neighbor_before,
               source_context_before, native_context_before);
        ++g_failures;
    }

    g_mutate_on_first_callback = mutate_on_callback;
    eng1_native_code_target_028290(action);
    ((void (__cdecl *)(unsigned long))(void *)(g_image + 0x28290))(action);

    if (g_eng1_setup_byte_72010 != *native_cell(PENDING_RVA)) {
        printf("FAIL %s: pending byte source=%02x native=%02x\n", name,
               g_eng1_setup_byte_72010, *native_cell(PENDING_RVA));
        ++g_failures;
    }
    if (g_eng1_setup_byte_72011 != *(native_cell(PENDING_RVA) + 1)) {
        printf("FAIL %s: adjacent byte source=%02x native=%02x\n", name,
               g_eng1_setup_byte_72011, *(native_cell(PENDING_RVA) + 1));
        ++g_failures;
    }
    if (g_eng1_setup_word_72012 !=
        *(unsigned short *)(void *)(native_cell(PENDING_RVA) + 2)) {
        printf("FAIL %s: adjacent word source=%04x native=%04x\n", name,
               g_eng1_setup_word_72012,
               *(unsigned short *)(void *)(native_cell(PENDING_RVA) + 2));
        ++g_failures;
    }
    for (i = 0; i < 8; ++i) {
        unsigned long source_value =
            g_eng1_simulation_lookup_owner_537e0.value_view_53800[i];
        unsigned long native_value =
            *(unsigned long *)(void *)(native_cell(TABLE_RVA) + i * 4UL);
        if (source_value != native_value) {
            printf("FAIL %s: table[%lu] source=%lu native=%lu\n", name,
                   i, source_value, native_value);
            ++g_failures;
        }
    }
    for (i = 0; i < RESOURCE_COUNT; ++i) {
        unsigned short source_state = g_actor_resources[i].residency_state;
        unsigned short native_state = *(unsigned short *)(void *)(
            native_cell(RESOURCE_STATE_RVA) + i * RESOURCE_STRIDE);
        if (source_state != native_state) {
            printf("FAIL %s: resource[%lu] state source=%u native=%u\n",
                   name, i, source_state, native_state);
            ++g_failures;
        }
    }
    source_context_tag = pointer_tag(g_engine_interface.context_004,
                                     &g_source_context_a,
                                     &g_source_context_b);
    native_context_tag = pointer_tag(
        *(unsigned char **)(void *)native_cell(CONTEXT_RVA),
        g_native_context_a, g_native_context_b);
    if (source_context_tag != native_context_tag) {
        printf("FAIL %s: context tag source=%lu native=%lu\n", name,
               source_context_tag, native_context_tag);
        ++g_failures;
    }
    if (context_mask((unsigned char *)&g_source_context_a) !=
            context_mask(g_native_context_a) ||
        context_mask((unsigned char *)&g_source_context_b) !=
            context_mask(g_native_context_b) ||
        context_word_neighbor((unsigned char *)&g_source_context_a) !=
            context_word_neighbor(g_native_context_a) ||
        context_word_neighbor((unsigned char *)&g_source_context_b) !=
            context_word_neighbor(g_native_context_b)) {
        printf("FAIL %s: context masks or adjacent WORD canaries differ\n", name);
        ++g_failures;
    }
    if ((g_source_context_a.load_complete != 0) !=
            (*(unsigned long *)(void *)(g_native_context_a + 0xb0) != 0) ||
        (g_source_context_a.load_failed != 0) !=
            (*(unsigned long *)(void *)(g_native_context_a + 0xb4) != 0) ||
        (g_source_context_b.load_complete != 0) !=
            (*(unsigned long *)(void *)(g_native_context_b + 0xb0) != 0) ||
        (g_source_context_b.load_failed != 0) !=
            (*(unsigned long *)(void *)(g_native_context_b + 0xb4) != 0)) {
        printf("FAIL %s: callback slots differ\n", name);
        ++g_failures;
    }
    if (g_source_callback_count - source_callbacks_before !=
        g_native_callback_count - native_callbacks_before) {
        printf("FAIL %s: callback count source=%lu native=%lu\n", name,
               g_source_callback_count - source_callbacks_before,
               g_native_callback_count - native_callbacks_before);
        ++g_failures;
    }
    for (i = source_callbacks_before;
         i < g_source_callback_count &&
         i - source_callbacks_before < 8UL; ++i) {
        unsigned long relative = i - source_callbacks_before;
        if (g_source_callback_contexts[i] !=
            g_native_callback_contexts[native_callbacks_before + relative]) {
            printf("FAIL %s: callback context[%lu] source=%lu native=%lu\n",
                   name, relative, g_source_callback_contexts[i],
                   g_native_callback_contexts[native_callbacks_before + relative]);
            ++g_failures;
        }
    }
    ++g_checks;
    if (g_failures == 0 && !g_quiet_case)
        printf("PASS %s action=%08lx pending=%02x->%02x callbacks=%lu context=%lu->%lu\n",
               name, action, source_pending_before, g_eng1_setup_byte_72010,
               g_source_callback_count - source_callbacks_before,
               source_context_before, source_context_tag);
}

static void run_mask_domain_matrix(int cancel_path)
{
    static const unsigned short states[6] = {0, 1, 2, 3, 4, 0x1357};
    unsigned long mask;
    unsigned long id;
    char label[64];
    unsigned long failures_before = g_failures;
    unsigned long checks_before = g_checks;

    g_quiet_case = 1;
    for (mask = 0; mask < 256UL; ++mask) {
        unsigned short context_neighbor;
        reset_world();
        g_eng1_setup_byte_72010 = (unsigned char)((mask * 37UL + 0x53UL) & 0xffUL);
        *native_cell(PENDING_RVA) = (unsigned char)((mask * 37UL + 0x53UL) & 0xffUL);
        g_eng1_setup_byte_72011 = (unsigned char)(mask ^ 0x96UL);
        *(native_cell(PENDING_RVA) + 1) = (unsigned char)(mask ^ 0x96UL);
        g_eng1_setup_word_72012 = (unsigned short)(0xa500U | mask);
        *(unsigned short *)(void *)(native_cell(PENDING_RVA) + 2) =
            (unsigned short)(0xa500U | mask);
        set_context_mask((unsigned char *)&g_source_context_a,
                         (unsigned short)((mask * 29UL + 0x35UL) & 0xffUL));
        set_context_mask(g_native_context_a,
                         (unsigned short)((mask * 29UL + 0x35UL) & 0xffUL));
        context_neighbor = (unsigned short)(0x5a00U | mask);
        memcpy((unsigned char *)&g_source_context_a + 0x1e,
               &context_neighbor, 2);
        memcpy(g_native_context_a + 0x1e, &context_neighbor, 2);
        for (id = 0; id < RESOURCE_COUNT; ++id) {
            unsigned short state = states[(mask + id) % 6UL];
            g_actor_resources[id].residency_state = state;
            *(unsigned short *)(void *)(native_cell(RESOURCE_STATE_RVA) +
                                        id * RESOURCE_STRIDE) = state;
        }
        sprintf(label, "%s low-byte=%02lx",
                cancel_path ? "cancel-mask" : "reset-mask", mask);
        compare_case(label,
                     (cancel_path ? 0x5a5a0c00UL : 0x5a5a0400UL) | mask,
                     0);
    }
    g_quiet_case = 0;
    if (g_failures == failures_before)
        printf("PASS %s all 256 low-byte masks; pending/context masks, state classes, BYTE/WORD canaries\n",
               cancel_path ? "cancel-mask" : "reset-mask");
    else
        printf("FAIL %s domain matrix: failures=%lu across %lu cases\n",
               cancel_path ? "cancel-mask" : "reset-mask",
               g_failures - failures_before, g_checks - checks_before);
}

static void compare_default_records(void)
{
    unsigned long i;
    for (i = 0; i < RESOURCE_COUNT; ++i) {
        unsigned char *native_record = native_cell(0x5cee0UL) +
                                       i * RESOURCE_STRIDE;
        ActorResource *source_record = &g_actor_resources[i];
        const char *native_path = *(const char **)(void *)native_record;
        if (source_record->entry_count !=
            *(unsigned short *)(void *)(native_record + 4) ||
            source_record->unknown_006 !=
            *(unsigned short *)(void *)(native_record + 6) ||
            source_record->file_length !=
            *(unsigned long *)(void *)(native_record + 8) ||
            source_record->residency_state !=
            *(unsigned short *)(void *)(native_record + 16) ||
            !native_path || !source_record->path ||
            strcmp((const char *)source_record->path, native_path) != 0) {
            printf("FAIL provider default record[%lu] differs\n", i);
            ++g_failures;
        }
    }
    for (i = 0; i < 8; ++i) {
        if (g_eng1_simulation_lookup_owner_537e0.value_view_53800[i] !=
            *(unsigned long *)(void *)(native_cell(TABLE_RVA) + i * 4UL)) {
            printf("FAIL provider default table[%lu] differs\n", i);
            ++g_failures;
        }
    }
    ++g_checks;
    if (g_failures == 0)
        printf("PASS 81 initialized resource_data rows and all eight default table values\n");
}

int main(int argc, char **argv)
{
    DWORD old_protect;
    unsigned long i;
    if (argc != 2) {
        printf("usage: resource_mask_differential ENG1.DLL\n");
        return 2;
    }
    g_oracle = LoadLibraryExA(argv[1], NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!g_oracle) {
        printf("LoadLibraryExA failed %lu\n", GetLastError());
        return 2;
    }
    g_image = (unsigned char *)g_oracle;
    if (!VirtualProtect(native_cell(CANCEL_CALLBACK_RVA), 4,
                        PAGE_READWRITE, &old_protect)) {
        printf("VirtualProtect callback cell failed %lu\n", GetLastError());
        FreeLibrary(g_oracle);
        return 2;
    }
    compare_default_records();

    reset_world();
    compare_case("all-eight-default-mask-reset", 0x123004ffUL, 0);

    reset_world();
    g_actor_resources[10].residency_state = 0;
    *(unsigned short *)(void *)(native_cell(RESOURCE_STATE_RVA) + 10UL * 20UL) = 0;
    compare_case("scalar-reset", 0x1230000aUL, 0);

    reset_world();
    g_actor_resources[10].residency_state = 2;
    *(unsigned short *)(void *)(native_cell(RESOURCE_STATE_RVA) + 10UL * 20UL) = 2;
    compare_case("scalar-cancel-with-context-clear", 0x1230080aUL, 0);

    reset_world();
    g_eng1_setup_byte_72010 = 0x40;
    *native_cell(PENDING_RVA) = 0x40;
    g_eng1_setup_byte_72011 = 0xa5;
    *(native_cell(PENDING_RVA) + 1) = 0xa5;
    set_context_mask((unsigned char *)&g_source_context_a, 0x42);
    set_context_mask(g_native_context_a, 0x42);
    g_actor_resources[8].residency_state = 3;
    *(unsigned short *)(void *)(native_cell(RESOURCE_STATE_RVA) + 8UL * 20UL) = 3;
    compare_case("narrow-reset-context-mask-and-state-three-skip",
                 0x12300407UL, 0);

    reset_world();
    g_eng1_setup_byte_72010 = 0x83;
    *native_cell(PENDING_RVA) = 0x83;
    g_actor_resources[6].residency_state = 2;
    g_actor_resources[7].residency_state = 2;
    g_actor_resources[12].residency_state = 2;
    *(unsigned short *)(void *)(native_cell(RESOURCE_STATE_RVA) + 6UL * 20UL) = 2;
    *(unsigned short *)(void *)(native_cell(RESOURCE_STATE_RVA) + 7UL * 20UL) = 2;
    *(unsigned short *)(void *)(native_cell(RESOURCE_STATE_RVA) + 12UL * 20UL) = 2;
    g_mutate_on_first_callback = 1;
    compare_case("cached-cancel-mask-live-table-and-context-reload",
                 0x12300c03UL, 1);

    run_mask_domain_matrix(0);
    run_mask_domain_matrix(1);

    for (i = 0; i < 8; ++i) {
        if (g_default_table[i] !=
            g_eng1_simulation_lookup_owner_537e0.value_view_53800[i]) {
            /* The mutation case intentionally changes slot one. */
            if (i != 1 || g_eng1_simulation_lookup_owner_537e0.value_view_53800[i] != 12)
                ++g_failures;
        }
    }
    printf("checks=%lu failures=%lu\n", g_checks, g_failures);
    FreeLibrary(g_oracle);
    return g_failures ? 1 : 0;
}
