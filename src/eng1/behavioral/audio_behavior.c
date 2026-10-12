/* Private gameplay-oriented ENG1 audio source unit.
 * Function/data views are address-backed; this file makes no historical TU/layout claim. */
#include "../../../calibration/engine_interface.h"
#include "../../../calibration/engine_fx_record.h"
#include "../../../calibration/resource_record.h"
#include "incomplete.h"

extern ActorResource g_actor_resources[81];

typedef struct FxActorView FxActorView;
struct FxActorView {
    unsigned char unknown_000[0x0c];
    short spatial_x_00c;
    short unknown_00e;
    short spatial_y_010;
    short unknown_012;
    unsigned char unknown_014[0x0e];
    unsigned char category_022;
    unsigned char mode_023;
    unsigned char unknown_024[0x44];
    unsigned long guard_068;
    unsigned long linked_06c;
};

typedef struct FxRowView FxRowView;
struct FxRowView {
    unsigned long unknown_000;
    unsigned long unknown_004;
    unsigned short id_008;
    unsigned short zero_00a;
    unsigned short bank_00c;
    unsigned short sample_00e;
    long pan_left_010;
    long pan_right_014;
    long volume_left_018;
    long volume_right_01c;
    long target_left_020;
    long target_right_024;
    unsigned char unknown_028[8];
    FxActorView *owner_030;
    unsigned long flags_034;
    unsigned long scale_038;
    long active_03c;
};

typedef char fx_actor_view_is_112[(sizeof(FxActorView) == 0x70) ? 1 : -1];
typedef char fx_row_view_is_64[(sizeof(FxRowView) == 0x40) ? 1 : -1];
typedef char fx_ulong_is_4[(sizeof(unsigned long) == 4) ? 1 : -1];
typedef char fx_pointer_is_4[(sizeof(void *) == 4) ? 1 : -1];

#define g_fx_rows_view ((FxRowView *)(void *)g_engine_fx_records)
extern void __cdecl dispatch_local_2c650(void);
extern void __cdecl dispatch_local_2c660(void);
extern int __cdecl count_active_fx_id(int effect_id);
extern short __cdecl dispatch_engine_2cb80(unsigned short bank, unsigned short sample,
                                            int zero_a, int frequency, int zero_b,
                                            short left, short right);

/* Native ENG1 zero-fill scratch cells, with minimum widths from reviewed loads/stores. */
long g_fx_x_721b8;
long g_fx_target_left_721bc;
long g_fx_target_right_721c0;
long g_fx_y_721c4;
long g_fx_focus_721c8;
long g_fx_square_left_721cc;
long g_fx_square_right_721d0;
unsigned long g_fx_effect_index_6ee88;
unsigned long g_fx_bank_value_6ee90;
unsigned long g_fx_sample_value_6ee94;
unsigned long g_fx_row_value_6eea0;
unsigned long g_fx_row_value_6eea4;
unsigned short g_fx_bank_word_6eeac;
unsigned short g_fx_sample_word_6eeae;
long g_fx_smooth_left_6eeb0;
long g_fx_smooth_right_6eeb4;
long g_fx_volume_left_6eeb8;
long g_fx_volume_right_6eebc;
FxActorView *g_fx_owner_6eed0;
unsigned long g_fx_full_effect_id_6eed4;
unsigned long g_fx_volume_scale_6eed8;
long g_fx_channel_6eee0;
unsigned long g_setup_restore_latch_6ee8c;
unsigned long g_setup_previous_mute_6ee98;

#define FX_CLAMP_640(value) do { \
    if ((value) < -640) (value) = -640; \
    else if ((value) > 640) (value) = 640; \
} while (0)

#define FX_CLAMP_127(value) do { \
    if ((value) < 0) (value) = 0; \
    if ((value) > 127) (value) = 127; \
} while (0)

