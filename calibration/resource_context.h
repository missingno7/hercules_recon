#ifndef HERCULES_RESOURCE_CONTEXT_H
#define HERCULES_RESOURCE_CONTEXT_H
#include <stddef.h>

/* Observed prefix extension only. Field meanings and original full extent remain unproved. */
typedef struct ResourceCallbackContext {
    unsigned char unknown_000[0x34];
    unsigned long unknown_034;
    unsigned long tick_038;
    unsigned long unknown_03c;
    unsigned long unknown_040;
    unsigned char unknown_044[0x48 - 0x44];
    unsigned long unknown_048;
    unsigned long unknown_04c;
    unsigned char unknown_050[0x54 - 0x50];
    unsigned long unknown_054;
    unsigned char unknown_058[0x8c - 0x58];
    unsigned long unknown_08c;
    unsigned char unknown_090[0x94 - 0x90];
    unsigned long unknown_094;
    unsigned char unknown_098[0x9c - 0x98];
    unsigned short mode_09c;
    short remaining_sectors_09e;
    short total_sectors_0a0;
    unsigned char unknown_0a2[0xa4 - 0xa2];
    unsigned char *destination_cursor_a4;
    unsigned char unknown_0a8[0xac - 0xa8];
    unsigned long unknown_0ac;
    void (__cdecl *load_complete)();
    void (__cdecl *load_failed)();
    int virtual_handle_0b8;
    unsigned long countdown_0bc;
    unsigned char unknown_0c0[0xd8 - 0xc0];
    long transfer_result_0d8;
    unsigned short transfer_status_0dc;
    unsigned char unknown_0de;
    unsigned char blocked_0df;
    unsigned char suppress_auto_close_0e0;
    unsigned char unknown_0e1[0xe4 - 0xe1];
    void *unknown_0e4;
    unsigned char unknown_0e8[0x128 - 0xe8];
    unsigned char unknown_128;
    unsigned char unknown_129[0x12c - 0x129];
    unsigned short unknown_12c;
    unsigned char unknown_12e[0x1b00 - 0x12e];
    unsigned long prepare_state_1b00;
    unsigned char unknown_1b04[0x1b30 - 0x1b04];
    unsigned long unknown_1b30;
} ResourceCallbackContext;

typedef char resource_context_observed_offsets[
    (offsetof(ResourceCallbackContext,unknown_034)==0x34 &&
     offsetof(ResourceCallbackContext,tick_038)==0x38 &&
     offsetof(ResourceCallbackContext,mode_09c)==0x9c &&
     offsetof(ResourceCallbackContext,remaining_sectors_09e)==0x9e &&
     offsetof(ResourceCallbackContext,total_sectors_0a0)==0xa0 &&
     offsetof(ResourceCallbackContext,destination_cursor_a4)==0xa4 &&
     offsetof(ResourceCallbackContext,unknown_0ac)==0xac &&
     offsetof(ResourceCallbackContext,virtual_handle_0b8)==0xb8 &&
     offsetof(ResourceCallbackContext,countdown_0bc)==0xbc &&
     offsetof(ResourceCallbackContext,load_complete)==0xb0 &&
     offsetof(ResourceCallbackContext,load_failed)==0xb4 &&
     offsetof(ResourceCallbackContext,transfer_result_0d8)==0xd8 &&
     offsetof(ResourceCallbackContext,transfer_status_0dc)==0xdc &&
     offsetof(ResourceCallbackContext,unknown_0de)==0xde &&
     offsetof(ResourceCallbackContext,blocked_0df)==0xdf &&
     offsetof(ResourceCallbackContext,suppress_auto_close_0e0)==0xe0 &&
     offsetof(ResourceCallbackContext,unknown_0e4)==0xe4 &&
     offsetof(ResourceCallbackContext,unknown_128)==0x128 &&
     offsetof(ResourceCallbackContext,unknown_12c)==0x12c &&
     offsetof(ResourceCallbackContext,prepare_state_1b00)==0x1b00 &&
     offsetof(ResourceCallbackContext,unknown_03c)==0x3c &&
     offsetof(ResourceCallbackContext,unknown_040)==0x40 &&
     offsetof(ResourceCallbackContext,unknown_048)==0x48 &&
     offsetof(ResourceCallbackContext,unknown_04c)==0x4c &&
     offsetof(ResourceCallbackContext,unknown_054)==0x54 &&
     offsetof(ResourceCallbackContext,unknown_08c)==0x8c &&
     offsetof(ResourceCallbackContext,unknown_094)==0x94 &&
     offsetof(ResourceCallbackContext,unknown_1b30)==0x1b30 &&
     sizeof(ResourceCallbackContext)==0x1b34) ? 1:-1];
#endif
