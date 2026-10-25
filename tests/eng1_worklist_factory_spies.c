#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include "../src/eng1/worklist_factory_types.h"
#include "../src/eng1/resource_runtime_data.h"

ActorNormal *g_normal_actor_pool;
ActorMiddle *g_middle_actor_pool;
ActorCore *g_alternate_actor_pool;
unsigned short g_normal_cursor_primary, g_normal_limit_primary;
unsigned short g_normal_limit_wrap, g_normal_cursor_alternate;
unsigned short g_normal_limit_wrap_start;
unsigned short g_alternate_cursor_primary, g_alternate_limit_primary;
unsigned short g_alternate_limit_wrap, g_alternate_cursor_alternate;
unsigned short g_alternate_limit_wrap_start;
unsigned short g_middle_cursor_primary, g_middle_limit_primary;
unsigned short g_middle_wrap_start_primary, g_middle_cursor_alternate;
unsigned short g_middle_wrap_start_alternate;

ResourceKindInfo g_kind_info[1];
unsigned long g_configuration_dword_71388, g_configuration_dword_71370;
unsigned long g_configuration_dword_71390, g_configuration_dword_7137c;
unsigned long g_configuration_dword_713a4, g_configuration_dword_7139c;

static union { unsigned long align; unsigned char bytes[0x134 * 2]; } normal_pool;
static union { unsigned long align; unsigned char bytes[0x0ec * 2]; } middle_pool;
static union { unsigned long align; unsigned char bytes[0x094 * 2]; } alternate_pool;
static union { unsigned long align; unsigned char bytes[40]; } worklist_row;
unsigned char eng1_fixture_normal_assets[16 * 8];
unsigned char eng1_fixture_middle_assets[8 * 8];
unsigned char eng1_fixture_alternate_assets[8 * 8];
Eng1ActorActionCallback eng1_fixture_normal_actions[8 * 3];
Eng1ActorActionCallback eng1_fixture_middle_actions[8 * 2];
Eng1ActorActionCallback eng1_fixture_alternate_actions[8 * 2];
static unsigned char unknown_normal_assets[16 * 9];
static Eng1ActorActionCallback unknown_normal_actions[9 * 3];

static jmp_buf incomplete_jump;
static int incomplete_armed;
static unsigned long incomplete_rva;
static char incomplete_reason[224];

void __cdecl eng1_incomplete_behavior(unsigned long original_rva,
                                      const char *reason)
{
    incomplete_rva = original_rva;
    strncpy(incomplete_reason, reason, sizeof(incomplete_reason) - 1);
    incomplete_reason[sizeof(incomplete_reason) - 1] = '\0';
    if (incomplete_armed) longjmp(incomplete_jump, 1);
    exit(0xe1);
}

static int callback_count;
static int callback_failed;
static void *callback_actor;
static Eng1ForestRecord20 *callback_record;
static unsigned short expected_kind;
static unsigned short expected_group;
static unsigned short expected_state;
static unsigned char expected_category;
static unsigned long expected_asset64;
static unsigned long expected_asset110;
static unsigned long expected_asset118;
static unsigned long expected_flags54;
static unsigned char expected_lifetime;
static int check_lifetime;
static int expect_duplicate_coordinates;
static int expect_state_2a;
static int expect_extended_normal_assets;
static int expect_state_54;

static unsigned short get16(const void *p)
{
    return *(const unsigned short *)p;
}

static unsigned long get32(const void *p)
{
    return *(const unsigned long *)p;
}

static void set32(void *p, unsigned long v)
{
    *(unsigned long *)p = v;
}

