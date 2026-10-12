#include <windows.h>

/* Fixture storage for the unused completion-gate sibling in the complete
 * dispatcher unit. Production owns this native 0x71ec0 DWORD in frame_data.c;
 * the play_sound scenarios neither read nor write it. */
unsigned long g_dispatch_callback_flag_71ec0;
#include <stdio.h>
#include <string.h>
#include "../calibration/engine_interface.h"
#include "../calibration/engine_fx_record.h"

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

typedef char fx_actor_is_112[(sizeof(FxActorView) == 0x70) ? 1 : -1];
typedef char pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char long_is_32[(sizeof(unsigned long) == 4) ? 1 : -1];

typedef void (__cdecl *PlaySoundEntry)(unsigned short, FxActorView *);
typedef short (__cdecl *StartCallback)(unsigned short, unsigned short,
                                       int, int, int, short, short);

extern void __cdecl play_sound(unsigned short, FxActorView *);
extern void __cdecl dispatch_local_2c650(void);
extern void __cdecl dispatch_local_2c660(void);
extern short __cdecl dispatch_engine_2cb80(unsigned short, unsigned short,
                                          int, int, int, short, short);
extern FxActorView *g_fx_owner_6eed0;
extern unsigned long g_fx_effect_index_6ee88;
extern unsigned long g_fx_bank_value_6ee90;
extern unsigned long g_fx_sample_value_6ee94;
extern unsigned long g_fx_row_value_6eea0;
extern unsigned long g_fx_row_value_6eea4;
extern unsigned short g_fx_bank_word_6eeac;
extern unsigned short g_fx_sample_word_6eeae;
extern long g_fx_x_721b8;
extern long g_fx_target_left_721bc;
extern long g_fx_target_right_721c0;
extern long g_fx_y_721c4;
extern long g_fx_focus_721c8;
extern long g_fx_square_left_721cc;
extern long g_fx_square_right_721d0;
extern long g_fx_smooth_left_6eeb0;
extern long g_fx_smooth_right_6eeb4;
extern long g_fx_volume_left_6eeb8;
extern long g_fx_volume_right_6eebc;
extern unsigned long g_fx_full_effect_id_6eed4;
extern unsigned long g_fx_volume_scale_6eed8;
extern long g_fx_channel_6eee0;

static HMODULE g_oracle;
static int g_events[4];
static int g_event_count;
static short g_start_result;
static unsigned short g_start_bank;
static unsigned short g_start_sample;
static int g_start_zero_a;
static int g_start_frequency;
static int g_start_zero_b;
static short g_start_left;
static short g_start_right;
static unsigned int g_checks;
static unsigned int g_failures;
static unsigned int g_cases;
static int g_native_exception;
static EngineInterface g_source_interface_saved;
static FxSoundRecordView g_source_rows_saved[24];

typedef struct Observation Observation;
struct Observation {
    unsigned long effect_index;
    unsigned long bank;
    unsigned long sample;
    unsigned long row_value_0;
    unsigned long row_value_4;
    unsigned short bank_word;
    unsigned short sample_word;
    unsigned long full_effect_id;
    unsigned long volume_scale;
    unsigned long owner;
    long channel;
    long x;
    long pan_left;
    long pan_right;
    long y;
    long focus;
    long square_left;
    long square_right;
    long smooth_left;
    long smooth_right;
    long volume_left;
    long volume_right;
    unsigned long actor_linked;
    int event_count;
    int events[4];
    unsigned short start_bank;
    unsigned short start_sample;
    int start_zero_a;
    int start_frequency;
    int start_zero_b;
    short start_left;
    short start_right;
    unsigned char channel_row[0x40];
};

typedef struct NativeSaved NativeSaved;
struct NativeSaved {
    unsigned char interface_cells[4][4];
    unsigned char scratch_6ee88[0x5c];
    unsigned char spatial_scratch[0x1c];
    unsigned char fx_rows[24 * 0x40];
};

static unsigned char *native_cell(unsigned long rva)
{
    return (unsigned char *)g_oracle + rva;
}

static unsigned long read32(unsigned long rva)
{
    unsigned long value;
    memcpy(&value, native_cell(rva), sizeof(value));
    return value;
}

static unsigned short read16(unsigned long rva)
{
    unsigned short value;
    memcpy(&value, native_cell(rva), sizeof(value));
    return value;
}

