#include <windows.h>
#include <mmsystem.h>

/* Measured direct callees; unresolved service bodies retain address labels. */
extern void __cdecl host_set_config_apply_guard_5750(DWORD);
extern void __cdecl host_registry_read_config(void);
extern void __cdecl host_service_5020(void);
extern void __cdecl host_service_72a0(void);
extern void __cdecl host_input_initialize_7bb0(void);
extern void __cdecl host_service_a0b0(void);
extern void __cdecl host_service_fdf0(void);
extern void __cdecl host_service_1ff0(void);
extern void __cdecl cleanup_native_4ac0(void);
extern unsigned short __cdecl cleanup_native_6ec0(int);
extern void __cdecl host_normal_exit_104d0(void);
extern DWORD g_startup_apply_guard_rva_51c450;
extern BYTE g_host_input_gate_rva_03a594;
extern HWND g_fatal_owner_window;

void __cdecl host_startup_5e10(void)
{
    BYTE gate;
    timeBeginPeriod(1);
    host_set_config_apply_guard_5750(0);
    host_registry_read_config();
    host_service_5020();
    host_service_72a0();
    host_input_initialize_7bb0();
    host_service_a0b0();
    host_service_fdf0();
    host_service_1ff0();
    g_startup_apply_guard_rva_51c450 = 1;
    cleanup_native_4ac0();
    gate = g_host_input_gate_rva_03a594;
    g_startup_apply_guard_rva_51c450 = 0;
    if (gate == 0) {
        MessageBoxA(g_fatal_owner_window,
            "You cannot play Hercules without the CD in the drive.\n"
            "The game will now exit.", "Hercules Notice", MB_ICONERROR);
        host_normal_exit_104d0();
    }
    cleanup_native_6ec0(1);
}
