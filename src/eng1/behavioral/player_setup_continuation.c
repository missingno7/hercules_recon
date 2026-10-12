/* Ordinary-C behavioral reconstruction of ENG1.DLL RVA 0x15d00.
 *
 * The entry has no stack arguments, uses cdecl cleanup, and leaves a 32-bit
 * EAX result (0 on the timer/command paths, 1 for opcode -3).  The callers
 * observed at 0x16790 and 0x19440 ignore that result.  This file owns no game
 * storage.  Unknown code/data services fail through eng1_incomplete_behavior;
 * all unresolved callees are separately recorded in the central manifest.
 */
#include <stddef.h>
#include "../../../calibration/engine_interface.h"
#include "setup_shared_state.h"
#include "simulation_data_owners.h"
#include "incomplete.h"
#include "script_54558_owner.h"

typedef unsigned char setup_u8;
typedef unsigned short setup_u16;
typedef unsigned long setup_u32;

typedef char setup_pointer_must_be_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char setup_word_must_be_16[(sizeof(setup_u16) == 2) ? 1 : -1];
typedef char setup_dword_must_be_32[(sizeof(setup_u32) == 4) ? 1 : -1];
typedef char setup_signed_long_must_be_32[(sizeof(long) == 4) ? 1 : -1];

extern void *g_normal_actor_pool;                 /* native cell 0x72860 */
extern setup_u8 *g_collision_context;             /* native cell 0x7217c */
extern unsigned long eng1_sim_state_72110;        /* native cell 0x72110 */
extern setup_u16 eng1_sim_state_72108;            /* native cell 0x72108 */
extern setup_u8 g_dispatch_key_7286c;
extern setup_u16 g_dispatch_setting_4d858;
extern setup_u16 g_dispatch_word_6ee80;
extern setup_u16 g_eng1_setup_word_72194;
extern void __cdecl dispatch_engine_2c4e0(unsigned long value);
extern unsigned int __cdecl dispatch_engine_2c460(int limit);
extern void __cdecl play_sound(unsigned short effect_id, void *actor);
extern void __cdecl dispatch_local_16790(void);

/* Missing native services are ordinary external dependencies. Transitional
 * builds provide centrally generated, nonreturning diagnostics for each RVA. */
extern setup_u32 __cdecl eng1_native_code_target_0030010(setup_u32, setup_u32, setup_u32, setup_u32);
extern void *__cdecl eng1_native_dep_0300c0(setup_u32, setup_u32, setup_u32, setup_u32, setup_u32);
extern void __cdecl spawn_actor_with_owner(void *, long, long, long, unsigned short);
extern void __cdecl eng1_native_code_target_001a220(void);
extern void __cdecl eng1_native_code_target_001a280(void);
extern void __cdecl eng1_native_code_target_001a3e0(void);
extern void __cdecl eng1_native_code_target_001a410(void);
extern void __cdecl eng1_native_code_target_0007f8e(int, void *);

/* Live loads match the native accesses which re-read the global pointer after
 * callbacks or indirect actor changes. */
#define SETUP_LIVE_ACTOR() \
    (*(void * volatile *)(void *)&g_normal_actor_pool)
#define SETUP_LIVE_COLLISION() \
    (*(setup_u8 * volatile *)(void *)&g_collision_context)
#define SETUP_LIVE_INTERFACE_CONTEXT() \
    (*(ResourceCallbackContext * volatile *)(void *)&g_engine_interface.context_004)

static setup_u16 setup_read_u16(const void *p)
{ return *(const setup_u16 *)(const void *)p; }
static setup_u32 setup_read_u32(const void *p)
{ return *(const setup_u32 *)(const void *)p; }
static void setup_write_u16(void *p, setup_u16 value)
{ *(setup_u16 *)(void *)p = value; }
static void setup_write_u32(void *p, setup_u32 value)
{ *(setup_u32 *)(void *)p = value; }
static long setup_s16(setup_u16 value)
{ return (value & 0x8000U) ? (long)value - 0x10000L : (long)value; }
static setup_u32 setup_sext16(setup_u16 value)
{ return (setup_u32)setup_s16(value); }

