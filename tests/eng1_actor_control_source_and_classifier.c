#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "../src/eng1/behavioral/actor_control_update.h"
#include "../src/eng1/behavioral/actor_control_state.h"
#include "../src/eng1/behavioral/actor_control_cells.h"
#include "../src/eng1/behavioral/actor_pipeline_storage.h"
#include "../src/eng1/behavioral/interpreters.h"
#include "../calibration/engine_interface.h"

typedef int (__cdecl *FixtureHostDispatch)(unsigned long);

extern unsigned long eng1_sim_state_72110;

static unsigned long g_root_value;
static unsigned long g_host_values[8];
static unsigned int g_host_count;
static unsigned int g_spatial_count;
static unsigned int g_update_count;
static unsigned int g_interpreter_count;
static unsigned int g_motion_count;
static unsigned long g_spatial_selector;
static Actor *g_spatial_actor;
static Actor *g_update_actor;
static char g_events[24];
static unsigned int g_event_count;

static void record_event(char value)
{
    if (g_event_count < sizeof(g_events) - 1U) {
        g_events[g_event_count++] = value;
        g_events[g_event_count] = '\0';
    }
}

static int __cdecl fixture_host_dispatch(unsigned long value)
{
    if (g_host_count < sizeof(g_host_values) / sizeof(g_host_values[0]))
        g_host_values[g_host_count] = value;
    ++g_host_count;
    record_event('H');
    return 0x12345678;
}

int __cdecl eng1_native_dep_002d3f0(Actor *actor, unsigned long selector)
{
    ++g_spatial_count;
    g_spatial_actor = actor;
    g_spatial_selector = selector;
    record_event('S');
    return 0;
}

void __cdecl eng1_native_dep_002d440(Actor *actor)
{
    g_spatial_actor = actor;
    record_event('D');
}

void __cdecl eng1_native_code_target_002a290(Actor *actor)
{
    ++g_update_count;
    g_update_actor = actor;
    record_event('A');
}

unsigned long __cdecl eng1_interpret_signed_pairs(Actor *actor)
{
    (void)actor;
    ++g_interpreter_count;
    record_event('I');
    return 0UL;
}

void __cdecl eng1_guarded_apply_frame_motion_2d(void *actor)
{
    (void)actor;
    ++g_motion_count;
    record_event('M');
}

static void fail(const char *name, const char *message)
{
    fprintf(stderr, "FAIL %s: %s\n", name, message);
}

static int require_true(int condition, const char *name, const char *message)
{
    if (condition)
        return 1;
    fail(name, message);
    return 0;
}

static void reset_case(Actor *actor, unsigned short selector)
{
    union {
        FixtureHostDispatch callback;
        unsigned long bits;
    } slot;

    memset(actor, 0, sizeof(*actor));
    memset(&g_eng1_actor_control_state_70ed8, 0,
           sizeof(g_eng1_actor_control_state_70ed8));
    memset(&g_actor_pipeline_storage_71440, 0,
           sizeof(g_actor_pipeline_storage_71440));
    memset(g_host_values, 0, sizeof(g_host_values));
    memset(g_events, 0, sizeof(g_events));
    g_host_count = 0U;
    g_spatial_count = 0U;
    g_update_count = 0U;
    g_interpreter_count = 0U;
    g_motion_count = 0U;
    g_spatial_selector = 0xffffffffUL;
    g_spatial_actor = 0;
    g_update_actor = 0;
    g_event_count = 0U;
    g_root_value = 0x04f80000UL;
    g_collision_context = (unsigned char *)(void *)&g_root_value;
    g_eng1_setup_dword_6efa4 = 0UL;
    eng1_sim_state_72110 = 0UL;

    slot.callback = fixture_host_dispatch;
    g_engine_interface.opaque_090[8] = slot.bits;

    actor->x = 0x04f80000L;
    actor->unknown_02a = 0U;
    actor->direction_x = 0x10U;
    actor->render_flags = 0x80000000UL;
    actor->interpolation_position = 0x1234L;
    actor->unknown_036 = 0x1234;
    g_eng1_actor_control_state_70ed8.words[26] = selector;
    g_eng1_actor_control_state_70ed8.words[9] = 0U;  /* 0x70eea */
}

