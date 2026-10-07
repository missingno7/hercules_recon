#include <stdio.h>
#include <string.h>
#include "../calibration/actor_pools.c"

static ActorNormal normal_pool[8];
static ActorCore alternate_pool[8];
ActorNormal *g_normal_actor_pool = normal_pool;
ActorCore *g_alternate_actor_pool = alternate_pool;
unsigned short g_normal_cursor_primary;
unsigned short g_normal_limit_primary;
unsigned short g_normal_limit_wrap;
unsigned short g_normal_cursor_alternate;
unsigned short g_normal_limit_wrap_start;
unsigned short g_alternate_cursor_primary;
unsigned short g_alternate_limit_primary;
unsigned short g_alternate_limit_wrap;
unsigned short g_alternate_cursor_alternate;
unsigned short g_alternate_limit_wrap_start;

static unsigned long checks;
static unsigned long failures;

static void expect(int ok, const char *label)
{
    ++checks;
    if (!ok) {
        ++failures;
        if (failures <= 20)
            printf("FAIL %s\n", label);
    }
}

typedef struct InitField InitField;
struct InitField {
    unsigned short offset;
    unsigned char width;
    unsigned long value;
};

#define F(o,w,v) { (unsigned short)(o), (unsigned char)(w), (unsigned long)(v) }
static const InitField alternate_fields[] = {
    F(0x1d,1,0xff),F(0x1e,1,0xff),F(0x1c,1,0xff),F(0x54,4,0x80000000UL),
    F(0x1f,1,0),F(0x22,1,1),F(0x30,2,0),F(0x2e,2,0),F(0x50,4,2),
    F(0x32,2,0xffff),F(0x38,2,0),F(0x36,2,0),F(0x34,2,0),F(0x12,2,0),
    F(0x10,2,0),F(0x0e,2,0),F(0x0c,2,0),F(0x76,2,0),F(0x74,2,0),
    F(0x72,2,0),F(0x70,2,0),F(0x3e,2,0x100),F(0x3c,2,0x100),F(0x3a,2,0x100),
    F(0x8c,2,0),F(0x8a,2,0),F(0x88,2,0),F(0x48,2,0xffff),F(0x42,2,0xffff),
    F(0x46,2,0xffff),F(0x44,2,0xffff),F(0x40,2,0xffff),F(0x92,2,0),F(0x90,2,0),
    F(0x20,1,0xff),F(0x8e,2,0),F(0x23,1,0),F(0x2a,2,0),F(0x6c,4,0),
    F(0x68,4,0),F(0x64,4,0),F(0x58,4,0),F(0x5c,4,0),F(0x60,4,0),
    F(0x08,4,0),F(0x04,4,0),F(0x00,4,0),F(0x4a,2,0x80),F(0x24,1,0x80),
    F(0x2c,2,0),F(0x26,2,0),F(0x21,1,0),F(0x4c,4,0),F(0x25,1,0),
    F(0x18,2,0),F(0x16,2,0),F(0x14,2,0),F(0x28,2,0)
};
static const InitField middle_fields[] = {
    F(0x94,1,0),F(0x96,1,0),F(0x95,1,0),F(0x97,1,0),F(0x9a,2,0),F(0x98,2,0),
    F(0xa4,4,0),F(0xa0,4,0),F(0x9c,4,0),F(0xb4,4,0),F(0xb0,4,0),F(0xac,4,0),
    F(0xa8,4,0),F(0xc4,4,0),F(0xc0,4,0),F(0xbc,4,0),F(0xb8,4,0),F(0xd4,4,0),
    F(0xd0,4,0),F(0xcc,4,0),F(0xc8,4,0),F(0xe0,4,0),F(0xdc,4,0),F(0xd8,4,0),
    F(0xe8,4,0),F(0xe4,4,0)
};
static const InitField normal_fields[] = {
    F(0xed,1,0),F(0xec,1,0),F(0xee,2,1),F(0xf8,4,0),F(0xf4,4,0),F(0xf0,4,0),
    F(0x104,4,0),F(0x100,4,0),F(0xfc,4,0),F(0x10c,4,0),F(0x108,4,0),F(0x110,4,0),
    F(0x114,2,0xffff),F(0x116,2,0),F(0x118,4,0),F(0x11c,2,0xffff),F(0x11e,2,0),
    F(0x120,4,0),F(0x124,2,0),F(0x126,2,0),F(0x130,4,0),F(0x12c,4,0),F(0x128,4,0)
};
#undef F

