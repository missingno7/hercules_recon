/* Provisional shared object view. TITLE 0x9770, ENG1 0x283a0, ENG3 0x1db80.
 * The pointer slot is checked; its pointee is deliberately not null-checked.
 * ~0x7f preserves the original unsigned-short promotion and two AND steps. */
typedef struct CommandOwner {
    unsigned char unknown_000[0x50];
    unsigned long flags_050;
    unsigned char unknown_054[0xd4];
    unsigned short **command_128;
} CommandOwner;

void clear_command_high_bit(CommandOwner *p)
{
    if (!(p->flags_050 & 0x10000000)) {
        if (p->command_128) {
            unsigned short *command = *p->command_128;
            unsigned short code = *command;
            if ((code & ~0x7f) == 0xb780)
                *command = code & 0x7fff;
        }
    }
}
