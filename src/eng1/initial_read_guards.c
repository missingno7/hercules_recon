/* Private preflight guards keyed by genuine source object/member addresses. */
#include <stdio.h>
#include "initial_read_guards.h"
#include "incomplete.h"
#include "interpreters.h"
#include "initial_custom_roots.h"
#include "initial_motion_roots.h"
#include "initial_unreadable_identities.h"
#include "descriptor_prefix_data.h"
extern unsigned short eng1_native_words_04dce8[8];
extern unsigned short g_eng1_sound_words_66d68[8];

typedef struct Eng1InitialReadLimit {
    const void *base;
    unsigned long original_rva, bytes;
} Eng1InitialReadLimit;
static const Eng1InitialReadLimit limits[] = {
    { &g_eng1_descriptor_payload_4f350, 0x4f350UL, 16UL },
    { (const unsigned char *)&g_eng1_descriptor_payload_4f350 + 4, 0x4f354UL, 12UL },
    { eng1_native_words_04dce8, 0x4dce8UL, 16UL },
    { g_eng1_sound_words_66d68, 0x66d68UL, 16UL },
    { &g_eng1_custom_mask_palette_05ebb8.columns, 0x5ebb8UL, 52UL },
    { &g_eng1_custom_mask_palette_05ebb8.mask_cells[0], 0x5ebbcUL, 48UL },
    { &g_eng1_custom_mask_palette_05ebb8.mask_cells[5], 0x5ebd0UL, 32UL },
    { &g_eng1_custom_image_05ebf0, 0x5ebf0UL, 36UL },
    { g_eng1_initial_motion_0527d0, 0x527d0UL, 36UL },
    { g_eng1_initial_motion_05c3a0, 0x5c3a0UL, 24UL },
    { g_eng1_initial_motion_05fa20, 0x5fa20UL, 42UL },
    { g_eng1_initial_motion_060148, 0x60148UL, 90UL },
    { g_eng1_initial_motion_066230, 0x66230UL, 6UL },
    { eng1_resource_byte_list_04fec0, 0x4fec0UL, 557UL },
    { eng1_resource_byte_list_050d88, 0x50d88UL, 167UL },
    { &g_eng1_unreadable_initialized_identity_065750, 0x65750UL, 0UL }
};

void eng1_guard_initial_read(const void *base, unsigned long index,
                             unsigned long stride, unsigned long width,
                             const char *site)
{
    unsigned int i;
    char reason[224];
    for (i = 0; i < sizeof(limits)/sizeof(limits[0]); ++i) {
        if (base != limits[i].base) continue;
        /* Division preflight prevents index*stride unsigned wrap. */
        if (stride == 0 || width == 0 || width > limits[i].bytes ||
            index > (limits[i].bytes-width)/stride) {
            sprintf(reason, "%s index %lu stride %lu width %lu capacity %lu",
                    site, index, stride, width, limits[i].bytes);
            eng1_incomplete_behavior(limits[i].original_rva, reason);
        }
        return;
    }
}

void eng1_guard_initial_palette(const unsigned short *source,
                                unsigned long word_count, const char *site)
{
    /* Native counts0/1 read no source WORD; all other cases read the last
     * source WORD at count-1. Preflight it before ANY destination write. */
    if (word_count > 1UL)
        eng1_guard_initial_read(source, word_count-1UL, 2UL, 2UL, site);
}

void eng1_guard_initial_mask(const unsigned long *source,
                             unsigned long rows, const char *site)
{
    unsigned long columns;
    unsigned int i;
    eng1_guard_initial_read(source, 0UL, 4UL, 4UL, site);
    columns = *source;
    if (rows == 0UL || columns == 0UL) return;
    /* Source count4 is mutable. Avoid rows*columns before its bounds gate. */
    for (i = 0; i < sizeof(limits)/sizeof(limits[0]); ++i) {
        if (source != limits[i].base) continue;
        if (rows > (limits[i].bytes/4UL - 1UL)/columns)
            eng1_guard_initial_read(source, limits[i].bytes/4UL,
                                    4UL, 4UL, site);
        eng1_guard_initial_read(source, rows*columns, 4UL, 4UL, site);
        return;
    }
}

extern void __cdecl apply_frame_motion_2d(void *);
extern void __cdecl apply_frame_motion_3d(void *);
void __cdecl eng1_guarded_apply_frame_motion_2d(void *pointer)
{
    Actor *actor = (Actor *)pointer;
    if (actor->active_step >= 0)
        eng1_guard_initial_read(actor->steps, (unsigned long)actor->step_index,
                                4UL, 4UL, "motion2D");
    apply_frame_motion_2d(pointer);
}
void __cdecl eng1_guarded_apply_frame_motion_3d(void *pointer)
{
    Actor *actor = (Actor *)pointer;
    if (actor->active_step >= 0)
        eng1_guard_initial_read(actor->steps, (unsigned long)actor->step_index,
                                6UL, 6UL, "motion3D");
    apply_frame_motion_3d(pointer);
}
void eng1_guard_unreadable_identity(const void *base, const char *site)
{
    eng1_guard_initial_read(base, 0UL, 1UL, 1UL, site);
}
