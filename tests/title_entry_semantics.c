/* Test-only spies; this fixture never substitutes for production providers. */
#include <stdio.h>
#include <string.h>
#include "../src/title/title_engine.h"

TitleEngineInterface g_engine_interface;
int g_257e8[30] = {1};
static ResourceCallbackContext context_a, context_b;
static int save_count, update_count, frame_count, error_count, phase, replace_context;
static int expected_flags;
void __cdecl PC_DLLEngineMain(const TitleEngineInterface *, unsigned int);

static void check(int test, const char *name)
{
    if (!test) { ++error_count; printf("FAIL %s\n", name); }
}

int title_0c390(int arena, int bytes)
{
    ++save_count;
    check(phase == 0, "save before update");
    check(g_257e8[0] == 1, "flag remains set in save");
    check(arena == 0x12340000 && bytes == 0x300000, "save arguments");
    phase = 1;
    return -37; /* Must be ignored, including on failure-looking values. */
}

void title_0ca00(void)
{
    ++update_count;
    check(g_257e8[0] == 0, "flag cleared before update");
    check(phase == 1 || phase == 0, "update ordering");
    if (replace_context) g_engine_interface.context_004 = &context_b;
    phase = 2;
}

void title_05fb0(void)
{
    ++frame_count;
    check(phase == 2, "frame after update");
    check(g_engine_interface.context_004->unknown_040 == expected_flags,
          "flag masked and written before frame");
    phase = 3;
}

int main(void)
{
    TitleEngineInterface input, before, replacement;
    unsigned int flags[4] = {0, 1, 2, 0xffffffffU};
    int i;
    memset(&g_engine_interface, 0xa5, sizeof(g_engine_interface));
    before = g_engine_interface;
    PC_DLLEngineMain(0, 0);
    check(!memcmp(&before, &g_engine_interface, 1000), "cold null does not copy");
    check(save_count + update_count + frame_count == 0 && g_257e8[0] == 1,
          "cold null does not start");

    memset(&input, 0x6b, sizeof(input));
    input.signature_000 = 0;
    PC_DLLEngineMain(&input, 0);
    check(!memcmp(&input, &g_engine_interface, 1000), "invalid signature still copies all bytes");
    check(save_count + update_count + frame_count == 0 && g_257e8[0] == 1,
          "invalid signature can retry");

    /* Zero callback slots are permitted by the native export. The test spies
     * isolate the entry; real wrappers would still require their used slots.
     */
    memset(&input, 0, sizeof(input));
    input.signature_000 = 0x5ac00cacUL;
    input.context_004 = &context_a;
    input.arena_008 = (void *)0x12340000;
    input.arena_bytes_00c = 0x300000;
    input.unused_038 = 0x87654321UL;
    input.slot_3e4 = (int (__cdecl *)())0x76543210;
    expected_flags = 1;
    PC_DLLEngineMain(&input, 3);
    check(save_count == 1 && update_count == 1 && frame_count == 1, "valid initial call");
    check(g_engine_interface.unused_038 == input.unused_038 &&
          g_engine_interface.slot_3e4 == input.slot_3e4, "opaque and final DWORD copied");

    memset(&replacement, 0xff, sizeof(replacement));
    for (i = 0; i < 4; ++i) {
        phase = 0;
        expected_flags = flags[i] & 1;
        PC_DLLEngineMain(i & 1 ? &replacement : 0, flags[i]);
        check(g_engine_interface.signature_000 == 0x5ac00cacUL,
              "warm call ignores host including null");
    }
    phase = 0;
    replace_context = 1;
    expected_flags = 1;
    context_b.unknown_040 = 99;
    PC_DLLEngineMain(0, 1);
    check(context_b.unknown_040 == 1, "context pointer reloaded after host update");
    check(save_count == 1 && update_count == 6 && frame_count == 6,
          "save once, update and frame each warm call");
    printf("entry ABI fixture: %s (%d failures)\n", error_count ? "FAIL" : "PASS", error_count);
    return error_count != 0;
}
