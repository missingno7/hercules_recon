#include "spatial_read_guard.h"
/*
 * Behavioral candidate for callback RVAs referenced by the measured ENG1
 * configuration/row-0 pointer tables.  The exact native starts and callback
 * argument flow are recorded in work/eng1_behavioral/default_callbacks/.
 * Calls whose bodies are not recovered remain external RVA-named dependencies.
 */
#include <stddef.h>
#include "../shared/engine_interface.h"

typedef unsigned char  eng1_u8;
typedef unsigned short eng1_u16;
typedef unsigned long  eng1_u32;

typedef struct Eng1CallbackActor Eng1CallbackActor;
struct Eng1CallbackActor {
    eng1_u32 x_000;
    eng1_u32 y_004;
    eng1_u32 z_008;
    eng1_u8 pad_00c[0x1c - 0x0c];
    eng1_u8 byte_01c;
    eng1_u8 pad_01d[0x25 - 0x1d];
    eng1_u8 byte_025;
    eng1_u8 pad_026[0x2a - 0x26];
    eng1_u16 state_02a;
    eng1_u8 pad_02c[0x2e - 0x2c];
    eng1_u16 kind_02e;
    eng1_u8 pad_030[0x38 - 0x30];
    eng1_u16 word_038;
    eng1_u8 pad_03a[0x4a - 0x3a];
    eng1_u16 word_04a;
    eng1_u32 timer_04c;
    eng1_u32 flags_050;
    eng1_u32 render_flags_054;
    eng1_u8 pad_058[0x94 - 0x58];
    eng1_u8 byte_094;
    eng1_u8 pad_095[0x97 - 0x95];
    eng1_u8 tag_097;
    eng1_u8 pad_098[0x9c - 0x98];
    eng1_u32 origin_x_09c;
    eng1_u32 origin_y_0a0;
    eng1_u8 pad_0a4[0xec - 0xa4];
    eng1_u8 byte_0ec;
    eng1_u8 pad_0ed;
    eng1_u16 progress_0ee;
    eng1_u8 pad_0f0[0xf4 - 0xf0];
    eng1_u32 dword_0f4;
    eng1_u8 pad_0f8[0x114 - 0xf8];
    eng1_u16 word_114;
    eng1_u8 pad_116[0x124 - 0x116];
    eng1_u16 word_124;
    eng1_u8 pad_126[2];
};

typedef char eng1_callback_actor_size_is_128[
    (sizeof(Eng1CallbackActor) == 0x128) ? 1 : -1];
typedef char eng1_callback_actor_offsets_are_measured[
    (offsetof(Eng1CallbackActor, byte_01c) == 0x1c &&
     offsetof(Eng1CallbackActor, byte_025) == 0x25 &&
     offsetof(Eng1CallbackActor, state_02a) == 0x2a &&
     offsetof(Eng1CallbackActor, kind_02e) == 0x2e &&
     offsetof(Eng1CallbackActor, word_038) == 0x38 &&
     offsetof(Eng1CallbackActor, word_04a) == 0x4a &&
     offsetof(Eng1CallbackActor, timer_04c) == 0x4c &&
     offsetof(Eng1CallbackActor, flags_050) == 0x50 &&
     offsetof(Eng1CallbackActor, render_flags_054) == 0x54 &&
     offsetof(Eng1CallbackActor, byte_094) == 0x94 &&
     offsetof(Eng1CallbackActor, tag_097) == 0x97 &&
     offsetof(Eng1CallbackActor, origin_x_09c) == 0x9c &&
     offsetof(Eng1CallbackActor, origin_y_0a0) == 0xa0 &&
     offsetof(Eng1CallbackActor, byte_0ec) == 0xec &&
     offsetof(Eng1CallbackActor, progress_0ee) == 0xee &&
     offsetof(Eng1CallbackActor, dword_0f4) == 0xf4 &&
     offsetof(Eng1CallbackActor, word_114) == 0x114 &&
     offsetof(Eng1CallbackActor, word_124) == 0x124) ? 1 : -1];

/* Only the observed prefix/count and indexed access widths are established. */
extern eng1_u32 g_actor_worklist_716c0[];
extern eng1_u32 g_configuration_dword_713bc;
extern eng1_u16 g_native_word_72168;
extern eng1_u8 g_actor_state_map_004d6ec[8];
extern unsigned char *g_native_pointer_7217c;

typedef struct Eng1Bounds3D Eng1Bounds3D;
struct Eng1Bounds3D {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
};

