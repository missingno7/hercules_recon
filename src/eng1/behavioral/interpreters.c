/* Private behavioral reconstruction of ENG1's 0x283e0, 0x28820, and 0x28aa0
 * actor-program interpreters.  Command data types are shared with the adopted
 * frame-program provider.  Shared frame helpers are external dependencies. */
#include "read_guard.h"

static short eng1_inc_word(short value)
{
    return (short)((unsigned short)value + 1);
}

static short eng1_neg_word(short value)
{
    return (short)(0U - (unsigned short)value);
}

static short eng1_guard_read_word(const short *program, short index,
                                  const char *site)
{
    eng1_guard_program_read((const void *)program, (long)index, 2U, 2U, site);
    return program[index];
}

static long eng1_fixed16(short value)
{
    return (long)((unsigned long)(long)value << 16);
}

static long eng1_fixed8(short value)
{
    return (long)((unsigned long)(long)value << 8);
}

static void eng1_add_fixed(long *value, short amount, int subtract)
{
    unsigned long left = (unsigned long)*value;
    unsigned long right = (unsigned long)eng1_fixed16(amount);
    *value = (long)(subtract ? left - right : left + right);
}

static void eng1_run_action_callback(Actor *actor, short index)
{
    Eng1ActorCallback *callbacks;

    callbacks = (Eng1ActorCallback *)(unsigned long)g_configuration_dword_71384;
    callbacks[index](actor);
}

static void eng1_update_angle(short *current, short *delta,
                              short target, int direction)
{
    int distance = (int)target - (int)*current;
    int step = (int)*delta;
    int magnitude;
    int step_magnitude;
    int next;

    if (*delta == 0) return;

    magnitude = (distance < 0) ? -distance : distance;
    step_magnitude = (step < 0) ? -step : step;
    if (magnitude < step_magnitude) {
        *current = target;
        *delta = 0;
        return;
    }

    next = (int)*current + direction * step;
    *current = (short)((unsigned short)next & 0x03ff);
}

static void eng1_update_triple_angles(Actor *actor,
                                     Eng1AnimationControl *control)
{
    eng1_update_angle(&actor->angle_yaw, &control->yaw_step,
                      control->yaw_target, -1);
    eng1_update_angle(&actor->angle_pitch, &control->pitch_step,
                      control->pitch_target, 1);
    eng1_update_angle(&actor->angle_roll, &control->roll_step,
                      control->roll_target, 1);
}

static void eng1_copy_kind_position(Actor *actor)
{
    unsigned char *position;
    unsigned long address;

    if (actor->unknown_058 == 0) return;
    address = (unsigned long)actor->unknown_058 +
              (unsigned long)g_kind_info[(unsigned short)actor->frame_group].owner;
    position = (unsigned char *)address;
    actor->x = *(long *)(position + 0);
    actor->y = *(long *)(position + 4);
    actor->z = *(long *)(position + 8);
}

static void eng1_pair_flags(Actor *actor, short opcode, short operand)
{
    unsigned long mask;

    mask = (unsigned long)(long)operand;
    switch (opcode) {
    case -20:
        actor->render_flags |= mask;
        break;
    case -21:
        actor->render_flags |= ((unsigned long)(unsigned short)operand << 16);
        break;
    case -22:
        actor->render_flags &= ~mask;
        break;
    case -23:
        actor->render_flags &= ~((unsigned long)(unsigned short)operand << 16);
        break;
    case -29:
        actor->flags |= mask;
        break;
    case -30:
        actor->flags &= ~mask;
        break;
    }
}

