/* Experimental x86 host-interface shim. See ../include/hercules_shim.h.
 * Modern MSVC x86 only: the relay is naked inline assembly and the backend
 * firewall uses structured exception handling. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <float.h>
#include <intrin.h>
#include <string.h>
#include <xmmintrin.h>

#include "hercules_frame.h"
#include "hercules_shim.h"

#define HS_EXTENTS 8u
#define HS_DEFAULT_VISITS 0x4000u
#define HS_MAX_VISITS 0x10000u

/* Relayed slots: the renderer_contract clone allowlist minus 174/175. */
static const renderer_u32 k_slots[] = {
    HERCULES_SLOT_CHAIN_INIT, HERCULES_SLOT_SUBMIT,
    HERCULES_SLOT_POINT_INPUT, HERCULES_SLOT_BATCH_POINT_INPUT,
    HERCULES_SLOT_PROJECT, HERCULES_SLOT_BATCH_PROJECT,
    HERCULES_SLOT_SCREEN_OUTPUT, HERCULES_SLOT_FT4_SCREEN_COPY,
    HERCULES_SLOT_UPLOAD_RECT, HERCULES_SLOT_UPLOAD_PAGE,
    HERCULES_SLOT_MOVE_RECT, HERCULES_SLOT_COLOR_REMAP,
    HERCULES_SLOT_MOVIE_DISPLAY
};
#define HS_SLOT_COUNT (sizeof(k_slots) / sizeof(k_slots[0]))

typedef struct HsExtent {
    renderer_raw32 first_slot;
    renderer_u32 slot_count;
} HsExtent;

static renderer_u32 g_clone[ENGINE_TABLE_WORD_COUNT];
static renderer_u32 g_original[ENGINE_TABLE_WORD_COUNT];
static volatile LONG g_installed;
static HsConfig g_config;
static HsBackendV1 g_backend;
static int g_have_backend;
static volatile LONG g_backend_disabled;
static volatile LONG g_sequence;
static HsStats g_stats;
static HsExtent g_extents[HS_EXTENTS];
static renderer_u32 g_extent_next;
static LONG g_submit_depth;

/* Pre-walk bookkeeping for the current slot-89 bracket. */
static renderer_raw32 g_visits[HS_MAX_VISITS];
static renderer_raw32 g_recheck[HS_MAX_VISITS];
static renderer_u32 g_frame;
static renderer_u32 g_prewalk;
static renderer_u32 g_prewalk_count;
static RendererAc60StateView g_model;

static renderer_u32 visit_capacity(void)
{
    renderer_u32 c = g_config.visit_capacity ? g_config.visit_capacity : HS_DEFAULT_VISITS;
    return c > HS_MAX_VISITS ? HS_MAX_VISITS : c;
}

/* ---- backend firewall ------------------------------------------------- */

typedef __declspec(align(16)) struct HsFpuState {
    unsigned char image[512];
} HsFpuState;

static void fpu_save(HsFpuState *s)
{
    _fxsave(s->image);
}

static void fpu_restore(const HsFpuState *s)
{
    HsFpuState current;
    _fxsave(current.image);
    if (memcmp(current.image, s->image, 6) != 0 ||
        memcmp(current.image + 24, s->image + 24, 4) != 0 ||
        memcmp(current.image + 32, s->image + 32, 128) != 0)
        g_stats.fpu_restores++;
    /* Restore even when controls agree: x87 tags, pending exception flags and
     * register values (including a host return in ST0) are part of the state.
     * Modern Windows x86 targets support FXSAVE/FXRSTOR and SSE. */
    _fxrstor(s->image);
}

static int backend_live(void)
{
    return g_have_backend && g_config.mode == HS_MODE_OBSERVE && !g_backend_disabled;
}

static int backend_filter(DWORD code)
{
    g_stats.backend_faults++;
    g_stats.last_fault_code = code;
    InterlockedExchange(&g_backend_disabled, 1);
    g_stats.backend_disabled = 1;
    return EXCEPTION_EXECUTE_HANDLER;
}

/* Every backend entry goes through one of these. A fault (access violation,
 * C++ throw 0xE06D7363, RaiseException, ...) disables the backend; the host
 * never sees it. Without this, a fault inside the engine tick reaches the
 * OnIdle __except at host 0x407A27 and becomes the fatal exit(-123) path. */
