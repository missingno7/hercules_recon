#ifndef HERCULES_RESOURCE_CONTEXT_H
#define HERCULES_RESOURCE_CONTEXT_H
#include <stddef.h>

/* Observed runtime prefix only. Names and complete original extent unproved. */
typedef struct ResourceCallbackContext {
    unsigned char unknown_000[0x38];
    unsigned long tick_038;
    unsigned char unknown_03c[0x9c - 0x3c];
    unsigned short mode_09c;
    short remaining_sectors_09e;
    short total_sectors_0a0;
    unsigned char unknown_0a2[0xb0 - 0xa2];
    void (__cdecl *load_complete)(void);
    void (__cdecl *load_failed)(void);
    unsigned char unknown_0b8[0xdf - 0xb8];
    unsigned char blocked_0df;
} ResourceCallbackContext;
typedef char resource_context_observed_offsets[
    (offsetof(ResourceCallbackContext,tick_038)==0x38 &&
     offsetof(ResourceCallbackContext,mode_09c)==0x9c &&
     offsetof(ResourceCallbackContext,remaining_sectors_09e)==0x9e &&
     offsetof(ResourceCallbackContext,total_sectors_0a0)==0xa0 &&
     offsetof(ResourceCallbackContext,load_complete)==0xb0 &&
     offsetof(ResourceCallbackContext,load_failed)==0xb4 &&
     offsetof(ResourceCallbackContext,blocked_0df)==0xdf) ? 1:-1];
#endif
