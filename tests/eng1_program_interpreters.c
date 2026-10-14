#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/eng1/behavioral/interpreters.h"

ResourceKindInfo g_kind_info[1];
unsigned long g_configuration_dword_71384;

static int remove_calls;
static int ratio_calls;
static int ratio_a;
static int ratio_b;
static int ratio_divisor;

int __cdecl remove_actor(Actor *actor)
{
    (void)actor;
    ++remove_calls;
    return 1;
}

void __cdecl remove_actor_preserving_resource(Actor *actor)
{
    (void)actor;
    ++remove_calls;
}

void __cdecl mark_actor_resource_for_release(Actor *actor)
{
    (void)actor;
}

void __cdecl spawn_actor_with_owner(Actor *actor, long x, long y, long z,
                                   unsigned short kind)
{
    (void)actor; (void)x; (void)y; (void)z; (void)kind;
}

void __cdecl spawn_actor_at_group(long x, long y, long z,
                                 unsigned short kind, unsigned short group)
{
    (void)x; (void)y; (void)z; (void)kind; (void)group;
}

void __cdecl stop_actor_sounds(Actor *actor)
{
    (void)actor;
}

void __cdecl play_sound(unsigned short effect_id, Actor *actor)
{
    (void)effect_id; (void)actor;
}

int __cdecl wrapped_ratio(int a, int b, int divisor)
{
    ++ratio_calls;
    ratio_a = a;
    ratio_b = b;
    ratio_divisor = divisor;
    return 8;
}

unsigned int __cdecl dispatch_engine_2c460(int limit)
{
    (void)limit;
    return 2;
}

static void require(int condition, const char *label)
{
    if (!condition) {
        printf("FAIL: %s\n", label);
        exit(1);
    }
}

static void test_pair_restart_uses_shared_advance_frame(void)
{
    Eng1CommandPair original[1] = { { -4, 0 } };
    Eng1CommandPair next[1] = { { 0, 3 } };
    void *frames[1];
    Actor actor;

    memset(&actor, 0, sizeof(actor));
    frames[0] = next;
    actor.frame_ticks = 0;
    actor.frame_state = -1;
    actor.frame_index = -1;
    actor.frames = frames;
    actor.image_or_program = original;

    require(eng1_interpret_signed_pairs(&actor) == 1,
            "pair -4 returns its restart flag");
    require(actor.frame_index == 0 && actor.image_or_program == next,
            "pair -4 delegates frame selection to advance_frame");
    require(actor.frame_state == 0 && actor.active_command == 0 &&
            actor.frame_ticks == 3,
            "pair restart resumes at the selected program's first record");
}

static void test_pair_select_uses_shared_select_frame(void)
{
    Eng1CommandPair original[1] = { { -12, 1 } };
    Eng1CommandPair next[1] = { { 0, 4 } };
    void *frames[2];
    Actor actor;

    memset(&actor, 0, sizeof(actor));
    frames[0] = original;
    frames[1] = next;
    actor.frame_ticks = 0;
    actor.frame_state = -1;
    actor.frame_index = -1;
    actor.frames = frames;
    actor.image_or_program = original;

    require(eng1_interpret_signed_pairs(&actor) == 1,
            "pair -12 returns its selection flag");
    require(actor.frame_index == 1 && actor.image_or_program == next,
            "pair -12 delegates image/program selection to select_frame");
    require(actor.frame_state == 0 && actor.frame_ticks == 4,
            "pair selection resets the shared state cursor before continuing");
}

static void test_word_stream_remove_and_duration(void)
{
    short program[2] = { 7, -6 };
    Actor actor;

    memset(&actor, 0, sizeof(actor));
    remove_calls = 0;
    actor.frame_ticks = 0;
    actor.frame_state = 0;
    actor.image_or_program = program;

    require(eng1_interpret_word_stream(&actor) == 0,
            "word-stream removal does not claim a restart flag");
    require(remove_calls == 1 && actor.active_command == 0,
            "word-stream -6 calls remove_actor and clears the active command");
    require(actor.frame_ticks == 7,
            "word-stream duration comes from program word zero");
}

static void test_triple_program_switch_and_interpolation(void)
{
    Eng1CommandTriple original[1] = { { -42, 1, 0 } };
    Eng1CommandTriple selected[1] = { { 0, 10, 20 } };
    unsigned short control_words[10];
    unsigned short *control_handle;
    void *frames[2];
    Actor actor;

    memset(&actor, 0, sizeof(actor));
    memset(control_words, 0, sizeof(control_words));
    frames[0] = original;
    frames[1] = selected;
    control_handle = control_words;
    actor.frame_ticks = 0;
    actor.frame_state = -1;
    actor.frames = frames;
    actor.image_or_program = original;
    actor.animation_control = &control_handle;

    require(eng1_interpret_animation_triples(&actor) == 1,
            "triple -42 returns its program-switch flag");
    require(actor.image_or_program == selected && actor.frame_state == 0,
            "triple -42 selects actor frame-list entry one and restarts at zero");
    require(actor.active_command == 0 && actor.frame_ticks == 20 &&
            actor.interpolation_position == 10,
            "triple positive record installs its duration and position target");
}

