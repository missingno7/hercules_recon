#include <windows.h>
#include <ddraw.h>
#include <stddef.h>
#include <string.h>
#include "host_color_cache.h"
typedef char fill_fx_layout[(sizeof(DDBLTFX)==100 && offsetof(DDBLTFX,dwFillColor)==80)?1:-1];
extern "C" {
extern LOGPALETTE *g_exe_96e3f8;
extern BYTE g_exe_ba01f0;
extern BYTE g_fatal_suppress_dialog;
extern BYTE *g_exe_96e3d8;
extern BYTE *g_exe_ba0210;
extern DWORD g_exe_ba0214;
extern IDirectDrawSurface *g_exe_96e3ec;
extern BYTE g_exe_43a590;
BYTE __cdecl graphics_native_06850(int red,int green,int blue)
{
    unsigned int distance,best,index;
    int difference;
    BYTE result;
    PALETTEENTRY *entry=g_exe_96e3f8->palPalEntry;
    for (index=0;index<256;++index,++entry) {
        difference=red-entry->peRed;
        distance=(green-entry->peGreen)*(green-entry->peGreen)+difference*difference;
        difference=blue-entry->peBlue;
        distance+=difference*difference;
        if (index==0 || distance<best) {
            best=distance;
            result=(BYTE)index;
            if (distance==0)
                break;
        }
    }
    return result;
}
void __cdecl graphics_native_06ee0(int red,int green,int blue)
{
    if (g_exe_ba01f0!=0)
        return;
    if (g_host_color_cache_56e418.red!=(BYTE)red ||
        g_host_color_cache_56e418.green!=(BYTE)green ||
        g_host_color_cache_56e418.blue!=(BYTE)blue) {
        g_host_color_cache_56e418.blue=(BYTE)blue;
        g_host_color_cache_56e418.green=(BYTE)green;
        g_host_color_cache_56e418.red=(BYTE)red;
        g_host_color_cache_56e418.index=graphics_native_06850((BYTE)red,(BYTE)green,(BYTE)blue);
    }
    if (g_fatal_suppress_dialog!=0) {
        DDBLTFX fx;
        memset(&fx,0,sizeof(fx));
        fx.dwSize=sizeof(fx);
        fx.dwFillColor=g_host_color_cache_56e418.index;
        if (g_exe_96e3ec!=0)
            g_exe_96e3ec->Blt(0,0,0,DDBLT_COLORFILL|DDBLT_WAIT,&fx);
    } else {
        g_exe_ba0210=g_exe_96e3d8;
        g_exe_ba0214=320;
        if (g_exe_96e3d8!=0)
            memset(g_exe_96e3d8,g_host_color_cache_56e418.index,320*240);
    }
    g_exe_43a590=1;
}
}
