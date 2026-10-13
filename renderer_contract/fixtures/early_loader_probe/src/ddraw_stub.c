#include <windows.h>

#include "hercules_host_abi.h"

typedef renderer_u32 **(WINAPI *FixtureSourceAccessor)(void);

static renderer_u32 g_private_clone[ENGINE_TABLE_WORD_COUNT];
static renderer_u32 g_selected_slot = HERCULES_SLOT_SUBMIT;
static renderer_u32 g_selected_value = 0xf17e0089u;
static renderer_u32 **g_source_pointer_cell = 0;
static int g_mock_state = 0;

static int resolve_fixture_source(void)
{
    HMODULE fixture_exe;
    FARPROC exported_accessor;
    FixtureSourceAccessor accessor;

    fixture_exe = GetModuleHandleA(0);
    if (fixture_exe == 0)
        return 0;
    exported_accessor = GetProcAddress(
        fixture_exe, "FixtureGetEngineTablePointerSlotV7");
    if (exported_accessor == 0)
        return 0;
    accessor = (FixtureSourceAccessor)exported_accessor;
    g_source_pointer_cell = accessor();
    if (g_source_pointer_cell == 0 || *g_source_pointer_cell == 0)
        return 0;
    if ((*g_source_pointer_cell)[0] != ENGINE_TABLE_SIGNATURE)
        return 0;
    return 1;
}

static int publish_clone(void)
{
    renderer_u32 status;
    if (g_source_pointer_cell == 0 || *g_source_pointer_cell == 0)
        return 0;
    status = (renderer_u32)engine_table_clone_selected(
        *g_source_pointer_cell,
        g_private_clone,
        &g_selected_slot,
        &g_selected_value,
        1u);
    if (status != ENGINE_TABLE_CLONE_OK)
        return 0;
    *g_source_pointer_cell = g_private_clone;
    g_mock_state = 2;
    return 1;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)instance;
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        g_mock_state = 0;
        if (!resolve_fixture_source()) {
            g_mock_state = -1;
            return TRUE;
        }
        g_mock_state = 1;
#if defined(EARLY_ATTACH_PUBLISH)
        if (!publish_clone()) {
            g_mock_state = -2;
            return TRUE;
        }
#endif
    }
    return TRUE;
}

int WINAPI FixtureMockIdentityV7(void)
{
    return 0x00dd7a51;
}

int WINAPI FixtureGetMockStateV7(void)
{
    return g_mock_state;
}

HRESULT WINAPI DirectDrawCreate(void *guid, void **out, void *outer)
{
    (void)guid;
    (void)outer;
    if (out != 0)
        *out = 0;
#if !defined(EARLY_ATTACH_PUBLISH)
    if (g_mock_state == 1 && !publish_clone())
        g_mock_state = -3;
#endif
    if (g_mock_state < 1)
        OutputDebugStringA(
            "Fixture ddraw.dll diagnostic: no valid V7 probe accessor; "
            "mock DirectDrawCreate refused the host.");
    else if (g_mock_state != 2)
        OutputDebugStringA(
            "Fixture ddraw.dll diagnostic: source clone was not published; "
            "mock DirectDrawCreate returns E_NOTIMPL.");
    /* Fixture API only: never loads or calls the genuine DirectDraw DLL. */
    return (HRESULT)0x80004001UL; /* E_NOTIMPL */
}
