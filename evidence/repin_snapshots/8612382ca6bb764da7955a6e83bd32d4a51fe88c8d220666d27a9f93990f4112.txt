#ifndef ENG1_INITIAL_MOTION_ROOTS_H
#define ENG1_INITIAL_MOTION_ROOTS_H
typedef struct Eng1InitialMotion2 { short x,y; } Eng1InitialMotion2;
typedef struct Eng1InitialMotion3 { short x,y,z; } Eng1InitialMotion3;
typedef char eng1_initial_motion_sizes[(sizeof(Eng1InitialMotion2)==4 && sizeof(Eng1InitialMotion3)==6)?1:-1];
extern Eng1InitialMotion2 g_eng1_initial_motion_0527d0[9];
extern Eng1InitialMotion3 g_eng1_initial_motion_05fa20[7];
extern Eng1InitialMotion3 g_eng1_initial_motion_060148[15];
extern Eng1InitialMotion3 g_eng1_initial_motion_066230[1];
extern Eng1InitialMotion2 g_eng1_initial_motion_05c3a0[6];
#endif
