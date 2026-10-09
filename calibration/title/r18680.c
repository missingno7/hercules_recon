#include "title_engine.h"

typedef struct TitleRecord {
    unsigned long unknown_00;
    unsigned short count_04;
    unsigned char unknown_06[6];
    unsigned char *table_0c;
    unsigned short type_10;
    unsigned short state_12;
} TitleRecord;

extern TitleRecord g_26110[18];

int title_04d80(int index, int key);

unsigned char *title_18680(int index, int key)
{
    TitleRecord *rec;
    int *entries;
    int value;

    if (key == 0) {
        return 0;
    }
    if (g_26110[index].type_10 != 3) {
        if (title_04d80(index, key) == 0) {
            return 0;
        }
    }
    rec = &g_26110[index];
    entries = (int *)(rec->table_0c + rec->count_04 * 4 + 8);
    value = entries[(short)key];
    if (value == 0) {
        return 0;
    }
    return (unsigned char *)entries + (value >> 8);
}
