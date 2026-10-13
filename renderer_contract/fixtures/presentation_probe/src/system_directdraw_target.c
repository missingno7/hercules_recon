#include <windows.h>
#include <stdio.h>

#include "presentation_probe.h"

typedef HRESULT (WINAPI *DirectDrawCreateFn)(void *guid, void **out_object,
                                              void *outer);
typedef PresentationForwardInfoV1 *(WINAPI *GetForwardInfoFn)(void);
typedef DWORD (WINAPI *GetFinalPathNameByHandleAFn)(HANDLE file,
                                                     LPSTR path,
                                                     DWORD capacity,
                                                     DWORD flags);

typedef struct UnknownVtablePrefix {
    void *query_interface;
    void *add_ref;
    ULONG (WINAPI *release)(void *self);
} UnknownVtablePrefix;

static int fail(const char *message)
{
    printf("INFRASTRUCTURE_ERROR=%s\n", message);
    return 2;
}

int main(int argc, char **argv)
{
    HMODULE candidate;
    FARPROC export_address;
    DirectDrawCreateFn directdraw_create;
    GetForwardInfoFn get_forward_info;
    PresentationForwardInfoV1 *forward_info;
    char expected_candidate_path[MAX_PATH];
    char candidate_path[MAX_PATH];
    char opened_file_path[MAX_PATH];
    DWORD opened_file_path_length = 0UL;
    DWORD open_file_error = 0UL;
    HMODULE kernel_module;
    GetFinalPathNameByHandleAFn get_final_path;
    HANDLE system_file;
    void *object;
    HRESULT result;
    ULONG release_result = 0UL;
    int release_called = 0;

    if (argc != 2)
        return fail("expected the absolute source-built candidate path");
    if (!GetFullPathNameA(argv[1], sizeof(expected_candidate_path),
                          expected_candidate_path, 0))
        return fail("could not resolve expected candidate path");
    candidate = LoadLibraryA("ddraw.dll");
    if (candidate == 0)
        return fail("source-built candidate ddraw.dll did not load");
    if (GetModuleFileNameA(candidate, candidate_path, sizeof(candidate_path)) == 0)
        return fail("could not read candidate module path");
    if (lstrcmpiA(candidate_path, expected_candidate_path) != 0)
        return fail("loaded module path is not the source-built candidate");

    export_address = GetProcAddress(candidate, "DirectDrawCreate");
    get_forward_info = (GetForwardInfoFn)GetProcAddress(candidate,
                                                       "ProbeGetForwardInfo");
    if (export_address == 0 || get_forward_info == 0)
        return fail("candidate export missing");
    directdraw_create = (DirectDrawCreateFn)export_address;
    object = 0;
    /* The only DirectDraw API call in this source-built executable. */
    result = directdraw_create(0, &object, 0);

    printf("RESULT=API_RETURNED\n");
    printf("API_HRESULT=0x%08lx\n",
           (unsigned long)(renderer_u32)result);
    printf("API_OBJECT=0x%08lx\n", (unsigned long)(renderer_u32)object);
    fflush(stdout);
    if (object != 0) {
        UnknownVtablePrefix *vtable =
            *(UnknownVtablePrefix **)object;
        if (vtable == 0 || vtable->release == 0) {
            printf("API_RELEASE_CALLED=0\n");
            printf("API_RELEASE_RETURN=UNAVAILABLE\n");
            return fail("returned object has no IUnknown::Release entry");
        }
        release_result = vtable->release(object);
        release_called = 1;
    }
    printf("API_RELEASE_CALLED=%d\n", release_called);
    printf("API_RELEASE_RETURN=0x%08lx\n",
           (unsigned long)release_result);

    forward_info = get_forward_info();
    if (forward_info == 0 ||
        forward_info->magic != 0x50465731UL)
        return fail("candidate forwarding diagnostics missing");

    kernel_module = GetModuleHandleA("kernel32.dll");
    get_final_path = kernel_module == 0 ? 0 :
        (GetFinalPathNameByHandleAFn)GetProcAddress(
            kernel_module, "GetFinalPathNameByHandleA");
    system_file = CreateFileA(forward_info->module_path, 0,
                              FILE_SHARE_READ | FILE_SHARE_WRITE |
                              FILE_SHARE_DELETE, 0, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (system_file != INVALID_HANDLE_VALUE) {
        if (get_final_path != 0) {
            opened_file_path_length = get_final_path(
                system_file, opened_file_path, sizeof(opened_file_path), 0UL);
        }
        if (opened_file_path_length == 0UL ||
            opened_file_path_length >= sizeof(opened_file_path))
            open_file_error = GetLastError();
        CloseHandle(system_file);
    } else {
        open_file_error = GetLastError();
    }

    printf("FORWARD_STATE=%lu\n", forward_info->state);
    printf("FORWARD_MODULE=0x%08lx\n",
           (unsigned long)forward_info->module);
    printf("FORWARD_EXPORT=0x%08lx\n",
           (unsigned long)forward_info->export_address);
    printf("FORWARD_REQUESTED_PATH=%s\n", forward_info->requested_path);
    printf("FORWARD_MODULE_PATH=%s\n", forward_info->module_path);
    printf("FORWARD_FILE_PATH=%s\n",
           opened_file_path_length > 0UL &&
           opened_file_path_length < sizeof(opened_file_path) ?
           opened_file_path : "");
    printf("FORWARD_FILE_OPEN_ERROR=0x%08lx\n",
           (unsigned long)open_file_error);
    printf("FORWARD_MACHINE=0x%04lx\n",
           (unsigned long)forward_info->machine);
    printf("FORWARD_TIMESTAMP=0x%08lx\n",
           (unsigned long)forward_info->timestamp);
    printf("FORWARD_IMAGE_SIZE=0x%08lx\n",
           (unsigned long)forward_info->image_size);
    printf("CANDIDATE_MODULE_PATH=%s\n", candidate_path);
    return 0;
}
