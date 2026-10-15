/* Synthetic host/engine harness for the experimental host shim.
 *
 * The "host" is a 250-word table whose relayed slots point at synthetic
 * cdecl services in this EXE; the "engine" copies the published table like
 * PC_DLLEngineMain's rep movsd and calls through the copy with an assembly
 * caller that checks ESP balance and callee-saved registers. Every relayed
 * call is compared with a direct call of the same service.
 *
 * Writes <out>/ot_image.bin and <out>/shim_frame.json for ac60_oracle.py,
 * which replays the same order-table image through the original AC60 bytes.
 * Usage: test_host_shim.exe <out-dir> */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <float.h>
#include <intrin.h>
#include <xmmintrin.h>
#include <stdio.h>
#include <string.h>

#include "hercules_shim.h"

#define REGION_BASE 0x20000000u
#define REGION_SIZE 0x10000u
#define OT_OFF 0x2000u
#define OT2_OFF 0x2800u
#define PKT_OFF 0x4000u
#define OT_SLOTS 16u

typedef unsigned int u32;
typedef unsigned char u8;

static int g_pass, g_fail;
#define CHECK(c, ...) do { if (c) g_pass++; else { g_fail++; \
    printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---- synthetic host services ------------------------------------------ */

typedef struct CallLog {
    u32 slot;
    u32 args[8];
    u32 n;
} CallLog;
static CallLog g_log[512];
static u32 g_log_n;
static u32 g_consumed[256], g_consumed_n;
static u32 g_a8f0[16][3], g_a8f0_n;
static int g_raise_143, g_raise_89, g_mutate_89;
static u32 g_in_ecx, g_in_edx;

static void log_call(u32 slot, u32 n, const u32 *args)
{
    CallLog *e = &g_log[g_log_n++ % 512];
    e->slot = slot;
    e->n = n;
    memset(e->args, 0, sizeof(e->args));
    memcpy(e->args, args, n * 4);
}

static int __cdecl fake_ac20(u32 *first, u32 count)
{
    u32 a[2], i;
    a[0] = (u32)first; a[1] = count;
    log_call(81, 2, a);
    /* Original AC20 shape: count-1 forward links, NULL in the last cell. */
    for (i = 1; (int)i < (int)count; ++i, ++first)
        *first = (u32)(first + 1);
    *first = 0;
    return 0x0AC20000 | (int)count;
}

static void __cdecl fake_a8f0(u32 rect, int x, int y)
{
    g_a8f0[g_a8f0_n][0] = rect;
    g_a8f0[g_a8f0_n][1] = (u32)x;
    g_a8f0[g_a8f0_n][2] = (u32)y;
    g_a8f0_n++;
}

/* Consumption modeled on the decompiled AC60 loop (cursor sentinel = first+1,
 * live link reload, NULL terminates). Command 01 calls A8F0 directly, as the
 * original does at 0xB1E7, bypassing the table. Returns EDX:EAX. */
static unsigned __int64 __cdecl fake_ac60(u32 *first)
{
    u32 *p = first, *sentinel = first + 1;
    u32 a[1];
    a[0] = (u32)first;
    log_call(89, 1, a);
    g_consumed_n = 0;
    if (g_raise_89)
        RaiseException(0xE0000089, 0, 0, NULL);
    for (;;) {
        p = (u32 *)*p;
        while (p == sentinel) {
            p = (u32 *)*p;
            sentinel++;
        }
        if (!p)
            break;
        g_consumed[g_consumed_n++] = (u32)p;
        if (((u8 *)p)[7] == 0x01)
            fake_a8f0((u32)(p + 4), (short)p[3], *(short *)((u8 *)p + 0x0e));
        if (g_mutate_89 && g_consumed_n == 2)
            *(u32 *)g_consumed[0] = 0; /* rewrite an already-consumed link */
    }
    SetLastError(0x89);
    return 0x1234567889898989ull;
}

