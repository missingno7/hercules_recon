/* ENG1 /Od translation unit 0x5ea0..0x83d8 (one non-/Gy .text contribution), ascending RVA.
 * Demo playback / game-state helpers followed by the sound-effect module whose optimized TITLE
 * copies are title_05080..title_05c60 (calibration/title/r04d80.c). Candidate source; names are
 * address-backed views, not original identifiers. Compile: cl /c /Od (no /Gy for the real layout).
 * The same source is the ENG3 unit 0xae00..0xd27f when compiled with /DENGINE3 (macro name
 * invented): ENG3 lacks the engine-start block of game_state_60f9 case 1/2 but keeps its local.
 * Local-variable names in 0x646c, 0x666b, 0x75e3, 0x7d67, 0x7f8e, 0x819a and 0x8330 are
 * layout-chosen: VC5 /Od orders a frame by 16-bucket identifier hash (see flags.md). */

typedef struct FxOwner {
    unsigned char unknown_00[0x0c];
    short x_0c;
    short unknown_0e;
    short y_10;
    unsigned char unknown_12[0x22 - 0x12];
    unsigned char slot_22;
    unsigned char unknown_23[0x68 - 0x23];
    long guard_68;
    long linked_6c;
} FxOwner;

typedef struct FxRow {
    unsigned long unknown_00;
    unsigned long unknown_04;
    short id_08;
    short count_0a;
    short bank_0c;
    short sample_0e;
    long pan_l_10;
    long pan_r_14;
    long vol_l_18;
    long vol_r_1c;
    long base_l_20;
    long base_r_24;
    unsigned char unknown_28[8];
    FxOwner *owner_30;
    unsigned long flags_34;
    long scale_38;
    unsigned long active_3c;
} FxRow;

typedef struct CtxMasks {
    unsigned long mask[4];
} CtxMasks;

typedef struct EngContext {
    unsigned char state_000;
    unsigned char unknown_001;
    unsigned char unknown_002;
    unsigned char mode_003;
    unsigned char mute_004;
    unsigned char unknown_005;
    unsigned char demo_006;
    unsigned char unknown_007;
    unsigned char demo_index_008;
    unsigned char unknown_009[3];
    unsigned char unknown_00c;
    unsigned char unknown_00d;
    unsigned char unknown_00e;
    unsigned char unknown_00f[0x30 - 0x0f];
    unsigned long flags_030;
    unsigned long unknown_034;
    unsigned long tick_038;
    unsigned long unknown_03c;
    unsigned char unknown_040[0x5c - 0x40];
    unsigned short pad_05c;
    unsigned short pad_05e;
    unsigned short pad_060;
    unsigned char unknown_062[0x6c - 0x62];
    CtxMasks masks_06c;
    unsigned char unknown_07c[0x9c - 0x7c];
    unsigned short mode_09c;
    unsigned char unknown_09e[0xb4 - 0x9e];
    void (__cdecl *load_failed_0b4)(int);
    unsigned char unknown_0b8[0xdf - 0xb8];
    unsigned char blocked_0df;
    unsigned char suppress_0e0;
    unsigned char unknown_0e1[3];
    void *handle_0e4;
    unsigned char unknown_0e8[0x128 - 0xe8];
    unsigned char unknown_128;
    unsigned char unknown_129[3];
    unsigned short unknown_12c;
    unsigned char unknown_12e[0x134 - 0x12e];
    unsigned char rgb_134[3];
    unsigned char unknown_137[0x150 - 0x137];
    unsigned long unknown_150[10];
    unsigned long unknown_178[5];
    unsigned long unknown_18c[(0x2f0 - 0x18c) / 4];
    unsigned char sound_2f0[0x800][3];
    unsigned char unknown_1af0[0x1b18 - 0x1af0];
    unsigned long volume_1b18;
    unsigned char unknown_1b1c[0x1b24 - 0x1b1c];
    unsigned long stereo_1b24;
    unsigned long music_1b28;
    unsigned long music_target_1b2c;
    unsigned long music_step_1b30;
} EngContext;

typedef struct EngInterface {
    unsigned long signature_000;
    EngContext *context_004;
} EngInterface;

typedef struct Palette {
    unsigned char unknown_00[0x19];
    unsigned char rgb_19[3];
} Palette;

typedef struct SlotState {
    unsigned short state_00;
    unsigned char unknown_02[0x12];
} SlotState;

typedef struct ClipRect {
    short x0;
    short y0;
    short x1;
    short y1;
} ClipRect;

