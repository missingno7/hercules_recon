#include <windows.h>
#include <stdio.h>
#include "incomplete.h"

ENG1_NORETURN void __cdecl eng1_incomplete_behavior(
    unsigned long original_rva, const char *reason)
{
    char message[384];
    DWORD arguments[2];
    /* Reasons are compile-time diagnostics; bound their display explicitly. */
    sprintf(message, "ENG1 incomplete behavior at RVA 0x%lx: %.280s\n",
            original_rva, reason);
    OutputDebugStringA(message);
    fputs(message, stderr);
    fflush(stderr);
    arguments[0] = original_rva;
    arguments[1] = (DWORD)reason;
    RaiseException(0xe0484731UL, EXCEPTION_NONCONTINUABLE, 2, arguments);
    ExitProcess(0xe0484731UL);
}
