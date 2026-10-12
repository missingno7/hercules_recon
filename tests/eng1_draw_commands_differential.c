#include <string.h>
#include <stdio.h>
#include <windows.h>
#include "../calibration/engine_interface.h"
#include "../src/eng1/behavioral/cold_start_frame_data.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef long s32;
typedef u32 (__cdecl *PackCallback)(u32, u32);
typedef void (__cdecl *PacketCallback)(void *);
typedef void (__cdecl *FlagCallback)(void *, u32);
typedef void (__cdecl *NativeDraw7)(s32, s32, s32, s32, s32, s32, s32);
typedef void (__cdecl *Native2c980)(void *, u32, u32, u32, u32);

#define CALLBACK_EVENT_CAPACITY 32
#define DRAW_BUFFER_SIZE 0x80

typedef struct CallbackEvent {
    u32 kind;
    u32 argc;
    u32 args[5];
} CallbackEvent;

extern u8 g_cold_start_state_bytes_70820[0x1c];
extern u32 g_frame_global_73484;
extern u32 g_frame_global_73488;
extern u8 *g_dispatch_frame_current_71ec4;
extern void __cdecl dispatch_local_33420(s32, s32, s32, s32, s32, s32, s32);
extern void __cdecl dispatch_local_33620(s32, s32, s32, s32, s32, s32, s32);

static u8 g_frame[0x1400];
static u8 g_output[DRAW_BUFFER_SIZE];
static int g_failures;
static int g_pack_calls;
static int g_packet_calls;
static int g_flag_calls;
static int g_2c980_calls;
static u32 g_pack_first[8];
static u32 g_pack_second[8];
static void *g_2c980_pointer;
static u32 g_2c980_args[4];
static HMODULE g_native_eng1;
static CallbackEvent g_events[CALLBACK_EVENT_CAPACITY];
static int g_event_count;
static int g_event_overflow;
static u8 *g_native_base;
static int g_active_native;
static int g_mutate_cursor_on_pack_call;
static u32 g_pack_cursor_mutation_offset;
static int g_mutate_cursor_in_2c980;
static u32 g_2c980_cursor_mutation_offset;

static void set_active_cursor(u32 cursor)
{
    if (g_active_native)
        *(u32 *)(void *)(g_native_base + 0x73484) = cursor;
    else
        g_frame_global_73484 = cursor;
}

static void check(int condition, const char *name)
{
    if (!condition) {
        ++g_failures;
        printf("FAIL: %s\n", name);
    }
}

static void test_native_module_load(const char *native_path)
{
    g_native_eng1 = LoadLibraryA(native_path);
    g_native_base = (u8 *)g_native_eng1;
    check(g_native_eng1 != NULL, "unpatched differential: original ENG1 loads");
}

static u16 get_u16(const u8 *bytes)
{
    return *(const u16 *)(const void *)bytes;
}

static u32 get_u32(const u8 *bytes)
{
    return *(const u32 *)(const void *)bytes;
}

static void log_event(u32 kind, u32 argc, u32 a0, u32 a1, u32 a2,
                      u32 a3, u32 a4)
{
    CallbackEvent *event;

    if (g_event_count >= CALLBACK_EVENT_CAPACITY) {
        g_event_overflow = 1;
        return;
    }
    event = &g_events[g_event_count++];
    event->kind = kind;
    event->argc = argc;
    event->args[0] = a0;
    event->args[1] = a1;
    event->args[2] = a2;
    event->args[3] = a3;
    event->args[4] = a4;
}

static void clear_callback_log(void)
{
    memset(g_events, 0, sizeof(g_events));
    g_event_count = 0;
    g_event_overflow = 0;
}

static u32 __cdecl fixture_pack(u32 first, u32 second)
{
    int call_index;

    call_index = g_pack_calls + 1;
    log_event(1UL, 2UL, first, second, 0UL, 0UL, 0UL);
    if (g_pack_calls < 8) {
        g_pack_first[g_pack_calls] = first;
        g_pack_second[g_pack_calls] = second;
    }
    ++g_pack_calls;
    if (g_mutate_cursor_on_pack_call == call_index)
        set_active_cursor((u32)(unsigned long)g_output +
                          g_pack_cursor_mutation_offset);
    return (second << 6) | ((first >> 4) & 0x3fUL);
}

