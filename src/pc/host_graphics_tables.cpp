#include "host_graphics_table_storage.h"
extern "C" {
/* Shared working SDK-width view; original source/prototype remains unproved. */
extern void __cdecl graphics_native_06780(short,BYTE,BYTE,BYTE);
extern void __cdecl graphics_native_069a0(unsigned char);
void __cdecl graphics_native_071a0(void)
{
    int row,column,value;
    unsigned char *clear = (unsigned char *)&g_host_graphics_tables_52c490;
    for (row=0;row<256;++row)
        graphics_native_06780(row,row,row,row);
    for (row=0;row<256;++row)
        for (column=0;column<256;++column) {
            value=(row*column)>>7;
            g_host_graphics_shade_51c488[row][column]=(unsigned char)(value>255?255:value);
        }
    for (row=0;row<256;++row)
        for (column=0;column<256;++column) {
            value=row+column;
            g_host_graphics_tables_52c490.average[row][column]=(unsigned char)(value>>1);
            g_host_graphics_tables_52c490.add[row][column]=(unsigned char)(value>255?255:value);
            value=column-row;
            g_host_graphics_tables_52c490.subtract[row][column]=(unsigned char)(value<0?0:value);
            value=(row>>1)+column;
            g_host_graphics_tables_52c490.half_add[row][column]=(unsigned char)(value>255?255:value);
        }
    /* Native32-row identity ramp extends beyond logical31-row remap. */
    for (row=0;row<32;++row)
        for (column=0;column<256;++column)
            clear[row*256+column]=(unsigned char)column;
    graphics_native_069a0(1);
}
}
