/* Initialized data prefix observed at ENG1 RVA 0x4dce8.
 * The next measured owner boundary is 0x4dcf8. The eight WORDs below match
 * that 16-byte prefix exactly; the logical table capacity is unproved. */
unsigned short eng1_native_words_04dce8[8] = {
    0x00d0U, 0x0000U, 0x00d3U, 0x000fU,
    0x0010U, 0x0000U, 0x0000U, 0x0000U
};

/* Sound WORD view [66d68,66d78); stops before the separately owned
 * command stream at66d78. This is a measured prefix, not full capacity. */
unsigned short g_eng1_sound_words_66d68[8] = {
    0x0071U, 0x0000U, 0x0000U, 0x0043U,
    0x0044U, 0x0000U, 0x0000U, 0x0000U
};
