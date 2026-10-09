#include "title_engine.h"

typedef struct TitleRecord {
    unsigned short count_00;
    unsigned char unknown_02[6];
    unsigned char *table_08;
    unsigned short type_0c;
    unsigned char unknown_0e[6];
} TitleRecord;

extern TitleRecord g_26114[];

int title_04d80(int index, int key);

unsigned char *title_18680(int index, int key)
{
    TitleRecord *rec;
    int *entries;
    int value;

    if (key == 0) {
        return 0;
    }
    if (g_26114[index].type_0c != 3) {
        if (title_04d80(index, key) == 0) {
            return 0;
        }
    }
    rec = &g_26114[index];
    entries = (int *)(rec->table_08 + rec->count_00 * 4 + 8);
    value = entries[(short)key];
    if (value == 0) {
        return 0;
    }
    return (unsigned char *)entries + (value >> 8);
}
