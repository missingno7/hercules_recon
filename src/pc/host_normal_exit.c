#include <stdlib.h>
#include <windows.h>

/* Physical PC identities shared with the existing fatal/shutdown providers. */
extern unsigned char g_fatal_entered;
extern HWND g_fatal_owner_window;
extern void __cdecl fatal_shutdown_chain(void);

void __cdecl host_normal_exit_104d0(void)
{
    if (g_fatal_entered)
        exit(-321);
    g_fatal_entered = 1;
    fatal_shutdown_chain();
    if (g_fatal_owner_window != 0)
        SendNotifyMessageA(g_fatal_owner_window, WM_CLOSE, 0, 0);
    exit(0);
}