static void __cdecl fixture_packet(void *record)
{
    log_event(2UL, 1UL, (u32)(unsigned long)record,
              0UL, 0UL, 0UL, 0UL);
    ++g_packet_calls;
    ((u8 *)record)[7] = 0x2cU;
}

static void __cdecl fixture_flag(void *record, u32 enabled)
{
    u8 *bytes;

    bytes = (u8 *)record;
    log_event(3UL, 2UL, (u32)(unsigned long)record, enabled,
              0UL, 0UL, 0UL);
    ++g_flag_calls;
    if (enabled != 0)
        bytes[7] |= 2U;
    else
        bytes[7] &= 0xfdU;
}

static void __cdecl fixture_2c980(void *first, u32 second, u32 third,
                                   u32 fourth, u32 fifth)
{
    u8 *bytes;

    ++g_2c980_calls;
    log_event(4UL, 5UL, (u32)(unsigned long)first, second, third,
              fourth, fifth);
    g_2c980_pointer = first;
    g_2c980_args[0] = second;
    g_2c980_args[1] = third;
    g_2c980_args[2] = fourth;
    g_2c980_args[3] = fifth;
    bytes = (u8 *)first;
    *(u32 *)(void *)(bytes + 4) = fourth;
    *(u32 *)(void *)(bytes + 8) = fifth;
    bytes[7] = 0xe1U;
    if (g_mutate_cursor_in_2c980)
        set_active_cursor((u32)(unsigned long)g_output +
                          g_2c980_cursor_mutation_offset);
}

static void set_host_callbacks(void)
{
    *(PackCallback *)(void *)((u8 *)&g_engine_interface + 0x1e8) = fixture_pack;
    *(PacketCallback *)(void *)((u8 *)&g_engine_interface + 0x2d0) = fixture_packet;
    *(FlagCallback *)(void *)((u8 *)&g_engine_interface + 0x2e8) = fixture_flag;
    g_engine_interface.dispatch_2c980_2ac = fixture_2c980;
}

static void set_native_callbacks(void)
{
    *(PackCallback *)(void *)(g_native_base + 0x71108) = fixture_pack;
    *(PacketCallback *)(void *)(g_native_base + 0x711f0) = fixture_packet;
    *(FlagCallback *)(void *)(g_native_base + 0x71208) = fixture_flag;
    *(Native2c980 *)(void *)(g_native_base + 0x711cc) = fixture_2c980;
}

static void reset_fixture(u32 initial_head)
{
    memset(g_frame, 0, sizeof(g_frame));
    memset(g_output, 0xa5, sizeof(g_output));
    memset(g_cold_start_state_bytes_70820, 0, 0x1c);
    memset(g_frame_color_state_prefix_6ffc8, 0, 0x0c);
    *(u32 *)(void *)(g_frame + 0x1374) = initial_head;
    g_dispatch_frame_current_71ec4 = g_frame;
    g_frame_global_73484 = (u32)(unsigned long)g_output;
    g_frame_global_73488 = 0xa1b2c3d4UL;
    g_pack_calls = 0;
    g_packet_calls = 0;
    g_flag_calls = 0;
    g_2c980_calls = 0;
    memset(g_pack_first, 0, sizeof(g_pack_first));
    memset(g_pack_second, 0, sizeof(g_pack_second));
    g_2c980_pointer = 0;
    memset(g_2c980_args, 0, sizeof(g_2c980_args));
    clear_callback_log();
    g_active_native = 0;
    g_mutate_cursor_on_pack_call = 0;
    g_pack_cursor_mutation_offset = 0;
    g_mutate_cursor_in_2c980 = 0;
    g_2c980_cursor_mutation_offset = 0;
}

