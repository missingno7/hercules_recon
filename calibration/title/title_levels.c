/* TITLE level table: a data-only C object (TITLE.DLL .data 0x234b0..0x2355f, literals to 0x23658).
   No code references it. It follows the 0x6350 unit's literals and precedes the object-system
   tables, and its strings are the first (folded) copies of those the 0x16300 unit's engine table
   points to. Literal order is the reverse of first use in the initializer, as measured. */

typedef struct TitleLevel {
    char *name_00;
    char *path_04;
    char *engine_08;
} TitleLevel;

TitleLevel g_234b0[12] = {
    {"PLAYROOM", "\\SRC1\\ENGINE\\", "ENGINE1"},
    {"TRAINING GROUND", "\\SRC1\\ENGINE\\", "ENGINE1"},
    {"TRAINING GAUNTLET", "\\SRC3\\ENGINE\\", "ENGINE3"},
    {"FOREST OF CENTAURS", "\\SRC1\\ENGINE\\", "ENGINE1"},
    {"NESSUS BATTLE", "\\SRC1\\ENGINE\\", "ENGINE1"},
    {"VISIT TO THEBES", "\\SRC1\\ENGINE\\", "ENGINE1"},
    {"HYDRA BATTLE", "\\SRC1\\ENGINE\\", "ENGINE1"},
    {"MEDUSA BATTLE", "\\SRC1\\ENGINE\\", "ENGINE1"},
    {"CYCLOPS CHASE", "\\SRC3\\ENGINE\\", "ENGINE3"},
    {"BATTLE OF TITANS", "\\SRC1\\ENGINE\\", "ENGINE1"},
    {"PASSAGEWAYS TORMENT", "\\SRC3\\ENGINE\\", "ENGINE3"},
    {"VORTEX OF SOULS", "\\SRC1\\ENGINE\\", "ENGINE1"},
};
int g_23540 = 64;
int g_23544 = 1;
short g_23548[12] = {5, 1, 11, 6, 3, 7, 9, 2, 8, 10, 0, 4};
