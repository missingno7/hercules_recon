/* ENG3 measured initialized prefix. Signed opcode/operand pairs through the first restart/return command, as read by the signed-pair interpreter.
 * Complete compiler storage is compared with a unique native file payload.
 * No native allocation end, capacity or source-type identity is claimed. */
typedef struct Eng3CommandPair { short opcode; short operand; } Eng3CommandPair;
Eng3CommandPair g_eng3_image_37e48[3] = {
    { -2, 7 },
    { 20, 1 },
    { -1, 0 }
};
