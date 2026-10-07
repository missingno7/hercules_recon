#include "../calibration/cleanup_helpers_writers.c"
#include <stdio.h>
#include <string.h>

ActorResource g_actor_resources[81];
signed char g_actor_link_map[16];
ActorResourceLinkRow g_actor_link_rows[256];
ActorTokenReference g_actor_token_references[32];

static int checks;
static int failures;

static void expect(int condition)
{
    ++checks;
    if (!condition) ++failures;
}

static void reset_tables(void)
{
    memset(g_actor_resources, 0, sizeof(g_actor_resources));
    memset(g_actor_link_map, 0, sizeof(g_actor_link_map));
    memset(g_actor_link_rows, 0, sizeof(g_actor_link_rows));
    memset(g_actor_token_references, 0, sizeof(g_actor_token_references));
}

/* Target branches independently test count zero and the saved residency state.
 * Exercise both chain fields, absent buffers, composite masks and signed loads.
 * Interior pointers make the negative reference index a legal fixture address;
 * this does not assert that production indices can be negative. */
static void predicate_matrix(void)
{
    static const short counts[] = {0, 1, 2, 0x40};
    static const unsigned short states[] = {0, 1, 2, 4, 6};
    Actor p, expected;
    ActorReference refs[3];
    int chain, mask, c, s, present;
    for (chain = 0; chain < 2; ++chain)
        for (mask = 1; mask < 16; ++mask)
            for (c = 0; c < 4; ++c)
                for (s = 0; s < 5; ++s)
                    for (present = 0; present < 2; ++present) {
                        reset_tables();
                        memset(&p, 0x5a, sizeof(p));
                        p.resource_index = 3;
                        p.reference_index_a = -1;
                        p.reference_index_b = 1;
                        p.link_key_a = chain == 0 ? (short)(0x20 | mask) : -1;
                        p.link_key_b = chain == 1 ? (short)(0x20 | mask) : -1;
                        p.initial_resource_index = 0x2345;
                        p.category = 0x31;
                        expected = p;
                        expected.link_key_a = expected.link_key_b = -1;
                        expected.reference_index_a = 0x2345;
                        expected.resource_index = 0x31;
                        memset(refs, 0, sizeof(refs));
                        refs[0][0] = refs[2][0] = counts[c];
                        refs[0][1] = 0x1234;
                        refs[2][1] = 0x5678;
                        g_actor_resources[3].references = present ? &refs[1] : 0;
                        g_actor_resources[3].residency_state = states[s];
                        g_actor_link_map[mask] = (signed char)(mask % 4);
                        g_actor_link_rows[2].live_mask = 0xa55a;
                        g_actor_link_rows[2].unknown_002 = 0x1357;
                        g_actor_link_rows[2].next[mask % 4] = 0xffff;
                        release_actor_resource_state(&p);
                        expect(memcmp(&p, &expected, sizeof(p)) == 0);
                        expect(refs[chain == 0 ? 0 : 2][0] ==
                               counts[c] - (present ? 1 : 0));
                        expect(refs[chain == 0 ? 2 : 0][0] == counts[c]);
                        expect(refs[0][1] == 0x1234 && refs[2][1] == 0x5678);
                        expect(g_actor_link_rows[2].live_mask ==
                               ((states[s] == 0 || (present && counts[c] == 1))
                                ? (0xa55a & (15 - mask)) : 0xa55a));
                        expect(g_actor_link_rows[2].unknown_002 == 0x1357);
                        expect(g_actor_resources[3].residency_state == states[s]);
                    }
}

