#include <stdlib.h>
#include "spatial_read_guard.h"
/*
 * Source hypothesis for callback RVAs referenced by the measured ENG1
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
    long x_000;
    long y_004;
    long z_008;
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
    long timer_04c;
    eng1_u32 flags_050;
    eng1_u32 render_flags_054;
    eng1_u8 pad_058[0x94 - 0x58];
    eng1_u8 byte_094;
    eng1_u8 pad_095[0x97 - 0x95];
    eng1_u8 tag_097;
    eng1_u8 pad_098[0x9c - 0x98];
    long origin_x_09c;
    long origin_y_0a0;
    eng1_u8 pad_0a4[0xec - 0xa4];
    eng1_u8 byte_0ec;
    eng1_u8 pad_0ed;
    short progress_0ee;
    eng1_u8 pad_0f0[0xf4 - 0xf0];
    long dword_0f4;
    eng1_u8 pad_0f8[0x114 - 0xf8];
    short word_114;
    eng1_u8 pad_116[0x124 - 0x116];
    short word_124;
    eng1_u8 pad_126[6];
    Eng1CallbackActor *previous_12c;
    Eng1CallbackActor *next_130;
};

typedef char eng1_callback_actor_size_is_134[
    (sizeof(Eng1CallbackActor) == 0x134) ? 1 : -1];
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
#include "actor_pipeline_storage.h"
extern eng1_u32 g_configuration_dword_713bc;
extern eng1_u16 g_native_word_72168;
extern eng1_u8 g_actor_state_map_004d6ec[8];
extern Eng1CallbackActor *g_native_pointer_7217c;

typedef struct Eng1Bounds3D Eng1Bounds3D;
struct Eng1Bounds3D {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
};

/* Unrecovered nested code stays a real link dependency, identified by RVA. */
extern Eng1CallbackActor *__cdecl find_tag(eng1_u32);
extern unsigned int __cdecl random_limit(int);
extern void __cdecl eng1_native_code_target_002a290(Eng1CallbackActor *);
extern eng1_u32 __cdecl eng1_native_code_target_002a9e0(Eng1CallbackActor *, eng1_u32);
extern void __cdecl eng1_native_code_target_00283e0(Eng1CallbackActor *);
extern eng1_u32 __cdecl eng1_native_code_target_0026de0(Eng1CallbackActor *);
extern eng1_u32 __cdecl eng1_native_code_target_0019b40(eng1_u32);
extern void __cdecl eng1_native_code_target_001ab50(void);
extern void __cdecl eng1_native_code_target_0030290(eng1_u32, eng1_u32);
extern int __cdecl eng1_native_code_target_0029c20(Eng1CallbackActor *);
extern eng1_u32 __cdecl dispatch_local_2a170(long, long, long, int);
extern int __cdecl eng1_native_code_target_002a710(void *, Eng1CallbackActor *);
extern int __cdecl eng1_native_code_target_002a880(Eng1CallbackActor *, Eng1Bounds3D *,
                                        eng1_u32);
extern int __cdecl bounds_overlap(Eng1Bounds3D *, Eng1Bounds3D *);
extern void __cdecl eng1_native_code_target_00266a0(Eng1CallbackActor *, eng1_u32,
                                         eng1_u32);
extern int __cdecl point_in_box_2a810(eng1_u32, eng1_u32, eng1_u32, void *);
extern int __cdecl eng1_native_code_target_002aa30(Eng1CallbackActor *, eng1_u32,
                                        eng1_u32);
extern void __cdecl eng1_native_code_target_0028820(Eng1CallbackActor *);

/* Existing measured providers, kept as ordinary external C functions. */
extern void __cdecl append_chain(Eng1CallbackActor *, Eng1CallbackActor *);
extern void __cdecl apply_frame_motion_2d(Eng1CallbackActor *);
extern void __cdecl advance_frame(Eng1CallbackActor *);
extern void __cdecl restart_steps(Eng1CallbackActor *);
extern void __cdecl select_frame(Eng1CallbackActor *, int);
extern void __cdecl advance_planar_motion(Eng1CallbackActor *);
extern void __cdecl reset_frame_state(Eng1CallbackActor *);
extern int __cdecl remove_actor(Eng1CallbackActor *);
extern int __cdecl remove_actor_variant(Eng1CallbackActor *);

/* Native chain links at 0x12c/0x130 extend this actor view to NStype size. */
extern unsigned long eng1_sim_state_72110;
extern long eng1_2a010(long, long, long, int);
typedef struct CallbackOffset { short x, y; } CallbackOffset;
extern int title_18600(Eng1CallbackActor *, CallbackOffset *);
extern Eng1CallbackActor *title_17ad0(long, long, long, int, int);


