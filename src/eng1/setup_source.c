/* Behavioral source reconstruction for ENG1.DLL RVA 0x15090 and its immediate
 * state-gated callee 0x16790.  The native body is ordinary x86 C-shaped
 * setup, but its source name and original translation-unit identity are not
 * inferred.  This unit owns no persistent storage. */
#include <stddef.h>
#include "../shared/engine_interface.h"
#include "setup_shared_state.h"
#include "script_54558_owner.h"
#include "data_layouts.h"
#include "simulation_data_owners.h"
#include "incomplete.h"

typedef unsigned char eng1_setup_u8;
typedef unsigned short eng1_setup_u16;

typedef struct Eng1SetupDescriptorFields24 {
    eng1_setup_u16 word_000;
    eng1_setup_u16 word_002;
    eng1_setup_u8 byte_004;
    eng1_setup_u8 unobserved_005[19];
} Eng1SetupDescriptorFields24;

typedef char eng1_setup_descriptor_row_is_24[
    (sizeof(Eng1SetupDescriptorFields24) == 24) ? 1 : -1];
typedef char eng1_setup_long_is_32[(sizeof(unsigned long) == 4) ? 1 : -1];
typedef char eng1_setup_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];

#define ENG1_READ_NORMAL_POOL() \
    (*(void * volatile *)(void *)&g_normal_actor_pool)
#define ENG1_READ_CONTEXT() \
    (*(ResourceCallbackContext * volatile *)(void *)&g_engine_interface.context_004)
#define ENG1_READ_COLLISION_CONTEXT() \
    (*(eng1_setup_u8 * volatile *)(void *)&g_collision_context)

/* The native body rereads this global pointer between several adjacent
 * stores.  Volatile qualifies this translation-unit view so those live loads
 * are preserved by the C compiler. */
extern void *g_normal_actor_pool;        /* native 0x72860 */
extern void *g_middle_actor_pool;                  /* native 0x73494 */
extern void __cdecl initialize_middle_actor(void *actor);
extern void __cdecl initialize_normal_actor(void *actor);
extern void *__cdecl attach_actor(void *actor, int kind); /* native 0x27cd0 */

extern void __cdecl dispatch_local_11fc0(void);
extern void __cdecl dispatch_local_15670(void);
extern void __cdecl dispatch_local_16790(void);
extern unsigned long __cdecl eng1_native_code_target_0015d00(void);
extern void __cdecl eng1_native_code_target_0018f60(void);
extern void __cdecl eng1_native_code_target_0019160(void);
extern void __cdecl eng1_native_code_target_002a290(void *actor);
extern void *__cdecl eng1_native_code_target_0027d40(
    void *actor, unsigned long action);

static ENG1_NORETURN void __cdecl eng1_candidate_missing(
    unsigned long rva, const char *reason)
{
    eng1_incomplete_behavior(rva, reason);
}

/* Missing code is supplied only by the centralized transitional manifest. */
extern void __cdecl eng1_native_code_target_00374c0(void);

/* Physical no-argument handler installed at table cell 10.  DWORD decrement
 * wraps as native x86 arithmetic; the default setup writes actor+0x108=2, so
 * this path leaves it at 1 and does not reenter 0x16790. */
void __cdecl dispatch_local_19440(void)
{
    --*(unsigned long *)(void *)((eng1_setup_u8 *)g_normal_actor_pool + 0x108);
    if (*(unsigned long *)(void *)((eng1_setup_u8 *)g_normal_actor_pool + 0x108) == 0)
        dispatch_local_16790();
    eng1_native_code_target_0015d00();
    eng1_native_code_target_002a290(g_collision_context);
}

/* Native 0x16790's default branch uses the source-owned interior view at
 * 0x53820. Its alternate branch uses the separately measured command stream
 * at 0x54558. */
