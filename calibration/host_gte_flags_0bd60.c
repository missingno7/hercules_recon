/* HERCULES.EXE 0xbd60/0xbd70/0xbe10 (lane h2 candidate): GTE translation-pad flag setters and
 * a 1024-entry square-root style table lookup, inside the recorded 0xbce0..0xbe40 host span.
 * Names provisional. */
typedef struct HostVector { long vx, vy, vz, pad; } HostVector;
typedef struct HostGte {
    short v[3][3];
    long mac[3][3];
    short sxy[2][3];
    long ofx, ofy, h, mac0;
    short rot[3][3];
    HostVector trans;
} HostGte;

extern HostGte g_host_gte;
extern short g_exe_439480[1024];

void __cdecl host_0bd60(void)
{
    g_host_gte.trans.pad = 1;
}

void __cdecl host_0bd70(void)
{
    g_host_gte.trans.pad = 0;
}

int __cdecl host_0be10(int value)
{
    return g_exe_439480[value & 0x3ff];
}
