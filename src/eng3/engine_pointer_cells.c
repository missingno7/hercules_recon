/*
 * ENG3 loader-zero pointer cells in the zero tail of .data. Each is one 32-bit
 * pointer that the handlers only ever read or write whole (direct [disp32]
 * dword accesses; 0x227ba also pushes the address of g_eng3_ranges_4078c).
 * Until now every unit only declared them extern. This file is their single
 * data-only definition; the pointee types stay incomplete here. Names keep the
 * provisional address labels used by the handler units.
 */
typedef struct Eng3Game Eng3Game;
typedef struct Eng3Host Eng3Host;
typedef struct KindAction KindAction;
typedef struct Eng3Range Eng3Range;

Eng3Range *g_eng3_ranges_4078c;
Eng3Game *g_eng3_game_40b8c;
KindAction *g_eng3_middle_actions_40b98;
Eng3Host *g_eng3_host_40bb4;
