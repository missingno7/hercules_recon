/* Observed fixed movie scratch prefix only. The native 0x5000-DWORD clear
 * supports these 81920 zero bytes; full original extent, ownership, aggregate,
 * translation unit and native placement remain unproved. No pointed object. */
#include <windows.h>

typedef char movie_byte_is_one[(sizeof(BYTE)==1)?1:-1];
BYTE g_movie_frame_scratch[81920] = {0};
