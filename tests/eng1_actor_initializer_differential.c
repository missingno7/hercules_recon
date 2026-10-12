#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "../src/eng1/behavioral/interpreters.h"
#include "../src/eng1/behavioral/resource_runtime_data.h"
#include "../src/eng1/behavioral/actor_pipeline_storage.h"
#include "../src/eng1/behavioral/actor_control_state.h"
#include "../src/eng1/behavioral/initial_motion_roots.h"

#define PIPELINE_BYTES 0x8c0UL
#define STATE_BYTES 0x38UL

extern void __cdecl eng1_native_code_target_02cc50(Actor *actor);
extern Eng1InitialMotion2 g_eng1_initial_motion_05c3a0[6];

static HMODULE g_oracle;
static void (__cdecl *g_native_entry)(Actor *);
static unsigned long g_checks;
static unsigned long g_failures;
static unsigned char g_saved_native_state[STATE_BYTES];
static unsigned char g_saved_native_pipeline[PIPELINE_BYTES];
static signed char g_saved_native_active_row;
static unsigned char g_saved_source_state[STATE_BYTES];
static unsigned char g_saved_source_pipeline[PIPELINE_BYTES];
static signed char g_saved_source_active_row;
static int g_native_snapshot_ready;
static int g_source_snapshot_ready;
static void *g_normal_frames[8];
static unsigned long g_image_sentinel;

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

static void seed_bytes(unsigned char *bytes, unsigned long count,
                       unsigned char base)
{
    unsigned long i;
    for (i = 0; i < count; ++i)
        bytes[i] = (unsigned char)(base + i * 13UL);
}

static void make_actor(Actor *actor, void **frames, short frame_index)
{
    memset(actor, 0xa5, sizeof(*actor));
    actor->x = 0x00010000L;
    actor->y = 0x00020000L;
    actor->z = 0x00030000L;
    actor->frame_group = 0x1234;
    actor->direction_x = 0x7fU;
    actor->flags = 0x10101010UL;
    actor->render_flags = 0x12345600UL;
    actor->image_or_program = (void *)0x22222222UL;
    actor->steps = (void *)0x11111111UL;
    actor->step_index = -8L;
    actor->frames = frames;
    actor->frame_index = frame_index;
    actor->active_step = 0x1234;
    actor->action_callback_index = 0x5678;
}

static void save_original_cells(void)
{
    memcpy(g_saved_native_state, native_cell(0x70ed8UL), STATE_BYTES);
    memcpy(g_saved_native_pipeline, native_cell(0x71440UL), PIPELINE_BYTES);
    g_saved_native_active_row = *(signed char *)native_cell(0x70d01UL);
    memcpy(g_saved_source_state, &g_eng1_actor_control_state_70ed8,
           STATE_BYTES);
    memcpy(g_saved_source_pipeline, &g_actor_pipeline_storage_71440,
           PIPELINE_BYTES);
    g_saved_source_active_row = g_active_row_70d01;
    g_native_snapshot_ready = 1;
    g_source_snapshot_ready = 1;
}

static void restore_original_cells(void)
{
    if (g_oracle != NULL && g_native_snapshot_ready) {
        memcpy(native_cell(0x70ed8UL), g_saved_native_state, STATE_BYTES);
        memcpy(native_cell(0x71440UL), g_saved_native_pipeline,
               PIPELINE_BYTES);
        *(signed char *)native_cell(0x70d01UL) = g_saved_native_active_row;
    }
    if (g_source_snapshot_ready) {
        memcpy(&g_eng1_actor_control_state_70ed8, g_saved_source_state,
               STATE_BYTES);
        memcpy(&g_actor_pipeline_storage_71440, g_saved_source_pipeline,
               PIPELINE_BYTES);
        g_active_row_70d01 = g_saved_source_active_row;
    }
}

