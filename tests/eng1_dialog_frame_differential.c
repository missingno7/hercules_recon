#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <windows.h>
#include "../calibration/engine_interface.h"
#include "../src/eng1/behavioral/cold_start_frame_data.h"
#include "../src/eng1/behavioral/frame_program_data.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef long s32;
typedef u32 (__cdecl *PackCallback)(u32, u32);
typedef void (__cdecl *Native2c60)(const u16 *, u32, u32);
typedef void (__cdecl *Native1f40)(void);
typedef void (__cdecl *Native2450)(s32, s32, u32, s32);
typedef void (__cdecl *Native2c980)(void *, u32, u32, u32, u32);
typedef void (__cdecl *Native2c9b0)(void *, const void *, u32, u32);
typedef void (__cdecl *FrameRecordCallback)(void *, const void *, u32, u32);

#define FRAME_BYTES 0x1600
#define CONTEXT_BYTES 0x1c00
#define OUTPUT_BYTES 0x4000
#define EVENT_CAPACITY 256

typedef struct Event {
    u32 kind;
    u32 argc;
    u32 args[5];
} Event;

typedef struct Snapshot {
    u8 frame[FRAME_BYTES];
    u8 context[0x200];
    u8 context_after[0x200];
    u8 output[OUTPUT_BYTES];
    Event events[EVENT_CAPACITY];
    u32 event_count;
    u32 overflow;
    u32 cursor_offset;
    u32 cursor_valid;
    u32 interface_context_address;
} Snapshot;

extern u8 g_cold_start_state_bytes_70820[0x1c];
extern u8 *g_dispatch_frame_current_71ec4;
extern u32 g_frame_global_73484;
extern u32 g_frame_global_73488;
extern int g_context_sequence_phase;
extern void __cdecl dispatch_local_2c60(const u16 *, u32, u32);
extern void __cdecl dispatch_local_1f40(void);

static HMODULE g_native_module;
static u8 *g_native_base;
static u8 g_frame[FRAME_BYTES];
static u8 g_frame_after_callback[FRAME_BYTES];
static u8 g_context[CONTEXT_BYTES];
static u8 g_context_after_callback[CONTEXT_BYTES];
static u8 g_output[OUTPUT_BYTES];
static Event g_events[EVENT_CAPACITY];
static u32 g_event_count;
static u32 g_event_overflow;
static int g_native_active;
static int g_mutate_cursor_on_2c9b0;
static int g_switch_frame_on_2c9b0;
static int g_mutate_operation_width_on_first_pack;
static u32 g_mutated_operation_index;
static u8 g_mutated_operation_width;
static int g_switch_context_on_first_pack;
static u32 g_pack_count;
static Snapshot g_native_snapshot;
static Snapshot g_candidate_snapshot;
static int g_failures;
static jmp_buf g_incomplete_jump;
static int g_incomplete_armed;
static unsigned long g_incomplete_rva;
static const char *g_incomplete_reason;
static u8 g_runtime_script_long[9] = {
    'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 0
};
static u8 g_callback_width_script[3] = { 'P', 'A', 0 };

static void event(u32 kind, u32 argc, u32 a0, u32 a1, u32 a2,
                  u32 a3, u32 a4)
{
    Event *next;
    if (g_event_count >= EVENT_CAPACITY) {
        g_event_overflow = 1;
        return;
    }
    next = &g_events[g_event_count++];
    next->kind = kind;
    next->argc = argc;
    next->args[0] = a0;
    next->args[1] = a1;
    next->args[2] = a2;
    next->args[3] = a3;
    next->args[4] = a4;
}

static void check(int condition, const char *name)
{
    if (!condition) {
        ++g_failures;
        printf("FAIL: %s\n", name);
    }
}

void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    if (g_incomplete_armed) {
        g_incomplete_rva = rva;
        g_incomplete_reason = reason;
        longjmp(g_incomplete_jump, 1);
    }
    fprintf(stderr, "unexpected incomplete path 0x%08lx: %s\n",
            rva, reason != 0 ? reason : "(no reason)");
    exit(1);
}

