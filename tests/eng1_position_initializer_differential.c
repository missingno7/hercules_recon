#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/behavioral/interpreters.h"
#include "../src/eng1/behavioral/actor_control_tables.h"

typedef void (__cdecl *NativeD9b0)(Actor *);

static unsigned char *native_module;
static unsigned long checks;
static unsigned long failures;

extern void __cdecl eng1_native_code_target_02d9b0(Actor *actor);

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

static unsigned char *native_rva(unsigned long rva)
{
    return native_module + rva;
}

static void seed_actor(Actor *actor, unsigned long x)
{
    unsigned char *bytes;

    bytes = (unsigned char *)(void *)actor;
    memset(bytes, 0xa5, sizeof(*actor));
    *(unsigned long *)(void *)(bytes + 0x00) = x;
    *(unsigned long *)(void *)(bytes + 0x04) = 0x11223344UL;
    *(unsigned long *)(void *)(bytes + 0x08) = 0x55667788UL;
    bytes[0x20] = 0x3cU;
    bytes[0x21] = 0x91U;
    bytes[0x22] = 0x6dU;
    bytes[0x23] = 0xe2U;
}

static void run_case(NativeD9b0 native, const char *label,
                     unsigned short high_word, unsigned short low_word,
                     unsigned char expected)
{
    Actor native_actor;
    Actor source_actor;
    unsigned char native_copy[sizeof(Actor)];
    unsigned char source_copy[sizeof(Actor)];
    unsigned long x;

    x = ((unsigned long)high_word << 16) | (unsigned long)low_word;
    seed_actor(&native_actor, x);
    seed_actor(&source_actor, x);

    native(&native_actor);
    eng1_native_code_target_02d9b0(&source_actor);

    memcpy(native_copy, &native_actor, sizeof(native_copy));
    memcpy(source_copy, &source_actor, sizeof(source_copy));
    check(memcmp(native_copy, source_copy, sizeof(native_copy)) == 0,
          "full Actor bytes match original DLL");
    check(*(unsigned long *)(void *)((unsigned char *)&source_actor + 0x00) == x,
          "DWORD actor+0 remains input x");
    check(*(unsigned long *)(void *)((unsigned char *)&source_actor + 0x04) ==
          0x03200000UL, "DWORD actor+4 overwritten");
    check(*(unsigned long *)(void *)((unsigned char *)&source_actor + 0x08) ==
          0x01000000UL, "DWORD actor+8 overwritten");
    check(((unsigned char *)&source_actor)[0x21] == expected,
          "BYTE actor+0x21 equals classifier result");
    check(((unsigned char *)&source_actor)[0x20] == 0x3cU &&
          ((unsigned char *)&source_actor)[0x22] == 0x6dU &&
          ((unsigned char *)&source_actor)[0x23] == 0xe2U,
          "adjacent byte and category preserved");

    if (failures == 0)
        printf("PASS 2d9b0 %s\n", label);
    else
        printf("CASE 2d9b0 %s has failures\n", label);
}

int main(int argc, char **argv)
{
    HMODULE module;
    NativeD9b0 native;
    const unsigned short *native_classifier;

    if (argc != 2) return 2;
    module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (module == 0) return 3;
    native_module = (unsigned char *)module;
    native = (NativeD9b0)(void *)native_rva(0x2d9b0UL);
    native_classifier = (const unsigned short *)(const void *)
        native_rva(0x65678UL);

    check(memcmp(g_eng1_classifier_rows_65678, native_classifier, 216U) == 0,
          "reused classifier data owner matches native 18x6 WORD prefix");

    run_case(native, "below first threshold", 0x04ffU, 0x1357U, 0U);
    run_case(native, "at first threshold", 0x0500U, 0x2468U, 1U);
    run_case(native, "below second threshold", 0x094fU, 0x369aU, 1U);
    run_case(native, "at second threshold", 0x0950U, 0x47abU, 2U);
    run_case(native, "below third threshold", 0x0c4fU, 0x58bcU, 2U);
    run_case(native, "at third threshold", 0x0c50U, 0x69cdU, 3U);
    run_case(native, "positive final-row compare before sentinel", 0x7fffU,
             0x7adeU, 17U);
    run_case(native, "negative signed high word", 0x8000U, 0x8befU, 0xffU);
    run_case(native, "zero high word sentinel scan", 0x0000U, 0x9cf0U, 0xffU);
    run_case(native, "all-ones high word", 0xffffU, 0xad01U, 0xffU);

    check(FreeLibrary(module) != 0, "native DLL unmapped");
    printf("eng1_call_2d9b0 cases=10 checks=%lu failures=%lu\n",
           checks, failures);
    return failures == 0 ? 0 : 1;
}
