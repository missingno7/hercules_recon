/* PC A1B0 initializes 16 words; A210 scans at most 16 slots and compares
 * its end pointer with RVA 0x56e680 (64 bytes after RVA 0x56e640).
 * This independent provider has no directory terminal-probe overlap.
 * Original TU, common/static declaration and whole-image layout unproved. */
int g_public_file_handles[16];
