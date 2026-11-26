/* ENG1 275d0/277e0: middle-slot first-fit/wrap allocation and coordinate setup.
 * Reuses the measured TITLE twin's direct-root scan and assignment ordering.
 * The sparse0xec view covers only observed fields; unknown bytes retain state.
 * WORD cursors/limits and the DWORD descriptor root use existing actual owners.
 * Allocation saves the found index without reserving it. The factory initializes
 * that slot before storing kind|4000, coordinates, category and image. Reloads
 * between stores are intentional. Neither view establishes native capacities.
 */
extern "C" {
typedef struct MiddleFactoryDescriptor {
    unsigned long d0;
    unsigned char b4, b5, b6, b7;
} MiddleFactoryDescriptor;




typedef struct MiddleActorView {
    long d00, d04;
    unsigned long d08;
    unsigned char unknown_0c[0x16];
    unsigned char b22;
    unsigned char unknown_23[0x0b];
    unsigned short w2e;
    unsigned char unknown_30[0x34];
    unsigned long d64;
    unsigned char unknown_68[0x34];
    unsigned long d9c, da0, da4;
    unsigned char unknown_a8[0x44];
} MiddleActorView;
extern void *g_middle_actor_pool;
extern unsigned short g_configuration_word_713ac,g_configuration_word_71432,g_configuration_word_713a0,g_configuration_word_713b8,g_configuration_word_713aa;
extern unsigned long g_configuration_dword_71390;
void initialize_middle_actor(void*);
#define middle_descriptors ((MiddleFactoryDescriptor*)g_configuration_dword_71390)
typedef char middle_slot_ec[sizeof(MiddleActorView)==0xec?1:-1];
typedef char descriptor8[sizeof(MiddleFactoryDescriptor)==8?1:-1];
void *allocate_middle_actor(unsigned short flags)
{
    int i;

    if (flags & 0x8000) {
        for (i = g_configuration_word_713ac; i < g_configuration_word_71432; i++) {
            if (((MiddleActorView *)g_middle_actor_pool)[i].w2e == 0)
                break;
        }
        if (i < g_configuration_word_71432) {
            g_configuration_word_713ac = i;
            return &((MiddleActorView *)g_middle_actor_pool)[i];
        }
        for (i = g_configuration_word_713a0; i < g_configuration_word_713ac; i++) {
            if (((MiddleActorView *)g_middle_actor_pool)[i].w2e == 0)
                break;
        }
        if (i < g_configuration_word_713ac) {
            g_configuration_word_713ac = i;
            return &((MiddleActorView *)g_middle_actor_pool)[i];
        }
        return 0;
    }
    for (i = g_configuration_word_713b8; i < g_configuration_word_713a0; i++) {
        if (((MiddleActorView *)g_middle_actor_pool)[i].w2e == 0)
            break;
    }
    if (i < g_configuration_word_713a0) {
        g_configuration_word_713b8 = i;
        return &((MiddleActorView *)g_middle_actor_pool)[i];
    }
    for (i = g_configuration_word_713aa; i < g_configuration_word_713b8; i++) {
        if (((MiddleActorView *)g_middle_actor_pool)[i].w2e == 0)
            break;
    }
    if (i < g_configuration_word_713b8) {
        g_configuration_word_713b8 = i;
        return &((MiddleActorView *)g_middle_actor_pool)[i];
    }
    return 0;
}

MiddleActorView *spawn_middle_277e0(unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4)
{
    MiddleActorView *p = (MiddleActorView *)allocate_middle_actor(a4);
    if (p != 0) {
        initialize_middle_actor(p);
        p->w2e = (unsigned short)(a4 | 0x4000);
        p->d00 = a1;
        p->d9c = a1;
        p->d04 = a2;
        p->da0 = a2;
        p->d08 = a3;
        p->da4 = a3;
        p->b22 = middle_descriptors[a4 & 0xfff].b6;
        p->d64 = middle_descriptors[a4 & 0xfff].d0;
    }
    return p;
}
}