static void call_service(const HsServiceEvent *e)
{
    HsFpuState fpu;
    if (!backend_live() || !g_backend.service) return;
    fpu_save(&fpu);
    __try { g_backend.service(g_backend.context, e); }
    __except (backend_filter(GetExceptionCode())) {}
    fpu_restore(&fpu);
}

static void call_frame_begin(const HsFrameBegin *f)
{
    HsFpuState fpu;
    if (!backend_live() || !g_backend.frame_begin) return;
    fpu_save(&fpu);
    __try { g_backend.frame_begin(g_backend.context, f); }
    __except (backend_filter(GetExceptionCode())) {}
    fpu_restore(&fpu);
}

static void call_primitive(const HsPrimitive *p)
{
    HsFpuState fpu;
    if (!backend_live() || !g_backend.primitive) return;
    fpu_save(&fpu);
    __try { g_backend.primitive(g_backend.context, p); }
    __except (backend_filter(GetExceptionCode())) {}
    fpu_restore(&fpu);
}

static void call_frame_end(const HsFrameEnd *f)
{
    HsFpuState fpu;
    if (!backend_live() || !g_backend.frame_end) return;
    fpu_save(&fpu);
    __try { g_backend.frame_end(g_backend.context, f); }
    __except (backend_filter(GetExceptionCode())) {}
    fpu_restore(&fpu);
}

/* ---- order-table pre-walk --------------------------------------------- */

/* In-process x86: raw32 tokens are this process's addresses. Faults while
 * reading are contained by the caller's __try; the original then performs
 * its own reads unchanged. */
static int live_resolve(void *context, renderer_raw32 address,
                        renderer_u32 byte_count, const renderer_u8 **bytes)
{
    (void)context;
    if (address == 0 || (renderer_u32)(address + byte_count) < address)
        return 0;
    *bytes = (const renderer_u8 *)(UINT_PTR)address;
    return 1;
}

typedef struct HsVisitContext {
    renderer_u32 count;
    renderer_raw32 *record;
    int emit;
} HsVisitContext;

static int on_primitive(void *context, renderer_raw32 node)
{
    HsVisitContext *v = (HsVisitContext *)context;
    HsPrimitive p;
    const renderer_u8 *bytes = (const renderer_u8 *)(UINT_PTR)node;
    renderer_u16 span;

    v->record[v->count++] = node;
    if (!v->emit)
        return 1;
    memset(&p, 0, sizeof(p));
    p.frame = g_frame;
    p.index = v->count - 1;
    p.node = node;
    /* Decode the 8-byte header first, then exactly AC60's capture span. */
    p.decode_status = renderer_decode_packet(bytes, 8, &p.decoded);
    span = renderer_packet_capture_size(&p.decoded);
    if (span > 8)
        p.decode_status = renderer_decode_packet(bytes, span, &p.decoded);
    p.apply_status = renderer_ac60_apply_packet(bytes, span ? span : 8, &g_model, &p.staged);
    p.state_after = g_model;
    g_stats.primitives++;
    call_primitive(&p);
    return 1;
}

static FrameWalkStatus walk(renderer_raw32 first, renderer_u32 slots,
                            renderer_raw32 *record, int emit, renderer_u32 *count,
                            int *faulted)
{
    FrameVisitGuard guard;
    HsVisitContext ctx;
    FrameWalkStatus status = FRAME_WALK_INVALID_ARGUMENT;
    static renderer_raw32 guard_buffer[HS_MAX_VISITS];

    ctx.count = 0;
    ctx.record = record;
    ctx.emit = emit;
    *faulted = 0;
    frame_visit_guard_init(&guard, guard_buffer, visit_capacity());
    __try {
        status = frame_walk_ac60_order_table_visit(live_resolve, NULL, first, slots,
                                                   &guard, on_primitive, &ctx);
    } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                    ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        *faulted = 1;
    }
    *count = ctx.count;
    return status;
}

static const HsExtent *find_extent(renderer_raw32 first)
{
    renderer_u32 i;
    for (i = 0; i < HS_EXTENTS; ++i)
        if (g_extents[i].slot_count && g_extents[i].first_slot == first)
            return &g_extents[i];
    return NULL;
}

