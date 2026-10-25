#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/shared/engine_interface.h"

#define ENTRY_RVA 0x4ca0UL
#define INIT_RVA 0x39b0UL
#define FRAME_OWNER_SLOT_RVA 0x72870UL
#define LINK_ROWS_RVA 0x72880UL
#define COORDINATES_RVA 0x734a0UL
#define PENDING_RVA 0x6ee7cUL
#define CURSOR_RVA 0x6ee78UL
#define LINK_MAP_RVA 0x5e128UL
#define GROUP_SEQUENCE_RVA 0x5e138UL
#define DECODE_SCRATCH_RVA 0x6e660UL
#define LINK_ROWS_BYTES 0x0c00UL
#define FRAME_OWNER_BYTES 0x8000UL
#define FRAME_OWNER_ALLOC_BYTES 0x10000UL
#define COORDINATE_BYTES 0x40UL
#define DECODE_SCRATCH_BYTES 0x400UL
#define ALTERNATE_POOL_SLOT_RVA 0x734e0UL
#define MAX_EVENTS 8UL

typedef unsigned char BYTE1;
typedef unsigned short WORD1;
typedef unsigned long DWORD1;

typedef struct LinkRow {
    WORD1 live_mask;
    WORD1 descriptor;
    WORD1 successor[4];
} LinkRow;

extern WORD1 __cdecl eng1_target_04ca0(const BYTE1 *, DWORD1, DWORD1);

extern LinkRow g_actor_link_rows[256];
extern signed char g_actor_link_map[16];
extern BYTE1 g_actor_group_sequence_5e138[112];
void *g_frame_owner_72870;
extern WORD1 g_frame_upload_coordinates_734a0[32];
extern DWORD1 g_frame_upload_pending_6ee7c;
extern DWORD1 g_frame_upload_cursor_6ee78;
extern BYTE1 g_frame_decode_scratch_6e660[DECODE_SCRATCH_BYTES];
extern EngineInterface g_engine_interface;

/* Link-only storage for unrelated frame_commands COMDATs.  The fixture calls
 * dispatch_local_050d0 only; none of these cells is read by that function. */
unsigned long g_dispatch_callback_flag_71ec0;
unsigned long g_frame_global_73484;
void *g_frame_owner_70c80;
void *g_frame_owner_7348c;
unsigned long g_frame_global_73498;
unsigned char eng1_profile_rows_5df08[12UL * 28UL];
unsigned char *g_dispatch_frame_base_71ec8;
unsigned char *g_dispatch_frame_current_71ec4;
unsigned char *g_header_pointer_70d38;
signed char g_root_count_70ed1;

typedef struct CallbackEvent {
    DWORD1 kind;
    DWORD1 value;
    WORD1 rectangle[4];
    DWORD1 owner_offset;
    DWORD1 payload_hash;
} CallbackEvent;

static DWORD1 g_unexpected_incomplete;
static DWORD1 g_native_callbacks;
static BYTE1 *g_native_owner_for_callback;
static BYTE1 *g_source_owner_for_callback;
static CallbackEvent g_native_events[MAX_EVENTS];
static CallbackEvent g_source_events[MAX_EVENTS];
static DWORD1 g_native_event_count;
static DWORD1 g_source_event_count;

void __cdecl eng1_incomplete_behavior(DWORD1 rva, const char *reason)
{
    ++g_unexpected_incomplete;
    printf("unexpected candidate incomplete RVA=%08lx reason=%s\n", rva, reason);
}

static void __cdecl native_host_callback(DWORD1 value)
{
    CallbackEvent *event;
    ++g_native_callbacks;
    if (g_native_event_count >= MAX_EVENTS)
        return;
    event = &g_native_events[g_native_event_count++];
    memset(event, 0, sizeof(*event));
    event->kind = 1;
    event->value = value;
}

static void __cdecl source_host_callback(DWORD1 value)
{
    CallbackEvent *event;
    if (g_source_event_count >= MAX_EVENTS)
        return;
    event = &g_source_events[g_source_event_count++];
    memset(event, 0, sizeof(*event));
    event->kind = 1;
    event->value = value;
}

