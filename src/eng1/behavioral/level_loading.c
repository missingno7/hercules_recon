#include "incomplete.h"
/*
 * Private behavioral source for six ENG1 startup/update callees.
 *
 * Address-backed names are local aliases, not historical source names.  The
 * PC calls and field accesses are described in
 * work/eng1_behavioral/level_loading/handoff.md.  No storage or callback
 * provider is defined here; unresolved externs remain explicit dependencies.
 */
#include "../../../calibration/engine_dispatch_externs.h"
#include "../../../calibration/resource_record.h"

extern EngineInterface g_engine_interface;
extern ActorResource g_actor_resources[81];

/* Existing providers in the ENG1 source graph. */
extern void *__cdecl engine_pool_allocate(void **, unsigned long,
                                          unsigned long);
extern void __cdecl dispatch_engine_2c2d0(void *);
extern void __cdecl dispatch_local_2c650(void);
extern void __cdecl dispatch_local_2c660(void);
extern void __cdecl resource_destroy(int);
extern int __cdecl resource_provider(int, int);

/* Resource/dispatch cells whose existing providers own their storage. */
extern unsigned char g_root_count_70ed2;
extern unsigned char g_header_byte_70ecd;
extern unsigned long g_frame_upload_pending_6ee7c;
extern unsigned long g_frame_upload_cursor_6ee78;
extern unsigned short g_configuration_word_713b6;
extern unsigned short g_configuration_word_713a0;
extern unsigned short g_configuration_word_713a8;
extern void *g_normal_actor_pool;
extern void *g_middle_actor_pool;
extern void *g_alternate_actor_pool;
extern u8 *g_dispatch_frame_current_71ec4;
extern u8 *g_dispatch_frame_base_71ec8;

/* Unprovided data cells observed by this family.  These are declarations only.
 * The comments bind each alias to its native ENG1 address for the integrator. */
extern void *g_resource_leaf_buffer_70c58;               /* 0x70c58, pointer cell */
extern unsigned long g_resource_scan_cursor_6ee74;       /* 0x6ee74 */
extern unsigned long g_resource_transition_ticks_6efb0;  /* 0x6efb0 */
extern unsigned long g_frame_owner_list_73490;           /* 0x73490 */
extern unsigned long g_frame_resource_list_734e4;        /* 0x734e4 */
extern unsigned long g_frame_actor_list_734f8;           /* 0x734f8 */
extern unsigned long g_frame_setup_gate_72868;           /* 0x72868 */
extern unsigned long g_frame_state_70c48;                /* 0x70c48 */
extern unsigned long g_frame_state_70c4c;                /* 0x70c4c */
extern unsigned short g_frame_setup_word_4d802;          /* 0x4d802 */
extern unsigned short g_frame_setup_word_4d806;          /* 0x4d806 */
extern void (__cdecl *g_frame_callbacks_4d6f8[])(void *); /* 0x4d6f8 */

/* Missing target callables, named only by their reviewed RVAs. */
extern int __cdecl eng1_target_02290(void);              /* 0x2290 */
extern void __cdecl eng1_target_02400(unsigned long);    /* 0x2400 */
extern void __cdecl eng1_target_02430(void);             /* 0x2430 */
extern void __cdecl eng1_target_03910(void *, void *);   /* 0x3910 */
extern void __cdecl eng1_target_039b0(void);             /* 0x39b0 */
extern void __cdecl eng1_target_033e0(void *, void *);   /* 0x33e0 */
extern unsigned short __cdecl eng1_target_04ca0(
    const unsigned char *, unsigned long,
    unsigned long);                                       /* 0x4ca0 */
extern unsigned long __cdecl eng1_target_05370(
    void *, unsigned short, unsigned long);               /* 0x5370 */
extern void __cdecl eng1_target_31330(void *);            /* 0x31330 */
extern void __cdecl eng1_target_027a0(void *);            /* 0x27a0 */
extern void __cdecl eng1_target_03cd0(void *);            /* 0x3cd0 */
extern int __cdecl eng1_target_320d0(void *);             /* 0x320d0 */
extern void __cdecl eng1_target_03bd0(void *);            /* 0x3bd0 */
extern unsigned long __cdecl eng1_target_02c290(int);    /* 0x2c290 */