static u32 __cdecl fake_3(u32 slot, u32 a, u32 b, u32 c)
{
    u32 v[3];
    v[0] = a; v[1] = b; v[2] = c;
    log_call(slot, 3, v);
    return slot * 0x01010101u ^ a ^ b ^ c;
}
static u32 __cdecl fake_101(u32 a, u32 b, u32 c) { return fake_3(101, a, b, c); }
static u32 __cdecl fake_102(u32 a, u32 b, u32 c, u32 d) { return fake_3(102, a, b, c) + d; }
static u32 __cdecl fake_106(void) { return fake_3(106, 0, 0, 0); }
static u32 __cdecl fake_107(u32 a, u32 b, u32 c, u32 d) { return fake_3(107, a, b, c) - d; }
static u32 __cdecl fake_114(u32 a, u32 b) { return fake_3(114, a, b, 0); }
static u32 __cdecl fake_117(u32 a, u32 b) { return fake_3(117, a, b, 0); }
static u32 __cdecl fake_129(u32 rect, u32 src)
{
    SetLastError(0x129);
    return fake_3(129, rect, src, 0);
}
static u32 __cdecl fake_130(u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7)
{
    u32 v[7];
    v[0] = a1; v[1] = a2; v[2] = a3; v[3] = a4; v[4] = a5; v[5] = a6; v[6] = a7;
    log_call(130, 7, v);
    return (a2 & 0xffff) | (a4 << 16) ^ a5 ^ a7;
}
static u32 __cdecl fake_131(u32 a, u32 b, u32 c) { return fake_3(131, a, b, c); }
static u32 __cdecl fake_143(u32 a)
{
    if (g_raise_143)
        RaiseException(0xE0001430, 0, 0, NULL);
    return fake_3(143, a, 0, 0);
}
/* Captures the caller's ECX/EDX: the relay must deliver them unchanged. */
static __declspec(naked) void fake_148(void)
{
    __asm {
        mov g_in_ecx, ecx
        mov g_in_edx, edx
        mov eax, ecx
        xor eax, edx
        mov edx, 0x14800148
        ret
    }
}

/* ---- checked cdecl caller ---------------------------------------------- */

static u32 g_cc_target, g_cc_esp_before, g_cc_esp_after, g_cc_ebp;
static u32 g_r_eax, g_r_edx, g_r_ebx, g_r_esi, g_r_edi, g_r_ebp;

/* Pushes nargs words, sets EBX/ESI/EDI/ECX/EDX sentinels, calls, records
 * registers and ESP, then performs the caller cleanup itself. */