extern EngInterface g_engine_interface;
extern FxRow g_fx_rows[24];
extern SlotState g_slots_5cef0[];
extern char *g_demo_paths[8];
extern CtxMasks g_demo_masks;
extern char *g_engine_paths[4];
extern short g_frame_word_6ee84;
extern int g_demo_phase;
extern int g_demo_index;
extern unsigned short *g_demo_words;
extern char g_cold_file_prefix_byte_6eef4[];
extern char g_control_argument_6eef8[];
extern char g_control_argument_6eefc[];
extern char g_control_argument_6ef00[];
extern char g_prefix_6ef04[];
extern char g_prefix_6ef08[];
extern Palette *g_dispatch_frame_current_71ec4;
extern Palette *g_dispatch_frame_base_71ec8;
extern unsigned long g_71ec0;
extern unsigned long g_dispatch_global_6ff88;

/* sound-effect statics */
extern unsigned long g_fx_index;
extern unsigned long g_fx_restore;
extern unsigned long g_fx_bank;
extern unsigned long g_fx_sample;
extern unsigned long g_fx_muted;
extern FxRow g_fx_pending;
extern long g_fx_channel;
extern long g_fx_x;
extern long g_fx_l;
extern long g_fx_r;
extern long g_fx_y;
extern long g_fx_focus;
extern long g_fx_sql;
extern long g_fx_sqr;

int dbg_printf(const char *fmt, ...);
int sprintf(char *buffer, const char *fmt, ...);
void *pool_allocate(void *slot, unsigned long bytes, unsigned long flags);
unsigned long file_load(const char *prefix, const char *name, void *dest, unsigned long bytes);
void resource_destroy(int a);
void dispatch_local_1f220(void);
void resource_reclaim(int a);
void engine_shutdown(void);
void dispatch_engine_2c200(void);
int dispatch_engine_2c210(const char *prefix, const char *name);
void engine_file_cancel(void);
void dispatch_engine_2c270(void);
void f_2c2c0(void);
void dispatch_engine_2c2d0(void *handle);
void f_2c310(int a, int b, int c, int d, int e);
void dispatch_engine_2c370(const char *prefix, const char *name);
void dispatch_engine_2c420(void);
void dispatch_engine_2c430(void);
void dispatch_engine_2c4e0(int a);
void dispatch_local_2c530(void (*callback)(void));
void dispatch_engine_2c580(void);
void dispatch_local_2c5b0(void);
void dispatch_engine_2c630(int a);
void dispatch_engine_2c640(int a);
void snd_lock(void);
void snd_unlock(void);
void f_2c810(ClipRect *rect, int a, int b);
int file_open(const char *path, int a, int b);
int file_close(int handle);
void file_read(int handle, void *dest, unsigned long bytes);
void dispatch_engine_2ca60(char *playing);
void dispatch_engine_2ca70(int channel, unsigned long *info);
void dispatch_local_2ca90(void);
void dispatch_engine_2caa0(void);
void dispatch_engine_2cab0(short left, short right);
void dispatch_local_2cad0(int a);
void dispatch_local_2cae0(void);
void dispatch_engine_2caf0(void);
void dispatch_engine_2cb00(short channel, short *left, short *right);
void snd_stop(short channel);
short snd_play(short bank, short sample, int a, int b, int c, short left, short right);
void snd_set_volume(short channel, short left, short right);
void dispatch_engine_2cc30(int a);
void dispatch_engine_2cc40(void (*callback)(void));

int demo_title_646c(void);
int fx_count_6613(int id);
void fx_update_666b(void);
void fx_clear_all_8117(void);

#define CTX (g_engine_interface.context_004)

