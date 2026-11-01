/* ENG3 measured initialized prefix. Seven signed WORD transform fields read at stride 14 by native 01890; prefix includes both observed sentinel command records. The interpreter may advance beyond this prefix.
 * Complete compiler storage is compared with a unique native file payload.
 * No native allocation end, capacity or source-type identity is claimed. */
typedef struct AnimationRecord {
    short x, y, z;
    short rotation_y, rotation_z, rotation_x;
    short duration;
} AnimationRecord;
AnimationRecord g_eng3_script_381d8[4] = {
    { 2, 0, 0, 0, 0, 0, 64 },
    { 4, 0, 0, 0, 0, 0, 128 },
    { 32767, -25, 0, 0, 0, 0, 0 },
    { 32767, -6, 0, 0, 0, 0, 0 }
};