/* Map source-backed program pointers to their measured original RVAs.  The
 * 0x53820 logical script is exactly three 4-byte records.  Other default
 * programs have local bounds only where the existing owner or a measured
 * sentinel proves them; the 0x538d0 stream stops at the owned 0x53900 edge. */
static setup_u32 setup_program_rva(const setup_u8 *program)
{
    setup_u32 p = (setup_u32)program;
    setup_u32 owner = (setup_u32)(void *)g_eng1_simulation_lookup_owner_537e0.value_view_53800;
    setup_u32 alternate = (setup_u32)(void *)g_eng1_setup_program_54558;
    if (p >= owner && p <= owner + 0x100UL)
        return 0x53800UL + (p - owner);
    if (p >= alternate && p <= alternate + 0x17cUL)
        return 0x54558UL + (p - alternate);
    return 0xffffffffUL;
}

static setup_u8 *setup_program_at_rva(setup_u32 rva)
{
    setup_u8 *owner = (setup_u8 *)(void *)
        g_eng1_simulation_lookup_owner_537e0.value_view_53800;
    if (rva >= 0x53800UL && rva <= 0x53900UL)
        return owner + (rva - 0x53800UL);
    if (rva >= 0x54558UL && rva <= 0x546d4UL)
        return (setup_u8 *)(void *)g_eng1_setup_program_54558 + (rva - 0x54558UL);
    eng1_incomplete_behavior(rva,
        "setup program has no measured source owner; abort before assignment");
    return 0;
}

static setup_u32 setup_program_prefix_bytes(const setup_u8 *program)
{
    setup_u32 rva = setup_program_rva(program);
    if (rva == 0x53820UL) return 12UL;
    if (rva == 0x53830UL) return 68UL;
    if (rva == 0x53878UL) return 88UL;
    if (rva == 0x538d0UL) return 48UL;
    if (rva >= 0x54558UL && rva <= 0x546d4UL)
        return 0x546d4UL - rva;
    eng1_incomplete_behavior(rva == 0xffffffffUL ? 0x15d00UL : rva,
        "setup command pointer has no registered measured prefix; abort before read");
    return 0;
}

/* The opcode read is separate from operand reads. Some handlers return or
 * change programs without reading an operand; -13 reads only its low BYTE. */
static setup_u16 setup_read_program(const setup_u8 *program, long index,
                                   setup_u32 lane, setup_u32 width)
{
    setup_u32 bytes = setup_program_prefix_bytes(program);
    setup_u32 offset;
    if (index < 0)
        eng1_incomplete_behavior(setup_program_rva(program),
            "negative setup command index is outside the measured prefix");
    offset = (setup_u32)index * 4UL + lane;
    if (offset > bytes || bytes - offset < width) {
        setup_u32 rva = setup_program_rva(program);
        eng1_incomplete_behavior(rva == 0x538d0UL ? 0x53900UL : rva,
            "setup command read exceeds its measured prefix; abort before read");
    }
    return width == 1 ? program[offset] : setup_read_u16(program + offset);
}

static setup_u16 setup_actor_operand(const setup_u8 *actor)
{
    long index = setup_s16(setup_read_u16(actor + 0x32));
    const setup_u8 *program = *(setup_u8 * const *)(const void *)(actor + 0x64);
    return setup_read_program(program, index, 2UL, 2UL);
}

static void setup_set_program(void *actor, setup_u32 rva)
{ *(setup_u8 **)(void *)((setup_u8 *)actor + 0x64) = setup_program_at_rva(rva); }

