/* Finite native differential for the private actor-control tail candidate.
 * The fixture calls selected original ENG1 RVAs only in this separate x86
 * process. It never patches code or writes the oracle file. Loaded data cells
 * modified for controlled fixtures are snapshotted and restored. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/actor_control_state.h"
#include "../src/eng1/actor_control_cells.h"
#include "../src/eng1/group_layouts.h"
#include "../src/eng1/interpreters.h"
#include "../src/eng1/actor_control_tail.h"
#include "../src/eng1/actor_control_tables.h"

typedef unsigned char diff_u8;
typedef unsigned short diff_u16;
typedef unsigned long diff_u32;
typedef int (__cdecl *NativeFind)(Actor *, diff_u32);
typedef void (__cdecl *NativeAdvance)(Actor *);

typedef struct TestRecord TestRecord;
struct TestRecord { short opcode, x, y; };

static HMODULE oracle_module;
static diff_u8 oracle_control_before[0x38];
static diff_u8 oracle_collision_before[4];
static diff_u8 oracle_classifier_before[216];
static diff_u8 oracle_group_before[0x80];
static diff_u8 candidate_classifier_before[216];
static diff_u8 candidate_group_before[0x80];
static diff_u8 candidate_control_before[0x38];
static unsigned char *candidate_collision_before;

static diff_u8 *oracle_rva(diff_u32 rva)
{
    return (diff_u8 *)(void *)oracle_module + rva;
}

static int check(int condition, const char *name)
{
    if (!condition) {
        printf("FAIL %s\n", name);
        return 0;
    }
    printf("PASS %s\n", name);
    return 1;
}

static void initialize_actor(Actor *actor, TestRecord *records,
                             void **frames)
{
    int i;
    memset(actor, 0, sizeof(*actor));
    actor->frames = frames;
    actor->initial_steps = records;
    actor->unknown_11c = 0;
    for (i = 0; i < 64; ++i)
        frames[i] = (void *)(unsigned long)(i + 1);
}

static void prepare_control(diff_u8 state[0x38], int index)
{
    memset(state, 0, 0x38);
    *(short *)(void *)(state + 0x1a) = (short)index;
}

static int run_find_case(const char *name, int matching)
{
    Actor native_actor, source_actor;
    TestRecord records[4];
    void *frames[64];
    diff_u8 control_seed[0x38], native_control[0x38], source_control[0x38];
    NativeFind native_find;
    int native_result, source_result;

    memset(records, 0, sizeof(records));
    if (matching) {
        records[0].opcode = 1;
        records[1].opcode = 20; records[1].x = -9; records[1].y = 7;
        records[2].opcode = 20; records[2].x = 0; records[2].y = 5;
        records[3].opcode = 0;
    } else {
        records[0].opcode = 20; records[0].y = 9;
        records[1].opcode = 1;
        records[2].opcode = 0;
    }
    initialize_actor(&native_actor, records, frames);
    initialize_actor(&source_actor, records, frames);
    native_actor.unknown_11c = source_actor.unknown_11c = 33;
    prepare_control(control_seed, 0);
    *(diff_u16 *)(void *)(control_seed + 0x12) = 44;

    memcpy(oracle_rva(0x70ed8UL), control_seed, sizeof(control_seed));
    native_find = (NativeFind)(void *)oracle_rva(0x2d3f0UL);
    native_result = native_find(&native_actor, matching ? 5UL : 8UL);
    memcpy(native_control, oracle_rva(0x70ed8UL), sizeof(native_control));
    memcpy(oracle_rva(0x70ed8UL), oracle_control_before,
           sizeof(oracle_control_before));

    memcpy(&g_eng1_actor_control_state_70ed8, control_seed,
           sizeof(control_seed));
    source_result = eng1_native_dep_002d3f0(&source_actor,
                                            matching ? 5UL : 8UL);
    memcpy(source_control, &g_eng1_actor_control_state_70ed8,
           sizeof(source_control));

    return check(native_result == source_result &&
                     memcmp(&native_actor, &source_actor,
                            sizeof(native_actor)) == 0 &&
                     memcmp(native_control, source_control,
                            sizeof(native_control)) == 0,
                 name);
}

static int run_advance_case(const char *name, short opcode, short x, short y,
                            short record_index, short state_code,
                            int table_word, int table_value,
                            int bounds_fixture, long root_x,
                            int gate_fixture, int gate_equal)
{
    Actor native_actor, source_actor;
    TestRecord records[4];
    void *frames[64];
    diff_u8 control_seed[0x38], native_control[0x38], source_control[0x38];
    diff_u8 *native_control_cell, *native_collision_cell;
    diff_u8 *native_table_cell, *native_group_cell;
    diff_u8 saved_table_word[2], saved_source_table_word[2];
    short *native_record_row;
    short *source_record_row;
    unsigned long *gate_words;
    long root_value;
    NativeAdvance native_advance;
    int actor_equal, control_equal, restored;
    int i;

    memset(records, 0, sizeof(records));
    records[record_index].opcode = opcode;
    records[record_index].x = x;
    records[record_index].y = y;
    initialize_actor(&native_actor, records, frames);
    initialize_actor(&source_actor, records, frames);
    native_actor.unknown_11c = source_actor.unknown_11c = record_index;
    native_actor.unknown_02a = source_actor.unknown_02a = state_code;
    prepare_control(control_seed, 0);

    if (opcode == 11) {
        native_actor.x = source_actor.x = 0x01000000L;
        native_actor.z = source_actor.z = (long)0xff000000UL;
        *((diff_u8 *)&native_actor + 0x1f) = 1;
        *((diff_u8 *)&source_actor + 0x1f) = 1;
    }
    if (opcode == 2) {
        native_actor.x = source_actor.x = 0x00800000L;
    }
    if (gate_fixture) {
        gate_words = (unsigned long *)0;
        {
            static diff_u16 transition_gate[0x128 / 2];
            memset(transition_gate, 0, sizeof(transition_gate));
            transition_gate[0x124 / 2] = 5;
            transition_gate[0x126 / 2] = gate_equal ? 5 : 6;
            gate_words = (unsigned long *)(void *)transition_gate;
            *(diff_u32 *)(void *)(control_seed + 0x14) =
                (diff_u32)(unsigned long)gate_words;
        }
    }

    native_control_cell = oracle_rva(0x70ed8UL);
    native_collision_cell = oracle_rva(0x7217cUL);
    native_table_cell = oracle_rva(0x65678UL);
    native_group_cell = oracle_rva(0x70e4cUL);
    if (table_word >= 0) {
        native_record_row = (short *)(void *)
            (native_table_cell + 4 + table_word * 12);
        saved_table_word[0] = ((diff_u8 *)native_record_row)[0];
        saved_table_word[1] = ((diff_u8 *)native_record_row)[1];
        *native_record_row = (short)table_value;
        source_record_row = &g_eng1_classifier_rows_65678[table_word][2];
        saved_source_table_word[0] = ((diff_u8 *)source_record_row)[0];
        saved_source_table_word[1] = ((diff_u8 *)source_record_row)[1];
        *source_record_row = (short)table_value;
    }
    if (bounds_fixture) {
        /* Signed class -1 reads cumulative owner words 13 and 14, which are
         * six and four bytes before the observed 0x70e6c member base. */
        *(short *)(void *)(native_group_cell + 0x1a) = 0;
        *(short *)(void *)(native_group_cell + 0x1c) = -1;
        g_resource_group_state_70e4c.cumulative_by_row[13] = 0;
        g_resource_group_state_70e4c.cumulative_by_row[14] = -1;
        root_value = root_x;
        *(diff_u32 *)(void *)native_collision_cell =
            (diff_u32)(unsigned long)&root_value;
        g_collision_context = (diff_u8 *)(void *)&root_value;
    } else {
        root_value = root_x;
    }

    memcpy(native_control_cell, control_seed, sizeof(control_seed));
    native_advance = (NativeAdvance)(void *)oracle_rva(0x2d440UL);
    native_advance(&native_actor);
    memcpy(native_control, native_control_cell, sizeof(native_control));
    /* Restore every loaded oracle data region touched by this controlled run. */
    memcpy(native_control_cell, oracle_control_before,
           sizeof(oracle_control_before));
    memcpy(native_collision_cell, oracle_collision_before,
           sizeof(oracle_collision_before));
    memcpy(native_table_cell, oracle_classifier_before,
           sizeof(oracle_classifier_before));
    memcpy(native_group_cell, oracle_group_before,
           sizeof(oracle_group_before));
    restored =
        memcmp(native_control_cell, oracle_control_before,
               sizeof(oracle_control_before)) == 0 &&
        memcmp(native_collision_cell, oracle_collision_before,
               sizeof(oracle_collision_before)) == 0 &&
        memcmp(native_table_cell, oracle_classifier_before,
               sizeof(oracle_classifier_before)) == 0 &&
        memcmp(native_group_cell, oracle_group_before,
               sizeof(oracle_group_before)) == 0;

    memcpy(&g_eng1_actor_control_state_70ed8, control_seed,
           sizeof(control_seed));
    eng1_native_dep_002d440(&source_actor);
    memcpy(source_control, &g_eng1_actor_control_state_70ed8,
           sizeof(source_control));
    if (table_word >= 0)
        memcpy(source_record_row, saved_source_table_word,
               sizeof(saved_source_table_word));
    if (bounds_fixture) {
        memcpy(&g_resource_group_state_70e4c, candidate_group_before,
               sizeof(candidate_group_before));
        g_collision_context = 0;
    }

    actor_equal = restored &&
        memcmp(&native_actor, &source_actor, sizeof(native_actor)) == 0;
    control_equal = memcmp(native_control, source_control,
                           sizeof(native_control)) == 0;
    if (!actor_equal || !control_equal) {
        for (i = 0; i < sizeof(native_actor); ++i) {
            if (((diff_u8 *)&native_actor)[i] !=
                ((diff_u8 *)&source_actor)[i])
                printf("  actor+%03x native=%02x source=%02x\n", i,
                       ((diff_u8 *)&native_actor)[i],
                       ((diff_u8 *)&source_actor)[i]);
        }
        for (i = 0; i < sizeof(native_control); ++i) {
            if (native_control[i] != source_control[i])
                printf("  control+%02x native=%02x source=%02x\n", i,
                       native_control[i], source_control[i]);
        }
    }
    return check(actor_equal && control_equal, name);
}