/* Carry the native captured owner, flags, and distinct intermediate/output cells. */
#define FX_UPDATE_SPATIAL_STATE() do { \
    g_fx_x_721b8 = (long)g_fx_owner_6eed0->spatial_x_00c - 160; \
    if (g_fx_x_721b8 > -20 && g_fx_x_721b8 < 20) { \
        g_fx_target_left_721bc = 0; \
        g_fx_target_right_721c0 = 0; \
    } else { \
        if (g_fx_x_721b8 > -20) g_fx_target_left_721bc = g_fx_x_721b8 * 2 - 40; \
        else g_fx_target_left_721bc = -20 - g_fx_x_721b8; \
        if (g_fx_x_721b8 > 20) g_fx_target_right_721c0 = g_fx_x_721b8 - 20; \
        else g_fx_target_right_721c0 = (-20 - g_fx_x_721b8) * 2; \
    } \
    FX_CLAMP_640(g_fx_target_left_721bc); \
    FX_CLAMP_640(g_fx_target_right_721c0); \
    g_fx_target_left_721bc = 127 - g_fx_target_left_721bc / 5; \
    g_fx_target_right_721c0 = 127 - g_fx_target_right_721c0 / 5; \
    FX_CLAMP_127(g_fx_target_left_721bc); \
    FX_CLAMP_127(g_fx_target_right_721c0); \
    g_fx_y_721c4 = (long)g_fx_owner_6eed0->spatial_y_010 * 2 - 320; \
    if ((g_fx_full_effect_id_6eed4 & 0x1000) != 0) g_fx_y_721c4 <<= 1; \
    if ((g_fx_full_effect_id_6eed4 & 0x2000) != 0) g_fx_y_721c4 >>= 1; \
    g_fx_focus_721c8 = 0x800 - g_fx_y_721c4; \
    if (g_fx_focus_721c8 < 0) g_fx_focus_721c8 = 0; \
    if (g_fx_focus_721c8 > 0x800) g_fx_focus_721c8 = 0x800; \
    g_fx_target_left_721bc = (g_fx_target_left_721bc * g_fx_focus_721c8) >> 11; \
    g_fx_target_right_721c0 = (g_fx_target_right_721c0 * g_fx_focus_721c8) >> 11; \
    if ((g_fx_full_effect_id_6eed4 & 0x4000) != 0) { \
        g_fx_smooth_left_6eeb0 = g_fx_target_left_721bc; \
        g_fx_smooth_right_6eeb4 = g_fx_target_right_721c0; \
    } else { \
        g_fx_smooth_left_6eeb0 += (g_fx_target_left_721bc - g_fx_smooth_left_6eeb0) >> 1; \
        g_fx_smooth_right_6eeb4 += (g_fx_target_right_721c0 - g_fx_smooth_right_6eeb4) >> 1; \
    } \
    if (g_engine_interface.context_004->unknown_000[4] != 0) { \
        g_fx_smooth_left_6eeb0 = 0; \
        g_fx_smooth_right_6eeb4 = 0; \
    } \
    g_fx_square_left_721cc = g_fx_volume_scale_6eed8 * g_fx_smooth_left_6eeb0; \
    g_fx_square_right_721d0 = g_fx_volume_scale_6eed8 * g_fx_smooth_right_6eeb4; \
    g_fx_square_left_721cc *= g_fx_square_left_721cc; \
    g_fx_square_right_721d0 *= g_fx_square_right_721d0; \
    g_fx_volume_left_6eeb8 = g_fx_square_left_721cc / 0x4000 * 129 / 128; \
    g_fx_volume_right_6eebc = g_fx_square_right_721d0 / 0x4000 * 129 / 128; \
} while (0)

#define FX_SPECIAL_SOURCE_GUARD(source_value, done_label) do { \
    if (*(volatile unsigned short *)&g_actor_resources[(source_value)->category_022].residency_state == 3) goto done_label; \
    else goto done_label; \
} while (0)