static void test_basic_record(void)
{
    reset_fixture(0x11223344UL);
    dispatch_local_33620((s32)0x12345678UL, (s32)0x9abcdeffUL,
                         (s32)0x123456abUL, (s32)0x123456cdUL,
                         (s32)0x9876fedcUL, (s32)0x13579bdfUL, 0x20L);

    check(get_u32(g_output) == 0x11223344UL, "basic: old linked head");
    check(get_u32(g_output + 4) == 0x64b2c3d4UL,
          "basic: color dword then type byte");
    check(get_u16(g_output + 8) == 0x5678U, "basic: arg1 truncates to WORD");
    check(get_u16(g_output + 0x0a) == 0xdeffU,
          "basic: arg2 truncates to WORD");
    check(g_output[0x0c] == 0xabU && g_output[0x0d] == 0xcdU,
          "basic: args3/4 truncate to BYTE");
    check(get_u16(g_output + 0x0e) == 0x1016U,
          "basic: actual host pack callback result");
    check(get_u16(g_output + 0x10) == 0xfedcU,
          "basic: arg5 truncates to WORD");
    check(get_u16(g_output + 0x12) == 0x9bdfU,
          "basic: arg6, not hidden stack tail");
    check(g_frame_global_73484 ==
          (u32)(unsigned long)(g_output + 0x14), "basic: cursor advances 0x14");
    check(*(u32 *)(void *)(g_frame + 0x1374) ==
          (u32)(unsigned long)g_output, "basic: frame head updated");
    check(g_pack_calls == 1 && g_pack_first[0] == 0x160UL &&
          g_pack_second[0] == 0x40UL, "basic: one exact pack callback");
    check(g_output[0x14] == 0xa5U && g_output[0x7f] == 0xa5U,
          "basic: output boundary canaries");
}

static void test_mode_four_extra_record(void)
{
    reset_fixture(0x01020304UL);
    dispatch_local_33620(1L, 2L, 3L, 4L, 5L, 6L, 4L);

    check(get_u16(g_output + 0x0e) == 0x0916U,
          "mode4: primary packed word");
    check(g_output[7] == 0x66U, "mode4: primary type byte");
    check(get_u32(g_output + 0x14) == (u32)(unsigned long)g_output,
          "mode4: extra command links to primary");
    check(get_u32(g_output + 0x18) == 0xe1000180UL,
          "mode4: host callback exact 12-byte packet payload");
    check(get_u32(g_output + 0x1c) == 0UL,
          "mode4: host callback second payload word");
    check(g_2c980_calls == 1 &&
          g_2c980_pointer == (void *)(g_output + 0x14) &&
          g_2c980_args[0] == 0UL && g_2c980_args[1] == 0UL &&
          g_2c980_args[2] == 0x180UL && g_2c980_args[3] == 0UL,
          "mode4: one actual 2c980 callback tuple");
    check(g_frame_global_73484 ==
          (u32)(unsigned long)(g_output + 0x20), "mode4: cursor advances 0x20");
    check(*(u32 *)(void *)(g_frame + 0x1374) ==
          (u32)(unsigned long)(g_output + 0x14),
          "mode4: extra command becomes frame head");
    check(g_output[0x20] == 0xa5U, "mode4: extra packet boundary canary");
}

static void test_ordinary_33420_forwarding(void)
{
    reset_fixture(0x55667788UL);
    dispatch_local_33420(0x1111L, 0x2222L, (s32)0x1234UL,
                         (s32)0x12ffUL, 0x3333L, 0x44L, 0x55L);

    check(get_u16(g_output + 8) == 0x1111U &&
          get_u16(g_output + 0x0a) == 0x2222U,
          "ordinary 33420: first arguments forwarded");
    check(g_output[0x0c] == 0x34U && g_output[0x0d] == 0xffU,
          "ordinary 33420: byte arguments narrowed before forwarding");
    check(get_u16(g_output + 0x10) == 0x3333U &&
          get_u16(g_output + 0x12) == 0x44U,
          "ordinary 33420: args5/6 forwarded");
    check(g_pack_calls == 1 && g_frame_global_73484 ==
          (u32)(unsigned long)(g_output + 0x14),
          "ordinary 33420: one command and cursor update");
}

