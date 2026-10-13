#ifndef HERCULES_SURFACE_SERVICE_PROPOSAL_H
#define HERCULES_SURFACE_SERVICE_PROPOSAL_H

/* Tool-side view of independently published cells in the pinned HERCULES
 * host. This is not a native aggregate or an addressable struct in the EXE.
 * Raw 32-bit addresses remain integers, never host pointers.
 */
#include "hercules_host_abi.h"

/* Presentation tests and clears this byte before attempting Flip/GDI. */
#define HERCULES_HOST_PRESENT_GATE_VA ((renderer_u32)0x0043a590u)

#define HERCULES_HOST_LOCK_STATE_VA ((renderer_u32)0x00ba01f0u)
#define HERCULES_HOST_DDRAW_MODE_VA ((renderer_u32)0x00ba020cu)
#define HERCULES_HOST_ACTIVE_PIXELS_VA ((renderer_u32)0x00ba0210u)
#define HERCULES_HOST_ACTIVE_PITCH_VA ((renderer_u32)0x00ba0214u)

typedef struct HerculesHostSurfaceViewSnapshot {
    renderer_raw32 active_pixel_address;
    renderer_i32 pitch_bytes;
    renderer_u8 lock_held;
    renderer_u8 directdraw_mode;
} HerculesHostSurfaceViewSnapshot;

/* The host helpers take no arguments. Callers ignore EAX and consume the
 * published cells instead. `void` describes this adapter's discarded return
 * value; it does not assert that the historical routines return no EAX value.
 */
#ifdef HERCULES_RENDERER_LIVE_X86
typedef void (HERCULES_CDECL *HerculesHostSurfaceLockService)(void);
typedef void (HERCULES_CDECL *HerculesHostSurfaceUnlockService)(void);
typedef void (HERCULES_CDECL *HerculesHostSurfacePresentService)(void);
#endif

#endif