static void verify_fields(const void *object, const InitField *fields, int count, const char *label)
{
    const unsigned char *bytes = (const unsigned char *)object;
    int i;
    for (i = 0; i < count; ++i) {
        unsigned long actual = 0;
        memcpy(&actual, bytes + fields[i].offset, fields[i].width);
        expect(actual == fields[i].value, label);
    }
}

static void reset_normal(unsigned short free_a, unsigned short free_b)
{
    int i;
    memset(normal_pool, 0, sizeof(normal_pool));
    for (i = 0; i < 8; ++i)
        normal_pool[i].base.base.kind = 1;
    normal_pool[free_a].base.base.kind = 0;
    normal_pool[free_b].base.base.kind = 0;
    g_normal_cursor_primary = 2;
    g_normal_limit_primary = 4;
    g_normal_limit_wrap = 4;
    g_normal_cursor_alternate = 5;
    g_normal_limit_wrap_start = 4;
}

static void reset_alternate(unsigned short free_a, unsigned short free_b)
{
    int i;
    memset(alternate_pool, 0, sizeof(alternate_pool));
    for (i = 0; i < 8; ++i)
        alternate_pool[i].kind = 1;
    alternate_pool[free_a].kind = 0;
    alternate_pool[free_b].kind = 0;
    g_alternate_cursor_primary = 2;
    g_alternate_limit_primary = 4;
    g_alternate_limit_wrap = 4;
    g_alternate_cursor_alternate = 5;
    g_alternate_limit_wrap_start = 4;
}