static void test_blend_pair(void)
{
    u8 *second;

    reset_fixture(0xabcdef01UL);
    g_cold_start_state_bytes_70820[0x14] = 2U;
    *(u16 *)(void *)(g_frame_color_state_prefix_6ffc8 + 8) = 0x10U;
    *(u32 *)(void *)g_frame_color_state_prefix_6ffc8 = 0x11223344UL;
    *(u32 *)(void *)(g_frame_color_state_prefix_6ffc8 + 4) = 0xaabbccddUL;
    dispatch_local_33420(0x10L, 0x20L, 3L, 5L, 6L, 7L, 8L);
    second = g_output + 0x28;

    check(get_u32(g_output) == 0xabcdef01UL &&
          get_u32(second) == (u32)(unsigned long)g_output,
          "blend: two-record linked-list order");
    check(get_u32(g_output + 4) == 0x2e223344UL &&
          get_u32(second + 4) == 0x2ebbccddUL,
          "blend: packet callback then enabled flag modify byte7");
    check(get_u32(g_output + 8) == 0x00200010UL &&
          get_u16(g_output + 0x0c) == 0x0503U &&
          get_u16(g_output + 0x0e) == 0x0a16U &&
          get_u32(g_output + 0x10) == 0x00200016UL &&
          get_u16(g_output + 0x14) == 0x0509U &&
          get_u16(g_output + 0x16) == 0x01a0U &&
          get_u32(g_output + 0x18) == 0x00270010UL &&
          get_u16(g_output + 0x1c) == 0x0c03U &&
          get_u32(g_output + 0x20) == 0x00270016UL &&
          get_u16(g_output + 0x24) == 0x0c09U,
          "blend: first descriptor measured fields");
    check(get_u16(second + 0x0e) == 0x0996U &&
          get_u16(second + 0x16) == 0x01c0U,
          "blend: second pack result and loop flag");
    check(g_packet_calls == 2 && g_flag_calls == 2 && g_pack_calls == 3,
          "blend: exact callback counts and order family");
    check(g_pack_first[0] == 0x160UL && g_pack_second[0] == 0x28UL &&
          g_pack_first[1] == 0x160UL && g_pack_second[1] == 0x26UL &&
          g_pack_first[2] == 0x160UL && g_pack_second[2] == 0x26UL,
          "blend: initial and per-record pack callback inputs");
    check(g_frame_global_73484 ==
          (u32)(unsigned long)(g_output + 0x50),
          "blend: cursor advances exactly 0x50");
    check(*(u32 *)(void *)(g_frame + 0x1374) ==
          (u32)(unsigned long)second, "blend: second descriptor becomes head");
    check(g_output[0x26] == 0xa5U && second[0x26] == 0xa5U &&
          g_output[0x50] == 0xa5U,
          "blend: unwritten descriptor tail and cursor canaries");
}

typedef struct DifferentialSnapshot {
    u8 output[DRAW_BUFFER_SIZE];
    CallbackEvent events[CALLBACK_EVENT_CAPACITY];
    int event_count;
    int event_overflow;
    int cursor_valid;
    u32 cursor_offset;
    u32 frame_head;
} DifferentialSnapshot;

static void reset_native_state(u32 initial_head, u8 mode, u16 blend_word)
{
    memset(g_native_base + 0x70820, 0, 0x1c);
    memset(g_native_base + 0x6ffc8, 0, 0x0c);
    *(u32 *)(void *)(g_native_base + 0x71ec4) =
        (u32)(unsigned long)g_frame;
    *(u32 *)(void *)(g_native_base + 0x73484) =
        (u32)(unsigned long)g_output;
    *(u32 *)(void *)(g_native_base + 0x73488) = 0xa1b2c3d4UL;
    *(u32 *)(void *)(g_frame + 0x1374) = initial_head;
    g_native_base[0x70834] = mode;
    *(u16 *)(void *)(g_native_base + 0x6ffd0) = blend_word;
    *(u32 *)(void *)(g_native_base + 0x6ffc8) = 0x11223344UL;
    *(u32 *)(void *)(g_native_base + 0x6ffcc) = 0xaabbccddUL;
}

