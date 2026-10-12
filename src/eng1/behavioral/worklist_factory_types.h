#ifndef ENG1_WORKLIST_FACTORY_TYPES_H
#define ENG1_WORKLIST_FACTORY_TYPES_H

#include "forest_actor_init.h"

typedef unsigned short ActorKind;
typedef struct ActorCore ActorCore;
typedef struct ActorMiddle ActorMiddle;
typedef struct ActorNormal ActorNormal;
typedef void (__cdecl *Eng1ActorActionCallback)(void *actor);

typedef char eng1_factory_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_factory_long_is_32[(sizeof(unsigned long) == 4) ? 1 : -1];

ActorNormal *__cdecl eng1_call_270c0(Eng1ForestRecord20 *record,
                                    unsigned long group_index);
ActorMiddle *__cdecl eng1_call_27720(Eng1ForestRecord20 *record,
                                    unsigned long group_index);
ActorCore *__cdecl eng1_call_27c40(Eng1ForestRecord20 *record,
                                  unsigned long group_index);

#endif
