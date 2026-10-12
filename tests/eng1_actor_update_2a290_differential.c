/* Bounded fixture for actor_update_2a290.c.
 * NATIVE executes the original loaded ENG1 code; SOURCE executes the private
 * ordinary-C candidate.  The helper spy calls through to native 0x20b50 so
 * this fixture checks 0x2a290's behavior while leaving that service opaque. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../calibration/engine_interface.h"

typedef unsigned char fixture_u8;
typedef unsigned short fixture_u16;
typedef unsigned long fixture_u32;
typedef long (__cdecl *NativeQuery)(long, long, long, int);
typedef void (__cdecl *NativeActorUpdate)(void *);

extern void __cdecl eng1_native_code_target_002a290(void *actor);
static HMODULE native_module;
static int helper_calls;
static fixture_u32 helper_result;
static long helper_x, helper_y, helper_z;
static int helper_parameter;
static int use_fixed_helper_result;
static fixture_u32 fixed_helper_result;

static fixture_u8 *native_rva(fixture_u32 rva)
{
    return (fixture_u8 *)(void *)native_module + rva;
}

long __cdecl eng1_native_dep_020b50(long x, long y, long z,
                                    int parameter)
{
    NativeQuery query;
    ++helper_calls;
    helper_x = x;
    helper_y = y;
    helper_z = z;
    helper_parameter = parameter;
    if (use_fixed_helper_result) {
        helper_result = fixed_helper_result;
        return (long)helper_result;
    }
    query = (NativeQuery)(void *)native_rva(0x20b50UL);
    helper_result = (fixture_u32)query(x, y, z, parameter);
    return (long)helper_result;
}

static void initialize_actor(fixture_u8 *actor, fixture_u32 flags,
                             int negative_coordinates)
{
    fixture_u32 i;
    for (i = 0; i < 0x128UL; ++i)
        actor[i] = (fixture_u8)(0x37U + i * 13UL + (i >> 2));

    *(fixture_u32 *)(void *)(actor + 0x00) = 0x00a00000UL;
    *(fixture_u32 *)(void *)(actor + 0x04) = 0x00800000UL;
    *(fixture_u32 *)(void *)(actor + 0x08) = 0x01400000UL;
    actor[0x25] = 0xa4U;
    *(fixture_u16 *)(void *)(actor + 0x38) = 0x0100U;
    *(fixture_u32 *)(void *)(actor + 0x54) = flags;
    if (negative_coordinates) {
        *(fixture_u32 *)(void *)(actor + 0x00) = 0xffff0000UL;
        *(fixture_u32 *)(void *)(actor + 0x04) = 0x80000000UL;
        *(fixture_u32 *)(void *)(actor + 0x08) = 0x7fff0000UL;
        *(fixture_u16 *)(void *)(actor + 0x38) = 0xfffeU;
    }
}

static void print_actor(const fixture_u8 *actor)
{
    fixture_u32 i;
    for (i = 0; i < 0x128UL; ++i)
        printf("%02x", (unsigned int)actor[i]);
}

int main(int argc, char **argv)
{
    fixture_u8 actor[0x128];
    fixture_u8 context[0x40];
    fixture_u8 *cell;
    fixture_u8 mode;

    if (argc != 6) {
        fprintf(stderr, "usage: neutralfixture.exe ENG1.DLL native|source|source-fixed|source-fixed-negative|probe mode flags result\n");
        return 2;
    }
    native_module = LoadLibraryA(argv[1]);
    if (native_module == 0) {
        fprintf(stderr, "LoadLibraryA failed: %lu\n", GetLastError());
        return 3;
    }
    mode = (fixture_u8)strtoul(argv[3], 0, 0);
    memset(context, 0, sizeof(context));
    context[0] = mode;
    cell = native_rva(0x70f24UL);
    *(fixture_u8 **)(void *)cell = context;
    g_engine_interface.context_004 = (ResourceCallbackContext *)(void *)context;

    initialize_actor(actor, (fixture_u32)strtoul(argv[4], 0, 0),
                     strcmp(argv[2], "source-fixed-negative") == 0);
    helper_calls = 0;
    helper_result = 0;
    helper_x = helper_y = helper_z = 0;
    helper_parameter = 0;
    use_fixed_helper_result = strcmp(argv[2], "source-fixed") == 0 ||
                              strcmp(argv[2], "source-fixed-negative") == 0;
    fixed_helper_result = (fixture_u32)strtoul(argv[5], 0, 0);

    if (strcmp(argv[2], "native") == 0) {
        NativeActorUpdate update;
        update = (NativeActorUpdate)(void *)native_rva(0x2a290UL);
        update(actor);
    } else if (strcmp(argv[2], "source") == 0) {
        eng1_native_code_target_002a290(actor);
    } else if (strcmp(argv[2], "source-fixed") == 0 ||
               strcmp(argv[2], "source-fixed-negative") == 0) {
        eng1_native_code_target_002a290(actor);
    } else if (strcmp(argv[2], "probe") == 0) {
        NativeQuery query;
        query = (NativeQuery)(void *)native_rva(0x20b50UL);
        helper_x = 160L;
        helper_y = 128L;
        helper_z = 320L;
        helper_parameter = 0x0100;
        helper_result = (fixture_u32)query(helper_x, helper_y, helper_z,
                                           helper_parameter);
        helper_calls = 1;
    } else {
        fprintf(stderr, "unknown fixture mode\n");
        return 2;
    }

    printf("mode=%s helper_calls=%d args=%08lx,%08lx,%08lx,%08lx result=%08lx actor=",
           argv[2], helper_calls, (fixture_u32)helper_x,
           (fixture_u32)helper_y, (fixture_u32)helper_z,
           (fixture_u32)helper_parameter, helper_result);
    print_actor(actor);
    printf("\n");
    return 0;
}