#define FX_PAN(r) \
    g_fx_x = (r)->owner_30->x_0c - 160; \
    if (g_fx_x > -20 && g_fx_x < 20) { \
        g_fx_r = g_fx_l = 0; \
    } else { \
        if (g_fx_x > -20) \
            g_fx_l = g_fx_x * 2 - 40; \
        else \
            g_fx_l = 0 - (g_fx_x + 20); \
        if (g_fx_x > 20) \
            g_fx_r = g_fx_x - 20; \
        else \
            g_fx_r = (0 - (g_fx_x + 20)) * 2; \
    } \
    if (g_fx_l < -640) \
        g_fx_l = -640; \
    else if (g_fx_l > 640) \
        g_fx_l = 640; \
    if (g_fx_r < -640) \
        g_fx_r = -640; \
    else if (g_fx_r > 640) \
        g_fx_r = 640; \
    g_fx_l = 127 - g_fx_l / 5; \
    g_fx_r = 127 - g_fx_r / 5; \
    if (g_fx_l < 0) \
        g_fx_l = 0; \
    if (g_fx_l > 127) \
        g_fx_l = 127; \
    if (g_fx_r < 0) \
        g_fx_r = 0; \
    if (g_fx_r > 127) \
        g_fx_r = 127; \
    g_fx_y = (r)->owner_30->y_10 * 2 - 320; \
    if ((r)->flags_34 & 0x1000) \
        g_fx_y <<= 1; \
    if ((r)->flags_34 & 0x2000) \
        g_fx_y >>= 1; \
    g_fx_focus = 0x800 - g_fx_y; \
    if (g_fx_focus < 0) \
        g_fx_focus = 0; \
    if (g_fx_focus > 0x800) \
        g_fx_focus = 0x800; \
    g_fx_l = (g_fx_l * g_fx_focus) >> 11; \
    g_fx_r = (g_fx_r * g_fx_focus) >> 11; \
    if ((r)->flags_34 & 0x4000) { \
        (r)->pan_l_10 = g_fx_l; \
        (r)->pan_r_14 = g_fx_r; \
    } else { \
        (r)->pan_l_10 += (g_fx_l - (r)->pan_l_10) >> 1; \
        (r)->pan_r_14 += (g_fx_r - (r)->pan_r_14) >> 1; \
    } \
    if (CTX->mute_004) { \
        (r)->pan_l_10 = 0; \
        (r)->pan_r_14 = 0; \
    } \
    g_fx_sql = (r)->scale_38 * (r)->pan_l_10; \
    g_fx_sqr = (r)->scale_38 * (r)->pan_r_14; \
    g_fx_sql *= g_fx_sql; \
    g_fx_sqr *= g_fx_sqr; \
    (r)->vol_l_18 = g_fx_sql / 0x4000; \
    (r)->vol_r_1c = g_fx_sqr / 0x4000; \
    (r)->vol_l_18 = (r)->vol_l_18 * 129 / 128; \
    (r)->vol_r_1c = (r)->vol_r_1c * 129 / 128;

/* 0x5ea0 */
void demo_load_5ea0(void)
{
    int index;
    unsigned long bytes;

    if (CTX->flags_030 & 0x1000000)
        CTX->demo_006 = 3;
    if (CTX->demo_006 == 3) {
        g_demo_index = 0x1518;
    } else if ((CTX->flags_030 & 0x8000000) || CTX->demo_006 == 1) {
        bytes = 0x1800;
        index = CTX->demo_index_008;
        if (CTX->flags_030 & 0x2000000)
            index = 0;
        pool_allocate(&g_demo_words, bytes, 0);
        file_load(g_cold_file_prefix_byte_6eef4, g_demo_paths[index], g_demo_words, bytes);
        CTX->unknown_001 = *(unsigned char *)g_demo_words;
        CTX->unknown_002 = CTX->unknown_001;
        CTX->flags_030 |= g_demo_words[1];
        CTX->unknown_00e = ((unsigned char *)g_demo_words)[4];
        CTX->masks_06c = g_demo_masks;
        CTX->unknown_00c = 3;
        g_demo_index = 3;
        g_demo_phase = 2;
    }
}

/* 0x6005 */
void demo_step_6005(void)
{
    int previous;

    if (CTX->mute_004)
        return;
    switch (g_demo_phase) {
    case 0:
        break;
    case 2:
        if (g_demo_index < 0x374) {
            if (CTX->pad_05c != 0)
                CTX->mode_003 = 0x13;
            CTX->pad_05c = g_demo_words[g_demo_index];
            g_demo_index++;
            previous = CTX->pad_05c;
            CTX->pad_05c = g_demo_words[g_demo_index];
            CTX->pad_05e = (previous ^ CTX->pad_05c) & CTX->pad_05c;
            CTX->pad_060 = 0;
        } else {
            CTX->mode_003 = 0x13;
        }
        break;
    }
}

