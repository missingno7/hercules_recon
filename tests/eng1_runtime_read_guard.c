#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/behavioral/interpreters.h"
#include "../src/eng1/behavioral/flags_word_programs.h"
#include "../src/eng1/behavioral/provisional_frame_pairs.h"
#include "../src/eng1/behavioral/finite_frame_prefixes.h"
#include "../src/eng1/behavioral/shared_selector_assets.h"
#include "../src/eng1/behavioral/read_guard.h"
extern unsigned short g_eng1_config_word_prefix_53370[44];
extern unsigned short g_eng1_config_word_pool_533c8[282];


typedef struct FixtureProgramPrefix {
    const unsigned char *pointer;
    unsigned long original_rva;
    unsigned int bytes;
    unsigned int pair_stride;
    unsigned int word_stride;
} FixtureProgramPrefix;

static const FixtureProgramPrefix g_fixture_program_prefixes[] = {
    { (const unsigned char *)g_eng1_config_word_prefix_53370, 0x53370UL, 88U, 4U, 2U },
    { (const unsigned char *)g_eng1_config_word_pool_533c8, 0x533c8UL, 564U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063248, 0x63248UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_06324c, 0x6324cUL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063250, 0x63250UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063254, 0x63254UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063258, 0x63258UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_06325c, 0x6325cUL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063260, 0x63260UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063264, 0x63264UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063a60, 0x63a60UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063a64, 0x63a64UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063ce0, 0x63ce0UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063ce4, 0x63ce4UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063ce8, 0x63ce8UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_provisional_pair_063cec, 0x63cecUL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_finite_pair_0591f0, 0x591f0UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_finite_pair_059228, 0x59228UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_finite_pair_059288, 0x59288UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_finite_pair_0592a8, 0x592a8UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_finite_pair_0592f8, 0x592f8UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_finite_pair_059300, 0x59300UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_finite_pair_059350, 0x59350UL, 4U, 4U, 2U },
    { (const unsigned char *)g_eng1_shared_selector_04dcb8, 0x4dcb8UL, 44U, 0U, 2U },
    { (const unsigned char *)g_eng1_shared_selector_060238, 0x60238UL, 44U, 0U, 2U },
    { (const unsigned char *)g_eng1_shared_selector_066cc0, 0x66cc0UL, 44U, 0U, 2U },
    { (const unsigned char *)g_eng1_flag_program_055538, 0x55538UL, 8U, 0U, 2U },
};

typedef struct FixtureFramePrefix {
    const void *pointer;
    unsigned long original_rva;
    unsigned int slots;
} FixtureFramePrefix;

static const FixtureFramePrefix g_fixture_frame_prefixes[] = {
    { (const void *)g_eng1_provisional_frames_06323c, 0x6323cUL, 1U },
    { (const void *)g_eng1_provisional_frames_063268, 0x63268UL, 10U },
    { (const void *)g_eng1_provisional_frames_063290, 0x63290UL, 10U },
    { (const void *)g_eng1_provisional_frames_063a68, 0x63a68UL, 4U },
    { (const void *)g_eng1_provisional_frames_063cf0, 0x63cf0UL, 6U },
    { (const void *)g_eng1_finite_frames_04f8e8, 0x4f8e8UL, 4U },
    { (const void *)g_eng1_finite_frames_04f8f8, 0x4f8f8UL, 4U },
    { (const void *)g_eng1_finite_frames_051590, 0x51590UL, 5U },
    { (const void *)g_eng1_finite_frames_059398, 0x59398UL, 10U },
    { (const void *)g_eng1_finite_frames_05c2d0, 0x5c2d0UL, 52U },
    { (const void *)g_eng1_finite_frames_05fed0, 0x5fed0UL, 36U },
    { (const void *)g_eng1_finite_frames_066e48, 0x66e48UL, 15U },
};
ResourceKindInfo g_kind_info[1];

static jmp_buf g_incomplete_escape;
static unsigned long g_last_rva;
static char g_last_reason[256];

void __cdecl eng1_incomplete_behavior(unsigned long original_rva,
                                      const char *reason)
{
    g_last_rva = original_rva;
    strncpy(g_last_reason, reason, sizeof(g_last_reason) - 1);
    g_last_reason[sizeof(g_last_reason) - 1] = '\0';
    longjmp(g_incomplete_escape, 1);
}

int __cdecl remove_actor(Actor *actor) { (void)actor; return 0; }
int __cdecl wrapped_ratio(int a, int b, int c) { (void)a; (void)b; (void)c; return 8; }
void __cdecl play_sound(unsigned short id, Actor *actor) { (void)id; (void)actor; }
unsigned int __cdecl dispatch_engine_2c460(int value) { (void)value; return 0; }
unsigned long g_configuration_dword_71384;
void __cdecl remove_actor_preserving_resource(Actor *actor) { (void)actor; }
void __cdecl mark_actor_resource_for_release(Actor *actor) { (void)actor; }
void __cdecl spawn_actor_with_owner(Actor *a, long x, long y, long z, unsigned short o)
{ (void)a; (void)x; (void)y; (void)z; (void)o; }
void __cdecl spawn_actor_at_group(long x, long y, long z, unsigned short o, unsigned short g)
{ (void)x; (void)y; (void)z; (void)o; (void)g; }
void __cdecl stop_actor_sounds(Actor *actor) { (void)actor; }