void __cdecl dispatch_local_16790(void)
{
    eng1_setup_u8 *actor;

    if (eng1_sim_state_72170 != 0)
    {
        eng1_native_code_target_0019160();
        return;
    }

    if (eng1_sim_state_72110 != 0) {
        if (g_eng1_setup_dword_7201c == 0) {
            eng1_native_code_target_0018f60();
            return;
        }
        actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
        eng1_sim_state_72108 = 0x2f;
        *(void **)(void *)(actor + 0x64) = g_eng1_setup_program_54558;
        actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
        *(eng1_setup_u16 *)(void *)(actor + 0x30) = 0xffffU;
        actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
        *(eng1_setup_u16 *)(void *)(actor + 0x32) = 0xffffU;
        eng1_native_code_target_0015d00();
        return;
    }

    actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
    eng1_sim_state_72108 = 0;
    *(void **)(void *)(actor + 0x64) =
        (void *)((eng1_setup_u8 *)&g_eng1_simulation_lookup_owner_537e0 + 0x40);
    actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
    *(eng1_setup_u16 *)(void *)(actor + 0x30) = 0xffffU;
    actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
    *(eng1_setup_u16 *)(void *)(actor + 0x32) = 0xffffU;
    eng1_native_code_target_0015d00();
    actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
    *(unsigned long *)(void *)(actor + 0x4c) = 2;
    actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
    actor[0x96] = 0xff;
}