#define FX_CALCULATE_PAN(source, effect_flags, left_state, right_state, scale_value, context_value) do { \
    g_fx_x_721b8 = (long)(source)->spatial_x_00c - 160; \
    if (g_fx_x_721b8 > -20 && g_fx_x_721b8 < 20) { \
        g_fx_target_left_721bc = 0; \
        g_fx_target_right_721c0 = 0; \
    } else { \
        if (g_fx_x_721b8 > -20) g_fx_target_left_721bc = g_fx_x_721b8 * 2 - 40; \
        else g_fx_target_left_721bc = -20 - g_fx_x_721b8; \
        if (g_fx_x_721b8 > 20) g_fx_target_right_721c0 = g_fx_x_721b8 - 20; \
        else g_fx_target_right_721c0 = (-20 - g_fx_x_721b8) * 2; \
    } \
    FX_CLAMP_640(g_fx_target_left_721bc); \
    FX_CLAMP_640(g_fx_target_right_721c0); \
    g_fx_target_left_721bc = 127 - g_fx_target_left_721bc / 5; \
    g_fx_target_right_721c0 = 127 - g_fx_target_right_721c0 / 5; \
    FX_CLAMP_127(g_fx_target_left_721bc); \
    FX_CLAMP_127(g_fx_target_right_721c0); \
    g_fx_y_721c4 = (long)(source)->spatial_y_010 * 2 - 320; \
    if ((effect_flags) & 0x1000) g_fx_y_721c4 <<= 1; \
    if ((effect_flags) & 0x2000) g_fx_y_721c4 >>= 1; \
    g_fx_focus_721c8 = 0x800 - g_fx_y_721c4; \
    if (g_fx_focus_721c8 < 0) g_fx_focus_721c8 = 0; \
    if (g_fx_focus_721c8 > 0x800) g_fx_focus_721c8 = 0x800; \
    g_fx_target_left_721bc = (g_fx_target_left_721bc * g_fx_focus_721c8) >> 11; \
    g_fx_target_right_721c0 = (g_fx_target_right_721c0 * g_fx_focus_721c8) >> 11; \
    if ((effect_flags) & 0x4000) { \
        (left_state) = g_fx_target_left_721bc; \
        (right_state) = g_fx_target_right_721c0; \
    } else { \
        (left_state) += (g_fx_target_left_721bc - (left_state)) >> 1; \
        (right_state) += (g_fx_target_right_721c0 - (right_state)) >> 1; \
    } \
    if ((context_value)->unknown_000[4] != 0) { \
        (left_state) = 0; \
        (right_state) = 0; \
    } \
    g_fx_square_left_721cc = (long)(scale_value) * (left_state); \
    g_fx_square_left_721cc *= g_fx_square_left_721cc; \
    g_fx_square_right_721d0 = (long)(scale_value) * (right_state); \
    g_fx_square_right_721d0 *= g_fx_square_right_721d0; \
    g_fx_volume_left_6eeb8 = g_fx_square_left_721cc / 0x4000 * 129 / 128; \
    g_fx_volume_right_6eebc = g_fx_square_right_721d0 / 0x4000 * 129 / 128; \
} while (0)
void __cdecl play_sound(unsigned short effect_id, FxActorView *actor)
{
    unsigned long default_volume;

    dispatch_local_2c650();
    g_fx_effect_index_6ee88 = effect_id & 0x07ff;
    g_fx_bank_value_6ee90 = g_engine_interface.context_004->sound_2f0[g_fx_effect_index_6ee88][0];
    g_fx_sample_value_6ee94 = g_engine_interface.context_004->sound_2f0[g_fx_effect_index_6ee88][1];
    if (g_fx_bank_value_6ee90 == 0xff || g_fx_sample_value_6ee94 == 0xff) goto done;
    if (actor == 0 && (effect_id & 0xf800) != 0) goto done;
    if ((effect_id & 0x4000) != 0 && actor != 0 && actor->guard_068 == 0) {
        FX_SPECIAL_SOURCE_GUARD(actor, done);
    }
    if (count_active_fx_id(g_fx_effect_index_6ee88) > 5) goto done;

    if ((effect_id & 0x4000) != 0) {
        g_fx_bank_word_6eeac = (unsigned short)g_fx_bank_value_6ee90;
        g_fx_sample_word_6eeae = (unsigned short)g_fx_sample_value_6ee94;
        g_fx_volume_scale_6eed8 = g_engine_interface.context_004->sound_2f0[g_fx_effect_index_6ee88][2];
        g_fx_owner_6eed0 = actor;
        g_fx_full_effect_id_6eed4 = effect_id;
        FX_UPDATE_SPATIAL_STATE();
        if (g_engine_interface.context_004->unknown_1b24 == 0) {
            default_volume = ((g_fx_smooth_left_6eeb0 + g_fx_smooth_right_6eeb4) *
                              g_engine_interface.context_004->unknown_1b18) >> 8;
            g_fx_channel_6eee0 = dispatch_engine_2cb80((unsigned short)g_fx_bank_value_6ee90,
                                                        (unsigned short)g_fx_sample_value_6ee94,
                                                        0, 0x3c, 0,
                                                        (short)default_volume,
                                                        (short)default_volume);
        } else {
            g_fx_channel_6eee0 = dispatch_engine_2cb80((unsigned short)g_fx_bank_value_6ee90,
                                                        (unsigned short)g_fx_sample_value_6ee94,
                                                        0, 0x3c, 0,
                                                        (short)((g_fx_smooth_left_6eeb0 *
                                                                 g_engine_interface.context_004->unknown_1b18) >> 7),
                                                        (short)((g_fx_smooth_right_6eeb4 *
                                                                 g_engine_interface.context_004->unknown_1b18) >> 7));
        }
        if (g_fx_channel_6eee0 >= 0) {
            g_fx_rows_view[g_fx_channel_6eee0].unknown_000 = g_fx_row_value_6eea0;
            g_fx_rows_view[g_fx_channel_6eee0].unknown_004 = g_fx_row_value_6eea4;
            g_fx_rows_view[g_fx_channel_6eee0].pan_left_010 = g_fx_smooth_left_6eeb0;
            g_fx_rows_view[g_fx_channel_6eee0].pan_right_014 = g_fx_smooth_right_6eeb4;
            g_fx_rows_view[g_fx_channel_6eee0].volume_left_018 = g_fx_volume_left_6eeb8;
            g_fx_rows_view[g_fx_channel_6eee0].volume_right_01c = g_fx_volume_right_6eebc;
        }
        effect_id &= (unsigned short)~0x4000;
    } else if ((effect_id & 0x0800) != 0) {
        g_fx_channel_6eee0 = dispatch_engine_2cb80((unsigned short)g_fx_bank_value_6ee90,
                                                    (unsigned short)g_fx_sample_value_6ee94,
                                                    0, 0x3c, 0, 0, 0);
        if (g_fx_channel_6eee0 >= 0) {
            g_fx_rows_view[g_fx_channel_6eee0].pan_left_010 = 0;
            g_fx_rows_view[g_fx_channel_6eee0].pan_right_014 = 0;
        }
    } else {
        default_volume = (g_engine_interface.context_004->unknown_1b18 * 0x7f) >> 7;
        g_fx_channel_6eee0 = dispatch_engine_2cb80((unsigned short)g_fx_bank_value_6ee90,
                                                    (unsigned short)g_fx_sample_value_6ee94,
                                                    0, 0x3c, 0,
                                                    (short)default_volume,
                                                    (short)default_volume);
        if (g_fx_channel_6eee0 >= 0) {
            g_fx_rows_view[g_fx_channel_6eee0].pan_left_010 = default_volume;
            g_fx_rows_view[g_fx_channel_6eee0].pan_right_014 = default_volume;
        }
    }

    if (g_fx_channel_6eee0 >= 0) {
        g_fx_rows_view[g_fx_channel_6eee0].target_left_020 =
            g_fx_rows_view[g_fx_channel_6eee0].pan_left_010;
        g_fx_rows_view[g_fx_channel_6eee0].target_right_024 =
            g_fx_rows_view[g_fx_channel_6eee0].pan_right_014;
        g_fx_rows_view[g_fx_channel_6eee0].bank_00c = (unsigned short)g_fx_bank_value_6ee90;
        g_fx_rows_view[g_fx_channel_6eee0].sample_00e = (unsigned short)g_fx_sample_value_6ee94;
        g_fx_rows_view[g_fx_channel_6eee0].owner_030 = actor;
        g_fx_rows_view[g_fx_channel_6eee0].id_008 = (unsigned short)g_fx_effect_index_6ee88;
        g_fx_rows_view[g_fx_channel_6eee0].flags_034 = effect_id;
        g_fx_rows_view[g_fx_channel_6eee0].zero_00a = 0;
        g_fx_rows_view[g_fx_channel_6eee0].scale_038 =
            g_engine_interface.context_004->sound_2f0[g_fx_effect_index_6ee88][2];
    }
    if (actor != 0) actor->linked_06c = 1;

done:
    dispatch_local_2c660();
}

