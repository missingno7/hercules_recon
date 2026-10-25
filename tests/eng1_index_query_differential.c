#include <windows.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/actor_pipeline_storage_views.h"
#include "../src/eng1/query_2a170.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef long s32;
typedef u32 (__cdecl *Query2a170)(u32, u32, u32, u32);
typedef u32 (__cdecl *Fallback20d30)(s32, s32, s32, u32);
typedef int (__cdecl *Point2a810)(u32, u32, u32, const void *);
typedef struct TestBox TestBox;
struct TestBox { short mn[3]; short mx[3]; };
typedef struct TestActor TestActor;
struct TestActor { u8 prefix[0xd8]; u32 return_d8; };


extern u32 __cdecl dispatch_local_2a170(u32, u32, u32, u32);

static HMODULE oracle;
static unsigned int cases, checks, failures;
static jmp_buf incomplete_target;
static int incomplete_armed;
static u32 incomplete_rva;
static char events[256];
static unsigned int event_count;
static unsigned int bounds_calls;
static unsigned int point_calls;
static unsigned int fallback_calls;
static TestActor actor_a, actor_b;
static int actor_a_bounds_success;
static int actor_b_bounds_success;
static int extend_count_after_a;
static int shrink_count_after_a;
static int grow_count_out_of_prefix_after_a;
static int replace_second_entry_after_a;
static int fallback_use_native20d30;
static u32 fallback_result;
static s32 fallback_args[3];
static u32 fallback_step;
static const Query2a170Bounds *last_bounds_from_builder;
static const Query2a170Bounds *last_bounds_to_point;

static u8 *native_cell(u32 rva)
{ return (u8 *)oracle + rva; }

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

static void reset_spies(void)
{
    event_count = 0;
    bounds_calls = 0;
    point_calls = 0;
    fallback_calls = 0;
    memset(events, 0, sizeof(events));
    actor_a_bounds_success = 1;
    actor_b_bounds_success = 1;
    extend_count_after_a = 0;
    shrink_count_after_a = 0;
    grow_count_out_of_prefix_after_a = 0;
    replace_second_entry_after_a = 0;
    fallback_use_native20d30 = 0;
    fallback_result = 0UL;
    fallback_args[0] = fallback_args[1] = fallback_args[2] = 0;
    fallback_step = 0UL;
    last_bounds_from_builder = 0;
    last_bounds_to_point = 0;
}

static void set_box(Query2a170Bounds *box, short min_x, short min_y,
                    short min_z, short max_x, short max_y, short max_z)
{
    box->mn[0] = (u16)min_x;
    box->mn[1] = (u16)min_y;
    box->mn[2] = (u16)min_z;
    box->mx[0] = (u16)max_x;
    box->mx[1] = (u16)max_y;
    box->mx[2] = (u16)max_z;
}

static void fill_actor_box(void *actor, Query2a170Bounds *box)
{
    if (actor == &actor_a)
        set_box(box, 20, 0, 0, 30, 20, 20);
    else
        set_box(box, 0, 0, 0, 10, 20, 5);
}

int __cdecl eng1_native_code_target_002a880(
    void *actor, Query2a170Bounds *bounds, u32 mode)
{
    ++bounds_calls;
    events[event_count++] = 'B';
    last_bounds_from_builder = bounds;
    if (mode != 0UL) {
        check(0, "native 0x2a170 passes literal zero mode to 0x2a880");
    }
    if (actor == &actor_a) {
        if (extend_count_after_a)
            g_actor_worklist_71b20[0] = 2UL;
        if (shrink_count_after_a)
            g_actor_worklist_71b20[0] = 0UL;
        if (grow_count_out_of_prefix_after_a)
            g_actor_worklist_71b20[0] = 120UL;
        if (replace_second_entry_after_a)
            g_actor_worklist_71b20[2] = (u32)(void *)&actor_b;
        if (!actor_a_bounds_success)
            return 0;
    } else if (actor == &actor_b) {
        if (!actor_b_bounds_success)
            return 0;
    } else {
        check(0, "2a170 visits only the configured worklist actor pointers");
        return 0;
    }
    fill_actor_box(actor, bounds);
    return 1;
}



static u32 __cdecl native_20d30(s32 x, s32 y, s32 z, u32 step)
{
    Fallback20d30 fn = (Fallback20d30)(void *)native_cell(0x20d30UL);
    return fn(x, y, z, step);
}

u32 __cdecl eng1_native_code_target_0020d30(
    s32 x, s32 y, s32 z, u32 step)
{
    ++fallback_calls;
    events[event_count++] = 'F';
    fallback_args[0] = x;
    fallback_args[1] = y;
    fallback_args[2] = z;
    fallback_step = step;
    if (fallback_use_native20d30)
        return native_20d30(x, y, z, step);
    return fallback_result;
}

