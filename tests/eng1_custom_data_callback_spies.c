/* Test-only linkage for image descriptors. These callbacks are outside fixture
 * scope and must never execute; no production implementation is supplied. */
#include <stdio.h>
#include <stdlib.h>
void __cdecl eng1_native_code_target_034b40(void *pointer,int index)
{
    (void)pointer; (void)index;
    fputs("unexpected custom initialization callback in source fixture\n",stderr);
    exit(91);
}
void __cdecl eng1_native_code_target_034bb0(void *pointer,int index)
{
    (void)pointer; (void)index;
    fputs("unexpected custom update callback in source fixture\n",stderr);
    exit(92);
}
