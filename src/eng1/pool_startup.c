/* Ordinary-C behavioral recovery for the three no-argument pool
 * sweeps in ENG1 startup chain 0x255f0. The shared pool pointers, configuration
 * words, pending-action pointer table, and actor initializers stay with their
 * existing providers. No pool allocation or storage is introduced here. */
#include "resource_runtime_data.h"

extern void *g_normal_actor_pool;
extern void *g_middle_actor_pool;
extern void *g_alternate_actor_pool;
extern unsigned short g_configuration_word_713b4;
extern unsigned short g_configuration_word_71432;
extern unsigned short g_configuration_word_713ba;
extern unsigned short g_configuration_word_71380;

/* The native calls pass only the actor address. The full measured actor
 * layouts and source providers remain in the existing initializer unit. */
extern void __cdecl initialize_normal_actor(void *actor);
extern void __cdecl initialize_middle_actor(void *actor);
extern void __cdecl initialize_alternate_actor(void *actor);

/* ENG1 RVA 0x274e0. Initialize WORD 0x713b4 normal-pool entries at stride
 * 0x134 through the existing native-27520 behavioral provider. */
void __cdecl eng1_call_274e0(void)
{
    int index;

    index = 0;
    while (index < g_configuration_word_713b4) {
        initialize_normal_actor((unsigned char *)g_normal_actor_pool +
                                index * 0x134);
        ++index;
    }
}

/* ENG1 RVA 0x279f0. This startup sweep is distinct from shutdown reset
 * 0x279a0: initialize middle entries, then set each pointed group count at
 * 0x713e0 to -1 using the configured 0x71380 prefix. */
void __cdecl eng1_call_279f0(void)
{
    int index;

    index = 0;
    while (index < g_configuration_word_71432) {
        initialize_middle_actor((unsigned char *)g_middle_actor_pool +
                                index * 0xec);
        ++index;
    }

    index = 0;
    while (index < g_configuration_word_71380) {
        pending_action_group_table[index]->count = -1;
        ++index;
    }
}

/* ENG1 RVA 0x27fa0. Initialize WORD 0x713ba alternate-pool entries at stride
 * 0x94 through the existing native-27fe0 behavioral provider. */
void __cdecl eng1_call_27fa0(void)
{
    int index;

    index = 0;
    while (index < g_configuration_word_713ba) {
        initialize_alternate_actor((unsigned char *)g_alternate_actor_pool +
                                   index * 0x94);
        ++index;
    }
}
