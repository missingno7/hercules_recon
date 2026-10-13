#ifndef RENDERER_PACKET_WIDTH_H
#define RENDERER_PACKET_WIDTH_H

/* Fixed-width wire fields for read-only PC packet/frame views.
 * Raw addresses stay renderer_u32 values and are never host pointers. */
#if defined(_MSC_VER) && _MSC_VER < 1600
typedef unsigned char renderer_u8;
typedef signed char renderer_i8;
typedef unsigned short renderer_u16;
typedef signed short renderer_i16;
typedef unsigned int renderer_u32;
typedef signed int renderer_i32;
#else
#include <stdint.h>
typedef uint8_t renderer_u8;
typedef int8_t renderer_i8;
typedef uint16_t renderer_u16;
typedef int16_t renderer_i16;
typedef uint32_t renderer_u32;
typedef int32_t renderer_i32;
#endif

typedef renderer_u32 renderer_raw32;

typedef char renderer_assert_u8[(sizeof(renderer_u8) == 1) ? 1 : -1];
typedef char renderer_assert_i8[(sizeof(renderer_i8) == 1) ? 1 : -1];
typedef char renderer_assert_u16[(sizeof(renderer_u16) == 2) ? 1 : -1];
typedef char renderer_assert_i16[(sizeof(renderer_i16) == 2) ? 1 : -1];
typedef char renderer_assert_u32[(sizeof(renderer_u32) == 4) ? 1 : -1];
typedef char renderer_assert_i32[(sizeof(renderer_i32) == 4) ? 1 : -1];
typedef char renderer_assert_raw32[(sizeof(renderer_raw32) == 4) ? 1 : -1];

/* Opt in only in a live callback adapter that stores native function/data
 * pointers. The raw packet decoder does not require a 32-bit host process. */
#define RENDERER_ASSERT_LIVE_X86_POINTER_WIDTH(name) \
    typedef char name[(sizeof(void *) == 4) ? 1 : -1]

#endif
