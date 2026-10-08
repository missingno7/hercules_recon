#include "engine_interface.h"
#include <stddef.h>

typedef struct HostCallbackStoragePrefix {
    EngineNoArgCallback callback_zero_000;
    EngineNoArgCallback callback_one_004;
    unsigned long delivery_gate_008;
} HostCallbackStoragePrefix;

typedef char host_callback_prefix_is_12_bytes[
    (sizeof(HostCallbackStoragePrefix) == 12) ? 1 : -1];
typedef char host_callback_prefix_offsets_are_observed[
    (offsetof(HostCallbackStoragePrefix, callback_zero_000) == 0 &&
     offsetof(HostCallbackStoragePrefix, callback_one_004) == 4 &&
     offsetof(HostCallbackStoragePrefix, delivery_gate_008) == 8) ? 1 : -1];

extern HostCallbackStoragePrefix g_host_callback_prefix;

void __cdecl host_invoke_callback_zero(void);
EngineNoArgCallback __cdecl host_set_callback_zero(EngineNoArgCallback callback);
void __cdecl host_invoke_callback_one(void);
EngineNoArgCallback __cdecl host_set_callback_one(EngineNoArgCallback callback);
unsigned long __cdecl host_disable_callback_gate(void);
unsigned long __cdecl host_enable_clear_callbacks(void);
