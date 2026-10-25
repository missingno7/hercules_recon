#include "../src/eng1/resource_runtime_data.h"
/* Bounded callback-spy fixture. Helpers here are test spies, not production providers. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* The 2D wrapper is linked but outside the spatial-effect fixture's scope. */
void __cdecl apply_frame_motion_2d(void *pointer)
{
    (void)pointer;
    fputs("unexpected 2D motion in spatial effect fixture\n",stderr);
    exit(94);
}

extern void __cdecl eng1_native_code_target_00f0a0(void *);
extern void __cdecl eng1_native_code_target_00f180(void *);
extern void __cdecl eng1_native_code_target_00f380(void *);
extern void __cdecl eng1_native_code_target_00f3b0(void *);
extern void __cdecl eng1_native_code_target_00f430(void *);
extern void __cdecl eng1_native_code_target_00f4f0(void *);
extern void __cdecl eng1_native_code_target_00fd00(void *);
extern void __cdecl eng1_native_code_target_00fe60(void *);
extern void __cdecl eng1_native_code_target_0102c0(void *);

unsigned char eng1_native_data_051fd4[4] = { 0x31, 0x41, 0x59, 0x26 };
unsigned short eng1_native_cell_0718e0;
unsigned short eng1_native_cell_0716a8;
unsigned long eng1_native_cell_071718;
unsigned short eng1_words_0713bc[512];
unsigned short eng1_words_071714[512];
unsigned long eng1_offsets_06fee8[256];
unsigned short *eng1_native_ptr_0713bc = eng1_words_0713bc;
unsigned short *eng1_native_ptr_071714 = eng1_words_071714;
ResourceKindInfo g_kind_info[1];
long eng1_anchor_0734f4;
const long *eng1_native_ptr_0734f4 = &eng1_anchor_0734f4;

static int calls[128];
static unsigned long call_args[128];
static int call_count;
static int random_args[32];
static unsigned long random_results[32];
static int random_count, random_index;
static unsigned long raw_random_result;
static int raw_random_calls;
static long projection_result;
static unsigned long cap_result;
static unsigned long tile_result;
static int completion_result;
static unsigned long collision_result;
static unsigned long callback_result;
static int area_result;
static unsigned long spawn_coordinates[8][3];
static void *spawn_results[8];
static int spawn_count;

typedef union AlignedBlock {
    unsigned long alignment;
    unsigned char bytes[0x180];
} AlignedBlock;

static void record_call(int id, unsigned long arg)
{
    calls[call_count] = id;
    call_args[call_count] = arg;
    ++call_count;
}

static void reset_calls(void)
{
    call_count = 0;
    random_index = 0;
    random_count = 0;
    raw_random_calls = 0;
    spawn_count = 0;
}

static void add_random(int argument, unsigned long result)
{
    random_args[random_count] = argument;
    random_results[random_count] = result;
    ++random_count;
}

static unsigned long get32(AlignedBlock *p, unsigned long offset)
{
    return *(unsigned long *)(p->bytes + offset);
}

static unsigned short get16(AlignedBlock *p, unsigned long offset)
{
    return *(unsigned short *)(p->bytes + offset);
}

static unsigned char get8(AlignedBlock *p, unsigned long offset)
{
    return *(unsigned char *)(p->bytes + offset);
}

static void put32(AlignedBlock *p, unsigned long offset, unsigned long value)
{
    *(unsigned long *)(p->bytes + offset) = value;
}

static void put16(AlignedBlock *p, unsigned long offset, unsigned short value)
{
    *(unsigned short *)(p->bytes + offset) = value;
}

static void put8(AlignedBlock *p, unsigned long offset, unsigned char value)
{
    *(unsigned char *)(p->bytes + offset) = value;
}

static int expect_calls(const int *expected, int count)
{
    int i;
    if (call_count != count)
        return 0;
    for (i = 0; i < count; ++i)
        if (calls[i] != expected[i])
            return 0;
    return 1;
}