void __cdecl dispatch_local_02180(void)
{
    ResourceCallbackContext *context;
    unsigned char flags;
    unsigned long ticks;

    context = g_engine_interface.context_004;
    if (context->unknown_000[4] == 0x80) {
        if (context->mode_09c != 0)
            return;
        eng1_target_02430();
        context = g_engine_interface.context_004;
    }

    if (context->transfer_status_0dc != 0) {
        eng1_target_02400(0x10UL);
        context = g_engine_interface.context_004;
        goto update_gate;
    }

    if (context->unknown_060 != 0) {
        if (context->unknown_000[5] == 0) {
            eng1_target_02400(8UL);
            context = g_engine_interface.context_004;
        }
        goto update_gate;
    }

    if (context->unknown_000[4] != 8) {
        flags = ((unsigned char *)context)[0x5c] & 9;
        ticks = g_resource_transition_ticks_6efb0;
        if (ticks == 0) {
            if (flags == 1)
                g_resource_transition_ticks_6efb0 = 1;
        } else if (flags == 9) {
            ++ticks;
            g_resource_transition_ticks_6efb0 = ticks;
            if ((long)ticks > 0x3cL) {
                if (context->unknown_000[6] == 3)
                    context->unknown_000[3] = 0x13;
                else
                    context->unknown_000[3] = 0x12;
                return;
            }
        } else if (flags == 1) {
            g_resource_transition_ticks_6efb0 = 1;
        } else {
            g_resource_transition_ticks_6efb0 = 0;
        }
    }

update_gate:
    if (context->unknown_000[4] != 0) {
        if (eng1_target_02290() == 0)
            return;
        context = g_engine_interface.context_004;
        if ((context->unknown_05e & 0x4008U) != 0) {
            eng1_target_02430();
            dispatch_engine_2c420();
            return;
        }
    }

    if ((context->unknown_05e & 8U) != 0 &&
        context->unknown_000[5] == 0 &&
        context->unknown_000[6] != 3)
        eng1_target_02400(0x40UL);
}

void __cdecl dispatch_local_02f30(void)
{
    unsigned char *buffer;
    signed char count;
    unsigned long bytes;
    long i;

    buffer = (unsigned char *)g_resource_leaf_buffer_70c58;
    if (buffer != 0)
        dispatch_engine_2c2d0(buffer);

    count = (signed char)g_root_count_70ed2;
    if (count != 0) {
        bytes = (unsigned long)((long)count * 2L);
        engine_pool_allocate(&g_resource_leaf_buffer_70c58,
                             bytes, 0UL);
        count = (signed char)g_root_count_70ed2;
    }

    i = 0;
    if (count <= 0)
        return;
    do {
        buffer = (unsigned char *)g_resource_leaf_buffer_70c58;
        buffer[i * 2] = 0;
        buffer = (unsigned char *)g_resource_leaf_buffer_70c58;
        buffer[i * 2 + 1] = 0;
        ++i;
        count = (signed char)g_root_count_70ed2;
    } while (i < (long)count);
}

void __cdecl dispatch_local_03270(void *descriptor_item,
                                 void *descriptor_child)
{
    unsigned char *record_cursor;
    unsigned char *record;
    unsigned char *references;
    unsigned char *metadata;
    unsigned char *payload;
    unsigned char *callback_data;
    unsigned char *reference_slot;
    unsigned long encoded;
    unsigned long callback_value;
    unsigned short entry_count;
    unsigned short metadata_count;
    unsigned short callback_result;
    signed char payload_kind;
    unsigned short resource_index;
    unsigned long reference_index;
    unsigned long metadata_index;

    g_frame_upload_pending_6ee7c = 0;
    g_frame_upload_cursor_6ee78 = 0;
    eng1_target_03910(descriptor_item, descriptor_child);
    eng1_target_02c290(0UL);
    eng1_target_039b0();
    eng1_target_033e0(descriptor_item, descriptor_child);

    if (g_header_byte_70ecd != 0)
        g_actor_resources[0].residency_state = 3;

    record_cursor = (unsigned char *)&g_actor_resources[1];
    for (resource_index = 1; resource_index <= 80; ++resource_index) {
        record = record_cursor;
        if (*(unsigned long *)(record + 12) != 0 &&
            *(unsigned short *)(record + 16) == 3) {
            entry_count = *(unsigned short *)(record + 4);
            references = (unsigned char *)
                *(unsigned long *)(record + 12);
            for (reference_index = 0;
                 reference_index <= entry_count;
                 ++reference_index) {
                *(unsigned short *)(references + reference_index * 4) = 0;
                *(unsigned short *)(references + reference_index * 4 + 2) = 0;
            }

            entry_count = *(unsigned short *)(record + 4);
            references = (unsigned char *)
                *(unsigned long *)(record + 12);
            metadata = references + (unsigned long)entry_count * 4UL + 8UL;
            metadata_count = *(unsigned short *)metadata;
            for (metadata_index = 1;
                 metadata_index <= *(unsigned short *)(record + 4);
                 ++metadata_index) {
                encoded = *(unsigned long *)(metadata +
                                             (unsigned long)metadata_index * 4UL);
                if ((encoded & 0x40UL) != 0) {
                    payload = metadata + (long)((long)encoded >> 8);
                    if (payload[0] != 0) {
                        payload_kind = (signed char)payload[1];
                        callback_data = (unsigned char *)
                            g_engine_interface.opaque_010[0];
                        callback_value = *(unsigned long *)(metadata + 4UL +
                                                   (unsigned long)metadata_count * 4UL);
                        *(unsigned long *)callback_data = callback_value;
                        callback_value = *(unsigned long *)(metadata + 8UL +
                                                   (unsigned long)metadata_count * 4UL);
                        *(unsigned long *)(callback_data + 4) = callback_value;
                        reference_slot = references + metadata_index * 4UL;
                        *(unsigned short *)reference_slot = 0x40;
                        callback_result = eng1_target_04ca0(
                            payload + 8 + (long)payload_kind * 6L,
                            payload[0], 0x40UL);
                        *(unsigned short *)(reference_slot + 2) = callback_result;
                    }
                }
            }
        }
        record_cursor += 20;
    }

    dispatch_local_2c650();
    dispatch_local_050d0();
    dispatch_engine_2c630(0UL);
    dispatch_local_2c660();
}

