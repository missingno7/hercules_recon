#include <windows.h>
#include <string.h>

#include "presentation_probe.h"

#define PINNED_HOST_BASE 0x00400000UL
#define PINNED_HOST_SIZE 0x007ad000UL
#define PINNED_HOST_TIMESTAMP 0x33fafddaUL
#define PINNED_HOST_TABLE_RVA 0x000361b8UL
#define PINNED_HOST_POINTER_RVA 0x000365a0UL
#define PINNED_CHAIN_INIT 0x0040ac20UL
#define PINNED_SUBMIT 0x0040ac60UL
#define PINNED_IMPORT_NAME "ddraw.dll"
#define PINNED_EXPORT_NAME "DirectDrawCreate"
#define PRESENTATION_FORWARD_MAGIC 0x50465731UL

static renderer_u32 g_clone[ENGINE_TABLE_WORD_COUNT];
static renderer_u32 g_replace_slots[2] = {
    HERCULES_SLOT_CHAIN_INIT, HERCULES_SLOT_SUBMIT
};
static PresentationProbeV1 g_probe = {
    PRESENTATION_PROBE_MAGIC, PRESENTATION_STATE_UNCHECKED,
    0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL
};
static PresentationChainInitFn g_original_chain_init;
static PresentationSubmitFn g_original_submit;
static PresentationDirectDrawCreateFn g_real_directdraw_create;
static HMODULE g_real_ddraw_module;
static HMODULE g_probe_module;
static PresentationForwardInfoV1 g_forward_info = {
    PRESENTATION_FORWARD_MAGIC, PRESENTATION_FORWARD_UNRESOLVED,
    0UL, 0UL, 0UL, 0UL, 0UL, 0UL, { 0 }, { 0 }
};
#if defined(PRESENTATION_SYNTHETIC)
static char g_synthetic_forwarder[MAX_PATH];
#endif

HRESULT WINAPI DirectDrawCreate(void *guid, void **out_object, void *outer);

static void trace_event(renderer_u32 kind, renderer_u32 arg0,
                        renderer_u32 arg1, renderer_u32 arg2,
                        renderer_u32 target)
{
    LONG sequence = InterlockedIncrement((LONG *)&g_probe.trace_count);
    PresentationTraceEventV1 *event;

    if ((renderer_u32)sequence > PRESENTATION_TRACE_CAPACITY) {
        InterlockedIncrement((LONG *)&g_probe.trace_dropped);
        return;
    }
    event = &g_probe.trace[(renderer_u32)sequence - 1UL];
    event->sequence = (renderer_u32)sequence;
    event->kind = kind;
    event->arg0 = arg0;
    event->arg1 = arg1;
    event->arg2 = arg2;
    event->target = target;
}

static int PRESENTATION_CDECL chain_init_wrapper(void *first_slot,
                                                  renderer_u32 count)
{
    PresentationChainInitFn original = g_original_chain_init;
    int result;
    trace_event(PRESENTATION_EVENT_CHAIN_INIT, (renderer_u32)first_slot,
                count, 0UL, (renderer_u32)original);
    if (original == 0)
        return 0;
    result = original(first_slot, count);
    trace_event(PRESENTATION_EVENT_CHAIN_INIT_RETURN,
                (renderer_u32)first_slot, (renderer_u32)result,
                count, (renderer_u32)original);
    return result;
}

static void PRESENTATION_CDECL submit_wrapper(void *first_slot)
{
    PresentationSubmitFn original = g_original_submit;
    trace_event(PRESENTATION_EVENT_SUBMIT, (renderer_u32)first_slot,
                0UL, 0UL, (renderer_u32)original);
    if (original != 0) {
        original(first_slot);
        trace_event(PRESENTATION_EVENT_SUBMIT_RETURN,
                    (renderer_u32)first_slot, 0UL, 0UL,
                    (renderer_u32)original);
    }
}

