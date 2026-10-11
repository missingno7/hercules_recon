typedef struct Eng3MiddleActor Eng3MiddleActor;
typedef struct Actor Actor;
/* The full shared actor tag is defined by its users; only its native stride
 * is needed here. No conflicting opaque member declaration is supplied. */
enum { ENG3_MIDDLE_ACTOR_STRIDE = 0xec };

extern unsigned short g_eng3_middle_limit_primary;
extern Eng3MiddleActor *g_eng3_middle_actor_pool;
extern int remove_actor_variant(Eng3MiddleActor *actor);
extern void reset_eng3_middle_actor(Eng3MiddleActor *actor);
extern void clear_actor(Actor *actor);

void eng3_process_middle_actor_pool(void)
{
    int index;
    int byte_offset;
    Eng3MiddleActor *actor;

    index = 0;
    if (g_eng3_middle_limit_primary <= index)
        return;

    byte_offset = 0;
    do {
        actor = (Eng3MiddleActor *)((unsigned char *)g_eng3_middle_actor_pool + byte_offset);
        if (remove_actor_variant(actor) == 0)
            clear_actor((Actor *)actor);
        else
            reset_eng3_middle_actor(actor);

        ++index;
        byte_offset += ENG3_MIDDLE_ACTOR_STRIDE;
    } while (index < g_eng3_middle_limit_primary);
}