static void set_candidate_state(u8 mode, u16 blend_word)
{
    g_cold_start_state_bytes_70820[0x14] = mode;
    *(u16 *)(void *)(g_frame_color_state_prefix_6ffc8 + 8) = blend_word;
    *(u32 *)(void *)g_frame_color_state_prefix_6ffc8 = 0x11223344UL;
    *(u32 *)(void *)(g_frame_color_state_prefix_6ffc8 + 4) = 0xaabbccddUL;
}

static void capture_snapshot(DifferentialSnapshot *snapshot,
                             u32 cursor_address)
{
    u32 output_address;

    output_address = (u32)(unsigned long)g_output;
    memcpy(snapshot->output, g_output, sizeof(snapshot->output));
    snapshot->event_count = g_event_count;
    snapshot->event_overflow = g_event_overflow;
    memcpy(snapshot->events, g_events, sizeof(snapshot->events));
    snapshot->cursor_valid = cursor_address >= output_address &&
        cursor_address <= output_address + DRAW_BUFFER_SIZE;
    snapshot->cursor_offset = snapshot->cursor_valid
        ? cursor_address - output_address : 0xffffffffUL;
    snapshot->frame_head = get_u32(g_frame + 0x1374);
}

static void call_native_draw(u32 rva, const s32 args[7])
{
    NativeDraw7 draw;

    draw = (NativeDraw7)(void *)(g_native_base + rva);
    draw(args[0], args[1], args[2], args[3],
         args[4], args[5], args[6]);
}

static void call_candidate_draw(u32 rva, const s32 args[7])
{
    if (rva == 0x33620UL)
        dispatch_local_33620(args[0], args[1], args[2], args[3],
                             args[4], args[5], args[6]);
    else
        dispatch_local_33420(args[0], args[1], args[2], args[3],
                             args[4], args[5], args[6]);
}