void __cdecl eng1_incomplete_behavior(unsigned long original_rva,
                                     const char *reason)
{
    fprintf(stderr, "candidate incomplete path RVA=%08lx: %s\n",
            original_rva, reason);
    ExitProcess(90);
}

void __cdecl play_sound(unsigned short effect_id, Actor *actor)
{
    fprintf(stderr, "unexpected audio path in finite fixture: id=%04x actor=%p\n",
            (unsigned int)effect_id, (void *)actor);
    ExitProcess(91);
}

void eng1_guard_initial_read(const void *base, unsigned long index,
                             unsigned long stride, unsigned long width,
                             const char *site)
{
    /* Controlled fixture records are runtime buffers, not any measured
     * initial-data root registered by the production guard provider. */
    (void)base;
    (void)index;
    (void)stride;
    (void)width;
    (void)site;
}

int main(int argc, char **argv)
{
    diff_u8 *oracle_table, *oracle_group;
    int ok;

    if (argc != 2) {
        fprintf(stderr, "usage: native_diff.exe ENG1.DLL\n");
        return 2;
    }
    oracle_module = LoadLibraryA(argv[1]);
    if (oracle_module == 0) {
        fprintf(stderr, "LoadLibraryA failed: %lu\n", GetLastError());
        return 3;
    }

    memcpy(candidate_control_before, &g_eng1_actor_control_state_70ed8, 0x38);
    candidate_collision_before = g_collision_context;
    memcpy(oracle_control_before, oracle_rva(0x70ed8UL),
           sizeof(oracle_control_before));
    memcpy(oracle_collision_before, oracle_rva(0x7217cUL),
           sizeof(oracle_collision_before));
    oracle_table = oracle_rva(0x65678UL);
    oracle_group = oracle_rva(0x70e4cUL);
    memcpy(oracle_classifier_before, oracle_table,
           sizeof(oracle_classifier_before));
    memcpy(oracle_group_before, oracle_group, sizeof(oracle_group_before));
    memcpy(candidate_classifier_before, g_eng1_classifier_rows_65678,
           sizeof(candidate_classifier_before));
    memcpy(candidate_group_before, &g_resource_group_state_70e4c,
           sizeof(candidate_group_before));
    if (!check(memcmp(oracle_classifier_before,
                      candidate_classifier_before, 216) == 0,
               "shared-classifier-prefix-matches-loaded-oracle"))
        return 4;
    if (!check(memcmp(oracle_group_before, candidate_group_before, 0x80) == 0,
               "resource-group-prefix-matches-loaded-oracle"))
        return 4;

    ok = 1;
    ok &= run_find_case("2d3f0-match-return-actor-and-control", 1);
    ok &= run_find_case("2d3f0-terminator-miss-return-actor-and-control", 0);
    ok &= run_advance_case("opcode-0-no-op", 0, 0, 0, 0, 0,
                           -1, 0, 0, 0, 0, 0);
    ok &= run_advance_case("opcode-1-fixed-xy", 1, -2, 3, 0, 0,
                           -1, 0, 0, 0, 0, 0);
    ok &= run_advance_case("opcode-2-state-15-selects-frame-seven", 2,
                           0, 0, 0, 0x15, -1, 0, 0, 0, 0, 0);
    ok &= run_advance_case("opcode-3-stores-record-x-and-control-y", 3,
                           7, 0x12, 0, 0, -1, 0, 0, 0, 0, 0);
    ok &= run_advance_case("opcode-4-equal-external-gate", 4, 0, 0,
                           0, 0, -1, 0, 0, 0, 1, 1);
    ok &= run_advance_case("opcode-4-unequal-external-gate", 4, 0, 0,
                           0, 0, -1, 0, 0, 0, 1, 0);
    ok &= run_advance_case("opcode-7-selects-2e-and-advances", 7, 0, 0,
                           0, 0, -1, 0, 0, 0, 0, 0);
    ok &= run_advance_case("opcode-8-fixed-z", 8, -3, 0, 0, 0,
                           -1, 0, 0, 0, 0, 0);
    ok &= run_advance_case("opcode-10-default-state-increments", 10, 0, 0,
                           0, 0, -1, 0, 0, 0, 0, 0);
    ok &= run_advance_case("opcode-11-negative-row-cache-before-bounds",
                           11, -0x100, 0x100, 0, 0, -1, 0, 1, 0, 0, 0);
    ok &= run_advance_case("opcode-12-y-index-below-four-increments-only",
                           12, 3, 2, 2, 0, 2, 3, 0, 0, 0, 0);
    ok &= run_advance_case("opcode-12-y-index-at-least-four-adds-x",
                           12, 3, 2, 2, 0, 2, 5, 0, 0, 0, 0);
    ok &= run_advance_case("opcode-13-selects-zero-and-advances", 13,
                           0, 0, 0, 0, -1, 0, 0, 0, 0, 0);
    ok &= run_advance_case("opcode-14-is-no-op", 14, 0, 0, 0, 0,
                           -1, 0, 0, 0, 0, 0);
    ok &= run_advance_case("opcode-15-default-increments", 15, 0, 0,
                           0, 0, -1, 0, 0, 0, 0, 0);

    memcpy(oracle_rva(0x70ed8UL), oracle_control_before,
           sizeof(oracle_control_before));
    memcpy(oracle_rva(0x7217cUL), oracle_collision_before,
           sizeof(oracle_collision_before));
    memcpy(oracle_rva(0x65678UL), oracle_classifier_before,
           sizeof(oracle_classifier_before));
    memcpy(oracle_rva(0x70e4cUL), oracle_group_before,
           sizeof(oracle_group_before));
    memcpy(&g_eng1_actor_control_state_70ed8, candidate_control_before,
           sizeof(oracle_control_before));
    memcpy(g_eng1_classifier_rows_65678, candidate_classifier_before,
           sizeof(candidate_classifier_before));
    memcpy(&g_resource_group_state_70e4c, candidate_group_before,
           sizeof(candidate_group_before));
    g_collision_context = candidate_collision_before;

    if (!ok)
        return 1;
    if (!FreeLibrary(oracle_module)) return 2;
    printf("bounded native differential passed; original loaded data cells restored\n");
    printf("actor_tail_native_cases=17 checks=19 failures=0\n");
    return 0;
}
