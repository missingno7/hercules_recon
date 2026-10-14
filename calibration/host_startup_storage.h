#ifndef HERCULES_HOST_STARTUP_STORAGE_H
#define HERCULES_HOST_STARTUP_STORAGE_H
#include <windows.h>
#include <mmsystem.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Distinct accessed prefixes; original aggregates, declarations and TUs unknown. */
extern BYTE g_exe_b9e458[1024];
extern DWORD g_config_default_values_rva_36158[9];
extern DWORD g_config_table_a_rva_56e4f0[28];
extern DWORD g_config_table_b_rva_56e560[28];
extern DWORD g_startup_apply_guard_rva_51c450;
extern BYTE g_host_input_joy_available_rva_79e8a0[16];
extern JOYCAPSA g_host_input_joy_caps_rva_79e8b0[16];
extern DWORD g_host_input_joy_count_rva_7a0218;
extern DWORD g_host_input_last_state_first_rva_56e5d0;
extern DWORD g_host_input_last_state_second_rva_56e5d4;
extern DWORD g_host_input_pending_rva_7a0234;
extern BYTE g_host_input_window_active_rva_03a594;
#ifdef __cplusplus
}
#endif
#endif
