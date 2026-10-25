/* TITLE resource slots: one 20-byte record per loadable title file. The table g_26110 is
   initialized data of the 0x17cb0 unit (paths and entry counts); +0x0c holds the loaded data
   (released through title_0c330) and +0x10/+0x12 the load state. */
#ifndef TITLE_SLOTS_H
#define TITLE_SLOTS_H

typedef struct TitleRecord {
    char *path_00;
    unsigned short count_04;
    unsigned short unknown_06;
    void *unknown_08;
    unsigned char *table_0c;
    unsigned short state_10;
    unsigned short state_12;
} TitleRecord;

extern TitleRecord g_26110[18];

#endif
