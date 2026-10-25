#ifndef HERCULES_HOST_FILE_CALLBACKS_H
#define HERCULES_HOST_FILE_CALLBACKS_H
/* Observed PC callback ABI views; original prototypes/TU remain unproved. */
#ifdef __cplusplus
extern "C" {
#endif
unsigned long __cdecl host_file_sync(const char *, const char *, void *, unsigned long);
int __cdecl host_file_async(const char *, const char *, int, void *, unsigned long);
int __cdecl host_cancel_load(void);
void __cdecl host_file_progress(void);
unsigned long __cdecl resource_file_length(const char *, const char *);
int __cdecl host_file_open_only(const char *, const char *);
int __cdecl host_frame_close_state(void);
void __cdecl host_file_prepare(void);

/* Distinct from the shared zero-text view at RVA3f74c. Capacity is unknown. */
extern char g_resource_filename[];
extern char g_resource_filename_format[5];
extern char g_resource_loading_format[27];
extern char g_resource_async_open_failure_format[24];
extern char g_resource_sync_open_failure_format[20];
extern char g_resource_sync_loaded_format[11];
extern char g_resource_close_failure_format[11];
extern char g_resource_opening_format[12];
extern char g_resource_close_success_format[11];
#ifdef __cplusplus
}
#endif
#endif
