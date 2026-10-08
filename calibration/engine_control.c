/* Ordinary-C semantic experiment for the observed ENG1 control handler.
 * The routine/prototype names are experiment labels, not recovered source names. */
#include "engine_interface.h"
#include "resource_record.h"

extern ActorResource g_actor_resources[];
extern short g_control_threshold;
extern unsigned long g_control_forwarded_value;
extern unsigned long g_control_global_71ec0;
extern unsigned char g_control_argument_6eef8[];
extern unsigned char g_control_argument_6eefc[];
extern unsigned char g_control_argument_6ef00[];

/* Real internal engine wrappers/helpers are extern dependencies, not stubs. */
extern void __cdecl resource_destroy(int resource_id);
extern void __cdecl host_cancel(void);
extern void __cdecl resource_reclaim(int wait_mode);
extern void __cdecl host_two_argument(void *first, unsigned long second);
extern void __cdecl host_free_allocation(void *allocation);
extern void __cdecl resource_cleanup(void);
extern void __cdecl resource_helper_646c(void);
extern void __cdecl host_worker(void);
extern void __cdecl host_two_argument_update(void *first, unsigned long second);
extern EngineNoArgCallback __cdecl host_register_callback_a(EngineNoArgCallback callback);
extern EngineNoArgCallback __cdecl host_register_callback_b(EngineNoArgCallback callback);
extern void __cdecl host_reset_resource(unsigned long value);
extern void __cdecl host_clean(unsigned long value);
extern void __cdecl engine_init(void);
extern void __cdecl engine_shutdown(void);
extern void __cdecl host_poll(void);
extern void __cdecl resource_test(void);
extern void __cdecl host_busy_callback(void);
extern void __cdecl host_zero_argument(void);

int __cdecl runtime_control(void)
{
    unsigned long selector;
    int resource_id;

    selector = (unsigned long)g_engine_interface.context_004->unknown_000[3];
    switch (selector) {
    case 0:
        return 0;

    case 1:
    case 2:
        if (g_engine_interface.context_004->suppress_auto_close_0e0 != 0 ||
            g_engine_interface.context_004->unknown_0e4 != 0) {
            g_engine_interface.context_004->unknown_000[3] = 0;
            return 0;
        }

        resource_id = 3;
        resource_destroy(resource_id);
        g_actor_resources[resource_id].residency_state = 6;

        resource_id = 1;
        resource_destroy(resource_id);
        g_actor_resources[resource_id].residency_state = 6;

        if (g_engine_interface.context_004->mode_09c == 1)
            host_cancel();
        if (g_engine_interface.context_004->load_failed != 0)
            g_engine_interface.context_004->load_failed(0);
        resource_reclaim(1);
        host_two_argument(g_control_argument_6eef8, g_control_forwarded_value);

        g_engine_interface.context_004->unknown_000[5] = 1;
        g_engine_interface.context_004->blocked_0df = 1;
        g_engine_interface.context_004->unknown_000[3] = 0;
        g_engine_interface.context_004->unknown_000[3] = 0;
        return 0;

    case 5:
        if (g_control_threshold >= 128) {
            g_engine_interface.context_004->unknown_000[5] = 0;
            g_engine_interface.context_004->unknown_000[3] = 0;
        }
        return 0;

    case 18:
    case 19:
        if (g_engine_interface.context_004->mode_09c == 1)
            host_cancel();
        if (g_engine_interface.context_004->load_failed != 0)
            g_engine_interface.context_004->load_failed(0);

        if (g_engine_interface.context_004->unknown_0e4 != 0)
            host_free_allocation(g_engine_interface.context_004->unknown_0e4);

        resource_cleanup();
        resource_helper_646c();
        host_worker();
        g_engine_interface.context_004->unknown_000[0] = 0xff;
        host_two_argument_update(g_control_argument_6eefc,
                                 g_control_forwarded_value);
        /* PC code falls directly into case 33 without redispatch. */

    case 33:
        if (g_engine_interface.context_004->mode_09c == 1)
            host_cancel();
        if (g_engine_interface.context_004->load_failed != 0)
            g_engine_interface.context_004->load_failed(0);

        g_engine_interface.context_004->unknown_128 = 0;
        g_control_global_71ec0 = 0;
        host_register_callback_a(0);
        host_register_callback_b(0);
        host_reset_resource(0);
        host_clean(0);
        engine_init();
        engine_shutdown();
        return 33;

    default:
        if (g_engine_interface.context_004->unknown_0e4 != 0) {
            do {
                host_poll();
                host_clean(0);
                resource_test();
                host_busy_callback();
            } while (g_engine_interface.context_004->suppress_auto_close_0e0 != 0 &&
                     g_engine_interface.context_004->unknown_0e4 != 0);
        }

        if (g_engine_interface.context_004->unknown_000[6] == 3)
            g_engine_interface.context_004->unknown_000[3] = 19;

        resource_cleanup();
        host_worker();
        host_clean(0);
        g_engine_interface.context_004->unknown_12c = 0;

        if (g_engine_interface.context_004->unknown_0e4 != 0)
            host_zero_argument();
        else
            host_two_argument(g_control_argument_6ef00,
                              g_control_forwarded_value);

        while (1)
            host_busy_callback();
        return 0;
    }
}
