#include <stddef.h>
#include <stdio.h>
#include "resource_context.h"

extern ResourceCallbackContext *g_resource_context;
extern char g_resource_filename[];
extern const char g_resource_filename_format[];
extern const char g_resource_loading_format[];
extern const char g_resource_async_open_failure_format[];
extern const char g_resource_sync_open_failure_format[];
extern const char g_resource_sync_loaded_format[];
extern const char g_resource_close_failure_format[];
extern unsigned long g_resource_prepare_scalar;

extern void __cdecl host_diagnostic(const char *format, ...);
extern void __cdecl empty_host_callback(void);
extern int __cdecl virtual_file_open(const char *path, int mode, int flags);
extern long __cdecl virtual_file_seek(int handle, long offset, int origin);
extern size_t __cdecl virtual_file_read(int handle, void *destination, size_t bytes);
extern int __cdecl virtual_file_close(int handle);

void __cdecl host_file_prepare(void)
{
    ResourceCallbackContext *context;

    empty_host_callback();
    context = g_resource_context;
    context->prepare_state_1b00 = 0;
    g_resource_prepare_scalar = 0;
}

unsigned long __cdecl host_file_sync(
    const char *prefix, const char *suffix, void *destination, unsigned long byte_count)
{
    ResourceCallbackContext *context;
    int handle;
    int close_result;

    host_file_prepare();
    sprintf(g_resource_filename, g_resource_filename_format, prefix, suffix);
    handle = virtual_file_open(g_resource_filename, 0, 0);
    context = g_resource_context;
    context->virtual_handle_0b8 = handle;
    context = g_resource_context;
    if (context->virtual_handle_0b8 == -1)
        goto open_failed;

    for (;;) {
        host_diagnostic(g_resource_loading_format,
                        g_resource_filename, destination, (int)byte_count);
        context = g_resource_context;
        handle = context->virtual_handle_0b8;
        (void)virtual_file_read(handle, destination, byte_count);

        context = g_resource_context;
        handle = context->virtual_handle_0b8;
        close_result = virtual_file_close(handle);
        if (close_result == 0)
            break;

        handle = virtual_file_open(g_resource_filename, 0, 0);
        context = g_resource_context;
        context->virtual_handle_0b8 = handle;
        context = g_resource_context;
        if (context->virtual_handle_0b8 == -1)
            goto open_failed;
    }

    context = g_resource_context;
    context->transfer_status_0dc = 0;
    host_diagnostic(g_resource_sync_loaded_format);
    return byte_count;

open_failed:
    host_diagnostic(g_resource_sync_open_failure_format, g_resource_filename);
    return 0;
}

int __cdecl host_file_async(
    const char *prefix, const char *suffix, int sector_offset,
    void *destination, unsigned long byte_count)
{
    ResourceCallbackContext *context;
    int handle;
    unsigned long sector_byte_offset;
    unsigned long corrected_count;
    long signed_corrected_count;
    short sector_count;

    context = g_resource_context;
    if (context->mode_09c == 1)
        return 1;

    host_file_prepare();
    sprintf(g_resource_filename, g_resource_filename_format, prefix, suffix);
    handle = virtual_file_open(g_resource_filename, 0, 0);
    context = g_resource_context;
    context->virtual_handle_0b8 = handle;
    context = g_resource_context;
    handle = context->virtual_handle_0b8;
    if (handle == -1) {
        host_diagnostic(g_resource_async_open_failure_format,
                        g_resource_filename, (int)byte_count);
        return 3;
    }

    if (sector_offset != 0) {
        sector_byte_offset = (unsigned long)sector_offset << 11;
        (void)virtual_file_seek(handle, (long)sector_byte_offset, 0);
    }

    context = g_resource_context;
    context->destination_cursor_a4 = (unsigned char *)destination;
    context = g_resource_context;
    context->mode_09c = 1;

    corrected_count = byte_count + 2047UL;
    signed_corrected_count = (long)corrected_count;
    sector_count = (short)(signed_corrected_count / 2048L);
    context = g_resource_context;
    context->total_sectors_0a0 = sector_count;
    context = g_resource_context;
    context->remaining_sectors_09e = context->total_sectors_0a0;
    context = g_resource_context;
    context->countdown_0bc = 0;
    return 2;
}

