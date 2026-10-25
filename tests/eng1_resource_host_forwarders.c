#include <stdio.h>
#include "../src/shared/engine_interface.h"

typedef unsigned long u32;

extern u32 __cdecl eng1_target_02c400(const char *prefix, const char *suffix);
extern u32 __cdecl eng1_target_02c4f0(u32 row_word_000);
extern u32 __cdecl eng1_target_02c480(void *row_buffer, u32 load_mode);

EngineInterface g_engine_interface;

static const char prefix_text[] = "prefix-fixture";
static const char suffix_text[] = "suffix-fixture";
static unsigned char row_buffer[12];
static int call_order[3];
static int call_count;
static const char *seen_prefix;
static const char *seen_suffix;
static u32 seen_row_word;
static void *seen_row_buffer;
static u32 seen_load_mode;

#define CHECK(condition) do { \
    if (!(condition)) { \
        printf("FAIL line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static u32 __cdecl spy_prefix_suffix(const char *prefix, const char *suffix)
{
    call_order[call_count++] = 1;
    seen_prefix = prefix;
    seen_suffix = suffix;
    return 0x89abcdefUL;
}

static u32 __cdecl spy_row_word(u32 row_word_000)
{
    call_order[call_count++] = 2;
    seen_row_word = row_word_000;
    return 0xfedcba98UL;
}

static u32 __cdecl spy_row_buffer(void *buffer, u32 load_mode)
{
    call_order[call_count++] = 3;
    seen_row_buffer = buffer;
    seen_load_mode = load_mode;
    return 0x81234567UL;
}

int main(void)
{
    u32 result;
    u32 input_word;
    u32 input_mode;

    g_engine_interface.opaque_07c[1] = (u32)spy_prefix_suffix;
    g_engine_interface.opaque_0bc[1] = (u32)spy_row_word;
    g_engine_interface.opaque_090[3] = (u32)spy_row_buffer;

    result = eng1_target_02c400(prefix_text, suffix_text);
    CHECK(result == 0x89abcdefUL);
    CHECK(call_count == 1 && call_order[0] == 1);
    CHECK(seen_prefix == prefix_text && seen_suffix == suffix_text);

    input_word = 0x76543210UL;
    result = eng1_target_02c4f0(input_word);
    CHECK(result == 0xfedcba98UL);
    CHECK(call_count == 2 && call_order[1] == 2);
    CHECK(seen_row_word == input_word);

    input_mode = 0x89abcdefUL;
    result = eng1_target_02c480(row_buffer, input_mode);
    CHECK(result == 0x81234567UL);
    CHECK(call_count == 3 && call_order[2] == 3);
    CHECK(seen_row_buffer == row_buffer && seen_load_mode == input_mode);
    CHECK(call_count == 3);

    puts("PASS: three native copied-interface forwards, argument order, and full DWORD EAX");
    return 0;
}
