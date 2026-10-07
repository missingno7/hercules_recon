#include "../calibration/cleanup_helpers_writers.c"
#include <stdio.h>
#include <string.h>
ActorResource g_actor_resources[1];
signed char g_actor_link_map[16];
ActorResourceLinkRow g_actor_link_rows[1];
ActorTokenReference g_actor_token_references[32];
static int checks;
static int expect(int value) { ++checks; return !value; }
int main(void)
{
    Actor actor, before;
    int i, j, failures = 0;
    for (i = 0; i < 32; ++i) {
        memset(&actor, 0x5a, sizeof(actor));
        actor.token_low = (unsigned char)i;
        actor.token_high = 0x72;
        before = actor;
        for (j = 0; j < 32; ++j) {
            g_actor_token_references[j][0] = (short)(j + 3);
            g_actor_token_references[j][1] = (short)(0x2300 + j);
        }
        release_actor_token(&actor);
        failures += expect(actor.token_low == 0xff && actor.token_high == 0xff);
        before.token_low = before.token_high = 0xff;
        failures += expect(memcmp(&actor, &before, sizeof(actor)) == 0);
        for (j = 0; j < 32; ++j) {
            failures += expect(g_actor_token_references[j][0] == j + 3 - (i == j));
            failures += expect(g_actor_token_references[j][1] == 0x2300 + j);
        }
        actor.token_high = 0x44;
        before = actor;
        release_actor_token(&actor);
        failures += expect(memcmp(&actor, &before, sizeof(actor)) == 0);
        failures += expect(g_actor_token_references[i][0] == i + 2);
    }
    actor.token_low = 0;
    g_actor_token_references[0][0] = 0;
    release_actor_token(&actor);
    failures += expect(g_actor_token_references[0][0] == -1);
    actor.token_low = 1;
    g_actor_token_references[1][0] = 0x40;
    release_actor_token(&actor);
    failures += expect(g_actor_token_references[1][0] == 0x3f);
    printf("checks=%d failures=%d\n", checks, failures);
    return failures != 0;
}