static void write32(unsigned long rva, unsigned long value)
{
    memcpy(native_cell(rva), &value, sizeof(value));
}

static void save_native(NativeSaved *saved)
{
    memcpy(saved->interface_cells[0], native_cell(0x70f24UL), 4);
    memcpy(saved->interface_cells[1], native_cell(0x71098UL), 4);
    memcpy(saved->interface_cells[2], native_cell(0x710a0UL), 4);
    memcpy(saved->interface_cells[3], native_cell(0x712a8UL), 4);
    memcpy(saved->scratch_6ee88, native_cell(0x6ee88UL),
           sizeof(saved->scratch_6ee88));
    memcpy(saved->spatial_scratch, native_cell(0x721b8UL),
           sizeof(saved->spatial_scratch));
    memcpy(saved->fx_rows, native_cell(0x721e0UL), sizeof(saved->fx_rows));
}

static void restore_native(const NativeSaved *saved)
{
    memcpy(native_cell(0x70f24UL), saved->interface_cells[0], 4);
    memcpy(native_cell(0x71098UL), saved->interface_cells[1], 4);
    memcpy(native_cell(0x710a0UL), saved->interface_cells[2], 4);
    memcpy(native_cell(0x712a8UL), saved->interface_cells[3], 4);
    memcpy(native_cell(0x6ee88UL), saved->scratch_6ee88,
           sizeof(saved->scratch_6ee88));
    memcpy(native_cell(0x721b8UL), saved->spatial_scratch,
           sizeof(saved->spatial_scratch));
    memcpy(native_cell(0x721e0UL), saved->fx_rows, sizeof(saved->fx_rows));
}

static void check(int condition, const char *label)
{
    ++g_checks;
    if (!condition) {
        ++g_failures;
        printf("FAIL %s\n", label);
    }
}

static int __cdecl report_native_exception(EXCEPTION_POINTERS *exception)
{
    EXCEPTION_RECORD *record;
    CONTEXT *context;

    record = exception->ExceptionRecord;
    context = exception->ContextRecord;
    printf("NATIVE_EXCEPTION code=%08lx address=%08lx eip=%08lx eax=%08lx ecx=%08lx edx=%08lx esp=%08lx info=%08lx\n",
           record->ExceptionCode,
           (unsigned long)record->ExceptionAddress,
           (unsigned long)context->Eip,
           (unsigned long)context->Eax,
           (unsigned long)context->Ecx,
           (unsigned long)context->Edx,
           (unsigned long)context->Esp,
           (unsigned long)(record->NumberParameters > 1 ?
                           record->ExceptionInformation[1] : 0UL));
    g_native_exception = 1;
    return EXCEPTION_EXECUTE_HANDLER;
}

static void record_event(int event)
{
    if (g_event_count < 4)
        g_events[g_event_count++] = event;
}

static void __cdecl spy_open(void)
{
    printf("SPY open\n");
    record_event(1);
}

static void __cdecl spy_close(void)
{
    printf("SPY close\n");
    record_event(2);
}

static short __cdecl spy_start(unsigned short bank, unsigned short sample,
                               int zero_a, int frequency, int zero_b,
                               short left, short right)
{
    printf("SPY start %u %u %d %d %d %d %d\n",
           bank, sample, zero_a, frequency, zero_b, left, right);
    record_event(3);
    g_start_bank = bank;
    g_start_sample = sample;
    g_start_zero_a = zero_a;
    g_start_frequency = frequency;
    g_start_zero_b = zero_b;
    g_start_left = left;
    g_start_right = right;
    return g_start_result;
}

static void reset_callbacks(short result)
{
    memset(g_events, 0, sizeof(g_events));
    g_event_count = 0;
    g_start_result = result;
    g_start_bank = 0;
    g_start_sample = 0;
    g_start_zero_a = 0;
    g_start_frequency = 0;
    g_start_zero_b = 0;
    g_start_left = 0;
    g_start_right = 0;
}

