/* HERCULES.EXE CD shim 0xa150/0xa190 (lane h2 candidate): PSX CdControl-style
 * location latch and constant status.  Names provisional. */
typedef struct HostCdLoc {
    unsigned char minute, second, sector, track;
} HostCdLoc;

extern HostCdLoc g_exe_96e634;

int __cdecl host_0a150(unsigned char command, HostCdLoc *param, unsigned char *result)
{
    switch (command) {
    case 2:
        g_exe_96e634 = *param;
        break;
    case 3:
        if (param != 0)
            g_exe_96e634 = *param;
        break;
    }
    return 1;
}

int __cdecl host_0a190(void)
{
    return 2;
}