static DWORD1 payload_hash(const BYTE1 *bytes, DWORD1 size)
{
    DWORD1 i;
    DWORD1 hash;
    hash = 2166136261UL;
    for (i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 16777619UL;
    }
    return hash;
}

static void capture_upload(CallbackEvent *events, DWORD1 *event_count,
                           BYTE1 *owner, void *rectangle,
                           const void *texels)
{
    const WORD1 *words;
    const BYTE1 *pixels;
    DWORD1 bytes;
    DWORD1 i;
    CallbackEvent *event;
    if (*event_count >= MAX_EVENTS)
        return;
    event = &events[(*event_count)++];
    memset(event, 0, sizeof(*event));
    event->kind = 2;
    words = (const WORD1 *)rectangle;
    for (i = 0; i < 4; ++i)
        event->rectangle[i] = words[i];
    pixels = (const BYTE1 *)texels;
    event->owner_offset = (DWORD1)(pixels - owner);
    bytes = (DWORD1)words[3] * 32UL;
    event->payload_hash = payload_hash(pixels, bytes);
}

static void __cdecl native_upload_callback(void *rectangle,
                                           const void *texels)
{
    capture_upload(g_native_events, &g_native_event_count,
                   g_native_owner_for_callback, rectangle, texels);
}

static void __cdecl source_upload_callback(void *rectangle,
                                           const void *texels)
{
    capture_upload(g_source_events, &g_source_event_count,
                   g_source_owner_for_callback, rectangle, texels);
}

static int compare_events(const char *label)
{
    DWORD1 i;
    if (g_native_event_count != g_source_event_count) {
        printf("mismatch: %s callback count native=%lu source=%lu\n",
               label, g_native_event_count, g_source_event_count);
        return 0;
    }
    for (i = 0; i < g_native_event_count; ++i) {
        if (memcmp(&g_native_events[i], &g_source_events[i],
                   sizeof(CallbackEvent)) != 0) {
            printf("mismatch: %s callback event %lu native kind=%lu source kind=%lu\n",
                   label, i, g_native_events[i].kind,
                   g_source_events[i].kind);
            return 0;
        }
    }
    return 1;
}

static int flush_protocol_matches(void)
{
    CallbackEvent *first;
    CallbackEvent *second;
    if (g_native_event_count != 4UL)
        return 0;
    first = &g_native_events[1];
    second = &g_native_events[2];
    return g_native_events[0].kind == 1UL &&
           g_native_events[0].value == 0UL &&
           first->kind == 2UL && first->rectangle[0] == 0x0300U &&
           first->rectangle[1] == 0x0000U &&
           first->rectangle[2] == 0x0010U &&
           first->rectangle[3] == 0x0200U &&
           first->owner_offset == 0UL &&
           second->kind == 2UL && second->rectangle[0] == 0x0310U &&
           second->rectangle[1] == 0x0000U &&
           second->rectangle[2] == 0x0010U &&
           second->rectangle[3] == 0x0200U &&
           second->owner_offset == 0x4000UL &&
           g_native_events[3].kind == 1UL &&
           g_native_events[3].value == 0UL;
}

static void print_flush_events(void)
{
    DWORD1 i;
    for (i = 0; i < g_native_event_count; ++i) {
        CallbackEvent *event;
        event = &g_native_events[i];
        if (event->kind == 1UL) {
            printf("flush event %lu slot+0x16c(value=%08lx)\n",
                   i, event->value);
        } else {
            printf("flush event %lu slot+0x204(rect=%04x,%04x,%04x,%04x owner+%04lx payload-hash=%08lx)\n",
                   i, event->rectangle[0], event->rectangle[1],
                   event->rectangle[2], event->rectangle[3],
                   event->owner_offset, event->payload_hash);
        }
    }
}

static int compare_bytes(const char *label, const void *left,
                         const void *right, DWORD1 bytes)
{
    DWORD1 i;
    if (memcmp(left, right, bytes) == 0)
        return 1;
    printf("mismatch: %s (%lu bytes)\n", label, bytes);
    for (i = 0; i < bytes && i < 64UL; ++i) {
        if (((const BYTE1 *)left)[i] != ((const BYTE1 *)right)[i]) {
            printf(" first difference +%04lx native/source=%02x/%02x\n",
                   i, ((const BYTE1 *)left)[i], ((const BYTE1 *)right)[i]);
            break;
        }
    }
    return 0;
}

