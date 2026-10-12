/* Bounded native/source fixture for the private ENG1 0x1e50 candidate.
 * Native calls use only the original loaded code; all descriptor/action tables
 * in this fixture are controlled test data, not reconstructed production data.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../calibration/engine_interface.h"
#include "../calibration/frame_configuration_row.h"
#include "../src/eng1/behavioral/data_layouts.h"
#include "../src/eng1/behavioral/actor_pipeline_storage.h"

typedef unsigned char fx_u8;
typedef unsigned short fx_u16;
typedef unsigned long fx_u32;

typedef void (__cdecl *FxCallback)(void *);
typedef struct FxCallbackRow12 {
    FxCallback slot0;
    FxCallback slot4;
    FxCallback slot8;
} FxCallbackRow12;

typedef void (__cdecl *NativeEntry)(void);

extern void __cdecl eng1_call_01e50(void);
extern void *g_normal_actor_pool;
extern void *g_pipeline_actor_7200c;
extern long g_camera_x_70ca0, g_camera_y_70ca4;

/* Link-only cells referenced by unused allocator functions in actor_pools.c.
 * The measured initializer path never reads or writes them. */
unsigned short g_normal_cursor_primary, g_normal_limit_primary;
unsigned short g_normal_limit_wrap, g_normal_cursor_alternate;
unsigned short g_normal_limit_wrap_start;
unsigned short g_alternate_cursor_primary, g_alternate_limit_primary;
unsigned short g_alternate_limit_wrap, g_alternate_cursor_alternate;
unsigned short g_alternate_limit_wrap_start;

/* These six arrays are the finite test registrations used by the existing
 * worklist guard when compiled with ENG1_WORKLIST_GUARD_FIXTURE. */
fx_u8 eng1_fixture_normal_assets[128];
fx_u8 eng1_fixture_middle_assets[64];
fx_u8 eng1_fixture_alternate_assets[64];
fx_u8 eng1_fixture_normal_actions[96];
fx_u8 eng1_fixture_middle_actions[64];
fx_u8 eng1_fixture_alternate_actions[64];

/* Controlled source-side views of the existing production data owners. */
ConfigurationRow72 g_configuration_rows_68af0[9];
Eng1FrameDescriptorRow24 eng1_descriptor_rows_5a690[12];
fx_u32 g_configuration_dword_71370;

static HMODULE g_native_module;
static int g_native_call;
static int g_callback_count;
static void *g_callback_actor[2];
static fx_u8 g_context[8];
static fx_u8 g_source_descriptors[2][24];
static fx_u8 g_native_descriptors[2][24];
static union { unsigned long align; fx_u8 bytes[0x268]; } g_source_pool;
static union { unsigned long align; fx_u8 bytes[0x268]; } g_native_pool;
static union { unsigned long align; fx_u8 bytes[0x134]; } g_source_redirect;
static union { unsigned long align; fx_u8 bytes[0x134]; } g_native_redirect;

static fx_u8 *native_rva(fx_u32 rva)
{
    return (fx_u8 *)(void *)g_native_module + rva;
}

static fx_u32 native_read32(fx_u32 rva)
{
    return *(fx_u32 *)(void *)native_rva(rva);
}

static void native_write32(fx_u32 rva, fx_u32 value)
{
    *(fx_u32 *)(void *)native_rva(rva) = value;
}

static void source_write32(void *base, fx_u32 offset, fx_u32 value)
{
    *(fx_u32 *)(void *)((fx_u8 *)base + offset) = value;
}

static void seed_actor(fx_u8 *actor, fx_u8 seed)
{
    unsigned int i;
    for (i = 0; i < 0x134U; ++i)
        actor[i] = (fx_u8)(seed + i * 17U + (i >> 3));
}

static void record_callback(void *actor)
{
    if (g_callback_count < 2)
        g_callback_actor[g_callback_count] = actor;
    ++g_callback_count;
}

static void __cdecl callback_first(void *actor)
{
    record_callback(actor);
    if (g_native_call) {
        native_write32(0x7200cUL, (fx_u32)(void *)g_native_redirect.bytes);
        native_write32(0x71370UL,
                       (fx_u32)(void *)eng1_fixture_middle_actions);
    } else {
        g_pipeline_actor_7200c = (void *)g_source_redirect.bytes;
        g_configuration_dword_71370 =
            (fx_u32)(void *)eng1_fixture_middle_actions;
    }
}

static void __cdecl callback_second(void *actor)
{
    record_callback(actor);
}

static void set_callback(void *table, unsigned int index,
                         unsigned int slot, FxCallback callback)
{
    FxCallbackRow12 *row = (FxCallbackRow12 *)table + index;
    if (slot == 0U) row->slot0 = callback;
    else row->slot8 = callback;
}

