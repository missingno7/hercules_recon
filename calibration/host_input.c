#include <windows.h>
#include <mmsystem.h>
#include <string.h>

/* PC-backed input primitives; measured external prefixes only.
 * Original declarations/TU and native placement remain unproved. */
extern BYTE g_host_input_joy_available_rva_79e8a0[16];
extern JOYCAPSA g_host_input_joy_caps_rva_79e8b0[16];
extern DWORD g_host_input_joy_count_rva_7a0218;
extern DWORD g_host_input_last_state_first_rva_56e5d0;
extern DWORD g_host_input_last_state_second_rva_56e5d4;
extern DWORD g_host_input_pending_rva_7a0234;
extern BYTE g_host_input_window_active_rva_03a594;
extern DWORD g_config_table_a_rva_56e4f0[28];
extern DWORD g_config_table_b_rva_56e560[28];
extern BYTE g_registry_config[72];



#define INPUT_ENABLED (*(BYTE *)(g_registry_config + 0x14))
#define INPUT_JOYSTICK_ID (*(SHORT *)(g_registry_config + 0x16))
#define INPUT_KEY_0 (*(SHORT *)(g_registry_config + 0x1c))
#define INPUT_KEY_1 (*(SHORT *)(g_registry_config + 0x1e))
#define INPUT_KEY_2 (*(SHORT *)(g_registry_config + 0x20))
#define INPUT_KEY_3 (*(SHORT *)(g_registry_config + 0x22))
#define INPUT_KEY_4 (*(SHORT *)(g_registry_config + 0x24))
#define INPUT_KEY_5 (*(SHORT *)(g_registry_config + 0x26))
#define INPUT_KEY_6 (*(SHORT *)(g_registry_config + 0x28))
#define INPUT_KEY_7 (*(SHORT *)(g_registry_config + 0x2a))
#define INPUT_KEY_8 (*(SHORT *)(g_registry_config + 0x2c))

/* No full native aggregate is implied by these measured accesses. */
typedef char input_joyinfo_size_must_be_16[(sizeof(JOYINFO) == 16) ? 1 : -1];
typedef char input_joyinfoex_size_must_be_52[(sizeof(JOYINFOEX) == 52) ? 1 : -1];
typedef char input_joycaps_size_must_be_404[(sizeof(JOYCAPSA) == 404) ? 1 : -1];

void host_input_initialize_7bb0(void)
{
    JOYINFO position;
    JOYCAPSA *capabilities;
    int device;
    int key;
    JOYCAPSA *capabilities_end;

    memset(g_host_input_joy_caps_rva_79e8b0, 0, 16 * sizeof(JOYCAPSA));
    g_host_input_joy_count_rva_7a0218 = 0;
    capabilities = &g_host_input_joy_caps_rva_79e8b0[0];
    capabilities_end = &g_host_input_joy_caps_rva_79e8b0[16];
    for (device = 0; capabilities < capabilities_end; ++device, ++capabilities) {
        g_host_input_joy_available_rva_79e8a0[device] = 0;
        if (joyGetPos(device, &position) == JOYERR_NOERROR &&
            joyGetDevCapsA(device, capabilities, sizeof(JOYCAPSA)) == JOYERR_NOERROR) {
            g_host_input_joy_available_rva_79e8a0[device] = 1;
            ++g_host_input_joy_count_rva_7a0218;
        }
    }
    for (key = 0; key < 256; ++key)
        GetAsyncKeyState((int)key);
    g_host_input_last_state_first_rva_56e5d0 = 0;
    g_host_input_last_state_second_rva_56e5d4 = 0;
}

