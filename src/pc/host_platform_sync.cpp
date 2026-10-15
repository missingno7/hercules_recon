#include "../../calibration/display_mode_contract.h"
/* Original PC platform-state callback hypothesis; source/TU identity unproved. */
#include <windows.h>
#include "../../calibration/host_startup_storage.h"

extern "C" {
extern HWND g_fatal_owner_window;
extern signed char g_exe_ba0238;
extern BYTE g_exe_ba0228;
extern DWORD g_exe_ba0230;


extern BYTE g_registry_config[72];
extern BYTE g_exe_91c454;
extern BYTE g_exe_91c458;
extern int g_exe_91c46c;

extern char g_exe_436810[];
extern char g_exe_43682c[];
extern char g_exe_43684c[];

BYTE g_exe_436638 = 0xff;
BYTE g_exe_43663c = 0xff;
DWORD g_exe_436640 = 0xff;
BYTE g_exe_436644 = 0xff;
BYTE g_exe_436648 = 0xff;
BYTE g_exe_43664c = 0xff;
DWORD g_exe_436650 = 0xff;

extern void __cdecl graphics_native_03770(void);
extern void __cdecl cleanup_native_9d60(void);
extern void __cdecl host_cd_unpause_9db0(void);
extern void __cdecl host_cd_resume_9f60(void);
extern void __cdecl host_sound_pause_ffb0(void);
extern void __cdecl audio_native_10010(void);
extern void __cdecl host_sound_resume_10030(void);

void __cdecl graphics_native_05a50(void)
{
    HWND window = g_fatal_owner_window;
    signed char requested_mode;
    signed char requested_type;
    DWORD requested_depth;

    if (window == 0)
        return;

    requested_mode = g_exe_ba0238;
    requested_type = (signed char)g_exe_ba0228;
    requested_depth = g_exe_ba0230;

    if ((DWORD)g_exe_436638 == requested_mode &&
        (DWORD)g_exe_43663c == requested_type &&
        g_exe_436640 == requested_depth &&
        g_exe_436644 == g_host_input_gate_rva_03a594 &&
        g_exe_436648 == g_registry_config[14] &&
        g_exe_43664c == g_registry_config[13] &&
        g_exe_436650 == (*(DWORD *)(g_registry_config + 16)))
        return;

    if (requested_depth != 0) {
        g_host_input_pending_rva_7a0234 = 1;
        g_exe_91c46c = 1;
    }

    if (requested_mode >= 2 && requested_type == 0 && requested_depth == 0) {
        if (g_host_input_gate_rva_03a594 != 0)
            SetWindowTextA(window, g_exe_43684c);
        else
            SetWindowTextA(window, g_exe_43682c);

        if (g_exe_91c458 != 0 || g_registry_config[14] != g_exe_436648) {
            g_exe_91c458 = 0;
            if (g_registry_config[14] != 0)
                host_sound_resume_10030();
            else
                audio_native_10010();
        }

        if (g_exe_91c454 != 0 || g_registry_config[13] != g_exe_43664c) {
            g_exe_91c454 = 0;
            if (g_registry_config[13] != 0)
                host_cd_resume_9f60();
            else
                host_cd_unpause_9db0();
        }
    } else {
        SetWindowTextA(window, g_exe_436810);
        cleanup_native_6ec0(0);
        graphics_native_03770();

        if (g_exe_ba0230 != 0) {
            g_exe_91c458 = 1;
            host_sound_pause_ffb0();
        }

        if (g_exe_ba0238 < 2 || g_exe_ba0228 != 0) {
            g_exe_91c454 = 1;
            cleanup_native_9d60();
        }
    }

    g_exe_436638 = (BYTE)g_exe_ba0238;
    g_exe_43663c = (BYTE)g_exe_ba0228;
    g_exe_436640 = g_exe_ba0230;
    g_exe_436644 = g_host_input_gate_rva_03a594;
    g_exe_436648 = g_registry_config[14];
    g_exe_43664c = g_registry_config[13];
    g_exe_436650 = (*(DWORD *)(g_registry_config + 16));
}
}
