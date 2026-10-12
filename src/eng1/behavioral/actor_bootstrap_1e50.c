/* Behavioral reconstruction of ENG1 RVA 0x1e50.
 * The entry is a no-argument actor bootstrap. Existing data and helper owners
 * remain external; unsupported descriptor/callback reads fail visibly. */
#include "../../../calibration/engine_interface.h"
#include "../../../calibration/frame_configuration_row.h"
#include "data_layouts.h"
#include "incomplete.h"
#include "worklist_read_guards.h"

typedef unsigned char eng1_u8;
typedef unsigned short eng1_u16;
typedef unsigned long eng1_u32;

typedef struct Eng1InitDescriptorView24 Eng1InitDescriptorView24;
struct Eng1InitDescriptorView24 {
    eng1_u8 prefix_000_00f[16];
    eng1_u16 x_word_010;
    eng1_u16 y_word_012;
    eng1_u16 z_word_014;
    eng1_u16 actor_kind_016;
};

typedef void (__cdecl *Eng1InitActorCallback)(void *actor);
typedef struct Eng1InitCallbackRow12 Eng1InitCallbackRow12;
struct Eng1InitCallbackRow12 {
    Eng1InitActorCallback slot_000;
    Eng1InitActorCallback slot_004;
    Eng1InitActorCallback slot_008;
};

typedef char eng1_init_descriptor_row_is_24[
    (sizeof(Eng1InitDescriptorView24) == 24) ? 1 : -1];
typedef char eng1_init_callback_row_is_12[
    (sizeof(Eng1InitCallbackRow12) == 12) ? 1 : -1];
typedef char eng1_init_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_init_ulong_is_32[(sizeof(eng1_u32) == 4) ? 1 : -1];

extern void *g_normal_actor_pool;
extern void *g_pipeline_actor_7200c;
extern long g_camera_x_70ca0;
extern long g_camera_y_70ca4;
extern eng1_u32 g_configuration_dword_71370;
extern ConfigurationRow72 g_configuration_rows_68af0[9];
extern Eng1FrameDescriptorRow24 eng1_descriptor_rows_5a690[12];
extern void __cdecl initialize_normal_actor(void *actor);

static void eng1_call_01e50_fail(unsigned long rva, const char *reason)
{
    eng1_incomplete_behavior(rva, reason);
}

static unsigned long eng1_call_01e50_child_rows(int level)
{
    switch (level) {
    case 0: return 2UL;
    case 1: return 8UL;
    case 2: return 0UL;
    case 3: return 10UL;
    case 4: return 2UL;
    case 5: return 8UL;
    case 6: return 2UL;
    case 7: return 2UL;
    case 8: return 0UL;
    case 9: return 12UL;
    case 10: return 0UL;
    case 11: return 1UL;
    default: return 0UL;
    }
}

static int eng1_call_01e50_known_action_base(unsigned long base)
{
    unsigned int row;

    row = 0;
    while (row < 8U) {
        if (g_configuration_rows_68af0[row].dword_01c == base)
            return 1;
        ++row;
    }
    return 0;
}

static Eng1InitActorCallback eng1_call_01e50_action(
    unsigned long table_base, unsigned long index, unsigned long slot_offset,
    const char *site)
{
    Eng1InitCallbackRow12 *row;
    Eng1InitActorCallback callback;

    if (table_base == 0UL || !eng1_call_01e50_known_action_base(table_base))
        eng1_call_01e50_fail(0x71370UL,
            "0x1e50 callback base is not one of the selected source-owned normal-action prefixes");

    eng1_guard_worklist_table_read((const void *)table_base, index, 12UL,
                                   12UL, site);
    row = (Eng1InitCallbackRow12 *)(table_base + index * 12UL);
    if (slot_offset == 0UL)
        callback = row->slot_000;
    else
        callback = row->slot_008;
    if (callback == 0)
        eng1_call_01e50_fail(0x71370UL,
            "0x1e50 selected callback cell is null in the measured source prefix");
    return callback;
}