static u32 __cdecl fixture_pack(u32 first, u32 second)
{
    event(1UL, 2UL, first, second, 0UL, 0UL, 0UL);
    if (g_pack_count == 0UL) {
        if (g_mutate_operation_width_on_first_pack) {
            u32 address;
            address = 0x4d760UL + g_mutated_operation_index * 3UL + 2UL;
            if (g_native_active)
                *(u8 *)(void *)(g_native_base + address) =
                    g_mutated_operation_width;
            else
                g_eng1_dialog_operations_4d760[address - 0x4d760UL] =
                    g_mutated_operation_width;
        }
        if (g_switch_context_on_first_pack) {
            if (g_native_active)
                *(u32 *)(void *)(g_native_base + 0x70f24) =
                    (u32)(unsigned long)g_context_after_callback;
            else
                g_engine_interface.context_004 =
                    (ResourceCallbackContext *)(void *)g_context_after_callback;
        }
    }
    ++g_pack_count;
    return (second << 6) | ((first >> 4) & 0x3fUL);
}

static void __cdecl fixture_2c980(void *record, u32 second, u32 third,
                                  u32 fourth, u32 fifth)
{
    u8 *bytes = (u8 *)record;
    event(2UL, 5UL, (u32)(unsigned long)record, second, third, fourth, fifth);
    *(u32 *)(void *)(bytes + 4) = fourth;
    *(u32 *)(void *)(bytes + 8) = fifth;
    bytes[7] = 0xe1U;
}

static void set_active_cursor(u32 value)
{
    if (g_native_active)
        *(u32 *)(void *)(g_native_base + 0x73484) = value;
    else
        g_frame_global_73484 = value;
}

static void set_active_frame(u8 *frame)
{
    if (g_native_active)
        *(u32 *)(void *)(g_native_base + 0x71ec4) =
            (u32)(unsigned long)frame;
    else
        g_dispatch_frame_current_71ec4 = frame;
}

static void __cdecl fixture_2c9b0(void *record, const void *rectangle,
                                  u32 mode, u32 key)
{
    u8 *bytes = (u8 *)record;
    event(3UL, 4UL, (u32)(unsigned long)record,
          (u32)(unsigned long)rectangle, mode, key, 0UL);
    *(u32 *)(void *)(bytes + 4) = 0x13579bdfUL;
    *(u32 *)(void *)(bytes + 8) = 0x2468ace0UL;
    bytes[7] = 0x6dU;
    if (g_mutate_cursor_on_2c9b0)
        set_active_cursor((u32)(unsigned long)(g_output + 0x100));
    if (g_switch_frame_on_2c9b0)
        set_active_frame(g_frame_after_callback);
}

static void set_candidate_callbacks(void)
{
    *(PackCallback *)(void *)((u8 *)&g_engine_interface + 0x1e8) =
        fixture_pack;
    g_engine_interface.dispatch_2c980_2ac = fixture_2c980;
    *(FrameRecordCallback *)(void *)((u8 *)&g_engine_interface + 0x2b0) =
        fixture_2c9b0;
}

static void set_native_callbacks(void)
{
    *(PackCallback *)(void *)(g_native_base + 0x71108) = fixture_pack;
    *(Native2c980 *)(void *)(g_native_base + 0x711cc) = fixture_2c980;
    *(Native2c9b0 *)(void *)(g_native_base + 0x711d0) = fixture_2c9b0;
}

static void clear_events(void)
{
    memset(g_events, 0, sizeof(g_events));
    g_event_count = 0;
    g_event_overflow = 0;
}

