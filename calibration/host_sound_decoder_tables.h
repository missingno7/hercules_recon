#ifndef HERCULES_HOST_SOUND_DECODER_TABLES_H
#define HERCULES_HOST_SOUND_DECODER_TABLES_H
#ifdef __cplusplus
extern "C" {
#endif
extern const int g_host_sound_index_deltas_35e08[16];
extern const int g_host_sound_steps_35e48[89];
void __cdecl host_sound_decode_2410(const signed char *packed, short *samples,
    int byte_count, int *state);
#ifdef __cplusplus
}
#endif
#endif