static int check_common_actor_path(const char *name, Actor *actor,
                                   const char *events)
{
    return require_true(g_spatial_count == 0U, name,
                        "unexpected unresolved 0x2d3f0 dependency call") &&
           require_true(g_spatial_actor == actor, name,
                        "0x2d440 spy did not receive the actor pointer") &&
           require_true(g_interpreter_count == 1U, name,
                        "existing 0x283e0 call position was lost") &&
           require_true(g_motion_count == 1U, name,
                        "existing 0x29160 call position was lost") &&
           require_true(strcmp(g_events, events) == 0, name,
                        "ordered callback trace differs") &&
           require_true((actor->render_flags & 0x80000000UL) != 0UL, name,
                        "non-0x0a actor path lost native high-bit mark") &&
           require_true((actor->render_flags & 0x10UL) != 0UL, name,
                        "direction bit 0x10 was not transferred to flags") &&
           require_true(g_eng1_actor_control_state_70ed8.words[7] ==
                            (unsigned short)actor->interpolation_position,
                        name, "WORD 0x70ee6 did not receive actor+0x4c") &&
           require_true(g_eng1_actor_control_state_70ed8.words[19] == 0U &&
                            g_eng1_actor_control_state_70ed8.words[23] == 0U &&
                            g_eng1_actor_control_state_70ed8.words[3] == 0U,
                        name, "common WORD clears at 70efe/70f06/70ede differ");
}

static int test_selector_one_transition(void)
{
    Actor actor;
    const char *name = "selector1-transition";

    reset_case(&actor, 1U);
    g_eng1_actor_control_state_70ed8.words[27] = 5U; /* 70f0e */
    g_eng1_actor_control_state_70ed8.words[6] = 2U;  /* 70ee4 */
    g_eng1_actor_control_state_70ed8.words[12] = 0U; /* 70ef0 */
    eng1_native_code_target_02cd30(&actor);

    return require_true(g_host_count == 1U, name,
                        "selector 1 should dispatch exactly once") &&
           require_true(g_host_values[0] == 0x21UL, name,
                        "selector 1 did not read base 0x5c3d0 lane 1") &&
           require_true(g_eng1_actor_control_state_70ed8.words[27] == 0U &&
                            g_eng1_actor_control_state_70ed8.words[24] == 0xfaU,
                        name, "selector 1 timer writes differ") &&
           require_true(g_eng1_actor_control_state_70ed8.words[26] == 0U &&
                            g_eng1_actor_control_state_70ed8.words[13] == 0U,
                        name, "classifier WORD stores were not zero-extended") &&
           check_common_actor_path(name, &actor, "HDIM");
}

static int test_selector_eight_cycle(void)
{
    Actor actor;
    const char *name = "selector8-cycle";

    reset_case(&actor, 8U);
    g_eng1_actor_control_state_70ed8.words[21] = 3U;  /* 70f02 */
    g_eng1_actor_control_state_70ed8.words[12] = 22U; /* 70ef0 */
    g_eng1_actor_control_state_70ed8.words[5] = 1U;   /* 70ee2 */
    g_eng1_actor_control_state_70ed8.words[2] = 2U;   /* 70eda */
    eng1_native_code_target_02cd30(&actor);

    return require_true(g_host_count == 1U, name,
                        "selector 8 should dispatch one cycle event") &&
           require_true(g_host_values[0] == 0x19UL, name,
                        "selector 8 did not index lane 3 from 5c3f0") &&
           require_true(g_eng1_actor_control_state_70ed8.words[21] == 4U &&
                            g_eng1_actor_control_state_70ed8.words[5] == 0xf0U &&
                            g_eng1_actor_control_state_70ed8.words[24] == 0xa0U &&
                            g_eng1_actor_control_state_70ed8.words[12] == 0U,
                        name, "selector 8 cycle state transition differs") &&
           check_common_actor_path(name, &actor, "HDIM");
}