static __declspec(naked) void __cdecl checked_call(u32 target, const u32 *args, u32 nargs)
{
    __asm {
        push ebp
        mov ebp, esp
        push ebx
        push esi
        push edi
        mov ecx, [ebp+16]
        mov esi, [ebp+12]
    copy_loop:
        test ecx, ecx
        jz copied
        push dword ptr [esi+ecx*4-4]
        dec ecx
        jmp copy_loop
    copied:
        mov eax, [ebp+8]
        mov g_cc_target, eax
        mov g_cc_esp_before, esp
        mov g_cc_ebp, ebp
        mov ebx, 0xB0B0B0B0
        mov esi, 0x51515151
        mov edi, 0xD1D1D1D1
        mov ecx, 0xC0C0C0C0
        mov edx, 0xD0D0D0D0
        call dword ptr [g_cc_target]
        mov g_cc_esp_after, esp
        mov g_r_eax, eax
        mov g_r_edx, edx
        mov g_r_ebx, ebx
        mov g_r_esi, esi
        mov g_r_edi, edi
        mov g_r_ebp, ebp
        lea esp, [ebp-12]
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}

typedef struct Outcome {
    u32 eax, edx, esp_ok, regs_ok, last_error, log_slot, log_args[8], log_n, in_ecx, in_edx;
} Outcome;

static Outcome call_checked(u32 target, const u32 *args, u32 nargs)
{
    Outcome o;
    memset(&o, 0, sizeof(o));
    SetLastError(0x5EED);
    g_in_ecx = g_in_edx = 0;
    {
        u32 before = g_log_n;
        checked_call(target, args, nargs);
        o.last_error = GetLastError();
        if (g_log_n != before) {
            CallLog *e = &g_log[(g_log_n - 1) % 512];
            o.log_slot = e->slot;
            o.log_n = e->n;
            memcpy(o.log_args, e->args, sizeof(o.log_args));
        }
    }
    o.eax = g_r_eax;
    o.edx = g_r_edx;
    o.esp_ok = g_cc_esp_after == g_cc_esp_before;
    o.regs_ok = g_r_ebx == 0xB0B0B0B0 && g_r_esi == 0x51515151 &&
                g_r_edi == 0xD1D1D1D1 && g_r_ebp == g_cc_ebp;
    o.in_ecx = g_in_ecx;
    o.in_edx = g_in_edx;
    return o;
}

/* ---- recording backend -------------------------------------------------- */

typedef struct Recorder {
    u32 kinds[1024];   /* 'S' service, 'B' begin, 'P' primitive, 'E' end */
    u32 n;
    HsServiceEvent services[256];
    u32 services_n;
    HsPrimitive prims[128];
    u32 prims_n;
    HsFrameBegin begin;
    HsFrameEnd end;
    int fault_at_primitive; /* -1 none */
    int raise_cpp_at_primitive;
    int change_fpu;
    int change_registers;
    u32 thread_mismatch;
} Recorder;
static Recorder g_rec;

static void push_kind(u32 k) { if (g_rec.n < 1024) g_rec.kinds[g_rec.n++] = k; }

static void __cdecl rec_service(void *ctx, const HsServiceEvent *e)
{
    (void)ctx;
    push_kind('S');
    if (e->thread_id != GetCurrentThreadId()) g_rec.thread_mismatch++;
    if (g_rec.services_n < 256) g_rec.services[g_rec.services_n++] = *e;
    if (g_rec.change_fpu) {
        unsigned int cw;
        _controlfp_s(&cw, _PC_24, _MCW_PC);
    }
    if (g_rec.change_registers) {
        __asm fninit
        __asm fld1
        _mm_setcsr(_mm_getcsr() ^ 0x6000);
        if (g_rec.change_registers == 2)
            RaiseException(0xe000f001, 0, 0, NULL);
    }
}
static void __cdecl rec_begin(void *ctx, const HsFrameBegin *f)
{
    (void)ctx;
    push_kind('B');
    g_rec.begin = *f;
}
static void __cdecl rec_primitive(void *ctx, const HsPrimitive *p)
{
    (void)ctx;
    push_kind('P');
    if ((int)g_rec.prims_n == g_rec.fault_at_primitive)
        *(volatile int *)0 = 1;
    if ((int)g_rec.prims_n == g_rec.raise_cpp_at_primitive)
        RaiseException(0xE06D7363, 0, 0, NULL);
    if (g_rec.prims_n < 128) g_rec.prims[g_rec.prims_n++] = *p;
}
static void __cdecl rec_end(void *ctx, const HsFrameEnd *f)
{
    (void)ctx;
    push_kind('E');
    g_rec.end = *f;
}

static HsBackendV1 recorder_backend(void)
{
    HsBackendV1 b;
    memset(&b, 0, sizeof(b));
    b.struct_size = sizeof(b);
    b.abi_version = HS_ABI_VERSION;
    b.context = &g_rec;
    b.service = rec_service;
    b.frame_begin = rec_begin;
    b.primitive = rec_primitive;
    b.frame_end = rec_end;
    return b;
}

static void reset_recorder(void)
{
    memset(&g_rec, 0, sizeof(g_rec));
    g_rec.fault_at_primitive = -1;
    g_rec.raise_cpp_at_primitive = -1;
}

/* ---- fixture ------------------------------------------------------------ */

static u8 *g_region;
static u32 g_source[ENGINE_TABLE_WORD_COUNT];
static u32 g_engine[ENGINE_TABLE_WORD_COUNT];
static u32 *g_pointer_cell;
static volatile u32 g_primary_cell;

static u32 put(u32 offset, const u8 *bytes, u32 size)
{
    memcpy(g_region + offset, bytes, size);
    return REGION_BASE + offset;
}

/* Prepend a packet to bucket b, as producers link newest-first. */
static void link_packet(u32 *ot, u32 bucket, u32 packet_address)
{
    *(u32 *)(UINT_PTR)packet_address = ot[bucket];
    ot[bucket] = packet_address;
}

typedef struct Fixture {
    const char *name;
    u32 bucket;
    u8 bytes[0x34];
    u32 size;
} Fixture;

static void set16(u8 *b, u32 off, int v) { b[off] = (u8)v; b[off + 1] = (u8)(v >> 8); }

static u32 build_frame(u32 *ot)
{
    static Fixture fx[10];
    u32 i, off = PKT_OFF, n = 0;
    memset(fx, 0, sizeof(fx));
    /* E1 page state: payload 0xE1000000 | alt bank 0x180 | mode 2 << 5 | page 5 */
    fx[n].name = "e1"; fx[n].bucket = 0; fx[n].size = 0x0c;
    fx[n].bytes[4] = (u8)(0x80 | (2 << 5) | 5); fx[n].bytes[5] = 0x01; fx[n].bytes[7] = 0xE1; n++;
    /* F3 clip: x=-4, y=250 (wraps), w=400, h=20 */
    fx[n].name = "f3"; fx[n].bucket = 1; fx[n].size = 0x0c; fx[n].bytes[7] = 0xF3;
    set16(fx[n].bytes, 4, -4); fx[n].bytes[6] = 250; set16(fx[n].bytes, 8, 400); set16(fx[n].bytes, 10, 20); n++;
    /* FT4 */
    fx[n].name = "ft4"; fx[n].bucket = 3; fx[n].size = 0x28; fx[n].bytes[7] = 0x2C;
    fx[n].bytes[4] = 0x80; fx[n].bytes[5] = 0x40; fx[n].bytes[6] = 0x20;
    set16(fx[n].bytes, 8, 10); set16(fx[n].bytes, 10, 20);
    set16(fx[n].bytes, 0x10, 60); set16(fx[n].bytes, 0x12, 20);
    set16(fx[n].bytes, 0x18, 10); set16(fx[n].bytes, 0x1a, 70);
    set16(fx[n].bytes, 0x20, 60); set16(fx[n].bytes, 0x22, 70);
    set16(fx[n].bytes, 0x0e, 0x7f00); set16(fx[n].bytes, 0x16, 0x0005); n++;
    /* Semi-transparent FT4 in the same bucket (newest first). */
    fx[n] = fx[n - 1]; fx[n].name = "ft4_semi"; fx[n].bytes[7] = 0x2E;
    set16(fx[n].bytes, 8, 100); n++;
    /* Variable sprite 0x64 */
    fx[n].name = "sprite64"; fx[n].bucket = 4; fx[n].size = 0x14; fx[n].bytes[7] = 0x64;
    set16(fx[n].bytes, 8, 30); set16(fx[n].bytes, 10, 40); fx[n].bytes[0x0c] = 8; fx[n].bytes[0x0d] = 16;
    set16(fx[n].bytes, 0x10, 24); set16(fx[n].bytes, 0x12, 12); n++;
    /* Fixed 8x8 and 16x16 sprites */
    fx[n].name = "sprite8"; fx[n].bucket = 5; fx[n].size = 0x10; fx[n].bytes[7] = 0x74;
    set16(fx[n].bytes, 8, 5); set16(fx[n].bytes, 10, 6); n++;
    fx[n].name = "sprite16"; fx[n].bucket = 5; fx[n].size = 0x10; fx[n].bytes[7] = 0x7C;
    set16(fx[n].bytes, 8, 50); set16(fx[n].bytes, 10, 60); n++;
    /* Command 01 VRAM move: dest (12,34), src rect at +0x10 */
    fx[n].name = "cmd01"; fx[n].bucket = 7; fx[n].size = 0x18; fx[n].bytes[7] = 0x01;
    set16(fx[n].bytes, 0x0c, 12); set16(fx[n].bytes, 0x0e, 34);
    set16(fx[n].bytes, 0x10, 64); set16(fx[n].bytes, 0x12, 256); set16(fx[n].bytes, 0x14, 32); set16(fx[n].bytes, 0x16, 16); n++;
    /* Known-undrawn 0x68 and a GT4 in the last bucket. */
    fx[n].name = "tag68"; fx[n].bucket = 9; fx[n].size = 0x14; fx[n].bytes[7] = 0x68; n++;
    fx[n].name = "gt4"; fx[n].bucket = OT_SLOTS - 2; fx[n].size = 0x34; fx[n].bytes[7] = 0x3C;
    fx[n].bytes[4] = 0x10; fx[n].bytes[0x10] = 0x20; fx[n].bytes[0x1c] = 0x30; fx[n].bytes[0x28] = 0x40;
    set16(fx[n].bytes, 8, 1); set16(fx[n].bytes, 10, 2); set16(fx[n].bytes, 0x14, 3); set16(fx[n].bytes, 0x16, 4);
    set16(fx[n].bytes, 0x20, 5); set16(fx[n].bytes, 0x22, 6); set16(fx[n].bytes, 0x2c, 7); set16(fx[n].bytes, 0x2e, 8);
    set16(fx[n].bytes, 0x1a, 0x0101); n++;
    for (i = 0; i < n; ++i) {
        u32 addr = put(off, fx[i].bytes, fx[i].size);
        link_packet(ot, fx[i].bucket, addr);
        off += (fx[i].size + 3) & ~3u;
    }
    return n;
}

static u32 relay_args[8] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444,
                             0x55555555, 0x66666666, 0x77777777, 0x88888888 };

