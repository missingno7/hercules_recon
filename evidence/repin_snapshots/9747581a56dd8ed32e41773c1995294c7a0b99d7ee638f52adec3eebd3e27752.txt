#include "engine_dispatch_externs.h"

void __cdecl engine_dispatch(void)
{
    u32 entry_state;
    u32 descriptor;
    u32 interface_argument;
    u8 *frame_base;
    u8 *frame_current;

    entry_state = g_dispatch_entry_state_6fcec;
    if (entry_state == 0) {
        dispatch_engine_2c860(8, 8);
        g_engine_interface.context_004->unknown_000[1] = 0;
        g_engine_interface.context_004->unknown_000[2] = 0;
        g_engine_interface.context_004->unknown_000[0x0a] = 1;
        g_engine_interface.context_004->unknown_08c = g_dispatch_global_58b00;
        g_engine_interface.context_004->unknown_094 = g_dispatch_global_58af8;
        g_engine_interface.context_004->unknown_048 = 0x13546547UL;
        g_engine_interface.context_004->unknown_04c = 0xecab9ab8UL;
        dispatch_engine_2c350();
        dispatch_local_083d3();
        dispatch_local_082b7(0);
        descriptor = g_dispatch_descriptors_5a6a4[
            ((int)(s8)g_engine_interface.context_004->unknown_000[0]) * 6];
        dispatch_engine_2c3b0(descriptor);
        dispatch_engine_2cc30(1);
        dispatch_local_0827f(1);
        g_dispatch_startup_gate_6fce8 = 0;
        dispatch_local_1f0f0();
        dispatch_local_1f750();
        dispatch_local_32b10();
        dispatch_local_255f0();
        dispatch_local_05ea0();
        dispatch_engine_2c840(
            (int)(s8)g_engine_interface.context_004->unknown_000[0], 0);
    } else if (entry_state == 1) {
        goto iteration;
    } else {
        return;
    }

restart:
    dispatch_local_1f320();
    dispatch_engine_2c5a0(4);
    dispatch_engine_2c360();
    if (g_dispatch_startup_gate_6fce8 == 0)
        dispatch_engine_2c540(0, 0);
    g_dispatch_startup_gate_6fce8 = 1;
    g_dispatch_callback_flag_71ec0 = 0;
    dispatch_engine_2cc40(dispatch_completion_gate);
    g_dispatch_key_7286c = 8;
    g_dispatch_setting_4d858 = 128;
    g_dispatch_word_6ee80 = 0;
    dispatch_engine_2c4e0(0);
    g_engine_interface.context_004->unknown_1b30 = 256;
    dispatch_engine_2c520(1);
    g_engine_interface.context_004->unknown_03c = g_dispatch_global_6ff88;
    g_engine_interface.context_004->unknown_000[4] = 0;
    g_engine_interface.context_004->unknown_000[5] = 1;
    g_engine_interface.context_004->unknown_040 = 0;
    g_engine_interface.context_004->unknown_000[3] = 5;
    g_engine_interface.context_004->unknown_054 = dispatch_engine_2cc30(1);
    dispatch_local_01000(g_dispatch_diagnostic_58ac8);
    g_dispatch_entry_state_6fcec = 1;

iteration:
    dispatch_engine_2c270();
    if (g_engine_interface.context_004->unknown_000[0] == 3)
        dispatch_local_0e670();
    interface_argument = g_engine_interface.opaque_010[0] + 0x400UL;
    g_engine_interface.context_004->unknown_034 =
        dispatch_engine_2ca40(interface_argument);
    dispatch_engine_2c420();
    dispatch_local_06005();
    dispatch_local_02180();
    dispatch_local_1f870(1);
    dispatch_engine_2c440();
    dispatch_local_05600();
    if (g_engine_interface.context_004->unknown_000[4] == 0) {
        dispatch_local_2fb70();
        dispatch_local_2f840();
        dispatch_local_258e0();
        dispatch_local_034d0();
        dispatch_local_156a0();
        dispatch_local_1fd90();
        dispatch_local_25d00();
    } else {
        dispatch_local_034d0();
        dispatch_local_256e0();
    }
    dispatch_local_03470();
    dispatch_local_05d50();
    dispatch_local_03a50();
    if (g_engine_interface.context_004->unknown_040 == 0)
        dispatch_local_32bd0();
    if (g_engine_interface.context_004->unknown_040 == 0)
        dispatch_local_2da90();
    dispatch_local_1f6f0();
    dispatch_engine_2c630(0);
    if (g_dispatch_global_6ff88 <
        g_engine_interface.context_004->unknown_03c + 1UL)
        dispatch_engine_2cc30(0);
    g_dispatch_callback_flag_71ec0 = 0;
    g_engine_interface.context_004->unknown_054 = dispatch_engine_2cc30(0);
    g_dispatch_callback_flag_71ec0 = 1;
    g_engine_interface.context_004->unknown_03c = g_dispatch_global_6ff88;
    dispatch_engine_2c570(g_dispatch_frame_current_71ec4);
    if (g_engine_interface.context_004->unknown_000[4] == 0)
        dispatch_local_02cb0();
    dispatch_local_050d0();
    dispatch_local_058b0();
    dispatch_engine_2c620(g_dispatch_frame_current_71ec4 + 0x88);
    dispatch_local_0666b();
    dispatch_engine_2ca40(g_engine_interface.context_004->unknown_034);
    frame_base = g_dispatch_frame_base_71ec8;
    frame_current = g_dispatch_frame_current_71ec4;
    if (frame_current == frame_base)
        frame_base += 0x1488;
    g_dispatch_frame_current_71ec4 = frame_base;
    ++g_engine_interface.context_004->tick_038;
    if (dispatch_local_060f9() != 0)
        goto restart;
}
