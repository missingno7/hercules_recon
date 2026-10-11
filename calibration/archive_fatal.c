#include <stdlib.h>

typedef void *FatalWindow;
typedef unsigned int FatalUINT;
typedef unsigned int FatalWPARAM;
typedef long FatalLPARAM;

extern unsigned char g_fatal_entered;
extern unsigned char g_fatal_suppress_dialog;
extern FatalWindow g_fatal_owner_window;
extern void __cdecl fatal_shutdown_chain(void);
__declspec(dllimport) int __stdcall MessageBoxA(FatalWindow, const char *, const char *, FatalUINT);
__declspec(dllimport) int __stdcall SendNotifyMessageA(FatalWindow, FatalUINT, FatalWPARAM, FatalLPARAM);

void __cdecl archive_fatal(const char *message)
{
    if (g_fatal_entered)
        exit(-321);
    g_fatal_entered = 1;
    fatal_shutdown_chain();
    if (!g_fatal_suppress_dialog)
        MessageBoxA(g_fatal_owner_window, message, "Hercules Error", 0x10);
    if (g_fatal_owner_window != 0)
        SendNotifyMessageA(g_fatal_owner_window, 0x10, 0, 0);
    exit(-123);
}
