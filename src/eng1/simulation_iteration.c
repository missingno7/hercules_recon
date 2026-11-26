/* Behavioral reconstruction for ENG1.DLL RVA 0x156a0.
 * The function stays RVA-labelled because its gameplay role and source unit
 * remain unproved. Its two state-update callees have separate identities.
 */
#include "../shared/engine_interface.h"

typedef unsigned char eng1_sim_u8;
typedef unsigned short eng1_sim_u16;
typedef unsigned long eng1_sim_u32;

extern eng1_sim_u32 eng1_sim_state_72110;
extern eng1_sim_u32 eng1_sim_state_72170;
extern eng1_sim_u16 eng1_sim_state_72108;
extern eng1_sim_u16 eng1_sim_state_6feb4;
extern void *g_normal_actor_pool;

/* Separate unresolved behavioral dependencies; do not replace with no-ops. */
extern void __cdecl eng1_native_code_target_0015780(void); /* RVA 0x15780 */
extern void __cdecl dispatch_local_12620(void); /* RVA 0x12620 */

typedef struct PlayerActor PlayerActor;
typedef struct PlayerCommand { short opcode, operand; } PlayerCommand;
struct PlayerActor {
    long x, y, z;
    unsigned char unknown_0c[0x21-0x0c];
    signed char depth;
    unsigned char category, mode;
    unsigned char unknown_24[0x30-0x24];
    short ticks, phase;
    unsigned short sprite, count, profile, scale;
    unsigned char unknown_3c[0x4a-0x3c];
    unsigned short alpha;
    long parameter;
    unsigned long flags, render_flags;
    long record_offset;
    void *entries;
    PlayerActor *child;
    PlayerCommand *program;
    unsigned char unknown_68[0x94-0x68];
    unsigned char facing, facing_y, facing_z, unknown_97;
    short unknown_98, unknown_9a;
    long prev_x, prev_y, prev_z;
    long motion_x, velocity_x, rate_x, unknown_b4;
    long motion_y, velocity_y, rate_y, unknown_c4;
    unsigned char unknown_c8[0xec-0xc8];
    unsigned char lifetime, lifetime_flag;
};
typedef struct PlayerContext {
    signed char level, sublevel;
    unsigned char unknown_02, action;
    unsigned char unknown_04[8];
    unsigned char control;
    unsigned char unknown_0d[3];
    unsigned char option;
    unsigned char unknown_11[0x24-0x11];
    short energy[6];
    unsigned long flags;
    unsigned char unknown_34[0x5c-0x34];
    unsigned short held, pressed, unknown_60, unknown_62, unknown_64;
    unsigned char unknown_66[6];
    unsigned short key_6c, key_6e, key_70, key_72;
    unsigned char unknown_74[0x1b30-0x74];
    long transition;
} PlayerContext;
typedef struct PlayerInterface { long signature; PlayerContext *context; } PlayerInterface;
void __cdecl dispatch_local_156a0(void)
{
    if ((((PlayerContext *)g_engine_interface.context_004)->unknown_64 & 0x400) &&
        (((PlayerContext *)g_engine_interface.context_004)->flags & 0x4000))
        eng1_sim_state_72110 = 1;
    if ((((PlayerContext *)g_engine_interface.context_004)->unknown_64 & 0x100) &&
        (((PlayerContext *)g_engine_interface.context_004)->flags & 0x4000)) {
        eng1_sim_state_72110 = 1;
        ((PlayerContext *)g_engine_interface.context_004)->control = 1;
        if (!((PlayerContext *)g_engine_interface.context_004)->option)
            ((PlayerContext *)g_engine_interface.context_004)->option = 1;
    }
    if ((((PlayerContext *)g_engine_interface.context_004)->unknown_64 & 0x200) &&
        (((PlayerContext *)g_engine_interface.context_004)->flags & 0x4000)) {
        ((PlayerContext *)g_engine_interface.context_004)->control = 1;
        eng1_sim_state_72110 = 1;
        ((PlayerContext *)g_engine_interface.context_004)->option = 0;
    }
    if ((((PlayerContext *)g_engine_interface.context_004)->unknown_64 & 0x800) &&
        eng1_sim_state_72108 != 0x19 && (((PlayerContext *)g_engine_interface.context_004)->flags & 0x2000))
        eng1_sim_state_72170 = 1;
    eng1_native_code_target_0015780();
    eng1_sim_state_6feb4 = 0;
    if ((((PlayerContext *)g_engine_interface.context_004)->flags & 0x8000) &&
        (((PlayerContext *)g_engine_interface.context_004)->held & 0xa000) == 0xa000)
        ((PlayerActor *)g_normal_actor_pool)->z = (((PlayerActor *)g_normal_actor_pool)->z + 0x400000) & 0xff800000;
    dispatch_local_12620();
}
