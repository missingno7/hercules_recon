#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/resource_runtime_data.h"

extern int __cdecl dispatch_local_20ea0(void *, int, int, long);
typedef int (__cdecl *Bounds)(void *, int, int, long);
static HMODULE oracle;
static unsigned long cases, checks, failures;
static unsigned long blob_a[64], blob_b[64];
static unsigned short output_a[16], output_b[16];
static int expect_fault;
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; printf("FAIL case=%lu line=%d\n", cases, __LINE__); } } while (0)

static void reset(int dataset)
{
    unsigned char *b = (unsigned char *)blob_a;
    unsigned short *row;
    unsigned i, j;
    memset(blob_a, 0xa5, sizeof(blob_a));
    blob_a[3] = 52; blob_a[4] = 48; blob_a[5] = 108;
    *(unsigned short *)(b + 64) = dataset == 1 ? 0 : dataset == 2 ? 1 : 3;
    *(unsigned short *)(b + 128) = 3;
    for (i = 0; i < 4; ++i) {
        row = (unsigned short *)(b + 68 + i * 16);
        row[0] = (unsigned short)(i * 100);
        for (j = 1; j < 8; ++j)
            row[j] = (unsigned short)(dataset == 3 ? 65535 - i * 401 - j * 103 : i * 50 + j * 13);
    }
    memcpy(blob_b, blob_a, sizeof(blob_a));
    memset(output_a, 0xa5, sizeof(output_a));
    memset(output_b, 0xa5, sizeof(output_b));
    g_root_count_70ed3 = 1;
    *(signed char *)((unsigned char *)oracle + 0x70ed3) = 1;
    g_header_pointer_70d4c = (unsigned char *)blob_b + 16;
    *(unsigned char **)((unsigned char *)oracle + 0x70d4c) = b + 16;
}

static unsigned long invoke(Bounds fn, void *out, int id, int mode, long position, int *result)
{
    unsigned long fault = 0, before, after;
    __asm mov before, esp
    __try { *result = fn(out, id, mode, position); }
    __except (fault = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {}
    __asm mov after, esp
    CHECK(before == after);
    return fault;
}

static void compare(int id, int mode, long position, int alias)
{
    int a = -99, b = -99;
    unsigned long fa, fb;
    void *oa = alias < 0 ? (void *)(output_a + 1) : (void *)((unsigned char *)blob_a + alias);
    void *ob = alias < 0 ? (void *)(output_b + 1) : (void *)((unsigned char *)blob_b + alias);
    ++cases;
    fa = invoke((Bounds)((unsigned char *)oracle + 0x20ea0), oa, id, mode, position, &a);
    fb = invoke(dispatch_local_20ea0, ob, id, mode, position, &b);
    CHECK(fa == fb);
    if (expect_fault) CHECK(fa != 0);
    CHECK(a == b);
    CHECK(memcmp(blob_a, blob_b, sizeof(blob_a)) == 0);
    CHECK(memcmp(output_a, output_b, sizeof(output_a)) == 0);
}

int main(int argc, char **argv)
{
    static int modes[] = {0, 1, 24, -1};
    static long positions[] = {-1, 0, 50, 100, 200, 65535};
    int d, m, p;
    if (argc != 2) return 2;
    oracle = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (!oracle) return 3;
    for (d = 0; d < 4; ++d)
        for (m = 0; m < 4; ++m)
            for (p = 0; p < 6; ++p) {
                reset(d); compare(0, modes[m], positions[p], -1);
            }
    reset(0);
    g_header_pointer_70d4c = 0;
    *(unsigned char **)((unsigned char *)oracle + 0x70d4c) = 0;
    compare(1, 24, 50, -1);
    reset(0); g_root_count_70ed3 = -1;
    *(signed char *)((unsigned char *)oracle + 0x70ed3) = -1;
    g_header_pointer_70d4c = 0;
    *(unsigned char **)((unsigned char *)oracle + 0x70d4c) = 0;
    compare(0, 24, 50, -1);
    reset(0); g_root_count_70ed3 = 0;
    *(signed char *)((unsigned char *)oracle + 0x70ed3) = 0;
    g_header_pointer_70d4c = 0;
    *(unsigned char **)((unsigned char *)oracle + 0x70d4c) = 0;
    compare(0, 24, 50, -1);
    reset(0); compare(-1, 24, 50, -1); /* valid fixture cell before the root */
    for (p = 0; p < 4; ++p) {
        reset(0); compare(0, 24, 50, p < 2 ? 68 + p * 2 : 84 + (p - 2) * 2);
    }
    reset(2);
    *(unsigned short *)((unsigned char *)blob_a + 68) = 65535;
    *(unsigned short *)((unsigned char *)blob_b + 68) = 65535;
    expect_fault = 1;
    compare(0, 65536, 32767, -1); /* native signed division overflow */
    expect_fault = 0;
    reset(0);
    *(unsigned short *)((unsigned char *)blob_a + 84) = 0;
    *(unsigned short *)((unsigned char *)blob_b + 84) = 0;
    compare(0, 24, 0, -1);
    reset(0);
    *(unsigned short *)((unsigned char *)blob_a + 68) = 200;
    *(unsigned short *)((unsigned char *)blob_b + 68) = 200;
    compare(0, 24, 50, -1);
    reset(0); blob_a[4] |= 3; blob_b[4] |= 3;
    compare(0, 24, 50, -1); /* native relative offset rounds down to DWORD */
    printf("camera_bounds_native_cases=%lu checks=%lu failures=%lu\n", cases, checks, failures);
    FreeLibrary(oracle);
    return failures ? 1 : 0;
}
