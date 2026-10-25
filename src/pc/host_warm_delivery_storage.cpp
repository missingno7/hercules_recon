#include "host_warm_delivery.h"

/* Separate accessed prefixes, all originally loader-zero cells. These provide
 * working source storage, not original aggregate/TU or placement claims. */
extern "C" {
HostInputPacketPrefix *g_host_input_packet_68e78c = 0;
EngineNoArgCallback g_host_secondary_callback_68e90c = 0;
DWORD g_host_secondary_delivery_68e910 = 0;
DWORD g_host_audio_channel_ids_799848[24] = {0};
DWORD g_host_audio_channel_cursors_7998a8[24] = {0};
}
