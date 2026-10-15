/* Original PC decoder hypothesis; source/TU identity is unproved.
 * Preserve low-WORD zero extension on entry and signed-predictor OR on exit. */
#include "../../calibration/host_sound_decoder_tables.h"

void __cdecl host_sound_decode_2410(const signed char *packed, short *samples,
    int byte_count, int *state)
{
    int index = *state >> 16;
    int predictor = *state & 0xffff;
    int step = g_host_sound_steps_35e48[index];
    int half = 0;
    int cached, code, difference;
    int count = byte_count >> 1;
    while (count > 0) {
        if (half)
            code = cached;
        else {
            cached = *packed++;
            code = cached >> 4;
        }
        code &= 15;
        half = !half;
        index += g_host_sound_index_deltas_35e08[code];
        if (index < 0)
            index = 0;
        else if (index > 88)
            index = 88;
        difference = step >> 3;
        if (code & 4)
            difference += step;
        if (code & 2)
            difference += step >> 1;
        if (code & 1)
            difference += step >> 2;
        if (code & 8)
            predictor -= difference;
        else
            predictor += difference;
        if (predictor > 32767)
            predictor = 32767;
        else if (predictor < -32768)
            predictor = -32768;
        *samples++ = (short)predictor;
        step = g_host_sound_steps_35e48[index];
        --count;
    }
    *state = (index << 16) | predictor;
}
