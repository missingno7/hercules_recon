/* TITLE file-name lists, defined in title_files.c (a C data unit linked after the sound-effect
   unit and before the unit at 0x5e90). Entry indexes name the files that code loads directly. */
#ifndef TITLE_FILES_H
#define TITLE_FILES_H

extern char *g_engine_paths[12];  /* engine module per level */
extern char *g_screen_files[];    /* NULL-terminated */
extern char *g_sequence_files[];  /* NULL-terminated */
extern char *g_language_files[32];

enum TitleSequenceFile {
    TITLE_SEQ_PSWD = 0,
    TITLE_SEQ_PSWDB = 1,
    TITLE_SEQ_CHEAT = 2,
    TITLE_SEQ_CONTINUE = 3,
    TITLE_SEQ_SEQ1 = 4,
    TITLE_SEQ_SEQ2 = 5,
    TITLE_SEQ_SEQ3 = 6,
    TITLE_SEQ_SEQ4 = 7,
    TITLE_SEQ_SEQ5 = 8,
    TITLE_SEQ_SEQ6 = 9,
    TITLE_SEQ_SEQ7 = 10,
    TITLE_SEQ_SEQ8 = 11,
    TITLE_SEQ_SEQ9 = 12,
    TITLE_SEQ_SEQ10 = 13,
    TITLE_SEQ_SEQ11 = 14,
    TITLE_SEQ_PSWD_LEV1 = 15,
    TITLE_SEQ_PSWD_LEV2 = 16,
    TITLE_SEQ_PSWD_LEV3 = 17,
    TITLE_SEQ_PSWD_LEV4 = 18,
    TITLE_SEQ_PSWD_LEV5 = 19,
    TITLE_SEQ_PSWD_LEV6 = 20,
    TITLE_SEQ_PSWD_LEV7 = 21,
    TITLE_SEQ_PSWD_LEV8 = 22,
    TITLE_SEQ_PSWD_LEV9 = 23,
    TITLE_SEQ_PSWD_LEV10 = 24,
    TITLE_SEQ_PSWD_LEV11 = 25,
    TITLE_SEQ_PSWD_LEV12 = 26,
    TITLE_SEQ_T001 = 27,
    TITLE_SEQ_T002 = 28,
    TITLE_SEQ_T003 = 29,
    TITLE_SEQ_T004 = 30,
    TITLE_SEQ_T005 = 31,
    TITLE_SEQ_T006 = 32,
    TITLE_SEQ_T007 = 33,
    TITLE_SEQ_T008 = 34,
    TITLE_SEQ_T009 = 35,
    TITLE_SEQ_T010 = 36,
    TITLE_SEQ_T011 = 37,
    TITLE_SEQ_T012 = 38,
    TITLE_SEQ_T013 = 39,
    TITLE_SEQ_T014 = 40,
    TITLE_SEQ_T015 = 41,
    TITLE_SEQ_T016 = 42,
    TITLE_SEQ_T017 = 43,
    TITLE_SEQ_T018 = 44,
    TITLE_SEQ_COUNT = 45
};

#endif