static void reset_source_scratch(unsigned long seed)
{
    g_fx_effect_index_6ee88 = 0;
    g_fx_bank_value_6ee90 = 0;
    g_fx_sample_value_6ee94 = 0;
    g_fx_row_value_6eea0 = seed;
    g_fx_row_value_6eea4 = seed ^ 0xffffffffUL;
    g_fx_bank_word_6eeac = 0;
    g_fx_sample_word_6eeae = 0;
    g_fx_owner_6eed0 = 0;
    g_fx_full_effect_id_6eed4 = 0;
    g_fx_volume_scale_6eed8 = 0;
    g_fx_channel_6eee0 = 0;
    g_fx_x_721b8 = 0;
    g_fx_target_left_721bc = 0;
    g_fx_target_right_721c0 = 0;
    g_fx_y_721c4 = 0;
    g_fx_focus_721c8 = 0;
    g_fx_square_left_721cc = 0;
    g_fx_square_right_721d0 = 0;
    g_fx_smooth_left_6eeb0 = 0;
    g_fx_smooth_right_6eeb4 = 0;
    g_fx_volume_left_6eeb8 = 0;
    g_fx_volume_right_6eebc = 0;
}

static void prepare_case(ResourceCallbackContext *context,
                         unsigned short effect, FxActorView *actor)
{
    memset(context, 0, sizeof(*context));
    context->sound_2f0[0x49][0] = 3;
    context->sound_2f0[0x49][1] = 7;
    context->sound_2f0[0x49][2] = 16;
    context->sound_2f0[0x4a][0] = 5;
    context->sound_2f0[0x4a][1] = 9;
    context->sound_2f0[0x4a][2] = 24;
    context->unknown_1b18 = 128;
    context->unknown_1b24 = 1;

    memset(actor, 0, sizeof(*actor));
    actor->spatial_x_00c = 120;
    actor->spatial_y_010 = 200;
    actor->guard_068 = 1;
    actor->linked_06c = 0;
    (void)effect;
}

static void capture_native(Observation *observation, FxActorView *actor,
                           unsigned long channel_index)
{
    memset(observation, 0, sizeof(*observation));
    observation->effect_index = read32(0x6ee88UL);
    observation->bank = read32(0x6ee90UL);
    observation->sample = read32(0x6ee94UL);
    observation->row_value_0 = read32(0x6eea0UL);
    observation->row_value_4 = read32(0x6eea4UL);
    observation->bank_word = read16(0x6eeacUL);
    observation->sample_word = read16(0x6eeaeUL);
    observation->owner = read32(0x6eed0UL);
    observation->full_effect_id = read32(0x6eed4UL);
    observation->volume_scale = read32(0x6eed8UL);
    observation->channel = (long)read32(0x6eee0UL);
    observation->x = (long)read32(0x721b8UL);
    observation->pan_left = (long)read32(0x721bcUL);
    observation->pan_right = (long)read32(0x721c0UL);
    observation->y = (long)read32(0x721c4UL);
    observation->focus = (long)read32(0x721c8UL);
    observation->square_left = (long)read32(0x721ccUL);
    observation->square_right = (long)read32(0x721d0UL);
    observation->smooth_left = (long)read32(0x6eeb0UL);
    observation->smooth_right = (long)read32(0x6eeb4UL);
    observation->volume_left = (long)read32(0x6eeb8UL);
    observation->volume_right = (long)read32(0x6eebcUL);
    observation->actor_linked = actor->linked_06c;
    observation->event_count = g_event_count;
    memcpy(observation->events, g_events, sizeof(g_events));
    observation->start_bank = g_start_bank;
    observation->start_sample = g_start_sample;
    observation->start_zero_a = g_start_zero_a;
    observation->start_frequency = g_start_frequency;
    observation->start_zero_b = g_start_zero_b;
    observation->start_left = g_start_left;
    observation->start_right = g_start_right;
    memcpy(observation->channel_row,
           native_cell(0x721e0UL + channel_index * 0x40UL),
           sizeof(observation->channel_row));
}