static void __cdecl action_probe(void *actor)
{
    unsigned char *a = (unsigned char *)actor;
    unsigned char *r = (unsigned char *)callback_record;
    ++callback_count;
    callback_actor = actor;
    if (actor == 0 || callback_record == 0 || (r[0x0d] & 0x80U) == 0)
        callback_failed = 1;
    if (get16(a + 0x2e) != expected_kind ||
        get16(a + 0x38) != expected_group ||
        get32(a + 0x58) != 20UL ||
        a[0x22] != expected_category ||
        get32(a + 0x64) != expected_asset64)
        callback_failed = 1;
    if (expect_duplicate_coordinates &&
        (get32(a + 0x00) != (unsigned long)callback_record->x ||
         get32(a + 0x04) != (unsigned long)callback_record->y ||
         get32(a + 0x08) != (unsigned long)callback_record->z ||
         get32(a + 0x9c) != (unsigned long)callback_record->x ||
         get32(a + 0xa0) != (unsigned long)callback_record->y ||
         get32(a + 0xa4) != (unsigned long)callback_record->z))
        callback_failed = 1;
    if (expect_state_2a && get16(a + 0x2a) != expected_state)
        callback_failed = 1;
    if (check_lifetime && a[0x94] != expected_lifetime)
        callback_failed = 1;
    if (expect_state_54 && get32(a + 0x54) != expected_flags54)
        callback_failed = 1;
    if (expect_extended_normal_assets &&
        (get32(a + 0x110) != expected_asset110 ||
         get32(a + 0x118) != expected_asset118))
        callback_failed = 1;
}

/* The provider source references these reset-only services; the fixture never
 * enters those routines. */
int remove_actor(void *p) { (void)p; return 0; }
int remove_middle_actor(void *p) { (void)p; return 0; }
int release_actor(void *p) { (void)p; return 0; }
void clear_actor(void *p) { (void)p; }

static void reset_common(void)
{
    memset(normal_pool.bytes, 0, sizeof(normal_pool.bytes));
    memset(middle_pool.bytes, 0, sizeof(middle_pool.bytes));
    memset(alternate_pool.bytes, 0, sizeof(alternate_pool.bytes));
    memset(worklist_row.bytes, 0, sizeof(worklist_row.bytes));
    memset(eng1_fixture_normal_assets, 0, sizeof(eng1_fixture_normal_assets));
    memset(eng1_fixture_middle_assets, 0, sizeof(eng1_fixture_middle_assets));
    memset(eng1_fixture_alternate_assets, 0, sizeof(eng1_fixture_alternate_assets));
    memset(eng1_fixture_normal_actions, 0, sizeof(eng1_fixture_normal_actions));
    memset(eng1_fixture_middle_actions, 0, sizeof(eng1_fixture_middle_actions));
    memset(eng1_fixture_alternate_actions, 0, sizeof(eng1_fixture_alternate_actions));
    memset(unknown_normal_assets, 0, sizeof(unknown_normal_assets));
    memset(unknown_normal_actions, 0, sizeof(unknown_normal_actions));
    g_normal_actor_pool = (ActorNormal *)normal_pool.bytes;
    g_middle_actor_pool = (ActorMiddle *)middle_pool.bytes;
    g_alternate_actor_pool = (ActorCore *)alternate_pool.bytes;
    g_normal_cursor_primary = 0;
    g_normal_limit_primary = 2;
    g_normal_limit_wrap = 2;
    g_normal_cursor_alternate = 0;
    g_normal_limit_wrap_start = 0;
    g_middle_cursor_primary = 0;
    g_middle_limit_primary = 2;
    g_middle_wrap_start_primary = 2;
    g_middle_cursor_alternate = 0;
    g_middle_wrap_start_alternate = 0;
    g_alternate_cursor_primary = 0;
    g_alternate_limit_primary = 2;
    g_alternate_limit_wrap = 2;
    g_alternate_cursor_alternate = 0;
    g_alternate_limit_wrap_start = 0;
    g_kind_info[0].owner = worklist_row.bytes;
    g_kind_info[0].count = 1;
    g_configuration_dword_71388 = (unsigned long)eng1_fixture_normal_assets;
    g_configuration_dword_71370 = (unsigned long)eng1_fixture_normal_actions;
    g_configuration_dword_71390 = (unsigned long)eng1_fixture_middle_assets;
    g_configuration_dword_7137c = (unsigned long)eng1_fixture_middle_actions;
    g_configuration_dword_713a4 = (unsigned long)eng1_fixture_alternate_assets;
    g_configuration_dword_7139c = (unsigned long)eng1_fixture_alternate_actions;
    incomplete_armed = 0;
    incomplete_rva = 0;
    incomplete_reason[0] = '\0';
    callback_count = 0;
    callback_failed = 0;
    callback_actor = 0;
    callback_record = 0;
}