static void reset_case(int native, int phase, u8 text_state, u8 flags,
                       u16 transfer_status)
{
    memset(g_frame, 0, sizeof(g_frame));
    memset(g_frame_after_callback, 0, sizeof(g_frame_after_callback));
    memset(g_context, 0, sizeof(g_context));
    memset(g_context_after_callback, 0, sizeof(g_context_after_callback));
    memset(g_output, 0xa5, sizeof(g_output));
    memset(g_cold_start_state_bytes_70820, 0, 0x1c);
    memset(g_frame_color_state_prefix_6ffc8, 0, 0x0c);
    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    memset(g_frame + 0x88, 0x44, sizeof(u32));
    memset(g_frame + 0x1374, 0x33, sizeof(u32));
    memset(g_frame_after_callback + 0x88, 0x77, sizeof(u32));
    g_context[0x04] = text_state;
    g_context[0x38] = flags;
    *(u16 *)(void *)(g_context + 0xdc) = transfer_status;
    clear_events();
    g_native_active = native;
    g_mutate_cursor_on_2c9b0 = 0;
    g_switch_frame_on_2c9b0 = 0;
    g_pack_count = 0UL;

    if (native) {
        *(u32 *)(void *)(g_native_base + 0x70f24) =
            (u32)(unsigned long)g_context;
        *(u32 *)(void *)(g_native_base + 0x71ec4) =
            (u32)(unsigned long)g_frame;
        *(u32 *)(void *)(g_native_base + 0x73484) =
            (u32)(unsigned long)g_output;
        *(u32 *)(void *)(g_native_base + 0x73488) = 0xa1b2c3d4UL;
        *(u32 *)(void *)(g_native_base + 0x6eee4) = (u32)phase;
        memset(g_native_base + 0x70820, 0, 0x1c);
        memset(g_native_base + 0x6ffc8, 0, 0x0c);
        set_native_callbacks();
    } else {
        g_engine_interface.context_004 =
            (ResourceCallbackContext *)(void *)g_context;
        g_dispatch_frame_current_71ec4 = g_frame;
        g_frame_global_73484 = (u32)(unsigned long)g_output;
        g_frame_global_73488 = 0xa1b2c3d4UL;
        g_context_sequence_phase = phase;
        set_candidate_callbacks();
    }
}

static void capture(Snapshot *snapshot, int native)
{
    u32 cursor;
    unsigned long base = (unsigned long)g_output;
    memcpy(snapshot->frame, g_frame, sizeof(g_frame));
    memcpy(snapshot->context, g_context, sizeof(snapshot->context));
    memcpy(snapshot->context_after, g_context_after_callback,
           sizeof(snapshot->context_after));
    memcpy(snapshot->output, g_output, sizeof(g_output));
    memcpy(snapshot->events, g_events, sizeof(g_events));
    snapshot->event_count = g_event_count;
    snapshot->overflow = g_event_overflow;
    cursor = native ? *(u32 *)(void *)(g_native_base + 0x73484)
                    : g_frame_global_73484;
    snapshot->interface_context_address = native
        ? *(u32 *)(void *)(g_native_base + 0x70f24)
        : (u32)(unsigned long)g_engine_interface.context_004;
    snapshot->cursor_valid = cursor >= (u32)base &&
        cursor <= (u32)(base + sizeof(g_output));
    snapshot->cursor_offset = snapshot->cursor_valid
        ? cursor - (u32)base : 0xffffffffUL;
}

