#include <stdio.h>
#include <string.h>

#include "../calibration/actor_factories.c"

static NormalFactoryAsset normal_rows[65536];
static AlternateFactoryAsset alternate_rows[4096];
NormalFactoryAsset *g_normal_factory_assets = normal_rows;
AlternateFactoryAsset *g_alternate_factory_assets = alternate_rows;

static Actor normal_actor;
static Actor alternate_actor;
static Actor *normal_result;
static Actor *alternate_result;
static int normal_trace[4];
static int alternate_trace[4];
static int normal_trace_count;
static int alternate_trace_count;
static int normal_arg;
static int alternate_arg;
static int normal_pool_selected;
static int alternate_pool_selected;
static int checks;
static int failures;

static void expect(int ok, const char *label)
{
    ++checks;
    if (!ok) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

Actor *allocate_normal_actor(ActorKind kind)
{
    normal_arg = kind;
    normal_pool_selected = (kind & 0x8000) != 0;
    normal_trace[normal_trace_count++] = 1;
    return normal_result;
}

void initialize_normal_actor(Actor *actor)
{
    normal_trace[normal_trace_count++] = 2;
    actor->kind = 0xeeee;
    actor->category = 0xa5;
    actor->image = (void *)1;
}

Actor *allocate_alternate_actor(ActorKind kind)
{
    alternate_arg = kind;
    alternate_pool_selected = (kind & 0x8000) != 0;
    alternate_trace[alternate_trace_count++] = 1;
    return alternate_result;
}

void initialize_alternate_actor(Actor *actor)
{
    alternate_trace[alternate_trace_count++] = 2;
    actor->kind = 0xeeee;
    actor->category = 0xa5;
    actor->image = (void *)1;
}

static void reset_fixture(void)
{
    memset(&normal_actor, 0, sizeof(normal_actor));
    memset(&alternate_actor, 0, sizeof(alternate_actor));
    memset(normal_trace, 0, sizeof(normal_trace));
    memset(alternate_trace, 0, sizeof(alternate_trace));
    normal_trace_count = alternate_trace_count = 0;
    normal_arg = alternate_arg = -1;
    normal_pool_selected = alternate_pool_selected = -1;
}

int main(void)
{
    unsigned int ni = 0x23;
    unsigned int ai = 0x57;
    unsigned int value;
    long nx = 0x1234567L, ny = -0x234567L, nz = 0x3456789L;
    long ax = -0x456789L, ay = 0x56789aL, az = -0x6789abL;
    void *normal_image = (void *)0x1234;
    void *normal_frames = (void *)0x2345;
    void *normal_steps = (void *)0x3456;
    void *alternate_image = (void *)0x4567;
    Actor *result;

    for (value = 0; value < 65536U; ++value) {
        normal_rows[value].image = (void *)(value + 0x10000U);
        normal_rows[value].field_004 = (void *)(value + 0x20000U);
        normal_rows[value].field_008 = (void *)(value + 0x30000U);
        normal_rows[value].category = (unsigned char)(value ^ 0x5aU);
    }
    for (value = 0; value < 4096U; ++value) {
        alternate_rows[value].image = (void *)(value + 0x40000U);
        alternate_rows[value].category = (unsigned char)(value ^ 0xa5U);
    }

    reset_fixture();
    normal_rows[ni].image = normal_image;
    normal_rows[ni].field_004 = normal_frames;
    normal_rows[ni].field_008 = normal_steps;
    normal_rows[ni].category = 0x6d;
    normal_result = &normal_actor;
    result = spawn_normal(nx, ny, nz, (ActorKind)0x23);
    expect(result == &normal_actor, "normal returns allocated actor");
    expect(normal_arg == (ActorKind)0x23 && normal_pool_selected == 0,
           "normal allocator receives the defined low-word token and selects pool from bit 15");
    expect(normal_trace_count == 2 && normal_trace[0] == 1 && normal_trace[1] == 2,
           "normal allocation precedes initialization");
    expect(normal_actor.kind == 0x23, "normal stores low 16-bit kind");
    expect(normal_actor.x == nx && normal_actor.y == ny && normal_actor.z == nz,
           "normal stores coordinates");
    expect(normal_actor.unknown_09c == nx && normal_actor.unknown_0a0 == ny &&
           normal_actor.unknown_0a4 == nz, "normal stores coordinate copies");
    expect(normal_actor.category == 0x6d && normal_actor.image == normal_image,
           "normal loads category and image from low-16-bit row");
    expect(normal_actor.frames == (void **)normal_frames &&
           normal_actor.initial_steps == normal_steps,
           "normal loads both additional descriptor pointers");

    reset_fixture();
    normal_result = 0;
    result = spawn_normal(nx, ny, nz, (ActorKind)0x23);
    expect(result == 0, "normal allocation failure returns null");
    expect(normal_trace_count == 1 && normal_trace[0] == 1,
           "normal failure skips initializer");

    reset_fixture();
    alternate_rows[ai].image = alternate_image;
    alternate_rows[ai].unknown_004 = 0x9876;
    alternate_rows[ai].category = 0x4c;
    alternate_result = &alternate_actor;
    result = spawn_kind(ax, ay, az, (ActorKind)0x57);
    expect(result == &alternate_actor, "alternate returns allocated actor");
    expect(alternate_arg == (ActorKind)0x57 && alternate_pool_selected == 0,
           "alternate allocator receives the defined low-word token and selects pool from bit 15");
    expect(alternate_trace_count == 2 && alternate_trace[0] == 1 &&
           alternate_trace[1] == 2, "alternate allocation precedes initialization");
    expect(alternate_actor.kind == 0x2057, "alternate stores kind with bit 13 set");
    expect(alternate_actor.x == ax && alternate_actor.y == ay &&
           alternate_actor.z == az, "alternate stores coordinates");
    expect(alternate_actor.category == 0x4c && alternate_actor.image == alternate_image,
           "alternate loads category and image from low-12-bit row");

    reset_fixture();
    alternate_result = 0;
    result = spawn_kind(ax, ay, az, (ActorKind)0x57);
    expect(result == 0, "alternate allocation failure returns null");
    expect(alternate_trace_count == 1 && alternate_trace[0] == 1,
           "alternate failure skips initializer");

    /* All 16-bit kind words are defined; the physical caller slot's upper
       half is not part of this typed interface. Both real allocators inspect
       only bit 15 of the word (TEST AH,80), while descriptors use 16/12 bits. */
    for (value = 0; value < 65536U; ++value) {
        ActorKind kind = (ActorKind)value;
        unsigned int alternate_index = value & 0xfff;
        int normal_ok, alternate_ok;

        reset_fixture();
        normal_result = &normal_actor;
        result = spawn_normal(0, 0, 0, kind);
        normal_ok = result == &normal_actor && normal_arg == (int)kind &&
            normal_pool_selected == ((value & 0x8000U) != 0) &&
            normal_actor.kind == kind &&
            normal_actor.image == normal_rows[value].image &&
            normal_actor.frames == (void **)normal_rows[value].field_004 &&
            normal_actor.initial_steps == normal_rows[value].field_008 &&
            normal_actor.category == normal_rows[value].category &&
            normal_trace_count == 2 && normal_trace[0] == 1 && normal_trace[1] == 2;
        expect(normal_ok, "normal preserves every 16-bit kind/index/flag combination");

        reset_fixture();
        alternate_result = &alternate_actor;
        result = spawn_kind(0, 0, 0, kind);
        alternate_ok = result == &alternate_actor && alternate_arg == (int)kind &&
            alternate_pool_selected == ((value & 0x8000U) != 0) &&
            alternate_actor.kind == (ActorKind)(kind | 0x2000) &&
            alternate_actor.image == alternate_rows[alternate_index].image &&
            alternate_actor.category == alternate_rows[alternate_index].category &&
            alternate_trace_count == 2 && alternate_trace[0] == 1 &&
            alternate_trace[1] == 2;
        expect(alternate_ok, "alternate preserves all flags with low-12 descriptor index");
    }

    printf("factory semantic checks=%d failures=%d\n", checks, failures);
    return failures ? 1 : 0;
}
