/* Exact eight-byte data contribution observed at ENG1 RVA 0x4d6ec. */
typedef unsigned char eng1_u8;

eng1_u8 g_actor_state_map_004d6ec[8] = {
    0xff, 0x00, 0x01, 0x02,
    0xff, 0x05, 0x06, 0x07
};

typedef char actor_state_map_width_is_eight[
    (sizeof(g_actor_state_map_004d6ec) == 8) ? 1 : -1];

/* Measured separately addressed loader-zero WORD at72168. */
unsigned short g_native_word_72168;

/* Selector bytes read by the spatial callback family at51fd4. */
unsigned char eng1_native_data_051fd4[4] = {0xff, 0x01, 0x02, 0x03};