static int compare_snapshots(const char *label)
{
    int equal = 1;
    if (memcmp(g_native_snapshot.output, g_candidate_snapshot.output,
               sizeof(g_native_snapshot.output)) != 0) {
        unsigned long i;
        for (i = 0; i < OUTPUT_BYTES; ++i) {
            if (g_native_snapshot.output[i] != g_candidate_snapshot.output[i]) {
                printf("FAIL: %s output[%04lx] native=%02x candidate=%02x\n",
                       label, i, g_native_snapshot.output[i],
                       g_candidate_snapshot.output[i]);
                break;
            }
        }
        equal = 0;
    }
    if (g_candidate_snapshot.cursor_valid) {
        u32 offset;
        for (offset = g_candidate_snapshot.cursor_offset;
             offset < OUTPUT_BYTES; ++offset) {
            if (g_candidate_snapshot.output[offset] != 0xa5U) {
                printf("FAIL: %s candidate output suffix canary at %04lx is %02x\n",
                       label, offset,
                       g_candidate_snapshot.output[offset]);
                equal = 0;
                break;
            }
        }
    }
    if (memcmp(g_native_snapshot.frame, g_candidate_snapshot.frame,
               sizeof(g_native_snapshot.frame)) != 0) {
        printf("FAIL: %s frame bytes differ\n", label);
        equal = 0;
    }
    if (memcmp(g_native_snapshot.context, g_candidate_snapshot.context,
               sizeof(g_native_snapshot.context)) != 0) {
        printf("FAIL: %s context bytes differ\n", label);
        equal = 0;
    }
    if (memcmp(g_native_snapshot.context_after,
               g_candidate_snapshot.context_after,
               sizeof(g_native_snapshot.context_after)) != 0 ||
        g_native_snapshot.interface_context_address !=
            g_candidate_snapshot.interface_context_address) {
        printf("FAIL: %s alternate context bytes/pointer differ\n", label);
        equal = 0;
    }
    if (g_native_snapshot.event_count != g_candidate_snapshot.event_count ||
        g_native_snapshot.overflow != g_candidate_snapshot.overflow ||
        memcmp(g_native_snapshot.events, g_candidate_snapshot.events,
               sizeof(g_native_snapshot.events)) != 0) {
        printf("FAIL: %s callback order/arguments native=%lu candidate=%lu\n",
               label, g_native_snapshot.event_count,
               g_candidate_snapshot.event_count);
        {
            u32 event_index;
            for (event_index = 0; event_index < 8 &&
                 event_index < g_native_snapshot.event_count; ++event_index) {
                Event *native_event = &g_native_snapshot.events[event_index];
                printf("  native event %lu kind=%lu argc=%lu args=%08lx,%08lx,%08lx,%08lx,%08lx\n",
                       event_index, native_event->kind, native_event->argc,
                       native_event->args[0], native_event->args[1],
                       native_event->args[2], native_event->args[3],
                       native_event->args[4]);
            }
            for (event_index = 0; event_index < 8 &&
                 event_index < g_candidate_snapshot.event_count; ++event_index) {
                Event *candidate_event = &g_candidate_snapshot.events[event_index];
                printf("  candidate event %lu kind=%lu argc=%lu args=%08lx,%08lx,%08lx,%08lx,%08lx\n",
                       event_index, candidate_event->kind, candidate_event->argc,
                       candidate_event->args[0], candidate_event->args[1],
                       candidate_event->args[2], candidate_event->args[3],
                       candidate_event->args[4]);
            }
        }
        equal = 0;
    }
    if (!g_native_snapshot.cursor_valid || !g_candidate_snapshot.cursor_valid ||
        g_native_snapshot.cursor_offset != g_candidate_snapshot.cursor_offset) {
        printf("FAIL: %s cursor native=%08lx/%08lx candidate=%08lx/%08lx\n",
               label, g_native_snapshot.cursor_valid,
               g_native_snapshot.cursor_offset,
               g_candidate_snapshot.cursor_valid,
               g_candidate_snapshot.cursor_offset);
        equal = 0;
    }
    check(equal, label);
    return equal;
}