static int make_clone(const renderer_u32 *source, renderer_u32 *output,
                      int publish, renderer_u32 *pointer_cell)
{
    renderer_u32 values[2];
    renderer_u32 status;

    if (source == 0 || output == 0)
        return ENGINE_TABLE_CLONE_INVALID_ARGUMENT;
    g_original_chain_init = (PresentationChainInitFn)source[
        HERCULES_SLOT_CHAIN_INIT];
    g_original_submit = (PresentationSubmitFn)source[HERCULES_SLOT_SUBMIT];
    values[0] = (renderer_u32)chain_init_wrapper;
    values[1] = (renderer_u32)submit_wrapper;
    status = (renderer_u32)engine_table_clone_selected(
        source, output, g_replace_slots, values, 2UL);
    if (status != ENGINE_TABLE_CLONE_OK)
        return (int)status;

    if (publish) {
        if (pointer_cell == 0)
            return ENGINE_TABLE_CLONE_INVALID_ARGUMENT;
        *pointer_cell = (renderer_u32)output;
        g_probe.source_pointer_after = *pointer_cell;
        g_probe.clone_pointer = (renderer_u32)output;
    }
    g_probe.original_chain_init = (renderer_u32)g_original_chain_init;
    g_probe.original_submit = (renderer_u32)g_original_submit;
    return ENGINE_TABLE_CLONE_OK;
}

static int pinned_host_layout(HMODULE image)
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;

    if ((renderer_u32)image != PINNED_HOST_BASE)
        return 0;
    dos = (IMAGE_DOS_HEADER *)image;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 ||
        dos->e_lfanew > 0x1000)
        return 0;
    nt = (IMAGE_NT_HEADERS *)((BYTE *)image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        nt->FileHeader.TimeDateStamp != PINNED_HOST_TIMESTAMP ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC ||
        nt->OptionalHeader.ImageBase != PINNED_HOST_BASE ||
        nt->OptionalHeader.SizeOfImage != PINNED_HOST_SIZE)
        return 0;
    return 1;
}

static int make_system_ddraw_path(char *path, DWORD capacity)
{
    UINT length;

#if defined(PRESENTATION_SYNTHETIC)
    length = (UINT)lstrlenA(g_synthetic_forwarder);
    if (length == 0 || length + 1U > capacity)
        return 0;
    lstrcpynA(path, g_synthetic_forwarder, (int)capacity);
    return 1;
#else
    length = GetSystemDirectoryA(path, capacity);
    if (length == 0 || length + 1U + sizeof("ddraw.dll") > capacity)
        return 0;
    if (path[length - 1U] != '\\')
        path[length++] = '\\';
    lstrcpyA(path + length, "ddraw.dll");
    return 1;
#endif
}

static PresentationDirectDrawCreateFn resolve_real_directdraw_create(void)
{
    char path[MAX_PATH];
    HMODULE module;
    FARPROC address;
    PVOID previous;
    DWORD length;

    if (g_real_directdraw_create != 0)
        return g_real_directdraw_create;
    if (!make_system_ddraw_path(path, sizeof(path))) {
        g_forward_info.state = PRESENTATION_FORWARD_PATH_FAILED;
        g_forward_info.win32_error = GetLastError();
        return 0;
    }
    lstrcpynA(g_forward_info.requested_path, path, MAX_PATH);
    module = LoadLibraryA(path);
    if (module == 0) {
        g_forward_info.state = PRESENTATION_FORWARD_LOAD_FAILED;
        g_forward_info.win32_error = GetLastError();
        return 0;
    }
    if (module == g_probe_module) {
        FreeLibrary(module);
        g_forward_info.state = PRESENTATION_FORWARD_SELF_MODULE;
        return 0;
    }
    address = GetProcAddress(module, PINNED_EXPORT_NAME);
    if (address == 0) {
        g_forward_info.state = PRESENTATION_FORWARD_EXPORT_FAILED;
        g_forward_info.win32_error = GetLastError();
        FreeLibrary(module);
        return 0;
    }
    if (address == (FARPROC)DirectDrawCreate) {
        FreeLibrary(module);
        g_forward_info.state = PRESENTATION_FORWARD_SELF_EXPORT;
        return 0;
    }
    length = GetModuleFileNameA(module, g_forward_info.module_path,
                                MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        g_forward_info.module_path[0] = '\0';
        g_forward_info.state = PRESENTATION_FORWARD_PATH_UNAVAILABLE;
        g_forward_info.win32_error = GetLastError();
        FreeLibrary(module);
        return 0;
    }
    g_forward_info.module_path[length] = '\0';
    {
        IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)module;
        IMAGE_NT_HEADERS *nt;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 ||
            dos->e_lfanew > 0x1000) {
            FreeLibrary(module);
            g_forward_info.state = PRESENTATION_FORWARD_EXPORT_FAILED;
            return 0;
        }
        nt = (IMAGE_NT_HEADERS *)((BYTE *)module + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE ||
            nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC) {
            FreeLibrary(module);
            g_forward_info.state = PRESENTATION_FORWARD_EXPORT_FAILED;
            return 0;
        }
        g_forward_info.machine = nt->FileHeader.Machine;
        g_forward_info.timestamp = nt->FileHeader.TimeDateStamp;
        g_forward_info.image_size = nt->OptionalHeader.SizeOfImage;
    }

    previous = InterlockedCompareExchange(
        (PVOID *)&g_real_directdraw_create, (PVOID)address, 0);
    if (previous != 0) {
        FreeLibrary(module);
        return (PresentationDirectDrawCreateFn)previous;
    }
    g_real_ddraw_module = module;
    g_probe.real_export = (renderer_u32)address;
    g_forward_info.module = (renderer_u32)module;
    g_forward_info.export_address = (renderer_u32)address;
    g_forward_info.state = PRESENTATION_FORWARD_READY;
    return (PresentationDirectDrawCreateFn)address;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    HMODULE image;
    renderer_u32 *source;
    renderer_u32 *pointer_cell;
    int status;

    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;

    g_probe_module = (HMODULE)instance;
    g_probe.process_attach_calls += 1UL;
    image = GetModuleHandleA(0);
    if (!pinned_host_layout(image)) {
        g_probe.state = PRESENTATION_STATE_OTHER_IMAGE;
        return TRUE;
    }
    g_probe.host_base = (renderer_u32)image;
    source = (renderer_u32 *)((BYTE *)image + PINNED_HOST_TABLE_RVA);
    pointer_cell = (renderer_u32 *)((BYTE *)image + PINNED_HOST_POINTER_RVA);
    g_probe.source_pointer_cell = (renderer_u32)pointer_cell;
    g_probe.source_pointer_before = *pointer_cell;
    if (*pointer_cell != (renderer_u32)source ||
        source[0] != ENGINE_TABLE_SIGNATURE ||
        source[HERCULES_SLOT_CHAIN_INIT] != PINNED_CHAIN_INIT ||
        source[HERCULES_SLOT_SUBMIT] != PINNED_SUBMIT) {
        g_probe.state = PRESENTATION_STATE_TABLE_MISMATCH;
        return TRUE;
    }
    status = make_clone(source, g_clone, 1, pointer_cell);
    if (status != ENGINE_TABLE_CLONE_OK) {
        g_probe.state = PRESENTATION_STATE_CLONE_FAILED;
        return TRUE;
    }
    g_probe.state = PRESENTATION_STATE_PUBLISHED;
    return TRUE;
}

