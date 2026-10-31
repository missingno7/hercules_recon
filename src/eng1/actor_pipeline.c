#include "worklist_factory_types.h"
/*
 * Private ordinary-C candidate for seven ENG1 actor-pool orchestration roots.
 * Function names match the behavioral graph bindings; descriptive labels do
 * not claim original source names or translation-unit ownership.
 *
 * The pool views use only measured actor strides and fields touched here.
 * Work-list capacities and the complete g_kind_info table extent remain
 * external and unproved. Native bounds, callback order, and dynamic reloads
 * are retained, including the unbounded profile index in 0x262a0.
 */
#include <stddef.h>
#include "../shared/engine_interface.h"
#include "../shared/resource_record.h"
#include "resource_runtime_data.h"
#include "data_layouts.h"

typedef struct ActorPipelineView ActorPipelineView;
struct ActorPipelineView {
    long x_000;
    long y_004;
    long z_008;
    unsigned char reserved_00c_021[0x16];
    unsigned char category_022;
    unsigned char reserved_023_02d[0x0b];
    unsigned short kind_02e;
    unsigned char reserved_030_037[8];
    unsigned short field_038;
    unsigned char reserved_03a_04f[0x16];
    unsigned long flags_050;
    unsigned char reserved_054_0eb[0x98];
    unsigned char lifetime_0ec;
    unsigned char lifetime_0ed;
};

typedef void (__cdecl *ActorPipelineCallback)(ActorPipelineView *);
typedef struct ActorPipelineCallback8 ActorPipelineCallback8;
struct ActorPipelineCallback8 {
    ActorPipelineCallback slot_000;
    ActorPipelineCallback slot_004;
};
typedef struct ActorPipelineCallback12 ActorPipelineCallback12;
struct ActorPipelineCallback12 {
    ActorPipelineCallback slot_000;
    ActorPipelineCallback slot_004;
    ActorPipelineCallback slot_008;
};
extern void __cdecl dispatch_local_262a0(void);

typedef char actor_pipeline_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char actor_pipeline_kind_is_at_2e[
    (offsetof(ActorPipelineView, kind_02e) == 0x2e) ? 1 : -1];
typedef char actor_pipeline_field38_is_at_38[
    (offsetof(ActorPipelineView, field_038) == 0x38) ? 1 : -1];
typedef char actor_pipeline_flags_is_at_50[
    (offsetof(ActorPipelineView, flags_050) == 0x50) ? 1 : -1];
typedef char actor_pipeline_lifetime_is_at_ec[
    (offsetof(ActorPipelineView, lifetime_0ec) == 0xec) ? 1 : -1];

/* Actor pool pointer owners and word configuration cells are supplied by the
 * existing pool/configuration providers. The declarations below are views. */
extern void *g_normal_actor_pool;
extern void *g_middle_actor_pool;
extern void *g_alternate_actor_pool;
extern unsigned short g_configuration_word_71378;
extern unsigned short g_configuration_word_713a8;
extern unsigned short g_configuration_word_713aa;
extern unsigned short g_configuration_word_713a0;
extern unsigned short g_configuration_word_7138c;
extern unsigned short g_configuration_word_713b6;
extern unsigned short g_configuration_word_713c4;
extern unsigned short g_configuration_word_713b8;
extern unsigned short g_configuration_word_71430;
extern unsigned short g_configuration_word_713b4;
extern unsigned short g_configuration_word_713ba;
extern unsigned short g_configuration_word_71432;

/* The callback cells are pointer-valued views over the measured DWORD cells. */
extern ActorPipelineCallback12 *g_configuration_dword_71370;
extern ActorPipelineCallback8 *g_configuration_dword_7137c;
extern ActorPipelineCallback8 *g_configuration_dword_7139c;

/* These native count-and-pointer lanes are 1-based: [0] is the current count,
 * and the first appended actor pointer is stored in [1]. Their capacities
 * have not been established, so this file supplies no storage or bounds. */