static void run_2c60_case(const char *label, int mutate_cursor, int switch_frame)
{
    u16 rectangle[4];
    Native2c60 call_native;

    rectangle[0] = 0x1234U;
    rectangle[1] = 0x5678U;
    rectangle[2] = 0x9abcU;
    rectangle[3] = 0xdef0U;
    reset_case(1, 0, 0, 0, 0);
    g_mutate_cursor_on_2c9b0 = mutate_cursor;
    g_switch_frame_on_2c9b0 = switch_frame;
    call_native = (Native2c60)(void *)(g_native_base + 0x2c60);
    g_native_active = 1;
    call_native(rectangle, 0x12345678UL, 0x238UL);
    capture(&g_native_snapshot, 1);

    reset_case(0, 0, 0, 0, 0);
    g_mutate_cursor_on_2c9b0 = mutate_cursor;
    g_switch_frame_on_2c9b0 = switch_frame;
    g_native_active = 0;
    dispatch_local_2c60(rectangle, 0x12345678UL, 0x238UL);
    capture(&g_candidate_snapshot, 0);
    compare_snapshots(label);
}

static void run_1f40_case(const char *label, u8 state, int phase,
                          u8 flags, u16 transfer_status)
{
    Native1f40 call_native;
    reset_case(1, phase, state, flags, transfer_status);
    call_native = (Native1f40)(void *)(g_native_base + 0x1f40);
    g_native_active = 1;
    call_native();
    capture(&g_native_snapshot, 1);

    reset_case(0, phase, state, flags, transfer_status);
    g_native_active = 0;
    dispatch_local_1f40();
    capture(&g_candidate_snapshot, 0);
    compare_snapshots(label);
}

static void run_1f40_context_switch_case(void)
{
    Native1f40 call_native;
    u32 pack_events;
    u32 index;

    g_switch_context_on_first_pack = 1;
    reset_case(1, 2, 0, 0x10U, 0);
    g_context_after_callback[0x04] = 2U;
    call_native = (Native1f40)(void *)(g_native_base + 0x1f40);
    g_native_active = 1;
    call_native();
    capture(&g_native_snapshot, 1);

    reset_case(0, 2, 0, 0x10U, 0);
    g_context_after_callback[0x04] = 2U;
    g_native_active = 0;
    dispatch_local_1f40();
    capture(&g_candidate_snapshot, 0);
    compare_snapshots("1f40 reloads interface context after phase-2 script callback");

    pack_events = 0UL;
    for (index = 0UL; index < g_candidate_snapshot.event_count; ++index) {
        if (g_candidate_snapshot.events[index].kind == 1UL)
            ++pack_events;
    }
    check(g_candidate_snapshot.interface_context_address ==
              (u32)(unsigned long)g_context_after_callback,
          "1f40 leaves callback-selected context installed");
    check(pack_events == 26UL,
          "1f40 uses the callback-selected state-2 context after script 14");
    g_switch_context_on_first_pack = 0;
}

static void run_2450_case(const char *label, s32 x, s32 y,
                          u32 script_id, s32 mode)
{
    Native2450 call_native;
    reset_case(1, 0, 0, 0, 0);
    call_native = (Native2450)(void *)(g_native_base + 0x2450);
    g_native_active = 1;
    call_native(x, y, script_id, mode);
    capture(&g_native_snapshot, 1);

    reset_case(0, 0, 0, 0, 0);
    g_native_active = 0;
    dispatch_local_2450(x, y, script_id, mode);
    capture(&g_candidate_snapshot, 0);
    compare_snapshots(label);
}

static void run_2450_callback_mutates_width_case(void)
{
    u8 **native_slot0 = (u8 **)(void *)(g_native_base + 0x4d720);
    u8 *saved_native_pointer = *native_slot0;
    u8 *saved_candidate_pointer = g_eng1_dialog_scripts_4d720[0];
    u8 *native_width = g_native_base + 0x4d760 + 15 * 3 + 2;
    u8 saved_native_width = *native_width;
    u8 saved_candidate_width = g_eng1_dialog_operations_4d760[15 * 3 + 2];

    *native_slot0 = g_callback_width_script;
    g_eng1_dialog_scripts_4d720[0] = g_callback_width_script;
    g_mutated_operation_index = 15UL;
    g_mutated_operation_width = 0x31U;
    g_mutate_operation_width_on_first_pack = 1;
    run_2450_case("2450 reloads operation width after first glyph callback",
                  0x200L, 0x30L, 0UL, 3L);
    g_mutate_operation_width_on_first_pack = 0;

    check(*(const u16 *)(const void *)(g_native_snapshot.output + 0x10) ==
              0x0eU,
          "2450 passes the pre-callback width to the first glyph");
    check(*(const u16 *)(const void *)(g_native_snapshot.output + 0x1c) ==
              0x231U,
          "2450 applies the callback-updated width to the next glyph x");
    check(g_native_snapshot.cursor_offset == 0x28UL,
          "2450 live-width discriminator emits two ordinary records");

    *native_slot0 = saved_native_pointer;
    g_eng1_dialog_scripts_4d720[0] = saved_candidate_pointer;
    *native_width = saved_native_width;
    g_eng1_dialog_operations_4d760[15 * 3 + 2] = saved_candidate_width;
}