void __cdecl dispatch_local_15090(void)
{
    ResourceCallbackContext *context;
    eng1_setup_u8 *actor_at_entry;
    eng1_setup_u8 *actor;
    Eng1SetupDescriptorFields24 *descriptor;
    void *descriptor_root;
    unsigned long flags;
    unsigned long row_count;
    int level;
    int subindex;
    unsigned long slot;

    context = ENG1_READ_CONTEXT();
    actor_at_entry = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();

#include "setup_handler_assignments.inc"

    *(eng1_setup_u16 *)(void *)((eng1_setup_u8 *)context + 0x22) = 0;
    context = ENG1_READ_CONTEXT();
    *(eng1_setup_u16 *)(void *)((eng1_setup_u8 *)context + 0x24) = 0x80;
    context = ENG1_READ_CONTEXT();
    flags = *(unsigned long *)(void *)((eng1_setup_u8 *)context + 0x30);
    *(eng1_setup_u16 *)(void *)((eng1_setup_u8 *)context + 0x26) =
        (flags & 0x200UL) ? 0xffU : 0U;
    context = ENG1_READ_CONTEXT();
    flags = *(unsigned long *)(void *)((eng1_setup_u8 *)context + 0x30);
    *(eng1_setup_u16 *)(void *)((eng1_setup_u8 *)context + 0x28) =
        (flags & 0x400UL) ? 0xffU : 0U;
    context = ENG1_READ_CONTEXT();
    flags = *(unsigned long *)(void *)((eng1_setup_u8 *)context + 0x30);
    *(eng1_setup_u16 *)(void *)((eng1_setup_u8 *)context + 0x2a) =
        (flags & 0x800UL) ? 0xffU : 0U;
    context = ENG1_READ_CONTEXT();
    flags = *(unsigned long *)(void *)((eng1_setup_u8 *)context + 0x30);
    *(eng1_setup_u16 *)(void *)((eng1_setup_u8 *)context + 0x2c) =
        (flags & 0x1000UL) ? 0xffU : 0U;

    context = ENG1_READ_CONTEXT();
    if (((eng1_setup_u8 *)context)[0] == 9)
        *(eng1_setup_u16 *)(void *)((eng1_setup_u8 *)context + 0x2a) = 0;
    context = ENG1_READ_CONTEXT();
    if (((eng1_setup_u8 *)context)[0] == 6 ||
        ((eng1_setup_u8 *)context)[0] == 7) {
        *(eng1_setup_u16 *)(void *)((eng1_setup_u8 *)context + 0x26) = 0;
        context = ENG1_READ_CONTEXT();
        *(eng1_setup_u16 *)(void *)((eng1_setup_u8 *)context + 0x28) = 0;
        context = ENG1_READ_CONTEXT();
        *(eng1_setup_u16 *)(void *)((eng1_setup_u8 *)context + 0x2a) = 0;
    }

    g_eng1_setup_dword_72174 = 0;
    g_eng1_setup_dword_72104 = 0;
    g_eng1_setup_byte_72010 = 0;
    dispatch_local_11fc0();

    g_eng1_setup_pointer_72020 = (eng1_setup_u8 *)g_middle_actor_pool;
    g_eng1_setup_pointer_72018 = g_eng1_setup_pointer_72020 + 0xec;
    g_eng1_setup_pointer_72164 = g_eng1_setup_pointer_72020 + 0x1d8;
    initialize_middle_actor(g_eng1_setup_pointer_72020);
    initialize_middle_actor(g_eng1_setup_pointer_72018);
    initialize_middle_actor(g_eng1_setup_pointer_72164);

    context = ENG1_READ_CONTEXT();
    eng1_sim_state_72110 = 0;
    g_eng1_setup_dword_72014 = 0;
    eng1_sim_state_6feb4 = 0;
    g_eng1_setup_byte_7215c = 0;
    if (((eng1_setup_u8 *)context)[0] == 9) {
        eng1_native_code_target_00374c0();
        return;
    }

    g_eng1_setup_dword_72118 = 0;
    g_eng1_setup_dword_72120 = 0;
    actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
    actor[0xec] = 0;
    g_eng1_setup_word_6ef74 = 0;
    g_eng1_setup_word_6ef78 = 0;
    g_eng1_setup_dword_7211c = 0;
    g_eng1_setup_word_6ef7c = 0;
    g_eng1_setup_word_6ef80 = 0;
    g_eng1_setup_dword_6ef88 = 0;
    g_eng1_setup_word_6ef8c = 0;
    g_eng1_setup_dword_717b4 = 0;
    g_eng1_setup_word_6ef90 = 0;
    g_eng1_setup_word_6ef94 = 0;
    g_eng1_setup_dword_6efa0 = 0;
    actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
    *(unsigned long *)(void *)(actor + 0x4c) = 0;
    context = ENG1_READ_CONTEXT();
    g_eng1_setup_dword_7216c = 0;
    g_eng1_setup_dword_6efac = 0;
    g_resource_transition_ticks_6efb0 = 0;
    g_eng1_setup_byte_6efb4 = 0;
    ((eng1_setup_u8 *)context)[5] = 0;
    context = ENG1_READ_CONTEXT();
    ((eng1_setup_u8 *)context)[4] = 0;
    actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
    g_eng1_setup_word_72158 = 0;
    g_eng1_setup_dword_72114 = 0;
    g_eng1_setup_dword_6efa8 = 0;
    g_eng1_setup_dword_72008 = 1;
    g_eng1_setup_dword_7201c = 0;
    g_eng1_setup_word_6ef98 = 0;
    g_eng1_setup_word_6ef9c = 0;
    eng1_sim_state_72170 = 0;
    g_eng1_setup_byte_72011 = 0;
    g_collision_context = actor;
    initialize_normal_actor(actor);
    dispatch_local_15670();

    context = ENG1_READ_CONTEXT();
    level = (int)(signed char)((eng1_setup_u8 *)context)[0];
    subindex = (int)(signed char)((eng1_setup_u8 *)context)[1];
    if (level < 0 || level > 11)
        eng1_candidate_missing(0x5a69cUL,
            "signed level root exceeds the measured 12-row descriptor prefix");
    if (subindex < 0)
        eng1_candidate_missing(0x5a69cUL,
            "negative signed descriptor subindex precedes the measured prefix");

    descriptor_root = eng1_descriptor_rows_5a690[level].pointer_00c;
    switch (level) {
    case 0: row_count = 2; break;
    case 1: row_count = 8; break;
    case 2: row_count = 0; break;
    case 3: row_count = 10; break;
    case 4: row_count = 2; break;
    case 5: row_count = 8; break;
    case 6: row_count = 2; break;
    case 7: row_count = 2; break;
    case 8: row_count = 0; break;
    case 9: row_count = 12; break;
    case 10: row_count = 0; break;
    case 11: row_count = 1; break;
    default: row_count = 0; break;
    }
    if (descriptor_root == 0 || row_count == 0)
        eng1_candidate_missing(0x5a69cUL,
            "selected descriptor root has no source-owned child prefix");
    if ((unsigned long)subindex >= row_count)
        eng1_candidate_missing(0x5a69cUL,
            "descriptor subindex exceeds the owned child-row prefix");
    descriptor = (Eng1SetupDescriptorFields24 *)
        ((eng1_setup_u8 *)descriptor_root + (unsigned long)subindex * 24UL);

    *(unsigned long *)(void *)(actor_at_entry + 0x00) =
        ((unsigned long)descriptor->word_000 - 0x20UL) << 16;
    *(unsigned long *)(void *)(actor_at_entry + 0x04) =
        ((unsigned long)descriptor->word_002 + 0x40UL) << 16;
    *(unsigned long *)(void *)(actor_at_entry + 0x08) =
        (unsigned long)descriptor->byte_004 << 23;

    ((unsigned char *)(void *)&g_camera_flags_70d10)[0] |= 0x40U;
    g_camera_depth_low_70d0c = 0x20;
    g_camera_depth_high_70d0e = 0xd8;
    g_camera_x_70ca0 = descriptor->word_000;
    g_camera_y_70ca4 = descriptor->word_002;
    g_camera_z_70ca8 = ((long)descriptor->byte_004 << 7) - 0x140L;
    g_eng1_setup_byte_70cfe = 0;
    g_camera_adjust_x_70d02 = 0x3c;
    g_camera_adjust_y_70d04 = (short)0xffc0U;
    g_camera_center_x_70cdc = 0xa0;
    g_camera_center_y_70ce0 = 0x80;
    g_camera_focal_70ce4 = 0x140;
    g_camera_z_base_70d1c = 0;
    g_camera_iterations_70cfd = 2;
    g_camera_flags_70d10 = 0x5e;
    g_eng1_setup_word_718e0 = 0;
    context = ENG1_READ_CONTEXT();
    if (((eng1_setup_u8 *)context)[0] == 3)
        g_eng1_setup_word_718e0 = 0x2e1;

    actor = (eng1_setup_u8 *)ENG1_READ_COLLISION_CONTEXT();
    *(eng1_setup_u16 *)(void *)(actor + 0x2e) = 1;
    actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
    actor[0x22] = 1;
    dispatch_local_16790();

    actor = (eng1_setup_u8 *)ENG1_READ_NORMAL_POOL();
    eng1_sim_state_72108 = 10;
    *(unsigned long *)(void *)(actor + 0x108) = 2;
    context = ENG1_READ_CONTEXT();
    level = (int)(signed char)((eng1_setup_u8 *)context)[0];
    if (level < 0 || level > 8)
        eng1_candidate_missing(0x55278UL,
            "signed category index exceeds the measured nine-row prefix");
    ((eng1_setup_u8 *)ENG1_READ_COLLISION_CONTEXT())[0x1c] =
        g_eng1_setup_categories_55278[level][0];

    slot = (unsigned long)(eng1_sim_state_72108 & 0x7fffU);
    g_eng1_setup_handlers_72040[slot]();

    actor = (eng1_setup_u8 *)ENG1_READ_COLLISION_CONTEXT();
    g_eng1_setup_dword_72180 = *(unsigned long *)(void *)(actor + 0xac);
    *(unsigned long *)(void *)(actor + 0x54) |= 0x00000600UL;
    actor = (eng1_setup_u8 *)ENG1_READ_COLLISION_CONTEXT();
    *(eng1_setup_u16 *)(void *)(actor + 0x4a) = 0xe0;
    g_eng1_setup_word_7183c = 0;
    g_eng1_setup_word_71a12 = 0;
    g_eng1_setup_word_71a14 = 0;
    g_eng1_setup_word_71848 = 0;
    g_eng1_setup_word_7216a = 1;
    (void)attach_actor(ENG1_READ_COLLISION_CONTEXT(), 0x2024);

    actor = (eng1_setup_u8 *)eng1_native_code_target_0027d40(
        ENG1_READ_COLLISION_CONTEXT(), 0xa007UL);
    if (actor != 0) {
        *(unsigned long *)(void *)(actor + 0x54) |= 0x04000605UL;
        *(eng1_setup_u16 *)(void *)(actor + 0x4a) = 0x80;
        *(eng1_setup_u16 *)(void *)(actor + 0x34) = 1;
        actor = (eng1_setup_u8 *)ENG1_READ_COLLISION_CONTEXT();
        *(unsigned long *)(void *)(actor + 0x54) &= 0xff7fffffUL;
    }
}