/* 0x60f9 */
int game_state_60f9(void)
{
    int slot;

    switch (CTX->mode_003) {
    case 0:
        return 0;
    case 5:
        if (g_frame_word_6ee84 >= 0x80) {
            CTX->unknown_005 = 0;
            CTX->mode_003 = 0;
        }
        return 0;
    case 1:
    case 2:
        if (CTX->suppress_0e0 || CTX->handle_0e4) {
            CTX->mode_003 = 0;
            return 0;
        }
#ifndef ENGINE3
        slot = 3;
        resource_destroy(slot);
        g_slots_5cef0[slot].state_00 = 6;
        slot = 1;
        resource_destroy(slot);
        g_slots_5cef0[slot].state_00 = 6;
        if (CTX->mode_09c == 1) {
            engine_file_cancel();
            if (CTX->load_failed_0b4)
                CTX->load_failed_0b4(0);
        }
        resource_reclaim(1);
        dispatch_engine_2c210(g_control_argument_6eef8, g_engine_paths[0]);
        CTX->unknown_005 = 1;
        CTX->blocked_0df = 1;
        CTX->mode_003 = 0;
#endif
        CTX->mode_003 = 0;
        return 0;
    case 18:
    case 19:
        if (CTX->mode_09c == 1) {
            engine_file_cancel();
            if (CTX->load_failed_0b4)
                CTX->load_failed_0b4(0);
        }
        if (CTX->handle_0e4)
            dispatch_engine_2c2d0(CTX->handle_0e4);
        dispatch_local_1f220();
        demo_title_646c();
        dispatch_engine_2c580();
        CTX->state_000 = 0xff;
        dispatch_engine_2c370(g_control_argument_6eefc, g_engine_paths[0]);
    case 33:
        if (CTX->mode_09c == 1) {
            engine_file_cancel();
            if (CTX->load_failed_0b4)
                CTX->load_failed_0b4(0);
        }
        CTX->unknown_128 = 0;
        g_71ec0 = 0;
        dispatch_engine_2c640(0);
        dispatch_engine_2cc40(0);
        dispatch_engine_2c630(0);
        dispatch_engine_2cc30(0);
        fx_clear_all_8117();
        engine_shutdown();
        return 0x21;
    default:
        if (CTX->handle_0e4) {
            do {
                dispatch_engine_2c270();
                dispatch_engine_2cc30(0);
                fx_update_666b();
                dispatch_engine_2c430();
                if (!CTX->suppress_0e0 || !CTX->handle_0e4)
                    break;
            } while (1);
        }
        if (CTX->demo_006 == 3)
            CTX->mode_003 = 0x13;
        dispatch_local_1f220();
        dispatch_engine_2c580();
        dispatch_engine_2cc30(0);
        CTX->unknown_12c = 0;
        if (CTX->handle_0e4)
            dispatch_engine_2c200();
        else
            dispatch_engine_2c370(g_control_argument_6ef00, g_engine_paths[0]);
        while (1)
            dispatch_engine_2c430();
    }
    return 0;
}

/* 0x646c */
int demo_title_646c(void)
{
    int offset;
    ClipRect clip;

    clip.x0 = 0;
    clip.y0 = 0;
    clip.x1 = 0x140;
    clip.y1 = 0x100;
    clip.y0 = (g_dispatch_frame_current_71ec4 == g_dispatch_frame_base_71ec8) << 8;
    f_2c810(&clip, 0x140, 0);
    offset = (g_dispatch_frame_current_71ec4 != g_dispatch_frame_base_71ec8) << 8;
    f_2c810(&clip, 0, offset);
    f_2c310(0x140, 0, 4, 0, 0);
    CTX->rgb_134[0] = g_dispatch_frame_current_71ec4->rgb_19[0];
    CTX->rgb_134[1] = g_dispatch_frame_current_71ec4->rgb_19[1];
    CTX->rgb_134[2] = g_dispatch_frame_current_71ec4->rgb_19[2];
    if (dispatch_engine_2c210(g_prefix_6ef04, g_engine_paths[0]) == 0)
        return 0;
    CTX->tick_038 = 0;
    CTX->unknown_03c = g_dispatch_global_6ff88;
    CTX->unknown_128 = 10;
    dispatch_engine_2cc40(f_2c2c0);
    dispatch_engine_2c4e0(1);
    CTX->music_step_1b30 = 4;
    do {
        dispatch_engine_2c420();
        dispatch_engine_2c270();
        dispatch_engine_2cc30(0);
        fx_update_666b();
        if (!CTX->suppress_0e0 || !CTX->handle_0e4)
            break;
    } while (1);
    dispatch_engine_2c580();
    if (CTX->handle_0e4)
        dispatch_engine_2c200();
    else
        dispatch_engine_2c370(g_prefix_6ef08, g_engine_paths[0]);
}

/* 0x6613 */
int fx_count_6613(int id)
{
    int count = 0;
    int i;

    for (i = 0; i < 24; i++) {
        if (g_fx_rows[i].active_3c != 0) {
            if (g_fx_rows[i].id_08 == id) {
                count++;
            }
        }
    }
    return count;
}

