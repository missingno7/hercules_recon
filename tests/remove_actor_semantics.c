#include <stdio.h>
#include <string.h>
#include "../calibration/remove_actor.c"

enum {
    EV_PENDING = 1, EV_CLEAR_COMMAND, EV_ATTACHMENT, EV_CHILD, EV_BUFFER,
    EV_CLEAN_294B0, EV_CLEAN_29620, EV_REFUSAL, EV_RELEASE, EV_CLEAR
};

KindInfo g_kind_info[8];
signed char g_special_kind;
static int events[32];
static int event_count;
static int checks;
static int failures;
static Actor *current_actor;
static Sidecar *callback_sidecar;
static unsigned short callback_kind;
static int mutate_sidecar_flags;

#define CHECK(v) do { ++checks; if (!(v)) { ++failures; if (failures < 12) printf("FAIL line %d\n", __LINE__); } } while (0)

static void record_event(int event, Actor *p)
{
    CHECK(event_count < 32);
    if (event_count < 32) events[event_count++] = event;
    if (event == EV_CLEAR) {
        if (p) CHECK(p->kind == 0);
    } else if (p) CHECK(p->kind == callback_kind);
    else if (current_actor) CHECK(current_actor->kind == callback_kind);
    if (event == EV_CHILD && mutate_sidecar_flags && callback_sidecar)
        callback_sidecar->flags = 0xa123;
}

static void expect_events(const int *expected, int count)
{
    int i;
    CHECK(event_count == count);
    for (i = 0; i < count && i < event_count; ++i) CHECK(events[i] == expected[i]);
}

static void reset_trace(Actor *p, unsigned short expected_kind)
{
    event_count = 0;
    current_actor = p;
    callback_kind = expected_kind;
    callback_sidecar = 0;
    mutate_sidecar_flags = 0;
}

void clear_actor(Actor *p)
{
    record_event(EV_CLEAR, p);
    CHECK(p->kind == 0);
}

void clear_command_high_bit(Actor *p) { record_event(EV_CLEAR_COMMAND, p); }
void detach_child(Actor *p) { record_event(EV_CHILD, p); }
void detach_attachment(Actor *p) { record_event(EV_ATTACHMENT, p); }
void detach_buffer(Actor *p) { record_event(EV_BUFFER, p); }
void actor_cleanup_294b0(Actor *p) { record_event(EV_CLEAN_294B0, p); }
void actor_cleanup_29620(Actor *p) { record_event(EV_CLEAN_29620, p); }
void actor_refusal_296b0(Actor *p) { record_event(EV_REFUSAL, p); }
void release_pending_action(void *p)
{
    record_event(EV_PENDING, current_actor);
    CHECK(p == current_actor->pending_action);
}
int release_actor(Actor *p)
{
    record_event(EV_RELEASE, p);
    return 7;
}

static void init_actor(Actor *p, unsigned short kind)
{
    memset(p, 0, sizeof(*p));
    p->kind = kind;
    p->profile = 3;
    p->sidecar_word = 0x0011;
    p->sidecar_value = 2;
    p->x = 0x12340000L;
    p->y = 0x23450000L;
    p->z = 0x34560000L;
    g_special_kind = 3;
    g_kind_info[3].sidecar_offset = 4;
}

static Sidecar *make_sidecar(long *storage, Actor *p, unsigned short flags)
{
    Sidecar *base = (Sidecar *)storage;
    Sidecar *adjusted = (Sidecar *)((unsigned char *)base + 4);
    memset(storage, 0, 16 * sizeof(long));
    adjusted->flags = flags;
    p->owner_context = base;
    callback_sidecar = adjusted;
    return adjusted;
}

