#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "../src/eng1/behavioral/color_transition_views.h"

typedef void (__cdecl *NativeInit)(void *object);

static unsigned long checks;
static unsigned long failures;

static void check_bytes(const void *actual, const void *expected,
                        unsigned long size, const char *label)
{
    ++checks;
    if (memcmp(actual, expected, size) != 0) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

static void fill_pattern(color_u8 *bytes, unsigned long size,
                         color_u8 seed)
{
    unsigned long i;
    for (i = 0; i < size; ++i)
        bytes[i] = (color_u8)(seed + i * 37UL + (i >> 1));
}

static int run_case(color_u8 *native_base, NativeInit native_init,
                    unsigned int case_index, color_u8 seed)
{
    color_u8 native_node[0x80];
    color_u8 source_node[0x80];
    color_u8 native_group[32];
    color_u8 native_timer[4];
    unsigned long i;
    char label[80];

    /* Unequal row values distinguish both native loads from default100/100. */
    {
        static const color_u32 current[5] = {100UL, 90UL, 0UL, 0xffffffffUL, 0x80000000UL};
        static const color_u32 target[5] = {100UL, 125UL, 0xffffffffUL, 0UL, 0x7fffffffUL};
        g_eng1_color_transition_pairs_589b8[0].current = current[case_index];
        g_eng1_color_transition_pairs_589b8[0].target = target[case_index];
        memcpy(native_base + 0x589b8, g_eng1_color_transition_pairs_589b8, 8);
    }

    fill_pattern(native_node, sizeof native_node, seed);
    memcpy(source_node, native_node, sizeof native_node);
    fill_pattern(native_group, sizeof native_group, (color_u8)(seed ^ 0x5a));
    memcpy(native_base + 0x6f4a8, native_group, sizeof native_group);
    memcpy(&g_eng1_color_transition_state_6f4a8, native_group,
           sizeof native_group);
    fill_pattern(native_timer, sizeof native_timer, (color_u8)(seed ^ 0xa5));
    memcpy(native_base + 0x71ecc, native_timer, sizeof native_timer);
    memcpy(&g_eng1_color_transition_timer_71ecc, native_timer,
           sizeof native_timer);

    for (i = 0; i != sizeof native_group; ++i) {
        ++checks;
        if (native_base[0x6f4a8 + i] !=
            ((color_u8 *)&g_eng1_color_transition_state_6f4a8)[i]) {
            ++failures;
            printf("FAIL precondition state setup case %u byte %lu\n",
                   case_index, i);
            break;
        }
    }

    native_init(native_node);
    eng1_native_code_target_01eba0(source_node);

    sprintf(label, "case %u complete node", case_index);
    check_bytes(source_node, native_node, sizeof native_node, label);
    sprintf(label, "case %u eight-DWORD state group", case_index);
    check_bytes(&g_eng1_color_transition_state_6f4a8,
                native_base + 0x6f4a8, sizeof g_eng1_color_transition_state_6f4a8,
                label);
    sprintf(label, "case %u transition timer", case_index);
    check_bytes(&g_eng1_color_transition_timer_71ecc,
                native_base + 0x71ecc, sizeof g_eng1_color_transition_timer_71ecc,
                label);
    return failures == 0;
}

int main(int argc, char **argv)
{
    HMODULE module;
    color_u8 *native_base;
    NativeInit native_init;
    unsigned int case_index;
    color_u8 seeds[5];

    if (argc != 2) {
        printf("usage: color_transition_init_1eba0_differential ENG1.DLL\n");
        return 2;
    }
    module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (!module) {
        printf("LoadLibraryExA(DONT_RESOLVE_DLL_REFERENCES) failed %lu\n",
               GetLastError());
        return 3;
    }

    native_base = (color_u8 *)module;
    native_init = (NativeInit)(void *)(native_base + 0x1eba0);
    seeds[0] = 0x00;
    seeds[1] = 0x17;
    seeds[2] = 0x63;
    seeds[3] = 0xa9;
    seeds[4] = 0xff;

    check_bytes(g_eng1_color_transition_pairs_589b8,
                native_base + 0x589b8,
                8UL * sizeof(Eng1ColorTransitionPair8),
                "complete 0x589b8 provider bytes");
    check_bytes(&g_eng1_color_transition_state_6f4a8,
                native_base + 0x6f4a8, 32UL,
                "initial eight-DWORD state prefix");
    check_bytes(&g_eng1_color_transition_timer_71ecc,
                native_base + 0x71ecc, 4UL,
                "initial timer DWORD");

    for (case_index = 0; case_index != 5; ++case_index)
        run_case(native_base, native_init, case_index, seeds[case_index]);

    printf("color_transition_1eba0_cases=5 checks=%lu failures=%lu\n",
           checks, failures);
    FreeLibrary(module);
    return failures ? 1 : 0;
}
