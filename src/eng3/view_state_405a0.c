/* Independently anchored ENG3 view prefix at 405a0, through the native
 * nine-DWORD copy at 201c7. No original object-end or TU identity claim.
 * Shared sprite Y reads use this owner at +4; do not also define g_70c64. */
typedef struct Eng3View {
    long x, y, z;
    long pitch, yaw;
    long unknown_14, unknown_18, unknown_1c;
    long column;
} Eng3View;
Eng3View g_eng3_view_405a0;
