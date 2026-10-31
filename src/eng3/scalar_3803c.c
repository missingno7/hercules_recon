/*
 * ENG3 initialized long at .data 0x3803c, 0x140 at load. handlers_16240.cpp declares it
 * extern long, stores 0x140, 0x168 or a computed value, and compares the height cell
 * against it. The neighbouring cells 0x38038 and 0x38040 are referenced separately.
 * Original sites 0x16342, 0x1752b, 0x18acc, 0x18b00, 0x18b3e, 0x18ef9 and 0x19060 hold
 * this address in a direct DWORD operand.
 *
 * file-data-owner-v1 owner (docs/file_data_owner_2026_10_31.md): this lone object is
 * its section's complete storage, and those 4 bytes are compared byte for byte with
 * the original file bytes at 0x3803c. No size, end, capacity or type of the original
 * object is proved, and runtime writes are not ruled out. Keep it the only object here.
 */
long g_eng3_3803c = 0x140;
