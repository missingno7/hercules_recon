/* Behavioral ENG1 cold-frame callees reconstructed from native instruction
 * and data flow. Host rendering remains an interface callback effect. */
#include "../../../calibration/engine_interface.h"
#include "../../../calibration/frame_lifecycle_bridges.h"
#include "incomplete.h"
#include "frame_program_data.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef long s32;

extern u32 g_frame_global_73484;
extern u8 *g_dispatch_frame_current_71ec4;
extern int g_context_sequence_phase;
extern void __cdecl dispatch_local_33620(s32, s32, s32, s32, s32, s32, s32);

typedef void (__cdecl *FrameRecordCallback)(void *, const void *, u32, u32);

typedef char frame_callees_require_32bit_ulong[(sizeof(u32) == 4) ? 1 : -1];

typedef struct MeasuredScriptPrefix {
    const u8 *base;
    u32 length;
} MeasuredScriptPrefix;

/* Source-derived bounds for the exact string bases linked below. This is
 * candidate metadata, not an additional native field or a slot-index bound. */
static const MeasuredScriptPrefix g_measured_script_prefixes[] = {
    { g_eng1_dialog_script_0, sizeof(g_eng1_dialog_script_0) },
    { g_eng1_dialog_script_1, sizeof(g_eng1_dialog_script_1) },
    { g_eng1_dialog_script_2, sizeof(g_eng1_dialog_script_2) },
    { g_eng1_dialog_script_3, sizeof(g_eng1_dialog_script_3) },
    { g_eng1_dialog_script_4, sizeof(g_eng1_dialog_script_4) },
    { g_eng1_dialog_script_5, sizeof(g_eng1_dialog_script_5) },
    { g_eng1_dialog_script_6, sizeof(g_eng1_dialog_script_6) },
    { g_eng1_dialog_script_7, sizeof(g_eng1_dialog_script_7) },
    { g_eng1_dialog_script_8, sizeof(g_eng1_dialog_script_8) },
    { g_eng1_dialog_script_9, sizeof(g_eng1_dialog_script_9) },
    { g_eng1_dialog_script_10, sizeof(g_eng1_dialog_script_10) },
    { g_eng1_dialog_script_11, sizeof(g_eng1_dialog_script_11) },
    { g_eng1_dialog_script_12, sizeof(g_eng1_dialog_script_12) },
    { g_eng1_dialog_script_13, sizeof(g_eng1_dialog_script_13) },
    { g_eng1_dialog_script_14, sizeof(g_eng1_dialog_script_14) },
};

static void write_u32(u8 *where, u32 value)
{
    *(u32 *)(void *)where = value;
}

static u32 read_u32(const u8 *where)
{
    return *(const u32 *)(const void *)where;
}

static s32 add_x_wrap(s32 x, u32 amount)
{
    return (s32)((u32)x + amount);
}

static u32 measured_script_prefix_length(const u8 *script)
{
    u32 index;

    for (index = 0;
         index < sizeof(g_measured_script_prefixes) /
                 sizeof(g_measured_script_prefixes[0]);
         ++index) {
        if (script == g_measured_script_prefixes[index].base)
            return g_measured_script_prefixes[index].length;
    }

    /* Other runtime pointers retain the native NUL-terminated read-through
     * behavior; their length must never be borrowed from the table slot. */
    return 0UL;
}

void __cdecl dispatch_local_2c9b0(void *record, const void *rectangle,
                                  u32 mode, u32 key)
{
    FrameRecordCallback callback;

    callback = *(FrameRecordCallback *)(void *)
        ((u8 *)&g_engine_interface + 0x2b0);
    callback(record, rectangle, mode, key);
}

void __cdecl dispatch_local_2c60(const u16 rectangle[4], u32 mode, u32 key)
{
    u8 *record;
    u8 *frame;

    /* The native body captures ESI before calling the host callback. */
    record = (u8 *)(unsigned long)g_frame_global_73484;
    dispatch_local_2c9b0(record, rectangle, mode, key);

    /* These are two separate reads of the live current-frame global, as in
     * 0x2c84 and 0x2c8c. The target callback runs before either read. */
    frame = *(u8 * volatile *)(void *)&g_dispatch_frame_current_71ec4;
    write_u32(record, read_u32(frame + 0x88));
    frame = *(u8 * volatile *)(void *)&g_dispatch_frame_current_71ec4;
    write_u32(frame + 0x88, (u32)(unsigned long)record);

    /* Native 0x2c98 advances the captured record pointer, overwriting any
     * callback change to the global cursor. */
    g_frame_global_73484 = (u32)(unsigned long)(record + 0x18);
}