void __cdecl play_actor_sound(unsigned short effect_id, FxActorView *owner,
                              FxActorView *source)
{
    ResourceCallbackContext *context;
    unsigned long effect_index;
    unsigned long bank;
    unsigned long sample;
    unsigned long volume_scale;
    long channel;
    long smooth_left;
    long smooth_right;
    long left_volume;
    long right_volume;
    long default_volume;

    dispatch_local_2c650();
    context = g_engine_interface.context_004;
    effect_index = effect_id & 0x07ff;
    bank = context->sound_2f0[effect_index][0];
    sample = context->sound_2f0[effect_index][1];
    if (bank == 0xff || sample == 0xff) goto done;
    if (owner == 0 && (effect_id & 0xf800) != 0) goto done;
    if (source == 0 && (effect_id & 0xf800) != 0) goto done;
    if ((effect_id & 0x4000) != 0 && source != 0 && source->guard_068 == 0) {
        FX_SPECIAL_SOURCE_GUARD(source, done);
    }
    if (count_active_fx_id(effect_index) > 5) goto done;

    if ((effect_id & 0x4000) != 0) {
        volume_scale = context->sound_2f0[effect_index][2];
        FX_CALCULATE_PAN(source, effect_id, smooth_left, smooth_right,
                         volume_scale, context);
        eng1_incomplete_behavior(0x75e3UL, "actor4000 opaque row words");
        return;
        if (context->unknown_1b24 == 0) {
            default_volume = ((smooth_left + smooth_right) * context->unknown_1b18) >> 8;
            channel = dispatch_engine_2cb80(bank, sample, 0, 0x3c, 0,
                                             (short)default_volume,
                                             (short)default_volume);
        } else {
            left_volume = (smooth_left * context->unknown_1b18) >> 7;
            right_volume = (smooth_right * context->unknown_1b18) >> 7;
            channel = dispatch_engine_2cb80(bank, sample, 0, 0x3c, 0,
                                             (short)left_volume,
                                             (short)right_volume);
        }
        if (channel >= 0) {
            g_fx_rows_view[channel].pan_left_010 = smooth_left;
            g_fx_rows_view[channel].pan_right_014 = smooth_right;
            g_fx_rows_view[channel].volume_left_018 = g_fx_volume_left_6eeb8;
            g_fx_rows_view[channel].volume_right_01c = g_fx_volume_right_6eebc;
        }
        effect_id &= (unsigned short)~0x4000;
    } else if ((effect_id & 0x0800) != 0) {
        channel = dispatch_engine_2cb80(bank, sample, 0, 0x3c, 0, 0, 0);
        if (channel >= 0) {
            g_fx_rows_view[channel].pan_left_010 = 0;
            g_fx_rows_view[channel].pan_right_014 = 0;
        }
    } else {
        default_volume = (context->unknown_1b18 * 0x7f) >> 7;
        channel = dispatch_engine_2cb80(bank, sample, 0, 0x3c, 0,
                                         (short)default_volume,
                                         (short)default_volume);
        if (channel >= 0) {
            g_fx_rows_view[channel].pan_left_010 = default_volume;
            g_fx_rows_view[channel].pan_right_014 = default_volume;
        }
    }

    if (channel >= 0) {
        g_fx_rows_view[channel].target_left_020 = g_fx_rows_view[channel].pan_left_010;
        g_fx_rows_view[channel].target_right_024 = g_fx_rows_view[channel].pan_right_014;
        g_fx_rows_view[channel].bank_00c = bank;
        g_fx_rows_view[channel].sample_00e = sample;
        g_fx_rows_view[channel].owner_030 = owner;
        g_fx_rows_view[channel].id_008 = (unsigned short)effect_index;
        g_fx_rows_view[channel].flags_034 = effect_id;
        g_fx_rows_view[channel].zero_00a = 0;
        g_fx_rows_view[channel].scale_038 = context->sound_2f0[effect_index][2];
    }
    if (owner != 0) owner->linked_06c = 1;
done:
    dispatch_local_2c660();
}