long __cdecl eng1_native_dep_020b50(long x, long y, long z, int step)
{
    record_call(1, (unsigned long)step);
    call_args[call_count - 1] = (unsigned long)step;
    call_args[call_count - 1] ^= ((unsigned long)x << 16) ^ (unsigned short)y ^
                                ((unsigned long)(unsigned short)z << 1);
    return projection_result;
}

unsigned long __cdecl eng1_native_dep_0299d0(void *p)
{ record_call(2, (unsigned long)p); return cap_result; }

unsigned int __cdecl dispatch_engine_2c460(int value)
{
    record_call(3, (unsigned long)value);
    if (random_index >= random_count)
        return 0;
    if (random_args[random_index] != value)
        return 0xffffffffUL;
    return random_results[random_index++];
}

unsigned long __cdecl eng1_native_dep_02c470(void)
{ record_call(4, 0); ++raw_random_calls; return raw_random_result; }

void __cdecl eng1_native_dep_02a290(void *p)
{ record_call(7, (unsigned long)p); }

void __cdecl eng1_native_dep_0283e0(void *p)
{ record_call(8, (unsigned long)p); }

unsigned long __cdecl eng1_native_dep_02a170(long x, long y, long plane, int step)
{
    record_call(10, (unsigned long)step);
    call_args[call_count - 1] = ((unsigned long)x << 16) ^ (unsigned short)y ^
                                plane ^ (unsigned short)step;
    return tile_result;
}

int __cdecl eng1_native_dep_029c20(void *p)
{ record_call(11, (unsigned long)p); return completion_result; }

void __cdecl eng1_native_dep_02bc50(void *p)
{ record_call(12, (unsigned long)p); }

void __cdecl eng1_native_dep_00ef10(void *p)
{ record_call(13, (unsigned long)p); }

unsigned long __cdecl eng1_native_dep_02bd20(void *p)
{ record_call(14, (unsigned long)p); return collision_result; }

unsigned long __cdecl eng1_native_dep_00ff70(void *p, int value)
{ record_call(15, (unsigned long)value); return callback_result; }

void __cdecl eng1_native_dep_006da0(unsigned long event, void *p)
{ record_call(16, event); }

void *__cdecl eng1_native_dep_0300c0(long x, long y, long z,
                                     unsigned long kind, unsigned long flags)
{
    void *result = 0;
    if (spawn_count < 8) {
        spawn_coordinates[spawn_count][0] = (unsigned long)x;
        spawn_coordinates[spawn_count][1] = (unsigned long)y;
        spawn_coordinates[spawn_count][2] = (unsigned long)z;
        result = spawn_results[spawn_count];
    }
    ++spawn_count;
    record_call(17, kind);
    call_args[call_count - 1] ^= (unsigned long)x ^ (unsigned long)y ^
                                (unsigned long)z ^ flags;
    return result;
}

int __cdecl eng1_native_dep_02aa30(void *p, int width, int height)
{ record_call(22, (unsigned long)width ^ ((unsigned long)height << 16)); return area_result; }

void __cdecl advance_frame(void *p) { record_call(5, (unsigned long)p); }
void __cdecl restart_steps(void *p) { record_call(6, (unsigned long)p); }
void __cdecl apply_frame_motion_3d(void *p) { record_call(21, (unsigned long)p); }
void __cdecl select_frame(void *p, int frame) { record_call(20, (unsigned long)frame); }
int __cdecl remove_actor(void *p) { record_call(18, (unsigned long)p); return 0; }
int __cdecl remove_actor_variant(void *p) { record_call(19, (unsigned long)p); return 0; }

#define REQUIRE(condition) do { \
    if (!(condition)) { printf("FAIL line %d\n", __LINE__); return 1; } \
} while (0)

