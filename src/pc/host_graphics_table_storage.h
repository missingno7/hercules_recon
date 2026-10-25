#ifndef HERCULES_GRAPHICS_TABLE_STORAGE_H
#define HERCULES_GRAPHICS_TABLE_STORAGE_H
#include <windows.h>
#include <stddef.h>
/* Working shared native-address views; original aggregate/TU unproved. */
typedef struct HostGraphicsTableBlock {
    unsigned char remap[31][256];
    WINDOWPLACEMENT placement;
    unsigned char unknown_01f2c[4];
    RECT rectangle;
    unsigned char average[256][256];
    unsigned char add[256][256];
    unsigned char subtract[256][256];
    unsigned char half_add[256][256];
} HostGraphicsTableBlock;
typedef char table_layout[(offsetof(HostGraphicsTableBlock,placement)==0x1f00 &&
    offsetof(HostGraphicsTableBlock,rectangle)==0x1f30 &&
    offsetof(HostGraphicsTableBlock,average)==0x1f40 &&
    offsetof(HostGraphicsTableBlock,add)==0x11f40 &&
    offsetof(HostGraphicsTableBlock,subtract)==0x21f40 &&
    offsetof(HostGraphicsTableBlock,half_add)==0x31f40 &&
    sizeof(HostGraphicsTableBlock)==0x41f40)?1:-1];
#ifdef __cplusplus
extern "C" {
#endif
extern HostGraphicsTableBlock g_host_graphics_tables_52c490;
extern unsigned char g_host_graphics_shade_51c488[256][256];
#ifdef __cplusplus
}
#endif
#define g_exe_92e390 (g_host_graphics_tables_52c490.placement)
#define g_exe_92e3c0 (g_host_graphics_tables_52c490.rectangle)
#endif