static void record_extent(renderer_raw32 first, renderer_u32 count)
{
    renderer_u32 i;
    for (i = 0; i < HS_EXTENTS; ++i) {
        if (g_extents[i].first_slot == first) {
            g_extents[i].slot_count = count;
            return;
        }
    }
    g_extents[g_extent_next].first_slot = first;
    g_extents[g_extent_next].slot_count = count;
    g_extent_next = (g_extent_next + 1) % HS_EXTENTS;
}

static void submit_enter(renderer_raw32 first)
{
    const HsExtent *extent;
    HsFrameBegin begin;
    FrameWalkStatus status;
    int faulted;

    g_frame = ++g_stats.frames;
    g_prewalk_count = 0;
    if (InterlockedIncrement(&g_submit_depth) != 1) {
        g_prewalk = HS_PREWALK_SKIP_NESTED;
        return;
    }
    if (!backend_live()) {
        g_prewalk = HS_PREWALK_SKIP_MODE;
        return;
    }
    if (!g_config.primary_callback_cell || *g_config.primary_callback_cell != 0) {
        g_prewalk = HS_PREWALK_SKIP_PRIMARY_CALLBACK;
        return;
    }
    extent = find_extent(first);
    if (!extent) {
        g_prewalk = HS_PREWALK_SKIP_UNKNOWN_EXTENT;
        return;
    }
    begin.frame = g_frame;
    begin.thread_id = GetCurrentThreadId();
    begin.first_slot = first;
    begin.slot_count = extent->slot_count;
    call_frame_begin(&begin);
    status = walk(first, extent->slot_count, g_visits, 1, &g_prewalk_count, &faulted);
    g_prewalk = faulted ? HS_PREWALK_SKIP_WALK_FAULT
              : status != FRAME_WALK_OK ? HS_PREWALK_SKIP_WALK_ERROR : HS_PREWALK_DONE;
}

static void submit_leave(renderer_raw32 first)
{
    HsFrameEnd end;
    renderer_u32 count = 0;
    int faulted = 0;

    end.frame = g_frame;
    end.primitives = g_prewalk_count;
    end.prewalk = g_prewalk;
    end.consumption_consistent = 0;
    if (g_prewalk == HS_PREWALK_DONE) {
        /* Falsifier for "pre-walk == consumption": the original must leave
         * every link it consumed unchanged. Any difference is reported. */
        const HsExtent *extent = find_extent(first);
        FrameWalkStatus status = walk(first, extent->slot_count, g_recheck, 0, &count, &faulted);
        end.consumption_consistent = !faulted && status == FRAME_WALK_OK &&
            count == g_prewalk_count &&
            memcmp(g_recheck, g_visits, count * sizeof(renderer_raw32)) == 0;
        if (!end.consumption_consistent)
            g_stats.consumption_mismatches++;
        call_frame_end(&end);
    }
    g_stats.prewalk_skips[g_prewalk]++;
    InterlockedDecrement(&g_submit_depth);
}

/* ---- relay entry/exit (called from the naked relay) ------------------- */

renderer_u32 __cdecl hs_enter(renderer_u32 slot, const renderer_u32 *stack)
{
    DWORD last_error = GetLastError();
    HsServiceEvent e;
    renderer_u32 i;

    g_stats.service_calls++;
    if (slot == HERCULES_SLOT_CHAIN_INIT)
        record_extent(stack[0], stack[1]);
    if (backend_live()) {
        memset(&e, 0, sizeof(e));
        e.sequence = (renderer_u32)InterlockedIncrement(&g_sequence);
        e.thread_id = GetCurrentThreadId();
        e.slot = slot;
        e.phase = 0;
        for (i = 0; i < HS_RELAY_STACK_WORDS; ++i)
            e.stack_words[i] = stack[i];
        e.target = g_original[slot];
        call_service(&e);
    }
    if (slot == HERCULES_SLOT_SUBMIT)
        submit_enter(stack[0]);
    SetLastError(last_error);
    return g_original[slot];
}

