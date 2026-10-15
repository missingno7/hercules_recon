#include <windows.h>
extern BYTE g_registry_config[72];
extern int g_restart_level_rva_215c90;
int cleanup_native_4b50(int level)
{
    if (level != -1) {
        if (level == -2) {
            DWORD packed = g_restart_level_rva_215c90;
            packed = (packed << 8) | (*(DWORD*)(g_registry_config+12) & 0xff);
            packed = (packed << 8) | *(DWORD*)(g_registry_config+24);
            return packed;
        }
        if (level == 0) {
            g_restart_level_rva_215c90 = 0;
            return 0;
        }
        g_restart_level_rva_215c90 = level > g_registry_config[4+(*(DWORD*)(g_registry_config+12)&0xff)] ? 0 : level;
    }
    return g_restart_level_rva_215c90;
}