void __cdecl dispatch_local_03470(void)
{
    ResourceCallbackContext *context;
    long resource_index;
    long attempts;

    context = g_engine_interface.context_004;
    if (context->mode_09c != 0 || context->blocked_0df != 0)
        return;

    resource_index = (long)g_resource_scan_cursor_6ee74;
    for (attempts = 0; attempts < 80; ++attempts) {
        ++resource_index;
        if (resource_index >= 0x51)
            resource_index = 1;
        if (g_actor_resources[resource_index].residency_state == 1) {
            g_resource_scan_cursor_6ee74 = (unsigned long)resource_index;
            resource_provider((int)resource_index, 1);
            return;
        }
    }
    g_resource_scan_cursor_6ee74 = (unsigned long)resource_index;
}

void __cdecl dispatch_local_034d0(void)
{
    int resource_index;
    unsigned short old_count;

    for (resource_index = 1; resource_index <= 80; ++resource_index) {
        old_count = g_actor_resources[resource_index].entry_count;
        if (old_count == 4 || old_count == 7) {
            resource_destroy(resource_index);
            if (old_count == 7)
                g_actor_resources[resource_index].entry_count = 6;
        }
    }
}

void __cdecl dispatch_local_03a50(void)
{
    unsigned long result;
    unsigned char *node;
    unsigned long flags;
    unsigned long callback_index;

    g_frame_state_70c48 = 0;
    g_frame_state_70c4c = 0;
    g_frame_setup_gate_72868 = 1;
    g_frame_resource_list_734e4 = 0xffffffffUL;
    g_frame_owner_list_73490 = 0xffffffffUL;
    g_frame_actor_list_734f8 = 0xffffffffUL;

    result = eng1_target_05370(g_alternate_actor_pool,
                               g_configuration_word_713b6, 0x94UL);
    result = eng1_target_05370(g_middle_actor_pool,
                               g_configuration_word_713a0, 0xecUL);
    result = eng1_target_05370(g_normal_actor_pool,
                               g_configuration_word_713a8, 0x134UL);

    if (g_dispatch_frame_current_71ec4 != g_dispatch_frame_base_71ec8)
        g_frame_setup_word_4d802 = 0x100;
    else
        g_frame_setup_word_4d802 = 0;
    g_frame_setup_word_4d806 = 0xf0;

    g_frame_upload_pending_6ee7c = 0;
    g_frame_upload_cursor_6ee78 = 0;
    eng1_target_31330((void *)&g_frame_owner_list_73490);

    node = (unsigned char *)(unsigned long)g_frame_actor_list_734f8;
    while ((unsigned long)node != 0xffffffffUL) {
        if (g_engine_interface.context_004->unknown_040 == 0)
            eng1_target_027a0(node);
        node = (unsigned char *)*(unsigned long *)(node + 0x68);
    }

    node = (unsigned char *)(unsigned long)g_frame_resource_list_734e4;
    while ((unsigned long)node != 0xffffffffUL) {
        flags = *(unsigned long *)(node + 0x54);
        if ((flags & 0x40000000UL) != 0) {
            *(unsigned short *)(node + 0x0c) =
                (unsigned short)((long)*(unsigned long *)node >> 16);
            *(unsigned short *)(node + 0x0e) =
                (unsigned short)((long)*(unsigned long *)(node + 4) >> 16);
            *(unsigned short *)(node + 0x10) = 4;
            eng1_target_03cd0(node);
        } else if ((flags & 0x20000000UL) != 0) {
            callback_index = node[0x23];
            if (callback_index >= 10)
                eng1_incomplete_behavior(0x3a50UL, "callback4d6f8 index outside proven prefix");
            g_frame_callbacks_4d6f8[callback_index](node);
        } else if (eng1_target_320d0(node) == -1) {
            node[0x1f] |= 8;
        } else {
            eng1_target_03bd0(node);
        }
        node = (unsigned char *)*(unsigned long *)(node + 0x68);
    }

    (void)result;
}