extern unsigned long g_actor_worklist_714c0[];
extern unsigned long g_actor_worklist_71580[];
extern unsigned long g_actor_worklist_716c0[];
extern unsigned long g_actor_worklist_71760[];
extern unsigned long g_actor_worklist_717c0[];
extern unsigned long g_actor_worklist_71900[];
extern unsigned long g_actor_worklist_71a20[];
extern unsigned long g_actor_worklist_71b20[];
extern unsigned long g_actor_worklist_71be0[];
extern unsigned long g_actor_worklist_71ce0[];

/* Other shared cells are accessed at their observed widths. The 81-byte
 * category-state snapshot is written by 0x258e0; it is not a resource table. */
extern unsigned long g_pipeline_dword_71844;
extern unsigned long g_pipeline_dword_71840;
extern unsigned long g_pipeline_dword_71a18;
extern unsigned short g_pipeline_word_71376;
extern unsigned short g_pipeline_word_713ae;
extern unsigned short g_pipeline_word_71bc4;
extern unsigned short g_pipeline_word_71c5c;
extern unsigned short g_pipeline_word_71ca0;
extern unsigned short g_pipeline_word_71740;
extern unsigned short g_pipeline_word_71720;
extern unsigned short g_pipeline_word_71cc0;
extern unsigned short g_pipeline_word_71c60;
extern unsigned short g_pipeline_word_6fec0;
extern unsigned short g_pipeline_word_6fec4;
extern unsigned long g_pipeline_dword_71680;
extern unsigned long g_pipeline_dword_716a8;
extern unsigned long g_pipeline_dword_71718;
extern unsigned long g_pipeline_dword_71460;
extern ActorResource g_actor_resources[81];
extern unsigned char g_resource_state_snapshot_719c0[81];
extern unsigned long g_actor_worklist_71460[];
extern short g_actor_stage_table_5a848[];
extern unsigned char g_pipeline_actor_slots_71440[];
extern ActorPipelineView *g_pipeline_actor_7200c;
extern unsigned char *g_collision_context;
extern unsigned long g_frame_global_734f4;
extern unsigned long g_pipeline_view_origin_70ca0;
extern unsigned long g_pipeline_view_origin_70cdc;

/* Existing actor helpers are direct native dependencies: remove_actor=27260,
 * remove_actor_variant=27860, release_actor=27e20; reset pools=27490/279a0/
 * 27f50. The remaining address-labelled calls have no current provider name. */
extern int __cdecl remove_actor(ActorPipelineView *);
extern int __cdecl remove_actor_variant(ActorPipelineView *);
extern int __cdecl release_actor(ActorPipelineView *);
extern void __cdecl reset_normal_actor_pool(void);
extern void __cdecl reset_middle_actor_pool(void);
extern void __cdecl reset_alternate_actor_pool(void);
extern void __cdecl eng1_call_274e0(void);
extern void __cdecl eng1_call_1fdf0(void);
extern void __cdecl reset_command_table(void);
extern void __cdecl initialize_pending_action_groups(void);
extern void __cdecl eng1_call_279f0(void);
extern void __cdecl eng1_call_27fa0(void);
extern void __cdecl eng1_call_01e50(void);
extern void __cdecl eng1_call_12960(void);
extern void __cdecl eng1_call_26040(void);
extern void __cdecl dispatch_local_1fd20(void);
extern void __cdecl eng1_call_26480(void);
extern void __cdecl eng1_call_26730(void);
extern unsigned short __cdecl eng1_call_2a880(
    ActorPipelineView *, void *, unsigned long);

/* ENG1 RVA 0x255f0. This is cold actor-pool initialization, not a generic
 * engine entry: five setup calls lead to the native tail target 0x27fa0. */
#include "actor_pipeline_storage_views.h"

void __cdecl dispatch_local_255f0(void)
{
    g_pipeline_dword_71844 = 0;
    g_pipeline_word_713ae = 0;
    eng1_call_274e0();
    eng1_call_1fdf0();
    reset_command_table();
    initialize_pending_action_groups();
    eng1_call_279f0();
    eng1_call_27fa0();
}

/* ENG1 RVA 0x25620. Clear the observed per-frame actor/work-list cells, invoke
 * the existing setup dependencies, then seed the 0x71376 word with five. */