/* 0x6613: count matching effect IDs among the active 24-row prefix. */
int __cdecl count_active_fx_id(int effect_id)
{
    int count;
    int i;

    count = 0;
    for (i = 0; i < 24; ++i) {
        if (g_fx_rows_view[i].active_03c != 0 &&
            g_fx_rows_view[i].id_008 == effect_id)
            ++count;
    }
    return count;
}

/* Private ordinary-C hypothesis for the shared PC tick.  Address-based field
 * names and working ABI views below are not original source/TU claims. */
#include "../../../calibration/engine_dispatch_externs.h"
#include "../../../calibration/engine_interface.h"

typedef struct FxTickActorView FxTickActorView;
struct FxTickActorView {
    unsigned char unknown_000[0x0c];
    short spatial_x_00c;
    short unknown_00e;
    short spatial_y_010;
    unsigned char unknown_012[0x56];
    unsigned long guard_068;
    unsigned long linked_06c;
};

typedef struct FxTickRowView FxTickRowView;
struct FxTickRowView {
    unsigned long unknown_000;
    unsigned long unknown_004;
    unsigned short effect_id_008;
    short delay_word_00a;
    unsigned short unknown_00c;
    unsigned short unknown_00e;
    long pan_left_010;
    long pan_right_014;
    long volume_left_018;
    long volume_right_01c;
    unsigned long unknown_020[4];
    FxTickActorView *owner_030;
    unsigned long flags_034;
    unsigned long scale_038;
    long active_03c;
};

