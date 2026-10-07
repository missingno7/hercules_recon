#define CLEANUP_HELPERS_SEMANTIC_FIXTURE
#include "../calibration/cleanup_helpers.c"
#include <stdio.h>

typedef struct CallRecord {
    int operation;
    Actor *actor;
} CallRecord;

static CallRecord calls[16];
static int call_count;
static int clear_parent_render_flags;
static int check_count;

static void record_call(int operation, Actor *p)
{
    if (call_count < 16) {
        calls[call_count].operation = operation;
        calls[call_count].actor = p;
    }
    ++call_count;
    if (operation == 1 && clear_parent_render_flags) p->render_flags = 0;
}

void release_actor_resource_state(Actor *p)
{
    record_call(1, p);
}

void release_actor_token(Actor *p)
{
    record_call(2, p);
}

static int expect(int condition)
{
    ++check_count;
    if (!condition) return 1;
    return 0;
}

static void reset_calls(void)
{
    int i;
    call_count = 0;
    clear_parent_render_flags = 0;
    for (i = 0; i < 16; ++i) {
        calls[i].operation = 0;
        calls[i].actor = 0;
    }
}

int main(void)
{
    Actor parent = {0};
    Actor child = {0};
    int failures = 0;

    parent.x = 0x12345678;
    parent.flags = 0x2468;
    parent.child = &child;
    parent.token_low = 7;
    parent.render_flags = 0x10000800;
    reset_calls();
    clear_actor(&parent);
    failures += expect(call_count == 0);
    failures += expect(parent.x == 0x12345678 && parent.flags == 0x2468);
    failures += expect(parent.child == &child && parent.token_low == 7);

    parent.render_flags = 0;
    reset_calls();
    clear_actor(&parent);
    failures += expect(call_count == 2);
    failures += expect(calls[0].operation == 1 && calls[0].actor == &parent);
    failures += expect(calls[1].operation == 2 && calls[1].actor == &parent);
    failures += expect(parent.child == &child && parent.x == 0x12345678);

    parent.render_flags = 0x00800000;
    child.render_flags = 0;
    child.x = 0x4321;
    clear_parent_render_flags = 1;
    reset_calls();
    clear_parent_render_flags = 1;
    clear_actor(&parent);
    failures += expect(call_count == 5);
    failures += expect(calls[0].operation == 1 && calls[0].actor == &parent);
    failures += expect(calls[1].operation == 2 && calls[1].actor == &parent);
    failures += expect(calls[2].operation == 1 && calls[2].actor == &child);
    failures += expect(calls[3].operation == 2 && calls[3].actor == &child);
    failures += expect(calls[4].operation == 2 && calls[4].actor == &child);
    failures += expect(parent.render_flags == 0 && parent.child == &child);
    failures += expect(child.x == 0x4321);

    parent.render_flags = 0x00800000;
    parent.child = 0;
    reset_calls();
    clear_actor(&parent);
    failures += expect(call_count == 2);
    failures += expect(calls[0].actor == &parent && calls[1].actor == &parent);

    parent.render_flags = 0x10800000;
    parent.child = &child;
    reset_calls();
    clear_actor(&parent);
    failures += expect(call_count == 0);
    failures += expect(parent.child == &child);

    if (failures) return 1;
    printf("checks=%d failures=0\n", check_count);
    return 0;
}
