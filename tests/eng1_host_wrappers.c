#include "../src/shared/engine_interface.h"

typedef void (__cdecl *SpySlot0)(void);
typedef void (__cdecl *SpySlotWord)(unsigned long);
typedef void (__cdecl *SpySlotWords2)(unsigned long, unsigned long);
typedef void (__cdecl *SpySlotWords3)(unsigned long, unsigned long,
                                      unsigned long);
typedef void (__cdecl *SpySlotCallback)(EngineNoArgCallback);
typedef void (__cdecl *SpySlotConstPointer)(const void *);
typedef void (__cdecl *SpySlotPointer)(void *);
typedef void (__cdecl *SpySlotPointerPair)(void *, const void *);

typedef union SpySlotBits {
    unsigned long word;
    SpySlot0 f0;
    SpySlotWord fword;
    SpySlotWords2 fwords2;
    SpySlotWords3 fwords3;
    SpySlotCallback fcallback;
    SpySlotConstPointer fconst_pointer;
    SpySlotPointer fpointer;
    SpySlotPointerPair fpointer_pair;
} SpySlotBits;

typedef char spy_requires_x86_words[(sizeof(unsigned long) == 4 &&
                                     sizeof(SpySlot0) == 4) ? 1 : -1];

EngineInterface g_engine_interface;

void __cdecl dispatch_local_2c530(EngineNoArgCallback);
void __cdecl dispatch_local_2c5b0(void);
void __cdecl dispatch_local_2c650(void);
void __cdecl dispatch_local_2c660(void);
void __cdecl dispatch_local_2ca90(void);
void __cdecl dispatch_local_2cad0(unsigned long);
void __cdecl dispatch_local_2cae0(void);
void __cdecl dispatch_engine_2c670(unsigned long, unsigned long,
                                   unsigned long);
void __cdecl dispatch_engine_2c690(const void *);
void __cdecl dispatch_engine_2c6d0(void);
void __cdecl dispatch_engine_2c6e0(void);
void __cdecl dispatch_engine_2c700(const void *);
void __cdecl dispatch_engine_2c710(const void *);
void __cdecl dispatch_engine_2c740(void *);
void __cdecl dispatch_engine_2c770(void *);
void __cdecl dispatch_engine_2c790(void *);
void __cdecl dispatch_engine_2c7f0(void *, const void *);
void __cdecl dispatch_engine_2c920(void *, const void *);
void __cdecl dispatch_engine_2c940(void *, const void *);
void __cdecl dispatch_engine_2c9d0(unsigned long, unsigned long);
void __cdecl dispatch_engine_2c9f0(unsigned long);

static unsigned int g_calls;
static unsigned int g_kind;
static unsigned long g_words[3];
static const void *g_first_pointer;
static const void *g_second_pointer;
static EngineNoArgCallback g_callback;

static void reset(unsigned int kind)
{
    g_calls = 0;
    g_kind = kind;
    g_words[0] = 0;
    g_words[1] = 0;
    g_words[2] = 0;
    g_first_pointer = (const void *)0;
    g_second_pointer = (const void *)0;
    g_callback = (EngineNoArgCallback)0;
}

static int called(unsigned int kind)
{
    return g_calls == 1 && g_kind == kind;
}

static void __cdecl spy0(void)
{
    ++g_calls;
}

static void __cdecl spy_word(unsigned long a)
{
    ++g_calls;
    g_words[0] = a;
}

static void __cdecl spy_words2(unsigned long a, unsigned long b)
{
    ++g_calls;
    g_words[0] = a;
    g_words[1] = b;
}

static void __cdecl spy_words3(unsigned long a, unsigned long b,
                               unsigned long c)
{
    ++g_calls;
    g_words[0] = a;
    g_words[1] = b;
    g_words[2] = c;
}

static void __cdecl spy_callback(EngineNoArgCallback callback)
{
    ++g_calls;
    g_callback = callback;
}

static void __cdecl spy_const_pointer(const void *pointer)
{
    ++g_calls;
    g_first_pointer = pointer;
}

static void __cdecl spy_pointer(void *pointer)
{
    ++g_calls;
    g_first_pointer = pointer;
}

static void __cdecl spy_pointer_pair(void *first, const void *second)
{
    ++g_calls;
    g_first_pointer = first;
    g_second_pointer = second;
}