static Eng1ForestRecord20 *new_record(unsigned short kind,
                                      unsigned short flags,
                                      unsigned short state,
                                      long x, long y, long z)
{
    Eng1ForestRecord20 *record =
        (Eng1ForestRecord20 *)(worklist_row.bytes + 20);
    record->kind = kind;
    record->flags = flags;
    record->state = state;
    record->x = x;
    record->y = y;
    record->z = z;
    callback_record = record;
    return record;
}

static void expect(unsigned short kind, unsigned short state,
                   unsigned char category, unsigned long image,
                   unsigned long second, unsigned long third,
                   unsigned char lifetime, int lifetime_is_checked,
                   unsigned long state54,
                   int duplicate_coordinates, int state_at_2a,
                   int extended_assets, int flags54)
{
    expected_kind = kind;
    expected_group = 0;
    expected_state = state;
    expected_category = category;
    expected_asset64 = image;
    expected_asset110 = second;
    expected_asset118 = third;
    expected_lifetime = lifetime;
    check_lifetime = lifetime_is_checked;
    expected_flags54 = state54;
    expect_duplicate_coordinates = duplicate_coordinates;
    expect_state_2a = state_at_2a;
    expect_extended_normal_assets = extended_assets;
    expect_state_54 = flags54;
}

static int test_normal(void)
{
    Eng1ForestRecord20 *record;
    ActorNormal *actor;
    unsigned char *slot;
    reset_common();
    record = new_record(2, 0x1045U, 0x1234U,
                       0x00120000L, 0x00030000L, 0x00000001L);
    set32(eng1_fixture_normal_assets + 2 * 16 + 0, 0x11112222UL);
    set32(eng1_fixture_normal_assets + 2 * 16 + 4, 0x33334444UL);
    set32(eng1_fixture_normal_assets + 2 * 16 + 8, 0x55556666UL);
    eng1_fixture_normal_assets[2 * 16 + 0x0c] = 0x5aU;
    eng1_fixture_normal_actions[2 * 3] = action_probe;
    expect(2, 0x1234U, 0x5aU, 0x11112222UL,
           0x33334444UL, 0x55556666UL, 0xffU, 1,
           0x80000010UL, 1, 1, 1, 1);
    actor = eng1_call_270c0(record, 0);
    slot = normal_pool.bytes;
    return actor == (ActorNormal *)slot && callback_count == 1 &&
           callback_actor == actor && !callback_failed &&
           record->flags == 0x9045U && get16(slot + 0x2e) == 2 &&
           get16(slot + 0x2a) == 0x1234U &&
           get32(slot + 0x54) == 0x80000010UL &&
           get32(slot + 0x58) == 20UL;
}

static int test_middle(void)
{
    Eng1ForestRecord20 *record;
    ActorMiddle *actor;
    unsigned char *slot;
    reset_common();
    record = new_record(0x4003U, 0x0045U, 0xbeefU,
                       0x00210000L, 0x00070000L, 0x00010001L);
    set32(eng1_fixture_middle_assets + 3 * 8, 0x22223333UL);
    eng1_fixture_middle_assets[3 * 8 + 6] = 0x33U;
    eng1_fixture_middle_actions[3 * 2] = action_probe;
    expect(0x4003U, 0, 0x33U, 0x22223333UL, 0, 0,
           0, 1, 0x80000000UL, 1, 0, 0, 1);
    actor = eng1_call_27720(record, 0);
    slot = middle_pool.bytes;
    return actor == (ActorMiddle *)slot && callback_count == 1 &&
           callback_actor == actor && !callback_failed &&
           record->flags == 0x8045U && get16(slot + 0x2e) == 0x4003U &&
           get32(slot + 0x54) == 0x80000000UL &&
           get32(slot + 0x58) == 20UL;
}