int main(void)
{
    Actor actor;
    long storage[16];
    Sidecar *sidecar;
    int result;
    int bit_index;
    int no_events[1] = {0};
    int refusal_events[] = {EV_REFUSAL};
    int flag_cleanup_events[] = {EV_CLEAN_294B0, EV_CLEAR};
    int main_null_events[] = {EV_PENDING, EV_CLEAR_COMMAND, EV_ATTACHMENT, EV_CHILD,
                              EV_BUFFER, EV_CLEAN_294B0, EV_CLEAN_29620, EV_CLEAR};
    int blocked_events[] = {EV_PENDING};
    int main_sidecar_events[] = {EV_PENDING, EV_CLEAR_COMMAND, EV_CHILD, EV_BUFFER,
                                 EV_ATTACHMENT, EV_CLEAN_294B0, EV_CLEAN_29620, EV_CLEAR};
    int variant_null_events[] = {EV_CHILD, EV_ATTACHMENT, EV_BUFFER, EV_CLEAN_29620, EV_CLEAR};
    int variant_sidecar_events[] = {EV_CHILD, EV_ATTACHMENT, EV_BUFFER, EV_CLEAN_29620, EV_CLEAR};
    int release_events[] = {EV_RELEASE};

    init_actor(&actor, 0);
    reset_trace(&actor, 0);
    CHECK(remove_actor(&actor) == 1);
    expect_events(no_events, 0);

    init_actor(&actor, 1);
    actor.render_flags = 0x00000800;
    actor.pending_action = (void *)0x1234;
    reset_trace(&actor, 1);
    CHECK(remove_actor(&actor) == 0);
    expect_events(refusal_events, 1);
    CHECK(actor.pending_action == (void *)0x1234);

    /* Exercise every one-hot 32-bit render flag. Bit 11 alone reaches the
       refusal callback; each callback observes the still-live kind value. */
    for (bit_index = 0; bit_index < 32; ++bit_index) {
        init_actor(&actor, 1);
        actor.flags = 0x10000000;
        actor.render_flags = 1UL << bit_index;
        reset_trace(&actor, 1);
        result = remove_actor(&actor);
        if (bit_index == 11) {
            CHECK(result == 0);
            expect_events(refusal_events, 1);
            CHECK(actor.kind == 1);
        } else {
            CHECK(result == 1);
            expect_events(flag_cleanup_events, 2);
            CHECK(actor.kind == 0);
        }
    }

    init_actor(&actor, 1);
    actor.flags = 0x10000000;
    actor.pending_action = (void *)0x1234;
    reset_trace(&actor, 1);
    CHECK(remove_actor(&actor) == 1);
    expect_events(flag_cleanup_events, 2);
    CHECK(actor.kind == 0);

    init_actor(&actor, 1);
    actor.pending_action = (void *)0x1234;
    reset_trace(&actor, 1);
    CHECK(remove_actor(&actor) == 1);
    expect_events(main_null_events, 8);
    CHECK(actor.kind == 0);

    init_actor(&actor, 1);
    actor.pending_action = (void *)0x1234;
    sidecar = make_sidecar(storage, &actor, 0x2000);
    reset_trace(&actor, 1);
    callback_sidecar = sidecar;
    CHECK(remove_actor(&actor) == 0);
    expect_events(blocked_events, 1);
    CHECK(actor.kind == 1);

    init_actor(&actor, 1);
    actor.pending_action = (void *)0x1234;
    sidecar = make_sidecar(storage, &actor, 0x8180);
    reset_trace(&actor, 1);
    callback_sidecar = sidecar;
    mutate_sidecar_flags = 1;
    CHECK(remove_actor(&actor) == 1);
    expect_events(main_sidecar_events, 8);
    CHECK(actor.kind == 0);
    CHECK(sidecar->x == 0x12340000L && sidecar->y == 0x23450000L && sidecar->z == 0x34560000L);
    CHECK(sidecar->field_00e == 0x0211);
    CHECK(sidecar->flags == 0x2123);

    init_actor(&actor, 0);
    reset_trace(&actor, 0);
    CHECK(remove_actor_variant(&actor) == 1);
    expect_events(no_events, 0);

    init_actor(&actor, 0x2000);
    reset_trace(&actor, 0x2000);
    CHECK(remove_actor_variant(&actor) == 7);
    expect_events(release_events, 1);

    init_actor(&actor, 0x2000);
    reset_trace(&actor, 0x2000);
    CHECK(remove_actor(&actor) == 7);
    expect_events(release_events, 1);

    init_actor(&actor, 0x4000);
    reset_trace(&actor, 0x4000);
    CHECK(remove_actor_variant(&actor) == 1);
    expect_events(variant_null_events, 5);
    CHECK(actor.kind == 0);

    init_actor(&actor, 0x4000);
    reset_trace(&actor, 0x4000);
    CHECK(remove_actor(&actor) == 1);
    expect_events(variant_null_events, 5);
    CHECK(actor.kind == 0);

    init_actor(&actor, 0x4000);
    sidecar = make_sidecar(storage, &actor, 0x8180);
    reset_trace(&actor, 0x4000);
    callback_sidecar = sidecar;
    mutate_sidecar_flags = 1;
    CHECK(remove_actor_variant(&actor) == 1);
    expect_events(variant_sidecar_events, 5);
    CHECK(actor.kind == 0);
    CHECK(sidecar->x == 0x12340000L && sidecar->y == 0x23450000L && sidecar->z == 0x34560000L);
    CHECK(sidecar->field_00e == 0x0011);
    CHECK(sidecar->flags == 0x2123);

    printf("%d checks; %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
