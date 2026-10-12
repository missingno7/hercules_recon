/* Bounded host-only checks for the initialized table prefixes. */
#include <stdio.h>

typedef void (__cdecl *Eng1FrameNodeCallback)(void *node);
typedef signed long Eng1ScriptDword;

extern const Eng1FrameNodeCallback g_frame_callbacks_4d6f8_prefix[10];
extern const Eng1ScriptDword g_frame_script_58d50_prefix[9];

#define DECLARE_CALLBACK(name) void __cdecl name(void *node) { (void)node; }
DECLARE_CALLBACK(eng1_native_code_target_003f60)
DECLARE_CALLBACK(eng1_native_code_target_004010)
DECLARE_CALLBACK(eng1_native_code_target_0303d0)
DECLARE_CALLBACK(eng1_native_code_target_030430)
DECLARE_CALLBACK(eng1_native_code_target_0304f0)
DECLARE_CALLBACK(eng1_native_code_target_030a20)
DECLARE_CALLBACK(eng1_native_code_target_004c10)
DECLARE_CALLBACK(eng1_empty_c720)
DECLARE_CALLBACK(eng1_native_code_target_0310e0)

static int fail(const char *message)
{
    puts(message);
    return 1;
}

int main(void)
{
    static const Eng1ScriptDword expected_script[9] = {
        0L, 0L, 1L, 1L, 2L, 2L, 3L, 3L, -1L
    };
    unsigned long index;

    if (sizeof(void *) != 4 || sizeof(g_frame_callbacks_4d6f8_prefix) != 40 ||
        sizeof(g_frame_script_58d50_prefix) != 36)
        return fail("typed prefix extent differs");
    if (g_frame_callbacks_4d6f8_prefix[0] != eng1_native_code_target_003f60 ||
        g_frame_callbacks_4d6f8_prefix[1] != eng1_native_code_target_004010 ||
        g_frame_callbacks_4d6f8_prefix[2] != eng1_native_code_target_0303d0 ||
        g_frame_callbacks_4d6f8_prefix[3] != eng1_native_code_target_030430 ||
        g_frame_callbacks_4d6f8_prefix[4] != eng1_native_code_target_0304f0 ||
        g_frame_callbacks_4d6f8_prefix[5] != eng1_native_code_target_030a20 ||
        g_frame_callbacks_4d6f8_prefix[6] != eng1_native_code_target_004c10 ||
        g_frame_callbacks_4d6f8_prefix[7] != eng1_empty_c720 ||
        g_frame_callbacks_4d6f8_prefix[8] != eng1_native_code_target_0310e0 ||
        g_frame_callbacks_4d6f8_prefix[9] != 0)
        return fail("callback prefix order or terminal NULL differs");
    for (index = 0; index < 9; ++index) {
        if (g_frame_script_58d50_prefix[index] != expected_script[index])
            return fail("script prefix values differ");
    }

    puts("bounded frame data prefixes passed");
    return 0;
}
