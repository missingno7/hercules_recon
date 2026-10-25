#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "../src/shared/engine_interface.h"
#include "../src/eng1/data_layouts.h"
#include "../src/eng1/incomplete.h"
#include "../src/eng1/actor_pipeline_storage.h"
#include "../src/eng1/simulation_data_owners.h"
#include "../src/eng1/setup_shared_state.h"
#include "../src/eng1/script_54558_owner.h"

extern void __cdecl dispatch_local_15090(void);
extern void __cdecl dispatch_local_16790(void);
extern void __cdecl dispatch_local_19440(void);

EngineInterface g_engine_interface;
ENG1ActorPipelineStorage71440 g_actor_pipeline_storage_71440;
void *g_normal_actor_pool;
void *g_middle_actor_pool;
unsigned char *g_collision_context;
signed char g_active_category_70cfe;
unsigned long g_resource_transition_ticks_6efb0;
unsigned long eng1_sim_state_72104, eng1_sim_state_72174;
unsigned long eng1_sim_state_72110, eng1_sim_state_72170;
unsigned short eng1_sim_state_72108, eng1_sim_state_6feb4;
unsigned char g_cold_start_state_bytes_70820[0x1c];
Eng1FrameDescriptorRow24 eng1_descriptor_rows_5a690[12];

long g_camera_x_70ca0, g_camera_y_70ca4, g_camera_z_70ca8;
long g_camera_center_x_70cdc, g_camera_center_y_70ce0, g_camera_focal_70ce4;
unsigned char g_camera_iterations_70cfd;
short g_camera_adjust_x_70d02, g_camera_adjust_y_70d04;
short g_camera_depth_low_70d0c, g_camera_depth_high_70d0e;
unsigned short g_camera_flags_70d10;
long g_camera_z_base_70d1c;

static jmp_buf incomplete_boundary;
static unsigned long incomplete_rva;
static unsigned int middle_calls, normal_calls, attach_calls, failures, checks;
static unsigned int untouched_slot_calls;
static void *middle_args[3];
static void *normal_arg;
static int redirect_context_on_normal_init;
static int allow_15d00_return, allow_2a290_return, allow_27d40_return;
static int change_pool_on_first_15d00;
static int change_collision_on_second_15d00;
static unsigned int resource_calls, post_handler_calls, action_lookup_calls;
static void *post_handler_actor, *action_lookup_actor, *action_lookup_result;
static unsigned long action_lookup_code;
static void *attach_actor_arg;
static int attach_actor_kind;

static void __cdecl untouched_slot_probe(void)
{
    ++untouched_slot_calls;
}

static unsigned char context_bytes[sizeof(ResourceCallbackContext)];
static unsigned char normal_actor[0x200];
static unsigned char alternate_context_actor[0x200];
static unsigned char post_callback_actor[0x200];
static unsigned char late_callback_context_actor[0x200];
static unsigned char action_result_actor[0x200];
static unsigned char middle_pool[0x400];
static unsigned char child_rows[4][12][24];

void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    (void)reason;
    incomplete_rva = rva;
    longjmp(incomplete_boundary, 1);
}

unsigned long __cdecl eng1_native_code_target_0015d00(void)
{
    ++resource_calls;
    if (allow_15d00_return) {
        if (change_pool_on_first_15d00 && resource_calls == 1)
            g_normal_actor_pool = post_callback_actor;
        if (change_collision_on_second_15d00 && resource_calls == 2)
            g_collision_context = late_callback_context_actor;
        return 0;
    }
    eng1_incomplete_behavior(0x15d00UL,
        "test boundary for unrecovered native 0x15d00");
}

void __cdecl eng1_native_code_target_0018f60(void)
{
    eng1_incomplete_behavior(0x18f60UL,
        "typed unresolved native target 0x18f60");
}

void __cdecl eng1_native_code_target_0019160(void)
{
    eng1_incomplete_behavior(0x19160UL,
        "typed unresolved native target 0x19160");
}

