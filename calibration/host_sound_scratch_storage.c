#include "host_sound_bank.h"

/* Measured accessed prefix: native base, end-byte witness and PC asset domain.
 * Original total allocation, source element type, TU and placement are unproved. */
short g_host_sound_scratch_699848[524288] = {0};
typedef char sound_scratch_short_is_two[(sizeof(short) == 2) ? 1 : -1];
typedef char sound_scratch_prefix_is_measured[
    (sizeof(g_host_sound_scratch_699848) == 1048576) ? 1 : -1];
