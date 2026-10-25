/* Bounded callback-spy fixture. These providers are test spies, not production code. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../calibration/engine_interface.h"

/* The 3D preflight wrapper is linked but outside this fixture's callback path. */
void __cdecl apply_frame_motion_3d(void *pointer)
{
    (void)pointer;
    fputs("unexpected 3D motion in default callback fixture\n",stderr);
    exit(93);
}

typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern void __cdecl eng1_native_code_target_001010(void *);
extern void __cdecl eng1_native_code_target_001040(void *);
extern void __cdecl eng1_native_code_target_0010f0(void *);
extern void __cdecl eng1_native_code_target_001310(void *);
extern void __cdecl eng1_native_code_target_0013c0(void *);
extern void __cdecl eng1_native_code_target_001d80(void *);
extern void __cdecl eng1_native_code_target_001e00(void *);
extern void __cdecl eng1_native_code_target_001e30(void *);

EngineInterface g_engine_interface;
u32 g_actor_worklist_716c0[32];
u32 g_configuration_dword_713bc;
extern u16 g_native_word_72168;
/* Oracle bytes at 0x4d6ec..0x4d6f3; callbacks read byte windows [0,3] and [4,7]. */
extern u8 g_actor_state_map_004d6ec[8];
unsigned char g_native_world_7217c[0x200];
unsigned char *g_native_pointer_7217c = g_native_world_7217c;

static int call_ids[32];
static u32 call_args[32][3];
static int call_count;
static u32 random_values[8];
static int random_count, random_index;
static unsigned char resource_token;
static void *resource_value = &resource_token;
static int collision_result;
static int area_result;

static void record_call(int id, u32 a, u32 b, u32 c)
{
    call_ids[call_count] = id;
    call_args[call_count][0] = a;
    call_args[call_count][1] = b;
    call_args[call_count][2] = c;
    ++call_count;
}

static void reset_calls(void)
{
    call_count = 0;
    random_count = 0;
    random_index = 0;
}

static void add_random(u32 value)
{
    random_values[random_count++] = value;
}

static u32 get32(const void *actor, unsigned offset)
{ return *(const u32 *)((const unsigned char *)actor + offset); }
static u16 get16(const void *actor, unsigned offset)
{ return *(const u16 *)((const unsigned char *)actor + offset); }
static u8 get8(const void *actor, unsigned offset)
{ return *((const unsigned char *)actor + offset); }
static void put32(void *actor, unsigned offset, u32 value)
{ *(u32 *)((unsigned char *)actor + offset) = value; }
static void put16(void *actor, unsigned offset, u16 value)
{ *(u16 *)((unsigned char *)actor + offset) = value; }
static void put8(void *actor, unsigned offset, u8 value)
{ *((unsigned char *)actor + offset) = value; }

static int expect_calls(const int *expected, int count)
{
    int i;
    if (call_count != count)
        return 0;
    for (i = 0; i < count; ++i)
        if (call_ids[i] != expected[i])
            return 0;
    return 1;
}

#define REQUIRE(condition) do { \
    if (!(condition)) { printf("FAIL line %d\n", __LINE__); return 1; } \
} while (0)