void __cdecl eng1_incomplete_behavior(u32 rva, const char *reason)
{
    (void)reason;
    incomplete_rva = rva;
    if (incomplete_armed)
        longjmp(incomplete_target, 1);
    printf("unexpected incomplete path at %08lx\n", rva);
    ExitProcess(0xe0484731UL);
}

static u32 fixed_from_integer(s32 value)
{ return (u32)(value * 65536L); }

static void native_empty_worklist_differential(void)
{
    Query2a170 native_fn = (Query2a170)(void *)native_cell(0x2a170UL);
    u32 x = 0xffff8000UL;
    u32 y = 0x00028000UL;
    u32 z = 0x8001ffffUL;
    u32 step = 7UL;
    u32 native_result;
    u32 source_result;

    reset_spies();
    *(volatile u32 *)(void *)native_cell(0x71b20UL) = 0UL;
    *(volatile u8 *)(void *)native_cell(0x70d01UL) = 0xffU;
    *(volatile u32 *)(void *)native_cell(0x6fedcUL) = 0xdeadbeefUL;
    g_actor_worklist_71b20[0] = 0UL;
    g_eng1_spatial_match_actor_6fedc = 0x10203040UL;
    fallback_use_native20d30 = 1;

    native_result = native_fn(x, y, z, step);
    source_result = dispatch_local_2a170(x, y, z, step);
    check(native_result == source_result,
          "native and candidate agree on a safely controlled empty-worklist fallback");
    check(native_result == 0UL,
          "0x20d30 sentinel mode returns low-WORD zero on the fallback path");
    check(*(volatile u32 *)(void *)native_cell(0x6fedcUL) == 0UL &&
          g_eng1_spatial_match_actor_6fedc == 0UL,
          "native and candidate clear their 0x6fedc DWORD on entry");
    check(fallback_calls == 1U && fallback_args[0] == -1L &&
          fallback_args[1] == 2L && fallback_args[2] == -32767L &&
          fallback_step == step,
          "fallback receives sign-extended upper-WORD coordinates and original step");
    check(bounds_calls == 0U && point_calls == 0U,
          "empty-worklist path skips both actor dependencies");
    ++cases;
}

static void source_live_count_extension_hit(void)
{
    u32 result;
    reset_spies();
    actor_a.return_d8 = 0x11112222UL;
    actor_b.return_d8 = 0x89abcdefUL;
    g_actor_worklist_71b20[0] = 1UL;
    g_actor_worklist_71b20[1] = (u32)(void *)&actor_a;
    g_actor_worklist_71b20[2] = (u32)(void *)&actor_b;
    extend_count_after_a = 1;

    result = dispatch_local_2a170(fixed_from_integer(5),
                                    fixed_from_integer(10),
                                    fixed_from_integer(3), 0UL);
    check(result == actor_b.return_d8,
          "first hit returns the matched actor's full DWORD at +0xd8");
    check(g_eng1_spatial_match_actor_6fedc == (u32)(void *)&actor_b,
          "first hit stores the matched actor pointer at the 0x6fedc view");
    check(bounds_calls == 2U && point_calls == 2U && fallback_calls == 0U,
          "count reload observes extension performed by the first bounds provider");
    check(events[0] == 'B' && events[1] == 'P' &&
          events[2] == 'B' && events[3] == 'P',
          "each actor's bounds provider precedes its point-in-box test");
    check(last_bounds_from_builder == last_bounds_to_point,
          "point test receives the exact 12-byte local written by bounds provider");
    ++cases;
}

static void source_live_count_shrink_fallback(void)
{
    u32 result;
    reset_spies();
    actor_a.return_d8 = 0x11112222UL;
    actor_b.return_d8 = 0x33334444UL;
    g_actor_worklist_71b20[0] = 2UL;
    g_actor_worklist_71b20[1] = (u32)(void *)&actor_a;
    g_actor_worklist_71b20[2] = (u32)(void *)&actor_b;
    actor_a_bounds_success = 0;
    shrink_count_after_a = 1;
    fallback_result = 0x12345678UL;

    result = dispatch_local_2a170(fixed_from_integer(5),
                                    fixed_from_integer(10),
                                    fixed_from_integer(3), 0x55aaUL);
    check(result == 0x5678UL,
          "no-hit fallback returns only the low WORD of 0x20d30 EAX");
    check(g_eng1_spatial_match_actor_6fedc == 0UL,
          "no-hit fallback leaves entry-cleared match pointer at zero");
    check(bounds_calls == 1U && point_calls == 0U && fallback_calls == 1U,
          "live count shrink prevents reading the second list entry");
    check(events[0] == 'B' && events[1] == 'F',
          "false bounds result skips point test before fallback");
    check(fallback_args[0] == 5L && fallback_args[1] == 10L &&
          fallback_args[2] == 3L && fallback_step == 0x55aaUL,
          "fallback receives each signed integer coordinate in x/y/z order");
    ++cases;
}

