/* Working observed cells only. Original aggregate/TU/layout identity is unproved.
 * Pointer definitions supply four-byte cells, not their heap/COM targets. */
#include <windows.h>
#include <ddraw.h>
#include <stddef.h>

typedef char sdk_int_4[(sizeof(int)==4)?1:-1];
typedef char sdk_long_4[(sizeof(LONG)==4)?1:-1];
typedef char sdk_dword_4[(sizeof(DWORD)==4)?1:-1];
typedef char sdk_ptr_4[(sizeof(void *)==4)?1:-1];
typedef char sdk_byte_1[(sizeof(BYTE)==1)?1:-1];
typedef char sdk_dc_4[(sizeof(HDC)==4)?1:-1];
typedef char sdk_palette_4[(sizeof(HPALETTE)==4)?1:-1];
typedef char sdk_bitmap_4[(sizeof(HBITMAP)==4)?1:-1];
typedef char sdk_menu_4[(sizeof(HMENU)==4)?1:-1];
typedef char sdk_placement_44[(sizeof(WINDOWPLACEMENT)==44)?1:-1];
typedef char sdk_placement_show_8[(offsetof(WINDOWPLACEMENT,showCmd)==8)?1:-1];
typedef char sdk_rect_16[(sizeof(RECT)==16)?1:-1];

extern "C" {
/* Native initialized writable byte, not a loader-zero flag. */
BYTE g_exe_43a590 = 1;

/* Signed state/dirty views match existing consumers and actual dword accesses. */
int g_exe_91c44c = 0;
int g_exe_91c46c = 0;
LONG g_exe_91c480 = 0;
LONG g_exe_92c48c = 0;
WINDOWPLACEMENT g_exe_92e390 = {0};
RECT g_exe_92e3c0 = {0};

BYTE g_exe_96e3d0 = 0;
int g_exe_96e3d4 = 0;
BYTE *g_exe_96e3d8 = 0;
HPALETTE g_exe_96e3dc = 0;
IDirectDraw *g_exe_96e3e4 = 0;
IDirectDrawSurface *g_exe_96e3e8 = 0;
IDirectDrawSurface *g_exe_96e3ec = 0;
IDirectDrawPalette *g_exe_96e3f0 = 0;
BITMAPINFO *g_exe_96e3f4 = 0;
LOGPALETTE *g_exe_96e3f8 = 0;
HPALETTE g_exe_96e3fc = 0;
HDC g_exe_96e400 = 0;
HBITMAP g_exe_96e404 = 0;
int g_exe_96e40c = 0;
BYTE g_exe_96e410 = 0;
HMENU g_exe_96e414 = 0;

BYTE *g_exe_ba0210 = 0;
DWORD g_exe_ba0214 = 0;
int g_exe_ba0220 = 0;
int g_exe_ba0224 = 0;
}
