#ifndef ENG1_PROGRAM_INTERPRETERS_H
#define ENG1_PROGRAM_INTERPRETERS_H

#include <stddef.h>
#include "frame_programs.h"
#include "resource_runtime_data.h"

typedef struct Actor Actor;
struct Actor {
    long x, y, z;
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category;
    unsigned char unknown_023[0x2a - 0x23];
    unsigned short unknown_02a;
    unsigned char unknown_02c[0x2e - 0x2c];
    unsigned short kind;
    short frame_ticks;
    short frame_state;
    short active_command;
    short unknown_036;
    short frame_group;
    unsigned char unknown_03a[0x4c - 0x3a];
    long interpolation_position;
    unsigned long flags;
    unsigned long render_flags;
    void *unknown_058;
    Actor *attachment;
    Actor *child;
    void *image_or_program;
    void *unknown_068;
    void *buffer;
    short angle_yaw;
    short angle_pitch;
    short angle_roll;
    unsigned char unknown_076[0x94 - 0x76];
    unsigned char direction_x;
    unsigned char direction_y;
    unsigned char direction_z;
    unsigned char tag;
    short vertical_step;
    unsigned char unknown_09a[0xa8 - 0x9a];
    long unknown_a8;
    unsigned char unknown_0ac[0xb8 - 0xac];
    long unknown_b8;
    unsigned char unknown_0bc[0xc8 - 0xbc];
    long unknown_c8;
    unsigned char unknown_0cc[0xe4 - 0xcc];
    Actor *prev_scene;
    Actor *next_scene;
    void *unknown_0ec;
    void *steps;
    long step_index;
    long unknown_f8;
    unsigned char unknown_0fc[0x110 - 0x0fc];
    void **frames;
    short frame_index;
    short unknown_116;
    void *initial_steps;
    short unknown_11c;
    short active_step;
    unsigned char unknown_120[4];
    short action_callback_index;
    short unknown_126;
    unsigned short **animation_control;
    Actor *prev_chain;
    Actor *next_chain;
};

typedef struct Eng1AnimationControl Eng1AnimationControl;
struct Eng1AnimationControl {
    unsigned short attributes;
    short x_current;
    short x_target;
    short x_step;
    short yaw_target;
    short yaw_step;
    short pitch_target;
    short pitch_step;
    short roll_target;
    short roll_step;
};

typedef void (__cdecl *Eng1ActorCallback)(Actor *);

typedef char eng1_program_actor_is_32bit[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_program_actor_size[(sizeof(Actor) == 0x134) ? 1 : -1];
typedef char eng1_program_field_02a[(offsetof(Actor, unknown_02a) == 0x2a) ? 1 : -1];
typedef char eng1_program_field_064[(offsetof(Actor, image_or_program) == 0x64) ? 1 : -1];
typedef char eng1_program_field_070[(offsetof(Actor, angle_yaw) == 0x70) ? 1 : -1];
typedef char eng1_program_field_094[(offsetof(Actor, direction_x) == 0x94) ? 1 : -1];
typedef char eng1_program_field_098[(offsetof(Actor, vertical_step) == 0x98) ? 1 : -1];
typedef char eng1_program_field_0a8[(offsetof(Actor, unknown_a8) == 0xa8) ? 1 : -1];
typedef char eng1_program_field_0b8[(offsetof(Actor, unknown_b8) == 0xb8) ? 1 : -1];
typedef char eng1_program_field_0c8[(offsetof(Actor, unknown_c8) == 0xc8) ? 1 : -1];
typedef char eng1_program_field_0f4[(offsetof(Actor, step_index) == 0xf4) ? 1 : -1];
typedef char eng1_program_field_110[(offsetof(Actor, frames) == 0x110) ? 1 : -1];
typedef char eng1_program_field_114[(offsetof(Actor, frame_index) == 0x114) ? 1 : -1];
typedef char eng1_program_field_11e[(offsetof(Actor, active_step) == 0x11e) ? 1 : -1];
typedef char eng1_program_field_124[(offsetof(Actor, action_callback_index) == 0x124) ? 1 : -1];
typedef char eng1_program_field_128[(offsetof(Actor, animation_control) == 0x128) ? 1 : -1];
typedef char eng1_animation_control_size[(sizeof(Eng1AnimationControl) == 20) ? 1 : -1];

/* Existing source providers: calibration/macro_actor.c or src/shared/frame_state.c,
 * calibration/remove_actor.c, src/eng1/actor_handlers_mid.c, and ENG1 audio. */
extern void __cdecl advance_frame(Actor *);
extern void __cdecl select_frame(Actor *, int);
extern int __cdecl remove_actor(Actor *);
extern int __cdecl wrapped_ratio(int, int, int);
extern void __cdecl play_sound(unsigned short, Actor *);
extern unsigned int __cdecl dispatch_engine_2c460(int);
extern unsigned long g_configuration_dword_71384;

/* Native entries 0x27410 and 0x27450 remain separate semantic dependencies:
 * the first marks the resource owner then calls remove_actor; the second only
 * marks the resource owner. They are not supplied by the current code graph. */
extern void __cdecl remove_actor_preserving_resource(Actor *);
extern void __cdecl mark_actor_resource_for_release(Actor *);

/* Native entries 0x300c0 and 0x30180 allocate and initialize actors. Their
 * pointer/call shapes are measured; the original semantic names are unknown. */
extern void __cdecl spawn_actor_with_owner(Actor *, long, long, long, unsigned short);
extern void __cdecl spawn_actor_at_group(long, long, long, unsigned short, unsigned short);

/* Native entry 0x7ef6 stops all active sound rows owned by the actor. */
extern void __cdecl stop_actor_sounds(Actor *);

unsigned long __cdecl eng1_interpret_signed_pairs(Actor *);
unsigned long __cdecl eng1_interpret_word_stream(Actor *);
unsigned long __cdecl eng1_interpret_animation_triples(Actor *);

#endif