unsigned long __cdecl eng1_interpret_signed_pairs(Actor *actor)
{
    Eng1CommandPair *program;
    Eng1CommandPair *record;
    short opcode;
    int result = 0;

    actor->frame_ticks = (short)(actor->frame_ticks - 1);
    if (actor->frame_ticks >= 0) return 0;

    for (;;) {
        actor->frame_state = eng1_inc_word(actor->frame_state);
        program = (Eng1CommandPair *)actor->image_or_program;
        eng1_guard_program_read((const void *)program, (long)actor->frame_state,
                                4U, 4U, "signed-pair opcode/operand record");
        record = program + actor->frame_state;
        opcode = record->opcode;
        if (opcode >= 0) break;

        switch (opcode) {
        case -1:
            actor->frame_state = -1;
            result = 1;
            break;
        case -2:
            actor->direction_x = (unsigned char)~actor->direction_x;
            break;
        case -3:
            actor->render_flags ^= 0x10;
            break;
        case -4:
            eng1_guarded_advance_frame(actor);
            actor->frame_state = -1;
            result = 1;
            break;
        case -5:
            spawn_actor_with_owner(actor, actor->x, actor->y, actor->z,
                                  (unsigned short)record->operand);
            break;
        case -6:
            remove_actor(actor);
            opcode = 0;
            break;
        case -7:
            remove_actor_preserving_resource(actor);
            opcode = 0;
            break;
        case -8:
            mark_actor_resource_for_release(actor);
            break;
        case -10:
            eng1_run_action_callback(actor, actor->action_callback_index);
            break;
        case -11:
            eng1_run_action_callback(actor, record->operand);
            break;
        case -12:
            eng1_guarded_select_frame(actor, (int)record->operand);
            result = 1;
            break;
        case -13:
            if (actor->active_step > 0)
                actor->active_step = eng1_neg_word(actor->active_step);
            actor->unknown_f8 = 0;
            break;
        case -14:
            if (actor->active_step < 0)
                actor->active_step = eng1_neg_word(actor->active_step);
            actor->unknown_f8 = 0;
            break;
        case -15:
            actor->active_step = eng1_neg_word(actor->active_step);
            actor->unknown_f8 = 0;
            break;
        case -16:
            actor->step_index = (long)record->operand;
            break;
        case -17:
            actor->vertical_step = (short)((unsigned short)record->operand & 0x00ff);
            break;
        case -18:
            actor->category = (unsigned char)record->operand;
            break;
        case -25:
            eng1_copy_kind_position(actor);
            break;
        case -26:
            eng1_add_fixed(&actor->x, record->operand,
                           actor->direction_x != 0);
            break;
        case -27:
            eng1_add_fixed(&actor->y, record->operand, 0);
            break;
        case -28:
            eng1_add_fixed(&actor->z, record->operand, 0);
            break;
        case -31:
            spawn_actor_at_group(actor->x, actor->y, actor->z,
                                 (unsigned short)record->operand,
                                 (unsigned short)actor->frame_group);
            break;
        case -32:
            actor->interpolation_position = (long)record->operand;
            break;
        case -39:
            play_sound((unsigned short)record->operand, 0);
            break;
        case -40:
            play_sound((unsigned short)record->operand, actor);
            break;
        case -41:
            actor->unknown_02a = (unsigned short)record->operand;
            break;
        case -43:
            actor->unknown_a8 = eng1_fixed8(record->operand);
            break;
        case -44:
            actor->unknown_b8 = eng1_fixed8(record->operand);
            break;
        case -45:
            actor->unknown_c8 = eng1_fixed8(record->operand);
            break;
        case -46:
            stop_actor_sounds(actor);
            break;
        case -47:
            if (actor->child) {
                actor->render_flags |= 0x00800000;
                actor->child->active_command = record->operand;
                actor->child->render_flags =
                    (actor->child->render_flags & ~0x10UL) |
                    ((unsigned long)actor->direction_x & 0x10);
            }
            break;
        case -48:
            actor->render_flags &= ~0x00800000UL;
            break;
        default:
            eng1_pair_flags(actor, opcode, record->operand);
            break;
        }

        if (opcode >= 0) break;
    }

    actor->active_command = opcode;
    actor->frame_ticks = record->operand;
    return (unsigned long)result;
}

