#include "../calibration/engine_interface.h"

typedef void (__cdecl *SpyColorCallback)(unsigned long);

typedef union SpySlotBits {
    unsigned long word;
    SpyColorCallback color;
} SpySlotBits;

typedef char spy_requires_x86_words[(sizeof(unsigned long) == 4 &&
                                     sizeof(SpyColorCallback) == 4) ? 1 : -1];

EngineInterface g_engine_interface;
void __cdecl dispatch_engine_2c830(unsigned long frame_value);

static unsigned int g_calls;
static unsigned long g_value;

static void reset(void)
{
    g_calls = 0;
    g_value = 0;
}

static void __cdecl spy_color(unsigned long frame_value)
{
    ++g_calls;
    g_value = frame_value;
}

#define CHECK(test) do { if (!(test)) return __LINE__; } while (0)

int main(void)
{
    SpySlotBits bits;

    bits.color = spy_color;
    g_engine_interface.opaque_1fc[16] = bits.word;

    reset();
    dispatch_engine_2c830(0x12345680UL);
    CHECK(g_calls == 1 && g_value == 0x12345680UL);

    reset();
    dispatch_engine_2c830(0xa1b2c3ffUL);
    CHECK(g_calls == 1 && g_value == 0xa1b2c3ffUL);

    reset();
    dispatch_engine_2c830(0xdeadbeffUL);
    CHECK(g_calls == 1 && g_value == 0xdeadbeffUL);

    return 0;
}
