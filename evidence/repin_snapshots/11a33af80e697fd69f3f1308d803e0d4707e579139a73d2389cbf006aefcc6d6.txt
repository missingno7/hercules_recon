/*
 * Bounded ordinary-C behavioral reconstruction for ENG1 RVA 0x4ca0.
 *
 * Finite native differential evidence does not claim arbitrary-input coverage.
 * It models the measured counted records and byte-program path while using
 * separately owned ENG1 state supplied by the integrating graph.  The known
 * 32-entry flush uses the existing recovered dispatchers and resumes from the
 * consumer-cleared shared queue.  No target bytes are copied or called.
 */
#include <stddef.h>

typedef unsigned char Eng1Byte;
typedef unsigned short Eng1Word;
typedef unsigned long Eng1Dword;

typedef struct Eng1PayloadLinkRow {
    Eng1Word live_mask;
    Eng1Word descriptor;
    Eng1Word successor[4];
} Eng1PayloadLinkRow;

extern Eng1PayloadLinkRow g_actor_link_rows[256];
extern signed char g_actor_link_map[16];
extern Eng1Byte g_actor_group_sequence_5e138[112];
extern void *g_frame_owner_72870;
extern Eng1Word g_frame_upload_coordinates_734a0[32];
extern Eng1Dword g_frame_upload_pending_6ee7c;
extern Eng1Dword g_frame_upload_cursor_6ee78;
extern Eng1Byte g_frame_decode_scratch_6e660[0x400];
extern void __cdecl eng1_incomplete_behavior(Eng1Dword, const char *);
extern void __cdecl dispatch_engine_2c630(Eng1Dword);
extern void __cdecl dispatch_local_050d0(void);

static void __cdecl payload_unsupported(const char *reason)
{
    eng1_incomplete_behavior(0x4ca0UL, reason);
}

static int __cdecl choose_existing_mask(Eng1Word live_mask,
                                        int payload_class,
                                        Eng1Word *mask_out,
                                        int *slot_out)
{
    if (live_mask == 0x000fU)
        return 0;

    if (payload_class == 0) {
        if ((live_mask & 0x0001U) == 0) {
            *mask_out = 0x0001U;
            *slot_out = 0;
            return 1;
        }
        if ((live_mask & 0x0002U) == 0) {
            *mask_out = 0x0002U;
            *slot_out = 1;
            return 1;
        }
        if ((live_mask & 0x0004U) == 0) {
            *mask_out = 0x0004U;
            *slot_out = 2;
            return 1;
        }
        if ((live_mask & 0x0008U) == 0) {
            *mask_out = 0x0008U;
            *slot_out = 3;
            return 1;
        }
        return 0;
    }

    if (payload_class == 1) {
        if ((live_mask & 0x0003U) == 0) {
            *mask_out = 0x0003U;
            *slot_out = 0;
            return 1;
        }
        if ((live_mask & 0x000cU) == 0) {
            *mask_out = 0x000cU;
            *slot_out = 2;
            return 1;
        }
        return 0;
    }

    if (payload_class == 2 && (live_mask & 0x0005U) == 0) {
        *mask_out = 0x0005U;
        *slot_out = 0;
        return 1;
    }
    return 0;
}

static int __cdecl find_empty_group(Eng1Dword flags, Eng1Dword cursor,
                                    Eng1Dword *sequence_index)
{
    long index;
    Eng1Dword tries;

    if ((flags & 0x40UL) != 0) {
        for (index = 111; index > 84; --index) {
            Eng1Byte group;
            group = g_actor_group_sequence_5e138[index];
            if (g_actor_link_rows[group].live_mask == 0) {
                *sequence_index = (Eng1Dword)index;
                return 1;
            }
        }
    }

    index = (long)(cursor % 84UL);
    for (tries = 0; tries < 84UL; ++tries) {
        Eng1Byte group;
        group = g_actor_group_sequence_5e138[index];
        if (g_actor_link_rows[group].live_mask == 0) {
            *sequence_index = (Eng1Dword)index;
            return 1;
        }
        ++index;
        if (index >= 84)
            index = 0;
    }
    return 0;
}