/* Unrecovered nested code stays a real link dependency, identified by RVA. */
extern Eng1CallbackActor *__cdecl eng1_native_code_target_0029670(eng1_u32);
extern unsigned int __cdecl dispatch_engine_2c460(int);
extern void __cdecl eng1_native_code_target_002a290(Eng1CallbackActor *);
extern eng1_u32 __cdecl eng1_native_code_target_002a9e0(Eng1CallbackActor *, eng1_u32);
extern void __cdecl eng1_native_code_target_00283e0(Eng1CallbackActor *);
extern eng1_u32 __cdecl eng1_native_code_target_0026de0(Eng1CallbackActor *);
extern eng1_u32 __cdecl eng1_native_code_target_0019b40(eng1_u32);
extern void __cdecl eng1_native_code_target_001ab50(void);
extern void __cdecl eng1_native_code_target_0030290(eng1_u32, eng1_u32);
extern int __cdecl eng1_native_code_target_0029c20(Eng1CallbackActor *);
extern eng1_u32 __cdecl eng1_native_code_target_002a170(eng1_u32, eng1_u32,
                                             eng1_u32, eng1_u32);
extern int __cdecl eng1_native_code_target_002a710(void *, Eng1CallbackActor *);
extern int __cdecl eng1_native_code_target_002a880(Eng1CallbackActor *, Eng1Bounds3D *,
                                        eng1_u32);
extern int __cdecl bounds_overlap(Eng1Bounds3D *, Eng1Bounds3D *);
extern void __cdecl eng1_native_code_target_00266a0(Eng1CallbackActor *, eng1_u32,
                                         eng1_u32);
extern int __cdecl eng1_native_code_target_002a810(eng1_u32, eng1_u32, eng1_u32, void *);
extern int __cdecl eng1_native_code_target_002aa30(Eng1CallbackActor *, eng1_u32,
                                        eng1_u32);
extern void __cdecl eng1_native_code_target_0028820(Eng1CallbackActor *);

/* Existing measured providers, kept as ordinary external C functions. */
extern void __cdecl append_chain(Eng1CallbackActor *, Eng1CallbackActor *);
extern void __cdecl eng1_guarded_apply_frame_motion_2d(Eng1CallbackActor *);
extern void __cdecl advance_frame(Eng1CallbackActor *);
extern void __cdecl restart_steps(Eng1CallbackActor *);
extern void __cdecl select_frame(Eng1CallbackActor *, int);
extern void __cdecl advance_planar_motion(Eng1CallbackActor *);
extern void __cdecl reset_frame_state(Eng1CallbackActor *);
extern int __cdecl remove_actor(Eng1CallbackActor *);
extern int __cdecl remove_actor_variant(Eng1CallbackActor *);

/* RVA 0x1010: initialize the observed tag and append the actor to g_actor_worklist_716c0. */
void __cdecl eng1_native_code_target_001010(Eng1CallbackActor *actor)
{
    eng1_u32 count;

    actor->render_flags_054 = 0;
    actor->flags_050 = 0x81;
    actor->tag_097 = 0x2a;
    count = g_actor_worklist_716c0[0] + 1;
    g_actor_worklist_716c0[0] = count;
    g_actor_worklist_716c0[count] = (eng1_u32)actor;
}

/* RVA 0x1040: one of the two measured default-actor initializer variants. */
void __cdecl eng1_native_code_target_001040(Eng1CallbackActor *actor)
{
    void *resource;
    eng1_u32 state_index;
    eng1_u8 *configuration;

    resource = eng1_native_code_target_0029670(0x2a);
    if (resource != 0)
        append_chain(resource, actor);

    actor->byte_094 = (eng1_u8)(0 - (eng1_u8)dispatch_engine_2c460(2));
    actor->flags_050 |= 0x200;
    actor->render_flags_054 |= 0x08;
    advance_frame(actor);
    restart_steps(actor);
    actor->dword_0f4 = 2;
    actor->word_124 = 7;
    actor->progress_0ee = 9;

    configuration = (eng1_u8 *)g_engine_interface.context_004;
    if (configuration[0] == 3) {
        actor->byte_01c = 3;
        eng1_native_code_target_002a290(actor);
        return;
    }
    state_index = dispatch_engine_2c460(4);
    actor->byte_01c = g_actor_state_map_004d6ec[state_index];
    eng1_native_code_target_002a290(actor);
}