void __cdecl hs_leave(renderer_u32 slot, const renderer_u32 *stack,
                      renderer_u32 raw_eax, renderer_u32 raw_edx)
{
    DWORD last_error = GetLastError();
    HsServiceEvent e;
    renderer_u32 i;

    if (slot == HERCULES_SLOT_SUBMIT)
        submit_leave(stack[0]);
    if (backend_live()) {
        memset(&e, 0, sizeof(e));
        e.sequence = (renderer_u32)InterlockedIncrement(&g_sequence);
        e.thread_id = GetCurrentThreadId();
        e.slot = slot;
        e.phase = 1;
        for (i = 0; i < HS_RELAY_STACK_WORDS; ++i)
            e.stack_words[i] = stack[i];
        e.raw_eax = raw_eax;
        e.raw_edx = raw_edx;
        e.target = g_original[slot];
        call_service(&e);
    }
    SetLastError(last_error);
}

/* The original's own exceptions are never caught here; they keep unwinding
 * to the host's handlers (OnIdle's __except on the real host). The
 * termination handler only closes a slot-89 bracket that unwinding abandons. */
static void hs_abandon(renderer_u32 slot)
{
    if (slot == HERCULES_SLOT_SUBMIT) {
        InterlockedDecrement(&g_submit_depth);
        g_stats.abandoned_brackets++;
    }
}

renderer_u32 __cdecl hs_invoke(renderer_u32 slot, const renderer_u32 *stack,
                               renderer_u32 in_ecx, renderer_u32 in_edx,
                               renderer_u32 *out_edx)
{
    renderer_u32 target = hs_enter(slot, stack);
    renderer_u32 r_eax = 0, r_edx = 0;
    int completed = 0;
    __try {
        __asm {
            mov eax, stack
            push dword ptr [eax+28]
            push dword ptr [eax+24]
            push dword ptr [eax+20]
            push dword ptr [eax+16]
            push dword ptr [eax+12]
            push dword ptr [eax+8]
            push dword ptr [eax+4]
            push dword ptr [eax]
            mov ecx, in_ecx
            mov edx, in_edx
            call target
            add esp, 32
            mov r_eax, eax
            mov r_edx, edx
        }
        completed = 1;
    } __finally {
        if (!completed)
            hs_abandon(slot);
    }
    hs_leave(slot, stack, r_eax, r_edx);
    *out_edx = r_edx;
    return r_eax;
}

/* Generic cdecl relay. Entry: EAX = slot (set by the per-slot stub; EAX is
 * not an argument register for these cdecl callbacks), [esp] = return
 * address, [esp+4..] = caller's arguments. The original receives a copy of
 * the first HS_RELAY_STACK_WORDS stack words plus the caller's ECX/EDX, and
 * its EAX/EDX are returned unchanged. EBX/ESI/EDI/EBP are preserved; the
 * caller still cleans its own arguments. */
static __declspec(naked) void hs_relay(void)
{
    __asm {
        push ebp
        mov ebp, esp
        push 0
        push eax
        lea eax, [ebp-4]
        push eax
        push edx
        push ecx
        lea eax, [ebp+8]
        push eax
        push dword ptr [ebp-8]
        call hs_invoke
        add esp, 20
        mov edx, [ebp-4]
        mov esp, ebp
        pop ebp
        ret
    }
}

#define HS_STUB(n) \
    static __declspec(naked) void hs_stub_##n(void) { __asm mov eax, n __asm jmp hs_relay }
HS_STUB(81) HS_STUB(89) HS_STUB(101) HS_STUB(102) HS_STUB(106) HS_STUB(107)
HS_STUB(114) HS_STUB(117) HS_STUB(129) HS_STUB(130) HS_STUB(131) HS_STUB(143)
HS_STUB(148)

static renderer_u32 stub_for(renderer_u32 slot)
{
    switch (slot) {
    case 81: return (renderer_u32)(UINT_PTR)hs_stub_81;
    case 89: return (renderer_u32)(UINT_PTR)hs_stub_89;
    case 101: return (renderer_u32)(UINT_PTR)hs_stub_101;
    case 102: return (renderer_u32)(UINT_PTR)hs_stub_102;
    case 106: return (renderer_u32)(UINT_PTR)hs_stub_106;
    case 107: return (renderer_u32)(UINT_PTR)hs_stub_107;
    case 114: return (renderer_u32)(UINT_PTR)hs_stub_114;
    case 117: return (renderer_u32)(UINT_PTR)hs_stub_117;
    case 129: return (renderer_u32)(UINT_PTR)hs_stub_129;
    case 130: return (renderer_u32)(UINT_PTR)hs_stub_130;
    case 131: return (renderer_u32)(UINT_PTR)hs_stub_131;
    case 143: return (renderer_u32)(UINT_PTR)hs_stub_143;
    case 148: return (renderer_u32)(UINT_PTR)hs_stub_148;
    default: return 0;
    }
}