static void __cdecl release_first_chain(Eng1Word key)
{
    while (key != 0xffffU) {
        Eng1Word group;
        Eng1Word mask;
        int successor_slot;

        group = (Eng1Word)(key >> 4);
        mask = (Eng1Word)(key & 0x000fU);
        if (group >= 256U || mask >= 16U) {
            payload_unsupported("0x4ca0 link key leaves measured link-row prefix");
            return;
        }
        g_actor_link_rows[group].live_mask &=
            (Eng1Word)(0x000fU - mask);
        successor_slot = (int)g_actor_link_map[mask];
        if (successor_slot < 0 || successor_slot >= 4) {
            payload_unsupported("0x4ca0 link-map result leaves four successor slots");
            return;
        }
        key = g_actor_link_rows[group].successor[successor_slot];
    }
}

static int __cdecl decode_program(const Eng1Byte **stream,
                                 Eng1Byte **destination,
                                 Eng1Dword copy_words,
                                 Eng1Dword row_count)
{
    Eng1Byte *scratch;
    Eng1Byte *scratch_end;
    Eng1Byte *read;
    Eng1Byte *write;
    Eng1Dword rows;

    scratch = g_frame_decode_scratch_6e660;
    scratch_end = scratch + 0x400;
    read = scratch;
    write = scratch;
    for (;;) {
        signed char command;

        command = (signed char)**stream;
        ++*stream;
        if (command <= 0) {
            if (write == scratch_end) {
                payload_unsupported("0x4ca0 literal exceeds the measured 0x400 scratch prefix");
                return 0;
            }
            *write++ = (Eng1Byte)command;
            continue;
        }
        if (command == 2)
            break;
        if ((Eng1Dword)(scratch_end - write) < (Eng1Dword)(unsigned char)command) {
            payload_unsupported("0x4ca0 repeat run exceeds the measured 0x400 scratch prefix");
            return 0;
        }
        {
            Eng1Byte value;
            Eng1Dword run;
            value = **stream;
            ++*stream;
            run = (Eng1Dword)(unsigned char)command;
            while (run != 0) {
                *write++ = value;
                --run;
            }
        }
    }

    for (rows = 0; rows < row_count; ++rows) {
        Eng1Dword word_index;
        for (word_index = 0; word_index < copy_words; ++word_index) {
            Eng1Word value;
            value = *(Eng1Word *)(read + word_index * 2UL);
            *(Eng1Word *)(*destination + word_index * 2UL) = value;
        }
        read += copy_words * 2UL;
        *destination += 32;
    }
    return 1;
}