static int test_selector_eight_threshold_11(void)
{
    Actor actor;
    const char *name = "selector8-threshold-11";

    reset_case(&actor, 8U);
    g_eng1_actor_control_state_70ed8.words[21] = 0U;  /* 70f02 */
    g_eng1_actor_control_state_70ed8.words[12] = 11U; /* 70ef0 */
    g_eng1_actor_control_state_70ed8.words[5] = 0U;   /* 70ee2 */
    g_eng1_actor_control_state_70ed8.words[2] = 2U;   /* 70eda: stays nonzero */
    eng1_native_code_target_02cd30(&actor);

    return require_true(g_host_count == 1U, name,
                        "native selector-8 threshold is greater than 0x0a") &&
           require_true(g_host_values[0] == 0x16UL, name,
                        "selector 8 timer 11 did not dispatch 5c3f0 lane 0") &&
           require_true(g_eng1_actor_control_state_70ed8.words[21] == 1U &&
                            g_eng1_actor_control_state_70ed8.words[5] == 0xf0U &&
                            g_eng1_actor_control_state_70ed8.words[24] == 0xa0U &&
                            g_eng1_actor_control_state_70ed8.words[12] == 0U,
                        name, "selector 8 timer-11 state transition differs") &&
           check_common_actor_path(name, &actor, "HDIM");
}

static int test_selector_twelve_transition(void)
{
    Actor actor;
    const char *name = "selector12-transition";

    reset_case(&actor, 12U);
    g_eng1_actor_control_state_70ed8.words[27] = 8U;
    g_eng1_actor_control_state_70ed8.words[6] = 2U;
    eng1_native_code_target_02cd30(&actor);

    return require_true(g_host_count == 1U, name,
                        "selector 12 should dispatch exactly once") &&
           require_true(g_host_values[0] == 0x1bUL, name,
                        "selector 12 did not read base 0x5c3d0 lane 12") &&
           require_true(g_eng1_actor_control_state_70ed8.words[27] == 0U &&
                            g_eng1_actor_control_state_70ed8.words[24] == 0xfaU,
                        name, "selector 12 timer writes differ") &&
           check_common_actor_path(name, &actor, "HDIM");
}

static int test_pipeline_table_transition(void)
{
    Actor actor;
    const char *name = "pipeline-table-transition";

    reset_case(&actor, 8U);
    g_actor_pipeline_storage_71440.word_cells[6] = 251U; /* 0x7144c */
    g_eng1_actor_control_state_70ed8.words[12] = 10U;   /* 0x70ef0 */
    g_eng1_actor_control_state_70ed8.words[2] = 2U;     /* 0x70eda */
    g_eng1_actor_control_state_70ed8.words[4] = 1U;     /* 0x70ee0 */
    g_eng1_actor_control_state_70ed8.words[8] = 2U;     /* 0x70ee8 */
    eng1_native_code_target_02cd30(&actor);

    return require_true(g_host_count == 1U, name,
                        "pipeline threshold should dispatch once") &&
           require_true(g_host_values[0] == 0x18UL, name,
                        "gate-zero transition did not use 5c3f8 lane 2") &&
           require_true(g_actor_pipeline_storage_71440.word_cells[6] == 0U &&
                            g_eng1_actor_control_state_70ed8.words[8] == 3U &&
                            g_eng1_actor_control_state_70ed8.words[4] == 0x15eU &&
                            g_eng1_actor_control_state_70ed8.words[24] == 0xa0U,
                        name, "pipeline transition writes differ") &&
           check_common_actor_path(name, &actor, "HDIM");
}

static int test_pipeline_table_gate_set(void)
{
    Actor actor;
    const char *name = "pipeline-table-gate-set";

    reset_case(&actor, 8U);
    g_actor_pipeline_storage_71440.word_cells[6] = 251U;
    g_eng1_actor_control_state_70ed8.words[12] = 10U;
    g_eng1_actor_control_state_70ed8.words[2] = 2U;
    g_eng1_actor_control_state_70ed8.words[4] = 1U;
    g_eng1_actor_control_state_70ed8.words[8] = 2U;
    g_eng1_setup_dword_6efa4 = 1UL;
    eng1_native_code_target_02cd30(&actor);

    return require_true(g_host_count == 1U, name,
                        "set 0x6efa4 pipeline gate should dispatch once") &&
           require_true(g_host_values[0] == 0x16UL, name,
                        "set gate should select 0x5c400 lane 2, not 0x5c3f8") &&
           require_true(g_eng1_actor_control_state_70ed8.words[8] == 3U &&
                            g_eng1_actor_control_state_70ed8.words[4] == 0x15eU,
                        name, "set-gate transition writes differ") &&
           check_common_actor_path(name, &actor, "HDIM");
}

