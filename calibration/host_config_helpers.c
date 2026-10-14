/* PC-backed Config defaults, application and input-table construction.
 * Existing providers stay separate; original declarations/TU unproved. */
#include <windows.h>
#include "engine_interface.h"

extern BYTE g_registry_config[72];
extern EngineInterface g_host_engine_interface;
extern void __cdecl audio_native_fdd0(unsigned short first,
                                      unsigned short second);
extern void __cdecl cleanup_native_1fb0(BYTE value);
extern void __cdecl cleanup_native_1fd0(BYTE value);

extern DWORD g_config_default_values_rva_36158[9];
extern DWORD g_config_table_a_rva_56e4f0[28];
extern DWORD g_config_table_b_rva_56e560[28];
extern DWORD g_startup_apply_guard_rva_51c450;

void __cdecl cleanup_native_4ac0(void)
{
    DWORD mode = *(DWORD *)(g_registry_config + 0x10);

    audio_native_fdd0((unsigned short)mode, (unsigned short)mode);
    if (g_startup_apply_guard_rva_51c450 != 0) {
        cleanup_native_1fb0((BYTE)*(DWORD *)(g_registry_config + 0x18));
        cleanup_native_1fd0(g_registry_config[0x0c]);
    }
}

void __cdecl cleanup_native_4be0(void)
{
    DWORD i;

    for (i = 0; i < 18; ++i)
        ((DWORD *)g_registry_config)[i] = 0;

    g_registry_config[0x04] = 1;
    g_registry_config[0x05] = 1;
    g_registry_config[0x06] = 1;
    *(DWORD *)(g_registry_config + 0x08) = 0x00010103;
    g_registry_config[0x0c] = 1;
    g_registry_config[0x0d] = 1;
    g_registry_config[0x0e] = 1;
    *(DWORD *)(g_registry_config + 0x10) = 0x00003fff;
    g_registry_config[0x14] = 1;
    *(unsigned short *)(g_registry_config + 0x16) = 0xffff;
    *(DWORD *)(g_registry_config + 0x18) = 3;
    *(unsigned short *)(g_registry_config + 0x1c) = (unsigned short)g_config_default_values_rva_36158[0];
    *(unsigned short *)(g_registry_config + 0x1e) = (unsigned short)g_config_default_values_rva_36158[1];
    *(unsigned short *)(g_registry_config + 0x20) = (unsigned short)g_config_default_values_rva_36158[2];
    *(unsigned short *)(g_registry_config + 0x22) = (unsigned short)g_config_default_values_rva_36158[3];
    *(unsigned short *)(g_registry_config + 0x24) = (unsigned short)g_config_default_values_rva_36158[4];
    *(unsigned short *)(g_registry_config + 0x26) = (unsigned short)g_config_default_values_rva_36158[5];
    *(unsigned short *)(g_registry_config + 0x28) = (unsigned short)g_config_default_values_rva_36158[6];
    *(unsigned short *)(g_registry_config + 0x2a) = (unsigned short)g_config_default_values_rva_36158[7];
    *(unsigned short *)(g_registry_config + 0x2c) = (unsigned short)g_config_default_values_rva_36158[8];

    cleanup_native_4ac0();
}

BYTE __cdecl cleanup_native_7b70(DWORD level, DWORD selected_index,
                                 DWORD config_value, DWORD mask)
{
    DWORD bit;
    DWORD population = 0;

    for (bit = 0; bit < 28; ++bit)
        if ((config_value >> bit) & 1)
            ++population;

    if (population != level)
        return 0;
    g_config_table_a_rva_56e4f0[selected_index] = config_value;
    g_config_table_b_rva_56e560[selected_index] = mask;
    return 1;
}

void __cdecl cleanup_native_7aa0(void)
{
    DWORD i;
    DWORD selected = 0;
    int level;

    for (i = 0; i < 28; ++i) {
        g_config_table_a_rva_56e4f0[i] = 0;
        g_config_table_b_rva_56e560[i] = 0;
    }

    for (level = 3; level >= 1; --level) {
        if (cleanup_native_7b70((DWORD)level, selected,
                               *(DWORD *)(g_registry_config + 0x30), 0x8000))
            ++selected;
        if (cleanup_native_7b70((DWORD)level, selected,
                               *(DWORD *)(g_registry_config + 0x34), 0x4000))
            ++selected;
        if (cleanup_native_7b70((DWORD)level, selected,
                               *(DWORD *)(g_registry_config + 0x38), 0x2000))
            ++selected;
        if (cleanup_native_7b70((DWORD)level, selected,
                               *(DWORD *)(g_registry_config + 0x3c), 0x1000))
            ++selected;
        if (cleanup_native_7b70((DWORD)level, selected,
                               *(DWORD *)(g_registry_config + 0x40), 0x0800))
            ++selected;
        if (cleanup_native_7b70((DWORD)level, selected,
                               *(DWORD *)(g_registry_config + 0x44), 0x0008))
            ++selected;
    }
}
