typedef unsigned int UINT;
typedef unsigned long MMRESULT;

__declspec(dllimport) MMRESULT __stdcall timeEndPeriod(UINT period);

/* Offset labels identify observed direct callees; their original names and
   translation-unit ownership remain unproved. */
extern void __cdecl cleanup_native_4da0(void);
extern void __cdecl cleanup_native_ff70(void);
extern void __cdecl cleanup_native_a120(void);
extern void __cdecl empty_host_callback(void);
extern void __cdecl cleanup_native_74c0(void);
extern void __cdecl archive_shutdown(void);

void __cdecl fatal_shutdown_chain(void)
{
    timeEndPeriod(1);
    cleanup_native_4da0();
    cleanup_native_ff70();
    cleanup_native_a120();
    empty_host_callback();
    cleanup_native_74c0();
    archive_shutdown();
}
