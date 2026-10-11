#include <stdlib.h>

typedef void *CleanupHandle;
typedef struct CleanupObject CleanupObject;
typedef unsigned long (__stdcall *CleanupRelease)(CleanupObject *object);
typedef struct CleanupVtable {
    void *query_interface;
    void *add_ref;
    CleanupRelease release;
} CleanupVtable;
struct CleanupObject {
    CleanupVtable *vtable;
};

#define RELEASE_COM(slot) \
    do { \
        if ((slot) != 0) { \
            (slot)->vtable->release(slot); \
            (slot) = 0; \
        } \
    } while (0)

extern void __cdecl cleanup_native_6ec0(int value);
extern void __cdecl cleanup_native_6bb0(void);
extern void __cdecl cleanup_native_9e10(void);
extern void __cdecl cleanup_native_ff40(int value);
extern CleanupHandle g_fatal_owner_window;

extern CleanupObject *g_exe_96e3ec;
extern CleanupObject *g_exe_96e3e8;
extern CleanupObject *g_exe_96e3f0;
extern CleanupObject *g_exe_96e3e4;
extern CleanupHandle g_exe_96e3dc;
extern CleanupHandle g_exe_96e3fc;
extern CleanupHandle g_exe_96e400;
extern void *g_exe_96e3f8;
extern void *g_exe_96e3f4;
extern CleanupHandle g_exe_96e404;
extern int g_exe_96e630;
extern CleanupObject *g_exe_79e444;
extern CleanupObject *g_exe_79e440;

__declspec(dllimport) CleanupHandle __stdcall SelectPalette(CleanupHandle dc, CleanupHandle palette, int background);
__declspec(dllimport) int __stdcall DeleteObject(CleanupHandle object);
__declspec(dllimport) int __stdcall ReleaseDC(CleanupHandle window, CleanupHandle dc);
__declspec(dllimport) int __stdcall InvalidateRect(CleanupHandle window, const void *rect, int erase);
__declspec(dllimport) unsigned int __stdcall mciSendStringA(const char *command, char *return_buffer, unsigned int return_length, unsigned long callback);

void __cdecl cleanup_native_74c0(void)
{
    cleanup_native_6ec0(0);
    cleanup_native_6bb0();

    RELEASE_COM(g_exe_96e3ec);
    RELEASE_COM(g_exe_96e3e8);
    RELEASE_COM(g_exe_96e3f0);
    RELEASE_COM(g_exe_96e3e4);

    if (g_exe_96e3fc != 0 && g_exe_96e400 != 0) {
        SelectPalette(g_exe_96e400, g_exe_96e3dc, 0);
        DeleteObject(g_exe_96e3fc);
        g_exe_96e3fc = 0;
    }

    if (g_exe_96e3f8 != 0) {
        free(g_exe_96e3f8);
        g_exe_96e3f8 = 0;
    }
    if (g_exe_96e3f4 != 0) {
        free(g_exe_96e3f4);
        g_exe_96e3f4 = 0;
    }
    if (g_exe_96e400 != 0) {
        ReleaseDC(g_fatal_owner_window, g_exe_96e400);
        g_exe_96e400 = 0;
    }
    if (g_exe_96e404 != 0) {
        DeleteObject(g_exe_96e404);
        g_exe_96e404 = 0;
    }

    InvalidateRect(0, 0, 1);
}

void __cdecl cleanup_native_a120(void)
{
    cleanup_native_9e10();
    if (g_exe_96e630 != 0)
        mciSendStringA("close cdaudio wait", 0, 0, 0);
}

void __cdecl cleanup_native_ff70(void)
{
    cleanup_native_ff40(1);
    RELEASE_COM(g_exe_79e444);
    RELEASE_COM(g_exe_79e440);
}
