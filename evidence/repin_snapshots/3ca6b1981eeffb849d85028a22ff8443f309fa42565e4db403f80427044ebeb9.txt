#ifndef ENG1_SECONDARY_RECORDS_H
#define ENG1_SECONDARY_RECORDS_H
#include <stddef.h>
typedef struct Eng1SpawnRecord { short x, y, z; unsigned short kind, sprite, state; } Eng1SpawnRecord;
typedef struct Eng1MotionPair { short x, y; } Eng1MotionPair;
typedef char spawn_record_is_12[(sizeof(Eng1SpawnRecord)==12)?1:-1];
typedef char spawn_record_offsets[((offsetof(Eng1SpawnRecord, kind)==6)&&(offsetof(Eng1SpawnRecord,sprite)==8)&&(offsetof(Eng1SpawnRecord,state)==10))?1:-1];
typedef char motion_pair_is_4[(sizeof(Eng1MotionPair)==4)?1:-1];
extern Eng1SpawnRecord g_eng1_spawn_records_0563f8[11];
extern Eng1SpawnRecord g_eng1_spawn_records_056480[11];
extern Eng1SpawnRecord g_eng1_spawn_records_056508[2];
extern Eng1MotionPair g_eng1_motion_pairs_051b28[14];
extern Eng1MotionPair g_eng1_motion_pairs_051f00[8];
#endif
