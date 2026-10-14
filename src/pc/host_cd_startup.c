#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>

extern DWORD g_exe_96e61c;
extern DWORD g_exe_96e620;
extern DWORD g_exe_96e624;
extern DWORD g_exe_96e628;
extern DWORD g_exe_96e62c;
extern BOOL g_exe_96e630;
extern BYTE g_host_input_gate_rva_03a594;
extern HWND g_fatal_owner_window;
extern int __cdecl cleanup_native_9c30(void);

void __cdecl host_cd_validate_10520(void)
{
    char command[256];
    unsigned int expected_minutes[2] = {0, 34};
    unsigned int value, seconds, frames;
    int track;

    g_host_input_gate_rva_03a594 = 0;
    if (cleanup_native_9c30() == 0)
        return;
    if (mciSendStringA("status cdaudio number of tracks wait", command,
                       sizeof(command), 0) != 0)
        return;
    if (sscanf(command, "%u", &value) != 1 || value != 2)
        return;
    for (track = 1; track <= 2; ++track) {
        if (mciSendStringA("set cdaudio time format tmsf", 0, 0, 0) != 0)
            return;
        sprintf(command, "status cdaudio type track %u wait", track);
        if (mciSendStringA(command, command, sizeof(command), 0) != 0)
            return;
        value = _stricmp(command, "audio") == 0;
        if (value != (track != 1))
            return;
        if (track != 1) {
            sprintf(command, "status cdaudio length track %u wait", track);
            if (mciSendStringA(command, command, sizeof(command), 0) != 0)
                return;
            if (sscanf(command, "%u:%u:%u", &value, &seconds, &frames) != 3 ||
                value != expected_minutes[track - 1])
                return;
        }
    }
    g_host_input_gate_rva_03a594 = 1;
}

void __cdecl host_service_a0b0(void)
{
    int ready;
    g_exe_96e62c = 0;
    g_exe_96e628 = 0;
    g_exe_96e624 = 0;
    g_exe_96e620 = 0;
    g_exe_96e61c = 0;
    g_exe_96e630 = 0;
    do {
        ready = cleanup_native_9c30();
        host_cd_validate_10520();
        if (ready != 0 && g_host_input_gate_rva_03a594 != 0)
            break;
    } while (MessageBoxA(g_fatal_owner_window,
        "To play Hercules you must close all other CD applications and insert the game CD.",
        "Hercules Notice", MB_RETRYCANCEL | MB_ICONEXCLAMATION) != IDCANCEL);
}
