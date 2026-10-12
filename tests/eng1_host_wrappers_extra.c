#include "../calibration/engine_interface.h"

typedef void (__cdecl *SpySlot0)(void);
typedef void (__cdecl *SpySlotPointer)(void *);

typedef union SpySlotBits {
    unsigned long word;
    SpySlot0 f0;
    SpySlotPointer fpointer;
} SpySlotBits;

typedef char spy_requires_x86_words[(sizeof(unsigned long) == 4 &&
                                     sizeof(SpySlot0) == 4 &&
                                     sizeof(SpySlotPointer) == 4) ? 1 : -1];

EngineInterface g_engine_interface;

void __cdecl dispatch_engine_2c6c0(void);
void __cdecl dispatch_engine_2c750(void *);
void __cdecl dispatch_engine_2c760(void *);
void __cdecl dispatch_engine_2c7b0(void *);

static unsigned int g_calls;
static unsigned int g_kind;
static void *g_pointer;

static void reset(void)
{
    g_calls = 0;
    g_kind = 0;
    g_pointer = (void *)0;
}

static void __cdecl spy_2c6c0(void)
{
    ++g_calls;
    g_kind = 1;
}

static void __cdecl spy_2c750(void *pointer)
{
    ++g_calls;
    g_kind = 2;
    g_pointer = pointer;
}

static void __cdecl spy_2c760(void *pointer)
{
    ++g_calls;
    g_kind = 3;
    g_pointer = pointer;
}

static void __cdecl spy_2c7b0(void *pointer)
{
    ++g_calls;
    g_kind = 4;
    g_pointer = pointer;
}

#define CHECK(test) do { if (!(test)) return __LINE__; } while (0)

int main(void)
{
    SpySlotBits bits;
    void *p1 = (void *)0x12345678UL;
    void *p2 = (void *)0x23456789UL;
    void *p3 = (void *)0x3456789aUL;

    bits.f0 = spy_2c6c0;
    g_engine_interface.opaque_174[10] = bits.word;
    bits.fpointer = spy_2c750;
    g_engine_interface.opaque_174[20] = bits.word;
    bits.fpointer = spy_2c760;
    g_engine_interface.opaque_174[21] = bits.word;
    bits.fpointer = spy_2c7b0;
    g_engine_interface.opaque_174[28] = bits.word;

    reset();
    dispatch_engine_2c6c0();
    CHECK(g_calls == 1 && g_kind == 1 && g_pointer == (void *)0);

    reset();
    dispatch_engine_2c750(p1);
    CHECK(g_calls == 1 && g_kind == 2 && g_pointer == p1);

    reset();
    dispatch_engine_2c760(p2);
    CHECK(g_calls == 1 && g_kind == 3 && g_pointer == p2);

    reset();
    dispatch_engine_2c7b0(p3);
    CHECK(g_calls == 1 && g_kind == 4 && g_pointer == p3);

    return 0;
}
