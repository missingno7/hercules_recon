/* ENG1 context-sequence destination 0x44120. The 0x70xxx/0x71xxx globals are
 * named by observed RVAs where no current owner exists. Calls and state reloads
 * follow the native contribution order. The neighbouring destination 0x44240
 * is not reconstructed yet. */
#include "../shared/engine_interface.h"
#include "setup_shared_state.h"

typedef struct Actor { long x, y, z; } Actor;
typedef struct Callback8 { void (*initialize)(Actor *); void (*update)(Actor *); } Callback8;
typedef struct Callback12 { void (*initialize)(Actor *); void (*update)(Actor *); void (*draw)(Actor *); } Callback12;
extern Actor *g_normal_actor_pool;
extern Callback12 *g_configuration_dword_71370;
extern Callback8 *g_configuration_dword_7139c;
extern Actor *spawn_normal(int x, int y, int z, int kind);
extern Actor *spawn_kind(long x, long y, long z, int kind);
extern void LinkedSoundFx_6da0(unsigned short id, void *owner);
extern void dispatch_engine_2c4d0(void);
extern unsigned int __cdecl random_limit(int range);
extern int eng1_2c030(int x, int y);
extern int __cdecl eng1_native_code_target_002c4c0(unsigned long value);
extern void __cdecl eng1_native_code_target_0019240(void);

extern unsigned long g_eng1_native_dword_70190;
extern unsigned long g_eng1_native_dword_70194;
extern void *g_eng1_native_dword_70198;
extern unsigned long g_eng1_native_dword_7019c;
extern void *g_eng1_native_dword_7018c;
extern Actor *g_eng1_native_actor_701a0;
extern int g_eng1_native_state_7023c;
extern int g_eng1_native_state_70240;
extern int g_eng1_native_state_70298;
extern int g_eng1_native_state_7028c;
extern unsigned long g_eng1_native_dword_70288;
extern unsigned char g_eng1_native_program_0069180[];
extern unsigned char g_eng1_native_program_0069220[];
extern unsigned char g_eng1_native_program_00692b8[];

void __cdecl eng1_native_code_target_0044120(void)
{
    Actor *actor;
    int count;
    signed char selector;

    g_normal_actor_pool->x = 0x01350000;
    g_normal_actor_pool->y = 0x02200000;
    g_normal_actor_pool->z = 0x00800000;
    actor = spawn_normal(0, 0, 0, 0x28);
    if (actor != 0)
        g_configuration_dword_71370[0x28].initialize(actor);
    g_eng1_native_dword_70194 = 0;
    count = 0x1e;
    g_eng1_native_dword_70288 = g_normal_actor_pool->x;
    g_eng1_native_dword_7019c = 0;
    for (; count != 0; --count) {
        actor = spawn_kind(0, 0, 0, 0x2b);
        if (actor != 0)
            g_configuration_dword_7139c[0x2b].initialize(actor);
    }
    LinkedSoundFx_6da0(0x2c0, 0);
    g_eng1_setup_dword_72188 = 5;
    g_eng1_native_state_70240 = 0;
    g_eng1_native_state_70298 = 0;
    g_eng1_native_state_7023c = 0;
    g_eng1_native_dword_70190 = 0;
    selector = (signed char)g_engine_interface.context_004->unknown_000[0x0e];
    switch (selector) {
    case 0:
        g_eng1_native_dword_70198 = g_eng1_native_program_0069180;
        break;
    case 1:
        g_eng1_native_dword_70198 = g_eng1_native_program_0069220;
        break;
    case 2:
        g_eng1_native_dword_70198 = g_eng1_native_program_00692b8;
        break;
    default:
        break;
    }
    g_eng1_native_dword_7018c = g_eng1_native_dword_70198;
}
