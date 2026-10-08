#include "../calibration/resource_destroy.c"
#include <stdio.h>

ActorResource g_actor_resources[81];

static unsigned long watched_index;
static unsigned long callback_count;
static void *callback_argument;
static ActorReference *callback_reference_before;
static unsigned short callback_state_before;
static unsigned short callback_unknown_before;
static ActorReference replacement[1];
static int checks;
static int failures;

static void expect(int condition)
{
    ++checks;
    if (!condition) ++failures;
}

void __cdecl release_resource_buffer(void *buffer)
{
    ActorResource *resource = &g_actor_resources[watched_index];
    ++callback_count;
    callback_argument = buffer;
    callback_reference_before = resource->references;
    callback_state_before = resource->residency_state;
    callback_unknown_before = resource->unknown_012;

    /* Force observable writes that the destroy routine must perform afterward. */
    resource->references = &replacement[0];
    resource->residency_state = 0x7171U;
    resource->unknown_012 = 0x8181U;
}

static void set_record(unsigned long index, ActorReference *buffer,
                       unsigned short state, unsigned short unknown)
{
    static char path[] = "DUMMY_RESOURCE_PATH";
    ActorResource *resource = &g_actor_resources[index];
    resource->path = path;
    resource->entry_count = 0x1234U;
    resource->unknown_006 = 0x5678U;
    resource->file_length = 0x89abcdefUL;
    resource->references = buffer;
    resource->residency_state = state;
    resource->unknown_012 = unknown;
}

static void expect_preserved(unsigned long index)
{
    ActorResource *resource = &g_actor_resources[index];
    expect(resource->path != 0);
    expect(resource->entry_count == 0x1234U);
    expect(resource->unknown_006 == 0x5678U);
    expect(resource->file_length == 0x89abcdefUL);
}

int main(void)
{
    ActorReference buffers[4];
    unsigned long calls_before;

    watched_index = 4;
    set_record(watched_index, &buffers[1], 4U, 0xa4a4U);
    resource_destroy(watched_index);
    expect(callback_count == 1);
    expect(callback_argument == &buffers[1]);
    expect(callback_reference_before == &buffers[1]);
    expect(callback_state_before == 4U);
    expect(callback_unknown_before == 0xa4a4U);
    expect(g_actor_resources[watched_index].references == 0);
    expect(g_actor_resources[watched_index].residency_state == 0);
    expect(g_actor_resources[watched_index].unknown_012 == 0);
    expect_preserved(watched_index);

    watched_index = 7;
    set_record(watched_index, &buffers[2], 7U, 0xb7b7U);
    resource_destroy(watched_index);
    expect(callback_count == 2);
    expect(callback_argument == &buffers[2]);
    expect(callback_reference_before == &buffers[2]);
    expect(callback_state_before == 7U);
    expect(callback_unknown_before == 0xb7b7U);
    expect(g_actor_resources[watched_index].references == 0);
    expect(g_actor_resources[watched_index].residency_state == 0);
    expect(g_actor_resources[watched_index].unknown_012 == 0);
    expect_preserved(watched_index);

    watched_index = 9;
    set_record(watched_index, 0, 4U, 0xc9c9U);
    calls_before = callback_count;
    resource_destroy(watched_index);
    expect(callback_count == calls_before);
    expect(g_actor_resources[watched_index].references == 0);
    expect(g_actor_resources[watched_index].residency_state == 0);
    expect(g_actor_resources[watched_index].unknown_012 == 0);
    expect_preserved(watched_index);

    printf("checks=%d failures=%d\n", checks, failures);
    return failures != 0;
}