#define CHECK(test) do { if (!(test)) return __LINE__; } while (0)
#define INSTALL(member, index, field, function) do { \
    SpySlotBits bits; bits.field = function; g_engine_interface.member[index] = bits.word; \
} while (0)

int main(void)
{
    EngineNoArgCallback marker_callback = spy0;
    const void *p1 = (const void *)0x12345678UL;
    const void *p2 = (const void *)0x23456789UL;

    INSTALL(opaque_0d0, 0, fcallback, spy_callback);
    reset(1);
    dispatch_local_2c530(marker_callback);
    CHECK(called(1) && g_callback == marker_callback);

    INSTALL(opaque_0f0, 0, f0, spy0);
    INSTALL(opaque_174, 1, f0, spy0);
    INSTALL(opaque_174, 3, f0, spy0);
    INSTALL(opaque_2f0, 13, f0, spy0);
    INSTALL(opaque_334, 14, f0, spy0);
    reset(2); dispatch_local_2c5b0(); CHECK(called(2));
    reset(3); dispatch_local_2c650(); CHECK(called(3));
    reset(4); dispatch_local_2c660(); CHECK(called(4));
    reset(5); dispatch_local_2ca90(); CHECK(called(5));
    reset(6); dispatch_local_2cae0(); CHECK(called(6));

    INSTALL(opaque_334, 13, fword, spy_word);
    reset(7); dispatch_local_2cad0(0x1000UL);
    CHECK(called(7) && g_words[0] == 0x1000UL);

    INSTALL(opaque_174, 7, fwords3, spy_words3);
    reset(8); dispatch_engine_2c670(0x11UL, 0x22UL, 0x33UL);
    CHECK(called(8) && g_words[0] == 0x11UL &&
          g_words[1] == 0x22UL && g_words[2] == 0x33UL);

    INSTALL(opaque_174, 8, fconst_pointer, spy_const_pointer);
    reset(9); dispatch_engine_2c690(p1);
    CHECK(called(9) && g_first_pointer == p1);

    INSTALL(opaque_174, 11, f0, spy0);
    INSTALL(opaque_174, 13, f0, spy0);
    reset(10); dispatch_engine_2c6d0(); CHECK(called(10));
    reset(11); dispatch_engine_2c6e0(); CHECK(called(11));

    INSTALL(opaque_174, 15, fconst_pointer, spy_const_pointer);
    INSTALL(opaque_174, 16, fconst_pointer, spy_const_pointer);
    reset(12); dispatch_engine_2c700(p1);
    CHECK(called(12) && g_first_pointer == p1);
    reset(13); dispatch_engine_2c710(p2);
    CHECK(called(13) && g_first_pointer == p2);

    INSTALL(opaque_174, 19, fpointer, spy_pointer);
    INSTALL(opaque_174, 24, fpointer, spy_pointer);
    INSTALL(opaque_174, 26, fpointer, spy_pointer);
    reset(14); dispatch_engine_2c740((void *)p1);
    CHECK(called(14) && g_first_pointer == p1);
    reset(15); dispatch_engine_2c770((void *)p1);
    CHECK(called(15) && g_first_pointer == p1);
    reset(16); dispatch_engine_2c790((void *)p2);
    CHECK(called(16) && g_first_pointer == p2);

    INSTALL(opaque_1fc, 2, fpointer_pair, spy_pointer_pair);
    INSTALL(opaque_284, 3, fpointer_pair, spy_pointer_pair);
    INSTALL(opaque_284, 4, fpointer_pair, spy_pointer_pair);
    reset(17); dispatch_engine_2c7f0((void *)p1, p2);
    CHECK(called(17) && g_first_pointer == p1 && g_second_pointer == p2);
    reset(18); dispatch_engine_2c920((void *)p1, p2);
    CHECK(called(18) && g_first_pointer == p1 && g_second_pointer == p2);
    reset(19); dispatch_engine_2c940((void *)p2, p1);
    CHECK(called(19) && g_first_pointer == p2 && g_second_pointer == p1);

    INSTALL(opaque_2b0, 2, fwords2, spy_words2);
    INSTALL(opaque_2b0, 3, fword, spy_word);
    reset(20); dispatch_engine_2c9d0(0x11223344UL, 0xaabbccddUL);
    CHECK(called(20) && g_words[0] == 0x11223344UL &&
          g_words[1] == 0xaabbccddUL);
    reset(21); dispatch_engine_2c9f0(0x55667788UL);
    CHECK(called(21) && g_words[0] == 0x55667788UL);

    return 0;
}