static int test_alternate(void)
{
    Eng1ForestRecord20 *record;
    ActorCore *actor;
    unsigned char *slot;
    reset_common();
    record = new_record(0x2005U, 0x0101U, 0x9876U,
                       0x00310000L, 0x00090000L, 0x000b0000L);
    set32(eng1_fixture_alternate_assets + 5 * 8, 0x44445555UL);
    eng1_fixture_alternate_assets[5 * 8 + 6] = 0x77U;
    eng1_fixture_alternate_actions[5 * 2] = action_probe;
    expect(0x2005U, 0, 0x77U, 0x44445555UL, 0, 0,
           0, 0, 0x80000000UL, 0, 0, 0, 0);
    actor = eng1_call_27c40(record, 0);
    slot = alternate_pool.bytes;
    return actor == (ActorCore *)slot && callback_count == 1 &&
           callback_actor == actor && !callback_failed &&
           record->flags == 0x8101U && get16(slot + 0x2e) == 0x2005U &&
           get32(slot + 0x00) == (unsigned long)record->x &&
           get32(slot + 0x04) == (unsigned long)record->y &&
           get32(slot + 0x08) == (unsigned long)record->z &&
           get32(slot + 0x58) == 20UL;
}

static int test_allocation_failure(void)
{
    Eng1ForestRecord20 *record;
    unsigned char *slot;
    reset_common();
    record = new_record(2, 0x1234U, 0x4321U,
                       0x10000L, 0x20000L, 0x30000L);
    slot = normal_pool.bytes;
    *(unsigned short *)(slot + 0x2e) = 1;
    g_normal_limit_wrap = 1;
    incomplete_armed = 1;
    if (setjmp(incomplete_jump) != 0) {
        incomplete_armed = 0;
        return 0;
    }
    if (eng1_call_270c0(record, 1) != 0) {
        incomplete_armed = 0;
        return 0;
    }
    incomplete_armed = 0;
    return callback_count == 0 && !callback_failed && record->flags == 0x1234U;
}

static int invoke_normal_catching(Eng1ForestRecord20 *record,
                                  unsigned long group_index)
{
    incomplete_armed = 1;
    if (setjmp(incomplete_jump) == 0) {
        (void)eng1_call_270c0(record, group_index);
        incomplete_armed = 0;
        return 0;
    }
    incomplete_armed = 0;
    return 1;
}

static int test_registered_last_asset_row(void)
{
    Eng1ForestRecord20 *record;
    ActorNormal *actor;
    reset_common();
    record = new_record(7, 0x0011U, 0x2222U, 0x10000L, 0x20000L, 0x30000L);
    set32(eng1_fixture_normal_assets + 7 * 16, 0x77778888UL);
    eng1_fixture_normal_assets[7 * 16 + 0x0c] = 0x77U;
    eng1_fixture_normal_actions[7 * 3] = action_probe;
    expect(7, 0x2222U, 0x77U, 0x77778888UL, 0, 0,
           0, 0, 0x80000000UL, 0, 1, 0, 0);
    actor = eng1_call_270c0(record, 0);
    return actor == (ActorNormal *)normal_pool.bytes && callback_count == 1 &&
           callback_actor == actor && !callback_failed && incomplete_rva == 0;
}

static int test_registered_asset_boundary(void)
{
    Eng1ForestRecord20 *record;
    reset_common();
    record = new_record(8, 0x0101U, 0x4321U, 0x10000L, 0x20000L, 0x30000L);
    expect(8, 0x4321U, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0);
    if (!invoke_normal_catching(record, 0)) return 0;
    return incomplete_rva == 0x5c408UL &&
           strstr(incomplete_reason, "index 8") != 0 &&
           callback_count == 0 && (record->flags & 0x8000U) != 0 &&
           get16(normal_pool.bytes + 0x2e) == 8U;
}