void __cdecl dispatch_local_25620(void)
{
    g_pipeline_word_71ca0 = 0;
    g_pipeline_word_71740 = 0;
    g_pipeline_word_71720 = 0;
    g_pipeline_word_71cc0 = 0;
    g_pipeline_dword_71680 = 0;
    g_pipeline_word_71c60 = 0;
    g_actor_worklist_714c0[0] = 0;
    g_actor_worklist_71760[0] = 0;
    g_actor_worklist_717c0[0] = 0;
    g_actor_worklist_716c0[0] = 0;
    g_actor_worklist_71580[0] = 0;
    g_actor_worklist_71a20[0] = 0;
    g_actor_worklist_71be0[0] = 0;
    g_actor_worklist_71b20[0] = 0;
    g_pipeline_dword_71a18 = 0;
    eng1_call_01e50();
    eng1_call_12960();
    eng1_call_26040();
    dispatch_local_1fd20();
    dispatch_local_262a0();
    g_pipeline_word_71376 = 5;
}

/* ENG1 RVA 0x256a0. Pool removal helpers run normal, middle, alternate; only
 * then are the alternate scan cursors restored from their measured starts. */
void __cdecl engine_shutdown(void)
{
    reset_normal_actor_pool();
    reset_middle_actor_pool();
    reset_alternate_actor_pool();
    g_configuration_word_713c4 = g_configuration_word_71378;
    g_configuration_word_713b8 = g_configuration_word_713aa;
    g_configuration_word_71430 = g_configuration_word_7138c;
}

/* ENG1 RVA 0x256e0. Visit alternate, middle, normal scan windows. Capture
 * each actor's kind and flags once before its callback pair, as native code
 * reuses those values even if a callback mutates the actor. */
void __cdecl dispatch_local_256e0(void)
{
    unsigned int index;
    unsigned short kind;
    unsigned short flags;
    ActorPipelineView *actor;

    index = g_configuration_word_7138c;
    while (index < g_configuration_word_713b6) {
        actor = (ActorPipelineView *)((unsigned char *)g_alternate_actor_pool +
                                      index * 0x94UL);
        kind = actor->kind_02e;
        if (kind != 0) {
            flags = *(unsigned short *)((unsigned char *)actor + 0x50);
            if (flags & 4UL) {
                if (flags & 1UL)
                    g_configuration_dword_7139c[kind & 0x0fffU].slot_004(actor);
                if (flags & 2UL)
                    g_configuration_dword_7139c[kind & 0x0fffU].slot_004(actor);
            }
        }
        ++index;
    }

    index = g_configuration_word_713aa;
    while (index < g_configuration_word_713a0) {
        actor = (ActorPipelineView *)((unsigned char *)g_middle_actor_pool +
                                      index * 0xecUL);
        kind = actor->kind_02e;
        if (kind != 0) {
            flags = *(unsigned short *)((unsigned char *)actor + 0x50);
            if (flags & 4UL) {
                if (!(flags & 0x800UL)) {
                    *(unsigned long *)((unsigned char *)actor + 0x9c) =
                        (unsigned long)actor->x_000;
                    *(unsigned long *)((unsigned char *)actor + 0xa0) =
                        (unsigned long)actor->y_004;
                }
                if (flags & 1UL)
                    g_configuration_dword_7137c[kind & 0x0fffU].slot_004(actor);
                if (flags & 2UL)
                    g_configuration_dword_7137c[kind & 0x0fffU].slot_004(actor);
            }
        }
        ++index;
    }

    index = g_configuration_word_71378;
    while (index < g_configuration_word_713a8) {
        actor = (ActorPipelineView *)((unsigned char *)g_normal_actor_pool +
                                      index * 0x134UL);
        kind = actor->kind_02e;
        if (kind != 0) {
            flags = *(unsigned short *)((unsigned char *)actor + 0x50);
            if (flags & 4UL) {
                if (!(flags & 0x800UL)) {
                    *(unsigned long *)((unsigned char *)actor + 0x9c) =
                        (unsigned long)actor->x_000;
                    *(unsigned long *)((unsigned char *)actor + 0xa0) =
                        (unsigned long)actor->y_004;
                    *(unsigned long *)((unsigned char *)actor + 0xa4) =
                        (unsigned long)actor->z_008;
                }
                if (flags & 1UL)
                    g_configuration_dword_71370[kind].slot_004(actor);
                if (flags & 2UL)
                    g_configuration_dword_71370[kind].slot_008(actor);
            }
        }
        ++index;
    }
}

