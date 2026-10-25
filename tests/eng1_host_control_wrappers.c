#include "../src/shared/engine_interface.h"

typedef void (__cdecl *SpySlot0)(void);
typedef unsigned long (__cdecl *SpySlotPointerDwordResult)(
    void *, unsigned long);
typedef void (__cdecl *SpySlotPointerDword)(void *, unsigned long);
typedef unsigned int (__cdecl *SpySlotRandomLimit)(int);

typedef union SpySlotBits {
    unsigned long word;
    SpySlot0 f0;
    SpySlotPointerDwordResult fpointer_dword_result;
    SpySlotPointerDword fpointer_dword;
    SpySlotRandomLimit frandom_limit;
} SpySlotBits;

typedef char spy_requires_x86_words[(sizeof(unsigned long) == 4 &&
                                     sizeof(unsigned int) == 4 &&
                                     sizeof(SpySlot0) == 4 &&
                                     sizeof(SpySlotPointerDwordResult) == 4 &&
                                     sizeof(SpySlotRandomLimit) == 4) ? 1 : -1];

EngineInterface g_engine_interface;

void __cdecl dispatch_engine_2c200(void);
unsigned long __cdecl dispatch_engine_2c210(void *, unsigned long);
void __cdecl dispatch_engine_2c370(void *, unsigned long);
void __cdecl dispatch_engine_2c430(void);
void __cdecl dispatch_engine_2c580(void);
unsigned int __cdecl dispatch_engine_2c460(int);

static unsigned int g_calls;
static unsigned int g_kind;
static void *g_pointer;
static unsigned long g_word;
static unsigned long g_result_word;
static int g_limit;
static unsigned int g_result_uint;

static void reset(void)
{
    g_calls = 0;
    g_kind = 0;
    g_pointer = (void *)0;
    g_word = 0;
    g_result_word = 0;
    g_limit = 0;
    g_result_uint = 0;
}

static void __cdecl spy_2c200(void)
{
    ++g_calls;
    g_kind = 1;
}

static unsigned long __cdecl spy_2c210(void *first, unsigned long second)
{
    ++g_calls;
    g_kind = 2;
    g_pointer = first;
    g_word = second;
    return g_result_word;
}

static void __cdecl spy_2c370(void *first, unsigned long second)
{
    ++g_calls;
    g_kind = 3;
    g_pointer = first;
    g_word = second;
}

static void __cdecl spy_2c430(void)
{
    ++g_calls;
    g_kind = 4;
}

static void __cdecl spy_2c580(void)
{
    ++g_calls;
    g_kind = 5;
}

static unsigned int __cdecl spy_2c460(int limit)
{
    ++g_calls;
    g_kind = 6;
    g_limit = limit;
    return g_result_uint;
}

#define CHECK(test) do { if (!(test)) return __LINE__; } while (0)

int main(void)
{
    SpySlotBits bits;
    void *p1 = (void *)0x12345678UL;
    void *p2 = (void *)0x23456789UL;
    unsigned long returned_word;
    unsigned int returned_uint;

    bits.f0 = spy_2c200;
    g_engine_interface.opaque_010[2] = bits.word;
    bits.fpointer_dword_result = spy_2c210;
    g_engine_interface.opaque_010[3] = bits.word;
    bits.fpointer_dword = spy_2c370;
    g_engine_interface.opaque_064[0] = bits.word;
    bits.f0 = spy_2c430;
    g_engine_interface.opaque_088[0] = bits.word;
    bits.f0 = spy_2c580;
    g_engine_interface.opaque_0e4[0] = bits.word;
    bits.frandom_limit = spy_2c460;
    g_engine_interface.opaque_090[1] = bits.word;

    reset();
    dispatch_engine_2c200();
    CHECK(g_calls == 1 && g_kind == 1);

    reset();
    g_result_word = 0x89abcdefUL;
    returned_word = dispatch_engine_2c210(p1, 0x10293847UL);
    CHECK(g_calls == 1 && g_kind == 2 && g_pointer == p1 &&
          g_word == 0x10293847UL && returned_word == 0x89abcdefUL);

    reset();
    dispatch_engine_2c370(p2, 0xfedcba98UL);
    CHECK(g_calls == 1 && g_kind == 3 && g_pointer == p2 &&
          g_word == 0xfedcba98UL);

    reset();
    dispatch_engine_2c430();
    CHECK(g_calls == 1 && g_kind == 4);

    reset();
    dispatch_engine_2c580();
    CHECK(g_calls == 1 && g_kind == 5);

    reset();
    g_result_uint = 0xf1234567UL;
    returned_uint = dispatch_engine_2c460(73);
    CHECK(g_calls == 1 && g_kind == 6 && g_limit == 73 &&
          returned_uint == 0xf1234567UL);

    return 0;
}