static int test_registered_callback_boundary(void)
{
    Eng1ForestRecord20 *record;
    reset_common();
    record = new_record(8, 0x0011U, 0x4321U, 0x10000L, 0x20000L, 0x30000L);
    set32(unknown_normal_assets + 8 * 16, 0x88889999UL);
    unknown_normal_assets[8 * 16 + 0x0c] = 0x88U;
    g_configuration_dword_71388 = (unsigned long)unknown_normal_assets;
    expect(8, 0x4321U, 0x88U, 0x88889999UL, 0, 0,
           0, 0, 0x80000000UL, 1, 1, 0, 0);
    if (!invoke_normal_catching(record, 0)) return 0;
    return incomplete_rva == 0x5c688UL &&
           strstr(incomplete_reason, "index 8") != 0 &&
           callback_count == 0 && (record->flags & 0x8000U) != 0 &&
           get32(normal_pool.bytes + 0x64) == 0x88889999UL &&
           get32(normal_pool.bytes + 0x58) == 20UL;
}

static int test_group_owner_boundary(void)
{
    Eng1ForestRecord20 *record;
    reset_common();
    record = new_record(2, 0x0011U, 0x4321U, 0x10000L, 0x20000L, 0x30000L);
    set32(eng1_fixture_normal_assets + 2 * 16, 0x22223333UL);
    eng1_fixture_normal_actions[2 * 3] = action_probe;
    expect(2, 0x4321U, 0, 0x22223333UL, 0, 0,
           0, 0, 0x80000000UL, 1, 1, 0, 0);
    if (!invoke_normal_catching(record, 1)) return 0;
    return incomplete_rva == 0x6fee8UL && callback_count == 0 &&
           (record->flags & 0x8000U) != 0 &&
           get16(normal_pool.bytes + 0x2e) == 2U &&
           get16(normal_pool.bytes + 0x38) == 1U &&
           get32(normal_pool.bytes + 0x58) == 0UL;
}

static int test_unknown_runtime_tables_keep_native_path(void)
{
    Eng1ForestRecord20 *record;
    ActorNormal *actor;
    reset_common();
    record = new_record(8, 0x0011U, 0x4321U, 0x10000L, 0x20000L, 0x30000L);
    set32(unknown_normal_assets + 8 * 16, 0x88889999UL);
    unknown_normal_assets[8 * 16 + 0x0c] = 0x88U;
    unknown_normal_actions[8 * 3] = action_probe;
    g_configuration_dword_71388 = (unsigned long)unknown_normal_assets;
    g_configuration_dword_71370 = (unsigned long)unknown_normal_actions;
    expect(8, 0x4321U, 0x88U, 0x88889999UL, 0, 0,
           0, 0, 0x80000000UL, 1, 1, 0, 0);
    actor = eng1_call_270c0(record, 0);
    return actor == (ActorNormal *)normal_pool.bytes && callback_count == 1 &&
           callback_actor == actor && !callback_failed && incomplete_rva == 0;
}

int main(void)
{
    int cases = 9;
    int failures = 0;
    int normal_ok = test_normal();
    int middle_ok = test_middle();
    int alternate_ok = test_alternate();
    int failure_ok = test_allocation_failure();
    int last_asset_ok = test_registered_last_asset_row();
    int asset_boundary_ok = test_registered_asset_boundary();
    int callback_boundary_ok = test_registered_callback_boundary();
    int group_boundary_ok = test_group_owner_boundary();
    int unknown_tables_ok = test_unknown_runtime_tables_keep_native_path();
    failures += !normal_ok;
    failures += !middle_ok;
    failures += !alternate_ok;
    failures += !failure_ok;
    failures += !last_asset_ok;
    failures += !asset_boundary_ok;
    failures += !callback_boundary_ok;
    failures += !group_boundary_ok;
    failures += !unknown_tables_ok;
    printf("normal=%d middle=%d alternate=%d allocation_failure=%d\n",
           normal_ok, middle_ok, alternate_ok, failure_ok);
    printf("last_asset=%d asset_guard=%d callback_guard=%d group_guard=%d unknown_tables=%d\n",
           last_asset_ok, asset_boundary_ok, callback_boundary_ok,
           group_boundary_ok, unknown_tables_ok);
    printf("worklist_factory_cases=%d checks=9 failures=%d\n", cases, failures);
    return failures ? 1 : 0;
}
