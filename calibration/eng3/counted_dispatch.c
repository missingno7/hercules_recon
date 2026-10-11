/* PC-derived count-prefixed dispatch lists. No storage capacity or owner inferred. */
typedef struct DispatchActor {
    unsigned char prefix_000[0x97];
    unsigned char callback_index_097;
} DispatchActor;

typedef void (*ActorCallback)(DispatchActor *context, DispatchActor *actor);

/* Slot zero is the signed count; later 32-bit words hold actor pointers.
   The producer at ENG3 RVA 0x1b9f0 increments the count before storing a slot. */
extern long g_actor_vector_primary[];
extern long g_actor_vector_secondary[];
extern ActorCallback g_actor_callbacks[];

#define DEFINE_DISPATCH(function_name, vector_name)                         \
    void function_name(DispatchActor *context)                             \
    {                                                                     \
        long index = 0;                                                   \
        DispatchActor *actor;                                             \
        long *cursor;                                                     \
                                                                          \
        if ((vector_name)[0] > 0) {                                       \
            cursor = (vector_name);                                       \
            do {                                                          \
                actor = (DispatchActor *)*++cursor;                        \
                ++index;                                                  \
                g_actor_callbacks[actor->callback_index_097](context, actor); \
            } while (index < (vector_name)[0]);                            \
        }                                                                 \
    }

DEFINE_DISPATCH(dispatch_primary, g_actor_vector_primary)
DEFINE_DISPATCH(dispatch_secondary, g_actor_vector_secondary)