DWORD host_input_sample_7c50(int device)
{
    DWORD button_masks[28] = {
        0x00000001UL, 0x00000002UL, 0x00000004UL, 0x00000008UL,
        0x00000010UL, 0x00000020UL, 0x00000040UL, 0x00000080UL,
        0x00000100UL, 0x00000200UL, 0x00000400UL, 0x00000800UL,
        0x00001000UL, 0x00002000UL, 0x00004000UL, 0x00008000UL,
        0x00010000UL, 0x00020000UL, 0x00040000UL, 0x00080000UL,
        0x00100000UL, 0x00200000UL, 0x00400000UL, 0x00800000UL,
        0x01000000UL, 0x02000000UL, 0x04000000UL, 0x08000000UL
    };
    JOYINFOEX position;
    JOYCAPSA *capabilities;
    DWORD result;
    DWORD index;
    DWORD range;
    LONG axis;

    if (g_host_input_joy_available_rva_79e8a0[device] == 0)
        return 0;
    memset(&position, 0, sizeof(position));
    position.dwSize = sizeof(position);
    position.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNBUTTONS;
    joyGetPosEx((UINT)device, &position);
    result = 0;
    for (index = 0; index < 28; ++index) {
        if ((position.dwButtons & button_masks[index]) != 0)
            result |= 1UL << index;
    }
    capabilities = &g_host_input_joy_caps_rva_79e8b0[device];
    range = capabilities->wXmax - capabilities->wXmin;
    axis = (LONG)range;
    if (range != 0)
        axis = (LONG)((position.dwXpos * 200UL - capabilities->wXmin * 200UL) / range) - 100;
    if (axis < -33)
        result |= 0x10000000UL;
    if (axis > 33)
        result |= 0x20000000UL;
    range = capabilities->wYmax - capabilities->wYmin;
    axis = (LONG)range;
    if (range != 0)
        axis = (LONG)((position.dwYpos * 200UL - capabilities->wYmin * 200UL) / range) - 100;
    if (axis < -33)
        result |= 0x40000000UL;
    if (axis > 33)
        result |= 0x80000000UL;
    return result;
}

DWORD host_input_joystick_7e90(void)
{
    SHORT device;
    DWORD buttons;
    DWORD result;
    DWORD index;

    device = INPUT_JOYSTICK_ID;
    if (device == (SHORT)0xffff)
        return 0;
    buttons = host_input_sample_7c50((int)device);
    result = 0;
    if (buttons & 0x10000000UL)
        result = 0x80;
    if (buttons & 0x20000000UL)
        result |= 0x20;
    if (buttons & 0x40000000UL)
        result |= 0x10;
    if (buttons & 0x80000000UL)
        result |= 0x40;
    buttons &= 0x0fffffffUL;
    for (index = 0; index < 28; ++index) {
        if ((buttons & g_config_table_a_rva_56e4f0[index]) == g_config_table_a_rva_56e4f0[index]) {
            result |= g_config_table_b_rva_56e560[index];
            buttons &= ~g_config_table_a_rva_56e4f0[index];
        }
    }
    return result;
}

void host_input_set_last_state_7f10(DWORD first, DWORD second)
{
    if (first != 0xffffffffUL)
        g_host_input_last_state_first_rva_56e5d0 = first;
    if (second != 0xffffffffUL)
        g_host_input_last_state_second_rva_56e5d4 = second;
}

DWORD host_input_keyboard_7f30(void)
{
    DWORD result;
    SHORT state;

    if (INPUT_ENABLED == 0)
        return 0;
    state = GetAsyncKeyState(0x12);
    if (((DWORD)(unsigned short)state & 0x8001UL) != 0)
        return 0;
    result = 0;
    if (GetAsyncKeyState(0x1b) & 0x8000)
        result = g_host_input_last_state_first_rva_56e5d0;
    if (GetAsyncKeyState(0x20) & 0x8000)
        result |= g_host_input_last_state_second_rva_56e5d4;
    if (GetAsyncKeyState(0x0d) & 0x8000)
        result |= g_host_input_last_state_second_rva_56e5d4;
    if (GetAsyncKeyState((int)INPUT_KEY_0) & 0x8000)
        result |= 0x10;
    if (GetAsyncKeyState((int)INPUT_KEY_1) & 0x8000)
        result |= 0x40;
    if (GetAsyncKeyState((int)INPUT_KEY_2) & 0x8000)
        result |= 0x80;
    if (GetAsyncKeyState((int)INPUT_KEY_3) & 0x8000)
        result |= 0x20;
    if (GetAsyncKeyState((int)INPUT_KEY_7) & 0x8000)
        result |= 0x1000;
    if (GetAsyncKeyState((int)INPUT_KEY_8) & 0x8000)
        result |= 0x800;
    if (GetAsyncKeyState((int)INPUT_KEY_5) & 0x8000)
        result |= 0x4000;
    if (GetAsyncKeyState((int)INPUT_KEY_4) & 0x8000)
        result |= 0x8000;
    if (GetAsyncKeyState((int)INPUT_KEY_6) & 0x8000)
        result |= 0x2000;
    return result;
}

DWORD host_input_collect_8040(void)
{
    DWORD result;

    result = host_input_keyboard_7f30();
    if (g_host_input_pending_rva_7a0234 != 0) {
        if (result != 0)
            result = 0;
        else
            g_host_input_pending_rva_7a0234 = 0;
    }
    result |= host_input_joystick_7e90();
    if (result & 0x10)
        result &= ~0x40UL;
    if (result & 0x80)
        result &= ~0x20UL;
    if (g_host_input_window_active_rva_03a594 == 0)
        result = 0;
    return result;
}
