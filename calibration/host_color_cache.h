#ifndef HERCULES_HOST_COLOR_CACHE_H
#define HERCULES_HOST_COLOR_CACHE_H
#include <windows.h>
#include <stddef.h>
/* Four lowBYTE state slots with observed4-byte spacing/read backing. */
typedef struct HostColorCache {
    BYTE red; BYTE opaque_001[3];
    BYTE green; BYTE opaque_005[3];
    BYTE blue; BYTE opaque_009[3];
    BYTE index; BYTE opaque_00d[3];
} HostColorCache;
typedef char color_cache_layout[(sizeof(HostColorCache)==16 && offsetof(HostColorCache,green)==4 && offsetof(HostColorCache,blue)==8 && offsetof(HostColorCache,index)==12)?1:-1];
#ifdef __cplusplus
extern "C" {
#endif
extern HostColorCache g_host_color_cache_56e418;
#ifdef __cplusplus
}
#endif
#endif