void __cdecl dispatch_local_24d0(s32 x, s32 y, s32 operation0,
                                  s32 operation1, s32 operation2,
                                  s32 height, s32 mode)
{
    dispatch_local_33620(x, y, operation0, operation1, operation2,
                         height, mode);
}

void __cdecl dispatch_local_2450(s32 x, s32 y, u32 script_id, s32 mode)
{
    const u8 *script;
    u32 script_length;
    u32 cursor;
    s32 draw_x;

    if (script_id >= 15UL) {
        eng1_incomplete_behavior(0x2450UL,
            "dialog script id exceeds measured pointer-table prefix");
        return;
    }

    script = g_eng1_dialog_scripts_4d720[script_id];
    script_length = measured_script_prefix_length(script);
    draw_x = x;
    for (cursor = 0; ; ++cursor) {
        s32 command;
        u32 operation_index;
        const u8 *operation;

        if (script_length != 0UL && cursor >= script_length) {
            eng1_incomplete_behavior(0x2450UL,
                "known dialog string base has no NUL within its measured prefix");
            return;
        }
        command = (s32)(signed char)script[cursor];
        if (command == 0)
            return;
        operation_index = ((u32)command - 1UL) & 0x1fUL;
        if (operation_index == 0x1fUL) {
            draw_x = add_x_wrap(draw_x, 0x10UL);
            continue;
        }

        operation = g_eng1_dialog_operations_4d760 + operation_index * 3UL;
        dispatch_local_24d0(draw_x, y, operation[0], operation[1],
                            operation[2], 0x12, mode);
        draw_x = add_x_wrap(draw_x, operation[2]);
    }
}

void __cdecl dispatch_local_1f40(void)
{
    ResourceCallbackContext *context;
    u8 text_state;
    u8 table_index;

    if (g_context_sequence_phase == 2) {
        context = g_engine_interface.context_004;
        if ((*((u8 *)(void *)context + 0x38) & 0x10U) != 0)
            dispatch_local_2450(0x55, 0x70, 14UL, 3);
    }

    /* 0x1f68 reloads the current interface context after any script callback. */
    context = g_engine_interface.context_004;
    text_state = *((u8 *)(void *)context + 0x04);
    if (text_state == 0)
        return;
    if (text_state < 2U || text_state > 128U)
        return;

    table_index = g_eng1_dialog_state_map_2100[text_state - 2U];
    switch (table_index) {
    case 0:
        if (text_state == 2U) {
            dispatch_local_2450(0x72, 0x50, 0UL, 3);
            dispatch_local_2450(0x62, 0x78, 1UL, 5);
            dispatch_local_2450(0x83, 0x8c, 2UL, 4);
        } else {
            dispatch_local_2450(0x72, 0x50, 0UL, 3);
            dispatch_local_2450(0x62, 0x78, 1UL, 4);
            dispatch_local_2450(0x83, 0x8c, 2UL, 5);
        }
        return;
    case 1:
        if (text_state == 4U) {
            dispatch_local_2450(0x3f, 0x50, 3UL, 3);
            dispatch_local_2450(0x8d, 0x78, 4UL, 5);
            dispatch_local_2450(0x8b, 0x8c, 5UL, 4);
        } else {
            dispatch_local_2450(0x3f, 0x50, 3UL, 3);
            dispatch_local_2450(0x8d, 0x78, 4UL, 4);
            dispatch_local_2450(0x8b, 0x8c, 5UL, 5);
        }
        return;
    case 2:
        dispatch_local_2450(0x32, 0x64, 6UL, 3);
        dispatch_local_2450(0x2d, 0x78, 7UL, 3);
        dispatch_local_2450(0x20, 0x8c, 8UL, 3);
        return;
    case 3:
        dispatch_local_2450(0x2f, 0x64, 9UL, 3);
        dispatch_local_2450(0x77, 0x78, 10UL, 3);
        /* 0x20a3 reloads the context after the preceding callback-capable
         * script calls before reading its WORD at +0xdc. */
        context = g_engine_interface.context_004;
        if (*(u16 *)(void *)((u8 *)context + 0xdc) == 0x10U)
            dispatch_local_2450(0x2d, 0x94, 12UL, 3);
        else
            dispatch_local_2450(0x3b, 0x94, 11UL, 3);
        return;
    case 4:
        dispatch_local_2450(0x72, 0x50, 0UL, 3);
        dispatch_local_2450(0x62, 0x78, 1UL, 4);
        dispatch_local_2450(0x83, 0x8c, 2UL, 5);
        return;
    case 5:
        if ((*((u8 *)(void *)context + 0x38) & 0x08U) != 0)
            dispatch_local_2450(0x67, 0x78, 13UL, 3);
        return;
    case 6:
        return;
    default:
        eng1_incomplete_behavior(0x1f40UL,
            "dialog state selects an unsupported measured-map index");
        return;
    }
}