static void check_abi_against_direct(void)
{
    const u32 *slots;
    u32 count = hs_relayed_slots(&slots), i;
    for (i = 0; i < count; ++i) {
        u32 slot = slots[i];
        Outcome direct, relayed;
        if (slot == HERCULES_SLOT_CHAIN_INIT || slot == HERCULES_SLOT_SUBMIT)
            continue; /* exercised with real order tables below */
        direct = call_checked(g_source[slot], relay_args, 7);
        relayed = call_checked(g_engine[slot], relay_args, 7);
        CHECK(relayed.esp_ok && relayed.regs_ok, "slot %u ESP/callee-saved", slot);
        /* EDX is compared only where the service defines it (148 here, 89
         * below); for 32-bit returns it is the callee's scratch residue. */
        CHECK(direct.eax == relayed.eax && (slot != 148 || direct.edx == relayed.edx),
              "slot %u EAX/EDX %08x:%08x vs %08x:%08x", slot, direct.edx, direct.eax, relayed.edx, relayed.eax);
        CHECK(direct.log_slot == relayed.log_slot && direct.log_n == relayed.log_n &&
              memcmp(direct.log_args, relayed.log_args, sizeof(direct.log_args)) == 0,
              "slot %u forwarded arguments differ", slot);
        CHECK(direct.last_error == relayed.last_error, "slot %u LastError %x vs %x",
              slot, direct.last_error, relayed.last_error);
        CHECK(direct.in_ecx == relayed.in_ecx && direct.in_edx == relayed.in_edx,
              "slot %u inbound ECX/EDX", slot);
    }
}