static void source_live_entry_reload(void)
{
    u32 result;
    reset_spies();
    actor_a.return_d8 = 0x11112222UL;
    actor_b.return_d8 = 0x89abcdefUL;
    g_actor_worklist_71b20[0] = 2UL;
    g_actor_worklist_71b20[1] = (u32)(void *)&actor_a;
    g_actor_worklist_71b20[2] = 0x76543210UL;
    actor_a_bounds_success = 0;
    replace_second_entry_after_a = 1;

    result = dispatch_local_2a170(fixed_from_integer(5),
                                    fixed_from_integer(10),
                                    fixed_from_integer(3), 0UL);
    check(result == actor_b.return_d8,
          "second worklist pointer is read live after the first provider returns");
    check(bounds_calls == 2U && point_calls == 1U && fallback_calls == 0U,
          "replacement entry skips no list slot and reaches normal hit test");
    check(events[0] == 'B' && events[1] == 'B' && events[2] == 'P',
          "reloaded second entry preserves provider and point-test order");
    check(g_eng1_spatial_match_actor_6fedc == (u32)(void *)&actor_b,
          "live entry replacement stores the replacement actor on hit");
    ++cases;
}

static void native_negative_count_differential(void)
{
    Query2a170 native_fn = (Query2a170)(void *)native_cell(0x2a170UL);
    u32 x = 0xfffe0000UL;
    u32 y = 0x80000000UL;
    u32 z = 0x00018000UL;
    u32 step = 9UL;
    u32 native_result;
    u32 source_result;

    reset_spies();
    *(volatile u32 *)(void *)native_cell(0x71b20UL) = 0xffffffffUL;
    *(volatile u8 *)(void *)native_cell(0x70d01UL) = 0xffU;
    *(volatile u32 *)(void *)native_cell(0x6fedcUL) = 0x10203040UL;
    g_actor_worklist_71b20[0] = 0xffffffffUL;
    g_eng1_spatial_match_actor_6fedc = 0x50607080UL;
    fallback_use_native20d30 = 1;

    native_result = native_fn(x, y, z, step);
    source_result = dispatch_local_2a170(x, y, z, step);
    check(native_result == source_result,
          "native and candidate agree when signed worklist count is negative");
    check(native_result == 0UL,
          "negative count takes the safe 0x20d30 sentinel fallback");
    check(*(volatile u32 *)(void *)native_cell(0x6fedcUL) == 0UL &&
          g_eng1_spatial_match_actor_6fedc == 0UL,
          "negative count still clears each implementation's match pointer");
    check(bounds_calls == 0U && point_calls == 0U && fallback_calls == 1U,
          "negative signed count skips worklist dependencies and falls back");
    check(fallback_args[0] == -2L && fallback_args[1] == -32768L &&
          fallback_args[2] == 1L && fallback_step == step,
          "negative-count fallback keeps signed upper-WORD argument mapping");
    ++cases;
}

static void source_high_count_first_hit(void)
{
    u32 result;
    reset_spies();
    actor_b.return_d8 = 0x89abcdefUL;
    g_actor_worklist_71b20[0] = 120UL;
    g_actor_worklist_71b20[1] = (u32)(void *)&actor_b;

    result = dispatch_local_2a170(fixed_from_integer(5),
                                    fixed_from_integer(10),
                                    fixed_from_integer(3), 0UL);
    check(result == actor_b.return_d8 &&
          g_eng1_spatial_match_actor_6fedc == (u32)(void *)&actor_b,
          "count above the measured span can still return on an owned first hit");
    check(bounds_calls == 1U && point_calls == 1U && fallback_calls == 0U,
          "high count does not preempt supported work before an out-of-prefix read");
    ++cases;
}