/* Return the physical 32-bit EAX result retained by the native entry. */
setup_u32 __cdecl dispatch_local_15d00(void)
{
    setup_u8 *actor;
    setup_u8 *program;
    setup_u8 *collision;
    setup_u8 *context;
    setup_u8 *child;
    setup_u16 timer;
    setup_u16 index_word;
    setup_u16 operand;
    setup_u16 command;
    long index;
    setup_u32 value;
    setup_u32 flags;
    setup_u32 x;
    setup_u32 y;
    setup_u32 z;
    setup_u32 selector;
    setup_u32 rva;
    long signed_y;
    long cutoff;

    actor = (setup_u8 *)SETUP_LIVE_ACTOR();
    timer = setup_read_u16(actor + 0x30);
    setup_write_u16(actor + 0x30, (setup_u16)(timer - 1U));
    actor = (setup_u8 *)SETUP_LIVE_ACTOR();
    timer = setup_read_u16(actor + 0x30);
    if (setup_s16(timer) >= 0)
        return 0UL;

    for (;;) {
        actor = (setup_u8 *)SETUP_LIVE_ACTOR();
        index_word = setup_read_u16(actor + 0x32);
        setup_write_u16(actor + 0x32, (setup_u16)(index_word + 1U));
        actor = (setup_u8 *)SETUP_LIVE_ACTOR();
        index = setup_s16(setup_read_u16(actor + 0x32));
        program = *(setup_u8 **)(void *)(actor + 0x64);
        command = setup_read_program(program, index, 0UL, 2UL);
        if (setup_s16(command) >= 0) {
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            setup_write_u16(actor + 0x34, command);
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            index = setup_s16(setup_read_u16(actor + 0x32));
            program = *(setup_u8 **)(void *)(actor + 0x64);
            operand = setup_read_program(program, index, 2UL, 2UL);
            setup_write_u16(actor + 0x30, operand);
            return 0UL;
        }

        switch (setup_s16(command)) {
        case -52: /* 0x16472: height-gated sound sequence. */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            value = context[0];
            if (value == 3 || value == 1) {
                setup_u16 height = g_eng1_setup_word_718e0;
                if (height != 0) {
                    actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                    signed_y = setup_s16((setup_u16)(setup_read_u32(actor + 4) >> 16));
                    if (signed_y > (long)height) {
                        collision = SETUP_LIVE_COLLISION();
                        play_sound(setup_read_program(program, index, 2UL, 2UL), collision);
                        g_eng1_setup_dword_6efa8 = 3;
                        break;
                    }
                }
                value = g_eng1_setup_dword_6efa8;
                if (value == 3) {
                    collision = SETUP_LIVE_COLLISION();
                    play_sound(0x401eU, collision);
                    g_eng1_setup_dword_6efa8 = 2;
                } else if (value == 2) {
                    collision = SETUP_LIVE_COLLISION();
                    play_sound(0x401fU, collision);
                    g_eng1_setup_dword_6efa8 = 1;
                } else if (value == 1) {
                    collision = SETUP_LIVE_COLLISION();
                    play_sound(0x4020U, collision);
                    g_eng1_setup_dword_6efa8 = 0;
                }
            }
            break;
        case -51: /* 0x16668 */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            context[3] = 2;
            break;
        case -50: /* 0x16618: DWORD load, then WORD store to same low lane. */
            rva = setup_program_rva(program);
            value = (rva == 0x53c40UL) ? 0x10UL : 0xfffffff0UL;
            value += (setup_u32)g_eng1_setup_dword_6ef9c;
            value &= 0x3ffUL;
            g_eng1_setup_word_6ef9c = (setup_u16)value;
            break;
        case -49: /* 0x1660b */
            operand = setup_read_program(program, index, 2UL, 2UL);
            g_eng1_setup_word_6ef98 = operand;
            break;
        case -48: /* 0x16201 */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            value = actor[0x94];
            if (eng1_sim_state_72110 == 0 &&
                !((context[0x5c] & 0x80U) != 0 && value != 0) &&
                !((context[0x5c] & 0x20U) != 0 && value == 0)) {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_set_program(actor, 0x54850UL);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x30, 0xffffU);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            }
            break;
        case -47: /* 0x161ac */
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            value = actor[0x94];
            if ((eng1_sim_state_72110 == 0 &&
                 (context[0x5c] & 0x80U) != 0 && value == 0) ||
                ((context[0x5c] & 0x20U) != 0 && value != 0)) {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_set_program(actor, 0x54890UL);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x30, 0xffffU);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            }
            break;
        case -46: /* 0x165f6: clear collision flag only when child exists. */
            collision = SETUP_LIVE_COLLISION();
            child = *(setup_u8 **)(void *)(collision + 0x60);
            if (child != 0) {
                flags = setup_read_u32(collision + 0x54);
                setup_write_u32(collision + 0x54, flags & 0xff7fffffUL);
            }
            break;
        case -45: /* 0x16541: configure child actor, with later live reloads. */
            collision = SETUP_LIVE_COLLISION();
            child = *(setup_u8 **)(void *)(collision + 0x60);
            if (child == 0)
                break;
            operand = setup_read_program(program, index, 2UL, 2UL);
            setup_write_u16(child + 0x34, operand);
            collision = SETUP_LIVE_COLLISION();
            g_eng1_setup_dword_6efa0 = 5UL;
            flags = setup_read_u32(collision + 0x54) | 0x800000UL;
            setup_write_u32(collision + 0x54, flags);
            setup_write_u32(child + 0x54, 0x04000605UL);
            collision = SETUP_LIVE_COLLISION();
            value = collision[0x94];
            child[0x22] = 1;
            value = (value & 0x10U) | 0x04000605UL;
            setup_write_u32(child + 0x54, value);
            value = setup_read_u16(child + 0x34);
            if (value == 0x1dfU || value == 0x1e0U) {
                context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
                if (context[0] == 3 || context[0] == 1) {
                    setup_u16 limit = g_eng1_setup_word_718e0;
                    if (limit != 0) {
                        actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                        signed_y = setup_s16((setup_u16)(setup_read_u32(actor + 4) >> 16));
                        cutoff = (long)limit - 8L;
                        if (signed_y > cutoff) {
                            collision = SETUP_LIVE_COLLISION();
                            flags = setup_read_u32(collision + 0x54);
                            setup_write_u32(collision + 0x54, flags & 0xff7fffffUL);
                        }
                    }
                }
            }
            break;
        case -44: /* 0x1615f */
            g_eng1_setup_word_6ef84 = 0;
            break;
        case -43: /* 0x16198 */
            operand = setup_read_program(program, index, 2UL, 2UL);
            g_eng1_setup_word_72012 = (setup_u16)(
                g_eng1_setup_word_72012 & (setup_u16)~operand);
            break;
        case -42: /* 0x16187 */
            operand = setup_read_program(program, index, 2UL, 2UL);
            g_eng1_setup_word_72012 = (setup_u16)(
                g_eng1_setup_word_72012 | operand);
            break;
        case -41: eng1_native_code_target_001a280(); break;
        case -40: eng1_native_code_target_001a220(); break;
        case -39: /* 0x15dc3 */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            if ((setup_read_u16(context + 0x5c) & setup_read_u16(context + 0x6c)) == 0 ||
                g_eng1_setup_byte_6efb4 != 2) {
                eng1_sim_state_72108 = 0x2e;
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_set_program(actor, 0x541d8UL);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x30, 0xffffU);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            }
            break;
        case -38: g_eng1_setup_word_6ef90 = 1; break;
        case -37: g_eng1_setup_word_6ef7c = 0; break;
        case -36: g_eng1_setup_word_6ef7c = 1; break;
        case -35: /* 0x163f5 */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            if ((setup_read_u16(context + 0x5c) & setup_read_u16(context + 0x70)) == 0) {
                if ((context[0x5c] & 0xa0U) != 0) {
                    dispatch_local_16790();
                } else {
                    eng1_sim_state_72108 = 0x29;
                    setup_set_program(actor, 0x53f20UL);
                    actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                    setup_write_u16(actor + 0x30, 0xffffU);
                    actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                    setup_write_u16(actor + 0x32, 0xffffU);
                    (void)dispatch_local_15d00();
                }
            }
            break;
        case -34: /* 0x163bc */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            if ((context[0x5c] & 0x40U) != 0) {
                eng1_sim_state_72108 = 0x0b;
                setup_set_program(actor, 0x547e8UL);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x30, 0xffffU);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
                (void)dispatch_local_15d00();
            } else {
                dispatch_local_16790();
            }
            break;
        case -33: /* 0x16358 */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            flags = setup_read_u16(context + 0x5c);
            value = setup_read_u16(context + 0x70);
            if ((flags & value) != 0) {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                eng1_sim_state_72108 = 0x0e;
                setup_set_program(actor, 0x54078UL);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x30, 0xffffU);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            } else if ((context[0x5c] & 0x40U) == 0) {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                program = *(setup_u8 **)(void *)(actor + 0x64);
                index = setup_s16(setup_read_u16(actor + 0x32));
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                *(setup_u8 **)(void *)(actor + 0x64) = (setup_u8 *)(
                    (setup_u32)program + (setup_u32)(index * 4L + 4L));
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            } else {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            }
            break;
        case -32: /* 0x16333 */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            if ((setup_read_u16(context + 0x5c) & setup_read_u16(context + 0x6c)) == 0) {
                eng1_sim_state_72108 = 0x2e;
                setup_set_program(actor, 0x54470UL);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x30, 0xffffU);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            }
            break;
        case -31: /* 0x162ee: clear collision flag and restart transition. */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            if ((setup_read_u16(context + 0x5c) &
                 setup_read_u16(context + 0x6c)) == 0) {
                collision = SETUP_LIVE_COLLISION();
                flags = setup_read_u32(collision + 0x54) & 0xff7fffffUL;
                setup_write_u32(collision + 0x54, flags);
                g_eng1_setup_word_6ef7c = 0;
                eng1_sim_state_72108 = 0x2e;
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_set_program(actor, 0x541b0UL);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x30, 0xffffU);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            }
            break;
        case -30: /* 0x1625b */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            if ((setup_read_u16(context + 0x5c) & setup_read_u16(context + 0x72)) != 0 &&
                eng1_sim_state_72110 == 0) {
                g_eng1_setup_word_72194 = (setup_u16)(g_eng1_setup_word_72194 + 0x10U);
                if (setup_s16(g_eng1_setup_word_72194) > 0xffL) {
                    if (g_eng1_setup_dword_7216c == 0) {
                        collision = SETUP_LIVE_COLLISION();
                        g_eng1_setup_dword_7216c = 1;
                        play_sound(0x4029U, collision);
                    }
                    g_eng1_setup_word_72194 = 0xffU;
                }
            } else {
                setup_set_program(actor, 0x54018UL);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x30, 0xffffU);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
                collision = SETUP_LIVE_COLLISION();
                eng1_native_code_target_0007f8e(0x15, collision);
            }
            break;
        case -29: /* 0x1644a */
            if (g_eng1_setup_dword_6ef88 == 0) {
                collision = SETUP_LIVE_COLLISION();
                play_sound(setup_read_program(program, index, 2UL, 2UL), collision);
            }
            break;
        case -28: /* 0x1614e: this overwrites the command-local WORD. */
            command = g_eng1_setup_word_7216a;
            break;
        case -27: /* 0x15d9a */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            if ((setup_read_u16(context + 0x6c) & setup_read_u16(context + 0x5c)) != 0) {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x30, 0xffffU);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            } else {
                program = *(setup_u8 **)(void *)(actor + 0x64);
                index = setup_s16(setup_read_u16(actor + 0x32));
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                *(setup_u8 **)(void *)(actor + 0x64) = (setup_u8 *)(
                    (setup_u32)program + (setup_u32)(index * 4L + 4L));
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            }
            break;
        case -26: eng1_native_code_target_001a410(); break;
        case -25: eng1_native_code_target_001a3e0(); break;
        case -24: /* 0x16100: signed coordinate step, DWORD modular arithmetic. */
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            value = actor[0x94] ? 0x00100000UL : 0xfff00000UL;
            collision = SETUP_LIVE_COLLISION();
            setup_write_u32(collision, setup_read_u32(collision) + value);
            break;
        case -23: /* 0x1609d: configure current child from current command. */
            collision = SETUP_LIVE_COLLISION();
            child = *(setup_u8 **)(void *)(collision + 0x60);
            if (child != 0) {
                g_eng1_setup_dword_6efa0 = 2;
                flags = setup_read_u32(collision + 0x54) | 0x800000UL;
                setup_write_u32(collision + 0x54, flags);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                value = setup_read_u32(child + 0x54) & 0xfffffffbUL;
                index = setup_s16(setup_read_u16(actor + 0x32));
                program = *(setup_u8 **)(void *)(actor + 0x64);
                operand = setup_read_program(program, index, 2UL, 2UL);
                setup_write_u32(child + 0x54, value);
                setup_write_u16(child + 0x34, operand);
                collision = SETUP_LIVE_COLLISION();
                flags = collision[0x94] & 0x10U;
                child[0x22] = 1;
                value = (value & 0xffffffefUL) | flags;
                setup_write_u32(child + 0x54, value);
            }
            break;
        case -22: /* 0x1607a */
            collision = SETUP_LIVE_COLLISION();
            operand = setup_actor_operand(collision);
            flags = setup_read_u32(collision + 0x54);
            value = (setup_u32)setup_s16(operand) << 16;
            setup_write_u32(collision + 0x54, flags & ~value);
            break;
        case -21: /* 0x1605a */
            collision = SETUP_LIVE_COLLISION();
            operand = setup_actor_operand(collision);
            flags = setup_read_u32(collision + 0x54);
            setup_write_u32(collision + 0x54, flags & ~setup_sext16(operand));
            break;
        case -20: /* 0x16039 */
            collision = SETUP_LIVE_COLLISION();
            operand = setup_actor_operand(collision);
            flags = setup_read_u32(collision + 0x54);
            value = (setup_u32)setup_s16(operand) << 16;
            setup_write_u32(collision + 0x54, flags | value);
            break;
        case -19: /* 0x16020 */
            collision = SETUP_LIVE_COLLISION();
            operand = setup_actor_operand(collision);
            flags = setup_read_u32(collision + 0x54);
            setup_write_u32(collision + 0x54, flags | setup_sext16(operand));
            break;
        case -17: /* 0x15fff */
            operand = setup_read_program(program, index, 2UL, 2UL);
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            x = setup_read_u32(actor);
            y = setup_read_u32(actor + 4);
            z = setup_read_u32(actor + 8);
            (void)eng1_native_dep_0300c0(x, y, z, operand, 0);
            break;
        case -18: /* 0x15fd9 */
            operand = setup_read_program(program, index, 2UL, 2UL);
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            x = setup_read_u32(actor);
            y = setup_read_u32(actor + 4);
            z = setup_read_u32(actor + 8);
            collision = SETUP_LIVE_COLLISION();
            (void)spawn_actor_with_owner(collision, x, y, z, operand);
            break;
        case -16: /* 0x15fc8 */
            operand = setup_read_program(program, index, 2UL, 2UL);
            g_eng1_setup_word_7183c = operand;
            break;
        case -15: /* 0x15fb3 */
            operand = setup_read_program(program, index, 2UL, 2UL);
            collision = SETUP_LIVE_COLLISION();
            flags = setup_read_u32(collision + 0x50);
            setup_write_u32(collision + 0x50, flags & ~setup_sext16(operand));
            break;
        case -14: /* 0x15fa0 */
            collision = SETUP_LIVE_COLLISION();
            operand = setup_read_program(program, index, 2UL, 2UL);
            flags = setup_read_u32(collision + 0x50);
            setup_write_u32(collision + 0x50, flags | setup_sext16(operand));
            break;
        case -13: /* 0x15f8e */
            collision = SETUP_LIVE_COLLISION();
            operand = setup_read_program(program, index, 2UL, 1UL);
            collision[0x22] = (setup_u8)operand;
            break;
        case -12: /* 0x15d74 */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            if ((context[0x5c] & 0x10U) != 0) {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            } else {
                program = *(setup_u8 **)(void *)(actor + 0x64);
                index = setup_s16(setup_read_u16(actor + 0x32));
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                *(setup_u8 **)(void *)(actor + 0x64) = (setup_u8 *)(
                    (setup_u32)program + (setup_u32)(index * 4L + 4L));
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            }
            break;
        case -11: /* 0x16644: context class controls interface byte +3. */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            context[3] = context[0x0c] == 0 ? 0x22 : 0x21;
            break;
        case -10: /* 0x15f58: callback order and shared dispatch writes. */
            g_dispatch_key_7286c = 0xf8;
            g_dispatch_setting_4d858 = 0x80;
            g_dispatch_word_6ee80 = 0;
            dispatch_engine_2c4e0(1UL);
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            setup_write_u32(context + 0x1b30, 4UL);
            break;
        case -9: /* 0x1665c */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            context[3] = 0x31;
            break;
        case -8: /* 0x15f25: restart the default 0x53820 program. */
            if (program != setup_program_at_rva(0x53830UL))
                setup_write_u32(actor + 0x4c, 2UL);
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            setup_set_program(actor, 0x53820UL);
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            setup_write_u16(actor + 0x32, 0xffffU);
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            setup_write_u16(actor + 0x30, 0xffffU);
            break;
        case -7: /* 0x15e4e: state query, callback and selector sequence. */
            selector = 0;
            if (g_eng1_setup_dword_7201c != 0) {
                setup_set_program(actor, 0x53a10UL);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x30, 0xffffU);
                break;
            }
            value = setup_read_u32(actor + 0x4c);
            if (value != 0) {
                setup_write_u32(actor + 0x4c, value - 1UL);
                selector = 10UL;
            } else {
                collision = SETUP_LIVE_COLLISION();
                x = setup_read_u32(actor);
                y = setup_read_u32(actor + 4);
                z = setup_read_u32(collision + 8);
                value = actor[0x94] ? (x - 0x380000UL) : (x + 0x380000UL);
                flags = y - 0x10000UL;
                selector = eng1_native_code_target_0030010(value, flags, z, 1UL) ? 2UL : 0UL;
                selector += dispatch_engine_2c460(0x14);
            }
            selector &= 0xffffUL;
            if ((short)selector == 0) {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_set_program(actor, 0x538d0UL);
            } else if ((short)selector == 1) {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_set_program(actor, 0x53930UL);
            } else if ((short)selector == 2) {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_set_program(actor, 0x53878UL);
            } else {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_set_program(actor, 0x53830UL);
            }
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            setup_write_u16(actor + 0x32, 0xffffU);
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            setup_write_u16(actor + 0x30, 0xffffU);
            break;
        case -6: /* 0x15e45: toggle the iteration actor's DWORD flags. */
            flags = setup_read_u32(actor + 0x54) ^ 0x10UL;
            setup_write_u32(actor + 0x54, flags);
            break;
        case -5: /* 0x15d68 */
            context = (setup_u8 *)(void *)SETUP_LIVE_INTERFACE_CONTEXT();
            if ((context[0x5c] & 0x40U) != 0) {
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            } else {
                program = *(setup_u8 **)(void *)(actor + 0x64);
                index = setup_s16(setup_read_u16(actor + 0x32));
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                *(setup_u8 **)(void *)(actor + 0x64) = (setup_u8 *)(
                    (setup_u32)program + (setup_u32)(index * 4L + 4L));
                actor = (setup_u8 *)SETUP_LIVE_ACTOR();
                setup_write_u16(actor + 0x32, 0xffffU);
            }
            break;
        case -4: /* 0x15df4: invert normal state, merge into live collision. */
            actor[0x94] = (setup_u8)~actor[0x94];
            collision = SETUP_LIVE_COLLISION();
            flags = setup_read_u32(collision + 0x54) & 0xffffffefUL;
            flags |= collision[0x94] & 0x10U;
            setup_write_u32(collision + 0x54, flags);
            collision = SETUP_LIVE_COLLISION();
            child = *(setup_u8 **)(void *)(collision + 0x60);
            if (child != 0) {
                flags = setup_read_u32(child + 0x54) & 0xffffffefUL;
                flags |= collision[0x94] & 0x10U;
                setup_write_u32(child + 0x54, flags);
            }
            break;
        case -3: /* 0x16685 */
            return 1UL;
        case -2: /* 0x15d84: advance the program by current record + one. */
            index = setup_s16(setup_read_u16(actor + 0x32));
            program = *(setup_u8 **)(void *)(actor + 0x64);
            *(setup_u8 **)(void *)(actor + 0x64) = (setup_u8 *)(
                (setup_u32)program + (setup_u32)(index * 4L + 4L));
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            setup_write_u16(actor + 0x32, 0xffffU);
            break;
        case -1: /* 0x163ad: restart record index only. */
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            setup_write_u16(actor + 0x32, 0xffffU);
            break;
        default:
            /* Native opcodes below -52 reach the common loop tail unchanged. */
            break;
        }

        if (setup_s16(command) < 0) {
            actor = (setup_u8 *)SETUP_LIVE_ACTOR();
            (void)actor;
            continue;
        }
        actor = (setup_u8 *)SETUP_LIVE_ACTOR();
        setup_write_u16(actor + 0x34, command);
        actor = (setup_u8 *)SETUP_LIVE_ACTOR();
        index = setup_s16(setup_read_u16(actor + 0x32));
        program = *(setup_u8 **)(void *)(actor + 0x64);
        operand = setup_read_program(program, index, 2UL, 2UL);
        setup_write_u16(actor + 0x30, operand);
        return 0UL;
    }
}