int main(void)
{
    AlignedBlock p, source;
    int expected[8];

    /* 0x0f0a0: byte/WORD/DWORD copies and signed projection inputs. */
    memset(&p, 0, sizeof(p));
    memset(&source, 0, sizeof(source));
    put32(&source, 0x00, 0x12340000UL);
    put32(&source, 0x04, 0x01000000UL);
    put32(&source, 0x08, 0x00200000UL);
    put32(&source, 0x50, 0);
    put32(&source, 0x54, 0x88000010UL);
    put8(&source, 0x22, 0x71);
    put16(&source, 0x34, 0x2345);
    put16(&source, 0x36, 0x3456);
    put16(&source, 0x3a, 0x0200);
    put16(&source, 0x70, 0x0200);
    put32(&p, 0x54, 0x30);
    put32(&p, 0x5c, (unsigned long)&source);
    put16(&p, 0x38, 0x0321);
    projection_result = 16;
    reset_calls();
    eng1_native_code_target_00f0a0(&p);
    REQUIRE(get32(&p, 0x00) == 0x12340000UL);
    REQUIRE(get32(&p, 0x04) == 0x010e0000UL);
    REQUIRE(get32(&p, 0x08) == 0x00200000UL);
    REQUIRE(get32(&p, 0x54) == 0x8c000030UL);
    REQUIRE(get8(&p, 0x22) == 0x71);
    REQUIRE(get16(&p, 0x34) == 0x2345);
    REQUIRE(get16(&p, 0x36) == 0x3456);
    REQUIRE(get16(&p, 0x3a) == 0x01f8);
    REQUIRE(get16(&p, 0x3c) == 0x01f8);
    REQUIRE(get16(&p, 0x70) == 0x0101);
    REQUIRE(get16(&p, 0x72) == 0x40);
    expected[0] = 1;
    REQUIRE(expect_calls(expected, 1));

    /* 0x0f180: the zero global limit retains the paired flag transition. */
    memset(&p, 0, sizeof(p));
    memset(&source, 0, sizeof(source));
    put32(&source, 0x00, 0x11110000UL);
    put32(&source, 0x04, 0x02000000UL);
    put32(&source, 0x08, 0x22220000UL);
    put32(&source, 0x54, 0x88000010UL);
    put16(&source, 0x3a, 0x0100);
    put16(&source, 0x70, 0x0300);
    put32(&p, 0x5c, (unsigned long)&source);
    put32(&p, 0x54, 1);
    put16(&p, 0x38, 0x0456);
    eng1_native_cell_0718e0 = 0;
    projection_result = 2;
    cap_result = 0x20;
    reset_calls();
    eng1_native_code_target_00f180(&p);
    REQUIRE(get32(&p, 0x00) == 0x11110000UL);
    REQUIRE(get32(&p, 0x04) == 0x02000000UL);
    REQUIRE(get32(&p, 0x08) == 0x22220000UL);
    REQUIRE(get32(&p, 0x54) == 0x8c000010UL);
    REQUIRE(get16(&p, 0x3a) == 0x00ff);
    REQUIRE(get16(&p, 0x70) == 0x0201);
    REQUIRE(get16(&p, 0x72) == 0x40);
    REQUIRE(get16(&p, 0x4a) == 0);
    REQUIRE(get16(&p, 0x26) == 0 && get16(&source, 0x26) == 0);
    REQUIRE(get32(&source, 0x54) == 0x88000010UL);
    expected[0] = 1; expected[1] = 2;
    REQUIRE(expect_calls(expected, 2));

    /* 0x0f180: positive world limit updates both render words and signed step state. */
    memset(&p, 0, sizeof(p));
    memset(&source, 0, sizeof(source));
    put32(&source, 0x00, 0x01000000UL);
    put32(&source, 0x04, 0x00140000UL);
    put32(&source, 0x08, 0x00300000UL);
    put16(&source, 0x3a, 0x0200);
    put16(&source, 0x70, 5);
    put32(&p, 0x5c, (unsigned long)&source);
    put16(&p, 0x38, 0);
    eng1_native_cell_0718e0 = 10;
    eng1_native_cell_0716a8 = 0x44;
    eng1_native_cell_071718 = 5;
    projection_result = 0;
    cap_result = 0;
    reset_calls();
    eng1_native_code_target_00f180(&p);
    REQUIRE(get32(&p, 0x04) == 0x00130000UL);
    REQUIRE(get32(&p, 0x08) == 0x002c0000UL);
    REQUIRE(get32(&p, 0x54) == 0x80050001UL);
    REQUIRE(get32(&source, 0x54) == 0x00030000UL);
    REQUIRE(get16(&source, 0x26) == 10 && get16(&p, 0x26) == 9);
    REQUIRE(get16(&p, 0x70) == 0x0104 && get16(&p, 0x72) == 0x44);
    REQUIRE(get16(&p, 0x3c) == 0x225);
    expected[0] = 1; expected[1] = 2;
    REQUIRE(expect_calls(expected, 2));
    eng1_native_cell_0718e0 = 0;

    /* 0x0f380 is an initializer with no callback or helper dependency. */
    memset(&p, 0, sizeof(p));
    put32(&p, 0x50, 0x1234);
    put32(&p, 0x54, 0x01000000UL);
    put8(&p, 0x22, 0x55);
    put16(&p, 0x34, 0x9876);
    reset_calls();
    eng1_native_code_target_00f380(&p);
    REQUIRE(get32(&p, 0x50) == 0x10000UL);
    REQUIRE(get32(&p, 0x54) == 0x11000604UL);
    REQUIRE(get16(&p, 0x4a) == 0x10);
    REQUIRE(get8(&p, 0x22) == 0);
    REQUIRE(get16(&p, 0x34) == 0);
    REQUIRE(expect_calls(expected, 0));

    /* 0x0f3b0: preserved y on source flag bit 3 and a separate 0x80 step. */
    memset(&p, 0, sizeof(p));
    memset(&source, 0, sizeof(source));
    put32(&source, 0x00, 0x45670000UL);
    put32(&source, 0x04, 0x01000000UL);
    put32(&source, 0x08, 0x12340000UL);
    put32(&source, 0x50, 8);
    put16(&source, 0x3a, 0x0250);
    put32(&p, 0x04, 0x03000000UL);
    put32(&p, 0x5c, (unsigned long)&source);
    put16(&p, 0x38, 5);
    put16(&p, 0x3c, 0x55);
    projection_result = 32;
    reset_calls();
    eng1_native_code_target_00f3b0(&p);
    REQUIRE(get32(&p, 0x00) == 0x45670000UL);
    REQUIRE(get32(&p, 0x04) == 0x03200000UL);
    REQUIRE(get32(&p, 0x08) == 0x125c0000UL);
    REQUIRE(get16(&p, 0x3a) == 0x0240);
    REQUIRE(get16(&p, 0x3c) == 0x55);
    REQUIRE(get16(&p, 0x72) == 0x80);
    expected[0] = 1;
    REQUIRE(expect_calls(expected, 1));

    /* 0x0f430: exact bounded-RNG and frame callback order. */
    memset(&p, 0, sizeof(p));
    reset_calls();
    add_random(4, 1);
    add_random(2, 1);
    add_random(0x80, 7);
    add_random(0x2000, 0x123);
    raw_random_result = 0x1ff;
    eng1_native_code_target_00f430(&p);
    REQUIRE(get16(&p, 0x124) == 3 && get16(&p, 0x126) == 4);
    REQUIRE(get8(&p, 0x96) == 0xff && get8(&p, 0x1c) == 0x41);
    REQUIRE(get8(&p, 0x94) == 0xff && get16(&p, 0x4a) == 0x67);
    REQUIRE(get8(&p, 0x95) == 0xff);
    REQUIRE(get32(&p, 0xbc) == 0xdff00UL);
    REQUIRE(get32(&p, 0xb8) == 0 && get32(&p, 0xc0) == 0);
    REQUIRE(get32(&p, 0xc4) == 0x8123UL);
    expected[0] = 3; expected[1] = 5; expected[2] = 6; expected[3] = 7;
    expected[4] = 3; expected[5] = 3; expected[6] = 4; expected[7] = 3;
    REQUIRE(expect_calls(expected, 8));
    REQUIRE(call_args[0] == 4 && call_args[4] == 2 && call_args[5] == 0x80);
    REQUIRE(call_args[7] == 0x2000);
    REQUIRE(random_index == random_count);
    REQUIRE(raw_random_calls == 1);

    /* 0x0f4f0: common update path and live final flag merge. */
    memset(&p, 0, sizeof(p));
    put8(&p, 0x25, 1);
    put8(&p, 0x94, 0x10);
    put32(&p, 0x54, 0x20);
    put32(&p, 0x04, 0x01000000UL);
    eng1_anchor_0734f4 = 0;
    completion_result = 1;
    reset_calls();
    eng1_native_code_target_00f4f0(&p);
    REQUIRE(get32(&p, 0x54) == 0x38);
    REQUIRE(get16(&p, 0x28) == 0 && get8(&p, 0x97) == 0);
    expected[0] = 7; expected[1] = 11;
    REQUIRE(expect_calls(expected, 2));

    /* 0x0f4f0: selector 1 takes the first tile-state branch and uses frame 0x15. */
    memset(&p, 0, sizeof(p));
    put32(&p, 0x08, 0x00080000UL);
    put16(&p, 0x2a, 1);
    put16(&p, 0x38, 4);
    put32(&p, 0xe0, 0x00100000UL);
    tile_result = 3;
    eng1_words_0713bc[3] = 0;
    completion_result = 1;
    eng1_anchor_0734f4 = 0;
    reset_calls();
    eng1_native_code_target_00f4f0(&p);
    REQUIRE(get32(&p, 0x08) == 0x00100000UL);
    REQUIRE(get16(&p, 0x2a) == 0 && get8(&p, 0x94) == 0xff);
    expected[0] = 8; expected[1] = 21; expected[2] = 10;
    expected[3] = 20; expected[4] = 11;
    REQUIRE(expect_calls(expected, 5));
    REQUIRE(call_args[0] == (unsigned long)&p);
    REQUIRE(call_args[3] == 0x15);

    /* 0x0fd00: selector 1 consumes one raw draw, then one bounded draw. */
    memset(&p, 0, sizeof(p));
    put16(&p, 0x2a, 1);
    reset_calls();
    add_random(0x2000, 0x34);
    raw_random_result = 0x12;
    eng1_native_code_target_00fd00(&p);
    REQUIRE(get8(&p, 0x94) == 1 && get8(&p, 0x95) == 0);
    REQUIRE(get32(&p, 0x54) == 0x10000000UL);
    REQUIRE(get32(&p, 0xbc) == 0x61200UL);
    REQUIRE(get32(&p, 0xb8) == 0x18480UL);
    REQUIRE(get32(&p, 0xc0) == 0x2034UL);
    REQUIRE(get32(&p, 0xe0) == 0 && get32(&p, 0xd8) == 6);
    expected[0] = 4; expected[1] = 3;
    REQUIRE(expect_calls(expected, 2));
    REQUIRE(call_args[1] == 0x2000);
    REQUIRE(random_index == random_count);
    REQUIRE(raw_random_calls == 1);

    /* 0x0fd00: selector 2 initializes its paired vertical state and directions. */
    memset(&p, 0, sizeof(p));
    put16(&p, 0x2a, 2);
    reset_calls();
    add_random(0x1000, 0x56);
    raw_random_result = 0x20;
    eng1_native_code_target_00fd00(&p);
    REQUIRE(get8(&p, 0x94) == 0 && get8(&p, 0x95) == 0);
    REQUIRE(get32(&p, 0xbc) == 0x42000UL);
    REQUIRE(get32(&p, 0xb8) == 0x10800UL);
    REQUIRE(get32(&p, 0xc0) == 0x1056UL);
    REQUIRE(get32(&p, 0xac) == 0x20000UL && get32(&p, 0xb0) == 0x1000UL);
    REQUIRE(get32(&p, 0xe0) == 0xfffffff4UL && get32(&p, 0xd8) == 0);
    expected[0] = 4; expected[1] = 3;
    REQUIRE(expect_calls(expected, 2));
    REQUIRE(random_index == random_count);
    REQUIRE(raw_random_calls == 1);

    /* 0x0fe60: paired movement cancels, while the two WORD steps advance. */
    memset(&p, 0, sizeof(p));
    put32(&p, 0x04, 0x10000000UL);
    put16(&p, 0x70, 10);
    put32(&p, 0xd8, 0x1234);
    put16(&p, 0x74, 20);
    put32(&p, 0xe0, 0x12345678UL);
    put32(&p, 0xa0, 0x20000000UL);
    collision_result = 0;
    reset_calls();
    eng1_native_code_target_00fe60(&p);
    REQUIRE(get32(&p, 0x04) == 0x10000000UL);
    REQUIRE(get32(&p, 0xa0) == 0x20000000UL);
    REQUIRE(get16(&p, 0x70) == 0x123e);
    REQUIRE(get16(&p, 0x74) == 0x568c);
    expected[0] = 14;
    REQUIRE(expect_calls(expected, 1));

    /* 0x0fe60: event upper-word preservation and exact two-spawn coordinate order. */
    memset(&p, 0, sizeof(p));
    memset(spawn_results, 0, sizeof(spawn_results));
    put32(&p, 0x00, 0x00400000UL);
    put32(&p, 0x04, 0x00500000UL);
    put32(&p, 0x08, 0x00600000UL);
    put32(&p, 0xa0, 0x01000000UL);
    eng1_words_071714[0] = 0x1234;
    spawn_results[1] = &source;
    collision_result = 1;
    callback_result = 0xabcd0000UL;
    reset_calls();
    add_random(0x20, 0x10);
    add_random(0x20, 0x1f);
    add_random(10, 7);
    eng1_native_code_target_00fe60(&p);
    REQUIRE(spawn_count == 2);
    REQUIRE(spawn_coordinates[0][0] == 0x00400000UL);
    REQUIRE(spawn_coordinates[0][1] == 0x00500000UL);
    REQUIRE(spawn_coordinates[0][2] == 0x00400000UL);
    REQUIRE(spawn_coordinates[1][0] == 0x004f0000UL);
    REQUIRE(spawn_coordinates[1][1] == 0x00500000UL);
    REQUIRE(spawn_coordinates[1][2] == 0x00400000UL);
    REQUIRE(get32(&source, 0x4c) == 17);
    REQUIRE(get32(&p, 0x04) == 0x00500000UL);
    REQUIRE(get32(&p, 0xa0) == 0x01000000UL);
    expected[0] = 14; expected[1] = 15; expected[2] = 16;
    expected[3] = 17; expected[4] = 3; expected[5] = 3;
    expected[6] = 17; expected[7] = 3; expected[8] = 19;
    REQUIRE(expect_calls(expected, 9));
    REQUIRE(call_args[2] == 0xabcdd234UL);
    REQUIRE(call_args[4] == 0x20 && call_args[5] == 0x20 && call_args[7] == 10);

    /* 0x102c0: screen-cell math precedes the bit-1 removal transition. */
    memset(&p, 0, sizeof(p));
    put32(&p, 0x00, 0x00400000UL);
    put32(&p, 0x08, 0x02000000UL);
    collision_result = 2;
    reset_calls();
    eng1_native_code_target_0102c0(&p);
    REQUIRE(get16(&p, 0x70) == 0x400);
    REQUIRE(get16(&p, 0x72) == 0x200);
    expected[0] = 14; expected[1] = 19;
    REQUIRE(expect_calls(expected, 2));

    /* The native inline row supplies owner bits, while actor+58 is a byte offset. */
    {
        unsigned short record[16];
        memset(record, 0xff, sizeof(record));
        memset(&p, 0, sizeof(p));
        g_kind_info[0].owner = record;
        put32(&p, 0x04, 0x05000000UL);
        put32(&p, 0x58, 4UL);
        put16(&p, 0x38, 0);
        reset_calls();
        eng1_native_code_target_00f4f0(&p);
        REQUIRE(record[8] == 0xfeffU);
        REQUIRE(record[7] == 0xffffU && record[9] == 0xffffU);
    }

    puts("bounded spatial/effect callback spies: OK");
    return 0;
}