static void prefix_guards(void)
{
    int jumped;
    reset_spies();
    g_actor_worklist_71b20[0] = 120UL;
    actor_a_bounds_success = 0;
    {
        unsigned int i;
        for (i = 1; i < 120U; ++i)
            g_actor_worklist_71b20[i] = (u32)(void *)&actor_a;
    }
    incomplete_armed = 1;
    jumped = setjmp(incomplete_target);
    if (jumped == 0) {
        (void)dispatch_local_2a170(fixed_from_integer(40), 0UL, 0UL, 0UL);
        check(0, "the first unsupported worklist entry must fail visibly");
    } else {
        check(incomplete_rva == 0x71b20UL,
              "out-of-prefix entry reports the worklist owner RVA");
    }
    check(bounds_calls == 119U && point_calls == 0U && fallback_calls == 0U,
          "all 119 owned list entries are processed before the next read is guarded");

    reset_spies();
    g_actor_worklist_71b20[0] = 1UL;
    {
        unsigned int i;
        for (i = 1; i < 120U; ++i)
            g_actor_worklist_71b20[i] = (u32)(void *)&actor_a;
    }
    actor_a_bounds_success = 0;
    grow_count_out_of_prefix_after_a = 1;
    jumped = setjmp(incomplete_target);
    if (jumped == 0) {
        (void)dispatch_local_2a170(0UL, 0UL, 0UL, 0UL);
        check(0, "callback-grown count outside the measured prefix must fail visibly");
    } else {
        check(incomplete_rva == 0x71b20UL,
              "callback-grown count reports the worklist owner RVA");
    }
    check(bounds_calls == 119U && point_calls == 0U && fallback_calls == 0U,
          "callback-grown count reads owned slots and guards before slot 120");
    incomplete_armed = 0;
    ++cases;
}

static void point_box_native_cases(void)
{
    Point2a810 native_fn = (Point2a810)(void *)native_cell(0x2a810UL);
    TestBox box;
    struct PointCase { s32 x, y, z; int expected; } tests[7];
    unsigned int i;

    box.mn[0] = -3; box.mn[1] = -2; box.mn[2] = -1;
    box.mx[0] = 4; box.mx[1] = 5; box.mx[2] = 6;
    tests[0].x = 0; tests[0].y = 0; tests[0].z = 0; tests[0].expected = 1;
    tests[1].x = -3; tests[1].y = -2; tests[1].z = -1; tests[1].expected = 1;
    tests[2].x = 4; tests[2].y = 5; tests[2].z = 6; tests[2].expected = 1;
    tests[3].x = 5; tests[3].y = 0; tests[3].z = 0; tests[3].expected = 0;
    tests[4].x = 0; tests[4].y = -3; tests[4].z = 0; tests[4].expected = 0;
    tests[5].x = 0; tests[5].y = 0; tests[5].z = 7; tests[5].expected = 0;
    tests[6].x = -1; tests[6].y = 3; tests[6].z = 4; tests[6].expected = 1;

    for (i = 0; i < 7U; ++i) {
        u32 x = fixed_from_integer(tests[i].x);
        u32 y = fixed_from_integer(tests[i].y);
        u32 z = fixed_from_integer(tests[i].z);
        int source_result = point_in_box_2a810(x, y, z, (const Query2a170Bounds *)(void *)&box);
        int native_result = native_fn(x, y, z, &box);
        check(source_result == tests[i].expected,
              "reused 2a810 source view matches the reviewed box predicate");
        check(native_result == source_result,
              "reused 2a810 source view agrees with native boundary/control result");
    }
    ++cases;
}

int main(int argc, char **argv)
{
    unsigned int native_cases, native_checks, box_checks, spy_cases, spy_checks;
    if (argc != 2) return 2;
    oracle = LoadLibraryA(argv[1]);
    if (oracle == 0) return 3;
    native_empty_worklist_differential();
    native_negative_count_differential();
    native_cases=cases; native_checks=checks;
    point_box_native_cases();
    box_checks=checks-native_checks;
    source_live_count_extension_hit();
    source_live_count_shrink_fallback();
    source_live_entry_reload();
    source_high_count_first_hit();
    spy_cases=cases-native_cases-1; spy_checks=checks-native_checks-box_checks;
    prefix_guards();
    printf("index_query_native_cases=%u checks=%u failures=%u\n",native_cases,native_checks,failures);
    printf("box_native_cases=7 checks=%u failures=%u\n",box_checks,failures);
    printf("index_query_source_spy_cases=%u checks=%u failures=%u\n",spy_cases,spy_checks,failures);
    printf("index_query_source_guard_cases=2 checks=%u failures=%u\n",checks-native_checks-box_checks-spy_checks,failures);
    FreeLibrary(oracle);
    return failures ? 1 : 0;
}

int __cdecl fixture_point_in_box(unsigned long x,unsigned long y,unsigned long z,const Query2a170Bounds *box)
{ ++point_calls; last_bounds_to_point=box; events[event_count++]='P'; return point_in_box_2a810(x,y,z,box); }
