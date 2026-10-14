#include <windows.h>
#include <winreg.h>

extern BYTE g_registry_config[72];
extern char g_registry_key[];
extern BYTE g_exe_b9e458[];
extern DWORD __cdecl cleanup_native_4cc0(void);
extern void __cdecl cleanup_native_4be0(void);
extern void __cdecl cleanup_native_7aa0(void);
extern void __cdecl cleanup_native_4ac0(void);

int __cdecl host_registry_read_path(void)
{
    HKEY key = (HKEY)0;
    DWORD bytes = 0x400;

    if (RegOpenKeyExA((HKEY)0x80000002, g_registry_key, 0, 0x20019, &key) != 0)
        return 0;
    if (RegQueryValueExA(key, "Path", 0, 0,
                         g_exe_b9e458, &bytes) != 0)
        return 0;
    RegCloseKey(key);
    return SetCurrentDirectoryA((LPCSTR)g_exe_b9e458) != 0;
}

void __cdecl host_registry_read_config(void)
{
    HKEY key = (HKEY)0;
    DWORD bytes = 0;
    DWORD stored;
    DWORD checksum;
    LONG level;

    RegOpenKeyExA((HKEY)0x80000002, g_registry_key, 0, 0x20019, &key);
    if (key != (HKEY)0) {
        bytes = 0x48;
        if (RegQueryValueExA(key, "Config", 0, 0,
                             g_registry_config, &bytes) == 0 && bytes == 0x48) {
            RegCloseKey(key);
            checksum = cleanup_native_4cc0();
            stored = *(DWORD *)g_registry_config;
            level = *(LONG *)(g_registry_config + 0x18);
            if ((stored == checksum || stored == (DWORD)-1) &&
                level >= 2 && level <= 4 && g_registry_config[0x0c] <= 2) {
                cleanup_native_7aa0();
                cleanup_native_4ac0();
                return;
            }
        }
        if (key != (HKEY)0)
            RegCloseKey(key);
    }
    cleanup_native_4be0();
    cleanup_native_7aa0();
    cleanup_native_4ac0();
}
