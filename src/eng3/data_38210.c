/* ENG3 measured initialized prefix. Seven signed WORD transform fields read at stride 14 by native 01890; prefix includes both observed sentinel command records. The interpreter may advance beyond this prefix.
 * Complete compiler storage is compared with a unique native file payload.
 * No native allocation end, capacity or source-type identity is claimed. */
typedef struct AnimationRecord {
    short x, y, z;
    short rotation_y, rotation_z, rotation_x;
    short duration;
} AnimationRecord;
AnimationRecord g_eng3_script_38210[12] = {
    { 0, 0, 0, 0, 0, 0, 2 },
    { -2, 0, 0, 0, 0, 0, 31 },
    { -3, 0, 0, -2, -4, 0, 31 },
    { -4, 0, 0, -4, -4, 0, 31 },
    { -6, 0, 0, 0, 0, 0, 460 },
    { -6, 1, 0, 0, 0, 0, 31 },
    { -6, 2, 0, 0, 8, 0, 63 },
    { -4, 0, 0, 0, 0, 0, 40 },
    { -4, 0, 0, 6, -8, 0, 31 },
    { -4, 0, 0, 0, 0, 0, 57 },
    { 32767, -25, 0, 0, 0, 0, 0 },
    { 32767, -6, 0, 0, 0, 0, 0 }
};
