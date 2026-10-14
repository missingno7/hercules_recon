#ifndef HERCULES_HOST_WARM_DELIVERY_H
#define HERCULES_HOST_WARM_DELIVERY_H

#include <windows.h>
#include <dsound.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "host_callback_protocol_externs.h"

/* Minimum observed four-byte packet prefix; original declaration/TU unknown. */
typedef struct HostInputPacketPrefix {
    BYTE byte_000;
    BYTE byte_001;
    WORD buttons_002;
} HostInputPacketPrefix;
typedef char host_input_packet_prefix_is_four[
    (sizeof(HostInputPacketPrefix) == 4) ? 1 : -1];

extern HostInputPacketPrefix *g_host_input_packet_68e78c;
extern EngineNoArgCallback g_host_secondary_callback_68e90c;
extern DWORD g_host_secondary_delivery_68e910;
extern DWORD g_host_audio_channel_ids_799848[24];
extern DWORD g_host_audio_channel_cursors_7998a8[24];
extern BYTE g_registry_config[72];
extern IDirectSoundBuffer *g_exe_799908[4800];

void __cdecl host_audio_poll_f820(void);
void __cdecl host_notify_secondary_be40(void);
void __cdecl host_warm_delivery_bcf0(void);
DWORD __cdecl host_register_input_packet_bd30(HostInputPacketPrefix *packet);
DWORD __cdecl host_poll_input_alias_bd50(void);

#ifdef __cplusplus
}
#endif
#endif
