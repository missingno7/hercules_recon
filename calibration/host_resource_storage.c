#include "resource_context.h"

/* Minimum typed prefix proved by the native clear; original full extent and owner remain unknown. */
ResourceCallbackContext g_resource_context_storage = {0};

/* File-backed initial DWORD; host_file_prepare clears it at runtime. */
unsigned long g_resource_prepare_scalar = 0xFFFFFFFFUL;
