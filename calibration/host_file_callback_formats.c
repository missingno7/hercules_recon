#include "host_file_callbacks.h"
/* Complete observed PC format literals; original TU/layout unproved. */
char g_resource_filename_format[] = "%s%s";
char g_resource_loading_format[] = "Loading %s @ 0x%x,size %d ";
char g_resource_async_open_failure_format[] = "Unable to open %s (%d)\n";
char g_resource_sync_open_failure_format[] = "Unable to open %s \n";
char g_resource_sync_loaded_format[] = "- Loaded \n";
char g_resource_close_failure_format[] = "- Failed \n";
