/* Observed 24-row FX prefix. Opaque members carry no recovered semantics. */
#ifndef HERCULES_ENGINE_FX_RECORD_H
#define HERCULES_ENGINE_FX_RECORD_H
#include <stddef.h>

typedef struct FxSoundRecordView {
    unsigned long unknown_000[2];
    short effect_id_008;
    unsigned short unknown_00a;
    unsigned char unknown_00c[0x30 - 0x0c];
    unsigned long unknown_030;
    unsigned long unknown_034;
    unsigned long unknown_038;
    unsigned long active_03c;
} FxSoundRecordView;

typedef char fx_sound_record_is_0x40[(sizeof(FxSoundRecordView) == 0x40) ? 1 : -1];
typedef char fx_sound_id_is_at_0x08[(offsetof(FxSoundRecordView, effect_id_008) == 8) ? 1 : -1];
typedef char fx_sound_active_is_at_0x3c[(offsetof(FxSoundRecordView, active_03c) == 0x3c) ? 1 : -1];

extern FxSoundRecordView g_engine_fx_records[24];
#endif
