/*
 * Isolated 32-bit loader/entry smoke fixture for behavioral TITLE builds.
 * This invokes only the export with the native cold null / bad-signature
 * inputs. It does not call the original game or synthesize callback targets.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN64)
#error Build this fixture with the pinned 32-bit compiler.
#endif

typedef char fixture_pointer_width_must_be_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char fixture_dword_width_must_be_32[(sizeof(unsigned long) == 4) ? 1 : -1];

typedef void (__cdecl *TitleEntry)(const unsigned long *, unsigned long);

/* Capture ESP around the compiler-generated cdecl call. A callee-cleanup
 * entrypoint or a stack imbalance fails this check. */
static int call_cdecl_balanced(TitleEntry entry,
                               const unsigned long *host,
                               unsigned long flags)
{
    volatile unsigned long before = 0;
    volatile unsigned long after = 0;

    __asm mov eax, esp
    __asm mov before, eax
    entry(host, flags);
    __asm mov eax, esp
    __asm mov after, eax
    return before == after;
}

int main(int argc, char **argv)
{
    HMODULE module;
    FARPROC address;
    TitleEntry entry;
    unsigned long words[250];
    unsigned int i;

    if (argc != 2) {
        fprintf(stderr, "usage: title_loader_fixture.exe <absolute-TITLE.DLL-path>\n");
        return 2;
    }

    module = LoadLibraryA(argv[1]);
    if (!module) {
        fprintf(stderr, "FAIL LoadLibraryA error=%lu path=%s\n",
                (unsigned long)GetLastError(), argv[1]);
        return 10;
    }

    address = GetProcAddress(module, "PC_DLLEngineMain");
    if (!address) {
        fprintf(stderr, "FAIL GetProcAddress error=%lu\n",
                (unsigned long)GetLastError());
        FreeLibrary(module);
        return 11;
    }

    /* Win32 FARPROC and the measured export are both 32-bit addresses. */
    entry = (TitleEntry)address;
    if (!call_cdecl_balanced(entry, 0, 0)) {
        fprintf(stderr, "FAIL cdecl stack after cold null-host call\n");
        FreeLibrary(module);
        return 12;
    }

    /* The native entry copies this table, tests word 0, and returns before
     * callbacks when the copied signature is invalid. */
    for (i = 0; i < 250; ++i)
        words[i] = 0x5a5a5a5aUL;
    words[0] = 0;
    if (!call_cdecl_balanced(entry, words, 0)) {
        fprintf(stderr, "FAIL cdecl stack after invalid-signature call\n");
        FreeLibrary(module);
        return 13;
    }

    if (!FreeLibrary(module)) {
        fprintf(stderr, "FAIL FreeLibrary error=%lu\n",
                (unsigned long)GetLastError());
        return 14;
    }

    printf("PASS x86 LoadLibrary/GetProcAddress; cold null and invalid-signature calls returned with cdecl stack balanced\n");
    return 0;
}