void __cdecl host_cancel_load(void)
{
    ResourceCallbackContext *context;
    int handle;

    context = g_resource_context;
    if (context->mode_09c == 0)
        return;

    handle = context->virtual_handle_0b8;
    if (virtual_file_close(handle) != 0) {
        host_diagnostic(g_resource_close_failure_format);
        return;
    }

    context = g_resource_context;
    context->mode_09c = 0;
    context = g_resource_context;
    context->unknown_0ac = 0;
}

void __cdecl host_file_progress(void)
{
    ResourceCallbackContext *context;
    void (__cdecl *callback)();
    short chosen_sectors;
    long chosen_byte_count;
    unsigned long byte_delta;
    int handle;
    unsigned char *destination;

    context = g_resource_context;
    if (context->countdown_0bc != 0) {
        --context->countdown_0bc;
        context = g_resource_context;
        if (context->countdown_0bc == 0)
            context->mode_09c = 0;
    }

    context = g_resource_context;
    if (context->mode_09c != 1)
        return;

    if (context->transfer_result_0d8 == -1) {
        context->transfer_status_0dc = 0x100;
        context = g_resource_context;
        callback = context->load_failed;
        if (callback != 0)
            callback(0);
        context = g_resource_context;
        context->mode_09c = 0;
        context = g_resource_context;
        context->transfer_result_0d8 = 0;
        return;
    }

    chosen_sectors = context->remaining_sectors_09e;
    if (chosen_sectors < 5) {
        context->remaining_sectors_09e = 0;
    } else {
        chosen_sectors = 5;
        context->remaining_sectors_09e =
            (short)(context->remaining_sectors_09e - 5);
    }

    if (chosen_sectors != 0) {
        chosen_byte_count = (long)chosen_sectors * 2048L;
        byte_delta = (unsigned long)chosen_byte_count;
        context = g_resource_context;
        handle = context->virtual_handle_0b8;
        destination = context->destination_cursor_a4;
        (void)virtual_file_read(handle, destination, (size_t)byte_delta);
        context = g_resource_context;
        context->destination_cursor_a4 = (unsigned char *)
            ((unsigned long)context->destination_cursor_a4 + byte_delta);
    }

    context = g_resource_context;
    if (context->remaining_sectors_09e != 0)
        return;

    context->transfer_status_0dc = 0;
    context = g_resource_context;
    if (context->suppress_auto_close_0e0 == 0) {
        host_cancel_load();
        context = g_resource_context;
        context->mode_09c = 4;
        context = g_resource_context;
        context->countdown_0bc = 12;
    }

    context = g_resource_context;
    callback = context->load_complete;
    if (callback != 0)
        callback(0);
}

unsigned long __cdecl resource_file_length(const char *prefix, const char *suffix)
{
    ResourceCallbackContext *context;
    int handle;
    unsigned long length;
    int close_result;

    sprintf(g_resource_filename, g_resource_filename_format, prefix, suffix);
    handle = virtual_file_open(g_resource_filename, 0, 0);
    context = g_resource_context;
    context->virtual_handle_0b8 = handle;
    context = g_resource_context;
    handle = context->virtual_handle_0b8;
    if (handle == -1)
        return 0;

    for (;;) {
        length = (unsigned long)virtual_file_seek(handle, 0L, 2);
        context = g_resource_context;
        handle = context->virtual_handle_0b8;
        close_result = virtual_file_close(handle);
        if (close_result == 0)
            return length;

        handle = virtual_file_open(g_resource_filename, 0, 0);
        context = g_resource_context;
        context->virtual_handle_0b8 = handle;
        context = g_resource_context;
        handle = context->virtual_handle_0b8;
        if (handle == -1)
            return 0;
    }
}