void __cdecl eng1_native_code_target_002a290(void *actor)
{
    ++post_handler_calls;
    post_handler_actor = actor;
    if (allow_2a290_return)
        return;
    eng1_incomplete_behavior(0x2a290UL,
        "typed unresolved native target 0x2a290");
}

void *__cdecl eng1_native_code_target_0027d40(void *actor,
                                               unsigned long action)
{
    ++action_lookup_calls;
    action_lookup_actor = actor;
    action_lookup_code = action;
    if (allow_27d40_return)
        return action_lookup_result;
    eng1_incomplete_behavior(0x27d40UL,
        "typed unresolved pointer-returning native target 0x27d40");
    return 0;
}

/* Test-only missing code endpoints; production uses centralized fatal entries. */
#define SETUP_MISSING_SPY(name, rva) void __cdecl name(void) { eng1_incomplete_behavior(rva, "unrecovered fixture endpoint"); }
SETUP_MISSING_SPY(eng1_native_code_target_0016840, 0x0016840UL)
SETUP_MISSING_SPY(eng1_native_code_target_0016b40, 0x0016b40UL)
SETUP_MISSING_SPY(eng1_native_code_target_0016da0, 0x0016da0UL)
SETUP_MISSING_SPY(eng1_native_code_target_0016f40, 0x0016f40UL)
SETUP_MISSING_SPY(eng1_native_code_target_00171b0, 0x00171b0UL)
SETUP_MISSING_SPY(eng1_native_code_target_0017400, 0x0017400UL)
SETUP_MISSING_SPY(eng1_native_code_target_00177a0, 0x00177a0UL)
SETUP_MISSING_SPY(eng1_native_code_target_00178f0, 0x00178f0UL)
SETUP_MISSING_SPY(eng1_native_code_target_0017a90, 0x0017a90UL)
SETUP_MISSING_SPY(eng1_native_code_target_0017c30, 0x0017c30UL)
SETUP_MISSING_SPY(eng1_native_code_target_0017c70, 0x0017c70UL)
SETUP_MISSING_SPY(eng1_native_code_target_0017cd0, 0x0017cd0UL)
SETUP_MISSING_SPY(eng1_native_code_target_0017eb0, 0x0017eb0UL)
SETUP_MISSING_SPY(eng1_native_code_target_0018240, 0x0018240UL)
SETUP_MISSING_SPY(eng1_native_code_target_00182c0, 0x00182c0UL)
SETUP_MISSING_SPY(eng1_native_code_target_0018550, 0x0018550UL)
SETUP_MISSING_SPY(eng1_native_code_target_0018770, 0x0018770UL)
SETUP_MISSING_SPY(eng1_native_code_target_0018830, 0x0018830UL)
SETUP_MISSING_SPY(eng1_native_code_target_0018a10, 0x0018a10UL)
SETUP_MISSING_SPY(eng1_native_code_target_0018ba0, 0x0018ba0UL)
SETUP_MISSING_SPY(eng1_native_code_target_0018c30, 0x0018c30UL)
SETUP_MISSING_SPY(eng1_native_code_target_0018ed0, 0x0018ed0UL)
SETUP_MISSING_SPY(eng1_native_code_target_0019140, 0x0019140UL)
SETUP_MISSING_SPY(eng1_native_code_target_00191e0, 0x00191e0UL)
SETUP_MISSING_SPY(eng1_native_code_target_0019260, 0x0019260UL)
SETUP_MISSING_SPY(eng1_native_code_target_0019290, 0x0019290UL)
SETUP_MISSING_SPY(eng1_native_code_target_00193f0, 0x00193f0UL)
SETUP_MISSING_SPY(eng1_native_code_target_0019480, 0x0019480UL)
SETUP_MISSING_SPY(eng1_native_code_target_0019c20, 0x0019c20UL)
SETUP_MISSING_SPY(eng1_native_code_target_00379e0, 0x00379e0UL)
SETUP_MISSING_SPY(eng1_native_code_target_00374c0, 0x374c0UL)
#undef SETUP_MISSING_SPY

