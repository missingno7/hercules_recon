/* Private runtime guard candidate. Registry pointers are linked source
 * objects; original RVAs are diagnostic identities only. */
#include <stdio.h>
#include <string.h>
#include "incomplete.h"
#include "read_guard.h"

extern unsigned char g_eng1_provisional_pair_063248[];
extern unsigned char g_eng1_provisional_pair_06324c[];
extern unsigned char g_eng1_provisional_pair_063250[];
extern unsigned char g_eng1_provisional_pair_063254[];
extern unsigned char g_eng1_provisional_pair_063258[];
extern unsigned char g_eng1_provisional_pair_06325c[];
extern unsigned char g_eng1_provisional_pair_063260[];
extern unsigned char g_eng1_provisional_pair_063264[];
extern unsigned char g_eng1_provisional_pair_063a60[];
extern unsigned char g_eng1_provisional_pair_063a64[];
extern unsigned char g_eng1_provisional_pair_063ce0[];
extern unsigned char g_eng1_provisional_pair_063ce4[];
extern unsigned char g_eng1_provisional_pair_063ce8[];
extern unsigned char g_eng1_provisional_pair_063cec[];
extern unsigned char g_eng1_finite_pair_0591f0[];
extern unsigned char g_eng1_finite_pair_059228[];
extern unsigned char g_eng1_finite_pair_059288[];
extern unsigned char g_eng1_finite_pair_0592a8[];
extern unsigned char g_eng1_finite_pair_0592f8[];
extern unsigned char g_eng1_finite_pair_059300[];
extern unsigned char g_eng1_finite_pair_059350[];
extern unsigned char g_eng1_shared_selector_04dcb8[];
extern unsigned char g_eng1_shared_selector_060238[];
extern unsigned char g_eng1_shared_selector_066cc0[];
extern unsigned short g_eng1_flag_program_055538[4];
extern void *g_eng1_provisional_frames_06323c[];
extern void *g_eng1_provisional_frames_063268[];
extern void *g_eng1_provisional_frames_063290[];
extern void *g_eng1_provisional_frames_063a68[];
extern void *g_eng1_provisional_frames_063cf0[];
extern void *g_eng1_finite_frames_04f8e8[];
extern void *g_eng1_finite_frames_04f8f8[];
extern void *g_eng1_finite_frames_051590[];
extern void *g_eng1_finite_frames_059398[];
extern void *g_eng1_finite_frames_05c2d0[];
extern void *g_eng1_finite_frames_05fed0[];
extern void *g_eng1_finite_frames_066e48[];

typedef struct Eng1ProgramLimit {
    const void *pointer;
    unsigned long original_rva;
    unsigned int proven_bytes;
} Eng1ProgramLimit;

typedef struct Eng1FrameLimit {
    const void *pointer;
    unsigned long original_rva;
    unsigned int proven_slots;
} Eng1FrameLimit;

#define ENG1_SOURCE_OWNER(symbol, byte_addend) \
    ((const void *)((const unsigned char *)(symbol) + (byte_addend)))
static const Eng1ProgramLimit g_program_limits[] = {
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063248, 0U), 0x63248UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_06324c, 0U), 0x6324cUL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063250, 0U), 0x63250UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063254, 0U), 0x63254UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063258, 0U), 0x63258UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_06325c, 0U), 0x6325cUL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063260, 0U), 0x63260UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063264, 0U), 0x63264UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063a60, 0U), 0x63a60UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063a64, 0U), 0x63a64UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063ce0, 0U), 0x63ce0UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063ce4, 0U), 0x63ce4UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063ce8, 0U), 0x63ce8UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_pair_063cec, 0U), 0x63cecUL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_pair_0591f0, 0U), 0x591f0UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_pair_059228, 0U), 0x59228UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_pair_059288, 0U), 0x59288UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_pair_0592a8, 0U), 0x592a8UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_pair_0592f8, 0U), 0x592f8UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_pair_059300, 0U), 0x59300UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_pair_059350, 0U), 0x59350UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_shared_selector_04dcb8, 0U), 0x4dcb8UL, 44U },
    { ENG1_SOURCE_OWNER(g_eng1_shared_selector_060238, 0U), 0x60238UL, 44U },
    { ENG1_SOURCE_OWNER(g_eng1_shared_selector_066cc0, 0U), 0x66cc0UL, 44U },
    { ENG1_SOURCE_OWNER(g_eng1_flag_program_055538, 0U), 0x55538UL, 8U },
};

