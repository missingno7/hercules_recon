/* PC-backed host input, audio and callback-delivery recovery.
 * Original declarations/TU and native placement remain unproved. */
#include "host_warm_delivery.h"

extern "C" {
extern DWORD __cdecl host_poll_input_8040(void);

void __cdecl host_audio_poll_f820(void)
{
    int slot;
    DWORD id;
    DWORD status;
    DWORD write_cursor;
    IDirectSoundBuffer *buffer;

    for (slot = 0; slot < 24; ++slot) {
        id = g_host_audio_channel_ids_799848[slot];
        if (id != 0xffffffffUL) {
            g_host_audio_channel_cursors_7998a8[slot] = 0;
            buffer = g_exe_799908[(id & 0xffffUL) + (id >> 16) * 4];
            if (buffer == 0) {
                g_host_audio_channel_ids_799848[slot] = 0xffffffffUL;
            } else if (g_registry_config[0x0e] != 0) {
                buffer->GetStatus(&status);
                if ((status & DSBSTATUS_PLAYING) == 0) {
                    g_host_audio_channel_ids_799848[slot] = 0xffffffffUL;
                } else {
                    /* Native reloads the channel id and sound-bank entry after
                     * GetStatus, before invoking GetCurrentPosition. */
                    id = g_host_audio_channel_ids_799848[slot];
                    buffer = g_exe_799908[(id & 0xffffUL) + (id >> 16) * 4];
                    buffer->GetCurrentPosition(
                        &g_host_audio_channel_cursors_7998a8[slot], &write_cursor);
                }
            }
        }
    }
}

void __cdecl host_notify_secondary_be40(void)
{
    EngineNoArgCallback callback;
    if (g_host_secondary_delivery_68e910 != 0) {
        callback = g_host_secondary_callback_68e90c;
        if (callback != 0 && g_host_callback_prefix.delivery_gate_008 != 0)
            callback();
    }
}

void __cdecl host_warm_delivery_bcf0(void)
{
    if (g_host_input_packet_68e78c != 0) {
        g_host_input_packet_68e78c->buttons_002 = (WORD)~host_poll_input_8040();
        g_host_input_packet_68e78c->byte_001 = 0x40;
        g_host_input_packet_68e78c->byte_000 = 0;
    }
    host_audio_poll_f820();
    host_invoke_callback_one();
    host_notify_secondary_be40();
}

DWORD __cdecl host_register_input_packet_bd30(HostInputPacketPrefix *packet)
{
    g_host_input_packet_68e78c = packet;
    return 1;
}

DWORD __cdecl host_poll_input_alias_bd50(void)
{
    return host_poll_input_8040();
}
}