void __cdecl initialize_middle_actor(void *actor)
{
    if (middle_calls < 3)
        middle_args[middle_calls] = actor;
    ++middle_calls;
}

void __cdecl initialize_normal_actor(void *actor)
{
    ++normal_calls;
    normal_arg = actor;
    if (redirect_context_on_normal_init)
        g_collision_context = alternate_context_actor;
}

void *__cdecl attach_actor(void *actor, int kind)
{
    attach_actor_arg = actor;
    attach_actor_kind = kind;
    ++attach_calls;
    return 0;
}

static void check(int ok, const char *label)
{
    ++checks;
    if (!ok) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

static unsigned long read_u32(const void *address)
{
    return *(const unsigned long *)address;
}

static unsigned short read_u16(const void *address)
{
    return *(const unsigned short *)address;
}

static void reset_fixture(void)
{
    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    memset(context_bytes, 0x5a, sizeof(context_bytes));
    memset(normal_actor, 0x55, sizeof(normal_actor));
    memset(alternate_context_actor, 0xaa, sizeof(alternate_context_actor));
    memset(post_callback_actor, 0x33, sizeof(post_callback_actor));
    memset(late_callback_context_actor, 0x99, sizeof(late_callback_context_actor));
    memset(action_result_actor, 0x77, sizeof(action_result_actor));
    memset(middle_pool, 0x66, sizeof(middle_pool));
    memset(child_rows, 0, sizeof(child_rows));
    memset(eng1_descriptor_rows_5a690, 0, sizeof(eng1_descriptor_rows_5a690));
    memset(&g_actor_pipeline_storage_71440, 0, sizeof(g_actor_pipeline_storage_71440));
    g_engine_interface.context_004 = (ResourceCallbackContext *)(void *)context_bytes;
    g_normal_actor_pool = normal_actor;
    g_middle_actor_pool = middle_pool;
    g_collision_context = 0;
    g_active_category_70cfe = 0;
    eng1_sim_state_72104 = 0;
    eng1_sim_state_72174 = 0;
    eng1_sim_state_72110 = 0;
    eng1_sim_state_72170 = 0;
    eng1_sim_state_72108 = 0;
    g_eng1_setup_dword_7201c = 0;
    g_eng1_setup_dword_7211c = 0;
    eng1_sim_state_6feb4 = 0;
    g_resource_transition_ticks_6efb0 = 0x12345678UL;
    g_camera_x_70ca0 = g_camera_y_70ca4 = g_camera_z_70ca8 = 0;
    g_camera_center_x_70cdc = g_camera_center_y_70ce0 = g_camera_focal_70ce4 = 0;
    g_camera_iterations_70cfd = 0;
    g_camera_adjust_x_70d02 = g_camera_adjust_y_70d04 = 0;
    g_camera_depth_low_70d0c = g_camera_depth_high_70d0e = 0;
    g_camera_flags_70d10 = 0;
    g_camera_z_base_70d1c = 0;
    middle_calls = normal_calls = attach_calls = 0;
    untouched_slot_calls = 0;
    g_eng1_setup_handlers_72040[15] = 0;
    normal_arg = 0;
    redirect_context_on_normal_init = 0;
    allow_15d00_return = allow_2a290_return = allow_27d40_return = 0;
    change_pool_on_first_15d00 = 0;
    change_collision_on_second_15d00 = 0;
    resource_calls = post_handler_calls = action_lookup_calls = 0;
    post_handler_actor = action_lookup_actor = action_lookup_result = 0;
    action_lookup_code = 0;
    attach_actor_arg = 0;
    attach_actor_kind = 0;
    incomplete_rva = 0;
}

static void prepare_level1(void)
{
    unsigned char *ctx;
    ctx = context_bytes;
    ctx[0] = 1;
    ctx[1] = 0;
    *(unsigned long *)(void *)(ctx + 0x30) = 0x1e00UL;
    eng1_descriptor_rows_5a690[1].pointer_00c = (void *)&child_rows[1][0][0];
    eng1_descriptor_rows_5a690[3].pointer_00c = (void *)&child_rows[3][0][0];
    *(unsigned short *)(void *)&child_rows[1][0][0] = 0x0123U;
    *(unsigned short *)(void *)&child_rows[1][0][2] = 0x0345U;
    child_rows[1][0][4] = 0x12U;
    *(unsigned short *)(void *)&child_rows[3][0][0] = 0x0555U;
    *(unsigned short *)(void *)&child_rows[3][0][2] = 0x0666U;
    child_rows[3][0][4] = 0x77U;
    redirect_context_on_normal_init = 1;
}

static void test_setup_to_15d00(void)
{
    unsigned char *ctx;
    unsigned char *actor;
    unsigned int table_index;
    int zero_late_holes;

    reset_fixture();
    ctx = context_bytes;
    prepare_level1();
    *(unsigned long *)(void *)(normal_actor + 0x54) = 0x00800000UL;
    g_eng1_setup_handlers_72040[15] = untouched_slot_probe;

    if (setjmp(incomplete_boundary) == 0) {
        dispatch_local_15090();
        check(0, "0x15090 must stop at explicit 0x15d00 dependency");
        return;
    }

    actor = normal_actor;
    zero_late_holes = 1;
    for (table_index = 30; table_index <= 39; ++table_index) {
        if (g_eng1_setup_handlers_72040[table_index] != 0)
            zero_late_holes = 0;
    }
    check(incomplete_rva == 0x15d00UL, "0x15090 default frontier is 0x15d00");
    check(resource_calls == 1, "default 0x16790 calls the 0x15d00 dependency once");
    check(middle_calls == 3 &&
          middle_args[0] == middle_pool &&
          middle_args[1] == middle_pool + 0xec &&
          middle_args[2] == middle_pool + 0x1d8,
          "three middle-pool initializers preserve native pointer order");
    check(normal_calls == 1 && normal_arg == normal_actor,
          "normal actor initializer receives the pool base");
    check(attach_calls == 0, "actor attachment follows the 0x15d00 boundary");
    check(g_eng1_setup_handlers_72040[10] == dispatch_local_19440 &&
          g_eng1_setup_handlers_72040[2] == g_eng1_setup_handlers_72040[4] &&
          g_eng1_setup_handlers_72040[2] == g_eng1_setup_handlers_72040[19] &&
          g_eng1_setup_handlers_72040[2] == g_eng1_setup_handlers_72040[23],
          "native handler indices point to source symbols with shared aliases");
    check(g_eng1_setup_handlers_72040[15] == untouched_slot_probe &&
          g_eng1_setup_handlers_72040[21] == 0 &&
          g_eng1_setup_handlers_72040[26] == 0 &&
          zero_late_holes,
          "native stores preserve a warm slot and all measured loader-zero holes");
    check(untouched_slot_calls == 0,
          "preseeded untouched slot is preserved but never dispatched by this path");
    check(read_u16(ctx + 0x22) == 0 && read_u16(ctx + 0x24) == 0x80U &&
          read_u16(ctx + 0x26) == 0xffU && read_u16(ctx + 0x28) == 0xffU &&
          read_u16(ctx + 0x2a) == 0xffU && read_u16(ctx + 0x2c) == 0xffU,
          "context setup preserves all four flag-derived WORDs");
    check(read_u32(actor + 0x00) == 0x01030000UL &&
          read_u32(actor + 0x04) == 0x03850000UL &&
          read_u32(actor + 0x08) == 0x09000000UL,
          "row[level] selects the 24-byte descriptor child and fixed-point coordinates");
    check(g_camera_x_70ca0 == 0x123L && g_camera_y_70ca4 == 0x345L &&
          g_camera_z_70ca8 == 0x7c0L && g_camera_flags_70d10 == 0x5eU &&
          g_camera_iterations_70cfd == 2,
          "camera and depth state use the selected descriptor fields");
    check(g_camera_adjust_x_70d02 == 0x3c &&
          (unsigned short)g_camera_adjust_y_70d04 == 0xffc0U &&
          g_camera_depth_low_70d0c == 0x20 && g_camera_depth_high_70d0e == 0xd8,
          "camera adjustment WORDs retain their native offsets and widths");
    check(read_u16(alternate_context_actor + 0x2e) == 1 &&
          actor[0x22] == 1,
          "0x2e uses live collision context while +0x22 reloads normal actor pool");
    check(actor[0x1c] == 0x55,
          "category store follows the 0x15d00 return");
    check(read_u32(actor + 0x108) == 0x55555555UL,
          "setup counter store follows the 0x15d00 return");
    check(eng1_sim_state_72108 == 0,
          "handler index follows the 0x15d00 return");
    check(eng1_sim_state_72108 == 0 &&
          *(void **)(void *)(actor + 0x64) ==
              (void *)((unsigned char *)&g_eng1_simulation_lookup_owner_537e0 + 0x40) &&
          read_u16(actor + 0x30) == 0xffffU && read_u16(actor + 0x32) == 0xffffU,
          "0x16790 uses the measured 0x53820 interior owner and prior WORD stores");
    check((read_u32(alternate_context_actor + 0x54) & 4UL) == 0 &&
          read_u16(alternate_context_actor + 0x4a) == 0xa0U,
          "actual 0x15670 helper uses the callback-updated collision context");
    check(g_resource_transition_ticks_6efb0 == 0,
          "setup clears the native 0x6efb0 transition counter");
}

static void test_19440_counter(unsigned long initial, unsigned long expected)
{
    unsigned char *actor;
    reset_fixture();
    actor = normal_actor;
    *(unsigned long *)(void *)(actor + 0x108) = initial;
    g_collision_context = normal_actor;
    if (setjmp(incomplete_boundary) == 0) {
        dispatch_local_19440();
        check(0, "0x19440 must stop at explicit 0x15d00 dependency");
        return;
    }
    check(incomplete_rva == 0x15d00UL, "0x19440 reaches 0x15d00 before 0x2a290");
    check(read_u32(actor + 0x108) == expected,
          "0x19440 counter decrement preserves its x86 DWORD result");
    if (initial == 1)
        check(*(void **)(void *)(actor + 0x64) ==
                  (void *)((unsigned char *)&g_eng1_simulation_lookup_owner_537e0 + 0x40),
              "zero counter reenters 0x16790 before 0x15d00");
    ++checks;
}

static void test_7201c_predicate(void)
{
    unsigned char *actor;
    reset_fixture();
    actor = normal_actor;
    eng1_sim_state_72110 = 1;
    g_eng1_setup_dword_7201c = 0;
    g_eng1_setup_dword_7211c = 1;
    if (setjmp(incomplete_boundary) == 0) {
        dispatch_local_16790();
        check(0, "7201c zero branch must call typed 0x18f60 dependency");
        return;
    }
    check(incomplete_rva == 0x18f60UL && eng1_sim_state_72108 == 0,
          "0x16790 reads 0x7201c independently of contradictory 0x7211c");
    ++checks;
}

static void test_pending_16790_program_prefix(void)
{
    unsigned char *actor;
    reset_fixture();
    actor = normal_actor;
    eng1_sim_state_72110 = 1;
    g_eng1_setup_dword_7201c = 1;
    g_eng1_setup_dword_7211c = 0;
    if (setjmp(incomplete_boundary) == 0) {
        dispatch_local_16790();
        check(0, "0x7201c nonzero branch must tail-call 0x15d00 after its stores");
        return;
    }
    check(incomplete_rva == 0x15d00UL && eng1_sim_state_72108 == 0x2fU,
          "measured 0x54558 program is installed before the 0x15d00 tail call");
    check(*(void **)(void *)(actor + 0x64) == g_eng1_setup_program_54558,
          "pending 0x16790 stores the measured command-owner pointer");
    check(read_u16(actor + 0x30) == 0xffffU &&
          read_u16(actor + 0x32) == 0xffffU,
          "pending branch reloads the normal pool for both WORD stores");
    ++checks;
}

static void test_transition_16790_dependency(void)
{
    reset_fixture();
    eng1_sim_state_72170 = 1;
    eng1_sim_state_72110 = 1;
    if (setjmp(incomplete_boundary) == 0) {
        dispatch_local_16790();
        check(0, "nonzero 0x72170 must tail-dispatch through typed 0x19160");
        return;
    }
    check(incomplete_rva == 0x19160UL && eng1_sim_state_72108 == 0,
          "0x72170 branch reaches the typed 0x19160 boundary first");
    ++checks;
}

static void test_returning_dependencies_with_null_action(void)
{
    reset_fixture();
    prepare_level1();
    allow_15d00_return = 1;
    change_pool_on_first_15d00 = 1;
    change_collision_on_second_15d00 = 1;
    allow_2a290_return = 1;
    allow_27d40_return = 1;
    action_lookup_result = 0;
    if (setjmp(incomplete_boundary) == 0) {
        dispatch_local_15090();
    } else {
        check(0, "controlled missing-dependency spies should return in this case");
        return;
    }
    check(incomplete_rva == 0,
          "controlled dependency returns expose the measured continuation");
    check(resource_calls == 2 && post_handler_calls == 1 &&
          action_lookup_calls == 1 && attach_calls == 1,
          "resource, post-handler, attach, and action callbacks keep native order");
    check(g_normal_actor_pool == post_callback_actor &&
          read_u32(post_callback_actor + 0x4c) == 2UL &&
          post_callback_actor[0x96] == 0xff &&
          read_u32(post_callback_actor + 0x108) == 1UL,
          "0x16790 reloads the live normal pool after 0x15d00 returns");
    check(post_handler_actor == late_callback_context_actor &&
          attach_actor_arg == late_callback_context_actor && attach_actor_kind == 0x2024 &&
          action_lookup_actor == late_callback_context_actor &&
          action_lookup_code == 0xa007UL,
          "callback arguments reload the collision context after 0x15d00 and preserve action code");
    check(alternate_context_actor[0x1c] == 3 &&
          (read_u32(late_callback_context_actor + 0x54) & 0x00800000UL) != 0,
          "category precedes the callback swap and null result leaves the live collision bit set");
    check(action_lookup_result == 0 && eng1_sim_state_72108 == 10,
          "null 0x27d40 result leaves handler state intact");
}

static void test_nonnull_action_result_clears_collision_bit(void)
{
    reset_fixture();
    prepare_level1();
    allow_15d00_return = 1;
    allow_2a290_return = 1;
    allow_27d40_return = 1;
    action_lookup_result = action_result_actor;
    *(unsigned long *)(void *)(alternate_context_actor + 0x54) = 0x00800000UL;
    if (setjmp(incomplete_boundary) == 0) {
        dispatch_local_15090();
    } else {
        check(0, "controlled nonnull action-result spies should return");
        return;
    }
    check(action_lookup_actor == alternate_context_actor &&
          action_lookup_code == 0xa007UL,
          "nonnull result follows the same typed 0x27d40 call");
    check((read_u32(alternate_context_actor + 0x54) & 0x00800000UL) == 0,
          "collision bit 0x00800000 clears only on the nonnull result branch");
    check((read_u32(action_result_actor + 0x54) & 0x04000605UL) == 0x04000605UL &&
          read_u16(action_result_actor + 0x4a) == 0x80U &&
          read_u16(action_result_actor + 0x34) == 1U,
          "nonnull action actor receives its measured DWORD and WORD updates");
}

int main(void)
{
    test_setup_to_15d00();
    test_19440_counter(2UL, 1UL);
    test_19440_counter(1UL, 0UL);
    test_7201c_predicate();
    test_pending_16790_program_prefix();
    test_transition_16790_dependency();
    test_returning_dependencies_with_null_action();
    test_nonnull_action_result_clears_collision_bit();
    printf("setup_15090_cases=8 checks=%u failures=%u\n", checks, failures);
    return failures ? 1 : 0;
}