static void capture_source(Observation *observation, FxActorView *actor,
                           unsigned long channel_index)
{
    memset(observation, 0, sizeof(*observation));
    observation->effect_index = g_fx_effect_index_6ee88;
    observation->bank = g_fx_bank_value_6ee90;
    observation->sample = g_fx_sample_value_6ee94;
    observation->row_value_0 = g_fx_row_value_6eea0;
    observation->row_value_4 = g_fx_row_value_6eea4;
    observation->bank_word = g_fx_bank_word_6eeac;
    observation->sample_word = g_fx_sample_word_6eeae;
    observation->owner = (unsigned long)g_fx_owner_6eed0;
    observation->full_effect_id = g_fx_full_effect_id_6eed4;
    observation->volume_scale = g_fx_volume_scale_6eed8;
    observation->channel = g_fx_channel_6eee0;
    observation->x = g_fx_x_721b8;
    observation->pan_left = g_fx_target_left_721bc;
    observation->pan_right = g_fx_target_right_721c0;
    observation->y = g_fx_y_721c4;
    observation->focus = g_fx_focus_721c8;
    observation->square_left = g_fx_square_left_721cc;
    observation->square_right = g_fx_square_right_721d0;
    observation->smooth_left = g_fx_smooth_left_6eeb0;
    observation->smooth_right = g_fx_smooth_right_6eeb4;
    observation->volume_left = g_fx_volume_left_6eeb8;
    observation->volume_right = g_fx_volume_right_6eebc;
    observation->actor_linked = actor->linked_06c;
    observation->event_count = g_event_count;
    memcpy(observation->events, g_events, sizeof(g_events));
    observation->start_bank = g_start_bank;
    observation->start_sample = g_start_sample;
    observation->start_zero_a = g_start_zero_a;
    observation->start_frequency = g_start_frequency;
    observation->start_zero_b = g_start_zero_b;
    observation->start_left = g_start_left;
    observation->start_right = g_start_right;
    memcpy(observation->channel_row, &g_engine_fx_records[channel_index],
           sizeof(observation->channel_row));
}

static void compare_observations(const Observation *native,
                                 const Observation *source,
                                 const char *case_name)
{
#define EQ_FIELD(field) do { \
    char field_label[128]; \
    sprintf(field_label, "%s %s", case_name, #field); \
    check(native->field == source->field, field_label); \
} while (0)
    EQ_FIELD(effect_index);
    EQ_FIELD(bank);
    EQ_FIELD(sample);
    EQ_FIELD(row_value_0);
    EQ_FIELD(row_value_4);
    EQ_FIELD(bank_word);
    EQ_FIELD(sample_word);
    EQ_FIELD(full_effect_id);
    EQ_FIELD(volume_scale);
    EQ_FIELD(owner);
    EQ_FIELD(channel);
    EQ_FIELD(x);
    EQ_FIELD(pan_left);
    EQ_FIELD(pan_right);
    EQ_FIELD(y);
    EQ_FIELD(focus);
    EQ_FIELD(square_left);
    EQ_FIELD(square_right);
    EQ_FIELD(smooth_left);
    EQ_FIELD(smooth_right);
    EQ_FIELD(volume_left);
    EQ_FIELD(volume_right);
    EQ_FIELD(event_count);
    check(memcmp(native->events, source->events, sizeof(native->events)) == 0,
          "callback order");
    EQ_FIELD(start_bank);
    EQ_FIELD(start_sample);
    EQ_FIELD(start_zero_a);
    EQ_FIELD(start_frequency);
    EQ_FIELD(start_zero_b);
    EQ_FIELD(start_left);
    EQ_FIELD(start_right);
    check(memcmp(native->channel_row, source->channel_row,
                 sizeof(native->channel_row)) == 0,
          "written row bytes");
    EQ_FIELD(actor_linked);
#undef EQ_FIELD
}