static void prepare_descriptors(int callback_case)
{
    fx_u16 tag = callback_case ? 0x4007U : 0U;
    memset(g_source_descriptors, 0, sizeof(g_source_descriptors));
    memset(g_native_descriptors, 0, sizeof(g_native_descriptors));
    *(fx_u16 *)(void *)(g_source_descriptors[0] + 0x10) = 0x1234U;
    *(fx_u16 *)(void *)(g_source_descriptors[0] + 0x12) = 0xfedcU;
    *(fx_u16 *)(void *)(g_source_descriptors[0] + 0x14) = 0xabcdU;
    *(fx_u16 *)(void *)(g_source_descriptors[0] + 0x16) = tag;
    memcpy(g_native_descriptors, g_source_descriptors,
           sizeof(g_source_descriptors));
    memset(eng1_descriptor_rows_5a690, 0,
           sizeof(eng1_descriptor_rows_5a690));
    eng1_descriptor_rows_5a690[0].pointer_00c =
        (void *)g_source_descriptors;
}

static void prepare_source(int callback_case)
{
    memset(&g_source_pool, 0xa5, sizeof(g_source_pool));
    memset(&g_source_redirect, 0x5a, sizeof(g_source_redirect));
    seed_actor(g_source_pool.bytes + 0x134, 0x31U);
    seed_actor(g_source_redirect.bytes, 0x73U);
    *(fx_u16 *)(void *)(g_source_redirect.bytes + 0x2e) = 3U;
    g_normal_actor_pool = g_source_pool.bytes;
    g_pipeline_actor_7200c = 0;
    g_camera_x_70ca0 = 0x11223344L;
    g_camera_y_70ca4 = 0x55667788L;
    memset(g_context, 0, sizeof(g_context));
    g_context[0] = 0;
    g_context[1] = 0;
    g_engine_interface.context_004 =
        (ResourceCallbackContext *)(void *)g_context;
    memset(g_configuration_rows_68af0, 0,
           sizeof(g_configuration_rows_68af0));
    g_configuration_rows_68af0[0].dword_01c =
        (fx_u32)(void *)eng1_fixture_normal_actions;
    g_configuration_rows_68af0[1].dword_01c =
        (fx_u32)(void *)eng1_fixture_middle_actions;
    memset(eng1_fixture_normal_actions, 0,
           sizeof(eng1_fixture_normal_actions));
    memset(eng1_fixture_middle_actions, 0,
           sizeof(eng1_fixture_middle_actions));
    set_callback(eng1_fixture_normal_actions, 7U, 0U, callback_first);
    set_callback(eng1_fixture_middle_actions, 3U, 8U, callback_second);
    g_configuration_dword_71370 =
        (fx_u32)(void *)eng1_fixture_normal_actions;
    g_callback_count = 0;
    g_callback_actor[0] = g_callback_actor[1] = 0;
    prepare_descriptors(callback_case);
}

static void prepare_native(int callback_case)
{
    memset(&g_native_pool, 0xa5, sizeof(g_native_pool));
    memset(&g_native_redirect, 0x5a, sizeof(g_native_redirect));
    seed_actor(g_native_pool.bytes + 0x134, 0x31U);
    seed_actor(g_native_redirect.bytes, 0x73U);
    *(fx_u16 *)(void *)(g_native_redirect.bytes + 0x2e) = 3U;
    memset(g_context, 0, sizeof(g_context));
    g_context[0] = 0;
    g_context[1] = 0;
    native_write32(0x72860UL, (fx_u32)(void *)g_native_pool.bytes);
    native_write32(0x7200cUL, 0UL);
    native_write32(0x70f24UL, (fx_u32)(void *)g_context);
    native_write32(0x5a69cUL, (fx_u32)(void *)g_native_descriptors);
    native_write32(0x70ca0UL, 0x11223344UL);
    native_write32(0x70ca4UL, 0x55667788UL);
    memset(eng1_fixture_normal_actions, 0,
           sizeof(eng1_fixture_normal_actions));
    memset(eng1_fixture_middle_actions, 0,
           sizeof(eng1_fixture_middle_actions));
    set_callback(eng1_fixture_normal_actions, 7U, 0U, callback_first);
    set_callback(eng1_fixture_middle_actions, 3U, 8U, callback_second);
    native_write32(0x71370UL,
                   (fx_u32)(void *)eng1_fixture_normal_actions);
    g_callback_count = 0;
    g_callback_actor[0] = g_callback_actor[1] = 0;
    prepare_descriptors(callback_case);
    /* The source view is not consulted by the native entry; reapply the native
     * pointer after prepare_descriptors has reset only source-side owners. */
    native_write32(0x5a69cUL, (fx_u32)(void *)g_native_descriptors);
}

static void print_hex(const fx_u8 *bytes, unsigned int count)
{
    unsigned int i;
    for (i = 0; i < count; ++i) printf("%02x", (unsigned int)bytes[i]);
}

