#include <stddef.h>

/* Minimum ENG1/ENG3 actor view supported by these three measured helpers only.
 * The relative byte offset at +0x58 is deliberately opaque here. */
typedef struct ActorDetachView ActorDetachView;
struct ActorDetachView {
    unsigned char reserved_000[0x50];
    unsigned long flags_050;
    unsigned long render_flags_054;
    unsigned char reserved_058[4];
    ActorDetachView *attachment_05c;
    ActorDetachView *child_060;
    unsigned char reserved_064[8];
    unsigned long fx_linked_06c;
};

typedef char actor_detach_view_is_0x70[
    (sizeof(ActorDetachView) == 0x70) ? 1 : -1];
typedef char actor_detach_flags_at_0x50[
    (offsetof(ActorDetachView, flags_050) == 0x50) ? 1 : -1];
typedef char actor_detach_render_at_0x54[
    (offsetof(ActorDetachView, render_flags_054) == 0x54) ? 1 : -1];
typedef char actor_detach_attachment_at_0x5c[
    (offsetof(ActorDetachView, attachment_05c) == 0x5c) ? 1 : -1];
typedef char actor_detach_child_at_0x60[
    (offsetof(ActorDetachView, child_060) == 0x60) ? 1 : -1];
typedef char actor_detach_buffer_at_0x6c[
    (offsetof(ActorDetachView, fx_linked_06c) == 0x6c) ? 1 : -1];

/* Physical helper roles are provisional labels, not recovered original names. */
extern int release_actor(ActorDetachView *actor);
extern void remove_fx_links(ActorDetachView *actor);

void detach_attachment(ActorDetachView *actor)
{
    ActorDetachView *attachment = actor->attachment_05c;

    if (attachment && !(actor->flags_050 & 0x00020000)) {
        if (actor->flags_050 & 0x00010000)
            actor = attachment;
        actor->attachment_05c = 0;
        if (attachment->flags_050 & 0x00010000)
            release_actor(attachment);
    }
}

void detach_child(ActorDetachView *actor)
{
    ActorDetachView *child = actor->child_060;

    if (child && !(actor->flags_050 & 0x40000000)) {
        actor->child_060 = 0;
        actor->render_flags_054 &= ~0x00800000;
        release_actor(child);
    }
}

void detach_buffer(ActorDetachView *actor)
{
    if (!(actor->flags_050 & 0x10000000) && actor->fx_linked_06c) {
        actor->fx_linked_06c = 0;
        remove_fx_links(actor);
    }
}
