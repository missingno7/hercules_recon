#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef unsigned int fixture_u32;

static const fixture_u32 kTableWords = 250UL;
static const fixture_u32 kSignature = 0x5ac00cacUL;
static const fixture_u32 kSelectedSlot = 89UL;
static const fixture_u32 kReplacement = 0xf17e0089UL;

/* The data and source pointer are initialized by the image loader. */
static fixture_u32 g_original_table[250] = {
    0x5ac00cacUL, 0xA5000001UL, 0xA5000002UL, 0xA5000003UL, 0xA5000004UL,
    0xA5000005UL, 0xA5000006UL, 0xA5000007UL, 0xA5000008UL, 0xA5000009UL,
    0xA500000aUL, 0xA500000bUL, 0xA500000cUL, 0xA500000dUL, 0xA500000eUL,
    0xA500000fUL, 0xA5000010UL, 0xA5000011UL, 0xA5000012UL, 0xA5000013UL,
    0xA5000014UL, 0xA5000015UL, 0xA5000016UL, 0xA5000017UL, 0xA5000018UL,
    0xA5000019UL, 0xA500001aUL, 0xA500001bUL, 0xA500001cUL, 0xA500001dUL,
    0xA500001eUL, 0xA500001fUL, 0xA5000020UL, 0xA5000021UL, 0xA5000022UL,
    0xA5000023UL, 0xA5000024UL, 0xA5000025UL, 0xA5000026UL, 0xA5000027UL,
    0xA5000028UL, 0xA5000029UL, 0xA500002aUL, 0xA500002bUL, 0xA500002cUL,
    0xA500002dUL, 0xA500002eUL, 0xA500002fUL, 0xA5000030UL, 0xA5000031UL,
    0xA5000032UL, 0xA5000033UL, 0xA5000034UL, 0xA5000035UL, 0xA5000036UL,
    0xA5000037UL, 0xA5000038UL, 0xA5000039UL, 0xA500003aUL, 0xA500003bUL,
    0xA500003cUL, 0xA500003dUL, 0xA500003eUL, 0xA500003fUL, 0xA5000040UL,
    0xA5000041UL, 0xA5000042UL, 0xA5000043UL, 0xA5000044UL, 0xA5000045UL,
    0xA5000046UL, 0xA5000047UL, 0xA5000048UL, 0xA5000049UL, 0xA500004aUL,
    0xA500004bUL, 0xA500004cUL, 0xA500004dUL, 0xA500004eUL, 0xA500004fUL,
    0xA5000050UL, 0xA5000051UL, 0xA5000052UL, 0xA5000053UL, 0xA5000054UL,
    0xA5000055UL, 0xA5000056UL, 0xA5000057UL, 0xA5000058UL, 0xA5000059UL,
    0xA500005aUL, 0xA500005bUL, 0xA500005cUL, 0xA500005dUL, 0xA500005eUL,
    0xA500005fUL, 0xA5000060UL, 0xA5000061UL, 0xA5000062UL, 0xA5000063UL,
    0xA5000064UL, 0xA5000065UL, 0xA5000066UL, 0xA5000067UL, 0xA5000068UL,
    0xA5000069UL, 0xA500006aUL, 0xA500006bUL, 0xA500006cUL, 0xA500006dUL,
    0xA500006eUL, 0xA500006fUL, 0xA5000070UL, 0xA5000071UL, 0xA5000072UL,
    0xA5000073UL, 0xA5000074UL, 0xA5000075UL, 0xA5000076UL, 0xA5000077UL,
    0xA5000078UL, 0xA5000079UL, 0xA500007aUL, 0xA500007bUL, 0xA500007cUL,
    0xA500007dUL, 0xA500007eUL, 0xA500007fUL, 0xA5000080UL, 0xA5000081UL,
    0xA5000082UL, 0xA5000083UL, 0xA5000084UL, 0xA5000085UL, 0xA5000086UL,
    0xA5000087UL, 0xA5000088UL, 0xA5000089UL, 0xA500008aUL, 0xA500008bUL,
    0xA500008cUL, 0xA500008dUL, 0xA500008eUL, 0xA500008fUL, 0xA5000090UL,
    0xA5000091UL, 0xA5000092UL, 0xA5000093UL, 0xA5000094UL, 0xA5000095UL,
    0xA5000096UL, 0xA5000097UL, 0xA5000098UL, 0xA5000099UL, 0xA500009aUL,
    0xA500009bUL, 0xA500009cUL, 0xA500009dUL, 0xA500009eUL, 0xA500009fUL,
    0xA50000a0UL, 0xA50000a1UL, 0xA50000a2UL, 0xA50000a3UL, 0xA50000a4UL,
    0xA50000a5UL, 0xA50000a6UL, 0xA50000a7UL, 0xA50000a8UL, 0xA50000a9UL,
    0xA50000aaUL, 0xA50000abUL, 0xA50000acUL, 0xA50000adUL, 0xA50000aeUL,
    0xA50000afUL, 0xA50000b0UL, 0xA50000b1UL, 0xA50000b2UL, 0xA50000b3UL,
    0xA50000b4UL, 0xA50000b5UL, 0xA50000b6UL, 0xA50000b7UL, 0xA50000b8UL,
    0xA50000b9UL, 0xA50000baUL, 0xA50000bbUL, 0xA50000bcUL, 0xA50000bdUL,
    0xA50000beUL, 0xA50000bfUL, 0xA50000c0UL, 0xA50000c1UL, 0xA50000c2UL,
    0xA50000c3UL, 0xA50000c4UL, 0xA50000c5UL, 0xA50000c6UL, 0xA50000c7UL,
    0xA50000c8UL, 0xA50000c9UL, 0xA50000caUL, 0xA50000cbUL, 0xA50000ccUL,
    0xA50000cdUL, 0xA50000ceUL, 0xA50000cfUL, 0xA50000d0UL, 0xA50000d1UL,
    0xA50000d2UL, 0xA50000d3UL, 0xA50000d4UL, 0xA50000d5UL, 0xA50000d6UL,
    0xA50000d7UL, 0xA50000d8UL, 0xA50000d9UL, 0xA50000daUL, 0xA50000dbUL,
    0xA50000dcUL, 0xA50000ddUL, 0xA50000deUL, 0xA50000dfUL, 0xA50000e0UL,
    0xA50000e1UL, 0xA50000e2UL, 0xA50000e3UL, 0xA50000e4UL, 0xA50000e5UL,
    0xA50000e6UL, 0xA50000e7UL, 0xA50000e8UL, 0xA50000e9UL, 0xA50000eaUL,
    0xA50000ebUL, 0xA50000ecUL, 0xA50000edUL, 0xA50000eeUL, 0xA50000efUL,
    0xA50000f0UL, 0xA50000f1UL, 0xA50000f2UL, 0xA50000f3UL, 0xA50000f4UL,
    0xA50000f5UL, 0xA50000f6UL, 0xA50000f7UL, 0xA50000f8UL, 0xA50000f9UL
};
static fixture_u32 *g_source_table = g_original_table;
static fixture_u32 g_first_copy[250];
static fixture_u32 g_after_copy[250];
static int g_constructor_ran = 0;
static fixture_u32 *g_pointer_at_constructor = 0;