void *__cdecl eng1_native_code_target_0029670(u32 kind)
{ record_call(1, kind, 0, 0); return resource_value; }
unsigned int __cdecl dispatch_engine_2c460(int bound)
{
    record_call(3, (u32)bound, 0, 0);
    if (random_index >= random_count)
        return 0;
    return random_values[random_index++];
}
void __cdecl append_chain(void *resource, void *actor)
{ record_call(2, (u32)resource, (u32)actor, 0); }
void __cdecl eng1_native_code_target_002a290(void *actor)
{ record_call(6, (u32)actor, 0, 0); }
void __cdecl eng1_native_code_target_00283e0(void *actor)
{ record_call(7, (u32)actor, 0, 0); }
u32 __cdecl eng1_native_code_target_002a9e0(void *actor, u32 query)
{ record_call(8, (u32)actor, query, 0); return 0; }
u32 __cdecl eng1_native_code_target_0026de0(void *actor)
{ record_call(9, (u32)actor, 0, 0); return 0; }
u32 __cdecl eng1_native_code_target_0019b40(u32 mask)
{ record_call(10, mask, 0, 0); return 0; }
void __cdecl eng1_native_code_target_001ab50(void)
{ record_call(11, 0, 0, 0); }
void __cdecl eng1_native_code_target_0030290(u32 action, u32 value)
{ record_call(12, action, value, 0); }
int __cdecl eng1_native_code_target_0029c20(void *actor)
{ record_call(13, (u32)actor, 0, 0); return 1; }
u32 __cdecl eng1_native_code_target_002a170(u32 x, u32 y, u32 z, u32 step)
{ record_call(14, x, y, z ^ step); return 0; }
int __cdecl eng1_native_code_target_002a710(void *area, void *actor)
{ record_call(15, (u32)area, (u32)actor, 0); return 0; }
int __cdecl eng1_native_code_target_002a880(void *actor, void *bounds, u32 kind)
{ record_call(16, (u32)actor, (u32)bounds, kind); return 0; }
int __cdecl bounds_overlap(void *a, void *b)
{ record_call(17, (u32)a, (u32)b, 0); return 0; }
void __cdecl eng1_native_code_target_00266a0(void *actor, u32 event, u32 value)
{ record_call(18, (u32)actor, event, value); }
int __cdecl eng1_native_code_target_002a810(u32 x, u32 y, u32 z, void *area)
{ record_call(19, x, y, z); call_args[call_count - 1][2] ^= (u32)area; return collision_result; }
int __cdecl eng1_native_code_target_002aa30(void *actor, u32 width, u32 height)
{ record_call(20, (u32)actor, width, height); return area_result; }
void __cdecl eng1_native_code_target_0028820(void *actor)
{ record_call(21, (u32)actor, 0, 0); }

void __cdecl advance_frame(void *actor) { record_call(22, (u32)actor, 0, 0); }
void __cdecl restart_steps(void *actor) { record_call(23, (u32)actor, 0, 0); }
void __cdecl apply_frame_motion_2d(void *actor) { record_call(24, (u32)actor, 0, 0); }
void __cdecl advance_planar_motion(void *actor) { record_call(25, (u32)actor, 0, 0); }
void __cdecl reset_frame_state(void *actor) { record_call(26, (u32)actor, 0, 0); }
void __cdecl select_frame(void *actor, int frame)
{ record_call(27, (u32)actor, (u32)frame, 0); }
int __cdecl remove_actor(void *actor)
{ record_call(28, (u32)actor, 0, 0); return 0; }
int __cdecl remove_actor_variant(void *actor)
{ record_call(29, (u32)actor, 0, 0); return 0; }