Eng1Word __cdecl eng1_target_04ca0(const Eng1Byte *records,
                                      Eng1Dword count,
                                      Eng1Dword flags)
{
    const Eng1Byte *stream;
    Eng1Word first_key;
    Eng1Word *link_slot;
    Eng1Dword pending;
    Eng1Dword cursor;
    Eng1Dword record_index;

    stream = records + count * 4UL;
    first_key = 0xffffU;
    link_slot = 0;
    pending = g_frame_upload_pending_6ee7c;
    cursor = g_frame_upload_cursor_6ee78;
    if (pending > 32UL) {
        payload_unsupported("0x4ca0 pending count exceeds the measured coordinate prefix");
        return 0xffffU;
    }

    for (record_index = 0; record_index < count; ++record_index) {
        signed char code;
        int payload_class;
        int selected_slot;
        int found_existing;
        Eng1Word selected_mask;
        Eng1Dword coordinate_index;
        Eng1Dword sequence_index;
        Eng1Byte group;
        Eng1Word key;
        Eng1Dword row_count;
        Eng1Dword words_to_copy;
        Eng1Byte *destination;

        code = (signed char)records[record_index * 4UL];
        if (code < 0)
            continue;

        payload_class = ((code >> 1) & 1) + ((code >> 2) & 2);
        selected_mask = 0;
        selected_slot = 0;
        found_existing = 0;

        if (payload_class != 3) {
            for (coordinate_index = 0; coordinate_index < pending;
                 ++coordinate_index) {
                Eng1Word candidate_group;
                candidate_group = g_frame_upload_coordinates_734a0[coordinate_index];
                if (candidate_group >= 256U) {
                    payload_unsupported("0x4ca0 coordinate leaves the measured 256 link rows");
                    return 0xffffU;
                }
                if (choose_existing_mask(
                        g_actor_link_rows[candidate_group].live_mask,
                        payload_class, &selected_mask, &selected_slot)) {
                    group = (Eng1Byte)candidate_group;
                    found_existing = 1;
                    break;
                }
            }
        }

        if (!found_existing) {
            if (!find_empty_group(flags, cursor, &sequence_index)) {
                release_first_chain(first_key);
                g_frame_upload_cursor_6ee78 = cursor;
                g_frame_upload_pending_6ee7c = pending;
                return 0xffffU;
            }
            group = g_actor_group_sequence_5e138[sequence_index];
            selected_mask = payload_class == 3 ? 0x000fU :
                            payload_class == 2 ? 0x0005U :
                            payload_class == 1 ? 0x0003U : 0x0001U;
            selected_slot = 0;
            cursor = pending;
        }

        if (pending >= 32UL) {
            if (pending != 32UL) {
                payload_unsupported("0x4ca0 flush input exceeds the measured 32-coordinate prefix");
                return 0xffffU;
            }

            /* The real consumer drains the shared queue. Native resumes at
             * coordinate zero while retaining the first link key and chain;
             * both a full input queue and a two-record crossing are tested. */
            dispatch_engine_2c630(0);
            g_frame_upload_pending_6ee7c = pending;
            dispatch_local_050d0();
            dispatch_engine_2c630(0);
            pending = g_frame_upload_pending_6ee7c;
            if (pending != 0) {
                payload_unsupported("0x4ca0 flush consumer left a nonempty queue beyond the measured 32-WORD prefix");
                return 0xffffU;
            }
            cursor = 0;
        }

        cursor = pending;
        g_frame_upload_coordinates_734a0[pending] = group;
        ++pending;
        if (found_existing)
            g_actor_link_rows[group].live_mask |= selected_mask;
        else
            g_actor_link_rows[group].live_mask = selected_mask;

        key = (Eng1Word)(((Eng1Word)group << 4) | selected_mask);
        if (link_slot != 0)
            *link_slot = key;
        else
            first_key = key;

        if (g_actor_link_map[selected_mask] < 0 ||
            g_actor_link_map[selected_mask] >= 4) {
            payload_unsupported("0x4ca0 link-map result leaves four successor slots");
            return 0xffffU;
        }
        link_slot = &g_actor_link_rows[group].successor[
            g_actor_link_map[selected_mask]];
        *link_slot = 0xffffU;

        row_count = (Eng1Dword)((code & 0x0c) * 2 + 8);
        words_to_copy = (Eng1Dword)((code & 0x03) * 4 + 4);
        if (g_frame_owner_72870 == 0) {
            payload_unsupported("0x4ca0 frame-owner pointer is unavailable");
            return 0xffffU;
        }
        destination = (Eng1Byte *)g_frame_owner_72870 +
            (((pending - 1UL) * 4UL + ((Eng1Dword)selected_slot & 2UL)) * 256UL +
             ((Eng1Dword)selected_slot & 1UL) * 16UL);
        if (!decode_program(&stream, &destination, words_to_copy, row_count)) {
            g_frame_upload_cursor_6ee78 = cursor;
            g_frame_upload_pending_6ee7c = pending;
            return first_key;
        }
    }

    if (cursor >= 84UL)
        cursor = 0;
    g_frame_upload_cursor_6ee78 = cursor;
    g_frame_upload_pending_6ee7c = pending;
    return first_key;
}