static void seed_case(unsigned long case_id, int seed_native, int seed_source)
{
    unsigned char state_seed[STATE_BYTES];
    unsigned char pipeline_seed[PIPELINE_BYTES];
    unsigned short *state_words;
    unsigned short *pipeline_words;

    seed_bytes(state_seed, STATE_BYTES, (unsigned char)(0x21U + case_id));
    state_words = (unsigned short *)(void *)state_seed;
    state_words[26] = (unsigned short)(0x2468U + case_id);
    state_words[15] = 0U;
    if (case_id == 1UL)
        state_words[14] = 0x1234U;
    if (seed_native)
        memcpy(native_cell(0x70ed8UL), state_seed, STATE_BYTES);
    if (seed_source)
        memcpy(&g_eng1_actor_control_state_70ed8, state_seed, STATE_BYTES);

    seed_bytes(pipeline_seed, PIPELINE_BYTES,
               (unsigned char)(0x51U + case_id));
    pipeline_words = (unsigned short *)(void *)(pipeline_seed + 0x0c);
    pipeline_words[0] = 0xbeefU;
    pipeline_words[1] = 0x1234U;
    if (seed_native)
        memcpy(native_cell(0x71440UL), pipeline_seed, PIPELINE_BYTES);
    if (seed_source)
        memcpy(&g_actor_pipeline_storage_71440, pipeline_seed,
               PIPELINE_BYTES);

    if (seed_native)
        *(signed char *)native_cell(0x70d01UL) = (signed char)-1;
    if (seed_source)
        g_active_row_70d01 = (signed char)-1;
}

static int actor_bytes_match_except_live_pointers(const Actor *native_actor,
                                                   const Actor *source_actor)
{
    const unsigned char *native_bytes;
    const unsigned char *source_bytes;
    unsigned long i;

    native_bytes = (const unsigned char *)native_actor;
    source_bytes = (const unsigned char *)source_actor;
    for (i = 0; i < sizeof(*native_actor); ++i) {
        if ((i >= 0xf0UL && i < 0xf4UL) ||
            (i >= 0x110UL && i < 0x114UL))
            continue;
        ++g_checks;
        if (native_bytes[i] != source_bytes[i]) {
            ++g_failures;
            printf("FAIL actor byte +0x%lx: native=%02x source=%02x\n",
                   i, native_bytes[i], source_bytes[i]);
            return 0;
        }
    }
    return 1;
}

static int compare_state_bytes(const unsigned char *native_bytes)
{
    const unsigned char *source_bytes;
    unsigned long i;

    source_bytes = (const unsigned char *)&g_eng1_actor_control_state_70ed8;
    for (i = 0; i < STATE_BYTES; ++i) {
        ++g_checks;
        if (native_bytes[i] != source_bytes[i]) {
            ++g_failures;
            printf("FAIL state byte +0x%lx: native=%02x source=%02x\n",
                   i, native_bytes[i], source_bytes[i]);
            return 0;
        }
    }
    return 1;
}