static void test_allocators(void)
{
    unsigned int word;
    ActorNormal *normal_result;
    ActorCore *alternate_result;
    int i;

    for (word = 0; word < 0x10000; ++word) {
        reset_normal(3, 4);
        reset_alternate(3, 4);
        normal_result = allocate_normal_actor((ActorKind)word);
        alternate_result = allocate_alternate_actor((ActorKind)word);
        if (word & 0x8000) {
            expect(normal_result == &normal_pool[3], "normal bit15 primary allocation");
            expect(g_normal_cursor_primary == 3 && g_normal_cursor_alternate == 5, "normal primary cursor update");
            expect(alternate_result == &alternate_pool[3], "alternate bit15 primary allocation");
            expect(g_alternate_cursor_primary == 3 && g_alternate_cursor_alternate == 5, "alternate primary cursor update");
        } else {
            expect(normal_result == &normal_pool[4], "normal clear-bit wrap allocation");
            expect(g_normal_cursor_primary == 2 && g_normal_cursor_alternate == 4, "normal wrap cursor update");
            expect(alternate_result == &alternate_pool[4], "alternate clear-bit wrap allocation");
            expect(g_alternate_cursor_primary == 2 && g_alternate_cursor_alternate == 4, "alternate wrap cursor update");
        }
    }

    reset_normal(3, 4);
    normal_result = allocate_normal_actor(0x8001);
    expect(normal_result == &normal_pool[3], "normal selected slot first use");
    normal_result = allocate_normal_actor(0x8001);
    expect(normal_result == &normal_pool[3], "normal cursor reuses newly free slot");
    reset_alternate(3, 4);
    alternate_result = allocate_alternate_actor(0x8001);
    expect(alternate_result == &alternate_pool[3], "alternate selected slot first use");
    alternate_result = allocate_alternate_actor(0x8001);
    expect(alternate_result == &alternate_pool[3], "alternate cursor reuses newly free slot");

    for (i = 0; i < 8; ++i) {
        normal_pool[i].base.base.kind = 1;
        alternate_pool[i].kind = 1;
    }
    g_normal_cursor_primary = 2; g_normal_limit_primary = 4; g_normal_limit_wrap = 4;
    g_normal_cursor_alternate = 5; g_normal_limit_wrap_start = 5;
    g_alternate_cursor_primary = 2; g_alternate_limit_primary = 4; g_alternate_limit_wrap = 4;
    g_alternate_cursor_alternate = 5; g_alternate_limit_wrap_start = 5;
    expect(allocate_normal_actor(0x8000) == 0 && g_normal_cursor_primary == 2, "normal primary count-bound failure");
    expect(allocate_normal_actor(0x0000) == 0 && g_normal_cursor_alternate == 5, "normal wrap count-bound failure");
    expect(allocate_alternate_actor(0x8000) == 0 && g_alternate_cursor_primary == 2, "alternate primary count-bound failure");
    expect(allocate_alternate_actor(0x0000) == 0 && g_alternate_cursor_alternate == 5, "alternate wrap count-bound failure");

    g_normal_cursor_primary = 6; g_normal_limit_primary = 6; g_normal_limit_wrap = 6;
    g_normal_cursor_alternate = 6; g_normal_limit_wrap_start = 6;
    g_alternate_cursor_primary = 6; g_alternate_limit_primary = 6; g_alternate_limit_wrap = 6;
    g_alternate_cursor_alternate = 6; g_alternate_limit_wrap_start = 6;
    expect(allocate_normal_actor(0x8000) == 0, "normal empty primary interval");
    expect(allocate_normal_actor(0x0000) == 0, "normal empty alternate interval");
    expect(allocate_alternate_actor(0x8000) == 0, "alternate empty primary interval");
    expect(allocate_alternate_actor(0x0000) == 0, "alternate empty alternate interval");
}

static void test_initializers(void)
{
    ActorCore alternate;
    ActorMiddle middle;
    ActorNormal normal;
    memset(&alternate, 0xa5, sizeof(alternate));
    memset(&middle, 0xa5, sizeof(middle));
    memset(&normal, 0xa5, sizeof(normal));

    initialize_alternate_actor(&alternate);
    verify_fields(&alternate, alternate_fields, sizeof(alternate_fields)/sizeof(alternate_fields[0]), "alternate initializer field value");
    expect(((unsigned char *)&alternate)[0x1a] == 0xa5, "alternate initializer preserves untouched field gap");

    initialize_middle_actor(&middle);
    verify_fields(&middle, alternate_fields, sizeof(alternate_fields)/sizeof(alternate_fields[0]), "middle base initializer field value");
    verify_fields(&middle, middle_fields, sizeof(middle_fields)/sizeof(middle_fields[0]), "middle extension initializer field value");
    expect(((unsigned char *)&middle)[0x78] == 0xa5, "middle initializer preserves untouched base gap");

    initialize_normal_actor(&normal);
    verify_fields(&normal, alternate_fields, sizeof(alternate_fields)/sizeof(alternate_fields[0]), "normal alternate-prefix initializer field value");
    verify_fields(&normal, middle_fields, sizeof(middle_fields)/sizeof(middle_fields[0]), "normal middle-prefix initializer field value");
    verify_fields(&normal, normal_fields, sizeof(normal_fields)/sizeof(normal_fields[0]), "normal extension initializer field value");
    expect(((unsigned char *)&normal)[0x78] == 0xa5, "normal initializer preserves untouched base gap");
}

int main(void)
{
    test_allocators();
    test_initializers();
    printf("checks=%lu failures=%lu\n", checks, failures);
    return failures != 0;
}