static int expect_failure(unsigned long expected_rva, const char *reason_part)
{
    if (g_last_rva != expected_rva) {
        fprintf(stderr, "wrong incomplete RVA: %08lx expected %08lx\n",
                g_last_rva, expected_rva);
        return 0;
    }
    if (reason_part != 0 && strstr(g_last_reason, reason_part) == 0) {
        fprintf(stderr, "missing diagnostic text: %s in %s\n",
                reason_part, g_last_reason);
        return 0;
    }
    return 1;
}


static int expect_program_guard_fault(const FixtureProgramPrefix *prefix,
                                      long index, unsigned int stride,
                                      unsigned int width)
{
    char expected_index[40];
    sprintf(expected_index, "index %ld", index);
    if (setjmp(g_incomplete_escape) == 0) {
        eng1_guard_program_read(prefix->pointer, index, stride, width,
                                "fixture out-of-prefix program read");
        return 0;
    }
    return g_last_rva == prefix->original_rva &&
           strstr(g_last_reason, expected_index) != 0;
}

static int expect_frame_guard_fault(const FixtureFramePrefix *prefix,
                                    long index)
{
    char expected_index[40];
    sprintf(expected_index, "index %ld", index);
    if (setjmp(g_incomplete_escape) == 0) {
        eng1_guard_frame_slot(prefix->pointer, index,
                              "fixture out-of-prefix frame read");
        return 0;
    }
    return g_last_rva == prefix->original_rva &&
           strstr(g_last_reason, expected_index) != 0;
}

static int probe_all_registered_program_prefixes(void)
{
    unsigned int i;
    unsigned int mode;
    unsigned int stride;
    unsigned int capacity;
    unsigned int last_index;
    unsigned char sampled[6];

    for (i = 0; i < sizeof(g_fixture_program_prefixes) /
                        sizeof(g_fixture_program_prefixes[0]); ++i) {
        for (mode = 0; mode < 2; ++mode) {
            stride = (mode == 0) ? g_fixture_program_prefixes[i].pair_stride :
                                   g_fixture_program_prefixes[i].word_stride;
            if (stride == 0) continue;
            capacity = g_fixture_program_prefixes[i].bytes / stride;
            if (capacity == 0) return 0;
            last_index = capacity - 1;
            eng1_guard_program_read(g_fixture_program_prefixes[i].pointer,
                                    (long)last_index, stride, stride,
                                    "fixture last valid program read");
            memcpy(sampled,
                   g_fixture_program_prefixes[i].pointer +
                       last_index * stride,
                   stride);
            if (!expect_program_guard_fault(&g_fixture_program_prefixes[i],
                                            (long)capacity, stride, stride) ||
                !expect_program_guard_fault(&g_fixture_program_prefixes[i],
                                            -1L, stride, stride))
                return 0;
        }
    }
    return 1;
}

static int probe_all_registered_frame_prefixes(void)
{
    unsigned int i;
    unsigned int last_index;
    void *sampled_slot;

    for (i = 0; i < sizeof(g_fixture_frame_prefixes) /
                        sizeof(g_fixture_frame_prefixes[0]); ++i) {
        last_index = g_fixture_frame_prefixes[i].slots - 1;
        eng1_guard_frame_slot(g_fixture_frame_prefixes[i].pointer,
                              (long)last_index,
                              "fixture last valid frame read");
        memcpy(&sampled_slot,
               (const unsigned char *)g_fixture_frame_prefixes[i].pointer +
                   last_index * sizeof(void *),
               sizeof(void *));
        if (!expect_frame_guard_fault(&g_fixture_frame_prefixes[i],
                                      (long)g_fixture_frame_prefixes[i].slots) ||
            !expect_frame_guard_fault(&g_fixture_frame_prefixes[i], -1L))
            return 0;
    }
    return 1;
}