static void run_2450_data_mutation_cases(void)
{
    u8 **native_slot0 = (u8 **)(void *)(g_native_base + 0x4d720);
    u8 *saved_native_pointer = *native_slot0;
    u8 *saved_candidate_pointer = g_eng1_dialog_scripts_4d720[0];
    u8 *native_script0 = g_native_base + 0x4d90c;
    u8 *native_script1 = g_native_base + 0x4d900;
    u8 *native_operation0 = g_native_base + 0x4d760;
    u8 saved_native_script_byte = native_script0[0];
    u8 saved_candidate_script_byte = g_eng1_dialog_script_0[0];
    u8 saved_native_width = native_operation0[2];
    u8 saved_candidate_width = g_eng1_dialog_operations_4d760[2];

    *native_slot0 = native_script1;
    g_eng1_dialog_scripts_4d720[0] = g_eng1_dialog_script_1;
    run_2450_case("2450 writable pointer-table change uses exact script-1 base bound",
                  0x7fffffffL, 0x1234L, 0UL, 3L);

    *native_slot0 = native_script0;
    g_eng1_dialog_scripts_4d720[0] = g_eng1_dialog_script_0;
    native_script0[0] = 'A';
    g_eng1_dialog_script_0[0] = 'A';
    native_operation0[2] = 0x15U;
    g_eng1_dialog_operations_4d760[2] = 0x15U;
    run_2450_case("2450 writable script and operation bytes affect the parser",
                  (s32)0x80000000UL, 0x4321L, 0UL, 4L);

    *native_slot0 = g_runtime_script_long;
    g_eng1_dialog_scripts_4d720[0] = g_runtime_script_long;
    run_2450_case("2450 changed unknown-base pointer reads through its own NUL",
                  0x7ffffffeL, 0x5678L, 0UL, 5L);

    *native_slot0 = saved_native_pointer;
    g_eng1_dialog_scripts_4d720[0] = saved_candidate_pointer;
    native_script0[0] = saved_native_script_byte;
    g_eng1_dialog_script_0[0] = saved_candidate_script_byte;
    native_operation0[2] = saved_native_width;
    g_eng1_dialog_operations_4d760[2] = saved_candidate_width;
}