static int test_selector_twelve_pipeline_table(void)
{
    Actor actor;
    const char *name = "selector12-pipeline-table";

    reset_case(&actor, 12U);
    g_actor_pipeline_storage_71440.word_cells[6] = 251U;
    g_eng1_actor_control_state_70ed8.words[12] = 10U; /* 0x70ef0 */
    g_eng1_actor_control_state_70ed8.words[2] = 2U;   /* 0x70eda */
    g_eng1_actor_control_state_70ed8.words[4] = 1U;   /* 0x70ee0 */
    g_eng1_actor_control_state_70ed8.words[8] = 2U;   /* 0x70ee8 */
    eng1_native_code_target_02cd30(&actor);

    return require_true(g_host_count == 1U, name,
                        "selector 12 pipeline threshold should dispatch once") &&
           require_true(g_host_values[0] == 0x16UL, name,
                        "selector 12 must use 5c400 lane 2 independent of 6efa4") &&
           require_true(g_actor_pipeline_storage_71440.word_cells[6] == 0U &&
                            g_eng1_actor_control_state_70ed8.words[8] == 3U &&
                            g_eng1_actor_control_state_70ed8.words[4] == 0x15eU &&
                            g_eng1_actor_control_state_70ed8.words[24] == 0xa0U,
                        name, "selector 12 pipeline transition writes differ") &&
           check_common_actor_path(name, &actor, "HDIM");
}

static int test_selector_class_noop(void)
{
    Actor actor;
    const char *name = "selector9-noop";

    reset_case(&actor, 9U);
    eng1_native_code_target_02cd30(&actor);
    return require_true(g_host_count == 0U, name,
                        "selector class 3 unexpectedly dispatched") &&
           check_common_actor_path(name, &actor, "DIM");
}

static int test_invalid_selector_and_signed_sentinel(void)
{
    Actor actor;
    const char *name = "invalid-selector-signed-sentinel";

    reset_case(&actor, 0xfffeU);
    g_root_value = 0xffff0000UL;
    *(unsigned long *)(void *)&actor.x = 0x80000000UL;
    eng1_native_code_target_02cd30(&actor);

    return require_true(g_host_count == 0U, name,
                        "invalid selector must skip all handler-table reads") &&
           require_true(g_eng1_actor_control_state_70ed8.words[26] == 0x00ffU &&
                            g_eng1_actor_control_state_70ed8.words[13] == 0x00ffU,
                        name, "signed-high-WORD sentinel AL was not zero-extended") &&
           require_true(actor.x == 0x04f80000L, name,
                        "signed JGE clamp did not replace negative x") &&
           check_common_actor_path(name, &actor, "DIM");
}

static int test_classifier_transition_call_order(void)
{
    Actor actor;
    const char *name = "classifier-transition-order";

    reset_case(&actor, 0xabcdU);
    g_root_value = 0x04f80000UL;       /* classifier row 0 */
    actor.x = 0x05dc0000L;            /* classifier row 1 */
    g_eng1_actor_control_state_70ed8.words[9] = 2U; /* previous selector */
    eng1_native_code_target_02cd30(&actor);

    return require_true(g_spatial_count == 1U, name,
                        "different classifier rows should invoke 0x2d3f0") &&
           require_true(g_spatial_actor == &actor && g_spatial_selector == 0U,
                        name, "0x2d3f0 arguments differ") &&
           require_true(strcmp(g_events, "SDIM") == 0, name,
                        "0x2d3f0/0x2d440/interpreter/motion call order differs") &&
           require_true(g_eng1_actor_control_state_70ed8.words[26] == 0U &&
                            g_eng1_actor_control_state_70ed8.words[13] == 1U,
                        name, "classifier result bytes were not widened to WORD") &&
           require_true((actor.x >= 0x04f80000L), name,
                        "native x lower-bound store was lost");
}

