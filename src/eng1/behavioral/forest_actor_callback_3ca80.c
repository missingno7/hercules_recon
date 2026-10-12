/* Private ordinary-C reconstruction of ENG1.DLL RVA 0x3ca80.
 * Actor layout and touched offsets reuse the measured Actor view. The list at
 * actor+0x120 is runtime-produced by the separate 0x2b130 dependency; this
 * view does not claim an original declaration or a fixed allocation extent. */
#include <stdio.h>
#include "interpreters.h"
#include "actor_control_tables.h"
#include "actor_control_update.h"
#include "read_guard.h"
#include "incomplete.h"

typedef unsigned char call3ca80_u8;
typedef unsigned short call3ca80_u16;
typedef unsigned long call3ca80_u32;

typedef struct Eng1Call3ca80ActorListView Eng1Call3ca80ActorListView;
struct Eng1Call3ca80ActorListView {
    volatile long count;
    Actor *items[1];
};

typedef char call3ca80_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char call3ca80_long_is_32[(sizeof(long) == 4) ? 1 : -1];
typedef char call3ca80_actor_is_0x134[(sizeof(Actor) == 0x134) ? 1 : -1];
typedef void (__cdecl *Eng1Call3ca80Target)(Actor *);
typedef char call3ca80_callback_pointer_is_32[
    sizeof(Eng1Call3ca80Target) == 4 ? 1 : -1];

/* Both routines are real native dependencies, not successful local stubs.
 * 0x2b130 owns and writes actor+0x120 before returning nonzero. */
extern int __cdecl eng1_native_code_target_002b130(Actor *actor);
extern void __cdecl eng1_native_code_target_0041000(
    Actor *actor, call3ca80_u32 category);

static volatile call3ca80_u8 *eng1_call_3ca80_actor_byte(
    Actor *actor, unsigned int byte_offset)
{
    return (volatile call3ca80_u8 *)actor + byte_offset;
}

static volatile call3ca80_u32 *eng1_call_3ca80_actor_dword(
    Actor *actor, unsigned int byte_offset)
{
    return (volatile call3ca80_u32 *)(void *)
        ((volatile call3ca80_u8 *)actor + byte_offset);
}

static call3ca80_u16 eng1_call_3ca80_classifier_word(
    call3ca80_u8 category)
{
    call3ca80_u32 requested_rva;
    char reason[160];
    volatile call3ca80_u16 *row;

    requested_rva = 0x6567aUL + (call3ca80_u32)category * 12UL;
    if (category >= 18U) {
        sprintf(reason,
                "3ca80 classifier row at RVA 0x6567a: class byte %u requests RVA 0x%08lx outside the measured 18-row 0x65678 owner",
                (unsigned int)category, requested_rva);
        eng1_incomplete_behavior(0x6567aUL, reason);
        return 0;
    }

    row = (volatile call3ca80_u16 *)(void *)
        &g_eng1_classifier_rows_65678[0][1];
    return row[(unsigned int)category * 6U];
}

static Eng1Call3ca80ActorListView *eng1_call_3ca80_actor_list(
    Actor *actor)
{
    Eng1Call3ca80ActorListView *list;

    list = *(Eng1Call3ca80ActorListView * volatile *)(void *)
        ((volatile call3ca80_u8 *)actor + 0x120U);
    if (list == 0) {
        eng1_incomplete_behavior(0x120UL,
            "3ca80 2b130 returned nonzero without an actor list at actor+0x120");
        return 0;
    }
    return list;
}

/* RVA 0x3ca80. The function-pointer record is called with one Actor*.
 * Fixed-point subtraction is performed through unsigned bits to preserve the
 * x86 low-DWORD wrap behavior without signed-overflow expressions in C. */
void __cdecl eng1_native_code_target_03ca80(Actor *actor)
{
    Eng1Call3ca80ActorListView *list;
    volatile call3ca80_u32 *cursor;
    Actor *child;
    call3ca80_u16 row_bits;
    call3ca80_u8 class_byte;
    call3ca80_u32 index;
    call3ca80_u32 list_count;

    actor->x = (long)((call3ca80_u32)actor->x - 0x00020000UL);
    *eng1_call_3ca80_actor_dword(actor, 0xd8U) =
        (call3ca80_u32)actor->y - 0x00800000UL;
    *eng1_call_3ca80_actor_dword(actor, 0xdcU) = 0x40UL;
    *eng1_call_3ca80_actor_dword(actor, 0xe0U) = 0xa0UL;

    *eng1_call_3ca80_actor_byte(actor, 0x21U) =
        eng1_native_code_target_0041fd0((call3ca80_u32)actor->x);
    if (eng1_native_code_target_002b130(actor) == 0) {
        (void)remove_actor(actor);
        return;
    }

    /* Native reloads the separate byte at actor+0x21 after 2b130, then reads
     * this row WORD once. The declared Actor.category member is at +0x22. */
    class_byte = *eng1_call_3ca80_actor_byte(actor, 0x21U);
    row_bits = eng1_call_3ca80_classifier_word(class_byte);
    list = eng1_call_3ca80_actor_list(actor);
    list_count = (call3ca80_u32)list->count;
    if ((long)list_count <= 0L)
        return;

    cursor = (volatile call3ca80_u32 *)(void *)list;
    index = 0UL;
    for (;;) {
        child = (Actor *)(unsigned long)cursor[1];
        if (child == 0) {
            eng1_incomplete_behavior(0x120UL,
                "3ca80 actor list contains a null Actor* before its live count");
            return;
        }
        ++cursor;
        ++index;

        if (child->kind == 0x2fU) {
            *eng1_call_3ca80_actor_dword(child, 0xd8U) =
                (call3ca80_u32)child->y;
            *eng1_call_3ca80_actor_dword(child, 0xdcU) =
                *eng1_call_3ca80_actor_dword(actor, 0xdcU);
            *eng1_call_3ca80_actor_dword(child, 0xe0U) =
                *eng1_call_3ca80_actor_dword(actor, 0xe0U);
            eng1_native_code_target_0041000(child,
                (call3ca80_u32)*eng1_call_3ca80_actor_byte(actor, 0x21U));
        }

        /* The original reloads kind after 0x41000, which may mutate child.
         * Only kind 0x30 consumes the next low bit from the cached row WORD. */
        if (child->kind == 0x30U) {
            if ((row_bits & 1U) != 0U) {
                child->flags &= 0xfffffdffUL;
                eng1_guarded_select_frame(child, 12);
                child->unknown_02a = 9;
            }
            row_bits = (call3ca80_u16)(row_bits >> 1);
        }

        /* Native reloads the count at the backedge after each item. */
        if ((long)index >= (long)list->count)
            break;
    }
}