/* RVA 0x1010: initialize the observed tag and append the actor to ((eng1_u32 *)&g_actor_pipeline_storage_71440.dword_cells[0x280 / 4]). */
void __cdecl eng1_native_code_target_001010(Eng1CallbackActor *actor)
{
    eng1_u32 count;

    actor->render_flags_054 = 0;
    actor->flags_050 = 0x81;
    actor->tag_097 = 0x2a;
    count = ((eng1_u32 *)&g_actor_pipeline_storage_71440.dword_cells[0x280 / 4])[0] + 1;
    ((eng1_u32 *)&g_actor_pipeline_storage_71440.dword_cells[0x280 / 4])[0] = count;
    ((eng1_u32 *)&g_actor_pipeline_storage_71440.dword_cells[0x280 / 4])[count] = (eng1_u32)actor;
}

/* RVA 0x1040: one of the two measured default-actor initializer variants. */
void __cdecl eng1_native_code_target_001040(Eng1CallbackActor *actor)
{
    void *resource;

    resource = find_tag(0x2a);
    if (resource != 0)
        append_chain(resource, actor);

    actor->byte_094 = -random_limit(2);
    actor->flags_050 |= 0x200;
    actor->render_flags_054 |= 0x08;
    advance_frame(actor);
    restart_steps(actor);
    actor->dword_0f4 = 2;
    actor->word_124 = 7;
    actor->progress_0ee = 9;

    if (*(eng1_u8 *)g_engine_interface.context_004 == 3) {
        actor->byte_01c = 3;
    } else {
        actor->byte_01c = g_actor_state_map_004d6ec[random_limit(4)];
    }
    eng1_native_code_target_002a290(actor);
}

/* RVA 0x1310: same initialization sequence with its observed constants. */
void __cdecl eng1_native_code_target_001310(Eng1CallbackActor *actor)
{
    void *resource;

    resource = find_tag(0x2a);
    if (resource != 0)
        append_chain(resource, actor);

    actor->byte_094 = -random_limit(2);
    actor->flags_050 |= 0x200;
    actor->render_flags_054 |= 0x08;
    advance_frame(actor);
    restart_steps(actor);
    actor->dword_0f4 = 2;
    actor->word_124 = 8;
    actor->progress_0ee = 5;

    if (*(eng1_u8 *)g_engine_interface.context_004 == 3) {
        actor->byte_01c = 8;
    } else {
        actor->byte_01c = g_actor_state_map_004d6ec[random_limit(4) + 4];
    }
    eng1_native_code_target_002a290(actor);
}

/* RVA 0x10f0: actor update branch with a data-driven movement collision mask. */
void __cdecl eng1_native_code_target_0010f0(Eng1CallbackActor *actor)
{
    long action;
    int value;
    int collision;
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

    apply_frame_motion_2d(actor);
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
                        actor->progress_0ee -= value;
                        select_frame(actor, 0x0f);
                    } else {
                        select_frame(actor, 0x13);
                    }
                }
            } else if (actor->state_02a < 8) {
                eng1_native_code_target_0030290(action, 0);
                actor->progress_0ee -= 100;
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
        select_frame(actor, 1);
    } else {
        collision = ((eng1_u16 *)(unsigned long)g_configuration_dword_713bc)[
            dispatch_local_2a170((actor->byte_094 ? -0x300000L : 0x300000L) + actor->x_000,
                actor->y_004, actor->z_008, actor->word_038)];
        if (((eng1_u16)collision & 0x00c2) != 0) {
            actor->x_000 = actor->origin_x_09c;
            actor->y_004 = actor->origin_y_0a0;
            select_frame(actor, 1);
        }
    }

continue_update:
    if (actor->state_02a == 2) {
        if (eng1_native_code_target_002a710(
                (void *)((unsigned char *)g_native_pointer_7217c + 0x88), actor))
            select_frame(actor, 6);
    } else {
        if (eng1_native_code_target_002a880(actor, &sample, 2) &&
            bounds_overlap(
                (Eng1Bounds3D *)((unsigned char *)g_native_pointer_7217c + 0x88), &sample)) {
            if (actor->word_114 == 0x0c)
                eng1_native_code_target_00266a0(actor, 0x23, 0);
            else
                eng1_native_code_target_00266a0(actor, 3, 0);
        }
    }

    collision = actor->byte_094 & 0x10;
    actor->render_flags_054 = (actor->render_flags_054 & ~0x10UL) | collision;
}

