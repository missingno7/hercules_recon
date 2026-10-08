/* PC-derived close-state and channel-zero adapters; names/prototypes are hypotheses. */
#include "resource_context.h"
#include "host_callback_protocol_externs.h"

extern ResourceCallbackContext *g_resource_context;
extern const char g_resource_close_failure_format[];
extern const char g_resource_close_success_format[];
extern void __cdecl host_diagnostic(const char *format, ...);
extern int __cdecl virtual_file_close(int handle);

int __cdecl host_frame_close_state(void)
{
    ResourceCallbackContext *context;
    int close_result;
    int handle;

    context = g_resource_context;
    handle = context->virtual_handle_0b8;
    close_result = virtual_file_close(handle);
    if (close_result != 0) {
        host_diagnostic(g_resource_close_failure_format);
        return 0;
    }

    context = g_resource_context;
    context->unknown_0ac = 0;
    context = g_resource_context;
    context->mode_09c = 0;
    host_diagnostic(g_resource_close_success_format);
    return 1;
}

int __cdecl host_frame_notify_zero(void)
{
    host_invoke_callback_zero();
    return 0;
}