static void run_differential_case(const char *name, u32 rva, u8 mode,
                                  u16 blend_word, const s32 args[7],
                                  int mutate_pack_call,
                                  u32 pack_cursor_offset,
                                  int mutate_cursor_in_2c980,
                                  u32 cursor_2c980_offset)
{
    static const u32 initial_head = 0x55667788UL;
    DifferentialSnapshot native_snapshot;
    DifferentialSnapshot candidate_snapshot;
    u32 native_cursor;
    int index;
    int output_equal;
    int events_equal;

    if (g_native_eng1 == NULL)
        return;

    reset_fixture(initial_head);
    set_native_callbacks();
    reset_native_state(initial_head, mode, blend_word);
    g_active_native = 1;
    g_mutate_cursor_on_pack_call = mutate_pack_call;
    g_pack_cursor_mutation_offset = pack_cursor_offset;
    g_mutate_cursor_in_2c980 = mutate_cursor_in_2c980;
    g_2c980_cursor_mutation_offset = cursor_2c980_offset;
    call_native_draw(rva, args);
    native_cursor = get_u32(g_native_base + 0x73484);
    capture_snapshot(&native_snapshot, native_cursor);

    reset_fixture(initial_head);
    set_host_callbacks();
    set_candidate_state(mode, blend_word);
    g_active_native = 0;
    g_mutate_cursor_on_pack_call = mutate_pack_call;
    g_pack_cursor_mutation_offset = pack_cursor_offset;
    g_mutate_cursor_in_2c980 = mutate_cursor_in_2c980;
    g_2c980_cursor_mutation_offset = cursor_2c980_offset;
    call_candidate_draw(rva, args);
    capture_snapshot(&candidate_snapshot, g_frame_global_73484);

    output_equal = memcmp(native_snapshot.output, candidate_snapshot.output,
                          sizeof(native_snapshot.output)) == 0;
    events_equal = native_snapshot.event_count ==
                       candidate_snapshot.event_count &&
        native_snapshot.event_overflow == 0 &&
        candidate_snapshot.event_overflow == 0 &&
        memcmp(native_snapshot.events, candidate_snapshot.events,
               sizeof(native_snapshot.events)) == 0;

    if (!output_equal) {
        for (index = 0; index < DRAW_BUFFER_SIZE; ++index) {
            if (native_snapshot.output[index] !=
                candidate_snapshot.output[index]) {
                printf("FAIL: differential %s: output[%02x] native=%02x candidate=%02x\n",
                       name, index, native_snapshot.output[index],
                       candidate_snapshot.output[index]);
                break;
            }
        }
        printf("  native descriptor bytes:");
        for (index = 0; index < 0x50; ++index)
            printf(" %02x", native_snapshot.output[index]);
        printf("\n  candidate descriptor bytes:");
        for (index = 0; index < 0x50; ++index)
            printf(" %02x", candidate_snapshot.output[index]);
        printf("\n");
        ++g_failures;
    }
    check(native_snapshot.cursor_valid && candidate_snapshot.cursor_valid &&
          native_snapshot.cursor_offset == candidate_snapshot.cursor_offset,
          "differential: cursor offsets match");
    check(native_snapshot.frame_head == candidate_snapshot.frame_head,
          "differential: linked-list head matches");
    check(events_equal, "differential: callback order and arguments match");
    if (!events_equal) {
        printf("  native callbacks=%d candidate callbacks=%d\n",
               native_snapshot.event_count, candidate_snapshot.event_count);
        for (index = 0; index < native_snapshot.event_count &&
             index < CALLBACK_EVENT_CAPACITY; ++index)
            printf("  native event %d: kind=%lu argc=%lu args=%08lx,%08lx,%08lx,%08lx,%08lx\n",
                   index, native_snapshot.events[index].kind,
                   native_snapshot.events[index].argc,
                   native_snapshot.events[index].args[0],
                   native_snapshot.events[index].args[1],
                   native_snapshot.events[index].args[2],
                   native_snapshot.events[index].args[3],
                   native_snapshot.events[index].args[4]);
        for (index = 0; index < candidate_snapshot.event_count &&
             index < CALLBACK_EVENT_CAPACITY; ++index)
            printf("  candidate event %d: kind=%lu argc=%lu args=%08lx,%08lx,%08lx,%08lx,%08lx\n",
                   index, candidate_snapshot.events[index].kind,
                   candidate_snapshot.events[index].argc,
                   candidate_snapshot.events[index].args[0],
                   candidate_snapshot.events[index].args[1],
                   candidate_snapshot.events[index].args[2],
                   candidate_snapshot.events[index].args[3],
                   candidate_snapshot.events[index].args[4]);
    }
    if (output_equal && events_equal && native_snapshot.cursor_valid &&
        candidate_snapshot.cursor_valid &&
        native_snapshot.cursor_offset == candidate_snapshot.cursor_offset &&
        native_snapshot.frame_head == candidate_snapshot.frame_head)
        printf("differential %s: pass (cursor +0x%lx, %d callbacks)\n",
               name, candidate_snapshot.cursor_offset,
               candidate_snapshot.event_count);
}