/* ENG1 RVA 0x258e0. Build the 81-byte resource-state snapshot, classify each
 * live actor by its observed category byte, and preserve the native list and
 * callback order. These lists are external count+pointer lanes; no capacity
 * or generalized safety claim is made for them here. */
void __cdecl dispatch_local_258e0(void)
{
    unsigned int index;
    unsigned int resource_index;
    unsigned short kind;
    unsigned long flags;
    unsigned long tick;
    int table_index;
    int table_value;
    ActorPipelineView *actor;
    ResourceCallbackContext *context;

    g_pipeline_dword_71840 = 0;
    actor = (ActorPipelineView *)g_normal_actor_pool;
    g_pipeline_word_71bc4 = (unsigned short)(actor->z_008 >> 16);
    g_pipeline_word_71c5c = (unsigned short)(actor->z_008 >> 16);

    context = g_engine_interface.context_004;
    tick = context->tick_038;
    /* Native25919 uses a byte displacement (tick&7f)<<4, while the
     * later25956 access uses a SHORT index. Convert only this first access. */
    table_index = (int)((tick & 0x7fUL) << 3);
    table_value = (int)g_actor_stage_table_5a848[table_index];
    g_pipeline_dword_716a8 = (unsigned long)((table_value * 64) >> 16);
    tick = context->tick_038;
    table_index = (int)(((tick + 0x10UL) << 4) & 0x3ffUL);
    table_value = (int)g_actor_stage_table_5a848[table_index];
    g_pipeline_dword_71718 = (unsigned long)((table_value * 80) >> 16);
    /* Native25932..25964 clear these lanes before26480 can observe them. */
    g_pipeline_word_71ca0 = 0;
    g_pipeline_word_71740 = 0;
    g_pipeline_word_71720 = 0;
    g_pipeline_word_71cc0 = 0;
    g_pipeline_word_71c60 = 0;
    g_pipeline_dword_71680 = 0;
    eng1_call_26480();

    g_actor_worklist_714c0[0] = 0;
    g_actor_worklist_71760[0] = 0;
    g_actor_worklist_717c0[0] = 0;
    g_actor_worklist_716c0[0] = 0;
    g_actor_worklist_71580[0] = 0;
    g_actor_worklist_71a20[0] = 0;
    g_actor_worklist_71be0[0] = 0;
    g_actor_worklist_71b20[0] = 0;

    for (resource_index = 0; resource_index < 81; ++resource_index) {
        g_resource_state_snapshot_719c0[resource_index] =
            *((unsigned char *)&g_actor_resources[resource_index].residency_state);
    }

    index = g_configuration_word_7138c;
    while (index < g_configuration_word_713b6) {
        actor = (ActorPipelineView *)((unsigned char *)g_alternate_actor_pool +
                                      index * 0x94UL);
        kind = actor->kind_02e;
        if (kind != 0) {
            if (g_resource_state_snapshot_719c0[actor->category_022] == 4) {
                release_actor(actor);
            } else {
                flags = actor->flags_050;
                if (flags & 0x40UL) {
                    ++g_actor_worklist_71a20[0];
                    g_actor_worklist_71a20[g_actor_worklist_71a20[0]] =
                        (unsigned long)actor;
                }
                if (flags & 0x20000UL) {
                    ++g_actor_worklist_71580[0];
                    g_actor_worklist_71580[g_actor_worklist_71580[0]] =
                        (unsigned long)actor;
                }
                if (flags & 1UL)
                    g_configuration_dword_7139c[kind & 0x01ffU].slot_004(actor);
            }
        }
        ++index;
    }

    index = g_configuration_word_713aa;
    while (index < g_configuration_word_713a0) {
        actor = (ActorPipelineView *)((unsigned char *)g_middle_actor_pool +
                                      index * 0xecUL);
        kind = actor->kind_02e;
        if (kind != 0) {
            if (g_resource_state_snapshot_719c0[actor->category_022] == 4) {
                remove_actor_variant(actor);
            } else {
                flags = actor->flags_050;
                if (!(flags & 0x800UL)) {
                    *(unsigned long *)((unsigned char *)actor + 0x9c) =
                        (unsigned long)actor->x_000;
                    *(unsigned long *)((unsigned char *)actor + 0xa0) =
                        (unsigned long)actor->y_004;
                }
                if (flags & 0x40000UL) {
                    ++g_actor_worklist_714c0[0];
                    g_actor_worklist_714c0[g_actor_worklist_714c0[0]] =
                        (unsigned long)actor;
                }
                if (flags & 0x100000UL) {
                    ++g_actor_worklist_71b20[0];
                    g_actor_worklist_71b20[g_actor_worklist_71b20[0]] =
                        (unsigned long)actor;
                }
                if (flags & 0x20000UL) {
                    ++g_actor_worklist_71580[0];
                    g_actor_worklist_71580[g_actor_worklist_71580[0]] =
                        (unsigned long)actor;
                }
                if (flags & 0x800000UL) {
                    ++g_actor_worklist_71be0[0];
                    g_actor_worklist_71be0[g_actor_worklist_71be0[0]] =
                        (unsigned long)actor;
                }
                if (flags & 1UL)
                    g_configuration_dword_7137c[kind & 0x01ffU].slot_004(actor);
            }
        }
        ++index;
    }

    index = g_configuration_word_71378;
    while (index < g_configuration_word_713a8) {
        actor = (ActorPipelineView *)((unsigned char *)g_normal_actor_pool +
                                      index * 0x134UL);
        kind = actor->kind_02e;
        if (kind != 0) {
            if (g_resource_state_snapshot_719c0[actor->category_022] == 4) {
                remove_actor(actor);
            } else {
                flags = actor->flags_050;
                if (!(flags & 0x800UL)) {
                    *(unsigned long *)((unsigned char *)actor + 0x9c) =
                        (unsigned long)actor->x_000;
                    *(unsigned long *)((unsigned char *)actor + 0xa0) =
                        (unsigned long)actor->y_004;
                    *(unsigned long *)((unsigned char *)actor + 0xa4) =
                        (unsigned long)actor->z_008;
                }
                if (flags & 0x100000UL) {
                    ++g_actor_worklist_71b20[0];
                    g_actor_worklist_71b20[g_actor_worklist_71b20[0]] =
                        (unsigned long)actor;
                }
                if (flags & 0x80UL) {
                    ++g_actor_worklist_716c0[0];
                    g_actor_worklist_716c0[g_actor_worklist_716c0[0]] =
                        (unsigned long)actor;
                }
                if (flags & 0x20000UL) {
                    ++g_actor_worklist_71580[0];
                    g_actor_worklist_71580[g_actor_worklist_71580[0]] =
                        (unsigned long)actor;
                }
                if (flags & 0x400000UL) {
                    ++g_actor_worklist_717c0[0];
                    g_actor_worklist_717c0[g_actor_worklist_717c0[0]] =
                        (unsigned long)actor;
                }
                if (flags & 0x800000UL) {
                    ++g_actor_worklist_71be0[0];
                    g_actor_worklist_71be0[g_actor_worklist_71be0[0]] =
                        (unsigned long)actor;
                }
                if (!(flags & 0x10UL)) {
                    if (actor->lifetime_0ec != 0)
                        --actor->lifetime_0ec;
                    else
                        actor->lifetime_0ed = 0;
                } else {
                    ++g_actor_worklist_71760[0];
                    g_actor_worklist_71760[g_actor_worklist_71760[0]] =
                        (unsigned long)actor;
                }
                if (flags & 0x240000UL) {
                    ++g_actor_worklist_714c0[0];
                    g_actor_worklist_714c0[g_actor_worklist_714c0[0]] =
                        (unsigned long)actor;
                }
                if (flags & 1UL)
                    g_configuration_dword_71370[kind & 0xffffU].slot_004(actor);
            }
        }
        ++index;
    }
}