/* ---- public API -------------------------------------------------------- */

HS_API renderer_u32 HS_CALL hs_relayed_slots(const renderer_u32 **slots)
{
    if (slots) *slots = k_slots;
    return (renderer_u32)HS_SLOT_COUNT;
}

HS_API renderer_u32 HS_CALL hs_relay_address(renderer_u32 slot)
{
    return stub_for(slot);
}

HS_API void HS_CALL hs_get_stats(HsStats *stats)
{
    if (stats && stats->struct_size >= sizeof(HsStats)) {
        g_stats.struct_size = sizeof(HsStats);
        *stats = g_stats;
    }
}

HS_API HsInstallStatus HS_CALL hs_select_backend(const HsBackendV1 *backend,
                                                 renderer_u32 mode)
{
    if (backend && (backend->struct_size < sizeof(HsBackendV1) ||
                    backend->abi_version != HS_ABI_VERSION))
        return HS_ABI_MISMATCH;
    if (mode > HS_MODE_OBSERVE)
        return HS_INVALID_ARGUMENT;
    /* Switch only between frames; the relay itself never changes. */
    if (g_submit_depth != 0)
        return HS_INVALID_ARGUMENT;
    g_have_backend = 0;
    if (backend)
        g_backend = *backend;
    g_config.mode = mode;
    g_have_backend = backend != NULL;
    InterlockedExchange(&g_backend_disabled, 0);
    g_stats.backend_disabled = 0;
    return HS_INSTALLED;
}

HS_API HsInstallStatus HS_CALL hs_install(const renderer_u32 *source_words,
                                          renderer_u32 *host_pointer_cell,
                                          const HsConfig *config,
                                          const HsBackendV1 *backend)
{
    renderer_u32 values[HS_SLOT_COUNT];
    renderer_u32 i;
    LONG observed;

    if (!source_words || !host_pointer_cell || !config ||
        config->struct_size < sizeof(HsConfig) || config->mode > HS_MODE_OBSERVE ||
        ((UINT_PTR)host_pointer_cell & 3))
        return HS_INVALID_ARGUMENT;
    if (backend && (backend->struct_size < sizeof(HsBackendV1) ||
                    backend->abi_version != HS_ABI_VERSION))
        return HS_ABI_MISMATCH;
    if (InterlockedCompareExchange(&g_installed, 1, 0) != 0)
        return HS_ALREADY_INSTALLED;
    if (*(volatile renderer_u32 *)host_pointer_cell != (renderer_u32)(UINT_PTR)source_words) {
        g_installed = 0;
        return HS_SOURCE_POINTER_MISMATCH;
    }
    if (source_words[0] != ENGINE_TABLE_SIGNATURE) {
        g_installed = 0;
        return HS_BAD_SIGNATURE;
    }
    for (i = 0; i < HS_SLOT_COUNT; ++i) {
        if (source_words[k_slots[i]] == 0) {
            g_installed = 0;
            return HS_NULL_CALLBACK;
        }
        values[i] = stub_for(k_slots[i]);
    }
    memcpy(g_original, source_words, sizeof(g_original));
    if (engine_table_clone_selected(source_words, g_clone, k_slots, values,
                                    (renderer_u32)HS_SLOT_COUNT) != ENGINE_TABLE_CLONE_OK) {
        g_installed = 0;
        return HS_CLONE_REJECTED;
    }
    g_config = *config;
    if (backend) {
        g_backend = *backend;
        g_have_backend = 1;
    }
    /* Publish only a complete clone; refuse if the host changed the cell. */
    observed = InterlockedCompareExchange((volatile LONG *)host_pointer_cell,
                                          (LONG)(UINT_PTR)g_clone,
                                          (LONG)(UINT_PTR)source_words);
    if ((renderer_u32)observed != (renderer_u32)(UINT_PTR)source_words) {
        g_installed = 0;
        return HS_SOURCE_POINTER_MISMATCH;
    }
    return HS_INSTALLED;
}
