/* Initialized Entry tables consumed through the existing unsigned-char array
 * declarations. Each physical record has three DWORDs, WORD+c and tag WORD+e.
 * Values preserve the observed integral16.16 representation; later rendering
 * and the meanings of WORD+c/tag bits remain outside this data recovery.
 */
typedef char attachment_data_long4[sizeof(long) == 4 ? 1 : -1];
typedef char attachment_data_word2[sizeof(unsigned short) == 2 ? 1 : -1];

#define WORD_BYTES(v) (unsigned char)(v), (unsigned char)((unsigned short)(v) >> 8)
#define DWORD_BYTES(v) WORD_BYTES(v), WORD_BYTES((unsigned long)(v) >> 16)
#define ENTRY16(a,b,c,w,t) DWORD_BYTES(a), DWORD_BYTES(b), DWORD_BYTES(c), WORD_BYTES(w), WORD_BYTES(t)
#define POINT(a,b,c,w,t) ENTRY16((a)*65536L, (b)*65536L, (c)*65536L, w, t)
#define END_ENTRY ENTRY16(-1L, -1L, -1L, 0xffffU, 0xffffU)

unsigned char g_entries_64510[464] = {
    POINT(32, 0, 0, 1, 0),
    POINT(0, 96, 32, 2, 0),
    POINT(-32, 112, 0, 3, 0),
    POINT(0, 0, -32, 4, 0),
    POINT(32, 32, 0, 5, 0),
    POINT(0, 64, 32, 1, 0),
    POINT(32, 0, 32, 2, 0),
    POINT(-64, 0, 0, 3, 0),
    POINT(0, 112, -64, 4, 0),
    POINT(64, 112, 0, 5, 0),
    POINT(0, 96, 64, 1, 0),
    POINT(-64, -32, 0, 2, 0),
    POINT(0, -64, -64, 3, 0),
    POINT(-64, 0, -64, 4, 0),
    POINT(96, 0, 0, 5, 0),
    POINT(0, 96, 96, 1, 0),
    POINT(-96, 0, 0, 2, 0),
    POINT(0, 112, -96, 3, 0),
    POINT(96, 32, 0, 4, 0),
    POINT(0, 96, 96, 5, 0),
    POINT(96, 32, 96, 1, 0),
    POINT(-112, 64, 0, 2, 0),
    POINT(0, 0, -112, 3, 0),
    POINT(112, 112, 0, 4, 0),
    POINT(0, 0, 112, 5, 0),
    POINT(-112, -112, 0, 1, 0),
    POINT(0, -32, -112, 2, 0),
    POINT(-112, 96, -112, 3, 0),
    END_ENTRY
};

unsigned char g_entries_646e0[240] = {
    POINT(0, 96, 32, 2, 0),
    POINT(192, 0, -32, 4, 0),
    POINT(32, 32, 0, 5, 0),
    POINT(-64, 0, 0, 3, 0),
    POINT(0, 96, 64, 1, 0),
    POINT(0, -64, -192, 3, 0),
    POINT(0, 96, 96, 1, 0),
    POINT(0, 112, -96, 3, 0),
    POINT(96, 32, 0, 4, 0),
    POINT(96, 192, 96, 1, 0),
    POINT(-112, 64, 0, 2, 0),
    POINT(192, 112, 192, 4, 0),
    POINT(-112, -112, 0, 1, 0),
    POINT(-192, 96, -112, 3, 0),
    END_ENTRY
};

unsigned char g_entries_647d0[144] = {
    POINT(0, 96, 32, 2, 0),
    POINT(32, 32, 0, 5, 0),
    POINT(0, 96, 64, 1, 0),
    POINT(0, -64, -192, 3, 0),
    POINT(0, 112, -96, 3, 0),
    POINT(96, 192, 96, 1, 0),
    POINT(-112, 64, 0, 2, 0),
    POINT(-112, -112, 0, 1, 0),
    END_ENTRY
};

unsigned char g_level_entries_5e688[272] = {
    POINT(0, 0, 0, 5, 5),
    POINT(0, -16, 0, 6, 5),
    POINT(0, -32, 0, 7, 5),
    POINT(0, -48, 0, 8, 5),
    POINT(0, -64, 0, 9, 5),
    POINT(0, -80, 0, 10, 5),
    POINT(0, -96, 0, 11, 5),
    POINT(0, -112, 0, 12, 5),
    POINT(0, -128, 0, 13, 5),
    POINT(0, -144, 0, 14, 5),
    POINT(0, -160, 0, 15, 5),
    POINT(0, -176, 0, 16, 5),
    POINT(0, -192, 0, 17, 5),
    POINT(0, -208, 0, 18, 5),
    POINT(0, -224, 0, 19, 5),
    POINT(0, -240, 0, 20, 5),
    END_ENTRY
};

#undef END_ENTRY
#undef POINT
#undef ENTRY16
#undef DWORD_BYTES
#undef WORD_BYTES
