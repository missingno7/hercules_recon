#include <stdio.h>
#include <string.h>
#include "../src/eng1/interpreters.h"

static unsigned long calls;
static unsigned long checks;
static unsigned long failures;
static unsigned short attached_mutation;

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

void __cdecl eng1_guarded_advance_frame(Actor *actor)
{
    ++calls;
    check(actor != 0, "advance_frame receives actor");
    actor->unknown_02a = 0x1357U;
}

void __cdecl eng1_guarded_select_frame(Actor *actor, int index)
{
    ++calls;
    check(calls == 2 && index == 13,
          "select_frame follows advance_frame with selector 13");
    actor->unknown_02a = 0xbeefU;
}

void *__cdecl attach_actor(void *raw_actor, int kind)
{
    Actor *actor = (Actor *)raw_actor;
    unsigned char *bytes = (unsigned char *)raw_actor;

    ++calls;
    check(calls == 3 && kind == 0x2022,
          "attach_actor follows both frame helpers with kind 0x2022");
    check(actor->unknown_02a == 0 &&
          *(unsigned long *)(void *)(bytes + 0xdc) == 0xbeef0000UL,
          "saved state is read after both helpers and then cleared");
    check(*(unsigned long *)(void *)(bytes + 0x50) == 0x00000a02UL &&
          *(unsigned short *)(void *)(bytes + 0x124) == 0 &&
          *(unsigned short *)(void *)(bytes + 0xee) == 3,
          "all target stores precede attach_actor");
    attached_mutation = actor->unknown_02a;
    actor->unknown_02a = 0x8765U;
    return (void *)1;
}

extern void __cdecl eng1_native_code_target_03e220(Actor *actor);

int main(void)
{
    unsigned char storage[0x134];
    memset(storage, 0xa5, sizeof(storage));
    calls = 0;
    failures = 0;
    checks = 0;
    attached_mutation = 0;
    eng1_native_code_target_03e220((Actor *)storage);
    check(calls == 3, "all three helper calls execute in order");
    check(attached_mutation == 0,
          "attach callback observes target-cleared actor state");
    check(*(unsigned long *)(void *)(storage + 0xdc) == 0xbeef0000UL &&
          *(unsigned short *)(void *)(storage + 0x2a) == 0x8765U,
          "attach callback mutations survive target return");
    printf("source-only helper-order controls=1 checks=%lu failures=%lu\n",
           checks, failures);
    return failures ? 1 : 0;
}