static void run_case(EngineInterface *native_interface,
                     ResourceCallbackContext *context,
                     unsigned short effect, short channel_result,
                     const char *case_name)
{
    FxActorView actor;
    Observation native_observation;
    Observation source_observation;
    PlaySoundEntry native_entry;
    unsigned long seed;

    seed = 0x12345678UL ^ (unsigned long)effect;
    prepare_case(context, effect, &actor);
    native_interface->context_004 = context;
    g_engine_interface.context_004 = context;
    native_entry = (PlaySoundEntry)native_cell(0x6da0UL);

    memset(native_cell(0x6ee88UL), 0, 0x5c);
    write32(0x6eea0UL, seed);
    write32(0x6eea4UL, seed ^ 0xffffffffUL);
    memset(native_cell(0x721b8UL), 0, 0x1c);
    memset(native_cell(0x721e0UL), 0, 24 * 0x40);
    reset_callbacks(channel_result);
    printf("CALL native %s %04x\n", case_name, effect);
    __try {
        native_entry(effect, &actor);
    } __except(report_native_exception(GetExceptionInformation())) {
        return;
    }
    printf("RETURN native %s\n", case_name);
    capture_native(&native_observation, &actor, 5UL);

    memset(g_engine_fx_records, 0, sizeof(g_engine_fx_records));
    prepare_case(context, effect, &actor);
    reset_source_scratch(seed);
    reset_callbacks(channel_result);
    printf("CALL source %s %04x\n", case_name, effect);
    play_sound(effect, &actor);
    printf("RETURN source %s\n", case_name);
    capture_source(&source_observation, &actor, 5UL);

    ++g_cases;
    printf("CASE %s event=%04x callback=%d bank/sample=%u/%u volume=%d/%d actor_link(native/source)=%lu/%lu\n",
           case_name, effect, native_observation.event_count,
           native_observation.start_bank, native_observation.start_sample,
           native_observation.start_left, native_observation.start_right,
           native_observation.actor_linked, source_observation.actor_linked);
    compare_observations(&native_observation, &source_observation,
                         case_name);
}

int main(int argc, char **argv)
{
    NativeSaved saved;
    ResourceCallbackContext context;
    EngineInterface *native_interface;
    unsigned long original_base;
    int exit_code;

    exit_code = 1;
    setvbuf(stdout, 0, _IONBF, 0);
    if (argc != 2) {
        fprintf(stderr, "usage: eng1_event_6da0.exe ENG1.DLL\n");
        return 2;
    }
    g_oracle = LoadLibraryA(argv[1]);
    if (g_oracle == 0) {
        fprintf(stderr, "LoadLibraryA failed: %lu\n", GetLastError());
        return 2;
    }
    original_base = (unsigned long)g_oracle;
    if (original_base != 0x10000000UL) {
        fprintf(stderr, "oracle relocated to %08lx; native absolute references require preferred base\n",
                original_base);
        goto cleanup_module;
    }
    printf("LOADED oracle base=%08lx\n", original_base);

    save_native(&saved);
    native_interface = (EngineInterface *)(void *)native_cell(0x70f20UL);
    g_source_interface_saved = g_engine_interface;
    memcpy(g_source_rows_saved, g_engine_fx_records,
           sizeof(g_source_rows_saved));
    write32(0x71098UL, (unsigned long)spy_open);
    write32(0x710a0UL, (unsigned long)spy_close);
    write32(0x712a8UL, (unsigned long)spy_start);
    printf("NATIVE callbacks installed\n");

    g_engine_interface.opaque_174[1] = (unsigned long)spy_open;
    g_engine_interface.opaque_174[3] = (unsigned long)spy_close;
    g_engine_interface.opaque_334[21] = (unsigned long)spy_start;

    memset(&context, 0, sizeof(context));
    memset(g_engine_fx_records, 0, sizeof(g_engine_fx_records));
    g_engine_interface.context_004 = &context;

    run_case(native_interface, &context, 0x4049U, 5,
             "4049-channel-success");
    if (g_native_exception)
        goto restore_state;
    run_case(native_interface, &context, 0x4049U, -1,
             "4049-channel-failure");
    if (g_native_exception)
        goto restore_state;
    run_case(native_interface, &context, 0x404aU, 5,
             "404a-channel-success");
    if (g_native_exception)
        goto restore_state;
    run_case(native_interface, &context, 0x404aU, -1,
             "404a-channel-failure");

    printf("RESULT cases=%u checks=%u failures=%u native_source_equal=%s\n",
           g_cases, g_checks, g_failures,
           (g_cases == 4 && g_failures == 0) ? "yes" : "no");
    exit_code = (g_cases == 4 && g_failures == 0) ? 0 : 1;

restore_state:
    restore_native(&saved);
    g_engine_interface = g_source_interface_saved;
    memcpy(g_engine_fx_records, g_source_rows_saved,
           sizeof(g_source_rows_saved));
    if (g_native_exception)
        exit_code = 1;

cleanup_module:
    if (g_oracle != 0 && !FreeLibrary(g_oracle)) {
        fprintf(stderr, "FreeLibrary failed: %lu\n", GetLastError());
        exit_code = 1;
    }
    return exit_code;
}
