/* Bounded fixture for the observed 0x5710 -> 0x56c0 -> 0x5660/0x5780 path. */
#include <stdio.h>

typedef short Eng1TokenReference[2];
Eng1TokenReference g_actor_token_references[32];
void *g_frame_owner_7348c;
unsigned char g_frame_global_73498;
extern unsigned char g_frame_setup_result_734ec;

void __cdecl dispatch_local_05710(void);
unsigned char __cdecl dispatch_local_05660(unsigned long, unsigned long);
void __cdecl dispatch_local_05780(const unsigned short *, unsigned long,
                                  unsigned long);

static int fail(const char *message)
{
    puts(message);
    return 1;
}

int main(void)
{
    unsigned short frame_words[32 * 256];
    unsigned short full_words[256];
    unsigned long i;

    for (i = 0; i < 32UL * 256UL; ++i)
        frame_words[i] = 0xaaaaU;
    g_frame_owner_7348c = frame_words;
    g_frame_global_73498 = 0;
    g_frame_setup_result_734ec = 0;

    /* A dirty prefix proves the initializer resets all 32 exact rows. */
    for (i = 0; i < 32UL; ++i) {
        g_actor_token_references[i][0] = 7;
        g_actor_token_references[i][1] = (short)(0x1200U + i);
    }

    dispatch_local_05710();

    if (g_frame_setup_result_734ec != 0)
        return fail("unexpected selected token byte");
    /* 0x5660 reports a new entry with bit7; 0x56c0 clears that marker. */
    if (g_actor_token_references[0][0] != 1 ||
        (unsigned short)g_actor_token_references[0][1] != 0x7641U)
        return fail("token row zero was not initialized");
    for (i = 1; i < 32UL; ++i) {
        if (g_actor_token_references[i][0] != 0 ||
            g_actor_token_references[i][1] != -1)
            return fail("token reset extent or sentinel differs");
    }
    if (frame_words[0] != 0)
        return fail("selected 256-word frame slot has a nonzero first word");
    for (i = 1; i < 32UL; ++i) {
        if (frame_words[i] != 0xffffU)
            return fail("seed words were not copied with the high bit set");
    }
    for (i = 32; i < 256UL; ++i) {
        unsigned short expected;

        expected = (i % 32UL == 0) ? 0U : 0xffffU;
        if (frame_words[i] != expected)
            return fail("overlapping frame tail does not repeat the 32-word prefix");
    }
    if (g_frame_global_73498 != 1)
        return fail("frame initialization flag differs");

    /* A duplicate key increments the existing count and does not rewrite the
     * selected 512-byte slot. */
    frame_words[0] = 0x55aaU;
    if (dispatch_local_05660(0x7641UL, 5UL) != 0 ||
        g_actor_token_references[0][0] != 2 || frame_words[0] != 0x55aaU)
        return fail("duplicate-key path differs");

    /* At count zero, native code clears word zero and runs 256 self-copies of
     * words one through 256, including the next row's first word. */
    g_frame_global_73498 = 0;
    dispatch_local_05780(full_words, 1UL, 0UL);
    if (frame_words[256] != 0 || frame_words[257] != 0xaaaaU ||
        frame_words[511] != 0xaaaaU || frame_words[512] != 0xaaaaU ||
        g_frame_global_73498 != 1)
        return fail("zero-count frame branch differs");

    /* 0x2500 is the second 0x56c0 caller and passes a full 256-word count. */
    for (i = 0; i < 256UL; ++i)
        full_words[i] = (unsigned short)((i * 37UL) ^ 0x6000UL);
    dispatch_local_05780(full_words, 2UL, 256UL);
    for (i = 0; i < 256UL; ++i) {
        if (frame_words[512UL + i] != full_words[i])
            return fail("256-count raw-copy branch differs");
    }

    puts("frame token setup and zero/256 count fixture passed");
    return 0;
}