/* ENG1 RVA 0x25d00. Capture the current actor independently for each helper
 * call, then gather actor callbacks and deferred alternates before the active
 * actor callback, mode-dependent update, and address-wrap pass. */
void __cdecl dispatch_local_25d00(void)
{
    unsigned int index;
    unsigned short kind;
    unsigned long flags;
    unsigned long count;
    unsigned char low_flags;
    long level;
    unsigned char mode;
    unsigned char *mode_row;
    ActorPipelineView *actor;
    ResourceCallbackContext *context;

    g_actor_worklist_71460[0] = 0;
    g_actor_worklist_71ce0[0] = 0;
    g_actor_worklist_71900[0] = 0;
    actor = (ActorPipelineView *)g_collision_context;
    eng1_call_2a880(actor, (unsigned char *)actor + 0x88, 1);
    actor = (ActorPipelineView *)g_collision_context;
    g_pipeline_word_6fec4 = eng1_call_2a880(
        actor, g_pipeline_actor_slots_71440, 2);
    g_pipeline_word_6fec0 = 0;
    if (g_pipeline_word_71376 != 0)
        --g_pipeline_word_71376;

    index = g_configuration_word_713aa;
    count = g_configuration_word_713a0;
    while (index < count) {
        actor = (ActorPipelineView *)((unsigned char *)g_middle_actor_pool +
                                      index * 0xecUL);
        kind = actor->kind_02e & 0x3fffU;
        if (kind != 0 && kind < 10) {
            ++g_actor_worklist_71900[0];
            g_actor_worklist_71900[g_actor_worklist_71900[0]] =
                (unsigned long)actor;
        }
        ++index;
    }

    index = g_configuration_word_71378;
    while (index < g_configuration_word_713a8) {
        actor = (ActorPipelineView *)((unsigned char *)g_normal_actor_pool +
                                      index * 0x134UL);
        kind = actor->kind_02e;
        if (kind != 0) {
            low_flags = *((unsigned char *)actor + 0x50);
            if (low_flags & 2U)
                g_configuration_dword_71370[kind].slot_008(actor);
        }
        ++index;
    }

    index = g_configuration_word_713aa;
    while (index < g_configuration_word_713a0) {
        actor = (ActorPipelineView *)((unsigned char *)g_middle_actor_pool +
                                      index * 0xecUL);
        kind = actor->kind_02e;
        if (kind != 0) {
            low_flags = *((unsigned char *)actor + 0x50);
            if (low_flags & 2U)
                g_configuration_dword_7137c[kind & 0x01ffU].slot_004(actor);
        }
        ++index;
    }

    index = g_configuration_word_7138c;
    while (index < g_configuration_word_713b6) {
        actor = (ActorPipelineView *)((unsigned char *)g_alternate_actor_pool +
                                      index * 0x94UL);
        kind = actor->kind_02e;
        if (kind != 0) {
            flags = actor->flags_050;
            if (flags & 0x10000UL) {
                ++g_actor_worklist_71ce0[0];
                g_actor_worklist_71ce0[g_actor_worklist_71ce0[0]] =
                    (unsigned long)actor;
            }
            low_flags = *((unsigned char *)actor + 0x50);
            if (low_flags & 2U)
                g_configuration_dword_7139c[kind & 0x01ffU].slot_004(actor);
        }
        ++index;
    }

    index = 0;
    while (index < g_actor_worklist_71ce0[0]) {
        actor = (ActorPipelineView *)g_actor_worklist_71ce0[index + 1];
        kind = actor->kind_02e;
        g_configuration_dword_7139c[kind & 0x01ffU].slot_004(actor);
        ++index;
    }

    actor = g_pipeline_actor_7200c;
    kind = actor->kind_02e;
    if (kind != 0)
        g_configuration_dword_71370[kind].slot_008(actor);

    level = (signed char)g_engine_interface.context_004->unknown_000[0];
    if (level != 9)
        eng1_call_26730();

    /* Native25f60 clears the consumed color list even when level9 skips26730. */
    g_actor_pipeline_storage_71440.dword_cells[0x6a0 / 4] = 0;

    context = g_engine_interface.context_004;
    level = (signed char)context->unknown_000[0];
    mode_row = (unsigned char *)eng1_profile_rows_5df08;
    mode_row += level * 28 + 3;
    mode = *mode_row;
    if (mode != 0) {
        actor = (ActorPipelineView *)g_normal_actor_pool;
        for (index = 0; index < g_configuration_word_713b4; ++index) {
            actor->x_000 = (long)((((unsigned long)actor->x_000 - 0x400000UL) &
                                    0x0fffffffUL) + 0x400000UL);
            actor = (ActorPipelineView *)((unsigned char *)actor + 0x134);
        }
        actor = (ActorPipelineView *)g_middle_actor_pool;
        for (index = 0; index < g_configuration_word_71432; ++index) {
            actor->x_000 = (long)((((unsigned long)actor->x_000 - 0x400000UL) &
                                    0x0fffffffUL) + 0x400000UL);
            actor = (ActorPipelineView *)((unsigned char *)actor + 0xec);
        }
        actor = (ActorPipelineView *)g_alternate_actor_pool;
        for (index = 0; index < g_configuration_word_713ba; ++index) {
            actor->x_000 = (long)((((unsigned long)actor->x_000 - 0x400000UL) &
                                    0x0fffffffUL) + 0x400000UL);
            actor = (ActorPipelineView *)((unsigned char *)actor + 0x94);
        }
    }
}