unsigned long __cdecl eng1_interpret_word_stream(Actor *actor)
{
    short *program;
    short opcode;
    short operand;
    int result = 0;

    actor->frame_ticks = (short)(actor->frame_ticks - 1);
    if (actor->frame_ticks >= 0) return 0;

    for (;;) {
        actor->frame_state = eng1_inc_word(actor->frame_state);
        program = (short *)actor->image_or_program;
        opcode = eng1_guard_read_word(program, actor->frame_state,
                                      "WORD-stream opcode");
        if (opcode >= 0) break;

        switch (opcode) {
        case -1:
            actor->frame_state = 0;
            result = 1;
            break;
        case -2:
            actor->direction_x = (unsigned char)~actor->direction_x;
            break;
        case -3:
            actor->render_flags ^= 0x10;
            break;
        case -6:
            remove_actor(actor);
            opcode = 0;
            break;
        case -7:
            remove_actor_preserving_resource(actor);
            opcode = 0;
            break;
        case -8:
            mark_actor_resource_for_release(actor);
            break;
        case -9:
            actor->frame_state = eng1_inc_word(actor->frame_state);
            actor->frame_ticks = eng1_guard_read_word(program, actor->frame_state, "WORD-stream -9 duration operand");
            break;
        case -10:
            eng1_run_action_callback(actor, actor->action_callback_index);
            break;
        case -13:
            if (actor->active_step > 0)
                actor->active_step = eng1_neg_word(actor->active_step);
            actor->unknown_f8 = 0;
            break;
        case -14:
            if (actor->active_step < 0)
                actor->active_step = eng1_neg_word(actor->active_step);
            actor->unknown_f8 = 0;
            break;
        case -15:
            actor->active_step = eng1_neg_word(actor->active_step);
            actor->unknown_f8 = 0;
            break;
        case -16:
            actor->frame_state = eng1_inc_word(actor->frame_state);
            actor->step_index = (long)eng1_guard_read_word(program, actor->frame_state, "WORD-stream -16 operand");
            break;
        case -17:
            actor->frame_state = eng1_inc_word(actor->frame_state);
            actor->vertical_step =
                (short)((unsigned short)eng1_guard_read_word(program, actor->frame_state, "WORD-stream -17 operand") & 0x00ff);
            break;
        case -18:
            actor->frame_state = eng1_inc_word(actor->frame_state);
            actor->category = (unsigned char)eng1_guard_read_word(program, actor->frame_state, "WORD-stream -18 operand");
            break;
        case -20:
        case -21:
        case -22:
        case -23:
        case -29:
        case -30:
            /* These signed-stream opcodes carry no operand. Their matching
             * pair opcodes consume one, so the original default arm is a no-op. */
            break;
        case -31:
            actor->frame_state = eng1_inc_word(actor->frame_state);
            operand = eng1_guard_read_word(program, actor->frame_state, "WORD-stream -31 operand");
            spawn_actor_at_group(actor->x, actor->y, actor->z,
                                 (unsigned short)operand,
                                 (unsigned short)actor->frame_group);
            break;
        case -39:
            actor->frame_state = eng1_inc_word(actor->frame_state);
            play_sound((unsigned short)eng1_guard_read_word(program, actor->frame_state, "WORD-stream -39 operand"), 0);
            break;
        case -40:
            actor->frame_state = eng1_inc_word(actor->frame_state);
            play_sound((unsigned short)eng1_guard_read_word(program, actor->frame_state, "WORD-stream -40 operand"), actor);
            break;
        case -41:
            actor->frame_state = eng1_inc_word(actor->frame_state);
            actor->unknown_02a = (unsigned short)eng1_guard_read_word(program, actor->frame_state, "WORD-stream -41 operand") & 0x00ff;
            break;
        case -46:
            stop_actor_sounds(actor);
            break;
        default:
            break;
        }

        if (opcode >= 0) break;
    }

    actor->active_command = opcode;
    if (actor->frame_ticks < 0) {
        program = (short *)actor->image_or_program;
        actor->frame_ticks = eng1_guard_read_word(program, 0, "WORD-stream default duration");
    }
    return (unsigned long)result;
}

static void eng1_store_angle(Actor *actor, Eng1AnimationControl *control,
                             short opcode, short target, short divisor)
{
    if (opcode == -33 || opcode == -36) {
        control->yaw_target = target;
        control->yaw_step = (short)wrapped_ratio((int)actor->angle_yaw,
                                                (int)target, (int)divisor);
    } else if (opcode == -34 || opcode == -37) {
        control->pitch_target = target;
        control->pitch_step = (short)wrapped_ratio((int)actor->angle_pitch,
                                                  (int)target, (int)divisor);
    } else {
        control->roll_target = target;
        control->roll_step = (short)wrapped_ratio((int)actor->angle_roll,
                                                 (int)target, (int)divisor);
    }
}