/* RVA 0x1310: same initialization sequence with its observed constants. */
void __cdecl eng1_native_code_target_001310(Eng1CallbackActor *actor)
{
    void *resource;
    eng1_u32 state_index;
    eng1_u8 *configuration;

    resource = eng1_native_code_target_0029670(0x2a);
    if (resource != 0)
        append_chain(resource, actor);

    actor->byte_094 = (eng1_u8)(0 - (eng1_u8)dispatch_engine_2c460(2));
    actor->flags_050 |= 0x200;
    actor->render_flags_054 |= 0x08;
    advance_frame(actor);
    restart_steps(actor);
    actor->dword_0f4 = 2;
    actor->word_124 = 8;
    actor->progress_0ee = 5;

    configuration = (eng1_u8 *)g_engine_interface.context_004;
    if (configuration[0] == 3) {
        actor->byte_01c = 8;
        eng1_native_code_target_002a290(actor);
        return;
    }
    state_index = dispatch_engine_2c460(4);
    actor->byte_01c = g_actor_state_map_004d6ec[state_index + 4];
    eng1_native_code_target_002a290(actor);
}

/* RVA 0x10f0: actor update branch with a data-driven movement collision mask. */
void __cdecl eng1_native_code_target_0010f0(Eng1CallbackActor *actor)
{
    eng1_u32 action;
    eng1_u32 value;
    eng1_u32 movement_delta;
    eng1_u32 movement_index;
    eng1_u16 collision;
    Eng1Bounds3D sample;

    if (eng1_native_code_target_002a9e0(actor, 0x17c) != 0) {
        remove_actor(actor);
        return;
    }

    if ((actor->byte_025 & 1) != 0) {
        eng1_native_code_target_002a290(actor);
        goto continue_update;
    }

    eng1_native_code_target_00283e0(actor);
    if (actor->state_02a > 8)
        return;

    eng1_guarded_apply_frame_motion_2d(actor);
    if (actor->byte_0ec == 0) {
        action = eng1_native_code_target_0026de0(actor);
        if (action != 0) {
            if (action == 0x80) {
                value = eng1_native_code_target_0019b40(
                    ((eng1_u32)(eng1_u16)g_native_word_72168 ^ 0xffffffffUL) &
                    0x5f);
                if (value != 0) {
                    eng1_native_code_target_001ab50();
                    if (actor->state_02a < 8) {
                        actor->progress_0ee = (eng1_u16)(
                            actor->progress_0ee - (eng1_u16)value);
                        select_frame(actor, 0x0f);
                    } else {
                        select_frame(actor, 0x13);
                    }
                }
            } else if (actor->state_02a < 8) {
                eng1_native_code_target_0030290(action, 0);
                actor->progress_0ee =
                    (eng1_u16)(actor->progress_0ee - 0x64);
                select_frame(actor, 0x0f);
            }
            actor->byte_0ec = 5;
        }
    }

    if (actor->state_02a == 8)
        return;
    if ((short)actor->progress_0ee <= 0) {
        select_frame(actor, 0x12);
        actor->flags_050 &= ~0x200UL;
        return;
    }

    if (!eng1_native_code_target_0029c20(actor)) {
        actor->x_000 = actor->origin_x_09c;
        actor->y_004 = actor->origin_y_0a0;
    } else {
        movement_delta = (actor->byte_094 != 0)
            ? 0xffd00000UL : 0x00300000UL;
        movement_index = eng1_native_code_target_002a170(
            actor->x_000 + movement_delta, actor->y_004, actor->z_008,
            actor->word_038);
        eng1_guard_spatial_read((const void *)g_configuration_dword_713bc,
                                movement_index, 2UL, 2UL, "default actor collision");
        collision = ((eng1_u16 *)(unsigned long)
                     g_configuration_dword_713bc)[movement_index];
        if ((collision & 0x00c2) != 0) {
            actor->x_000 = actor->origin_x_09c;
            actor->y_004 = actor->origin_y_0a0;
            select_frame(actor, 1);
        }
    }

continue_update:
    if (actor->state_02a == 2) {
        if (eng1_native_code_target_002a710(
                (void *)(g_native_pointer_7217c + 0x88), actor))
            select_frame(actor, 6);
    } else {
        if (eng1_native_code_target_002a880(actor, &sample, 2) &&
            bounds_overlap(
                (Eng1Bounds3D *)(g_native_pointer_7217c + 0x88), &sample)) {
            eng1_native_code_target_00266a0(
                actor, actor->word_114 == 0x0c ? 0x23 : 3, 0);
        }
    }

    actor->render_flags_054 =
        (actor->render_flags_054 & ~0x10UL) | (actor->byte_094 & 0x10);
}