int main(void)
{
    unsigned char actor[0x128];
    unsigned char config[4];
    int expected[8];

    memset(actor, 0, sizeof(actor));
    memset(config, 0, sizeof(config));
    g_engine_interface.context_004 = (ResourceCallbackContext *)config;
    REQUIRE(g_actor_state_map_004d6ec[0] == 0xff &&
            g_actor_state_map_004d6ec[1] == 0x00 &&
            g_actor_state_map_004d6ec[2] == 0x01 &&
            g_actor_state_map_004d6ec[3] == 0x02 &&
            g_actor_state_map_004d6ec[4] == 0xff &&
            g_actor_state_map_004d6ec[5] == 0x05 &&
            g_actor_state_map_004d6ec[6] == 0x06 &&
            g_actor_state_map_004d6ec[7] == 0x07);

    /* 0x1010 is a void, one-pointer initializer and appends into the worklist. */
    reset_calls();
    eng1_native_code_target_001010(actor);
    REQUIRE(get32(actor, 0x54) == 0 && get32(actor, 0x50) == 0x81);
    REQUIRE(get8(actor, 0x97) == 0x2a);
    REQUIRE(g_actor_worklist_716c0[0] == 1 && g_actor_worklist_716c0[1] == (u32)actor);
    REQUIRE(call_count == 0);

    /* 0x1040 uses map bytes 0..3 after the second bounded draw. */
    memset(actor, 0, sizeof(actor));
    config[0] = 0;
    reset_calls();
    add_random(1);
    add_random(2);
    eng1_native_code_target_001040(actor);
    REQUIRE(get8(actor, 0x94) == 0xff);
    REQUIRE(get32(actor, 0x50) == 0x200 && get32(actor, 0x54) == 8);
    REQUIRE(get32(actor, 0xf4) == 2 && get16(actor, 0x124) == 7);
    REQUIRE(get16(actor, 0xee) == 9 && get8(actor, 0x1c) == 1);
    expected[0] = 1; expected[1] = 2; expected[2] = 3;
    expected[3] = 22; expected[4] = 23; expected[5] = 3; expected[6] = 6;
    REQUIRE(expect_calls(expected, 7) && random_index == random_count);
    REQUIRE(call_args[2][0] == 2 && call_args[5][0] == 4);
    REQUIRE(call_args[1][0] == (u32)&resource_token && call_args[1][1] == (u32)actor);

    /* 0x1310 reads the second four-byte window, including the 0x4d6f0 tail. */
    memset(actor, 0, sizeof(actor));
    reset_calls();
    add_random(1);
    add_random(3);
    eng1_native_code_target_001310(actor);
    REQUIRE(get8(actor, 0x1c) == 7 && get16(actor, 0x124) == 8);
    REQUIRE(get16(actor, 0xee) == 5 && get8(actor, 0x94) == 0xff);
    REQUIRE(random_index == random_count);
    REQUIRE(call_args[5][0] == 4);

    /* Both state-update callbacks pass their actor to the one-argument interpreter. */
    memset(actor, 0, sizeof(actor));
    put16(actor, 0x2a, 9);
    reset_calls();
    eng1_native_code_target_0010f0(actor);
    expected[0] = 8; expected[1] = 7;
    REQUIRE(expect_calls(expected, 2));
    REQUIRE(call_args[0][1] == 0x17c && call_args[1][0] == (u32)actor);

    reset_calls();
    eng1_native_code_target_0013c0(actor);
    REQUIRE(expect_calls(expected, 2));
    REQUIRE(call_args[0][1] == 0x17c && call_args[1][0] == (u32)actor);

    /* Original128e/155e push actor then collision+88: cdecl area comes first. */
    memset(actor, 0, sizeof(actor));
    put16(actor, 0x2a, 2);
    put8(actor, 0x25, 1);
    reset_calls();
    eng1_native_code_target_0010f0(actor);
    expected[0] = 8; expected[1] = 6; expected[2] = 15;
    REQUIRE(expect_calls(expected, 3));
    REQUIRE(call_args[2][0] == (u32)(g_native_world_7217c + 0x88));
    REQUIRE(call_args[2][1] == (u32)actor);
    reset_calls();
    eng1_native_code_target_0013c0(actor);
    REQUIRE(expect_calls(expected, 3));
    REQUIRE(call_args[2][0] == (u32)(g_native_world_7217c + 0x88));
    REQUIRE(call_args[2][1] == (u32)actor);

    /* 0x1d80 calls the planar helper, checks the frame area, then merges flags. */
    memset(actor, 0, sizeof(actor));
    put32(actor, 0x54, 0x20);
    put8(actor, 0x94, 0x10);
    reset_calls();
    eng1_native_code_target_001d80(actor);
    expected[0] = 25; expected[1] = 19; expected[2] = 20;
    REQUIRE(expect_calls(expected, 3));
    REQUIRE(get32(actor, 0x54) == 0x30);
    REQUIRE(call_args[1][0] == 0 && call_args[1][1] == 0 &&
            call_args[1][2] == (u32)(g_native_world_7217c + 0x88));
    REQUIRE(call_args[2][1] == 0x40 && call_args[2][2] == 0x40);

    /* 0x1e00 zero-extends the state word into the countdown. */
    memset(actor, 0, sizeof(actor));
    put16(actor, 0x2a, 0xff82);
    reset_calls();
    eng1_native_code_target_001e00(actor);
    expected[0] = 26;
    REQUIRE(expect_calls(expected, 1));
    REQUIRE(get32(actor, 0x4c) == 0xff82 && get16(actor, 0x4a) == 0x60);
    REQUIRE(get32(actor, 0x54) == 5);

    /* 0x1e30 decrements a live timer; the next call dispatches the void fallback. */
    put32(actor, 0x4c, 1);
    reset_calls();
    eng1_native_code_target_001e30(actor);
    REQUIRE(get32(actor, 0x4c) == 0 && call_count == 0);
    eng1_native_code_target_001e30(actor);
    expected[0] = 21;
    REQUIRE(expect_calls(expected, 1));
    REQUIRE(call_args[0][0] == (u32)actor);

    printf("ENG1 default callback fixture passed\n");
    return 0;
}
