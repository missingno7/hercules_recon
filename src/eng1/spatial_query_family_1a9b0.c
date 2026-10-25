/* Original-backed ENG1 spatial query family. Native entry/ABI evidence and
 * finite comparison limits: evidence/eng1_spatial_query_recovery_2026_10_25.json.
 * The external query-table owner/capacity remains unresolved. */
#include "actor_bounds_2a880.h"
#include "actor_pipeline_storage_views.h"
#include "setup_shared_state.h"
#include "../shared/engine_interface.h"

typedef char eng1_query_short_is_two_bytes[(sizeof(short) == 2) ? 1 : -1];
typedef char eng1_query_bounds_is_twelve_bytes[
    (sizeof(Query2a170Bounds) == 12) ? 1 : -1];
typedef char eng1_query_long_is_four[(sizeof(unsigned long) == 4) ? 1 : -1];
typedef char eng1_query_pointer_is_four[(sizeof(void *) == 4) ? 1 : -1];

/* The same native data name is already referenced by render_unit_30010.cpp.
 * Its source owner/capacity remains unresolved; do not invent either here. */
extern unsigned char g_71ee0[];
extern unsigned short g_pipeline_word_6fec4;

/* Reuse the accepted 0x2a7a0 provider; no duplicate implementation. */
extern int __cdecl bounds_overlap(
    Query2a170Bounds *left, Query2a170Bounds *right);

unsigned long __cdecl eng1_1a9e0(Query2a170Bounds *query_bounds)
{
    unsigned char *owner;
    unsigned short count;
    short *query_words;
    short query_min_x;
    short query_min_y;
    short query_min_z;
    short query_max_x;
    short query_max_y;
    short query_max_z;
    unsigned long index;

    owner = g_eng1_setup_pointer_72020;
    if (*(unsigned short *)(void *)(owner + 0x2e) == 0)
        return 0;

    count = *(unsigned short *)(void *)(owner + 0x36);
    if (count == 0)
        return 0;

    /* Native copies the complete six-WORD query before its first table read. */
    query_words = (short *)(void *)query_bounds;
    query_max_z = query_words[5];
    query_min_z = query_words[2];
    query_min_y = query_words[1];
    query_max_y = query_words[4];
    query_max_x = query_words[3];
    query_min_x = query_words[0];

    for (index = 0; index < (unsigned long)count; ++index) {
        short *record = (short *)(void *)(
            g_71ee0 + index * 12UL);
        short min_x = record[0];
        short min_y = record[1];
        short min_z = record[2];
        short max_x = record[3];
        short max_y = record[4];
        short max_z = record[5];
        int separated;

        separated = (max_y < query_min_y);
        separated |= (query_max_z < min_z);
        separated |= (query_max_y < min_y);
        separated |= (max_x < query_min_x);
        separated |= (query_max_x < min_x);
        separated |= (max_z < query_min_z);
        if (separated == 0) {
            unsigned char *current_owner;
            unsigned short selected;

            selected = (unsigned short)(index + 1UL);
            /* Native reloads this cell at the write site; retain the source
             * reload without asserting cross-thread volatile semantics. */
            current_owner = g_eng1_setup_pointer_72020;
            *(unsigned short *)(void *)(current_owner + 0x36) = selected;
            return (unsigned long)selected;
        }
    }

    return 0;
}

unsigned long __cdecl eng1_1ab00(Query2a170Bounds *query_bounds)
{
    unsigned char *context_actor;
    Query2a170Bounds context_bounds;

    context_actor = g_eng1_setup_pointer_72018;
    if (*(unsigned short *)(void *)(context_actor + 0x2e) == 0)
        return 0;

    if (!actor_bounds_2a880(context_actor, &context_bounds, 2UL))
        return 0;

    if (!bounds_overlap(query_bounds, &context_bounds))
        return 0;

    return 0x20;
}

unsigned long __cdecl eng1_1a9b0(Query2a170Bounds *query_bounds)
{
    unsigned long selection;
    ResourceCallbackContext *context;

    selection = eng1_1a9e0(query_bounds);
    if (selection != 0)
        return selection;

    /* The native load follows 0x1a9e0; keep the mutable global reread here. */
    context = g_engine_interface.context_004;
    if (*(unsigned char *)(void *)context == 9)
        return 0;

    return eng1_1ab00(query_bounds);
}

unsigned long __cdecl eng1_26de0(void *query_actor)
{
    Query2a170Bounds query_bounds;
    long *worklist_count;
    unsigned long *worklist;
    unsigned long selection;
    long index;

    if (!actor_bounds_2a880(query_actor, &query_bounds, 1UL))
        return 0;

    selection = eng1_1a9b0(&query_bounds);
    if (selection != 0)
        return selection;

    worklist = (unsigned long *)(void *)g_actor_worklist_71900;
    worklist_count = (long *)(void *)g_actor_worklist_71900;
    index = 0;
    if (*worklist_count > 0) {
        do {
            unsigned long actor_value;
            unsigned char *candidate_actor;
            unsigned long *position;
            unsigned long z;
            unsigned long y;
            unsigned long x;

            ++index;
            actor_value = worklist[index];
            candidate_actor = (unsigned char *)(void *)actor_value;
            position = (unsigned long *)(void *)candidate_actor;
            z = position[2];
            y = position[1];
            x = position[0];
            if (point_in_box_2a810(x, y, z, &query_bounds) != 0)
                return (unsigned long)(void *)candidate_actor;
        } while (index < *worklist_count);
    }

    if (g_pipeline_word_6fec4 != 0 &&
        bounds_overlap(
            (Query2a170Bounds *)(void *)g_actor_pipeline_storage_71440.byte_cells,
            &query_bounds) != 0)
        return 0x80UL;

    return 0;
}
