#include <windows.h>
#include <stdlib.h>

extern DWORD g_exe_4a3810;
extern DWORD g_exe_4a3814;
extern BYTE g_exe_48f810[81920];
extern DWORD g_exe_47f750[47];

void __cdecl movie_native_020c0(DWORD width, DWORD height)
{
    int i;

    g_exe_4a3810 = width;
    g_exe_4a3814 = height;
    ZeroMemory(g_exe_48f810, sizeof(g_exe_48f810));

    for (i = 0; i < 47; ++i) {
        g_exe_47f750[i] = (rand() % 5 - 2) * g_exe_4a3810 +
                          (rand() % 5 - 2);
    }
}