/* 0x666b */
void fx_update_666b(void)
{
    FxRow *entry;
    short leftvol;
    short rightvol;
    unsigned long info;
    char playing[24];
    int i;

    dispatch_engine_2caf0();
    dispatch_engine_2ca60(playing);
    for (i = 0; i < 24; i++) {
        if (playing[i] != 0) {
            dispatch_engine_2ca70(i, &info);
            if (0) {
                g_fx_rows[i].active_3c = 0;
            } else {
                g_fx_rows[i].active_3c = 1;
                if (g_fx_rows[i].owner_30 != 0) {
                    if (g_fx_rows[i].flags_34 & 0x800) {
                        if (g_fx_rows[i].count_0a > 1) {
                            if (g_fx_rows[i].owner_30->guard_68 != 0) {
                                if (g_fx_rows[i].owner_30->y_10 != 0) {
                                    entry = &g_fx_rows[i];
                                    FX_PAN(entry)
                                    if (CTX->stereo_1b24 == 0) {
                                        snd_set_volume((short)i,
                                                       (g_fx_rows[i].vol_l_18 + g_fx_rows[i].vol_r_1c) * CTX->volume_1b18 >> 8,
                                                       (g_fx_rows[i].vol_r_1c + g_fx_rows[i].vol_l_18) * CTX->volume_1b18 >> 8);
                                    } else {
                                        snd_set_volume((short)i,
                                                       g_fx_rows[i].vol_l_18 * CTX->volume_1b18 >> 7,
                                                       g_fx_rows[i].vol_r_1c * CTX->volume_1b18 >> 7);
                                    }
                                } else if (CTX->mute_004) {
                                    snd_set_volume((short)i, 0, 0);
                                }
                            }
                        } else {
                            g_fx_rows[i].count_0a++;
                        }
                    } else if (CTX->mute_004) {
                        snd_set_volume((short)i, 0, 0);
                    }
                } else if (CTX->mute_004) {
                    if (g_fx_muted == 0)
                        snd_set_volume((short)i, 0, 0);
                    g_fx_restore = 1;
                } else {
                    if (g_fx_restore == 1)
                        snd_set_volume((short)i, (short)g_fx_rows[i].vol_l_18, (short)g_fx_rows[i].vol_r_1c);
                    dispatch_engine_2cb00((short)i, &leftvol, &rightvol);
                    g_fx_rows[i].vol_l_18 = leftvol;
                    g_fx_rows[i].vol_r_1c = rightvol;
                }
            }
        } else {
            g_fx_rows[i].active_3c = 0;
        }
    }
    if (CTX->mute_004) {
        g_fx_muted = 1;
    } else {
        g_fx_muted = 0;
        g_fx_restore = 0;
    }
    dispatch_engine_2caa0();
    if (CTX->music_1b28 != CTX->music_target_1b2c) {
        for (i = 0; i < CTX->music_step_1b30; i++) {
            if (CTX->music_1b28 != CTX->music_target_1b2c) {
                if (CTX->music_1b28 > CTX->music_target_1b2c)
                    CTX->music_1b28--;
                else
                    CTX->music_1b28++;
            }
        }
        dispatch_engine_2cab0((short)CTX->music_1b28, (short)CTX->music_1b28);
    }
}