static void run_2450_missing_nul_guard(void)
{
    static u8 complete_prefix_plus_one[] = {
        'P', 'A', 'U', 'S', 'E', 'D', 'A', 0
    };
    Snapshot completed;
    Snapshot guarded;
    u8 *saved_pointer = g_eng1_dialog_scripts_4d720[0];
    u8 saved_terminator = g_eng1_dialog_script_0[6];
    int equal;

    reset_case(0, 0, 0, 0, 0);
    g_eng1_dialog_scripts_4d720[0] = complete_prefix_plus_one;
    dispatch_local_2450(0x7fffffffL, 0x2468L, 0UL, 3L);
    capture(&completed, 0);

    reset_case(0, 0, 0, 0, 0);
    g_eng1_dialog_scripts_4d720[0] = g_eng1_dialog_script_0;
    g_eng1_dialog_script_0[6] = 'A';
    g_incomplete_rva = 0UL;
    g_incomplete_reason = 0;
    if (setjmp(g_incomplete_jump) == 0) {
        g_incomplete_armed = 1;
        dispatch_local_2450(0x7fffffffL, 0x2468L, 0UL, 3L);
        g_incomplete_armed = 0;
        check(0, "2450 missing NUL must trap before an unmeasured read");
    } else {
        g_incomplete_armed = 0;
    }
    capture(&guarded, 0);

    equal = g_incomplete_rva == 0x2450UL &&
        g_incomplete_reason != 0 &&
        strstr(g_incomplete_reason, "measured prefix") != 0 &&
        completed.event_count == guarded.event_count &&
        completed.overflow == guarded.overflow &&
        memcmp(completed.events, guarded.events, sizeof(completed.events)) == 0 &&
        memcmp(completed.output, guarded.output, sizeof(completed.output)) == 0 &&
        memcmp(completed.frame, guarded.frame, sizeof(completed.frame)) == 0 &&
        memcmp(completed.context, guarded.context, sizeof(completed.context)) == 0 &&
        completed.cursor_valid == guarded.cursor_valid &&
        completed.cursor_offset == guarded.cursor_offset;
    check(equal, "2450 missing-NUL trap follows every measured byte and precedes the next read");

    g_eng1_dialog_script_0[6] = saved_terminator;
    g_eng1_dialog_scripts_4d720[0] = saved_pointer;
}

int main(int argc, char **argv)
{
    unsigned int state;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <original ENG1.DLL>\n", argv[0]);
        return 2;
    }
    g_native_module = LoadLibraryA(argv[1]);
    check(g_native_module != NULL, "load original ENG1 from argv");
    if (g_native_module == NULL)
        return 2;
    g_native_base = (u8 *)g_native_module;

    run_2c60_case("2c60 ordinary callback, record link and +0x18 cursor",
                  0, 0);
    run_2c60_case("2c60 callback-mutated cursor is replaced from captured ESI",
                  1, 0);
    run_2c60_case("2c60 links through the callback-updated live frame pointer",
                  0, 1);

    for (state = 0; state <= 130; ++state) {
        char label[64];
        sprintf(label, "1f40 native state byte %u", state);
        run_1f40_case(label, (u8)state, 0, 0, 0);
    }
    run_1f40_case("1f40 state 255 unsigned out-of-range return",
                  0xffU, 0, 0, 0);
    run_1f40_case("1f40 phase-2 flag-10 pre-script then state 2",
                  2, 2, 0x10U, 0);
    run_1f40_case("1f40 phase-2 flag-10 context reload then state 3",
                  3, 2, 0x10U, 0);
    run_1f40_case("1f40 state 128 flag-08 script gate", 128, 0, 0x08U, 0);
    run_1f40_case("1f40 state 16 transfer-status 0x10 selects script 12",
                  16, 0, 0, 0x10U);
    run_1f40_case("1f40 state 16 other transfer status selects script 11",
                  16, 0, 0, 0);
    run_1f40_context_switch_case();

    run_2450_case("2450 x86 DWORD wrap from INT_MAX while advancing widths",
                  0x7fffffffL, 0x1357L, 0UL, 3L);
    run_2450_case("2450 x86 DWORD wrap from INT_MIN while advancing widths",
                  (s32)0x80000000UL, 0x2468L, 0UL, 3L);
    run_2450_data_mutation_cases();
    run_2450_callback_mutates_width_case();
    run_2450_missing_nul_guard();

    if (g_native_module != NULL)
        FreeLibrary(g_native_module);
    if (g_failures != 0) {
        printf("%d frame-callee differential checks failed\n", g_failures);
        return 1;
    }
    printf("2c60, 132 state-value and 6 branch-context 1f40, and 6 direct 2450 native differentials passed; missing-NUL guard trapped at the measured prefix\n");
    return 0;
}