void __cdecl eng1_call_01e50(void)
{
    eng1_u8 *pool;
    volatile eng1_u8 *actor;
    volatile eng1_u8 *context;
    volatile Eng1InitDescriptorView24 *descriptor;
    Eng1InitActorCallback callback;
    unsigned long child_count;
    unsigned long table_base;
    unsigned long callback_index;
    int level;
    int subindex;
    void *volatile *active_actor = &g_pipeline_actor_7200c;

    pool = (eng1_u8 *)g_normal_actor_pool;
    if (pool == 0)
        eng1_call_01e50_fail(0x72860UL,
            "0x1e50 requires the selected normal actor pool pointer");
    actor = pool + 0x134UL;
    g_pipeline_actor_7200c = (void *)actor;
    initialize_normal_actor((void *)actor);

    /* Native reloads the context pointer after the initializer call. */
    context = (eng1_u8 *)g_engine_interface.context_004;
    if (context == 0)
        eng1_call_01e50_fail(0x70f24UL,
            "0x1e50 context pointer is null after normal actor initialization");
    level = (int)(signed char)context[0];
    subindex = (int)(signed char)context[1];

    if (level < 0 || level >= 12)
        eng1_call_01e50_fail(0x5a69cUL,
            "0x1e50 signed context level lies outside the owned 12-row descriptor prefix");
    child_count = eng1_call_01e50_child_rows(level);
    if (child_count == 0UL ||
        eng1_descriptor_rows_5a690[level].pointer_00c == 0)
        eng1_call_01e50_fail(0x5a69cUL,
            "0x1e50 selected descriptor root has no source-owned child-row prefix");
    if (subindex < 0 || (unsigned long)subindex >= child_count)
        eng1_call_01e50_fail(0x5a69cUL,
            "0x1e50 signed context subindex lies outside the measured 24-byte child-row prefix");

    descriptor = (volatile Eng1InitDescriptorView24 *)
        ((eng1_u8 *)eng1_descriptor_rows_5a690[level].pointer_00c +
         (unsigned long)subindex * 24UL);
    if (descriptor->actor_kind_016 == 0U)
        return;

    /* Preserve the native live-pointer reload before each actor field store. */
    actor = (volatile eng1_u8 *)*active_actor;
    if (actor == 0)
        eng1_call_01e50_fail(0x7200cUL,
            "0x1e50 active actor pointer is null before coordinate stores");
    *(volatile eng1_u32 *)(void *)(actor + 0x00) =
        (eng1_u32)descriptor->x_word_010 << 16;

    actor = (volatile eng1_u8 *)*active_actor;
    if (actor == 0)
        eng1_call_01e50_fail(0x7200cUL,
            "0x1e50 active actor pointer is null before coordinate stores");
    *(volatile eng1_u32 *)(void *)(actor + 0x04) =
        (eng1_u32)descriptor->y_word_012 << 16;

    actor = (volatile eng1_u8 *)*active_actor;
    if (actor == 0)
        eng1_call_01e50_fail(0x7200cUL,
            "0x1e50 active actor pointer is null before coordinate stores");
    *(volatile eng1_u32 *)(void *)(actor + 0x08) =
        (eng1_u32)descriptor->z_word_014 << 16;

    g_camera_x_70ca0 = (long)(eng1_u32)descriptor->x_word_010;
    g_camera_y_70ca4 = (long)(eng1_u32)descriptor->y_word_012;

    /* Native rereads the kind after coordinate/camera stores. */
    {
        eng1_u16 kind = descriptor->actor_kind_016;
        actor = (volatile eng1_u8 *)*active_actor;
        if (actor == 0)
            eng1_call_01e50_fail(0x7200cUL,
                "0x1e50 active actor pointer is null before kind store");
        *(volatile eng1_u16 *)(void *)(actor + 0x2e) = kind;
    }

    actor = (volatile eng1_u8 *)*active_actor;
    if (actor == 0)
        eng1_call_01e50_fail(0x7200cUL,
            "0x1e50 active actor pointer is null before category store");
    actor[0x22] = 1U;

    /* Native masks the freshly read WORD kind and reloads the base for each
     * callback; callback zero may replace both shared cells. */
    actor = (volatile eng1_u8 *)*active_actor;
    if (actor == 0)
        eng1_call_01e50_fail(0x7200cUL,
            "0x1e50 active actor pointer is null before the first callback");
    callback_index = (*(volatile eng1_u16 *)(void *)(actor + 0x2e)) & 0x03ffUL;
    table_base = *(volatile eng1_u32 *)&g_configuration_dword_71370;
    callback = eng1_call_01e50_action(table_base, callback_index, 0UL,
                                      "0x1e50 callback slot 0");
    callback((void *)actor);

    actor = (volatile eng1_u8 *)*active_actor;
    if (actor == 0)
        eng1_call_01e50_fail(0x7200cUL,
            "0x1e50 active actor pointer is null before the second callback");
    callback_index = (*(volatile eng1_u16 *)(void *)(actor + 0x2e)) & 0x03ffUL;
    table_base = *(volatile eng1_u32 *)&g_configuration_dword_71370;
    callback = eng1_call_01e50_action(table_base, callback_index, 8UL,
                                      "0x1e50 callback slot 2");
    callback((void *)actor);
}
