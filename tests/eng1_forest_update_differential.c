/* Native differential for the leaf at ENG1 RVA 0x3ca50. The oracle and
 * private C source are exercised against independent WORD cells. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/eng1/behavioral/setup_shared_state.h"
#include "../src/eng1/behavioral/actor_control_state.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef void (__cdecl *NativeTarget)(void);

extern void __cdecl eng1_native_code_target_003ca50(void);

static HMODULE native_module;

static volatile u8 *native_cell(u32 rva)
{
    return (volatile u8 *)(void *)native_module + rva;
}

static int expected_word(u16 input, u16 *output)
{
    *output = (input == 12U || (input >= 15U && input <= 17U))
        ? 0xffc0U : 0xfffeU;
    return 1;
}

static int fail(const char *message, u32 input, u16 native_value,
                u16 source_value, u16 expected)
{
    printf("FAIL %s input=%04x native=%04x source=%04x expected=%04x\n",
           message, (unsigned int)input, (unsigned int)native_value,
           (unsigned int)source_value, (unsigned int)expected);
    return 1;
}

int main(int argc, char **argv)
{
    u32 value;
    u16 old_native_input, old_native_output;
    u16 old_source_input, old_source_output;
    u8 old_native_before, old_native_after;
    u8 old_source_before, old_source_after;
    volatile u16 *native_input;
    volatile u16 *native_output;
    volatile u8 *native_bytes;
    volatile u8 *source_bytes;
    NativeTarget target;

    if (argc != 2) {
        fprintf(stderr, "usage: neutralfixture.exe ENG1.DLL\n");
        return 2;
    }
    native_module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (native_module == 0) {
        fprintf(stderr, "LoadLibraryExA failed: %lu\n", GetLastError());
        return 3;
    }

    native_input = (volatile u16 *)(void *)native_cell(0x70f0cUL);
    native_output = (volatile u16 *)(void *)native_cell(0x7183cUL);
    native_bytes = native_cell(0x7183cUL);
    source_bytes = g_actor_pipeline_storage_71440.byte_cells + 0x3fc;
    target = (NativeTarget)(void *)native_cell(0x3ca50UL);

    old_native_input = *native_input;
    old_native_output = *native_output;
    old_source_input = g_eng1_input_word_70f0c;
    old_source_output = g_eng1_setup_word_7183c;
    old_native_before = native_bytes[-1];
    old_native_after = native_bytes[2];
    old_source_before = source_bytes[-1];
    old_source_after = source_bytes[2];

    native_bytes[-1] = 0xa5U;
    native_bytes[2] = 0x5aU;
    source_bytes[-1] = 0xa5U;
    source_bytes[2] = 0x5aU;

    for (value = 0; value <= 0xffffUL; ++value) {
        u16 expected, native_result, source_result;

        *native_input = (u16)value;
        *native_output = 0x1357U;
        target();
        native_result = *native_output;

        g_eng1_input_word_70f0c = (u16)value;
        g_eng1_setup_word_7183c = 0x1357U;
        eng1_native_code_target_003ca50();
        source_result = g_eng1_setup_word_7183c;
        expected_word((u16)value, &expected);

        if (native_result != expected || source_result != expected) {
            *native_input = old_native_input;
            *native_output = old_native_output;
            g_eng1_input_word_70f0c = old_source_input;
            g_eng1_setup_word_7183c = old_source_output;
            native_bytes[-1]=old_native_before;native_bytes[2]=old_native_after;
            source_bytes[-1]=old_source_before;source_bytes[2]=old_source_after;
            FreeLibrary(native_module);
            return fail("sentinel mapping", value, native_result,
                        source_result, expected);
        }
        if (native_bytes[-1] != 0xa5U || native_bytes[2] != 0x5aU ||
            source_bytes[-1] != 0xa5U || source_bytes[2] != 0x5aU) {
            printf("FAIL non-WORD write input=%04x\n", (unsigned int)value);
            *native_input = old_native_input;
            *native_output = old_native_output;
            g_eng1_input_word_70f0c = old_source_input;
            g_eng1_setup_word_7183c = old_source_output;
            native_bytes[-1]=old_native_before;native_bytes[2]=old_native_after;
            source_bytes[-1]=old_source_before;source_bytes[2]=old_source_after;
            FreeLibrary(native_module);
            return 1;
        }
    }

    *native_input = old_native_input;
    *native_output = old_native_output;
    native_bytes[-1] = old_native_before;
    native_bytes[2] = old_native_after;
    g_eng1_input_word_70f0c = old_source_input;
    g_eng1_setup_word_7183c = old_source_output;
    source_bytes[-1] = old_source_before;
    source_bytes[2] = old_source_after;

    FreeLibrary(native_module);
    printf("PASS exhaustive_WORD_inputs=65536 native_entry=0x3ca50 "
           "input_width=2 output_width=2 canaries=preserved\n");
    printf("forest_update cases=65536 checks=131072 failures=0\n");
    return 0;
}