/* RVA 0x13c0: related actor update, with its own transition/action constant. */
void __cdecl eng1_native_code_target_0013c0(Eng1CallbackActor *actor)
{
    eng1_u32 action;
    eng1_u32 value;
    eng1_u32 movement_delta;
    eng1_u32 movement_index;
    eng1_u16 collision;
    Eng1Bounds3D sample;

    if (eng1_native_code_target_002a9e0(actor, 0x17c) != 0) {
        remove_actor(actor);
        return;
    }

    if ((actor->byte_025 & 1) != 0) {
        eng1_native_code_target_002a290(actor);
        goto continue_update;
    }

    eng1_native_code_target_00283e0(actor);
    if (actor->state_02a > 8)
        return;

    eng1_guarded_apply_frame_motion_2d(actor);
    if (actor->byte_0ec == 0) {
        action = eng1_native_code_target_0026de0(actor);
        if (action != 0) {
            if (action == 0x80) {
                value = eng1_native_code_target_0019b40(
                    ((eng1_u32)(eng1_u16)g_native_word_72168 ^ 0xffffffffUL) &
                    0x5f);
                if (value != 0) {
                    eng1_native_code_target_001ab50();
                    if (actor->state_02a < 8) {
                        actor->progress_0ee = (eng1_u16)(
                            actor->progress_0ee - (eng1_u16)value);
                        select_frame(actor, 0x0f);
                    } else {
                        select_frame(actor, 0x13);
                    }
                }
            } else if (actor->state_02a < 8) {
                eng1_native_code_target_0030290(action, 0);
                actor->progress_0ee =
                    (eng1_u16)(actor->progress_0ee - 0x64);
                select_frame(actor, 0x0f);
            }
            actor->byte_0ec = 5;
        }
    }

    if (actor->state_02a == 8)
        return;
    if ((short)actor->progress_0ee <= 0) {
        select_frame(actor, 0x12);
        actor->flags_050 &= ~0x200UL;
        return;
    }

    if (!eng1_native_code_target_0029c20(actor)) {
        actor->x_000 = actor->origin_x_09c;
        actor->y_004 = actor->origin_y_0a0;
    } else {
        movement_delta = (actor->byte_094 != 0)
            ? 0xffd00000UL : 0x00300000UL;
        movement_index = eng1_native_code_target_002a170(
            actor->x_000 + movement_delta, actor->y_004, actor->z_008,
            actor->word_038);
        eng1_guard_spatial_read((const void *)g_configuration_dword_713bc,
                                movement_index, 2UL, 2UL, "default actor collision");
        collision = ((eng1_u16 *)(unsigned long)
                     g_configuration_dword_713bc)[movement_index];
        if ((collision & 0x00c2) != 0) {
            actor->x_000 = actor->origin_x_09c;
            actor->y_004 = actor->origin_y_0a0;
            select_frame(actor, 1);
        }
    }

continue_update:
    if (actor->state_02a == 2) {
        if (eng1_native_code_target_002a710(
                (void *)(g_native_pointer_7217c + 0x88), actor))
            select_frame(actor, 6);
    } else {
        if (eng1_native_code_target_002a880(actor, &sample, 2) &&
            bounds_overlap(
                (Eng1Bounds3D *)(g_native_pointer_7217c + 0x88), &sample))
            eng1_native_code_target_00266a0(actor, 3, 0);
    }

    actor->render_flags_054 =
        (actor->render_flags_054 & ~0x10UL) | (actor->byte_094 & 0x10);
}

/* RVA 0x1d80: test the actor against the selected frame area and set flags. */
void __cdecl eng1_native_code_target_001d80(Eng1CallbackActor *actor)
{
    unsigned char *frame_area;

    advance_planar_motion(actor);
    frame_area = g_native_pointer_7217c + 0x88;
    if (eng1_native_code_target_002a810(actor->x_000, actor->y_004, actor->z_008,
                             frame_area)) {
        eng1_native_code_target_00266a0(actor, 3, 0);
        remove_actor_variant(actor);
    }

    actor->render_flags_054 =
        (actor->render_flags_054 & ~0x10UL) | (actor->byte_094 & 0x10);
    if (eng1_native_code_target_002aa30(actor, 0x40, 0x40))
        remove_actor_variant(actor);
}

/* RVA 0x1e00: start a countdown from the actor's current state word. */
void __cdecl eng1_native_code_target_001e00(Eng1CallbackActor *actor)
{
    reset_frame_state(actor);
    actor->render_flags_054 |= 5;
    actor->word_04a = 0x60;
    actor->timer_04c = actor->state_02a;
}

/* RVA 0x1e30: decrement the countdown, then dispatch its zero fallback. */
void __cdecl eng1_native_code_target_001e30(Eng1CallbackActor *actor)
{
    if (actor->timer_04c == 0)
        eng1_native_code_target_0028820(actor);
    else
        --actor->timer_04c;
}
