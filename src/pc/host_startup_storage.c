#include "host_startup_storage.h"

/* Working source storage for measured prefixes only. Original placement and
 * full capacities remain unproved; shared Config72 stays in registry_storage.c. */
BYTE g_exe_b9e458[1024] = {0};
DWORD g_config_default_values_rva_36158[9] = {
    0x26, 0x28, 0x25, 0x27, 0x58, 0x5a, 0x43, 0x44, 0x53
};
DWORD g_config_table_a_rva_56e4f0[28] = {0};
DWORD g_config_table_b_rva_56e560[28] = {0};
DWORD g_startup_apply_guard_rva_51c450 = 0;
BYTE g_host_input_joy_available_rva_79e8a0[16] = {0};
JOYCAPSA g_host_input_joy_caps_rva_79e8b0[16] = {0};
DWORD g_host_input_joy_count_rva_7a0218 = 0;
DWORD g_host_input_last_state_first_rva_56e5d0 = 0;
DWORD g_host_input_last_state_second_rva_56e5d4 = 0;
DWORD g_host_input_pending_rva_7a0234 = 0;
BYTE g_host_input_gate_rva_03a594 = 1;

typedef char input_caps_prefix_is_measured[
    (sizeof(g_host_input_joy_caps_rva_79e8b0) == 6464) ? 1 : -1];