static void test_triple_angle_command_uses_shared_ratio(void)
{
    Eng1CommandTriple program[2] = {
        { -33, 20, 10 },
        { 0, 5, 4 }
    };
    unsigned short control_words[10];
    unsigned short *control_handle;
    Eng1AnimationControl *control;
    Actor actor;

    memset(&actor, 0, sizeof(actor));
    memset(control_words, 0, sizeof(control_words));
    control = (Eng1AnimationControl *)control_words;
    control->x_target = 15;
    control_handle = control_words;
    actor.frame_ticks = 0;
    actor.frame_state = -1;
    actor.image_or_program = program;
    actor.animation_control = &control_handle;
    actor.angle_yaw = 100;
    ratio_calls = 0;

    require(eng1_interpret_animation_triples(&actor) == 0,
            "triple angle frame has no restart flag");
    require(ratio_calls == 1 && ratio_a == 100 && ratio_b == 20 &&
            ratio_divisor == 10,
            "triple -33 delegates wrapped delta calculation");
    require(control->yaw_target == 20 && control->yaw_step == 8 &&
            actor.angle_yaw == 92,
            "triple yaw transition applies the measured signed step");
    require(actor.interpolation_position == 7,
            "triple positive record advances the x control by its step");
}

static void test_triple_requires_control_handle(void)
{
    Actor actor;

    memset(&actor, 0, sizeof(actor));
    actor.frame_ticks = 9;
    require(eng1_interpret_animation_triples(&actor) == 0 &&
            actor.frame_ticks == 9,
            "triple interpreter returns before touching state without +0x128");
}

static void test_triple_zero_steps_preserve_raw_angles(void)
{
    unsigned short control_words[10];
    unsigned short expected_control[10];
    unsigned short *control_handle;
    Eng1AnimationControl *control;
    Actor actor;
    Actor expected;

    /* Native differential inputs are recorded in eng1_zero_angle_step.json.
     * This source fixture preserves that result without executing the oracle. */
    memset(&actor, 0, sizeof(actor));
    memset(control_words, 0, sizeof(control_words));
    control = (Eng1AnimationControl *)control_words;
    control->x_current = -1;
    control->yaw_target = 20;
    control->pitch_target = 40;
    control->roll_target = 60;
    control_handle = control_words;
    actor.animation_control = &control_handle;
    actor.frame_ticks = 1;
    actor.angle_yaw = (short)0xa5a5;
    actor.angle_pitch = (short)0xa55a;
    actor.angle_roll = (short)0x5aa5;
    expected = actor;
    expected.frame_ticks = 0;
    memcpy(expected_control, control_words, sizeof(control_words));

    require(eng1_interpret_animation_triples(&actor) == 0,
            "zero-step angle update returns the measured normal result");
    require(memcmp(&actor, &expected, sizeof(actor)) == 0,
            "zero steps preserve raw angle WORDs and decrement only the tick");
    require(memcmp(control_words, expected_control, sizeof(control_words)) == 0,
            "zero steps preserve the complete animation control record");
}

static void test_triple_clamp_masks_angles_and_preserves_equal_distance(void)
{
    static const short currents[3] = { 0x454, 0x454, 100 };
    static const short targets[3] = { 0x455, 0x456, 101 };
    static const short expected_angles[3][3] = {
        { 0x55, 0x55, 0x55 },
        { 0x52, 0x56, 0x56 },
        { 101, 101, 101 }
    };
    static const short expected_steps[3] = { 0, 2, 0 };
    unsigned short control_words[10];
    unsigned short expected_words[10];
    unsigned short *control_handle;
    Eng1AnimationControl *control;
    Eng1AnimationControl *expected_control;
    Actor actor;
    Actor expected;
    int i;

    /* Native-measured clamp, exact-distance and in-range cases; only the
     * actor angle is masked, while the target WORD remains unchanged. */
    for (i = 0; i < 3; ++i) {
        memset(&actor, 0, sizeof(actor));
        memset(control_words, 0, sizeof(control_words));
        control = (Eng1AnimationControl *)control_words;
        control->x_current = -1;
        control->yaw_target = targets[i];
        control->pitch_target = targets[i];
        control->roll_target = targets[i];
        control->yaw_step = 2;
        control->pitch_step = 2;
        control->roll_step = 2;
        control_handle = control_words;
        actor.animation_control = &control_handle;
        actor.frame_ticks = 1;
        actor.angle_yaw = currents[i];
        actor.angle_pitch = currents[i];
        actor.angle_roll = currents[i];
        expected = actor;
        expected.frame_ticks = 0;
        expected.angle_yaw = expected_angles[i][0];
        expected.angle_pitch = expected_angles[i][1];
        expected.angle_roll = expected_angles[i][2];
        memcpy(expected_words, control_words, sizeof(control_words));
        expected_control = (Eng1AnimationControl *)expected_words;
        expected_control->yaw_step = expected_steps[i];
        expected_control->pitch_step = expected_steps[i];
        expected_control->roll_step = expected_steps[i];

        require(eng1_interpret_animation_triples(&actor) == 0,
                "angle clamp matrix preserves the measured return");
        require(memcmp(&actor, &expected, sizeof(actor)) == 0,
                "angle clamp matrix matches complete native-measured actor state");
        require(memcmp(control_words, expected_words, sizeof(control_words)) == 0,
                "angle clamp matrix clears steps only for strict near-target cases");
    }
}

int main(void)
{
    test_pair_restart_uses_shared_advance_frame();
    test_pair_select_uses_shared_select_frame();
    test_word_stream_remove_and_duration();
    test_triple_program_switch_and_interpolation();
    test_triple_angle_command_uses_shared_ratio();
    test_triple_requires_control_handle();
    test_triple_zero_steps_preserve_raw_angles();
    test_triple_clamp_masks_angles_and_preserves_equal_distance();
    puts("ENG1 program-interpreter fixtures passed");
    return 0;
}