/* 0x6da0 LinkedSoundFx */
void LinkedSoundFx_6da0(unsigned short id, FxOwner *owner)
{
    unsigned long vol;

    snd_lock();
    g_fx_index = id & ~0xf800;
    g_fx_bank = CTX->sound_2f0[g_fx_index][0];
    g_fx_sample = CTX->sound_2f0[g_fx_index][1];
    dbg_printf("\n LinkedSoundFx %d :: %d %d", g_fx_index, g_fx_bank, g_fx_sample);
    dbg_printf("\n 0x%x 0x%x 0x%x \n", CTX->unknown_150[g_fx_bank], CTX->unknown_178[g_fx_bank],
               CTX->unknown_18c[g_fx_bank]);
    if (g_fx_bank == 0xff || g_fx_sample == 0xff)
        goto done;
    if (owner == 0 && (id & 0xf800))
        goto done;
    if ((id & 0x4000) && owner != 0 && owner->guard_68 == 0) {
        if (g_slots_5cef0[owner->slot_22].state_00 == 3)
            goto done;
        goto done;
    }
    if (fx_count_6613(g_fx_index) > 5)
        goto done;
    if (id & 0x4000) {
        g_fx_pending.bank_0c = (short)g_fx_bank;
        g_fx_pending.sample_0e = (short)g_fx_sample;
        g_fx_pending.scale_38 = CTX->sound_2f0[g_fx_index][2];
        g_fx_pending.owner_30 = owner;
        g_fx_pending.flags_34 = id;
        FX_PAN(&g_fx_pending)
        if (CTX->stereo_1b24 == 0) {
            g_fx_channel = snd_play((short)g_fx_bank, (short)g_fx_sample, 0, 0x3c, 0,
                                    (g_fx_pending.pan_l_10 + g_fx_pending.pan_r_14) * CTX->volume_1b18 >> 8,
                                    (g_fx_pending.pan_l_10 + g_fx_pending.pan_r_14) * CTX->volume_1b18 >> 8);
        } else {
            g_fx_channel = snd_play((short)g_fx_bank, (short)g_fx_sample, 0, 0x3c, 0,
                                    g_fx_pending.pan_l_10 * CTX->volume_1b18 >> 7,
                                    g_fx_pending.pan_r_14 * CTX->volume_1b18 >> 7);
        }
        if (g_fx_channel >= 0) {
            g_fx_rows[g_fx_channel].unknown_00 = g_fx_pending.unknown_00;
            g_fx_rows[g_fx_channel].unknown_04 = g_fx_pending.unknown_04;
            g_fx_rows[g_fx_channel].pan_l_10 = g_fx_pending.pan_l_10;
            g_fx_rows[g_fx_channel].pan_r_14 = g_fx_pending.pan_r_14;
            g_fx_rows[g_fx_channel].vol_l_18 = g_fx_pending.vol_l_18;
            g_fx_rows[g_fx_channel].vol_r_1c = g_fx_pending.vol_r_1c;
        }
        id &= ~0x4000;
    } else if (id & 0x800) {
        g_fx_channel = snd_play((short)g_fx_bank, (short)g_fx_sample, 0, 0x3c, 0, 0, 0);
        if (g_fx_channel >= 0) {
            g_fx_rows[g_fx_channel].pan_r_14 = 0;
            g_fx_rows[g_fx_channel].pan_l_10 = 0;
        }
    } else {
        vol = CTX->volume_1b18 * 127 >> 7;
        g_fx_channel = snd_play((short)g_fx_bank, (short)g_fx_sample, 0, 0x3c, 0, (short)vol, (short)vol);
        if (g_fx_channel >= 0) {
            g_fx_rows[g_fx_channel].pan_r_14 = vol;
            g_fx_rows[g_fx_channel].pan_l_10 = vol;
        }
    }
    if (g_fx_channel >= 0) {
        g_fx_rows[g_fx_channel].base_l_20 = g_fx_rows[g_fx_channel].pan_l_10;
        g_fx_rows[g_fx_channel].base_r_24 = g_fx_rows[g_fx_channel].pan_r_14;
        g_fx_rows[g_fx_channel].bank_0c = (short)g_fx_bank;
        g_fx_rows[g_fx_channel].sample_0e = (short)g_fx_sample;
        g_fx_rows[g_fx_channel].owner_30 = owner;
        g_fx_rows[g_fx_channel].id_08 = (short)g_fx_index;
        g_fx_rows[g_fx_channel].flags_34 = id;
        g_fx_rows[g_fx_channel].count_0a = 0;
        g_fx_rows[g_fx_channel].scale_38 = CTX->sound_2f0[g_fx_index][2];
        if (owner != 0)
            owner->linked_6c = 1;
    }
done:
    snd_unlock();
}