/* RVA 0x13c0: related actor update, with its own transition/action constant. */
void __cdecl eng1_native_code_target_0013c0(Eng1CallbackActor *actor)
{
    long action;
    int value;
    int collision;
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

    apply_frame_motion_2d(actor);
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
                        actor->progress_0ee -= value;
                        select_frame(actor, 0x0f);
                    } else {
                        select_frame(actor, 0x13);
                    }
                }
            } else if (actor->state_02a < 8) {
                eng1_native_code_target_0030290(action, 0);
                actor->progress_0ee -= 100;
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
        select_frame(actor, 1);
    } else {
        collision = ((eng1_u16 *)(unsigned long)g_configuration_dword_713bc)[
            dispatch_local_2a170((actor->byte_094 ? -0x300000L : 0x300000L) + actor->x_000,
                actor->y_004, actor->z_008, actor->word_038)];
        if (((eng1_u16)collision & 0x00c2) != 0) {
            actor->x_000 = actor->origin_x_09c;
            actor->y_004 = actor->origin_y_0a0;
            select_frame(actor, 1);
        }
    }

continue_update:
    if (actor->state_02a == 2) {
        if (eng1_native_code_target_002a710(
                (void *)((unsigned char *)g_native_pointer_7217c + 0x88), actor))
            select_frame(actor, 6);
    } else {
        if (eng1_native_code_target_002a880(actor, &sample, 2) &&
            bounds_overlap(
                (Eng1Bounds3D *)((unsigned char *)g_native_pointer_7217c + 0x88), &sample))
            eng1_native_code_target_00266a0(actor, 3, 0);
    }

    collision = actor->byte_094 & 0x10;
    actor->render_flags_054 = (actor->render_flags_054 & ~0x10UL) | collision;
}

