#include <stdio.h>
#include <string.h>
#include "../calibration/engine_interface.h"

#include "../calibration/frame_configuration_row.h"

extern void __cdecl frame_configuration_280f0(void);
extern void __cdecl frame_context_defaults_15600(void);

EngineInterface g_engine_interface;
ConfigurationRow72 g_configuration_rows_68af0[9];
unsigned char g_configuration_selected_71860;
unsigned short g_configuration_word_713b4, g_configuration_word_71432, g_configuration_word_713ba;
unsigned short g_configuration_word_71378, g_configuration_word_713c4;
unsigned short g_configuration_word_713aa, g_configuration_word_713b8;
unsigned short g_configuration_word_7138c, g_configuration_word_71430;
unsigned short g_configuration_word_713a8, g_configuration_word_71308;
unsigned short g_configuration_word_713a0, g_configuration_word_713ac;
unsigned short g_configuration_word_713b6, g_configuration_word_71374;
unsigned short g_configuration_word_71394, g_configuration_word_71380;
unsigned long g_configuration_dword_71388, g_configuration_dword_71370;
unsigned long g_configuration_dword_71390, g_configuration_dword_7137c;
unsigned long g_configuration_dword_713a4, g_configuration_dword_7139c;
unsigned long g_configuration_dword_713b0, g_configuration_dword_71384;
unsigned long g_configuration_dword_713c0, g_configuration_dword_71398;
unsigned long g_configuration_dword_713bc;
unsigned long g_context_default_dword_721a8, g_context_default_dword_72190;

static unsigned long checks;
static int failed(int condition) { ++checks; return condition; }

static int check_configuration(const ConfigurationRow72 *row)
{
    if (failed(g_configuration_selected_71860 != 0)) return 1;
    if (failed(g_configuration_word_713b4 != row->word_002)) return 2;
    if (failed(g_configuration_word_71432 != (unsigned short)(row->word_004 + 5))) return 3;
    if (failed(g_configuration_word_713ba != (unsigned short)(row->word_006 + 5))) return 4;
    if (failed(g_configuration_word_71378 != row->word_008 || g_configuration_word_713c4 != row->word_008)) return 5;
    if (failed(g_configuration_word_713aa != row->word_00a || g_configuration_word_713b8 != row->word_00a)) return 6;
    if (failed(g_configuration_word_7138c != row->word_00c || g_configuration_word_71430 != row->word_00c)) return 7;
    if (failed(g_configuration_word_713a8 != row->word_00e || g_configuration_word_71308 != row->word_00e)) return 8;
    if (failed(g_configuration_word_713a0 != row->word_010 || g_configuration_word_713ac != row->word_010)) return 9;
    if (failed(g_configuration_word_713b6 != row->word_012 || g_configuration_word_71374 != row->word_012)) return 10;
    if (failed(g_configuration_word_71394 != row->word_014 || g_configuration_word_71380 != row->word_016)) return 11;
    if (failed(g_configuration_dword_71388 != row->dword_018 || g_configuration_dword_71370 != row->dword_01c)) return 12;
    if (failed(g_configuration_dword_71390 != row->dword_020 || g_configuration_dword_7137c != row->dword_024)) return 13;
    if (failed(g_configuration_dword_713a4 != row->dword_028 || g_configuration_dword_7139c != row->dword_02c)) return 14;
    if (failed(g_configuration_dword_713b0 != row->dword_030 || g_configuration_dword_71384 != row->dword_034)) return 15;
    if (failed(g_configuration_dword_713c0 != row->dword_038 || g_configuration_dword_71398 != row->dword_040)) return 16;
    if (failed(g_configuration_dword_713bc != row->dword_044)) return 17;
    return 0;
}

static void fill_row(ConfigurationRow72 *row, unsigned short id, unsigned int n)
{
    unsigned int base;
    base = 0x1200 + n * 0x111;
    row->word_000 = id;
    row->word_002 = (unsigned short)(base + 2);
    row->word_004 = (unsigned short)(base + 4);
    row->word_006 = (unsigned short)(base + 6);
    row->word_008 = (unsigned short)(base + 8);
    row->word_00a = (unsigned short)(base + 10);
    row->word_00c = (unsigned short)(base + 12);
    row->word_00e = (unsigned short)(base + 14);
    row->word_010 = (unsigned short)(base + 16);
    row->word_012 = (unsigned short)(base + 18);
    row->word_014 = (unsigned short)(base + 20);
    row->word_016 = (unsigned short)(base + 22);
    row->dword_018 = 0x10203040UL + n;
    row->dword_01c = 0x11223340UL + n;
    row->dword_020 = 0x22334450UL + n;
    row->dword_024 = 0x33445560UL + n;
    row->dword_028 = 0x44556670UL + n;
    row->dword_02c = 0x55667780UL + n;
    row->dword_030 = 0x66778890UL + n;
    row->dword_034 = 0x778899a0UL + n;
    row->dword_038 = 0x8899aab0UL + n;
    row->unknown_03c = 0x99aabbc0UL + n;
    row->dword_040 = 0xaabbccd0UL + n;
    row->dword_044 = 0xbbccddeeUL + n;
}

