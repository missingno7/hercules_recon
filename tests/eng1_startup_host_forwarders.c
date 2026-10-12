#include "../calibration/engine_interface.h"

typedef unsigned long (__cdecl *SpyTwoDwords)(unsigned long, unsigned long);
typedef unsigned long (__cdecl *SpyPointerThreeDwords)(
    void *, unsigned long, unsigned long, unsigned long);

typedef union SpySlotBits {
    unsigned long word;
    SpyTwoDwords two_dwords;
    SpyPointerThreeDwords pointer_three_dwords;
} SpySlotBits;

typedef char spy_requires_x86_words[(sizeof(unsigned long) == 4 &&
                                     sizeof(unsigned int) == 4 &&
                                     sizeof(void *) == 4 &&
                                     sizeof(SpyTwoDwords) == 4 &&
                                     sizeof(SpyPointerThreeDwords) == 4) ?
                                    1 : -1];

EngineInterface g_engine_interface;

unsigned long __cdecl eng1_target_02c2a0(unsigned long, unsigned long);
unsigned long __cdecl eng1_target_02c4a0(void *, unsigned long,
                                         unsigned long, unsigned long);

static unsigned int g_calls;
static unsigned int g_kind;
static unsigned long g_first_word;
static unsigned long g_second_word;
static unsigned long g_third_word;
static unsigned long g_fourth_word;
static void *g_first_pointer;
static unsigned long g_result;

static void reset_spy(void)
{
    g_calls = 0;
    g_kind = 0;
    g_first_word = 0;
    g_second_word = 0;
    g_third_word = 0;
    g_fourth_word = 0;
    g_first_pointer = (void *)0;
    g_result = 0;
}

static unsigned long __cdecl spy_two_dwords(unsigned long first,
                                             unsigned long second)
{
    ++g_calls;
    g_kind = 1;
    g_first_word = first;
    g_second_word = second;
    return g_result;
}

static unsigned long __cdecl spy_pointer_three_dwords(
    void *first, unsigned long second, unsigned long third,
    unsigned long fourth)
{
    ++g_calls;
    g_kind = 2;
    g_first_pointer = first;
    g_second_word = second;
    g_third_word = third;
    g_fourth_word = fourth;
    return g_result;
}

static unsigned long __cdecl wrong_slot(void)
{
    ++g_calls;
    g_kind = 99;
    return 0x13579bdfUL;
}

#define CHECK(test) do { if (!(test)) return __LINE__; } while (0)

int main(void)
{
    SpySlotBits bits;
    void *root = (void *)0x12345678UL;
    unsigned long returned;

    bits.two_dwords = spy_two_dwords;
    g_engine_interface.opaque_038[0] = bits.word;
    bits.two_dwords = (SpyTwoDwords)wrong_slot;
    g_engine_interface.opaque_038[1] = bits.word;

    bits.pointer_three_dwords = spy_pointer_three_dwords;
    g_engine_interface.opaque_090[4] = bits.word;
    bits.pointer_three_dwords = (SpyPointerThreeDwords)wrong_slot;
    g_engine_interface.opaque_090[3] = bits.word;
    g_engine_interface.opaque_090[5] = bits.word;

    reset_spy();
    g_result = 0x89abcdefUL;
    returned = eng1_target_02c2a0(0x00000000UL, 0xfedcba98UL);
    CHECK(g_calls == 1 && g_kind == 1 &&
          g_first_word == 0x00000000UL &&
          g_second_word == 0xfedcba98UL && returned == 0x89abcdefUL);

    reset_spy();
    g_result = 0xf1234567UL;
    returned = eng1_target_02c4a0(root, 0x89abcdefUL,
                                  0x10293847UL, 0xfedcba98UL);
    CHECK(g_calls == 1 && g_kind == 2 && g_first_pointer == root &&
          g_second_word == 0x89abcdefUL &&
          g_third_word == 0x10293847UL &&
          g_fourth_word == 0xfedcba98UL && returned == 0xf1234567UL);

    return 0;
}