/* RVA 0x1d80: test the actor against the selected frame area and set flags. */
void __cdecl eng1_native_code_target_001d80(Eng1CallbackActor *actor)
{
    unsigned char *frame_area;

    advance_planar_motion(actor);
    frame_area = (unsigned char *)g_native_pointer_7217c + 0x88;
    if (point_in_box_2a810(actor->x_000, actor->y_004, actor->z_008,
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

void __cdecl eng1_native_code_target_0015e0(Eng1CallbackActor *actor)
{
    int distance, magnitude, count, collision;
    long movement_index;
    long movement_delta;
    Eng1CallbackActor *p;

    distance = ((long)actor->x_000 - g_native_pointer_7217c->x_000) >> 16;
    movement_delta = actor->byte_094 ? 0x300000 : -0x300000;
    collision = (unsigned short)((eng1_u16 *)g_configuration_dword_713bc)[
        dispatch_local_2a170(actor->x_000 + movement_delta,
            actor->y_004, actor->z_008, actor->word_038)];
    collision &= 0xc2;
    switch (actor->state_02a) {
    case 0: case 1: case 2:
        if (eng1_sim_state_72110) count = 999;
        else {
            count = 0;
            p = actor;
            if (p->previous_12c) {
                do { p = p->previous_12c; } while (p->previous_12c);
                while (p->next_130) {
                    if (p->state_02a == 4) count++;
                    p = p->next_130;
                }
            }
        }
        if (abs(((long)actor->z_008 - g_native_pointer_7217c->z_008) >> 16) > 16)
            count = 99;
        magnitude = labs(distance);
        if (magnitude > 0x40 && !random_limit(9)) { select_frame(actor, 0); return; }
        if (distance < 0x56 && collision) {
            if (random_limit(2)) select_frame(actor, 9);
            else select_frame(actor, 12);
            return;
        }
        if ((distance < 0 && actor->byte_094 == 255) ||
            (distance >= 0 && actor->byte_094 == 0)) {
            if (magnitude < 20) select_frame(actor, 22);
            else select_frame(actor, 1);
            return;
        }
        if (count < 3) {
            if (magnitude < 16) { select_frame(actor, 22); return; }
            if (magnitude < 34) { select_frame(actor, 27); return; }
        } else select_frame(actor, 22);
        if (magnitude < 44) {
            if (count < 4) {
                if (!random_limit(3)) { select_frame(actor, 27); return; }
                if (!random_limit(3)) { select_frame(actor, 22); return; }
                select_frame(actor, 9); return;
            }
            if (!random_limit(3)) { select_frame(actor, 27); return; }
            select_frame(actor, 22); return;
        }
        if (magnitude < 86) {
            if (count < 2) {
                if (magnitude > 50 && !random_limit(6)) { select_frame(actor, 30); return; }
                if (!random_limit(8)) { select_frame(actor, 22); return; }
                if (!random_limit(6)) { select_frame(actor, 0); return; }
                select_frame(actor, 12); return;
            }
            if (!random_limit(6)) { select_frame(actor, 30); return; }
            if (!random_limit(8)) { select_frame(actor, 22); return; }
            select_frame(actor, 0); return;
        }
        if (actor->word_114 != 4 && actor->word_114 != 5) select_frame(actor, 4);
        else select_frame(actor, 5);
        break;
    }
}

void __cdecl eng1_native_code_target_001910(Eng1CallbackActor *actor)
{
    int distance, magnitude, count, collision;
    long movement_index;
    long movement_delta;
    long left, right, best, direction;
    unsigned short lower_height;
    short upper_height;
    Eng1CallbackActor *p;
    CallbackOffset pos;

    distance = ((long)actor->x_000 - g_native_pointer_7217c->x_000) >> 16;
    movement_delta = actor->byte_094 ? 0x300000 : -0x300000;
    collision = (unsigned short)((eng1_u16 *)g_configuration_dword_713bc)[
        dispatch_local_2a170(actor->x_000 + movement_delta,
            actor->y_004, actor->z_008, actor->word_038)];
    collision &= 0xc2;
    switch (actor->state_02a) {
    case 0: case 1: case 2:
        if (eng1_sim_state_72110) count = 999;
        else {
            count = 0;
            if (actor->previous_12c) {
                p = actor;
                while (p->previous_12c) p = p->previous_12c;
                while (p->next_130) {
                    if (p->state_02a == 4) count++;
                    p = p->next_130;
                }
            }
        }
        if (abs(((long)actor->z_008 - g_native_pointer_7217c->z_008) >> 16) > 16)
            count = 99;
        magnitude = abs(distance);
        if (magnitude > 96 && !random_limit(9)) { select_frame(actor, 0); break; }
        if (distance < 128 && collision) {
            if (random_limit(2)) select_frame(actor, 9);
            else select_frame(actor, 12);
            break;
        }
        if ((distance < 0 && actor->byte_094 == 255) ||
            (distance >= 0 && actor->byte_094 == 0)) {
            if (magnitude < 20) select_frame(actor, 22);
            else select_frame(actor, 1);
            break;
        }
        if (magnitude < 44) { select_frame(actor, 22); break; }
        if (magnitude < 76) {
            if (count < 4 && !random_limit(6)) {
                select_frame(actor, 9);
                select_frame(actor, 22);
            } else select_frame(actor, 22);
            break;
        }
        if (magnitude < 128) {
            if (count < 2) {
                if (!random_limit(6)) { select_frame(actor, 4); break; }
                if (!random_limit(8)) { select_frame(actor, 22); break; }
                if (!random_limit(6)) { select_frame(actor, 0); break; }
                select_frame(actor, 9); break;
            }
            if (!random_limit(6)) { select_frame(actor, 4); break; }
            if (!random_limit(8)) { select_frame(actor, 22); break; }
            select_frame(actor, 0); break;
        }
        if (actor->word_114 != 4 && actor->word_114 != 5) select_frame(actor, 4);
        else select_frame(actor, 5);
        break;
    case 5:
        title_18600(actor, &pos);
        p = title_17ad0(actor->x_000 + ((long)pos.x << 16),
            actor->y_004 + ((long)pos.y << 16), actor->z_008, 0x4027, actor->word_038);
        if (p) p->byte_094 = actor->byte_094;
        break;
    case 9:
        left = eng1_2a010(actor->x_000 - 0x400000, actor->y_004, actor->z_008, 0);
        right = eng1_2a010(actor->x_000 + 0x400000, actor->y_004, actor->z_008, 0);
        direction = actor->byte_094;
        best = 0x7fff;
        lower_height = right & 0xffff;
        if (lower_height < best) { best = lower_height; direction = -1; }
        upper_height = (short)(right >> 16);
        if (upper_height < best) { best = upper_height; direction = -1; }
        lower_height = left & 0xffff;
        if (lower_height < best) { best = lower_height; direction = 0; }
        upper_height = (short)(left >> 16);
        if (upper_height < best) direction = 0;
        if (actor->byte_094) {
            if (right) {
                if (!left) actor->byte_094 = ~actor->byte_094;
                else actor->byte_094 = (eng1_u8)direction;
            }
        } else if (left) {
            if (!right) actor->byte_094 = 255;
            else actor->byte_094 = (eng1_u8)direction;
        }
        break;
    }
    actor->render_flags_054 = (actor->render_flags_054 & ~0x10UL) | (actor->byte_094 & 0x10);
}