static void init_table(void)
{
    static const unsigned short ids[9] = {0, 1, 3, 5, 6, 7, 9, 11, 999};
    unsigned int i;
    for (i = 0; i < 9; ++i) fill_row(&g_configuration_rows_68af0[i], ids[i], i);
}

static int check_defaults(unsigned char initial, unsigned char expected)
{
    ResourceCallbackContext context;
    unsigned char *bytes;
    unsigned int i;
    memset(&context, 0xa5, sizeof(context));
    bytes = (unsigned char *)&context;
    bytes[0x0c] = initial;
    g_engine_interface.context_004 = &context;
    g_context_default_dword_721a8 = 0xeeeeeeeeUL;
    g_context_default_dword_72190 = 0xddddddddUL;
    frame_context_defaults_15600();
    if (failed(g_context_default_dword_721a8 != 1 || g_context_default_dword_72190 != 0)) return 30;
    if (failed(bytes[0x0c] != expected || bytes[0x0d] != 0 || bytes[0x11] != 0)) return 31;
    if (failed(bytes[0x1c] != 0 || bytes[0x1d] != 0 || bytes[0x1e] != 0x80 || bytes[0x1f] != 0)) return 32;
    if (failed(bytes[0x20] != 0 || bytes[0x21] != 0)) return 33;
    for (i = 0; i < sizeof(context); ++i) {
        if (i == 0x0c || i == 0x0d || i == 0x11 || (i >= 0x1c && i <= 0x21)) continue;
        if (failed(bytes[i] != 0xa5)) return 34;
    }
    return 0;
}

int main(void)
{
    static const unsigned char input_levels[7] = {0, 1, 9, 10, 0x7f, 0x80, 0xff};
    static const unsigned char output_levels[7] = {1, 1, 9, 9, 9, 0x80, 0xff};
    ResourceCallbackContext context;
    ConfigurationRow72 saved;
    unsigned int i;
    int result;

    memset(&context, 0, sizeof(context));
    g_engine_interface.context_004 = &context;
    init_table();

    context.unknown_000[0] = 3;
    g_configuration_selected_71860 = 1;
    frame_configuration_280f0();
    result = check_configuration(&g_configuration_rows_68af0[2]);
    if (result) { printf("later hit output check %d\n", result); return 1; }

    context.unknown_000[0] = 2;
    frame_configuration_280f0();
    result = check_configuration(&g_configuration_rows_68af0[8]);
    if (result) { printf("miss did not copy sentinel row (%d)\n", result); return 2; }

    saved = g_configuration_rows_68af0[2];
    g_configuration_rows_68af0[2].word_004 = 0xfffa;
    g_configuration_rows_68af0[2].word_006 = 0xfffb;
    context.unknown_000[0] = 3;
    frame_configuration_280f0();
    if (failed(g_configuration_word_71432 != 0xffff || g_configuration_word_713ba != 0)) {
        printf("+5 did not wrap as two low words\n"); return 3;
    }
    g_configuration_rows_68af0[2] = saved;

    saved = g_configuration_rows_68af0[1];
    g_configuration_rows_68af0[1].word_000 = 0xffff;
    context.unknown_000[0] = 0xff;
    frame_configuration_280f0();
    result = check_configuration(&g_configuration_rows_68af0[1]);
    if (result) { printf("signed byte did not match 16-bit 0xffff row (%d)\n", result); return 4; }
    g_configuration_rows_68af0[1] = saved;

    saved = g_configuration_rows_68af0[0];
    fill_row(&g_configuration_rows_68af0[0], 999, 0);
    g_engine_interface.context_004 = 0;
    frame_configuration_280f0();
    result = check_configuration(&g_configuration_rows_68af0[0]);
    if (result) { printf("first sentinel path failed (%d)\n", result); return 5; }
    g_configuration_rows_68af0[0] = saved;

    for (i = 0; i < 7; ++i) {
        result = check_defaults(input_levels[i], output_levels[i]);
        if (result) { printf("defaults byte case %u failed (%d)\n", i, result); return 10 + (int)i; }
    }

    {
        ResourceCallbackContext first_context;
        ResourceCallbackContext second_context;
        memset(&first_context, 0xa5, sizeof(first_context));
        memset(&second_context, 0xa5, sizeof(second_context));
        first_context.unknown_000[0x0c] = 2;
        second_context.unknown_000[0x0c] = 0xff;
        g_engine_interface.context_004 = &first_context;
        frame_context_defaults_15600();
        g_engine_interface.context_004 = &second_context;
        frame_context_defaults_15600();
        if (failed(first_context.unknown_000[0x0c] != 2 || second_context.unknown_000[0x0c] != 0xff)) {
            printf("defaults did not follow the current interface context slot\n"); return 20;
        }
    }

    printf("frame configuration/defaults fixture: %lu checks, 0 failures\n", checks);
    return 0;
}
