#include "host_callback_protocol_externs.h"

/* Minimum observed three-cell prefix; excludes the neighboring packet cell and
 * any full original record/TU or native placement claim. */
HostCallbackStoragePrefix g_host_callback_prefix = {0};
