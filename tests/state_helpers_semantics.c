#include "../calibration/cleanup_helpers.c"
#include <stdio.h>

ActorResource g_actor_resources[256];
signed char g_actor_link_map[16];
ActorResourceLinkRow g_actor_link_rows[4096];
ActorTokenReference g_actor_token_references[256];
static int check_count;

static int expect(int condition)
{
    ++check_count;
    if (!condition) return 1;
    return 0;
}

int main(void)
{
    Actor actor = {0};
    ActorReference references[16] = {{0}};
    unsigned short saved_mask;
    int failures = 0;

    actor.token_low = 0x13;
    actor.token_high = 0x72;
    actor.x = 0x12345678;
    actor.render_flags = 0x800000;
    actor.child = (Actor *)0x1234;
    g_actor_token_references[0x13].count = 3;
    release_actor_token(&actor);
    failures += expect(g_actor_token_references[0x13].count == 2);
    failures += expect(actor.token_low == 0xff && actor.token_high == 0xff);
    failures += expect(actor.x == 0x12345678 && actor.render_flags == 0x800000);
    failures += expect(actor.child == (Actor *)0x1234);
    g_actor_token_references[0xff].count = 9;
    actor.token_high = 0x44;
    actor.token_low = 0xff;
    release_actor_token(&actor);
    failures += expect(g_actor_token_references[0xff].count == 9);
    failures += expect(actor.token_high == 0x44 && actor.token_low == 0xff);

    actor.initial_resource_index = 0x1234;
    actor.reference_index_a = 2;
    actor.reference_index_b = 3;
    actor.resource_index = 2;
    actor.category = 0x37;
    actor.link_key_a = 0x12;
    actor.link_key_b = 0x24;
    actor.x = 0x87654321;
    actor.render_flags = 0x13579bdf;
    actor.child = (Actor *)0x5678;
    g_actor_resources[2].references = references;
    g_actor_resources[2].release_mode = 0;
    references[2].count = 1;
    references[3].count = 2;
    g_actor_link_map[2] = 0;
    g_actor_link_map[3] = 3;
    g_actor_link_map[4] = 2;
    g_actor_link_rows[1].live_mask = 0xffff;
    g_actor_link_rows[1].next[0] = 0x13;
    g_actor_link_rows[1].next[3] = 0xffff;
    g_actor_link_rows[2].live_mask = 0xffff;
    g_actor_link_rows[2].next[2] = 0xffff;
    release_actor_resource_state(&actor);
    failures += expect(references[2].count == 0 && references[3].count == 1);
    failures += expect(g_actor_link_rows[1].live_mask == 0x0c);
    failures += expect(g_actor_link_rows[2].live_mask == 0x0b);
    failures += expect(actor.link_key_a == (short)-1 && actor.link_key_b == (short)-1);
    failures += expect(actor.reference_index_a == 0x1234 && actor.resource_index == 0x37);
    failures += expect(actor.x == 0x87654321 && actor.render_flags == 0x13579bdf);
    failures += expect(actor.child == (Actor *)0x5678);

    actor.resource_index = 3;
    actor.link_key_a = 0x35;
    actor.link_key_b = (short)-1;
    actor.reference_index_a = 5;
    actor.initial_resource_index = 0x22;
    actor.category = 0x41;
    references[5].count = 2;
    g_actor_resources[3].references = references;
    g_actor_resources[3].release_mode = 1;
    g_actor_link_map[5] = 1;
    g_actor_link_rows[3].live_mask = 0xffff;
    g_actor_link_rows[3].next[1] = 0xffff;
    saved_mask = g_actor_link_rows[3].live_mask;
    release_actor_resource_state(&actor);
    failures += expect(references[5].count == 1);
    failures += expect(g_actor_link_rows[3].live_mask == saved_mask);
    failures += expect(actor.link_key_a == (short)-1 && actor.link_key_b == (short)-1);
    failures += expect(actor.reference_index_a == 0x22 && actor.resource_index == 0x41);

    actor.resource_index = 0xffff;
    actor.link_key_a = 0x12;
    actor.reference_index_a = 0x33;
    actor.initial_resource_index = 0x44;
    actor.category = 0x55;
    release_actor_resource_state(&actor);
    failures += expect(actor.resource_index == 0xffff && actor.link_key_a == 0x12);
    failures += expect(actor.reference_index_a == 0x33);

    if (failures) return 1;
    printf("checks=%d failures=0\n", check_count);
    return 0;
}
