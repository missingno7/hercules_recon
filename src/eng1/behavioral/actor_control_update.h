#ifndef ENG1_ACTOR_CONTROL_UPDATE_H
#define ENG1_ACTOR_CONTROL_UPDATE_H
#include "interpreters.h"
#include "actor_control_tables.h"
void __cdecl eng1_native_code_target_02cd30(Actor *actor);
unsigned char __cdecl eng1_native_code_target_0041fd0(unsigned long value);
int __cdecl eng1_native_code_target_002c4c0(unsigned long value);
/* Internal bounded status API. Unsupported scans leave output unchanged;
 * the native-ABI wrapper reports and terminates before returning any value. */
int __cdecl eng1_classify_actor_position_checked(unsigned long value,
                                                unsigned char *result);
#endif