extern "C" fixture_u32 ** WINAPI
FixtureGetEngineTablePointerSlotV7(void)
{
    return &g_source_table;
}

class CaptureBeforeMain
{
public:
    CaptureBeforeMain()
    {
        fixture_u32 i;
        g_constructor_ran = 1;
        g_pointer_at_constructor = g_source_table;
        if (g_source_table != 0) {
            for (i = 0; i < kTableWords; ++i)
                g_first_copy[i] = g_source_table[i];
        }
    }
};

static CaptureBeforeMain g_capture_before_main;

typedef int (__cdecl *FixtureIdentityFn)(void);
typedef int (__cdecl *FixtureStateFn)(void);
extern "C" __declspec(dllimport) HRESULT WINAPI
DirectDrawCreate(void *, void **, void *);
static int is_expected_sibling(const char *exe_path, const char *dll_path)
{
    const char *exe_slash;
    const char *dll_slash;
    size_t exe_directory_length;
    size_t dll_directory_length;

    exe_slash = strrchr(exe_path, '\\');
    dll_slash = strrchr(dll_path, '\\');
    if (exe_slash == 0 || dll_slash == 0)
        return 0;
    exe_directory_length = (size_t)(exe_slash - exe_path);
    dll_directory_length = (size_t)(dll_slash - dll_path);
    if (exe_directory_length != dll_directory_length)
        return 0;
    if (_strnicmp(exe_path, dll_path, exe_directory_length) != 0)
        return 0;
    if (_stricmp(dll_slash + 1, "ddraw.dll") != 0)
        return 0;
    return 1;
}

