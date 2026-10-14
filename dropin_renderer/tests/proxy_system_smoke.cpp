#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>

#include <cstdio>
#include <cstring>
#include <cwchar>

#include "dropin_renderer/ddraw_proxy.hpp"

namespace {

using DirectDrawCreateFunction = HRESULT (WINAPI *)(
    GUID *, IDirectDraw **, IUnknown *);

struct ModuleIdentity {
    HMODULE module;
    wchar_t path[MAX_PATH];
    WORD machine;
    DWORD timestamp;
    DWORD image_size;
};

bool make_system_ddraw_path(wchar_t *path, DWORD capacity) noexcept
{
    const UINT directory_length = GetSystemDirectoryW(path, capacity);
    if (directory_length == 0 || directory_length >= capacity)
        return false;
    UINT length = directory_length;
    if (path[length - 1] != L'\\')
        path[length++] = L'\\';
    static constexpr wchar_t file_name[] = L"ddraw.dll";
    if (length + (sizeof(file_name) / sizeof(file_name[0])) > capacity)
        return false;
    std::wmemcpy(path + length, file_name,
                 sizeof(file_name) / sizeof(file_name[0]));
    return true;
}

bool make_expected_mapped_path(const wchar_t *logical_path,
                               wchar_t *mapped_path,
                               DWORD capacity) noexcept
{
    wchar_t wow64_directory[MAX_PATH] = {};
    SetLastError(ERROR_SUCCESS);
    const UINT wow64_length = GetSystemWow64DirectoryW(
        wow64_directory, static_cast<UINT>(MAX_PATH));
    if (wow64_length == 0) {
        const DWORD error = GetLastError();
        if (error != ERROR_CALL_NOT_IMPLEMENTED &&
            error != ERROR_INVALID_FUNCTION)
            return false;
        const size_t logical_length = std::wcslen(logical_path) + 1U;
        if (logical_length > capacity)
            return false;
        std::wmemcpy(mapped_path, logical_path, logical_length);
        return true;
    }
    if (wow64_length >= MAX_PATH)
        return false;
    UINT length = wow64_length;
    if (wow64_directory[length - 1] != L'\\')
        wow64_directory[length++] = L'\\';
    static constexpr wchar_t file_name[] = L"ddraw.dll";
    if (length + (sizeof(file_name) / sizeof(file_name[0])) > capacity)
        return false;
    std::wmemcpy(wow64_directory + length, file_name,
                 sizeof(file_name) / sizeof(file_name[0]));
    std::wmemcpy(mapped_path, wow64_directory,
                 length + (sizeof(file_name) / sizeof(file_name[0])));
    return true;
}

bool same_full_path(const wchar_t *left, const wchar_t *right) noexcept
{
    wchar_t full_left[MAX_PATH] = {};
    wchar_t full_right[MAX_PATH] = {};
    const DWORD left_length = GetFullPathNameW(
        left, MAX_PATH, full_left, nullptr);
    const DWORD right_length = GetFullPathNameW(
        right, MAX_PATH, full_right, nullptr);
    if (left_length == 0 || left_length >= MAX_PATH ||
        right_length == 0 || right_length >= MAX_PATH)
        return false;
    return CompareStringOrdinal(full_left, -1, full_right, -1, TRUE) ==
        CSTR_EQUAL;
}

bool canonical_file_path(const wchar_t *opened_path,
                         wchar_t *final_path,
                         DWORD capacity) noexcept
{
    HANDLE file = CreateFileW(opened_path, FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE |
                                  FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    const DWORD length = GetFinalPathNameByHandleW(
        file, final_path, capacity,
        FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    CloseHandle(file);
    return length != 0 && length < capacity;
}

const wchar_t *without_extended_prefix(const wchar_t *path) noexcept
{
    return path[0] == L'\\' && path[1] == L'\\' &&
           path[2] == L'?' && path[3] == L'\\'
        ? path + 4 : path;
}

bool read_identity(HMODULE module, ModuleIdentity *identity) noexcept
{
    if (module == nullptr || identity == nullptr)
        return false;
    const DWORD path_length = GetModuleFileNameW(
        module, identity->path, static_cast<DWORD>(MAX_PATH));
    if (path_length == 0 || path_length >= MAX_PATH)
        return false;
    identity->path[path_length] = L'\0';
    identity->module = module;

    const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(module);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 ||
        dos->e_lfanew > 0x1000)
        return false;
    const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(
        reinterpret_cast<const BYTE *>(module) + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
        return false;
    identity->machine = nt->FileHeader.Machine;
    identity->timestamp = nt->FileHeader.TimeDateStamp;
    identity->image_size = nt->OptionalHeader.SizeOfImage;
    return true;
}

bool find_loaded_system_ddraw(const wchar_t *logical_path,
                              const wchar_t *expected_mapped_path,
                              ModuleIdentity *identity) noexcept
{
    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    MODULEENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    bool found = false;
    if (Module32FirstW(snapshot, &entry)) {
        do {
            if (!same_full_path(logical_path, entry.szExePath) &&
                !same_full_path(expected_mapped_path, entry.szExePath))
                continue;
            if (!read_identity(entry.hModule, identity))
                break;
            found = true;
            break;
        } while (Module32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}

} // namespace

int run_system_smoke(const wchar_t *proxy_argument)
{
    wchar_t proxy_path[MAX_PATH] = {};
    const DWORD proxy_path_length = GetFullPathNameW(
        proxy_argument, MAX_PATH, proxy_path, nullptr);
    if (proxy_path_length == 0 || proxy_path_length >= MAX_PATH) {
        std::fwprintf(stderr, L"proxy path is invalid (error=%lu)\n",
                      GetLastError());
        return 3;
    }

    wchar_t system_path[MAX_PATH] = {};
    if (!make_system_ddraw_path(system_path, MAX_PATH)) {
        std::fwprintf(stderr, L"system ddraw path is unavailable (error=%lu)\n",
                      GetLastError());
        return 4;
    }
    wchar_t expected_mapped_path[MAX_PATH] = {};
    if (!make_expected_mapped_path(system_path, expected_mapped_path,
                                   MAX_PATH)) {
        std::fwprintf(stderr, L"expected system module path is unavailable\n");
        return 4;
    }

    HMODULE proxy_module = LoadLibraryW(proxy_path);
    if (proxy_module == nullptr) {
        std::fwprintf(stderr, L"proxy load failed (error=%lu)\n", GetLastError());
        return 5;
    }
    FARPROC proxy_address = GetProcAddress(proxy_module, "DirectDrawCreate");
    if (proxy_address == nullptr) {
        std::fwprintf(stderr, L"undecorated DirectDrawCreate export missing\n");
        FreeLibrary(proxy_module);
        return 6;
    }
    static_assert(sizeof(proxy_address) == sizeof(DirectDrawCreateFunction),
                  "Win32 export and function pointer widths must match");
    DirectDrawCreateFunction proxy_create = nullptr;
    std::memcpy(&proxy_create, &proxy_address, sizeof(proxy_create));

    /* No host, engine, window, display mode, or cooperative-level call is
     * made here. This checks only the source-built proxy's real system API
     * forwarder in a disposable diagnostic process. */
    GUID *default_guid = nullptr;
    IDirectDraw *object = nullptr;
    SetLastError(0x24681357UL);
    const HRESULT result = proxy_create(default_guid, &object, nullptr);
    const DWORD returned_error = GetLastError();

    ModuleIdentity system_identity = {};
    if (!find_loaded_system_ddraw(system_path, expected_mapped_path,
                                  &system_identity)) {
        std::fwprintf(stderr,
                      L"system DirectDraw module was not mapped by the forwarder; requested=%ls expected=%ls HRESULT=0x%08lx error=0x%08lx\n",
                      system_path, expected_mapped_path,
                      static_cast<DWORD>(result), returned_error);
        HANDLE snapshot = CreateToolhelp32Snapshot(
            TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
        if (snapshot != INVALID_HANDLE_VALUE) {
            MODULEENTRY32W entry = {};
            entry.dwSize = sizeof(entry);
            if (Module32FirstW(snapshot, &entry)) {
                do {
                    std::fwprintf(stderr, L"mapped_module=%ls\n", entry.szExePath);
                } while (Module32NextW(snapshot, &entry));
            }
            CloseHandle(snapshot);
        }
        if (object != nullptr)
            reinterpret_cast<IUnknown *>(object)->Release();
        FreeLibrary(proxy_module);
        return 7;
    }
    const FARPROC system_address = GetProcAddress(
        system_identity.module, "DirectDrawCreate");
    if (system_address == nullptr || system_address == proxy_address) {
        std::fwprintf(stderr,
                      L"system DirectDraw export is missing or resolves to proxy\n");
        if (object != nullptr)
            reinterpret_cast<IUnknown *>(object)->Release();
        FreeLibrary(proxy_module);
        return 8;
    }
    if (!same_full_path(system_path, system_identity.path) &&
        !same_full_path(expected_mapped_path, system_identity.path)) {
        std::fwprintf(stderr, L"mapped system module path differs from both system paths\n");
        if (object != nullptr)
            reinterpret_cast<IUnknown *>(object)->Release();
        FreeLibrary(proxy_module);
        return 9;
    }
    wchar_t canonical_path[MAX_PATH] = {};
    if (!canonical_file_path(system_path, canonical_path, MAX_PATH) ||
        !same_full_path(expected_mapped_path,
                        without_extended_prefix(canonical_path))) {
        std::fwprintf(stderr,
                      L"canonical system file path differs from WOW64-aware target: %ls\n",
                      canonical_path);
        if (object != nullptr)
            reinterpret_cast<IUnknown *>(object)->Release();
        FreeLibrary(proxy_module);
        return 10;
    }
    if (system_identity.machine != IMAGE_FILE_MACHINE_I386) {
        std::fwprintf(stderr, L"mapped DirectDraw image is not i386\n");
        if (object != nullptr)
            reinterpret_cast<IUnknown *>(object)->Release();
        FreeLibrary(proxy_module);
        return 11;
    }

    ULONG release_result = 0;
    bool released = false;
    if (object != nullptr) {
        release_result = reinterpret_cast<IUnknown *>(object)->Release();
        released = true;
    }

    std::wprintf(L"proxy_path=%ls\n", proxy_path);
    std::wprintf(L"requested_system_path=%ls\n", system_path);
    std::wprintf(L"mapped_system_path=%ls\n", system_identity.path);
    std::wprintf(L"mapped_base=0x%p machine=0x%04x timestamp=0x%08lx image_size=0x%08lx\n",
                 system_identity.module, system_identity.machine,
                 system_identity.timestamp, system_identity.image_size);
    std::wprintf(L"proxy_export=0x%p system_export=0x%p\n",
                 proxy_address, system_address);
    std::wprintf(L"hresult=0x%08lx object=0x%p last_error=0x%08lx\n",
                 static_cast<DWORD>(result), object, returned_error);
    if (released)
        std::wprintf(L"release_called=1 release_result=0x%08lx\n",
                     release_result);
    else
        std::wprintf(L"release_called=0\n");
    std::wprintf(L"expected_mapped_path=%ls\n", expected_mapped_path);
    std::wprintf(L"canonical_file_path=%ls\n", canonical_path);

    FreeLibrary(proxy_module);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::fprintf(stderr,
                     "usage: proxy_system_smoke.exe <production-ddraw.dll>\n");
        return 2;
    }
    wchar_t proxy_argument[MAX_PATH] = {};
    const int converted = MultiByteToWideChar(
        CP_ACP, 0, argv[1], -1, proxy_argument, MAX_PATH);
    if (converted == 0 || converted >= MAX_PATH) {
        std::fprintf(stderr, "proxy path conversion failed\n");
        return 3;
    }
    return run_system_smoke(proxy_argument);
}
