/* Observed recursive clear view; original names/types/TU unproved. */
typedef struct Actor Actor;
struct Actor {
    unsigned char unknown_000[0x54];
    unsigned long render_flags;
    unsigned char unknown_058[8];
    Actor *child;
};
typedef char child_offset_check[(unsigned long)&(((Actor *)0)->child)==0x60 ? 1 : -1];
extern void release_actor_resource_state(Actor *p);
extern void release_actor_token(Actor *p);

void clear_actor(Actor *p)
{
    Actor *child;
    unsigned long render_flags;

    render_flags = p->render_flags;
    if (!(render_flags & 0x10000800)) {
        release_actor_resource_state(p);
        release_actor_token(p);
        if (render_flags & 0x00800000) {
            child = p->child;
            if (child) {
                clear_actor(child);
                release_actor_token(child);
            }
        }
    }
}