static int test_native_classifier(unsigned int argc, char **argv)
{
    HMODULE image;
    unsigned long (__cdecl *native_classifier)(unsigned long);
    unsigned long high_word;
    unsigned long checked;

    if (argc != 2U) {
        fail("native-41fd0-classifier",
             "fixture runner must pass the read-only ENG1.DLL path");
        return 0;
    }
    image = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (image == 0) {
        fprintf(stderr, "FAIL native-41fd0-classifier: LoadLibraryExA error %lu\n",
                (unsigned long)GetLastError());
        return 0;
    }

    /* Execute the read-only native leaf only; the 0x2cd30 oracle entry is
     * never called. Compare its AL result against the production-source
     * classifier helper for every possible signed high WORD. */
    native_classifier = (unsigned long (__cdecl *)(unsigned long))
        ((unsigned char *)image + 0x41fd0UL);
    checked = 0UL;
    for (high_word = 0UL; high_word <= 0xffffUL; ++high_word) {
        unsigned long input;
        unsigned char actual;
        unsigned char expected;

        input = high_word << 16;
        actual = (unsigned char)native_classifier(input);
        expected = eng1_native_code_target_0041fd0(input);
        if (actual != expected) {
            fprintf(stderr,
                    "FAIL native-41fd0-classifier: high=0x%04lx native=0x%02x source=0x%02x\n",
                    high_word, (unsigned int)actual, (unsigned int)expected);
            FreeLibrary(image);
            return 0;
        }
        ++checked;
    }
    FreeLibrary(image);
    printf("PASS direct native 0x41fd0 differential: %lu signed high-WORD inputs\n",
           checked);
    return 1;
}

static int test_classifier_table_guards(void)
{
    unsigned short saved_first;
    unsigned short saved_sentinel;
    unsigned char result;
    int supported;

    saved_first = g_eng1_classifier_rows_65678[0][0];
    g_eng1_classifier_rows_65678[0][0] = 0xffffU;
    result = 0x5aU;
    supported = eng1_classify_actor_position_checked(0x7fff0000UL, &result);
    g_eng1_classifier_rows_65678[0][0] = saved_first;
    if (!require_true(supported && result == 0xffU,
                      "classifier-initial-dword-sentinel",
                      "initial low-WORD sentinel check must precede row comparisons"))
        return 0;

    saved_sentinel = g_eng1_classifier_rows_65678[17][0];
    g_eng1_classifier_rows_65678[17][0] = 0U;
    result = 0x5aU;
    supported = eng1_classify_actor_position_checked(0UL, &result);
    g_eng1_classifier_rows_65678[17][0] = saved_sentinel;

    return require_true(!supported, "classifier-extent-guard",
                        "scan without a sentinel must report unsupported extent") &&
           require_true(result == 0x5aU, "classifier-extent-guard",
                        "unsupported scan must not fabricate an AL result");
}

int main(int argc, char **argv)
{
    if (!test_selector_one_transition())
        return 1;
    printf("PASS selector1-transition\n");
    if (!test_selector_eight_cycle())
        return 1;
    printf("PASS selector8-cycle\n");
    if (!test_selector_eight_threshold_11())
        return 1;
    printf("PASS selector8-threshold-11\n");
    if (!test_selector_twelve_transition())
        return 1;
    printf("PASS selector12-transition\n");
    if (!test_pipeline_table_transition())
        return 1;
    printf("PASS pipeline-table-transition\n");
    if (!test_pipeline_table_gate_set())
        return 1;
    printf("PASS pipeline-table-gate-set\n");
    if (!test_selector_twelve_pipeline_table())
        return 1;
    printf("PASS selector12-pipeline-table\n");
    if (!test_selector_class_noop())
        return 1;
    printf("PASS selector9-noop\n");
    if (!test_invalid_selector_and_signed_sentinel())
        return 1;
    printf("PASS invalid-selector-signed-sentinel\n");
    if (!test_classifier_transition_call_order())
        return 1;
    printf("PASS classifier-transition-order\n");
    if (!test_native_classifier((unsigned int)argc, argv))
        return 1;
    if (!test_classifier_table_guards())
        return 1;
    printf("PASS initial DWORD sentinel and bounded 18-row helper guards\n");
    printf("actor_control_classifier_native_cases=65536 checks=65536 failures=0\n");
    printf("actor_control_source_spy_cases=10 failures=0\n");
    printf("classifier_source_guard_cases=2 failures=0\n");
    return 0;
}
