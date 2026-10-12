/* Private ordinary-C model of the observed ENG1 command-template table.
 * Backing records stay owned by the existing EXE pool allocator; ENG1 keeps
 * only the measured twenty pointer slots and never frees/rolls them back here.
 */
#include "../../../calibration/engine_interface.h"

#define ENG1_COMMAND_SLOT_COUNT 20

extern unsigned short *g_command_slot_storage_71320[ENG1_COMMAND_SLOT_COUNT];

extern void *__cdecl engine_pool_allocate(void **output_slot,
                                           unsigned long bytes,
                                           unsigned long flags);

/* ENG1 0x282e0 clears exactly 0x14 pointer DWORDs at 0x71320. */
void __cdecl reset_command_table(void)
{
    unsigned long index;
    for (index = 0; index < ENG1_COMMAND_SLOT_COUNT; ++index)
        g_command_slot_storage_71320[index] = 0;
}

/* ENG1 0x28300. Each successful callback fills the first free output slot;
 * the returned record receives only the low WORD of opcode. Failure leaves
 * prior successful allocations and slot pointers intact, as native does.
 */
int __cdecl initialize_command_table(unsigned long bytes,
                                     unsigned int opcode,
                                     unsigned long count)
{
    unsigned long index;
    unsigned long remaining;
    unsigned short *command;

    remaining = count;
    for (;;) {
        for (index = 0; index < ENG1_COMMAND_SLOT_COUNT; ++index) {
            if (!g_command_slot_storage_71320[index])
                break;
        }
        if (index == ENG1_COMMAND_SLOT_COUNT)
            return 0;

        command = (unsigned short *)engine_pool_allocate(
            (void **)&g_command_slot_storage_71320[index], bytes, 0);
        if (!command)
            return 0;

        *command = (unsigned short)opcode;
        if (--remaining == 0)
            return 1;
    }
}

/* ENG1 0x28360. Compare the pointed WORD zero-extended against the full
 * caller DWORD and return the address of the matching pointer-table slot.
 */
unsigned short **__cdecl allocate_command(unsigned int opcode)
{
    unsigned long index;

    for (index = 0; index < ENG1_COMMAND_SLOT_COUNT; ++index) {
        if (g_command_slot_storage_71320[index] &&
            (unsigned int)g_command_slot_storage_71320[index][0] == opcode)
            return &g_command_slot_storage_71320[index];
    }
    return 0;
}