static int run_case(HMODULE image, BYTE1 code, const BYTE1 *program,
                    DWORD1 program_size, WORD1 expected_key)
{
    BYTE1 *base;
    BYTE1 *native_owner;
    BYTE1 *source_owner;
    BYTE1 *native_rows;
    BYTE1 *native_coordinates;
    BYTE1 *native_scratch;
    BYTE1 input[64];
    DWORD1 i;
    WORD1 native_result;
    WORD1 source_result;
    int checks;

    base = (BYTE1 *)image;
    native_rows = base + LINK_ROWS_RVA;
    native_coordinates = base + COORDINATES_RVA;
    native_scratch = base + DECODE_SCRATCH_RVA;
    native_owner = (BYTE1 *)VirtualAlloc(0, FRAME_OWNER_BYTES,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
    source_owner = (BYTE1 *)VirtualAlloc(0, FRAME_OWNER_BYTES,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
    if (native_owner == 0 || source_owner == 0)
        return 0;

    memset(native_owner, 0x5a, FRAME_OWNER_BYTES);
    memset(source_owner, 0x5a, FRAME_OWNER_BYTES);
    *(void **)(base + FRAME_OWNER_SLOT_RVA) = native_owner;
    g_frame_owner_72870 = source_owner;

    memset(native_rows, 0, LINK_ROWS_BYTES);
    memset(native_coordinates, 0, COORDINATE_BYTES);
    *(DWORD1 *)(base + PENDING_RVA) = 0;
    *(DWORD1 *)(base + CURSOR_RVA) = 0;
    memset(native_scratch, 0, DECODE_SCRATCH_BYTES);

    /* Seed the same measured post-0x39b0 row state in both implementations. */
    for (i = 0; i < 256; ++i) {
        WORD1 descriptor;
        descriptor = (WORD1)(0x008cU | ((i & 8U) << 1) |
                             ((i & 0x80U) >> 6));
        ((LinkRow *)native_rows)[i].descriptor = descriptor;
    }
    ((LinkRow *)native_rows)[0].live_mask = 0x000fU;
    ((LinkRow *)native_rows)[16].live_mask = 0x000fU;
    ((LinkRow *)native_rows)[4].live_mask = 0x000fU;
    ((LinkRow *)native_rows)[1].live_mask = 0x000fU;
    memcpy(g_actor_link_rows, native_rows, LINK_ROWS_BYTES);
    memcpy(g_frame_upload_coordinates_734a0, native_coordinates,
           COORDINATE_BYTES);
    memset(&g_frame_upload_pending_6ee7c, 0,
           sizeof(g_frame_upload_pending_6ee7c));
    memset(&g_frame_upload_cursor_6ee78, 0, sizeof(g_frame_upload_cursor_6ee78));
    memset(g_frame_decode_scratch_6e660, 0, DECODE_SCRATCH_BYTES);
    memset(input, 0, sizeof(input));
    input[0] = code;
    if (program_size > sizeof(input) - 4)
        return 0;
    memcpy(input + 4, program, program_size);
    g_unexpected_incomplete = 0;
    g_native_callbacks = 0;
    *(void (__cdecl **)(DWORD1))(base + 0x70f20UL + 0x16cUL) =
        native_host_callback;

    __try {
        native_result = ((WORD1 (__cdecl *)(BYTE1 *, DWORD1, DWORD1))
                         (base + ENTRY_RVA))(input, 1UL, 0x40UL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("native exception code=%08lx code=%02x\n",
               GetExceptionCode(), code);
        VirtualFree(native_owner, 0, MEM_RELEASE);
        VirtualFree(source_owner, 0, MEM_RELEASE);
        return 0;
    }

    source_result = eng1_target_04ca0(input, 1UL, 0x40UL);
    checks = 0;
    checks += native_result == source_result;
    if (native_result != source_result)
        printf("mismatch: return native=%04x source=%04x\n",
               native_result, source_result);
    checks += source_result == expected_key;
    if (source_result != expected_key)
        printf("unexpected key: expected=%04x got=%04x\n",
               expected_key, source_result);
    checks += compare_bytes("link rows", native_rows, g_actor_link_rows,
                            LINK_ROWS_BYTES);
    checks += compare_bytes("coordinates", native_coordinates,
                            g_frame_upload_coordinates_734a0,
                            COORDINATE_BYTES);
    checks += compare_bytes("pending", base + PENDING_RVA,
                            &g_frame_upload_pending_6ee7c,
                            sizeof(g_frame_upload_pending_6ee7c));
    checks += compare_bytes("cursor", base + CURSOR_RVA,
                            &g_frame_upload_cursor_6ee78,
                            sizeof(g_frame_upload_cursor_6ee78));
    checks += compare_bytes("decode scratch", native_scratch,
                            g_frame_decode_scratch_6e660,
                            DECODE_SCRATCH_BYTES);
    checks += compare_bytes("frame owner", native_owner, source_owner,
                            FRAME_OWNER_BYTES);
    checks += g_native_callbacks == 0;
    checks += g_unexpected_incomplete == 0;
    printf("case code=%02x native=%04x source=%04x pending=%lu cursor=%lu checks=%d/10 callbacks=%lu incomplete=%lu\n",
           code, native_result, source_result,
           g_frame_upload_pending_6ee7c, g_frame_upload_cursor_6ee78,
           checks, g_native_callbacks, g_unexpected_incomplete);

    VirtualFree(native_owner, 0, MEM_RELEASE);
    VirtualFree(source_owner, 0, MEM_RELEASE);
    return checks == 10;
}

static int run_flush_case(HMODULE image)
{
    BYTE1 *base;
    BYTE1 *native_owner;
    BYTE1 *source_owner;
    BYTE1 *native_rows;
    BYTE1 *native_coordinates;
    BYTE1 input[8];
    DWORD1 i;
    DWORD1 native_pending;
    DWORD1 old_pool_slot;
    WORD1 native_spill;
    WORD1 native_result;
    WORD1 source_result;
    int native_exception;
    int checks;

    base = (BYTE1 *)image;
    native_rows = base + LINK_ROWS_RVA;
    native_coordinates = base + COORDINATES_RVA;
    native_owner = (BYTE1 *)VirtualAlloc(0, FRAME_OWNER_ALLOC_BYTES,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
    source_owner = (BYTE1 *)VirtualAlloc(0, FRAME_OWNER_ALLOC_BYTES,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
    if (native_owner == 0 || source_owner == 0)
        return 0;

    memset(native_owner, 0x5a, FRAME_OWNER_BYTES);
    memset(native_owner + FRAME_OWNER_BYTES, 0xa5,
           FRAME_OWNER_ALLOC_BYTES - FRAME_OWNER_BYTES);
    memset(source_owner, 0x5a, FRAME_OWNER_BYTES);
    memset(source_owner + FRAME_OWNER_BYTES, 0xa5,
           FRAME_OWNER_ALLOC_BYTES - FRAME_OWNER_BYTES);
    *(void **)(base + FRAME_OWNER_SLOT_RVA) = native_owner;
    g_frame_owner_72870 = source_owner;

    memset(native_rows, 0, LINK_ROWS_BYTES);
    memset(native_coordinates, 0, COORDINATE_BYTES);
    for (i = 0; i < 256; ++i) {
        WORD1 descriptor;
        descriptor = (WORD1)(0x008cU | ((i & 8U) << 1) |
                             ((i & 0x80U) >> 6));
        ((LinkRow *)native_rows)[i].descriptor = descriptor;
    }
    for (i = 0; i < 32; ++i) {
        ((LinkRow *)native_rows)[i].live_mask = 0x000fU;
        ((WORD1 *)native_coordinates)[i] = (WORD1)i;
    }
    memcpy(g_actor_link_rows, native_rows, LINK_ROWS_BYTES);
    memcpy(g_frame_upload_coordinates_734a0, native_coordinates,
           COORDINATE_BYTES);
    *(DWORD1 *)(base + PENDING_RVA) = 32UL;
    *(DWORD1 *)(base + CURSOR_RVA) = 0;
    g_frame_upload_pending_6ee7c = 32UL;
    g_frame_upload_cursor_6ee78 = 0;
    memset(base + DECODE_SCRATCH_RVA, 0, DECODE_SCRATCH_BYTES);
    memset(g_frame_decode_scratch_6e660, 0, DECODE_SCRATCH_BYTES);

    memset(input, 0, sizeof(input));
    input[0] = 0;
    input[4] = 1;
    input[5] = 0xa5;
    input[6] = 2;

    memset(g_native_events, 0, sizeof(g_native_events));
    memset(g_source_events, 0, sizeof(g_source_events));
    g_native_event_count = 0;
    g_source_event_count = 0;
    g_native_callbacks = 0;
    g_unexpected_incomplete = 0;
    g_native_owner_for_callback = native_owner;
    g_source_owner_for_callback = source_owner;
    g_engine_interface.dispatch_2c630_16c = source_host_callback;
    g_engine_interface.opaque_1fc[2] =
        (DWORD1)source_upload_callback;
    *(void (__cdecl **)(DWORD1))(base + 0x70f20UL + 0x16cUL) =
        native_host_callback;
    *(void (__cdecl **)(void *, const void *))
        (base + 0x70f20UL + 0x204UL) = native_upload_callback;

    old_pool_slot = *(DWORD1 *)(base + ALTERNATE_POOL_SLOT_RVA);
    *(DWORD1 *)(base + ALTERNATE_POOL_SLOT_RVA) = 0x56781234UL;
    native_exception = 0;
    __try {
        native_result = ((WORD1 (__cdecl *)(BYTE1 *, DWORD1, DWORD1))
                         (base + ENTRY_RVA))(input, 1UL, 0x40UL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        native_exception = (int)GetExceptionCode();
        native_result = 0xffffU;
    }
    native_pending = *(DWORD1 *)(base + PENDING_RVA);
    native_spill = *(WORD1 *)(base + ALTERNATE_POOL_SLOT_RVA);

    source_result = eng1_target_04ca0(input, 1UL, 0x40UL);
    checks = 0;
    checks += native_exception == 0;
    checks += compare_events("32-entry flush");
    checks += g_native_event_count == 4UL;
    checks += flush_protocol_matches();
    checks += g_native_callbacks == 2UL;
    checks += native_result == source_result;
    checks += g_unexpected_incomplete == 0UL;
    checks += g_frame_upload_pending_6ee7c == native_pending;
    checks += native_pending == 1UL;
    checks += native_spill == 0x1234U;
    checks += compare_bytes("flushed coordinate prefix",
                            native_coordinates,
                            g_frame_upload_coordinates_734a0,
                            COORDINATE_BYTES);
    checks += compare_bytes("flushed link rows", native_rows,
                            g_actor_link_rows, LINK_ROWS_BYTES);
    checks += compare_bytes("post-flush frame owner", native_owner,
                            source_owner, FRAME_OWNER_BYTES);
    checks += compare_bytes("post-flush decode scratch",
                            base + DECODE_SCRATCH_RVA,
                            g_frame_decode_scratch_6e660,
                            DECODE_SCRATCH_BYTES);
    checks += compare_bytes("post-flush cursor", base + CURSOR_RVA,
                            &g_frame_upload_cursor_6ee78,
                            sizeof(g_frame_upload_cursor_6ee78));
    print_flush_events();
    printf("flush case native=%04x source=%04x callbacks=%lu pending native/source=%lu/%lu alternate-pool-low native=%04x checks=%d/15\n",
           native_result, source_result, g_native_event_count,
           native_pending, g_frame_upload_pending_6ee7c, native_spill,
           checks);

    *(DWORD1 *)(base + ALTERNATE_POOL_SLOT_RVA) = old_pool_slot;
    VirtualFree(native_owner, 0, MEM_RELEASE);
    VirtualFree(source_owner, 0, MEM_RELEASE);
    return checks == 15;
}

static int run_actual_case(HMODULE image, const BYTE1 *input,
                           DWORD1 input_size)
{
    BYTE1 *base;
    BYTE1 *native_owner;
    BYTE1 *source_owner;
    BYTE1 *native_rows;
    BYTE1 *native_coordinates;
    BYTE1 *native_scratch;
    DWORD1 i;
    WORD1 native_result;
    WORD1 source_result;
    int checks;

    base = (BYTE1 *)image;
    native_rows = base + LINK_ROWS_RVA;
    native_coordinates = base + COORDINATES_RVA;
    native_scratch = base + DECODE_SCRATCH_RVA;
    if (input_size != 128UL)
        return 0;
    native_owner = (BYTE1 *)VirtualAlloc(0, FRAME_OWNER_BYTES,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
    source_owner = (BYTE1 *)VirtualAlloc(0, FRAME_OWNER_BYTES,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
    if (native_owner == 0 || source_owner == 0)
        return 0;

    memset(native_owner, 0x5a, FRAME_OWNER_BYTES);
    memset(source_owner, 0x5a, FRAME_OWNER_BYTES);
    *(void **)(base + FRAME_OWNER_SLOT_RVA) = native_owner;
    g_frame_owner_72870 = source_owner;
    memset(native_rows, 0, LINK_ROWS_BYTES);
    memset(native_coordinates, 0, COORDINATE_BYTES);
    *(DWORD1 *)(base + PENDING_RVA) = 0;
    *(DWORD1 *)(base + CURSOR_RVA) = 0;
    memset(native_scratch, 0, DECODE_SCRATCH_BYTES);

    for (i = 0; i < 256; ++i) {
        WORD1 descriptor;
        descriptor = (WORD1)(0x008cU | ((i & 8U) << 1) |
                             ((i & 0x80U) >> 6));
        ((LinkRow *)native_rows)[i].descriptor = descriptor;
    }
    ((LinkRow *)native_rows)[0].live_mask = 0x000fU;
    ((LinkRow *)native_rows)[16].live_mask = 0x000fU;
    ((LinkRow *)native_rows)[4].live_mask = 0x000fU;
    ((LinkRow *)native_rows)[1].live_mask = 0x000fU;
    memcpy(g_actor_link_rows, native_rows, LINK_ROWS_BYTES);
    memcpy(g_frame_upload_coordinates_734a0, native_coordinates,
           COORDINATE_BYTES);
    g_frame_upload_pending_6ee7c = 0;
    g_frame_upload_cursor_6ee78 = 0;
    memset(g_frame_decode_scratch_6e660, 0, DECODE_SCRATCH_BYTES);
    g_native_callbacks = 0;
    g_unexpected_incomplete = 0;
    *(void (__cdecl **)(DWORD1))(base + 0x70f20UL + 0x16cUL) =
        native_host_callback;

    __try {
        native_result = ((WORD1 (__cdecl *)(const BYTE1 *, DWORD1, DWORD1))
                         (base + ENTRY_RVA))(input, 1UL, 0x40UL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("actual native exception code=%08lx\n", GetExceptionCode());
        VirtualFree(native_owner, 0, MEM_RELEASE);
        VirtualFree(source_owner, 0, MEM_RELEASE);
        return 0;
    }
    source_result = eng1_target_04ca0(input, 1UL, 0x40UL);

    checks = 0;
    checks += native_result == source_result;
    if (native_result != source_result)
        printf("actual result mismatch native=%04x source=%04x\n",
               native_result, source_result);
    checks += compare_bytes("actual link rows", native_rows,
                            g_actor_link_rows, LINK_ROWS_BYTES);
    checks += compare_bytes("actual coordinates", native_coordinates,
                            g_frame_upload_coordinates_734a0,
                            COORDINATE_BYTES);
    checks += compare_bytes("actual pending", base + PENDING_RVA,
                            &g_frame_upload_pending_6ee7c,
                            sizeof(g_frame_upload_pending_6ee7c));
    checks += compare_bytes("actual cursor", base + CURSOR_RVA,
                            &g_frame_upload_cursor_6ee78,
                            sizeof(g_frame_upload_cursor_6ee78));
    checks += compare_bytes("actual decode scratch", native_scratch,
                            g_frame_decode_scratch_6e660,
                            DECODE_SCRATCH_BYTES);
    checks += compare_bytes("actual frame owner", native_owner,
                            source_owner, FRAME_OWNER_BYTES);
    checks += g_native_callbacks == 0;
    checks += g_unexpected_incomplete == 0;
    printf("actual case native=%04x source=%04x checks=%d/9 prefix_bytes=%lu\n",
           native_result, source_result, checks, input_size);

    VirtualFree(native_owner, 0, MEM_RELEASE);
    VirtualFree(source_owner, 0, MEM_RELEASE);
    return checks == 9;
}

static int run_flush_chain_case(HMODULE image)
{
    BYTE1 *base;
    BYTE1 *native_owner;
    BYTE1 *source_owner;
    BYTE1 *native_rows;
    BYTE1 *native_coordinates;
    BYTE1 input[64];
    DWORD1 i;
    WORD1 native_result;
    WORD1 source_result;
    int checks;

    base = (BYTE1 *)image;
    native_rows = base + LINK_ROWS_RVA;
    native_coordinates = base + COORDINATES_RVA;
    native_owner = (BYTE1 *)VirtualAlloc(0, FRAME_OWNER_BYTES,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
    source_owner = (BYTE1 *)VirtualAlloc(0, FRAME_OWNER_BYTES,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
    if (native_owner == 0 || source_owner == 0)
        return 0;
    memset(native_owner, 0x5a, FRAME_OWNER_BYTES);
    memset(source_owner, 0x5a, FRAME_OWNER_BYTES);
    *(void **)(base + FRAME_OWNER_SLOT_RVA) = native_owner;
    g_frame_owner_72870 = source_owner;

    memset(native_rows, 0, LINK_ROWS_BYTES);
    memset(native_coordinates, 0, COORDINATE_BYTES);
    for (i = 0; i < 256; ++i) {
        WORD1 descriptor;
        descriptor = (WORD1)(0x008cU | ((i & 8U) << 1) |
                             ((i & 0x80U) >> 6));
        ((LinkRow *)native_rows)[i].descriptor = descriptor;
    }
    for (i = 0; i < 31; ++i) {
        ((LinkRow *)native_rows)[i].live_mask = 0x000fU;
        ((WORD1 *)native_coordinates)[i] = (WORD1)i;
    }
    memcpy(g_actor_link_rows, native_rows, LINK_ROWS_BYTES);
    memcpy(g_frame_upload_coordinates_734a0, native_coordinates,
           COORDINATE_BYTES);
    *(DWORD1 *)(base + PENDING_RVA) = 31UL;
    *(DWORD1 *)(base + CURSOR_RVA) = 0;
    g_frame_upload_pending_6ee7c = 31UL;
    g_frame_upload_cursor_6ee78 = 0;
    memset(base + DECODE_SCRATCH_RVA, 0, DECODE_SCRATCH_BYTES);
    memset(g_frame_decode_scratch_6e660, 0, DECODE_SCRATCH_BYTES);

    memset(input, 0, sizeof(input));
    input[0] = 10;
    input[4] = 0;
    for (i = 0; i < 4; ++i) {
        input[8 + i * 2] = 0x7f;
        input[9 + i * 2] = 0x5a;
    }
    input[16] = 0x44;
    input[17] = 0x5a;
    input[18] = 0x02;
    input[19] = 0x01;
    input[20] = 0x33;
    input[21] = 0x02;

    memset(g_native_events, 0, sizeof(g_native_events));
    memset(g_source_events, 0, sizeof(g_source_events));
    g_native_event_count = 0;
    g_source_event_count = 0;
    g_native_callbacks = 0;
    g_unexpected_incomplete = 0;
    g_native_owner_for_callback = native_owner;
    g_source_owner_for_callback = source_owner;
    g_engine_interface.dispatch_2c630_16c = source_host_callback;
    g_engine_interface.opaque_1fc[2] =
        (DWORD1)source_upload_callback;
    *(void (__cdecl **)(DWORD1))(base + 0x70f20UL + 0x16cUL) =
        native_host_callback;
    *(void (__cdecl **)(void *, const void *))
        (base + 0x70f20UL + 0x204UL) = native_upload_callback;

    __try {
        native_result = ((WORD1 (__cdecl *)(BYTE1 *, DWORD1, DWORD1))
                         (base + ENTRY_RVA))(input, 2UL, 0x40UL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("chain flush native exception code=%08lx\n",
               GetExceptionCode());
        VirtualFree(native_owner, 0, MEM_RELEASE);
        VirtualFree(source_owner, 0, MEM_RELEASE);
        return 0;
    }
    source_result = eng1_target_04ca0(input, 2UL, 0x40UL);

    checks = 0;
    checks += native_result == source_result;
    checks += native_result == 0x076fU;
    checks += compare_events("multi-record 32-entry flush");
    checks += g_native_event_count == g_source_event_count;
    checks += g_native_callbacks == 2UL;
    checks += g_unexpected_incomplete == 0UL;
    checks += compare_bytes("chain-flush link rows", native_rows,
                            g_actor_link_rows, LINK_ROWS_BYTES);
    checks += compare_bytes("chain-flush coordinates", native_coordinates,
                            g_frame_upload_coordinates_734a0,
                            COORDINATE_BYTES);
    checks += compare_bytes("chain-flush pending", base + PENDING_RVA,
                            &g_frame_upload_pending_6ee7c,
                            sizeof(g_frame_upload_pending_6ee7c));
    checks += compare_bytes("chain-flush cursor", base + CURSOR_RVA,
                            &g_frame_upload_cursor_6ee78,
                            sizeof(g_frame_upload_cursor_6ee78));
    checks += compare_bytes("chain-flush scratch",
                            base + DECODE_SCRATCH_RVA,
                            g_frame_decode_scratch_6e660,
                            DECODE_SCRATCH_BYTES);
    checks += compare_bytes("chain-flush frame owner", native_owner,
                            source_owner, FRAME_OWNER_BYTES);
    printf("chain-flush case native=%04x source=%04x callbacks=%lu pending=%lu checks=%d/12\n",
           native_result, source_result, g_native_event_count,
           g_frame_upload_pending_6ee7c, checks);
    print_flush_events();

    VirtualFree(native_owner, 0, MEM_RELEASE);
    VirtualFree(source_owner, 0, MEM_RELEASE);
    return checks == 12;
}

int main(int argc, char **argv)
{
    HMODULE image;
    const BYTE1 program[] = {1, 0xa5, 2};
    BYTE1 actual_input[128];
    FILE *actual_file;
    DWORD1 passed;
    DWORD1 expected;

    if (argc != 2 && argc != 3) {
        puts("usage: native_source_diff ENG1.DLL [captured_payload.bin]");
        return 2;
    }
    if (argc == 3) {
    actual_file = fopen(argv[2], "rb");
    if (actual_file == 0 || fread(actual_input, 1, sizeof(actual_input),
                                  actual_file) != sizeof(actual_input)) {
        puts("could not read the captured 128-byte native input prefix");
        if (actual_file != 0)
            fclose(actual_file);
        return 2;
    }
    fclose(actual_file);
    }
    image = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (image == 0) {
        printf("LoadLibraryEx failed: %lu\n", GetLastError());
        return 3;
    }

    if (memcmp(g_actor_link_map,
               (BYTE1 *)image + LINK_MAP_RVA,
               sizeof(g_actor_link_map)) != 0 ||
        memcmp(g_actor_group_sequence_5e138,
               (BYTE1 *)image + GROUP_SEQUENCE_RVA,
               sizeof(g_actor_group_sequence_5e138)) != 0) {
        puts("measured source lookup providers differ from the native data image");
        FreeLibrary(image);
        return 4;
    }

    passed = 0;
    passed += run_case(image, 0, program, sizeof(program), 0x0021U);
    passed += run_case(image, 2, program, sizeof(program), 0x0023U);
    passed += run_case(image, 4, program, sizeof(program), 0x0021U);
    passed += run_case(image, 6, program, sizeof(program), 0x0023U);
    passed += run_flush_case(image);
    passed += run_flush_chain_case(image);
    expected = 6;
    if (argc == 3) {
        passed += run_actual_case(image, actual_input, sizeof(actual_input));
        expected = 7;
    }
    printf("native/source 0x4ca0 cases=%lu/%lu\n", passed, expected);
    FreeLibrary(image);
    return passed == expected ? 0 : 1;
}
