#ifndef ENG1_RUNTIME_READ_GUARD_H
#define ENG1_RUNTIME_READ_GUARD_H

#include "interpreters.h"

/* Checks exact registered source-object identity. Unknown runtime pointers
 * deliberately retain the existing interpreter policy. */
void eng1_guard_program_read(const void *program, long index,
                            unsigned int stride, unsigned int width,
                            const char *site);
void eng1_guard_frame_slot(const void *frames, long index, const char *site);

/* Behavioral wrappers preflight registered frame tables, then call the
 * unchanged exact shared helpers. They never alias advance_frame/select_frame. */
void __cdecl eng1_guarded_advance_frame(Actor *actor);
void __cdecl eng1_guarded_select_frame(Actor *actor, int index);

#endif