/* Explicit fixture-only incomplete endpoint: unsupported candidate paths
 * cannot silently succeed. No invalid path is expected in this matrix. */
void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    fprintf(stderr, "fixture caught unsupported RVA 0x%lx: %s\n", rva,
            reason ? reason : "(null)");
    fflush(stderr);
    ExitProcess(0xe1U);
}

static unsigned int g_checks, g_failures;
static void check(int ok, const char *name) {
    ++g_checks;
    if (!ok) { ++g_failures; fprintf(stderr, "FAIL %s\n", name); }
}
int main(int argc, char **argv)
{
    static const fx_u32 cells[7] = {0x72860UL,0x7200cUL,0x70f24UL,
        0x5a69cUL,0x71370UL,0x70ca0UL,0x70ca4UL};
    fx_u32 old[7], camera_x, camera_y;
    fx_u8 primary[0x134], redirect[0x134];
    int expected_callbacks, first_primary, second_redirect, active_redirect;
    unsigned int i, scenario;
    NativeEntry native;
    if (argc == 3) {
        prepare_source(1);
        if (!strcmp(argv[2],"pool")) g_normal_actor_pool=0;
        else if (!strcmp(argv[2],"level")) g_context[0]=0xffU;
        else if (!strcmp(argv[2],"subrow")) g_context[1]=2;
        else if (!strcmp(argv[2],"base")) g_configuration_dword_71370=1;
        else if (!strcmp(argv[2],"callback"))
            set_callback(eng1_fixture_normal_actions,7U,0U,0);
        else return 2;
        eng1_call_01e50();
        return 4;
    }
    if (argc != 2) return 2;
    g_native_module = LoadLibraryA(argv[1]);
    if (!g_native_module) return 3;
    for(i=0;i<7U;++i) old[i]=native_read32(cells[i]);
    native=(NativeEntry)(void *)native_rva(0x1e50UL);
    for(scenario=0;scenario<3U;++scenario) {
        g_native_call=0;
        prepare_source(scenario != 0);
        if(scenario==2U) {
            /* Bits10/11 distinguish the native 03ff mask from 0fff. */
            *(fx_u16 *)(void *)(g_source_descriptors[1]+0x16)=0x4c07U;
            *(fx_u16 *)(void *)(g_source_descriptors[1]+0x10)=0xffffU;
            *(fx_u16 *)(void *)(g_source_descriptors[1]+0x12)=0x8000U;
            *(fx_u16 *)(void *)(g_source_descriptors[1]+0x14)=1U;
            memcpy(g_native_descriptors,g_source_descriptors,sizeof(g_source_descriptors));
            g_context[1]=1;
        }
        eng1_call_01e50();
        memcpy(primary,g_source_pool.bytes+0x134,sizeof(primary));
        memcpy(redirect,g_source_redirect.bytes,sizeof(redirect));
        camera_x=(fx_u32)g_camera_x_70ca0;
        camera_y=(fx_u32)g_camera_y_70ca4;
        expected_callbacks=g_callback_count;
        first_primary=g_callback_actor[0]==g_source_pool.bytes+0x134;
        second_redirect=g_callback_actor[1]==g_source_redirect.bytes;
        active_redirect=g_pipeline_actor_7200c==g_source_redirect.bytes;
        g_native_call=1;
        prepare_native(scenario != 0);
        if(scenario==2U) {
            *(fx_u16 *)(void *)(g_native_descriptors[1]+0x16)=0x4c07U;
            *(fx_u16 *)(void *)(g_native_descriptors[1]+0x10)=0xffffU;
            *(fx_u16 *)(void *)(g_native_descriptors[1]+0x12)=0x8000U;
            *(fx_u16 *)(void *)(g_native_descriptors[1]+0x14)=1U;
            g_context[1]=1;
        }
        native();
        check(!memcmp(primary,g_native_pool.bytes+0x134,sizeof(primary)),"primary actor");
        check(!memcmp(redirect,g_native_redirect.bytes,sizeof(redirect)),"redirect actor");
        check(camera_x==native_read32(0x70ca0UL),"camera x");
        check(camera_y==native_read32(0x70ca4UL),"camera y");
        check(expected_callbacks==g_callback_count && expected_callbacks==(scenario ? 2 : 0),"callback count");
        check(first_primary==(g_callback_actor[0]==g_native_pool.bytes+0x134),"first callback actor");
        check(second_redirect==(g_callback_actor[1]==g_native_redirect.bytes),"second callback actor");
        check(active_redirect==(native_read32(0x7200cUL)==(fx_u32)(void *)g_native_redirect.bytes),"active actor reload");
        printf("PASS bootstrap scenario=%u\n",scenario);
    }
    for(i=0;i<7U;++i) native_write32(cells[i],old[i]);
    FreeLibrary(g_native_module);
    printf("actor_bootstrap cases=3 checks=%u failures=%u\n",g_checks,g_failures);
    return g_failures ? 1 : 0;
}
