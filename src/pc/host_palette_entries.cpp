#include <windows.h>
#include <stddef.h>
typedef char palette_offsets[(offsetof(LOGPALETTE,palPalEntry)==4 && sizeof(PALETTEENTRY)==4 && offsetof(BITMAPINFO,bmiColors)==40 && sizeof(RGBQUAD)==4)?1:-1];
extern "C" {
extern BITMAPINFO *g_exe_96e3f4;
extern LOGPALETTE *g_exe_96e3f8;
void __cdecl graphics_native_06780(short index,BYTE red,BYTE green,BYTE blue)
{
    if (index==0 || index==255)
        red=green=blue=(BYTE)index;
    g_exe_96e3f4->bmiColors[index].rgbRed=red;
    g_exe_96e3f8->palPalEntry[index].peRed=red;
    g_exe_96e3f4->bmiColors[index].rgbGreen=green;
    g_exe_96e3f8->palPalEntry[index].peGreen=green;
    g_exe_96e3f4->bmiColors[index].rgbBlue=blue;
    g_exe_96e3f8->palPalEntry[index].peBlue=blue;
    g_exe_96e3f8->palPalEntry[index].peFlags=4;
    g_exe_96e3f4->bmiColors[index].rgbReserved=0;
}
void __cdecl host_palette_get_rgb_6810(short index,BYTE *red,BYTE *green,BYTE *blue)
{
    *red=g_exe_96e3f8->palPalEntry[index].peRed;
    *green=g_exe_96e3f8->palPalEntry[index].peGreen;
    *blue=g_exe_96e3f8->palPalEntry[index].peBlue;
}
}