unsigned long __cdecl eng1_interpret_animation_triples(Actor *actor)
{
    Eng1AnimationControl *control;
    Eng1CommandTriple *program;
    Eng1CommandTriple *record;
    short opcode;
    short target;
    short duration;
    int division_duration;
    int result = 0;

    if (actor->animation_control == 0) return 0;
    control = (Eng1AnimationControl *)*actor->animation_control;
    actor->frame_ticks = (short)(actor->frame_ticks - 1);

    if (actor->frame_ticks < 0) {
        actor->frame_state = eng1_inc_word(actor->frame_state);
        program = (Eng1CommandTriple *)actor->image_or_program;
        eng1_guard_program_read((const void *)program, (long)actor->frame_state,
                                6U, 6U, "animation-triple opcode/operands");
        record = program + actor->frame_state;
        opcode = record->opcode;

        while (opcode < 0) {
            switch (opcode) {
            case -1:
                actor->frame_state = -1;
                result = 1;
                break;
            case -2:
                actor->direction_x = (unsigned char)~actor->direction_x;
                break;
            case -3:
                actor->render_flags ^= 0x10;
                break;
            case -4:
                eng1_guarded_advance_frame(actor);
                actor->frame_state = -1;
                result = 1;
                break;
            case -5:
                spawn_actor_with_owner(actor, actor->x, actor->y, actor->z,
                                       (unsigned short)record->operand_002);
                break;
            case -6:
                remove_actor(actor);
                opcode = 0;
                break;
            case -7:
                remove_actor_preserving_resource(actor);
                opcode = 0;
                break;
            case -8:
                mark_actor_resource_for_release(actor);
                break;
            case -10:
                eng1_run_action_callback(actor, actor->action_callback_index);
                break;
            case -11:
                eng1_run_action_callback(actor, record->operand_004);
                break;
            case -13:
                if (actor->active_step > 0)
                    actor->active_step = eng1_neg_word(actor->active_step);
                actor->unknown_f8 = 0;
                break;
            case -14:
                if (actor->active_step < 0)
                    actor->active_step = eng1_neg_word(actor->active_step);
                actor->unknown_f8 = 0;
                break;
            case -15:
                actor->active_step = eng1_neg_word(actor->active_step);
                actor->unknown_f8 = 0;
                break;
            case -16:
                actor->step_index = (long)record->operand_004;
                break;
            case -20:
            case -21:
            case -22:
            case -23:
            case -29:
            case -30:
                eng1_pair_flags(actor, opcode, record->operand_004);
                break;
            case -24:
                actor->unknown_036 = record->operand_004;
                control->x_target = record->operand_002;
                break;
            case -25:
                eng1_copy_kind_position(actor);
                break;
            case -26:
                eng1_add_fixed(&actor->x, record->operand_004,
                               actor->direction_x != 0);
                break;
            case -27:
                eng1_add_fixed(&actor->y, record->operand_004, 0);
                break;
            case -28:
                eng1_add_fixed(&actor->z, record->operand_004, 0);
                break;
            case -31:
                spawn_actor_at_group(actor->x, actor->y, actor->z,
                                     (unsigned short)record->operand_004,
                                     (unsigned short)actor->frame_group);
                break;
            case -32:
                actor->interpolation_position = (long)record->operand_004;
                break;
            case -33:
            case -34:
            case -35:
                eng1_store_angle(actor, control, opcode,
                                 record->operand_002, record->operand_004);
                break;
            case -36:
            case -37:
            case -38:
                if (opcode == -36)
                    target = (short)(((unsigned short)control->yaw_target +
                                      (unsigned short)record->operand_002) & 0x03ff);
                else if (opcode == -37)
                    target = (short)(((unsigned short)control->pitch_target +
                                      (unsigned short)record->operand_002) & 0x03ff);
                else
                    target = (short)(((unsigned short)control->roll_target +
                                      (unsigned short)record->operand_002) & 0x03ff);
                eng1_store_angle(actor, control, opcode, target,
                                 record->operand_004);
                break;
            case -39:
                play_sound((unsigned short)record->operand_004, 0);
                break;
            case -40:
                play_sound((unsigned short)record->operand_004, actor);
                break;
            case -41:
                actor->unknown_02a = (unsigned short)record->operand_004;
                break;
            case -42:
                eng1_guard_frame_slot((const void *)actor->frames,
                                      (long)record->operand_002,
                                      "animation-triple -42 frame slot");
                actor->image_or_program = actor->frames[record->operand_002];
                actor->frame_state = -1;
                result = 1;
                break;
            case -43:
                actor->unknown_a8 = eng1_fixed8(record->operand_004);
                break;
            case -44:
                actor->unknown_b8 = eng1_fixed8(record->operand_004);
                break;
            case -45:
                actor->unknown_c8 = eng1_fixed8(record->operand_004);
                break;
            case -46:
                stop_actor_sounds(actor);
                break;
            default:
                break;
            }

            if (opcode >= 0) break;
            actor->frame_state = eng1_inc_word(actor->frame_state);
            program = (Eng1CommandTriple *)actor->image_or_program;
            eng1_guard_program_read((const void *)program,
                                    (long)actor->frame_state, 6U, 6U,
                                    "animation-triple next opcode/operands");
            record = program + actor->frame_state;
            opcode = record->opcode;
        }

        actor->active_command = opcode;
        eng1_guard_program_read((const void *)actor->image_or_program,
                                (long)actor->frame_state, 6U, 6U,
                                "animation-triple terminal duration/target");
        record = (Eng1CommandTriple *)actor->image_or_program + actor->frame_state;
        duration = record->operand_004;
        if (((unsigned short)duration & 0x4000) != 0) {
            duration = (short)(((unsigned short)duration & 0x3fff) +
                               dispatch_engine_2c460(6));
        }
        actor->frame_ticks = duration;

        target = record->operand_002;
        if (target < 0) {
            control->x_current = target;
            control->x_step = eng1_neg_word(target);
        } else {
            division_duration = (int)duration;
            if (division_duration < 1) division_duration = 1;
            actor->interpolation_position = (long)target;
            control->x_current = target;
            control->x_step = (short)(((int)control->x_target - (int)target) /
                                      division_duration);
        }
    }

    if (actor->frame_ticks != 0) {
        actor->interpolation_position =
            (long)((unsigned long)actor->interpolation_position +
                   (unsigned long)(long)control->x_step);
    } else if (control->x_current >= 0) {
        actor->interpolation_position = (long)control->x_target;
    }

    eng1_update_triple_angles(actor, control);
    return (unsigned long)result;
}