/* ENG1 RVA 0x262a0. Two passes over the selected kind's 0x14-byte records:
 * submit records with the measured 0xc800 filter, then submit only records
 * that also intersect the native horizontal view window. */
void __cdecl dispatch_local_262a0(void)
{
    unsigned short actor_kind;
    unsigned short record_flags;
    unsigned short record_type;
    unsigned int index;
    unsigned int count;
    long camera_delta;
    long x;
    unsigned char *records;
    unsigned char *record;
    ActorPipelineView *current_actor;

    current_actor = (ActorPipelineView *)g_collision_context;
    actor_kind = current_actor->field_038;
    camera_delta = (long)(g_pipeline_view_origin_70ca0 -
                          g_pipeline_view_origin_70cdc);
    if (g_kind_info[actor_kind].owner == 0)
        return;

    records = (unsigned char *)g_kind_info[actor_kind].owner + 0x14;
    for (index = 0; (long)index < (long)g_kind_info[actor_kind].count; ++index) {
        record = records + index * 0x14UL;
        if (g_pipeline_actor_7200c ==
            (ActorPipelineView *)g_frame_global_734f4 &&
            (record[0x0d] & 4U))
            continue;
        record_flags = *(unsigned short *)(record + 0x0c);
        if ((record_flags & 0xc800U) != 0x0800U)
            continue;
        record_type = *(unsigned short *)(record + 0x10);
        record_type &= 0x7000U;
        if (record_type == 0)
            eng1_call_270c0(record, 0);
        else if (record_type == 0x4000U)
            eng1_call_27720(record, 0);
        else if (record_type == 0x2000U)
            eng1_call_27c40(record, 0);
    }

    /* Native reloads both row pointer and count after first-pass callbacks. */
    records = (unsigned char *)g_kind_info[actor_kind].owner + 0x14;
    for (index = 0; (long)index < (long)g_kind_info[actor_kind].count; ++index) {
        record = records + index * 0x14UL;
        if (g_pipeline_actor_7200c ==
            (ActorPipelineView *)g_frame_global_734f4 &&
            (record[0x0d] & 4U))
            continue;

        x = (*(long *)record) >> 16;
        x = (long)((unsigned long)x - (unsigned long)camera_delta);
        record_flags = *(unsigned short *)(record + 0x0c);
        if (record_flags & 0x0040U) {
            if (x < -0x1f4L || x > 0x140L + 0x1f4L)
                continue;
        } else {
            if (x < -0x182L || x > 0x140L + 0x17cL)
                continue;
        }
        if (record_flags & 0xc000U)
            continue;
        record_type = *(unsigned short *)(record + 0x10);
        record_type &= 0x7000U;
        if (record_type == 0)
            eng1_call_270c0(record, 0);
        else if (record_type == 0x4000U)
            eng1_call_27720(record, 0);
        else if (record_type == 0x2000U)
            eng1_call_27c40(record, 0);
    }
}