static int original_unchanged(void)
{
    fixture_u32 i;
    for (i = 0; i < kTableWords; ++i) {
        fixture_u32 expected = (i == 0) ? kSignature :
            (fixture_u32)(0xA5000000UL | i);
        if (g_original_table[i] != expected)
            return 0;
    }
    return 1;
}

static int copy_matches_source_except_selected(const fixture_u32 *copy)
{
    fixture_u32 i;
    for (i = 0; i < kTableWords; ++i) {
        fixture_u32 expected = (i == kSelectedSlot) ? kReplacement :
            g_original_table[i];
        if (copy[i] != expected)
            return 0;
    }
    return 1;
}

int main(void)
{
    HMODULE ddraw;
    char exe_path[MAX_PATH];
    char ddraw_path[MAX_PATH];
    DWORD exe_length;
    DWORD ddraw_length;
    FixtureIdentityFn identity;
    FixtureStateFn state;
    fixture_u32 **source_slot;
    fixture_u32 *pointer_before_call;
    fixture_u32 *pointer_after_call;
    void *mock_ddraw = 0;
    HRESULT hr;
    int expected_early;
    int state_before;
    int state_after;
    int fixture_guard;
    int first_copy_valid;
    int after_copy_valid;
    int original_ok;
    fixture_u32 i;

#if defined(EXPECT_EARLY_ATTACH)
    expected_early = 1;
#else
    expected_early = 0;
#endif

    exe_path[0] = '\0';
    ddraw_path[0] = '\0';
    exe_length = GetModuleFileNameA(0, exe_path, MAX_PATH);
    ddraw = GetModuleHandleA("ddraw.dll");
    ddraw_length = ddraw ? GetModuleFileNameA(ddraw, ddraw_path, MAX_PATH) : 0;
    identity = ddraw ? (FixtureIdentityFn)GetProcAddress(
        ddraw, "FixtureMockIdentityV7") : 0;
    state = ddraw ? (FixtureStateFn)GetProcAddress(
        ddraw, "FixtureGetMockStateV7") : 0;
    source_slot = FixtureGetEngineTablePointerSlotV7();
    fixture_guard = exe_length != 0 && exe_length < MAX_PATH &&
        ddraw_length != 0 && ddraw_length < MAX_PATH && ddraw != 0 &&
        identity != 0 && state != 0 &&
        source_slot != 0 && *source_slot != 0 &&
        is_expected_sibling(exe_path, ddraw_path) &&
        identity() == 0x00dd7a51 &&
        state() == (expected_early ? 2 : 1);
    if (!fixture_guard) {
        fprintf(stderr,
            "DIAGNOSTIC: refusing mock call; expected sibling source-built "
            "ddraw.dll and fixture-only V7 identity export (state=%d).\n",
            state ? state() : -999);
        return 31;
    }

    state_before = state();
    pointer_before_call = *source_slot;
    hr = DirectDrawCreate(0, &mock_ddraw, 0);
    state_after = state();
    pointer_after_call = *source_slot;

    for (i = 0; i < kTableWords; ++i)
        g_after_copy[i] = pointer_after_call[i];

    original_ok = original_unchanged();
    if (expected_early) {
        first_copy_valid = g_constructor_ran &&
            g_pointer_at_constructor != g_original_table &&
            g_first_copy[0] == kSignature &&
            copy_matches_source_except_selected(g_first_copy);
        after_copy_valid = first_copy_valid &&
            pointer_before_call == g_pointer_at_constructor &&
            pointer_after_call == pointer_before_call &&
            state_before == 2 && state_after == 2 &&
            copy_matches_source_except_selected(g_after_copy);
    } else {
        first_copy_valid = g_constructor_ran &&
            g_pointer_at_constructor == g_original_table;
        for (i = 0; i < kTableWords && first_copy_valid; ++i) {
            fixture_u32 expected = (i == 0) ? kSignature :
                (fixture_u32)(0xA5000000UL | i);
            if (g_first_copy[i] != expected)
                first_copy_valid = 0;
        }
        after_copy_valid = first_copy_valid &&
            pointer_before_call == g_original_table &&
            pointer_after_call != g_original_table &&
            state_before == 1 && state_after == 2 &&
            copy_matches_source_except_selected(g_after_copy);
    }

    printf("MODE=%s\n", expected_early ? "EARLY_ATTACH" : "DEFERRED_API");
    printf("EXE_PATH=%s\nDDRAW_PATH=%s\n", exe_path, ddraw_path);
    printf("FIXTURE_GUARD=%d\nIDENTITY=0x%08lx\n", fixture_guard,
        (unsigned long)identity());
    printf("CTOR_RAN=%d\n", g_constructor_ran);
    printf("DLL_STATE_BEFORE_API=%d\nDLL_STATE_AFTER_API=%d\n",
        state_before, state_after);
    printf("PTR_IS_ORIGINAL_AT_CTOR=%d\nPTR_IS_ORIGINAL_BEFORE_API=%d\n"
           "PTR_IS_ORIGINAL_AFTER_API=%d\n",
        g_pointer_at_constructor == g_original_table,
        pointer_before_call == g_original_table,
        pointer_after_call == g_original_table);
    printf("FIRST_COPY_SIG=0x%08lx\nFIRST_COPY_WORD1=0x%08lx\n"
           "FIRST_COPY_SLOT89=0x%08lx\nFIRST_COPY_WORD90=0x%08lx\n"
           "FIRST_COPY_WORD249=0x%08lx\n",
        (unsigned long)g_first_copy[0], (unsigned long)g_first_copy[1],
        (unsigned long)g_first_copy[89], (unsigned long)g_first_copy[90],
        (unsigned long)g_first_copy[249]);
    printf("POST_COPY_SIG=0x%08lx\nPOST_COPY_WORD1=0x%08lx\n"
           "POST_COPY_SLOT89=0x%08lx\nPOST_COPY_WORD90=0x%08lx\n"
           "POST_COPY_WORD249=0x%08lx\n",
        (unsigned long)g_after_copy[0], (unsigned long)g_after_copy[1],
        (unsigned long)g_after_copy[89], (unsigned long)g_after_copy[90],
        (unsigned long)g_after_copy[249]);
    printf("ORIGINAL_SLOT89=0x%08lx\nMOCK_HRESULT=0x%08lx\n",
        (unsigned long)g_original_table[89], (unsigned long)hr);
    printf("FIRST_COPY_VALID=%d\nPOST_COPY_VALID=%d\n"
           "ORIGINAL_UNCHANGED=%d\nOUT_POINTER_NULL=%d\n",
        first_copy_valid, after_copy_valid, original_ok, mock_ddraw == 0);

    if (!g_constructor_ran || !first_copy_valid || !after_copy_valid ||
        !original_ok || mock_ddraw != 0 || (unsigned long)hr != 0x80004001UL)
        return 32;
    return 0;
}
