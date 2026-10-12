#ifndef ENG1_ACTION_SPAWN_27D40_H
#define ENG1_ACTION_SPAWN_27D40_H

typedef struct Actor Actor;

/* ENG1 RVA 0x27d40: one actor pointer and one DWORD action/kind slot. */
Actor *__cdecl action_spawn_27d40(Actor *actor, unsigned long action);

#endif
