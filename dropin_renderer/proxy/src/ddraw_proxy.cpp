#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <cstring>
#include <cwchar>

#include "dropin_renderer/ddraw_proxy.hpp"
#if defined(DROPIN_HOST_SHIM)
#include "dropin_renderer/host_bootstrap.h"
#endif
#if defined(DROPIN_PROXY_TESTING)
#include "dropin_renderer/proxy_test_support.hpp"
#endif

namespace {

using DirectDrawCreateFunction = HRESULT (WINAPI *)(
    GUID *, IDirectDraw **, IUnknown *);

struct ForwarderState {
    HMODULE module;
    DirectDrawCreateFunction create;
    DWORD error;
};

INIT_ONCE g_forward_once = INIT_ONCE_STATIC_INIT;
ForwarderState g_forwarder = {};
HMODULE g_proxy_module = nullptr;

#if defined(DROPIN_PROXY_TESTING)
std::atomic<DirectDrawCreateFunction> g_test_backend{nullptr};
#endif

bool same_full_path(const wchar_t *expected, const wchar_t *actual) noexcept
{
    wchar_t full_expected[MAX_PATH] = {};
    wchar_t full_actual[MAX_PATH] = {};
    const DWORD expected_length = GetFullPathNameW(
        expected, MAX_PATH, full_expected, nullptr);
    const DWORD actual_length = GetFullPathNameW(
        actual, MAX_PATH, full_actual, nullptr);

    if (expected_length == 0 || expected_length >= MAX_PATH ||
        actual_length == 0 || actual_length >= MAX_PATH)
        return false;

    return CompareStringOrdinal(full_expected, -1, full_actual, -1, TRUE) ==
        CSTR_EQUAL;
}

bool expected_mapped_system_path(const wchar_t *logical_path,
                                 wchar_t *mapped_path,
                                 UINT capacity) noexcept
{
    wchar_t wow64_directory[MAX_PATH] = {};
    SetLastError(ERROR_SUCCESS);
    const UINT wow64_length = GetSystemWow64DirectoryW(
        wow64_directory, static_cast<UINT>(MAX_PATH));
    if (wow64_length == 0) {
        const DWORD error = GetLastError();
        if (error != ERROR_CALL_NOT_IMPLEMENTED &&
            error != ERROR_INVALID_FUNCTION) {
            g_forwarder.error = error == ERROR_SUCCESS
                ? ERROR_PATH_NOT_FOUND : error;
            return false;
        }

        const size_t logical_length = std::wcslen(logical_path) + 1U;
        if (logical_length > capacity) {
            g_forwarder.error = ERROR_INSUFFICIENT_BUFFER;
            return false;
        }
        std::wmemcpy(mapped_path, logical_path, logical_length);
        return true;
    }
    if (wow64_length >= MAX_PATH) {
        g_forwarder.error = ERROR_INSUFFICIENT_BUFFER;
        return false;
    }

    UINT length = wow64_length;
    if (wow64_directory[length - 1] != L'\\')
        wow64_directory[length++] = L'\\';
    static constexpr wchar_t file_name[] = L"ddraw.dll";
    if (length + (sizeof(file_name) / sizeof(file_name[0])) > capacity) {
        g_forwarder.error = ERROR_INSUFFICIENT_BUFFER;
        return false;
    }
    std::wmemcpy(wow64_directory + length, file_name,
                 sizeof(file_name) / sizeof(file_name[0]));
    std::wmemcpy(mapped_path, wow64_directory,
                 length + (sizeof(file_name) / sizeof(file_name[0])));
    return true;
}

bool canonical_file_matches(const wchar_t *opened_path,
                            const wchar_t *expected_path) noexcept
{
    HANDLE file = CreateFileW(opened_path, FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE |
                                  FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    wchar_t final_path[MAX_PATH] = {};
    const DWORD final_length = GetFinalPathNameByHandleW(
        file, final_path, static_cast<DWORD>(MAX_PATH),
        FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    CloseHandle(file);
    if (final_length == 0 || final_length >= MAX_PATH)
        return false;

    const wchar_t *comparable_path = final_path;
    if (final_path[0] == L'\\' && final_path[1] == L'\\' &&
        final_path[2] == L'?' && final_path[3] == L'\\')
        comparable_path += 4;
    return same_full_path(expected_path, comparable_path);
}

BOOL CALLBACK resolve_system_directdraw(
    PINIT_ONCE, PVOID, PVOID *) noexcept
{
    wchar_t requested_path[MAX_PATH] = {};
    wchar_t loaded_path[MAX_PATH] = {};
    wchar_t expected_loaded_path[MAX_PATH] = {};
    const UINT directory_length = GetSystemDirectoryW(
        requested_path, static_cast<UINT>(MAX_PATH));
    if (directory_length == 0 || directory_length >= MAX_PATH) {
        g_forwarder.error = GetLastError();
        if (g_forwarder.error == ERROR_SUCCESS)
            g_forwarder.error = ERROR_INSUFFICIENT_BUFFER;
        return TRUE;
    }

    UINT path_length = directory_length;
    if (requested_path[path_length - 1] != L'\\')
        requested_path[path_length++] = L'\\';
    static constexpr wchar_t file_name[] = L"ddraw.dll";
    if (path_length + (sizeof(file_name) / sizeof(file_name[0])) > MAX_PATH) {
        g_forwarder.error = ERROR_INSUFFICIENT_BUFFER;
        return TRUE;
    }
    std::wmemcpy(requested_path + path_length, file_name,
                 sizeof(file_name) / sizeof(file_name[0]));
    if (!expected_mapped_system_path(requested_path, expected_loaded_path,
                                     static_cast<UINT>(MAX_PATH)))
        return TRUE;

    HMODULE module = LoadLibraryW(requested_path);
    if (module == nullptr) {
        g_forwarder.error = GetLastError();
        if (g_forwarder.error == ERROR_SUCCESS)
            g_forwarder.error = ERROR_MOD_NOT_FOUND;
        return TRUE;
    }

    if (module == g_proxy_module) {
        FreeLibrary(module);
        g_forwarder.error = ERROR_BAD_EXE_FORMAT;
        return TRUE;
    }

    const DWORD loaded_length = GetModuleFileNameW(
        module, loaded_path, static_cast<DWORD>(MAX_PATH));
    if (loaded_length == 0 || loaded_length >= MAX_PATH ||
        (!same_full_path(requested_path, loaded_path) &&
         !same_full_path(expected_loaded_path, loaded_path)) ||
        !canonical_file_matches(requested_path, expected_loaded_path)) {
        const DWORD path_error = GetLastError();
        FreeLibrary(module);
        g_forwarder.error = path_error == ERROR_SUCCESS
            ? ERROR_BAD_PATHNAME : path_error;
        return TRUE;
    }

    FARPROC address = GetProcAddress(module, "DirectDrawCreate");
    if (address == nullptr) {
        g_forwarder.error = GetLastError();
        if (g_forwarder.error == ERROR_SUCCESS)
            g_forwarder.error = ERROR_PROC_NOT_FOUND;
        FreeLibrary(module);
        return TRUE;
    }

    static_assert(sizeof(FARPROC) == sizeof(DirectDrawCreateFunction),
                  "Win32 export and function pointer widths must match");
    DirectDrawCreateFunction exported_create = nullptr;
    std::memcpy(&exported_create, &address, sizeof(exported_create));
    FARPROC proxy_export = nullptr;
    const DirectDrawCreateFunction proxy_create = &DirectDrawCreate;
    std::memcpy(&proxy_export, &proxy_create, sizeof(proxy_export));
    if (address == proxy_export) {
        FreeLibrary(module);
        g_forwarder.error = ERROR_BAD_EXE_FORMAT;
        return TRUE;
    }

    /* Keep this reference for process lifetime: the export address remains
     * callable until process teardown, independent of COM object lifetime. */
    g_forwarder.module = module;
    g_forwarder.create = exported_create;
    g_forwarder.error = ERROR_SUCCESS;
    return TRUE;
}

DirectDrawCreateFunction get_system_directdraw() noexcept
{
    if (!InitOnceExecuteOnce(&g_forward_once, resolve_system_directdraw,
                             nullptr, nullptr)) {
        return nullptr;
    }
    return g_forwarder.create;
}

} // namespace

extern "C" HRESULT WINAPI DirectDrawCreate(
    GUID *guid, IDirectDraw **out_object, IUnknown *outer)
{
    const DWORD incoming_error = GetLastError();
#if defined(DROPIN_PROXY_TESTING)
    const DirectDrawCreateFunction test_backend =
        g_test_backend.load(std::memory_order_acquire);
    if (test_backend != nullptr)
        return test_backend(guid, out_object, outer);
#endif

    const DirectDrawCreateFunction original = get_system_directdraw();
    if (original == nullptr) {
        DWORD error = g_forwarder.error;
        if (error == ERROR_SUCCESS)
            error = ERROR_MOD_NOT_FOUND;
        SetLastError(error);
        return HRESULT_FROM_WIN32(error);
    }

    /* Lazy module resolution runs Win32 loader/path APIs. Restore the
     * caller's last-error value before the underlying API sees the call. */
    SetLastError(incoming_error);
    /* No logging, object wrapping, reference-count changes, or Win32 calls
     * follow the forwarded call, so its HRESULT, output, and last error pass
     * directly back to the importing caller. */
    return original(guid, out_object, outer);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_proxy_module = instance;
#if defined(DROPIN_HOST_SHIM)
        const DWORD incoming_error = GetLastError();
        hs_bootstrap_attach();
        SetLastError(incoming_error);
#endif
    }
    return TRUE;
}

#if defined(DROPIN_PROXY_TESTING)
namespace dropin_renderer::proxy::testing {

bool set_directdraw_backend(
    HRESULT (WINAPI *backend)(GUID *, IDirectDraw **, IUnknown *)) noexcept
{
    if (backend == nullptr)
        return false;
    DirectDrawCreateFunction expected = nullptr;
    return g_test_backend.compare_exchange_strong(
        expected, backend, std::memory_order_release,
        std::memory_order_relaxed);
}

} // namespace dropin_renderer::proxy::testing
#endif
