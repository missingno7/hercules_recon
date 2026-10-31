/*
 * ENG3 frame program at .data 0x37ca0: fourteen signed (opcode, operand) word pairs ending
 * with the -1 restart record. Actors run it through ->image (handlers_16240.cpp declares
 * it extern unsigned char[]). The record before it is the -1 sentinel of the preceding
 * program, and the next referenced program starts at 0x37cd8. Original sites 0x16295,
 * 0x18b30 and 0x18bce store this address (mov imm32).
 *
 * file-data-owner-v1 owner (docs/file_data_owner_2026_10_31.md): this lone object is
 * its section's complete storage, and those 56 bytes are compared byte for byte with
 * the original file bytes at 0x37ca0. No size, end, capacity or type of the original
 * object is proved, and runtime writes are not ruled out. Keep it the only object here.
 */
typedef struct Eng3CommandPair { short opcode; short operand; } Eng3CommandPair;
Eng3CommandPair g_eng3_image_37ca0[14] = {
    { -11, 1 },
    { 50, 1 },
    { 51, 1 },
    { 52, 1 },
    { 53, 2 },
    { 54, 2 },
    { 55, 2 },
    { -11, 2 },
    { 56, 1 },
    { 57, 1 },
    { 58, 2 },
    { 59, 2 },
    { 60, 2 },
    { -1, 0 }
};