typedef char fx_tick_actor_view_is_112[(sizeof(FxTickActorView) == 0x70) ? 1 : -1];
typedef char fx_tick_row_view_is_64[(sizeof(FxTickRowView) == 0x40) ? 1 : -1];
typedef char fx_tick_ulong_is_4[(sizeof(unsigned long) == 4) ? 1 : -1];
typedef char fx_tick_pointer_is_4[(sizeof(void *) == 4) ? 1 : -1];

#define g_engine_fx_records_tick ((FxTickRowView *)(void *)g_engine_fx_records)
extern void __cdecl dispatch_engine_2caf0(void);
extern void __cdecl dispatch_engine_2ca60(signed char *status);
extern void __cdecl dispatch_engine_2ca70(int channel, unsigned long *info);
extern void __cdecl dispatch_engine_2cbb0(short channel, short left, short right);
extern void __cdecl dispatch_engine_2cb00(short channel, short *left, short *right);
extern void __cdecl dispatch_engine_2cab0(short left, short right);
extern void __cdecl dispatch_engine_2caa0(void);

extern long g_fx_x_721b8;
extern long g_fx_target_left_721bc;
extern long g_fx_target_right_721c0;
extern long g_fx_y_721c4;
extern long g_fx_focus_721c8;
extern long g_fx_square_left_721cc;
extern long g_fx_square_right_721d0;
extern unsigned long g_setup_restore_latch_6ee8c;
extern unsigned long g_setup_previous_mute_6ee98;

