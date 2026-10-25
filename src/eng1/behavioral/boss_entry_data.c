/* ENG1 5a450: 35 initialized Entry records followed by the full -1 sentinel.
 * Preserve the existing shared Entry declaration and its opaque 14-byte prefix;
 * the three physical DWORDs contain signed integral 16.16 values. Their wider
 * gameplay meaning and the WORD at +0xc are not inferred here.
 */
#include "../../../calibration/eng1/actor_region.h"

typedef char boss_entry_size16[sizeof(Entry) == 16 ? 1 : -1];
typedef char boss_entry_long4[sizeof(long) == 4 ? 1 : -1];

/* Explicit little-endian data initializers retain the frozen shared layout.
 * These are data records, never instructions or byte-array functions. */
#define WORD_BYTES(v) (unsigned char)(v), (unsigned char)((unsigned short)(v) >> 8)
#define DWORD_BYTES(v) WORD_BYTES(v), WORD_BYTES((unsigned long)(v) >> 16)
#define ENTRY16(a,b,c,w,t) {{DWORD_BYTES(a), DWORD_BYTES(b), DWORD_BYTES(c), WORD_BYTES(w)}, t}
#define POINT(a,b,c,w) ENTRY16((a)*65536L, (b)*65536L, (c)*65536L, w, 0x8000U)

Entry g_entries_5a450[36] = {
    POINT(8, -11, -1, 0),
    POINT(9, -12, 5, 0),
    POINT(10, 13, -13, 0),
    POINT(11, 14, 8, 0),
    POINT(12, -15, -1, 0),
    POINT(13, -16, 16, 0),
    POINT(14, 8, -9, 0),
    POINT(15, 9, 7, 0),
    POINT(16, -10, -2, 0),
    POINT(-8, -11, 15, 0),
    POINT(-9, 12, -8, 0),
    POINT(-10, -13, 1, 0),
    POINT(-11, -14, -4, 0),
    POINT(-12, 15, 6, 0),
    POINT(-13, -16, -14, 0),
    POINT(-14, -8, 1, 0),
    POINT(-15, 9, -9, 0),
    POINT(-16, 10, 4, 0),
    POINT(0, 16, 0, 2),
    POINT(0, 0, 16, 3),
    POINT(-16, 0, 0, 4),
    POINT(0, -16, 0, 5),
    POINT(0, 0, -16, 6),
    POINT(16, 16, 0, 7),
    POINT(0, 16, 16, 8),
    POINT(16, 0, 16, 9),
    POINT(-32, 0, 0, 10),
    POINT(0, -32, 0, 11),
    POINT(0, 0, -32, 12),
    POINT(32, 0, 0, 13),
    POINT(0, 32, 0, 14),
    POINT(0, 0, 32, 15),
    POINT(-32, -32, 0, 16),
    POINT(0, -32, -32, 17),
    POINT(-32, 0, -32, 18),
    ENTRY16(-1L, -1L, -1L, 0xffffU, 0xffffU)
};

#undef POINT
#undef ENTRY16
#undef DWORD_BYTES
#undef WORD_BYTES
