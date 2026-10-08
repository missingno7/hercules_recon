#ifndef HERCULES_RESOURCE_CONTEXT_H
#define HERCULES_RESOURCE_CONTEXT_H
#include <stddef.h>

/* Observed prefix extension only. Field meanings and original full extent remain unproved. */
typedef struct ResourceCallbackContext {
    unsigned char unknown_000[0x38];
    unsigned long tick_038;
    unsigned char unknown_03c[0x9c - 0x3c];
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
    unsigned char unknown_0e1[0x1b00 - 0xe1];
    unsigned long prepare_state_1b00;
} ResourceCallbackContext;

typedef char resource_context_observed_offsets[
    (offsetof(ResourceCallbackContext,tick_038)==0x38 &&
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
     offsetof(ResourceCallbackContext,prepare_state_1b00)==0x1b00 &&
     sizeof(ResourceCallbackContext)==0x1b04) ? 1:-1];
#endif