void __cdecl dispatch_local_0666b(void)
{
    int i;
    signed char status[24];
    unsigned long info;
    short left;
    short right;
    FxTickRowView *row;

    dispatch_engine_2caf0();
    dispatch_engine_2ca60(status);
    for (i = 0; i < 24; ++i) {
        if (status[i] != 0) {
            dispatch_engine_2ca70(i, &info);
            if (0) {
                g_engine_fx_records_tick[i].active_03c = 0;
            } else {
                g_engine_fx_records_tick[i].active_03c = 1;
                if (g_engine_fx_records_tick[i].owner_030 != 0) {
                    if ((g_engine_fx_records_tick[i].flags_034 & 0x0800UL) != 0) {
                        if (g_engine_fx_records_tick[i].delay_word_00a > 1) {
                            if (g_engine_fx_records_tick[i].owner_030->guard_068 != 0) {
                                if (g_engine_fx_records_tick[i].owner_030->spatial_y_010 != 0) {
                                    row = &g_engine_fx_records_tick[i];
                                    g_fx_x_721b8 = (long)row->owner_030->spatial_x_00c - 160;
                                    if (g_fx_x_721b8 > -20 && g_fx_x_721b8 < 20) {
                                        g_fx_target_left_721bc = 0;
                                        g_fx_target_right_721c0 = g_fx_target_left_721bc;
                                    } else {
                                        if (g_fx_x_721b8 > -20)
                                            g_fx_target_left_721bc = g_fx_x_721b8 * 2 - 40;
                                        else
                                            g_fx_target_left_721bc = 0 - (g_fx_x_721b8 + 20);
                                        if (g_fx_x_721b8 > 20)
                                            g_fx_target_right_721c0 = g_fx_x_721b8 - 20;
                                        else
                                            g_fx_target_right_721c0 = (0 - (g_fx_x_721b8 + 20)) * 2;
                                    }
                                    if (g_fx_target_left_721bc < -640)
                                        g_fx_target_left_721bc = -640;
                                    else if (g_fx_target_left_721bc > 640)
                                        g_fx_target_left_721bc = 640;
                                    if (g_fx_target_right_721c0 < -640)
                                        g_fx_target_right_721c0 = -640;
                                    else if (g_fx_target_right_721c0 > 640)
                                        g_fx_target_right_721c0 = 640;
                                    g_fx_target_left_721bc = 127 - g_fx_target_left_721bc / 5;
                                    g_fx_target_right_721c0 = 127 - g_fx_target_right_721c0 / 5;
                                    if (g_fx_target_left_721bc < 0)
                                        g_fx_target_left_721bc = 0;
                                    if (g_fx_target_left_721bc > 127)
                                        g_fx_target_left_721bc = 127;
                                    if (g_fx_target_right_721c0 < 0)
                                        g_fx_target_right_721c0 = 0;
                                    if (g_fx_target_right_721c0 > 127)
                                        g_fx_target_right_721c0 = 127;

                                    g_fx_y_721c4 = (long)row->owner_030->spatial_y_010 * 2 - 320;
                                    if ((row->flags_034 & 0x1000UL) != 0)
                                        g_fx_y_721c4 <<= 1;
                                    if ((row->flags_034 & 0x2000UL) != 0)
                                        g_fx_y_721c4 >>= 1;
                                    g_fx_focus_721c8 = 0x800 - g_fx_y_721c4;
                                    if (g_fx_focus_721c8 < 0)
                                        g_fx_focus_721c8 = 0;
                                    if (g_fx_focus_721c8 > 0x800)
                                        g_fx_focus_721c8 = 0x800;
                                    g_fx_target_left_721bc =
                                        (g_fx_target_left_721bc * g_fx_focus_721c8) >> 11;
                                    g_fx_target_right_721c0 =
                                        (g_fx_target_right_721c0 * g_fx_focus_721c8) >> 11;
                                    if ((row->flags_034 & 0x4000UL) != 0) {
                                        row->pan_left_010 = g_fx_target_left_721bc;
                                        row->pan_right_014 = g_fx_target_right_721c0;
                                    } else {
                                        row->pan_left_010 +=
                                            (g_fx_target_left_721bc - row->pan_left_010) >> 1;
                                        row->pan_right_014 +=
                                            (g_fx_target_right_721c0 - row->pan_right_014) >> 1;
                                    }
                                    if (g_engine_interface.context_004->unknown_000[4] != 0) {
                                        row->pan_left_010 = 0;
                                        row->pan_right_014 = 0;
                                    }
                                    g_fx_square_left_721cc =
                                        (long)row->scale_038 * row->pan_left_010;
                                    g_fx_square_right_721d0 =
                                        (long)row->scale_038 * row->pan_right_014;
                                    g_fx_square_left_721cc *= g_fx_square_left_721cc;
                                    g_fx_square_right_721d0 *= g_fx_square_right_721d0;
                                    row->volume_left_018 = g_fx_square_left_721cc / 0x4000;
                                    row->volume_right_01c = g_fx_square_right_721d0 / 0x4000;
                                    row->volume_left_018 = row->volume_left_018 * 129 / 128;
                                    row->volume_right_01c = row->volume_right_01c * 129 / 128;
                                    if (g_engine_interface.context_004->unknown_1b24 == 0) {
                                        dispatch_engine_2cbb0(
                                            (short)i,
                                            (short)(((g_engine_fx_records_tick[i].volume_left_018 +
                                                      g_engine_fx_records_tick[i].volume_right_01c) *
                                                     g_engine_interface.context_004->unknown_1b18) >> 8),
                                            (short)(((g_engine_fx_records_tick[i].volume_right_01c +
                                                      g_engine_fx_records_tick[i].volume_left_018) *
                                                     g_engine_interface.context_004->unknown_1b18) >> 8));
                                    } else {
                                        dispatch_engine_2cbb0(
                                            (short)i,
                                            (short)((g_engine_fx_records_tick[i].volume_left_018 *
                                                     g_engine_interface.context_004->unknown_1b18) >> 7),
                                            (short)((g_engine_fx_records_tick[i].volume_right_01c *
                                                     g_engine_interface.context_004->unknown_1b18) >> 7));
                                    }
                                } else if (g_engine_interface.context_004->unknown_000[4] != 0) {
                                    dispatch_engine_2cbb0((short)i, 0, 0);
                                }
                            }
                        } else {
                            g_engine_fx_records_tick[i].delay_word_00a += 1;
                        }
                    } else if (g_engine_interface.context_004->unknown_000[4] != 0) {
                        dispatch_engine_2cbb0((short)i, 0, 0);
                    }
                } else {
                    if (g_engine_interface.context_004->unknown_000[4] != 0) {
                        if (g_setup_previous_mute_6ee98 == 0)
                            dispatch_engine_2cbb0((short)i, 0, 0);
                        g_setup_restore_latch_6ee8c = 1;
                    } else {
                        if (g_setup_restore_latch_6ee8c == 1)
                            dispatch_engine_2cbb0(
                                (short)i,
                                (short)g_engine_fx_records_tick[i].volume_left_018,
                                (short)g_engine_fx_records_tick[i].volume_right_01c);
                        dispatch_engine_2cb00((short)i, &left, &right);
                        g_engine_fx_records_tick[i].volume_left_018 = left;
                        g_engine_fx_records_tick[i].volume_right_01c = right;
                    }
                }
            }
        } else {
            g_engine_fx_records_tick[i].active_03c = 0;
        }
    }

    if (g_engine_interface.context_004->unknown_000[4] != 0) {
        g_setup_previous_mute_6ee98 = 1;
    } else {
        g_setup_previous_mute_6ee98 = 0;
        g_setup_restore_latch_6ee8c = 0;
    }
    dispatch_engine_2caa0();
    if (g_engine_interface.context_004->unknown_1b28 !=
        g_engine_interface.context_004->unknown_1b2c) {
        i = 0;
        do {
            ++i;
            if (g_engine_interface.context_004->unknown_1b28 !=
                g_engine_interface.context_004->unknown_1b2c) {
                if (g_engine_interface.context_004->unknown_1b28 >
                    g_engine_interface.context_004->unknown_1b2c) {
                    --g_engine_interface.context_004->unknown_1b28;
                } else {
                    ++g_engine_interface.context_004->unknown_1b28;
                }
            }
        } while (i < g_engine_interface.context_004->unknown_1b30);
        dispatch_engine_2cab0(
            (short)g_engine_interface.context_004->unknown_1b28,
            (short)g_engine_interface.context_004->unknown_1b28);
    }
}

/* The native 0x83d3 startup entry has no state effects. */
void __cdecl dispatch_local_083d3(void)
{
}
