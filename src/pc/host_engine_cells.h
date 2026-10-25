#include <setjmp.h>
#include <windows.h>

/* One address-backed view shared by the two candidate host functions. */
extern char g_host_engine_path_91bbd8[];       /* VA 0x91bbd8; capacity unknown */
extern char g_host_engine_error_91c040[];       /* VA 0x91c040; capacity unknown */
extern FARPROC g_host_engine_entry_91c45c;      /* VA 0x91c45c; 32-bit function pointer */
extern DWORD g_host_engine_gate_91c460;         /* VA 0x91c460; observed DWORD */
extern DWORD g_host_engine_selector_91c464;     /* VA 0x91c464; observed DWORD */
extern HMODULE g_host_engine_module_91c468;     /* VA 0x91c468; 32-bit module handle */
extern jmp_buf g_host_engine_jump_buffer_91bfd8; /* VA 0x91bfd8; runtime extent unproved */
