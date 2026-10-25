#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/interpreters.h"

typedef void (__cdecl *NativeA50)(Actor *);

static unsigned char *native_module;
static unsigned long checks;
static unsigned long failures;

extern void __cdecl eng1_native_code_target_040a50(Actor *actor);

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

static void seed_actor(Actor *actor, unsigned short control_word,
                       unsigned long old_render_flags)
{
    unsigned char *bytes;

    bytes = (unsigned char *)(void *)actor;
    memset(bytes, 0xa5, sizeof(*actor));
    *(unsigned short *)(void *)(bytes + 0x2a) = control_word;
    *(unsigned short *)(void *)(bytes + 0x70) = 0x7152U;
    *(unsigned short *)(void *)(bytes + 0x72) = 0x7233U;
    *(unsigned short *)(void *)(bytes + 0x74) = 0x7454U;
    bytes[0x76] = 0x76U;
    bytes[0x77] = 0x77U;
    *(unsigned long *)(void *)(bytes + 0x54) = old_render_flags;
    *(unsigned long *)(void *)(bytes + 0xd8) = 0xd8112233UL;
    *(unsigned long *)(void *)(bytes + 0xdc) = 0xdc445566UL;
    *(unsigned long *)(void *)(bytes + 0xe0) = 0xe0778899UL;
}

static void run_case(NativeA50 native, const char *label,
                     unsigned short control_word,
                     unsigned long old_render_flags,
                     unsigned short expected_70,
                     unsigned short expected_74,
                     unsigned long expected_d8,
                     unsigned long expected_dc)
{
    Actor native_actor;
    Actor source_actor;
    unsigned char native_copy[sizeof(Actor)];
    unsigned char source_copy[sizeof(Actor)];
    unsigned long before_failures;
    unsigned char *bytes;

    before_failures = failures;
    seed_actor(&native_actor, control_word, old_render_flags);
    seed_actor(&source_actor, control_word, old_render_flags);
    native(&native_actor);
    eng1_native_code_target_040a50(&source_actor);

    memcpy(native_copy, &native_actor, sizeof(native_copy));
    memcpy(source_copy, &source_actor, sizeof(source_copy));
    bytes = (unsigned char *)(void *)&source_actor;
    check(memcmp(native_copy, source_copy, sizeof(native_copy)) == 0,
          "full Actor state matches original DLL");
    check(*(unsigned long *)(void *)(bytes + 0x54) ==
          (old_render_flags | 0x10004000UL),
          "DWORD render flags OR mask");
    check(*(unsigned short *)(void *)(bytes + 0x2a) == control_word,
          "control WORD retained");
    check(*(unsigned short *)(void *)(bytes + 0x70) == expected_70 &&
          *(unsigned short *)(void *)(bytes + 0x72) == 0x7233U,
          "WORD +0x70 branch and adjacent +0x72 preservation");
    check(*(unsigned short *)(void *)(bytes + 0x74) == expected_74 &&
          bytes[0x76] == 0x76U && bytes[0x77] == 0x77U,
          "WORD +0x74 branch and adjacent byte preservation");
    check(*(unsigned long *)(void *)(bytes + 0xd8) == expected_d8,
          "DWORD +0xd8 branch result");
    check(*(unsigned long *)(void *)(bytes + 0xdc) == expected_dc &&
          *(unsigned long *)(void *)(bytes + 0xe0) == 0xe0778899UL,
          "DWORD +0xdc branch and adjacent +0xe0 preservation");
    check(bytes[0x29] == 0xa5U && bytes[0x2c] == 0xa5U,
          "control WORD neighboring bytes preserved");

    if (failures == before_failures)
        printf("PASS 40a50 %s\n", label);
    else
        printf("CASE 40a50 %s has failures\n", label);
}

int main(int argc, char **argv)
{
    HMODULE module;
    NativeA50 native;

    if (argc != 2) return 2;
    module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (module == 0) return 3;
    native_module = (unsigned char *)module;
    native = (NativeA50)(void *)native_rva(0x40a50UL);

    run_case(native, "no axis flags", 0x0005U, 0x01020304UL,
             0x7152U, 0x7454U, 0xd8112233UL, 0xdc445566UL);
    run_case(native, "bit 0x0100 only", 0x0107U, 0x80000001UL,
             0x7152U, 0x0100U, 7UL, 0xdc445566UL);
    run_case(native, "bit 0x0200 only", 0x0209U, 0x00000003UL,
             0x7152U, 0x7454U, 0xd8112233UL, 9UL);
    run_case(native, "bit 0x0400 only", 0x040bU, 0xabcdef01UL,
             0x0100U, 0x7454U, 0xd8112233UL, 11UL);
    run_case(native, "scale selector alone has no field write", 0x0805U,
             0x00010000UL, 0x7152U, 0x7454U, 0xd8112233UL, 0xdc445566UL);
    run_case(native, "scale 0x300 with bit 0x0100", 0x090dU,
             0x20304050UL, 0x7152U, 0x0300U, 13UL, 0xdc445566UL);
    run_case(native, "scale 0x300 with bit 0x0200", 0x0a0eU,
             0x76543210UL, 0x7152U, 0x7454U, 0xd8112233UL, 14UL);
    run_case(native, "scale 0x300 with bit 0x0400", 0x0c0fU,
             0x00000000UL, 0x0300U, 0x7454U, 0xd8112233UL, 15UL);
    run_case(native, "all three independent branches", 0x0f05U,
             0x11223344UL, 0x0300U, 0x0300U, 5UL, 5UL);
    run_case(native, "unselected upper state bits", 0xf005U,
             0x5555aaaaUL, 0x7152U, 0x7454U, 0xd8112233UL, 0xdc445566UL);

    check(FreeLibrary(module) != 0, "native DLL unmapped");
    printf("eng1_call_40a50 cases=10 checks=%lu failures=%lu\n",
           checks, failures);
    return failures == 0 ? 0 : 1;
}