static int compare_pipeline_bytes(const unsigned char *native_bytes)
{
    const unsigned char *source_bytes;
    unsigned long i;

    source_bytes = (const unsigned char *)&g_actor_pipeline_storage_71440;
    for (i = 0; i < PIPELINE_BYTES; ++i) {
        ++g_checks;
        if (native_bytes[i] != source_bytes[i]) {
            ++g_failures;
            printf("FAIL pipeline byte +0x%lx: native=%02x source=%02x\n",
                   i, native_bytes[i], source_bytes[i]);
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
    Actor native_actor, source_actor;
    void **native_frames;
    void **source_frames;
    unsigned long case_id;
    void *expected_image;
    unsigned long expected_live_selector;
    unsigned char native_state_after[STATE_BYTES];
    unsigned char native_pipeline_after[PIPELINE_BYTES];
    int result;

    g_oracle = NULL;
    g_native_snapshot_ready = 0;
    g_source_snapshot_ready = 0;
    result = 1;

    CHECK(argc == 2, "pass the read-only ENG1.DLL path");
    g_oracle = LoadLibraryExA(argv[1], NULL, DONT_RESOLVE_DLL_REFERENCES);
    CHECK(g_oracle != NULL, "load the original DLL without resolving imports");
    g_native_entry = (void (__cdecl *)(Actor *))(native_cell(0x2cc50UL));
    save_original_cells();

    g_normal_frames[0] = NULL;
    g_normal_frames[1] = NULL;
    g_normal_frames[2] = (void *)3UL;
    g_normal_frames[3] = &g_image_sentinel;
    g_normal_frames[4] = NULL;
    g_normal_frames[5] = NULL;
    g_normal_frames[6] = NULL;
    g_normal_frames[7] = NULL;

    for (case_id = 0; case_id < 3UL; ++case_id) {
        seed_case(case_id, 1, 1);
        if (case_id == 0UL) {
            native_frames = g_normal_frames;
            source_frames = g_normal_frames;
            make_actor(&native_actor, native_frames, 0);
            make_actor(&source_actor, source_frames, 0);
            expected_image = &g_image_sentinel;
        } else if (case_id == 1UL) {
            native_frames = (void **)native_cell(0x70ed8UL);
            source_frames = (void **)(void *)
                &g_eng1_actor_control_state_70ed8.bytes[0];
            make_actor(&native_actor, native_frames, 6);
            make_actor(&source_actor, source_frames, 6);
            expected_image = (void *)0x00000066UL;
        } else {
            native_frames = (void **)native_cell(0x71440UL);
            source_frames = (void **)(void *)
                &g_actor_pipeline_storage_71440.byte_cells[0];
            make_actor(&native_actor, native_frames, 2);
            make_actor(&source_actor, source_frames, 2);
            expected_image = (void *)0x12340000UL;
        }

        g_native_entry(&native_actor);
        memcpy(native_state_after, native_cell(0x70ed8UL), STATE_BYTES);
        memcpy(native_pipeline_after, native_cell(0x71440UL),
               PIPELINE_BYTES);

        seed_case(case_id, 0, 1);
        if (case_id == 0UL) {
            source_frames = g_normal_frames;
            make_actor(&source_actor, source_frames, 0);
        } else if (case_id == 1UL) {
            source_frames = (void **)(void *)
                &g_eng1_actor_control_state_70ed8.bytes[0];
            make_actor(&source_actor, source_frames, 6);
        } else {
            source_frames = (void **)(void *)
                &g_actor_pipeline_storage_71440.byte_cells[0];
            make_actor(&source_actor, source_frames, 2);
        }
        eng1_native_code_target_02cc50(&source_actor);

        CHECK(actor_bytes_match_except_live_pointers(&native_actor,
                                                      &source_actor),
              "native/source actor state excluding the two environment pointers");
        CHECK(compare_state_bytes(native_state_after),
              "all 56 native/source control-state bytes, including untouched 70f0c");
        CHECK(compare_pipeline_bytes(native_pipeline_after),
              "all 0x8c0 native/source actor-pipeline bytes");
        CHECK(native_actor.image_or_program == expected_image &&
              source_actor.image_or_program == expected_image,
              "frame advance ran after the initializer's state writes");
        CHECK(native_actor.frame_index == source_actor.frame_index,
              "native/source frame-index progression");
        CHECK(native_actor.steps == (void *)(native_cell(0x5c3a0UL)) &&
              source_actor.steps == (void *)g_eng1_initial_motion_05c3a0,
              "native/source initial-step pointer targets");
        CHECK(*(const unsigned short *)(const void *)(native_state_after + 0x34UL) ==
                  (unsigned short)(0x2468U + case_id) &&
              g_eng1_actor_control_state_70ed8.words[26] ==
                  (unsigned short)(0x2468U + case_id),
              "0x70f0c remains unchanged across initialization");
        CHECK(*(const unsigned short *)(const void *)(native_pipeline_after + 0x0cUL) == 0U &&
              g_actor_pipeline_storage_71440.word_cells[6] == 0U,
              "shared actor-pipeline WORD 0x7144c reset");
        CHECK(native_actor.direction_x == 0U &&
              source_actor.direction_x == 0U,
              "actor BYTE 0x94 cleared");
        CHECK(native_actor.active_step == (short)-1 &&
              source_actor.active_step == (short)-1,
              "actor WORD 0x11e disabled after initial-step assignment");
        CHECK(native_actor.action_callback_index == 0 &&
              source_actor.action_callback_index == 0,
              "actor WORD 0x124 cleared");
        CHECK(native_actor.step_index == 1L && source_actor.step_index == 1L,
              "actor DWORD 0xf4 set to one after frame advance");
        CHECK(native_actor.render_flags == 0x12345608UL &&
              source_actor.render_flags == 0x12345608UL,
              "late DWORD render flag OR occurs after actor update");
        expected_live_selector = (unsigned long)(0x2468U + case_id);
        CHECK(expected_live_selector ==
                  (unsigned long)g_eng1_actor_control_state_70ed8.words[26],
              "selector guard seed remained available for later 0x2cd30 consumers");
        printf("case %lu: native/source actor, state and pipeline agree\n", case_id);
    }

    CHECK(memcmp(g_eng1_initial_motion_05c3a0,
                 native_cell(0x5c3a0UL), 24UL) == 0,
          "private six-pair source prefix matches the original 0x5c3a0 bytes");
    printf("cases=3 checks=%lu failures=%lu\n", g_checks, g_failures);
    result = g_failures ? 1 : 0;

cleanup:
    restore_original_cells();
    if (g_oracle != NULL) {
        FreeLibrary(g_oracle);
        g_oracle = NULL;
    }
    if (g_failures != 0UL)
        printf("checks=%lu failures=%lu\n", g_checks, g_failures);
    return result;
}