static void test_unpatched_x86_differential(void)
{
    static const s32 ordinary_args[7] = {
        (s32)0x12345678UL, (s32)0x9abcdeffUL,
        (s32)0x123456abUL, (s32)0x123456cdUL,
        (s32)0x9876fedcUL, (s32)0x13579bdfUL, 0x20L
    };
    static const s32 mode4_args[7] = {1L, 2L, 3L, 4L, 5L, 6L, 4L};
    static const s32 blend_args[7] = {
        0x10L, 0x20L, 0x123L, 0x145L, 0x36L, 0x107L, 0x108L
    };
    static const s32 alternate_blend_args[7] = {
        0x41L, 0x52L, 0x134L, 0x256L, 0x78L, 0x234L, 0x34L
    };
    static const s32 edge_blend_args[4][7] = {
        {(s32)0xffffffe0UL, (s32)0x12345678UL, (s32)0x89abcdefUL,
         (s32)0xfedcba00UL, 0x30L, (s32)0xfffffff0UL, 3L},
        {(s32)0x80000000UL, (s32)0xdeadbeefUL, (s32)0xffffffffUL,
         (s32)0xffffffffUL, (s32)0x80000000UL, 1L, 3L},
        {1L, (s32)0xffff0000UL, 0x1feL, (s32)0x10000ffUL, 3L,
         (s32)0xffffff02UL, 3L},
        {0x7fffffffL, (s32)0x80000000UL, 0x80L, (s32)0x80000201UL,
         0x180L, 0x7ffffffeL, 3L}
    };

    run_differential_case("33620 ordinary seven-argument truncation",
                          0x33620UL, 0U, 0U, ordinary_args, 0, 0U,
                          0, 0U);
    run_differential_case("33620 arg7=4 extra packet through 2c980",
                          0x33620UL, 0U, 0U, mode4_args, 0, 0U,
                          0, 0U);
    run_differential_case("33420 ordinary mode gate off",
                          0x33420UL, 0U, 1U, ordinary_args, 0, 0U,
                          0, 0U);
    run_differential_case("33420 blend gate 0",
                          0x33420UL, 2U, 0U, blend_args, 0, 0U,
                          0, 0U);
    run_differential_case("33420 blend gate 1",
                          0x33420UL, 2U, 1U, blend_args, 0, 0U,
                          0, 0U);
    run_differential_case("33420 blend gate 127",
                          0x33420UL, 2U, 127U, blend_args, 0, 0U,
                          0, 0U);
    run_differential_case("33420 blend formulas with distinct coordinates",
                          0x33420UL, 2U, 1U, alternate_blend_args, 0, 0U,
                          0, 0U);
    run_differential_case("33420 blend unsigned edge vector 1",
                          0x33420UL, 2U, 1U, edge_blend_args[0], 0, 0U,
                          0, 0U);
    run_differential_case("33420 blend unsigned edge vector 2",
                          0x33420UL, 2U, 1U, edge_blend_args[1], 0, 0U,
                          0, 0U);
    run_differential_case("33420 blend unsigned edge vector 3",
                          0x33420UL, 2U, 1U, edge_blend_args[2], 0, 0U,
                          0, 0U);
    run_differential_case("33420 blend unsigned edge vector 4",
                          0x33420UL, 2U, 1U, edge_blend_args[3], 0, 0U,
                          0, 0U);
    run_differential_case("33420 blend gate 128",
                          0x33420UL, 2U, 128U, blend_args, 0, 0U,
                          0, 0U);

    /* These injected spy side effects are reload discriminators only; they
     * are not claims about the measured production callbacks. */
    run_differential_case("33620 callback cursor reload stress",
                          0x33620UL, 0U, 0U, mode4_args, 0, 0U,
                          1, 0x30U);
    run_differential_case("33420 entry cursor capture stress",
                          0x33420UL, 2U, 1U, blend_args, 1, 0x10U,
                          0, 0U);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("FAIL: provide the original ENG1.DLL path as argv[1]\n");
        return 2;
    }
    test_native_module_load(argv[1]);
    if (g_native_eng1 != NULL)
        set_native_callbacks();
    set_host_callbacks();
    test_basic_record();
    test_mode_four_extra_record();
    test_ordinary_33420_forwarding();
    test_blend_pair();
    test_unpatched_x86_differential();
    if (g_native_eng1 != NULL)
        FreeLibrary(g_native_eng1);
    if (g_failures != 0) {
        printf("draw-command fixture: %d failure(s)\n", g_failures);
        return 1;
    }
    printf("draw-command fixture: pass\n");
    return 0;
}