/* 0x75e3: LinkedSoundFx variant with a separate position owner and local state */
void LinkedSoundFxAt_75e3(unsigned short id, FxOwner *owner, FxOwner *source)
{
    long play_channel;
    int snd_bank;
    int fxid;
    int snd_sample;
    unsigned long def_volume;
    FxRow fx_rec;

    snd_lock();
    fxid = id & ~0xf800;
    snd_bank = CTX->sound_2f0[fxid][0];
    snd_sample = CTX->sound_2f0[fxid][1];
    if (snd_bank == 0xff || snd_sample == 0xff)
        goto done;
    if (owner == 0 && (id & 0xf800))
        goto done;
    if (source == 0 && (id & 0xf800))
        goto done;
    if ((id & 0x4000) && source != 0 && source->guard_68 == 0) {
        if (g_slots_5cef0[source->slot_22].state_00 == 3)
            goto done;
        goto done;
    }
    if (fx_count_6613(fxid) > 5)
        goto done;
    if (id & 0x4000) {
        fx_rec.bank_0c = (short)snd_bank;
        fx_rec.sample_0e = (short)snd_sample;
        fx_rec.scale_38 = CTX->sound_2f0[fxid][2];
        fx_rec.owner_30 = source;
        fx_rec.flags_34 = id;
        FX_PAN(&fx_rec)
        if (CTX->stereo_1b24 == 0) {
            play_channel = snd_play((short)snd_bank, (short)snd_sample, 0, 0x3c, 0,
                               (fx_rec.pan_l_10 + fx_rec.pan_r_14) * CTX->volume_1b18 >> 8,
                               (fx_rec.pan_l_10 + fx_rec.pan_r_14) * CTX->volume_1b18 >> 8);
        } else {
            play_channel = snd_play((short)snd_bank, (short)snd_sample, 0, 0x3c, 0,
                               fx_rec.pan_l_10 * CTX->volume_1b18 >> 7,
                               fx_rec.pan_r_14 * CTX->volume_1b18 >> 7);
        }
        if (play_channel >= 0) {
            g_fx_rows[play_channel].unknown_00 = fx_rec.unknown_00;
            g_fx_rows[play_channel].unknown_04 = fx_rec.unknown_04;
            g_fx_rows[play_channel].pan_l_10 = fx_rec.pan_l_10;
            g_fx_rows[play_channel].pan_r_14 = fx_rec.pan_r_14;
            g_fx_rows[play_channel].vol_l_18 = fx_rec.vol_l_18;
            g_fx_rows[play_channel].vol_r_1c = fx_rec.vol_r_1c;
        }
        id &= ~0x4000;
    } else if (id & 0x800) {
        play_channel = snd_play((short)snd_bank, (short)snd_sample, 0, 0x3c, 0, 0, 0);
        if (play_channel >= 0) {
            g_fx_rows[play_channel].pan_r_14 = 0;
            g_fx_rows[play_channel].pan_l_10 = 0;
        }
    } else {
        def_volume = CTX->volume_1b18 * 127 >> 7;
        play_channel = snd_play((short)snd_bank, (short)snd_sample, 0, 0x3c, 0, (short)def_volume, (short)def_volume);
        if (play_channel >= 0) {
            g_fx_rows[play_channel].pan_r_14 = def_volume;
            g_fx_rows[play_channel].pan_l_10 = def_volume;
        }
    }
    if (play_channel >= 0) {
        g_fx_rows[play_channel].base_l_20 = g_fx_rows[play_channel].pan_l_10;
        g_fx_rows[play_channel].base_r_24 = g_fx_rows[play_channel].pan_r_14;
        g_fx_rows[play_channel].bank_0c = (short)snd_bank;
        g_fx_rows[play_channel].sample_0e = (short)snd_sample;
        g_fx_rows[play_channel].owner_30 = owner;
        g_fx_rows[play_channel].id_08 = (short)fxid;
        g_fx_rows[play_channel].flags_34 = id;
        g_fx_rows[play_channel].count_0a = 0;
        g_fx_rows[play_channel].scale_38 = CTX->sound_2f0[fxid][2];
        if (owner != 0)
            owner->linked_6c = 1;
    }
done:
    snd_unlock();
}

/* 0x7cde */
void RemoveFxLinksKeep_7cde(int arg)
{
    int i;

    if (arg == 0)
        return;
    snd_lock();
    dbg_printf("\n RemoveFxLinks(0x%x)", arg);
    for (i = 0; i < 24; i++) {
        if (g_fx_rows[i].owner_30 == (FxOwner *)arg) {
            if ((g_fx_rows[i].flags_34 & 0x8000) == 0)
                snd_stop((short)i);
            g_fx_rows[i].owner_30 = 0;
        }
    }
    snd_unlock();
}

/* 0x7d67 */
void RemoveFxIdLinksKeep_7d67(unsigned short id, FxOwner *owner)
{
    int snd_bank;
    int i;
    int snd_sample;

    snd_lock();
    snd_bank = CTX->sound_2f0[id][0];
    snd_sample = CTX->sound_2f0[id][1];
    dbg_printf("\n RemoveFxIdLinks(%d,0x%x)", id & ~0xf800, owner);
    if (owner == 0) {
        for (i = 0; i < 24; i++) {
            if (snd_bank == g_fx_rows[i].bank_0c && snd_sample == g_fx_rows[i].sample_0e) {
                if ((g_fx_rows[i].flags_34 & 0x8000) == 0)
                    snd_stop((short)i);
                if (g_fx_rows[i].owner_30 != 0)
                    g_fx_rows[i].owner_30 = 0;
            }
        }
    } else {
        for (i = 0; i < 24; i++) {
            if (owner == g_fx_rows[i].owner_30 && snd_bank == g_fx_rows[i].bank_0c && snd_sample == g_fx_rows[i].sample_0e) {
                if ((g_fx_rows[i].flags_34 & 0x8000) == 0)
                    snd_stop((short)i);
                if (g_fx_rows[i].owner_30 != 0)
                    g_fx_rows[i].owner_30 = 0;
            }
        }
    }
    snd_unlock();
}

