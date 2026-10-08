#include "engine_interface.h"

#include "frame_configuration_row.h"

typedef char configuration_row_is_72_bytes[sizeof(ConfigurationRow72) == 72 ? 1 : -1];

/* Observed context-prefix view for the byte and WORD accesses in 15600. */
typedef struct ContextDefaultsObservedPrefix {
    unsigned char opaque_000[0x0c];
    signed char signed_byte_00c;
    unsigned char opaque_00d[0x11 - 0x0d];
    unsigned char byte_011;
    unsigned char opaque_012[0x1c - 0x12];
    unsigned short word_01c;
    unsigned short word_01e;
    unsigned short word_020;
} ContextDefaultsObservedPrefix;

typedef char context_defaults_view_is_22_bytes[
    sizeof(ContextDefaultsObservedPrefix) == 0x22 ? 1 : -1];
typedef char context_defaults_view_offsets[
    (offsetof(ContextDefaultsObservedPrefix, signed_byte_00c) == 0x0c &&
     offsetof(ContextDefaultsObservedPrefix, byte_011) == 0x11 &&
     offsetof(ContextDefaultsObservedPrefix, word_01c) == 0x1c &&
     offsetof(ContextDefaultsObservedPrefix, word_01e) == 0x1e &&
     offsetof(ContextDefaultsObservedPrefix, word_020) == 0x20) ? 1 : -1];

#define CURRENT_CONTEXT_DEFAULTS \
    ((ContextDefaultsObservedPrefix *)g_engine_interface.context_004)

extern ConfigurationRow72 g_configuration_rows_68af0[];
extern unsigned char g_configuration_selected_71860;

extern unsigned short g_configuration_word_713b4;
extern unsigned short g_configuration_word_71432;
extern unsigned short g_configuration_word_713ba;
extern unsigned short g_configuration_word_71378;
extern unsigned short g_configuration_word_713c4;
extern unsigned short g_configuration_word_713aa;
extern unsigned short g_configuration_word_713b8;
extern unsigned short g_configuration_word_7138c;
extern unsigned short g_configuration_word_71430;
extern unsigned short g_configuration_word_713a8;
extern unsigned short g_configuration_word_71308;
extern unsigned short g_configuration_word_713a0;
extern unsigned short g_configuration_word_713ac;
extern unsigned short g_configuration_word_713b6;
extern unsigned short g_configuration_word_71374;
extern unsigned short g_configuration_word_71394;
extern unsigned short g_configuration_word_71380;

extern unsigned long g_configuration_dword_71388;
extern unsigned long g_configuration_dword_71370;
extern unsigned long g_configuration_dword_71390;
extern unsigned long g_configuration_dword_7137c;
extern unsigned long g_configuration_dword_713a4;
extern unsigned long g_configuration_dword_7139c;
extern unsigned long g_configuration_dword_713b0;
extern unsigned long g_configuration_dword_71384;
extern unsigned long g_configuration_dword_713c0;
extern unsigned long g_configuration_dword_71398;
extern unsigned long g_configuration_dword_713bc;

extern unsigned long g_context_default_dword_721a8;
extern unsigned long g_context_default_dword_72190;

void __cdecl frame_configuration_280f0(void)
{
    ConfigurationRow72 *row;
    unsigned short selected_id;
    unsigned int selected_index;

    selected_index = 0;
    g_configuration_selected_71860 = 0;
    if (g_configuration_rows_68af0[selected_index].word_000 != 999) {
        selected_id = (unsigned short)(short)(signed char)
            g_engine_interface.context_004->unknown_000[0];
        while (g_configuration_rows_68af0[selected_index].word_000 != selected_id &&
               g_configuration_rows_68af0[selected_index].word_000 != 999) {
            ++selected_index;
        }
    }
    row = &g_configuration_rows_68af0[selected_index];

    g_configuration_word_713b4 = row->word_002;
    g_configuration_word_71432 = (unsigned short)(row->word_004 + 5);
    g_configuration_word_713ba = (unsigned short)(row->word_006 + 5);
    g_configuration_word_71378 = row->word_008;
    g_configuration_word_713c4 = row->word_008;
    g_configuration_word_713aa = row->word_00a;
    g_configuration_word_713b8 = row->word_00a;
    g_configuration_word_7138c = row->word_00c;
    g_configuration_word_71430 = row->word_00c;
    g_configuration_word_713a8 = row->word_00e;
    g_configuration_word_71308 = row->word_00e;
    g_configuration_word_713a0 = row->word_010;
    g_configuration_word_713ac = row->word_010;
    g_configuration_word_713b6 = row->word_012;
    g_configuration_word_71374 = row->word_012;
    g_configuration_word_71394 = row->word_014;
    g_configuration_word_71380 = row->word_016;

    g_configuration_dword_71388 = row->dword_018;
    g_configuration_dword_71370 = row->dword_01c;
    g_configuration_dword_71390 = row->dword_020;
    g_configuration_dword_7137c = row->dword_024;
    g_configuration_dword_713a4 = row->dword_028;
    g_configuration_dword_7139c = row->dword_02c;
    g_configuration_dword_713b0 = row->dword_030;
    g_configuration_dword_71384 = row->dword_034;
    g_configuration_dword_713c0 = row->dword_038;
    g_configuration_dword_71398 = row->dword_040;
    g_configuration_dword_713bc = row->dword_044;
}

void __cdecl frame_context_defaults_15600(void)
{
    g_context_default_dword_721a8 = 1;
    g_context_default_dword_72190 = 0;
    CURRENT_CONTEXT_DEFAULTS->word_01e = 0x80;
    CURRENT_CONTEXT_DEFAULTS->word_020 = 0;
    CURRENT_CONTEXT_DEFAULTS->word_01c = 0;
    CURRENT_CONTEXT_DEFAULTS->byte_011 = 0;
    if (CURRENT_CONTEXT_DEFAULTS->signed_byte_00c == 0) {
        CURRENT_CONTEXT_DEFAULTS->signed_byte_00c = 1;
    }
    if (CURRENT_CONTEXT_DEFAULTS->signed_byte_00c > 9) {
        CURRENT_CONTEXT_DEFAULTS->signed_byte_00c = 9;
    }
    ((unsigned char *)g_engine_interface.context_004)[0x0d] = 0;
}