int main(void)
{
    Actor actor, before;
    ActorReference refs[8];

    reset_tables();
    memset(refs, 0, sizeof(refs));
    memset(&actor, 0x5a, sizeof(actor));
    actor.resource_index = 2;
    actor.reference_index_a = -1;
    actor.reference_index_b = 1;
    actor.link_key_a = 0x23;
    actor.link_key_b = 0x51;
    actor.initial_resource_index = 0x1234;
    actor.category = 0x40;
    actor.x = 0x13579bdf;
    actor.render_flags = 0x2468ace0;
    g_actor_resources[2].references = &refs[2];
    g_actor_resources[2].residency_state = 2;
    refs[1][0] = 1;
    refs[1][1] = 0x1357;
    refs[3][0] = 2;
    refs[3][1] = 0x2468;
    g_actor_link_map[1] = 0;
    g_actor_link_map[3] = 0;
    g_actor_link_map[4] = 2;
    g_actor_link_rows[2].live_mask = 0xffff;
    g_actor_link_rows[2].unknown_002 = 0x2222;
    g_actor_link_rows[2].next[0] = 0x34;
    g_actor_link_rows[3].live_mask = 0xffff;
    g_actor_link_rows[3].unknown_002 = 0x3333;
    g_actor_link_rows[3].next[2] = 0xffff;
    g_actor_link_rows[5].live_mask = 0xa55a;
    g_actor_link_rows[5].next[0] = 0xffff;
    release_actor_resource_state(&actor);
    expect(refs[1][0] == 0 && refs[1][1] == 0x1357);
    expect(refs[3][0] == 1 && refs[3][1] == 0x2468);
    expect(g_actor_link_rows[2].live_mask == 0x000c);
    expect(g_actor_link_rows[3].live_mask == 0x000b);
    expect(g_actor_link_rows[5].live_mask == 0xa55a);
    expect(g_actor_link_rows[2].unknown_002 == 0x2222 && g_actor_link_rows[3].unknown_002 == 0x3333);
    expect(actor.link_key_a == (short)-1 && actor.link_key_b == (short)-1);
    expect(actor.reference_index_a == 0x1234 && actor.reference_index_b == 1 && actor.resource_index == 0x40);
    expect(actor.x == 0x13579bdf && actor.render_flags == 0x2468ace0);

    reset_tables();
    memset(refs, 0, sizeof(refs));
    memset(&actor, 0, sizeof(actor));
    actor.resource_index = 6;
    actor.reference_index_a = 0;
    actor.link_key_a = 0x61;
    actor.link_key_b = (short)-1;
    actor.initial_resource_index = 0x22;
    actor.category = 0x31;
    g_actor_resources[6].references = refs;
    g_actor_resources[6].residency_state = 0;
    refs[0][0] = 2;
    refs[0][1] = 0x7654;
    g_actor_link_map[1] = 0;
    g_actor_link_rows[6].live_mask = 0xffff;
    g_actor_link_rows[6].next[0] = 0xffff;
    release_actor_resource_state(&actor);
    expect(refs[0][0] == 1 && refs[0][1] == 0x7654);
    expect(g_actor_link_rows[6].live_mask == 0x000e);
    expect(actor.link_key_a == (short)-1 && actor.resource_index == 0x31);

    reset_tables();
    memset(&actor, 0, sizeof(actor));
    actor.resource_index = 7;
    actor.link_key_a = 0x72;
    actor.link_key_b = (short)-1;
    actor.category = 0x32;
    g_actor_resources[7].residency_state = 2;
    g_actor_link_map[2] = 1;
    g_actor_link_rows[7].live_mask = 0xffff;
    g_actor_link_rows[7].next[1] = 0xffff;
    release_actor_resource_state(&actor);
    expect(g_actor_link_rows[7].live_mask == 0xffff);
    expect(actor.link_key_a == (short)-1 && actor.resource_index == 0x32);

    reset_tables();
    memset(&actor, 0, sizeof(actor));
    actor.resource_index = 8;
    actor.link_key_a = 0x84;
    actor.link_key_b = (short)-1;
    actor.category = 0x33;
    g_actor_resources[8].residency_state = 0;
    g_actor_link_map[4] = 2;
    g_actor_link_rows[8].live_mask = 0xffff;
    g_actor_link_rows[8].next[2] = 0xffff;
    release_actor_resource_state(&actor);
    expect(g_actor_link_rows[8].live_mask == 0x000b);
    expect(actor.link_key_a == (short)-1 && actor.resource_index == 0x33);

    reset_tables();
    memset(refs, 0, sizeof(refs));
    memset(&actor, 0, sizeof(actor));
    actor.resource_index = 9;
    actor.reference_index_a = 2;
    actor.reference_index_b = -3;
    actor.link_key_a = (short)-1;
    actor.link_key_b = (short)-1;
    actor.initial_resource_index = 0x3456;
    actor.category = 0x34;
    g_actor_resources[9].references = refs;
    g_actor_resources[9].residency_state = 2;
    refs[2][0] = 7;
    refs[2][1] = 0x4567;
    release_actor_resource_state(&actor);
    expect(refs[2][0] == 7 && refs[2][1] == 0x4567);
    expect(actor.link_key_a == (short)-1 && actor.link_key_b == (short)-1);
    expect(actor.reference_index_a == 0x3456 && actor.reference_index_b == -3 && actor.resource_index == 0x34);

    reset_tables();
    memset(&actor, 0x6b, sizeof(actor));
    actor.resource_index = 0xffff;
    before = actor;
    release_actor_resource_state(&actor);
    expect(memcmp(&actor, &before, sizeof(actor)) == 0);

    predicate_matrix();
    printf("checks=%d failures=%d\n", checks, failures);
    return failures != 0;
}
