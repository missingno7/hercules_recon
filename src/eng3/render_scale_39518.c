/*
 * ENG3 render scale vector at .data 0x39518: PSX VECTOR layout vx, vy, vz, pad with
 * 0x2000 on each axis. eng3_1ff60(matrix, g_eng3_scale_39518) scales the camera
 * matrices (0x24f50); original sites 0x20c19, 0x2134b, 0x24cbc and 0x24e3e push this
 * address, and the callers declare it extern long[].
 *
 * file-data-owner-v1 owner (docs/file_data_owner_2026_10_31.md): this lone object is
 * its section's complete storage, and those 16 bytes are compared byte for byte with
 * the original file bytes at 0x39518. No size, end, capacity or type of the original
 * object is proved, and runtime writes are not ruled out. Keep it the only object here.
 */
long g_eng3_scale_39518[4] = { 0x2000, 0x2000, 0x2000, 0 };