static void write_outputs(const char *dir, u32 ot_address, u32 nfix)
{
    char path[MAX_PATH];
    FILE *f;
    u32 i;
    sprintf(path, "%s\\ot_image.bin", dir);
    f = fopen(path, "wb");
    fwrite(g_region + OT_OFF, 1, 0x3000, f); /* OT .. packets, at REGION_BASE+OT_OFF */
    fclose(f);
    sprintf(path, "%s\\shim_frame.json", dir);
    f = fopen(path, "w");
    fprintf(f, "{\n \"image_base\": %u,\n \"ot_address\": %u,\n \"slot_count\": %u,\n \"fixtures\": %u,\n",
            REGION_BASE + OT_OFF, ot_address, OT_SLOTS, nfix);
    fprintf(f, " \"fake_consumption\": [");
    for (i = 0; i < g_consumed_n; ++i) fprintf(f, "%s%u", i ? "," : "", g_consumed[i]);
    fprintf(f, "],\n \"primitives\": [\n");
    for (i = 0; i < g_rec.prims_n; ++i) {
        const HsPrimitive *p = &g_rec.prims[i];
        fprintf(f, "  {\"node\": %u, \"command\": %u, \"packet_kind\": %d, \"decode_status\": %d,"
                   " \"apply_status\": %d, \"staged_kind\": %d, \"semitransparent\": %u,"
                   " \"deferred\": [%d,%d,%d,%d,%d,%d]}%s\n",
                p->node, p->decoded.command, (int)p->decoded.kind, (int)p->decode_status,
                (int)p->apply_status, (int)p->staged.kind, p->staged.semitransparent,
                p->staged.deferred_copy.destination_x, p->staged.deferred_copy.destination_y,
                p->staged.deferred_copy.source_x, p->staged.deferred_copy.source_y,
                p->staged.deferred_copy.width, p->staged.deferred_copy.height,
                i + 1 < g_rec.prims_n ? "," : "");
    }
    if (g_rec.prims_n) {
        const RendererAc60StateView *s = &g_rec.prims[g_rec.prims_n - 1].state_after;
        fprintf(f, " ],\n \"final_state\": {\"A8E6F4\": %u, \"A8E6F8\": %u, \"A8E6FC\": %u,"
                   " \"A909B0\": %u, \"A909B4\": %u, \"43A488\": %d, \"43A48C\": %d},\n",
                s->page.page_selector, s->page.page_mode, s->page.alternate_bank,
                s->workspace.clip_left, s->workspace.clip_top,
                s->clip_bounds.right, s->clip_bounds.bottom);
    } else {
        fprintf(f, " ],\n \"final_state\": null,\n");
    }
    fprintf(f, " \"fake_a8f0\": [");
    for (i = 0; i < g_a8f0_n; ++i)
        fprintf(f, "%s[%u,%d,%d]", i ? "," : "", g_a8f0[i][0], (int)g_a8f0[i][1], (int)g_a8f0[i][2]);
    fprintf(f, "]\n}\n");
    fclose(f);
}