int main(void)
{
    Actor actor;
    short unknown_program[1];
    short unknown_opcode[1] = { 0 };
    unsigned short control_storage[10];
    unsigned short *control_pointer;
    Eng1CommandTriple unknown_triples[1];
    unsigned long result;

    if (!probe_all_registered_program_prefixes()) return 17;
    if (!probe_all_registered_frame_prefixes()) return 18;

    /* In-prefix WORD read at the last word and the duration fallback are
     * allowed; the accepted four-WORD owner is the actual linked provider. */
    memset(&actor, 0, sizeof(actor));
    actor.image_or_program = g_eng1_flag_program_055538;
    actor.frame_ticks = 0;
    actor.frame_state = -1;
    result = eng1_interpret_word_stream(&actor);
    if (result != 0 || actor.active_command != 0 || actor.frame_ticks != 0)
        return 1;
    eng1_guard_program_read(g_eng1_flag_program_055538, 3, 2U, 2U,
                            "fixture last valid WORD");

    /* Out-of-prefix opcode continuation faults before active-command success
     * can be fabricated from adjacent storage. */
    memset(&actor, 0, sizeof(actor));
    actor.image_or_program = g_eng1_flag_program_055538;
    actor.frame_ticks = 0;
    actor.frame_state = 3;
    actor.active_command = 0x1234;
    if (setjmp(g_incomplete_escape) == 0) {
        (void)eng1_interpret_word_stream(&actor);
        return 2;
    }
    if (!expect_failure(0x55538UL, "index 4") ||
        actor.active_command != 0x1234)
        return 3;

    /* Signed WORD wrap from 0x7fff to 0x8000 remains signed and faults before
     * the first dereference; the diagnostic records the wrapped index. */
    memset(&actor, 0, sizeof(actor));
    actor.image_or_program = g_eng1_flag_program_055538;
    actor.frame_ticks = 0;
    actor.frame_state = 32767;
    actor.active_command = 0x4321;
    if (setjmp(g_incomplete_escape) == 0) {
        (void)eng1_interpret_word_stream(&actor);
        return 4;
    }
    if (!expect_failure(0x55538UL, "index -32768") ||
        actor.active_command != 0x4321)
        return 5;

    /* Pair index wrap uses the registered source pointer and pair stride. */
    memset(&actor, 0, sizeof(actor));
    actor.image_or_program = g_eng1_provisional_pair_063248;
    actor.frame_ticks = 0;
    actor.frame_state = 32767;
    actor.active_command = 0x6789;
    if (setjmp(g_incomplete_escape) == 0) {
        (void)eng1_interpret_signed_pairs(&actor);
        return 6;
    }
    if (!expect_failure(0x63248UL, "index -32768") ||
        actor.active_command != 0x6789)
        return 7;

    /* A known four-byte pair prefix cannot satisfy one six-byte triple. */
    memset(&actor, 0, sizeof(actor));
    memset(control_storage, 0, sizeof(control_storage));
    control_pointer = control_storage;
    actor.animation_control = &control_pointer;
    actor.image_or_program = g_eng1_provisional_pair_063248;
    actor.frame_ticks = 0;
    actor.frame_state = -1;
    if (setjmp(g_incomplete_escape) == 0) {
        (void)eng1_interpret_animation_triples(&actor);
        return 8;
    }
    if (!expect_failure(0x63248UL, "animation-triple opcode/operands"))
        return 9;

    /* Registered frame table: slot zero is valid. Null slot zero requires a
     * second, independently checked jump-index slot and fails at the boundary. */
    memset(&actor, 0, sizeof(actor));
    actor.frames = g_eng1_provisional_frames_06323c;
    eng1_guarded_select_frame(&actor, 0);
    if (actor.frame_index != 0 || actor.image_or_program != (void *)g_eng1_provisional_pair_063228)
        return 10;
    if (setjmp(g_incomplete_escape) == 0) {
        eng1_guarded_select_frame(&actor, 1);
        return 11;
    }
    if (!expect_failure(0x6323cUL, "index 1"))
        return 12;

    g_eng1_provisional_frames_06323c[0] = 0;
    actor.frame_index = -1;
    if (setjmp(g_incomplete_escape) == 0) {
        eng1_guarded_advance_frame(&actor);
        return 13;
    }
    if (!expect_failure(0x6323cUL, "index 1") || actor.frame_index != -1)
        return 14;
    g_eng1_provisional_frames_06323c[0] = (void *)g_eng1_provisional_pair_063228;
    actor.frame_index = 32767;
    if (setjmp(g_incomplete_escape) == 0) {
        eng1_guarded_advance_frame(&actor);
        return 19;
    }
    if (!expect_failure(0x6323cUL, "index -32768") ||
        actor.frame_index != 32767)
        return 20;

    /* Unknown runtime program pointers deliberately keep the prior behavior. */
    unknown_program[0] = 0;
    memset(&actor, 0, sizeof(actor));
    actor.image_or_program = unknown_program;
    actor.frame_ticks = 0;
    actor.frame_state = -1;
    result = eng1_interpret_word_stream(&actor);
    if (result != 0 || actor.active_command != 0)
        return 15;

    /* Unknown triple pointers likewise use the existing behavior. */
    memset(&actor, 0, sizeof(actor));
    memset(control_storage, 0, sizeof(control_storage));
    control_pointer = control_storage;
    actor.animation_control = &control_pointer;
    unknown_triples[0].opcode = 0;
    unknown_triples[0].operand_002 = 3;
    unknown_triples[0].operand_004 = 1;
    actor.image_or_program = unknown_triples;
    actor.frame_ticks = 0;
    actor.frame_state = -1;
    result = eng1_interpret_animation_triples(&actor);
    if (result != 0 || actor.active_command != 0 || actor.frame_ticks != 1)
        return 16;

    puts("ENG1 runtime read-guard fixtures passed");
    return 0;
}