HRESULT WINAPI DirectDrawCreate(void *guid, void **out_object, void *outer)
{
    PresentationDirectDrawCreateFn original;
    HRESULT result;

    g_probe.directdrawcreate_calls += 1UL;
    original = resolve_real_directdraw_create();
    trace_event(PRESENTATION_EVENT_DDRAW_ENTER, (renderer_u32)guid,
                (renderer_u32)out_object, (renderer_u32)outer,
                (renderer_u32)original);
    if (original == 0)
        return (HRESULT)0x80004005UL;
    result = original(guid, out_object, outer);
    trace_event(PRESENTATION_EVENT_DDRAW_RETURN, (renderer_u32)result,
                (renderer_u32)out_object, 0UL, (renderer_u32)original);
    return result;
}

__declspec(dllexport) PresentationProbeV1 *WINAPI ProbeGetState(void)
{
    return &g_probe;
}

__declspec(dllexport) PresentationForwardInfoV1 *WINAPI ProbeGetForwardInfo(void)
{
    return &g_forward_info;
}

__declspec(dllexport) BOOL WINAPI ProbeResolveForwarder(void)
{
    return resolve_real_directdraw_create() != 0 ? TRUE : FALSE;
}

__declspec(dllexport) renderer_u32 WINAPI ProbeBuildSyntheticClone(
    const renderer_u32 *source, renderer_u32 *output)
{
    InterlockedExchange((LONG *)&g_probe.trace_count, 0L);
    InterlockedExchange((LONG *)&g_probe.trace_dropped, 0L);
    memset(g_probe.trace, 0, sizeof(g_probe.trace));
    return (renderer_u32)make_clone(source, output, 0, 0);
}

__declspec(dllexport) BOOL WINAPI ProbeSetSyntheticForwarderA(const char *path)
{
#if defined(PRESENTATION_SYNTHETIC)
    if (path == 0 || path[0] == '\0' || lstrlenA(path) >= MAX_PATH)
        return FALSE;
    lstrcpynA(g_synthetic_forwarder, path, MAX_PATH);
    return TRUE;
#else
    (void)path;
    return FALSE;
#endif
}
