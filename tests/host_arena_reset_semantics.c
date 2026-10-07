#include <stdio.h>
#include <stdarg.h>
#include "../calibration/host_arena_reset.c"
extern void __cdecl reinitialize_pool0(U8 *new_arena);
HostPoolState g_host_pool;
const char g_fmt_reinit_pool[] = "ReInitheap Heap%d @ 0x%x,size %d\n";

static U32 coalesce_calls, coalesce_id;
static U32 diagnostic_calls, diagnostic_id, diagnostic_bytes;
static U8 *diagnostic_arena;
static const char *diagnostic_format;

void __cdecl coalesce_pool(U32 pool_id)
{
    ++coalesce_calls;
    coalesce_id = pool_id;
}

void __cdecl host_diagnostic(const char *format, ...)
{
    va_list args;
    diagnostic_format = format;
    ++diagnostic_calls;
    va_start(args, format);
    diagnostic_id = va_arg(args, U32);
    diagnostic_arena = va_arg(args, U8 *);
    diagnostic_bytes = va_arg(args, U32);
    va_end(args);
}

static U32 checks, fails;
#define CHECK(x) do { ++checks; if (!(x)) ++fails; } while (0)

int main(void)
{
    PoolWord no_shift[32];
    PoolWord shifted[32];
    U32 i;

    for (i = 0; i < 32; ++i) {
        no_shift[i].descriptor = 0x13572468UL;
        shifted[i].descriptor = 0x24681357UL;
    }
    g_host_pool.saved_arena = (U8 *)no_shift;
    g_host_pool.saved_bytes = 128;
    g_host_pool.starts[0] = no_shift;
    g_host_pool.ends[0] = no_shift + 8;
    g_host_pool.bulk_first_header = no_shift;
    g_host_pool.bulk_first_words = 5;
    reinitialize_pool0(0);
    CHECK(g_host_pool.saved_arena == (U8 *)no_shift);
    CHECK(g_host_pool.saved_bytes == 128);
    CHECK(g_host_pool.starts[0] == no_shift);
    CHECK(g_host_pool.ends[0] == no_shift + 30);
    CHECK(no_shift[0].descriptor == (POOL_FREE | 27));
    CHECK(no_shift[1].descriptor == 0x13572468UL);
    CHECK(coalesce_calls == 1 && coalesce_id == 0);
    CHECK(diagnostic_calls == 1 && diagnostic_format == g_fmt_reinit_pool);
    CHECK(diagnostic_id == 0 && diagnostic_arena == (U8 *)no_shift && diagnostic_bytes == 128);

    g_host_pool.saved_arena = (U8 *)shifted;
    g_host_pool.saved_bytes = 128;
    g_host_pool.starts[0] = shifted;
    g_host_pool.ends[0] = shifted + 12;
    g_host_pool.bulk_first_header = shifted;
    g_host_pool.bulk_first_words = 7;
    coalesce_calls = diagnostic_calls = 0;
    reinitialize_pool0((U8 *)shifted + 4 * 4);
    CHECK(g_host_pool.saved_arena == (U8 *)shifted + 16);
    CHECK(g_host_pool.saved_bytes == 112);
    CHECK(g_host_pool.starts[0] == shifted + 4);
    CHECK(g_host_pool.ends[0] == shifted + 30);
    CHECK(shifted[4].descriptor == (POOL_FREE | 21));
    CHECK(shifted[0].descriptor == 0x24681357UL);
    CHECK(coalesce_calls == 1 && coalesce_id == 0);
    CHECK(diagnostic_calls == 1 && diagnostic_format == g_fmt_reinit_pool);
    CHECK(diagnostic_id == 0 && diagnostic_arena == (U8 *)shifted + 16 && diagnostic_bytes == 112);

    printf("checks=%lu failures=%lu\n", checks, fails);
    return (int)fails;
}
