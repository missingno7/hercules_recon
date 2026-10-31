#include <string.h>
#include "host_setup_storage.h"

extern int __cdecl identity_value(int);
/* Callsite-only, non-prototype views: accepted leaf definitions unchanged. */
extern int __cdecl host_zero_callback();
extern void __cdecl empty_host_callback();
extern int __cdecl host_return_one(void);
extern unsigned char g_host_setup_input_buffer0_7a2a00[];
extern unsigned char g_host_setup_input_buffer1_7a2a40[];
/* Native4180 supplies four arguments; existing packet owner consumes only the
 * first. Original common source prototype and buffer capacities are open. */
extern unsigned long __cdecl host_register_input_packet_bd30();
extern int __cdecl virtual_file_close(int);

void __cdecl host_input_packets_initialize_4180(void)
{
    (void)host_register_input_packet_bd30(g_host_setup_input_buffer1_7a2a40,
        0x22UL, g_host_setup_input_buffer0_7a2a00, 0x22UL);
}

void __cdecl host_resource_reset_10760(unsigned long ignored)
{
    g_host_engine_interface.context_004->mode_09c = 0;
    g_host_engine_interface.context_004->blocked_0df = 0;
    g_host_engine_interface.context_004->transfer_status_0dc = 0;
    memcpy(g_host_engine_interface.context_004->unknown_066 + 0x16,
           g_host_engine_interface.context_004->unknown_066 + 6, 16);
    g_host_engine_interface.context_004->unknown_000[15] =
        g_host_engine_interface.context_004->unknown_000[14];
}

int __cdecl host_setup_slots_reset_a1a0(void)
{
    unsigned long index;
    if (g_host_setup_slots_first_3744c != 0) {
        g_host_setup_slots_first_3744c = 0;
        g_public_file_count = 0;
        memset(g_public_file_handles, 0, sizeof(g_public_file_handles));
    }
    for (index = 0; index < 16; ++index) {
        if (g_public_file_handles[index] != 0)
            (void)virtual_file_close((int)index + 1);
    }
    g_public_file_count = 0;
    return 0;
}

int __cdecl host_frame_notify_zero(const void *ignored0, unsigned long ignored1,
                                 unsigned long ignored2, unsigned long ignored3)
{
    host_invoke_callback_zero();
    return 0;
}

void __cdecl host_service_10690(void)
{
    unsigned char local[32] = {0};
    initialize_pool(g_host_pool.saved_arena, g_host_pool.saved_bytes, 0);
    (void)identity_value(g_host_engine_interface.context_004->unknown_000[9]);
    (void)host_zero_callback(0);
    empty_host_callback(0);
    (void)host_enable_clear_callbacks();
    (void)host_zero_callback(0);
    host_input_packets_initialize_4180();
    (void)host_return_one();
    memcpy(g_host_engine_interface.context_004->unknown_066 + 6,
           g_host_setup_prefix_3a630.context_defaults_010, 16);
    host_resource_reset_10760(0);
    (void)host_frame_notify_zero(&g_host_setup_prefix_3a630, 0, 0, 0);
    (void)host_setup_slots_reset_a1a0();
    (void)host_zero_callback();
}
