#include <stdio.h>
#include <string.h>
#include "../src/shared/engine_interface.h"

extern unsigned long g_eng1_entry_first_use_5b648;
extern void __cdecl PC_DLLEngineMain(const EngineInterface *, unsigned long);
static ResourceCallbackContext context_a, context_b;
static EngineInterface supplied;
static int calls[16], count, failed;
static int swap_context;
static unsigned long expected_flags = 1;

#define CHECK(test) do { if (!(test)) { ++failed; printf("FAIL line %d\n", __LINE__); } } while (0)

void __cdecl engine_pool_save_arena(void *arena, unsigned long bytes)
{
    calls[count++] = 1;
    CHECK(arena == (void *)0x12340000UL);
    CHECK(bytes == 0x12345UL);
    CHECK(g_eng1_entry_first_use_5b648 == 1);
}
void __cdecl engine_host_update(void)
{
    calls[count++] = 2;
    CHECK(g_eng1_entry_first_use_5b648 == 0);
    if (swap_context)
        g_engine_interface.context_004 = &context_b;
}
void __cdecl engine_dispatch(void)
{
    calls[count++] = 3;
    CHECK(g_engine_interface.context_004->unknown_040 == expected_flags);
}
int main(void)
{
    int i;
    unsigned long *words = (unsigned long *)&supplied;
    for (i = 0; i < 250; ++i)
        words[i] = 0x11110000UL + i;
    supplied.context_004 = &context_a;
    supplied.arena_008 = (void *)0x12340000UL;
    supplied.arena_bytes_00c = 0x12345UL;

    PC_DLLEngineMain(0, 1);
    CHECK(count == 0 && g_eng1_entry_first_use_5b648 == 1);
    PC_DLLEngineMain(&supplied, 1);
    CHECK(count == 0 && g_eng1_entry_first_use_5b648 == 1);
    CHECK(memcmp(&supplied, &g_engine_interface, 1000) == 0);
    supplied.signature_000 = 0x5ac00cacUL;
    context_a.unknown_040 = 0xaaaaaaaaUL;
    context_b.unknown_040 = 0xbbbbbbbbUL;
    swap_context = 1;
    PC_DLLEngineMain(&supplied, 0xfffffffdUL);
    CHECK(count == 3 && calls[0] == 1 && calls[1] == 2 && calls[2] == 3);
    CHECK(context_a.unknown_040 == 0xaaaaaaaaUL);
    CHECK(context_b.unknown_040 == 1);
    CHECK(g_engine_interface.opaque_174[32] == supplied.opaque_174[32]);
    supplied.signature_000 = 0;
    PC_DLLEngineMain(&supplied, 3);
    CHECK(count == 5 && calls[3] == 2 && calls[4] == 3);
    CHECK(g_engine_interface.signature_000 == 0x5ac00cacUL);
    PC_DLLEngineMain(0, 1);
    CHECK(count == 7 && calls[5] == 2 && calls[6] == 3);
    expected_flags = 0;
    PC_DLLEngineMain(0, 0xfffffffeUL);
    CHECK(count == 9 && context_b.unknown_040 == 0);
    printf("ENG1 entry semantics: %s\n", failed ? "FAIL" : "PASS");
    return failed ? 1 : 0;
}