static u32 call_log_signature(u32 from)
{
    u32 h = 2166136261u, i, j;
    for (i = from; i < g_log_n; ++i) {
        CallLog *e = &g_log[i % 512];
        h = (h ^ e->slot) * 16777619u;
        for (j = 0; j < e->n; ++j) h = (h ^ e->args[j]) * 16777619u;
    }
    return h;
}

int main(int argc, char **argv)
{
    const char *out = argc > 1 ? argv[1] : ".";
    HsBackendV1 backend;
    HsConfig config;
    HsStats stats;
    u32 i, ot, ot2, nfix, sig_observe, sig_forward, sig_fault, faults_before;
    Outcome r, r_forward;
    u32 slot_args[2];
    const u32 *slots;
    u32 nslots;

    g_region = (u8 *)VirtualAlloc((void *)(UINT_PTR)REGION_BASE, REGION_SIZE,
                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if ((u32)(UINT_PTR)g_region != REGION_BASE) {
        printf("FAIL could not map fixed region\n");
        return 2;
    }
    g_pointer_cell = (u32 *)(g_region + 0x1000);
    ot = REGION_BASE + OT_OFF;
    ot2 = REGION_BASE + OT2_OFF;

    /* Synthetic host table: signature, a context word, distinct filler, and
     * services in the relayed slots. */
    for (i = 0; i < ENGINE_TABLE_WORD_COUNT; ++i) g_source[i] = 0xC0DE0000u + i;
    g_source[0] = ENGINE_TABLE_SIGNATURE;
    g_source[1] = REGION_BASE + 0x1800;
    g_source[81] = (u32)fake_ac20;  g_source[89] = (u32)fake_ac60;
    g_source[101] = (u32)fake_101;  g_source[102] = (u32)fake_102;
    g_source[106] = (u32)fake_106;  g_source[107] = (u32)fake_107;
    g_source[114] = (u32)fake_114;  g_source[117] = (u32)fake_117;
    g_source[129] = (u32)fake_129;  g_source[130] = (u32)fake_130;
    g_source[131] = (u32)fake_131;  g_source[143] = (u32)fake_143;
    g_source[148] = (u32)fake_148;
    *g_pointer_cell = (u32)g_source;

    reset_recorder();
    backend = recorder_backend();
    memset(&config, 0, sizeof(config));
    config.struct_size = sizeof(config);
    config.mode = HS_MODE_OBSERVE;
    config.primary_callback_cell = &g_primary_cell;

    /* 1. Installation and publication. */
    config.mode = 2;
    CHECK(hs_install(g_source, g_pointer_cell, &config, &backend) == HS_INVALID_ARGUMENT &&
          *g_pointer_cell == (u32)g_source, "unsupported install mode leaves pointer unchanged");
    config.mode = HS_MODE_OBSERVE;
    CHECK(hs_install(g_source, g_pointer_cell, &config, &backend) == HS_INSTALLED, "install");
    CHECK(hs_install(g_source, g_pointer_cell, &config, &backend) == HS_ALREADY_INSTALLED, "reinstall");
    CHECK(*g_pointer_cell != (u32)g_source, "pointer cell not republished");
    memcpy(g_engine, (void *)(UINT_PTR)*g_pointer_cell, sizeof(g_engine)); /* engine cold copy */
    nslots = hs_relayed_slots(&slots);
    for (i = 0; i < ENGINE_TABLE_WORD_COUNT; ++i) {
        u32 k, relayed = 0;
        for (k = 0; k < nslots; ++k) if (slots[k] == i) relayed = 1;
        if (relayed)
            CHECK(g_engine[i] == hs_relay_address(i) && g_engine[i] != g_source[i], "slot %u not relayed", i);
        else
            CHECK(g_engine[i] == g_source[i], "word %u changed", i);
    }

    /* 2. ABI: every non-frame relayed slot against a direct call. */
    check_abi_against_direct();
    CHECK(g_in_ecx == 0xC0C0C0C0 && g_in_edx == 0xD0D0D0D0, "slot 148 inbound ECX/EDX %08x %08x", g_in_ecx, g_in_edx);

    /* 3. Chain init through the relay records the order-table extent. */
    slot_args[0] = ot; slot_args[1] = OT_SLOTS;
    r = call_checked(g_engine[81], slot_args, 2);
    CHECK(r.esp_ok && r.regs_ok && r.eax == (0x0AC20000u | OT_SLOTS), "slot 81 relay %08x", r.eax);
    nfix = build_frame((u32 *)(UINT_PTR)ot);

    /* 4. Observed frame. */
    reset_recorder();
    g_a8f0_n = 0;
    {
        u32 before = g_log_n;
        slot_args[0] = ot;
        r = call_checked(g_engine[89], slot_args, 1);
        sig_observe = call_log_signature(before);
    }
    CHECK(r.esp_ok && r.regs_ok, "slot 89 ESP/callee-saved");
    CHECK(r.eax == 0x89898989u && r.edx == 0x12345678u, "slot 89 EDX:EAX %08x:%08x", r.edx, r.eax);
    CHECK(r.last_error == 0x89, "slot 89 LastError %x", r.last_error);
    CHECK(g_consumed_n == nfix, "fake consumed %u of %u", g_consumed_n, nfix);
    CHECK(g_rec.prims_n == g_consumed_n, "pre-walk %u vs consumption %u", g_rec.prims_n, g_consumed_n);
    for (i = 0; i < g_rec.prims_n && i < g_consumed_n; ++i)
        CHECK(g_rec.prims[i].node == g_consumed[i], "order %u: %08x vs %08x", i, g_rec.prims[i].node, g_consumed[i]);
    CHECK(g_rec.n > 1 && g_rec.kinds[g_rec.n - 2] == 'E' &&
          g_rec.end.prewalk == HS_PREWALK_DONE && g_rec.end.consumption_consistent == 1, "frame end %u/%u",
          g_rec.end.prewalk, g_rec.end.consumption_consistent);
    CHECK(g_rec.begin.slot_count == OT_SLOTS && g_rec.begin.first_slot == ot, "frame begin extent");
    CHECK(g_rec.n >= 4 && g_rec.kinds[0] == 'S' && g_rec.kinds[1] == 'B' &&
          g_rec.kinds[g_rec.n - 2] == 'E' && g_rec.kinds[g_rec.n - 1] == 'S', "event bracket order");
    CHECK(g_rec.services_n == 2 && g_rec.services[0].phase == 0 && g_rec.services[1].phase == 1 &&
          g_rec.services[1].raw_eax == 0x89898989u && g_rec.services[1].raw_edx == 0x12345678u,
          "slot 89 service enter/return");
    CHECK(g_a8f0_n == 1, "direct A8F0 edge executed once (%u)", g_a8f0_n);
    CHECK(g_rec.thread_mismatch == 0, "events on the calling thread");
    write_outputs(out, ot, nfix);

    /* 5. Forward-only: same gameplay-visible calls, no backend events. */
    reset_recorder();
    CHECK(hs_select_backend(&backend, HS_MODE_FORWARD_ONLY) == HS_INSTALLED, "select forward-only");
    {
        u32 before = g_log_n;
        r_forward = call_checked(g_engine[89], slot_args, 1);
        sig_forward = call_log_signature(before);
    }
    CHECK(g_rec.n == 0, "forward-only produced %u events", g_rec.n);
    CHECK(sig_forward == sig_observe && r_forward.eax == r.eax && r_forward.edx == r.edx,
          "forward-only call log/result differs");

    /* 6. Faulting backend: access violation in the 3rd primitive. */
    reset_recorder();
    g_rec.fault_at_primitive = 2;
    hs_select_backend(&backend, HS_MODE_OBSERVE);
    stats.struct_size = sizeof(stats);
    hs_get_stats(&stats);
    faults_before = stats.backend_faults;
    {
        u32 before = g_log_n;
        r = call_checked(g_engine[89], slot_args, 1);
        sig_fault = call_log_signature(before);
    }
    hs_get_stats(&stats);
    CHECK(stats.backend_faults == faults_before + 1 && stats.last_fault_code == EXCEPTION_ACCESS_VIOLATION &&
          stats.backend_disabled, "access violation contained");
    CHECK(sig_fault == sig_observe && r.eax == 0x89898989u && r.esp_ok && r.regs_ok, "host unaffected by backend fault");
    reset_recorder();
    call_checked(g_engine[89], slot_args, 1);
    CHECK(g_rec.n == 0, "disabled backend still called");

    /* 7. C++ exception code and FPU control word changes from the backend. */
    reset_recorder();
    g_rec.raise_cpp_at_primitive = 0;
    hs_select_backend(&backend, HS_MODE_OBSERVE);
    call_checked(g_engine[89], slot_args, 1);
    hs_get_stats(&stats);
    CHECK(stats.last_fault_code == 0xE06D7363u && stats.backend_disabled, "C++ throw contained");
    reset_recorder();
    g_rec.change_fpu = 1;
    hs_select_backend(&backend, HS_MODE_OBSERVE);
    {
        unsigned int before_cw, after_cw, restores_before;
        hs_get_stats(&stats);
        restores_before = stats.fpu_restores;
        _controlfp_s(&before_cw, 0, 0);
        call_checked(g_engine[143], relay_args, 1);
        _controlfp_s(&after_cw, 0, 0);
        hs_get_stats(&stats);
        CHECK(before_cw == after_cw && stats.fpu_restores > restores_before, "x87 control word restored");
    }
    g_rec.change_fpu = 0;

    /* A backend may destroy the x87 stack/status before returning or faulting.
     * Control-word restoration alone cannot preserve an original FP result. */
    {
        __declspec(align(16)) unsigned char before[512], after[512];
        int variant;
        for (variant = 1; variant <= 2; ++variant) {
            reset_recorder();
            g_rec.change_registers = variant;
            hs_select_backend(&backend, HS_MODE_OBSERVE);
            __asm fldpi
            _fxsave(before);
            call_checked(g_engine[143], relay_args, 1);
            _fxsave(after);
            _fxrstor(before);
            __asm fstp st(0)
            CHECK(memcmp(before, after, 6) == 0 &&
                  memcmp(before + 32, after + 32, 10) == 0 &&
                  memcmp(before + 24, after + 24, 4) == 0,
                  "x87 stack/status and MXCSR restored (fault=%d)", variant == 2);
        }
        reset_recorder();
        hs_select_backend(&backend, HS_MODE_OBSERVE);
    }

    /* 8. Exceptions raised by the original propagate unchanged. */
    {
        DWORD code = 0;
        g_raise_143 = 1;
        __try { call_checked(g_engine[143], relay_args, 1); }
        __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {}
        g_raise_143 = 0;
        CHECK(code == 0xE0001430u, "original exception not propagated (%08lx)", code);
        code = 0;
        g_raise_89 = 1;
        __try { call_checked(g_engine[89], slot_args, 1); }
        __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {}
        g_raise_89 = 0;
        CHECK(code == 0xE0000089u, "slot 89 exception not propagated");
        reset_recorder();
        call_checked(g_engine[89], slot_args, 1);
        hs_get_stats(&stats);
        CHECK(stats.abandoned_brackets == 1 && g_rec.n > 0 && g_rec.kinds[g_rec.n - 2] == 'E' &&
              g_rec.end.prewalk == HS_PREWALK_DONE && g_rec.end.consumption_consistent,
              "abandoned bracket recovery (%u, %u)", stats.abandoned_brackets, g_rec.end.prewalk);
    }

    /* 9. Preconditions: non-null primary callback, unknown extent. */
    reset_recorder();
    g_primary_cell = 0x401234;
    call_checked(g_engine[89], slot_args, 1);
    CHECK(g_rec.prims_n == 0 && g_consumed_n == nfix, "primary-callback guard");
    g_primary_cell = 0;
    hs_get_stats(&stats);
    CHECK(stats.prewalk_skips[HS_PREWALK_SKIP_PRIMARY_CALLBACK] == 1, "primary skip counted");
    ((u32 *)(UINT_PTR)ot2)[0] = 0; /* an order table never linked by slot 81 */
    slot_args[0] = ot2;
    call_checked(g_engine[89], slot_args, 1);
    hs_get_stats(&stats);
    CHECK(stats.prewalk_skips[HS_PREWALK_SKIP_UNKNOWN_EXTENT] == 1, "unknown-extent skip");
    slot_args[0] = ot;

    /* 10. Falsifier: the original rewriting a consumed link is detected. */
    reset_recorder();
    {
        u32 saved = *(u32 *)(UINT_PTR)g_consumed[0];
        g_mutate_89 = 1;
        call_checked(g_engine[89], slot_args, 1);
        g_mutate_89 = 0;
        CHECK(g_rec.end.consumption_consistent == 0, "mutation not detected");
        *(u32 *)(UINT_PTR)g_consumed[0] = saved;
    }
    hs_get_stats(&stats);
    printf("stats: calls=%u frames=%u prims=%u faults=%u fpu_restores=%u mismatches=%u abandoned=%u\n",
           stats.service_calls, stats.frames, stats.primitives, stats.backend_faults,
           stats.fpu_restores, stats.consumption_mismatches, stats.abandoned_brackets);
    printf("%s: %d passed, %d failed\n", g_fail ? "FAIL" : "PASS", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