/* 0x7ef6 */
void RemoveFxLinks_7ef6(int arg)
{
    int i;

    if (arg == 0)
        return;
    snd_lock();
    dbg_printf("\n RemoveFxLinks(0x%x)", arg);
    for (i = 0; i < 24; i++) {
        if (g_fx_rows[i].owner_30 == (FxOwner *)arg) {
            snd_stop((short)i);
            if (g_fx_rows[i].owner_30 != 0) {
                g_fx_rows[i].owner_30->linked_6c = 0;
                g_fx_rows[i].owner_30 = 0;
            }
        }
    }
    snd_unlock();
}

/* 0x7f8e */
void RemoveFxIdLinks_7f8e(unsigned short id, FxOwner *owner)
{
    int snd_bank;
    int i;
    int snd_sample;

    snd_lock();
    snd_bank = CTX->sound_2f0[id][0];
    snd_sample = CTX->sound_2f0[id][1];
    dbg_printf("\n RemoveFxIdLinks(%d,0x%x)", id & ~0xf800, owner);
    if (owner == 0) {
        for (i = 0; i < 24; i++) {
            if (snd_bank == g_fx_rows[i].bank_0c && snd_sample == g_fx_rows[i].sample_0e) {
                snd_stop((short)i);
                if (g_fx_rows[i].owner_30 != 0) {
                    g_fx_rows[i].owner_30->linked_6c = 0;
                    g_fx_rows[i].owner_30 = 0;
                }
            }
        }
    } else {
        for (i = 0; i < 24; i++) {
            if (owner == g_fx_rows[i].owner_30 && snd_bank == g_fx_rows[i].bank_0c && snd_sample == g_fx_rows[i].sample_0e) {
                snd_stop((short)i);
                if (g_fx_rows[i].owner_30 != 0) {
                    g_fx_rows[i].owner_30->linked_6c = 0;
                    g_fx_rows[i].owner_30 = 0;
                }
            }
        }
    }
    snd_unlock();
}

/* 0x8117 */
void fx_clear_all_8117(void)
{
    int i;

    snd_lock();
    dbg_printf("\n ClearAllSoundFx()");
    for (i = 0; i < 24; i++) {
        if (g_fx_rows[i].active_3c != 0) {
            snd_stop((short)i);
            g_fx_rows[i].bank_0c = 0xff;
            g_fx_rows[i].sample_0e = 0;
            g_fx_rows[i].owner_30 = 0;
        }
    }
    snd_unlock();
}

/* 0x819a */
void fx_clear_except_819a(unsigned short id)
{
    int snd_bank;
    int i;
    int fxid;
    int snd_sample;

    snd_lock();
    fxid = id & ~0xf800;
    snd_bank = CTX->sound_2f0[fxid][0];
    snd_sample = CTX->sound_2f0[fxid][1];
    dbg_printf("\n ClearAllSoundFx_Excpt(%d)", id & ~0xf800);
    for (i = 0; i < 24; i++) {
        if (snd_bank != g_fx_rows[i].bank_0c || snd_sample != g_fx_rows[i].sample_0e) {
            snd_stop((short)i);
            g_fx_rows[i].bank_0c = 0xff;
            g_fx_rows[i].sample_0e = 0;
            g_fx_rows[i].owner_30 = 0;
        }
    }
    snd_unlock();
}

/* 0x827f */
void fx_mode_827f(int a)
{
    snd_lock();
    dispatch_local_2ca90();
    if (a == 1) {
        dispatch_local_2cad0(0x1000);
    } else {
        dispatch_local_2cad0(5);
    }
    dispatch_local_2cae0();
    snd_unlock();
}

/* 0x82b7 */
void fx_init_82b7(int a)
{
    int i;

    dispatch_local_2c5b0();
    g_fx_restore = 0;
    g_fx_muted = 0;
    if (a == 0)
        dispatch_local_2c530(fx_update_666b);
    else
        dispatch_local_2c530(0);
    for (i = 0; i < 24; i++) {
        g_fx_rows[i].bank_0c = 0xff;
        g_fx_rows[i].owner_30 = 0;
    }
}

/* 0x8330 */
int load_file_8330(const char *prefix, const char *name, void *dest, unsigned long bytes)
{
    char path[64];
    int handle;

    sprintf(path, "%s%s", prefix, name);
retry:
    handle = file_open(path, 0, 0);
    if (handle == -1) {
        dbg_printf("Unable to open %s%s \n", prefix, name);
        return 0;
    }
    dbg_printf("Loading %s @ 0x%x,size %d ", path, dest, bytes);
    file_read(handle, dest, bytes);
    if (file_close(handle))
        goto retry;
    dbg_printf("- Loaded \n");
    return 0;
}

/* 0x83d3 */
void fx_nop_83d3(void)
{
}