static const Eng1FrameLimit g_frame_limits[] = {
    { ENG1_SOURCE_OWNER(g_eng1_provisional_frames_06323c, 0U), 0x6323cUL, 1U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_frames_063268, 0U), 0x63268UL, 10U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_frames_063290, 0U), 0x63290UL, 10U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_frames_063a68, 0U), 0x63a68UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_provisional_frames_063cf0, 0U), 0x63cf0UL, 6U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_frames_04f8e8, 0U), 0x4f8e8UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_frames_04f8f8, 0U), 0x4f8f8UL, 4U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_frames_051590, 0U), 0x51590UL, 5U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_frames_059398, 0U), 0x59398UL, 10U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_frames_05c2d0, 0U), 0x5c2d0UL, 52U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_frames_05fed0, 0U), 0x5fed0UL, 36U },
    { ENG1_SOURCE_OWNER(g_eng1_finite_frames_066e48, 0U), 0x66e48UL, 15U },
};

static const Eng1ProgramLimit *eng1_find_program_limit(const void *pointer)
{
    unsigned int i;
    for (i = 0; i < sizeof(g_program_limits) / sizeof(g_program_limits[0]); ++i)
        if (g_program_limits[i].pointer == pointer)
            return &g_program_limits[i];
    return 0;
}

static const Eng1FrameLimit *eng1_find_frame_limit(const void *pointer)
{
    unsigned int i;
    for (i = 0; i < sizeof(g_frame_limits) / sizeof(g_frame_limits[0]); ++i)
        if (g_frame_limits[i].pointer == pointer)
            return &g_frame_limits[i];
    return 0;
}

static void eng1_report_read_failure(unsigned long original_rva,
                                     const char *site, long index,
                                     unsigned int stride,
                                     unsigned int width,
                                     unsigned int capacity,
                                     const char *unit)
{
    char reason[224];
    sprintf(reason, "%s %s index %ld stride %u width %u capacity %u",
            site, unit, index, stride, width, capacity);
    eng1_incomplete_behavior(original_rva, reason);
}

void eng1_guard_program_read(const void *program, long index,
                            unsigned int stride, unsigned int width,
                            const char *site)
{
    const Eng1ProgramLimit *limit;
    unsigned int last_index;

    limit = eng1_find_program_limit(program);
    if (limit == 0) return;
    if (stride == 0 || width == 0 || width > stride ||
        width > limit->proven_bytes) {
        eng1_report_read_failure(limit->original_rva, site, index, stride,
                                 width, limit->proven_bytes, "program");
        return;
    }
    last_index = (limit->proven_bytes - width) / stride;
    if (index < 0 || (unsigned long)index > (unsigned long)last_index)
        eng1_report_read_failure(limit->original_rva, site, index, stride,
                                 width, limit->proven_bytes, "program");
}

void eng1_guard_frame_slot(const void *frames, long index, const char *site)
{
    const Eng1FrameLimit *limit;

    limit = eng1_find_frame_limit(frames);
    if (limit == 0) return;
    if (index < 0 || (unsigned long)index >= (unsigned long)limit->proven_slots)
        eng1_report_read_failure(limit->original_rva, site, index,
                                 sizeof(void *), sizeof(void *),
                                 limit->proven_slots, "frame-slot");
}

static short eng1_guard_inc_word(short value)
{
    return (short)((unsigned short)value + 1U);
}

void __cdecl eng1_guarded_advance_frame(Actor *actor)
{
    const Eng1FrameLimit *limit;
    void **frames;
    void *image;
    short cursor;
    short jump_slot;
    unsigned int transition_count;

    frames = actor->frames;
    limit = eng1_find_frame_limit(frames);
    if (limit == 0) {
        advance_frame(actor);
        return;
    }

    cursor = eng1_guard_inc_word(actor->frame_index);
    transition_count = 0;
    for (;;) {
        eng1_guard_frame_slot(frames, (long)cursor, "advance_frame data slot");
        image = frames[cursor];
        if (image != 0) break;

        jump_slot = eng1_guard_inc_word(cursor);
        eng1_guard_frame_slot(frames, (long)jump_slot,
                              "advance_frame null-following jump slot");
        cursor = (short)(long)frames[jump_slot];
        ++transition_count;
        if (transition_count >= limit->proven_slots) {
            /* With a stable table, at least one in-prefix state has repeated.
             * The exact helper will preserve any resulting native cycle. */
            advance_frame(actor);
            return;
        }
    }

    advance_frame(actor);
}

void __cdecl eng1_guarded_select_frame(Actor *actor, int index)
{
    if (eng1_find_frame_limit(actor->frames) != 0)
        eng1_guard_frame_slot(actor->frames, (long)index,
                              "select_frame direct slot");
    select_frame(actor, index);
}
