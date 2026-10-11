#include "engine_interface.h"

typedef unsigned char BYTE;
typedef unsigned long DWORD;
typedef long LONG;
typedef void *HKEY;
typedef const char *LPCSTR;

__declspec(dllimport) LONG __stdcall RegCreateKeyExA(HKEY, LPCSTR, DWORD, char *, DWORD, DWORD, void *, HKEY *, DWORD *);
__declspec(dllimport) LONG __stdcall RegSetValueExA(HKEY, LPCSTR, DWORD, DWORD, const BYTE *, DWORD);
__declspec(dllimport) LONG __stdcall RegCloseKey(HKEY);

extern BYTE g_registry_config[72];
/* EXE source table, distinct from engine copied destination tables. */
extern EngineInterface g_host_engine_interface;
extern char g_registry_key[];
extern char g_registry_value_name[];

int __cdecl cleanup_native_1fe0(void)
{
    return ((signed char *)g_host_engine_interface.context_004)[14];
}

void __cdecl cleanup_native_4b00(void)
{
    g_registry_config[12] = (BYTE)cleanup_native_1fe0();
}

DWORD __cdecl cleanup_native_4cc0(void)
{
    DWORD sum = 0x94375494;
    DWORD offset;
    for (offset = 4; offset < 0x44; ++offset)
        sum += *(DWORD *)(g_registry_config + offset);
    return sum;
}

void __cdecl cleanup_native_4da0(void)
{
    HKEY key = 0;
    DWORD disposition;

    cleanup_native_4b00();
    *(DWORD *)g_registry_config = cleanup_native_4cc0();
    RegCreateKeyExA((HKEY)0x80000002, g_registry_key, 0, 0, 0, 0x20006, 0,
                    &key, &disposition);
    if (key != 0) {
        RegSetValueExA(key, g_registry_value_name, 0, 3, g_registry_config, 0x48);
        RegCloseKey(key);
    }
}
