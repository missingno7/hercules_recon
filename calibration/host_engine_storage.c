#include "host_engine_cells.h"

/* Independent measured/API-contract prefixes; no native adjacency implied. */
FARPROC g_host_engine_entry_91c45c = 0;
DWORD g_host_engine_gate_91c460 = 0;
DWORD g_host_engine_selector_91c464 = 0;
HMODULE g_host_engine_module_91c468 = 0;
jmp_buf g_host_engine_jump_buffer_91bfd8 = {0};

typedef char host_engine_sdk_jump_buffer_size[(sizeof(jmp_buf) == 64) ? 1 : -1];
