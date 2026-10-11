/*
 * 32-bit Win32 debug-event sampler for one disposable Hercules process.
 * The probe launches only its supplied host with DEBUG_ONLY_THIS_PROCESS,
 * starts the host visibly for windowed tests, and never changes the debuggee's
 * memory or code. Its optional normal WM_COMMAND is sent only to its own PID.
 * Only the initial OS loader breakpoint is continued as handled; all later
 * exceptions are passed to the system as DBG_EXCEPTION_NOT_HANDLED.
 *
 * Output is JSON Lines: start, process/module/thread/debug/exception records,
 * then one final process status record. It is diagnostic evidence, not a
 * gameplay or whole-binary equivalence claim.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN64)
#error Build this probe with the pinned 32-bit compiler.
#endif

typedef char probe_pointer_width_must_be_32[(sizeof(void *) == 4) ? 1 : -1];

static void json_string(FILE *out, const char *s)
{
    const unsigned char *p = (const unsigned char *)(s ? s : "");
    fputc('"', out);
    while (*p) {
        if (*p == '"' || *p == '\\') {
            fputc('\\', out);
            fputc(*p, out);
        } else if (*p < 0x20) {
            fprintf(out, "\\u%04x", (unsigned int)*p);
        } else {
            fputc(*p, out);
        }
        ++p;
    }
    fputc('"', out);
}

static void wide_to_ansi(const WCHAR *wide, char *out, DWORD out_count)
{
    if (!wide || !out || !out_count)
        return;
    out[0] = 0;
    WideCharToMultiByte(CP_ACP, 0, wide, -1, out, (int)out_count, 0, 0);
    out[out_count - 1] = 0;
}

static void read_remote_image_name(HANDLE process, LPVOID remote_name_slot,
                                   BOOL is_unicode, char *out, DWORD out_count)
{
    DWORD remote_name = 0;
    DWORD read_count = 0;
    out[0] = 0;
    if (!remote_name_slot || !ReadProcessMemory(process, remote_name_slot,
            &remote_name, sizeof(remote_name), &read_count) ||
        read_count != (DWORD)sizeof(remote_name) || !remote_name)
        return;

    if (is_unicode) {
        WCHAR wide[512];
        memset(wide, 0, sizeof(wide));
        if (ReadProcessMemory(process, (LPCVOID)remote_name, wide,
                (DWORD)(sizeof(wide) - sizeof(WCHAR)), &read_count) && read_count >= sizeof(WCHAR)) {
            wide[(read_count / sizeof(WCHAR)) < 512 ?
                 (read_count / sizeof(WCHAR)) : 511] = 0;
            wide_to_ansi(wide, out, out_count);
        }
    } else {
        if (ReadProcessMemory(process, (LPCVOID)remote_name, out,
                out_count - 1, &read_count) && read_count) {
            out[read_count < out_count ? read_count : out_count - 1] = 0;
        }
    }
}

static void file_identity(HANDLE file, DWORD *size_high, DWORD *size_low,
                          DWORD *volume, DWORD *index_high, DWORD *index_low)
{
    BY_HANDLE_FILE_INFORMATION info;
    memset(&info, 0, sizeof(info));
    if (file && file != INVALID_HANDLE_VALUE && GetFileInformationByHandle(file, &info)) {
        *size_high = info.nFileSizeHigh;
        *size_low = info.nFileSizeLow;
        *volume = info.dwVolumeSerialNumber;
        *index_high = info.nFileIndexHigh;
        *index_low = info.nFileIndexLow;
    } else {
        *size_high = *size_low = *volume = *index_high = *index_low = 0;
    }
}

static void log_file_record(FILE *out, const char *kind, DWORD pid, DWORD tid,
                            LPVOID base, LPVOID image_name_slot, BOOL unicode,
                            HANDLE file, HANDLE process)
{
    char image_name[1024];
    DWORD size_high, size_low, volume, index_high, index_low;
    read_remote_image_name(process, image_name_slot, unicode, image_name, sizeof(image_name));
    file_identity(file, &size_high, &size_low, &volume, &index_high, &index_low);
    fprintf(out, "{\"type\":\"%s\",\"pid\":%lu,\"tid\":%lu,\"base\":\"0x%08lx\",\"image_name\":",
            kind, (unsigned long)pid, (unsigned long)tid, (unsigned long)base);
    json_string(out, image_name);
    fprintf(out, ",\"file_size\":\"0x%08lx%08lx\",\"volume_serial\":\"0x%08lx\",\"file_index\":\"0x%08lx%08lx\"}\n",
            (unsigned long)size_high, (unsigned long)size_low,
            (unsigned long)volume, (unsigned long)index_high,
            (unsigned long)index_low);
    fflush(out);
}

static const char *exception_name(DWORD code)
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_PRIV_INSTRUCTION: return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
    default: return "other";
    }
}

#define HERCULES_WINDOWED_COMMAND 32801

static HWND g_main_window = 0;
static BOOL g_windowed_state_reported = FALSE;
static BOOL g_windowed_command_verified = FALSE;
static BOOL g_mode_toggle_sent = FALSE;
static BOOL g_windowed_resize_sent = FALSE;
static BOOL g_window_mode_confirmed = FALSE;
static DWORD g_windowed_post_count = 0;
static DWORD g_main_thread_id = 0;
static DWORD g_target_pid = 0;
static DWORD g_title_base = 0;
static DWORD g_title_pool_rva = 0;
static char g_report_path[4096];

static BOOL read_remote_exact(HANDLE process, DWORD address, void *buffer,
                              DWORD byte_count, DWORD *bytes_read_out);

static BOOL find_menu_command(HMENU menu, UINT command, char *text, DWORD text_count)
{
    int index;
    int count;
    if (!menu)
        return FALSE;
    count = GetMenuItemCount(menu);
    for (index = 0; index < count; ++index) {
        MENUITEMINFOA item;
        char item_text[256];
        memset(&item, 0, sizeof(item));
        memset(item_text, 0, sizeof(item_text));
        item.cbSize = sizeof(item);
        item.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_TYPE;
        item.dwTypeData = item_text;
        item.cch = sizeof(item_text) - 1;
        if (!GetMenuItemInfoA(menu, (UINT)index, TRUE, &item))
            continue;
        if (item.wID == command) {
            if (text && text_count) {
                lstrcpynA(text, item_text, (int)text_count);
                text[text_count - 1] = 0;
            }
            return TRUE;
        }
        if (item.hSubMenu && find_menu_command(item.hSubMenu, command, text, text_count))
            return TRUE;
    }
    return FALSE;
}

static void log_window_state(FILE *out, HWND window, const char *phase)
{
    DWORD pid = 0;
    DWORD thread_id;
    DWORD style;
    DWORD ex_style;
    RECT window_rect;
    RECT client_rect;
    HMENU menu;
    UINT menu_state = 0xffffffff;
    char title[512];
    char class_name[256];
    char command_text[256];
    BOOL command_present;

    thread_id = GetWindowThreadProcessId(window, &pid);
    style = (DWORD)GetWindowLongA(window, GWL_STYLE);
    ex_style = (DWORD)GetWindowLongA(window, GWL_EXSTYLE);
    GetWindowRect(window, &window_rect);
    GetClientRect(window, &client_rect);
    title[0] = 0;
    class_name[0] = 0;
    command_text[0] = 0;
    GetWindowTextA(window, title, sizeof(title));
    GetClassNameA(window, class_name, sizeof(class_name));
    menu = GetMenu(window);
    command_present = find_menu_command(menu, HERCULES_WINDOWED_COMMAND,
            command_text, sizeof(command_text));
    if (menu)
        menu_state = GetMenuState(menu, HERCULES_WINDOWED_COMMAND, MF_BYCOMMAND);

    fprintf(out, "{\"type\":\"window_state\",\"phase\":");
    json_string(out, phase);
    fprintf(out, ",\"hwnd\":\"0x%08lx\",\"pid\":%lu,\"tid\":%lu,\"title\":",
            (unsigned long)window, (unsigned long)pid, (unsigned long)thread_id);
    json_string(out, title);
    fprintf(out, ",\"class\":");
    json_string(out, class_name);
    fprintf(out, ",\"visible\":%s,\"style\":\"0x%08lx\",\"ex_style\":\"0x%08lx\",\"window_rect\":[%ld,%ld,%ld,%ld],\"client_rect\":[%ld,%ld,%ld,%ld],\"menu_available\":%s,\"windowed_command_present\":%s,\"windowed_command_text\":",
            IsWindowVisible(window) ? "true" : "false",
            (unsigned long)style, (unsigned long)ex_style,
            (long)window_rect.left, (long)window_rect.top,
            (long)window_rect.right, (long)window_rect.bottom,
            (long)client_rect.left, (long)client_rect.top,
            (long)client_rect.right, (long)client_rect.bottom,
            menu ? "true" : "false", command_present ? "true" : "false");
    json_string(out, command_text);
    if (menu_state == 0xffffffff)
        fprintf(out, ",\"windowed_menu_state\":null}\n");
    else
        fprintf(out, ",\"windowed_menu_state\":%u,\"windowed_checked\":%s,\"windowed_enabled\":%s}\n",
                menu_state, (menu_state & MF_CHECKED) ? "true" : "false",
                (menu_state & (MF_DISABLED | MF_GRAYED)) ? "false" : "true");
    fflush(out);
}

typedef struct window_request_context {
    FILE *out;
    DWORD pid;
    HWND found;
} WINDOW_REQUEST_CONTEXT;

static BOOL CALLBACK find_main_window(HWND window, LPARAM value)
{
    WINDOW_REQUEST_CONTEXT *context = (WINDOW_REQUEST_CONTEXT *)value;
    DWORD pid = 0;
    HMENU menu;
    char menu_text[256];
    if (GetWindowThreadProcessId(window, &pid) == 0 || pid != context->pid)
        return TRUE;
    menu = GetMenu(window);
    if (!menu || !find_menu_command(menu, HERCULES_WINDOWED_COMMAND,
            menu_text, sizeof(menu_text)))
        return TRUE;
    context->found = window;
    return FALSE;
}

static BOOL CALLBACK write_bmp_header(FILE *file, DWORD width, DWORD height,
                                      DWORD stride, DWORD image_size)
{
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    memset(&file_header, 0, sizeof(file_header));
    memset(&info_header, 0, sizeof(info_header));
    file_header.bfType = 0x4d42;
    file_header.bfOffBits = sizeof(file_header) + sizeof(info_header);
    file_header.bfSize = file_header.bfOffBits + image_size;
    info_header.biSize = sizeof(info_header);
    info_header.biWidth = (LONG)width;
    info_header.biHeight = (LONG)height;
    info_header.biPlanes = 1;
    info_header.biBitCount = 24;
    info_header.biCompression = BI_RGB;
    info_header.biSizeImage = image_size;
    if (fwrite(&file_header, sizeof(file_header), 1, file) != 1)
        return FALSE;
    if (fwrite(&info_header, sizeof(info_header), 1, file) != 1)
        return FALSE;
    return TRUE;
}

static BOOL capture_window_client(FILE *out, HWND window, const char *report_path,
                                  const char *phase)
{
    HDC window_dc = 0;
    HDC memory_dc = 0;
    HBITMAP bitmap = 0;
    HGDIOBJ old_bitmap = 0;
    BITMAPINFO info;
    RECT client_rect;
    DWORD width;
    DWORD height;
    DWORD stride;
    DWORD image_size;
    DWORD lines = 0;
    unsigned char *pixels = 0;
    char bmp_path[4096];
    char *extension;
    FILE *bmp = 0;
    BOOL result = FALSE;

    if (!window || !GetClientRect(window, &client_rect))
        return FALSE;
    width = (DWORD)(client_rect.right - client_rect.left);
    height = (DWORD)(client_rect.bottom - client_rect.top);
    if (!width || !height || width > 4096 || height > 4096)
        return FALSE;

    window_dc = GetDC(window);
    if (!window_dc)
        goto done;
    memory_dc = CreateCompatibleDC(window_dc);
    if (!memory_dc)
        goto done;
    bitmap = CreateCompatibleBitmap(window_dc, (int)width, (int)height);
    if (!bitmap)
        goto done;
    old_bitmap = SelectObject(memory_dc, bitmap);
    if (!old_bitmap || old_bitmap == (HGDIOBJ)HGDI_ERROR)
        goto done;
    if (!BitBlt(memory_dc, 0, 0, (int)width, (int)height,
                window_dc, 0, 0, SRCCOPY))
        goto done;
    SelectObject(memory_dc, old_bitmap);
    old_bitmap = 0;

    stride = ((width * 24 + 31) / 32) * 4;
    image_size = stride * height;
    pixels = (unsigned char *)malloc(image_size);
    if (!pixels)
        goto done;
    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = (LONG)width;
    info.bmiHeader.biHeight = (LONG)height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 24;
    info.bmiHeader.biCompression = BI_RGB;
    lines = GetDIBits(window_dc, bitmap, 0, height, pixels, &info, DIB_RGB_COLORS);
    if (lines != height)
        goto done;

    lstrcpynA(bmp_path, report_path, sizeof(bmp_path));
    bmp_path[sizeof(bmp_path) - 1] = 0;
    extension = strrchr(bmp_path, '.');
    if (extension)
        *extension = 0;
    if (strlen(bmp_path) + strlen(phase) + 12 >= sizeof(bmp_path))
        goto done;
    strcat(bmp_path, ".");
    strcat(bmp_path, phase);
    strcat(bmp_path, ".bmp");
    bmp = fopen(bmp_path, "wb");
    if (!bmp)
        goto done;
    if (write_bmp_header(bmp, width, height, stride, image_size) &&
        fwrite(pixels, image_size, 1, bmp) == 1) {
        fflush(bmp);
        result = TRUE;
    }

done:
    if (bmp)
        fclose(bmp);
    if (pixels)
        free(pixels);
    if (old_bitmap && memory_dc)
        SelectObject(memory_dc, old_bitmap);
    if (bitmap)
        DeleteObject(bitmap);
    if (memory_dc)
        DeleteDC(memory_dc);
    if (window_dc)
        ReleaseDC(window, window_dc);
    if (out) {
        fprintf(out, "{\"type\":\"window_capture\",\"phase\":");
        json_string(out, phase);
        fprintf(out, ",\"hwnd\":\"0x%08lx\",\"width\":%lu,\"height\":%lu,\"result\":%s,\"path\":",
                (unsigned long)window, (unsigned long)width, (unsigned long)height,
                result ? "true" : "false");
        json_string(out, result ? bmp_path : "");
        fprintf(out, "}\n");
        fflush(out);
    }
    return result;
}

static void poll_for_windowed_menu(FILE *out, HANDLE process, DWORD pid,
                                   const char *report_path)
{
    WINDOW_REQUEST_CONTEXT context;
    DWORD window_pid = 0;
    DWORD style = 0;
    RECT client_rect;
    BOOL menu_present = FALSE;
    BOOL confirmed = FALSE;
    BOOL posted;
    BYTE requested_fullscreen = 0xff;
    BYTE current_mode = 0xff;
    DWORD requested_bytes = 0;
    DWORD current_bytes = 0;
    BOOL requested_read_ok;
    BOOL current_read_ok;
    DWORD error_code;
    char menu_text[256];
    char title[512];
    if (!out)
        return;
    memset(&context, 0, sizeof(context));
    context.out = out;
    context.pid = pid;
    EnumWindows(find_main_window, (LPARAM)&context);
    if (context.found) {
        g_main_window = context.found;
        g_windowed_command_verified = TRUE;
    }
    if (!g_windowed_command_verified || !g_main_window || !IsWindow(g_main_window))
        return;
    if (GetWindowThreadProcessId(g_main_window, &window_pid) == 0 || window_pid != pid) {
        g_main_window = 0;
        return;
    }
    {
        HMENU menu = GetMenu(g_main_window);
        char menu_check[256];
        menu_check[0] = 0;
        menu_present = menu && find_menu_command(menu,
                HERCULES_WINDOWED_COMMAND, menu_check, sizeof(menu_check));
    }

    SetRectEmpty(&client_rect);
    style = (DWORD)GetWindowLongA(g_main_window, GWL_STYLE);
    if (GetClientRect(g_main_window, &client_rect)) {
        confirmed = IsWindowVisible(g_main_window) &&
                client_rect.right - client_rect.left == 320 &&
                client_rect.bottom - client_rect.top == 240 &&
                (style & WS_CAPTION) != 0 && (style & WS_POPUP) == 0;
    }
    requested_read_ok = read_remote_exact(process, 0x0096e3d0,
            &requested_fullscreen, sizeof(requested_fullscreen), &requested_bytes);
    current_read_ok = read_remote_exact(process, 0x00ba020c,
            &current_mode, sizeof(current_mode), &current_bytes);

    /* The menu command only resizes the window. First use Hercules' ordinary
     * Alt+Enter accelerator to leave fullscreen, and require both native mode
     * bytes to confirm that change before posting the size command once. */
    if (requested_read_ok && current_read_ok &&
        requested_fullscreen == 1 && current_mode == 1 && !g_mode_toggle_sent) {
        BOOL down_posted;
        BOOL up_posted;
        DWORD down_error;
        DWORD up_error;
        SetLastError(0);
        down_posted = PostMessageA(g_main_window, WM_SYSKEYDOWN,
                (WPARAM)VK_RETURN, (LPARAM)0x201c0001L);
        down_error = down_posted ? 0 : GetLastError();
        SetLastError(0);
        up_posted = PostMessageA(g_main_window, WM_SYSKEYUP,
                (WPARAM)VK_RETURN, (LPARAM)0xe01c0001L);
        up_error = up_posted ? 0 : GetLastError();
        g_mode_toggle_sent = TRUE;
        fprintf(out, "{\"type\":\"native_alt_enter_request\",\"hwnd\":\"0x%08lx\",\"pid\":%lu,\"requested_fullscreen\":%u,\"current_mode\":%u,\"syskeydown\":\"0x201c0001\",\"syskeyup\":\"0xe01c0001\",\"keydown_posted\":%s,\"keydown_error\":%lu,\"keyup_posted\":%s,\"keyup_error\":%lu}\n",
                (unsigned long)g_main_window, (unsigned long)pid,
                (unsigned int)requested_fullscreen, (unsigned int)current_mode,
                down_posted ? "true" : "false", (unsigned long)down_error,
                up_posted ? "true" : "false", (unsigned long)up_error);
        fflush(out);
    }

    if (requested_read_ok && current_read_ok &&
        requested_fullscreen == 0 && current_mode == 0) {
        if (!g_window_mode_confirmed) {
            fprintf(out, "{\"type\":\"native_window_mode_confirmation\",\"pid\":%lu,\"requested_fullscreen\":%u,\"current_mode\":%u,\"elapsed_tick\":%lu}\n",
                    (unsigned long)pid, (unsigned int)requested_fullscreen,
                    (unsigned int)current_mode, (unsigned long)GetTickCount());
            fflush(out);
            g_window_mode_confirmed = TRUE;
        }
        if (!g_windowed_resize_sent && menu_present) {
            menu_text[0] = 0;
            title[0] = 0;
            if (GetMenu(g_main_window))
                find_menu_command(GetMenu(g_main_window), HERCULES_WINDOWED_COMMAND,
                        menu_text, sizeof(menu_text));
            GetWindowTextA(g_main_window, title, sizeof(title));
            SetLastError(0);
            posted = PostMessageA(g_main_window, WM_COMMAND,
                    (WPARAM)HERCULES_WINDOWED_COMMAND, (LPARAM)0);
            error_code = posted ? 0 : GetLastError();
            g_windowed_resize_sent = TRUE;
            ++g_windowed_post_count;
            fprintf(out, "{\"type\":\"windowed_size_request\",\"hwnd\":\"0x%08lx\",\"pid\":%lu,\"command\":%u,\"attempt\":%lu,\"native_mode_confirmed\":true,\"menu_verified\":true,\"menu_text\":",
                    (unsigned long)g_main_window, (unsigned long)pid,
                    (unsigned int)HERCULES_WINDOWED_COMMAND,
                    (unsigned long)g_windowed_post_count);
            json_string(out, menu_text);
            fprintf(out, ",\"title\":");
            json_string(out, title);
            fprintf(out, ",\"post_succeeded\":%s,\"error\":%lu}\n",
                    posted ? "true" : "false", (unsigned long)error_code);
            fflush(out);
            if (g_windowed_post_count == 1)
                log_window_state(out, g_main_window, "after_native_mode_before_size_command");
        }
    }

    if (confirmed && g_window_mode_confirmed && !g_windowed_state_reported) {
        log_window_state(out, g_main_window, "windowed_mode_confirmed");
        capture_window_client(out, g_main_window, report_path, "windowed_mode_confirmed");
        fprintf(out, "{\"type\":\"windowed_mode_confirmation\",\"hwnd\":\"0x%08lx\",\"pid\":%lu,\"client_width\":320,\"client_height\":240,\"caption_style\":true,\"requested_fullscreen\":%u,\"current_mode\":%u}\n",
                (unsigned long)g_main_window, (unsigned long)pid,
                (unsigned int)requested_fullscreen, (unsigned int)current_mode);
        fflush(out);
        g_windowed_state_reported = TRUE;
    } else if (!confirmed) {
        g_windowed_state_reported = FALSE;
    }

}

static void log_exception_context(FILE *out, HANDLE process, DWORD tid,
                                  EXCEPTION_DEBUG_INFO *exception)
{
    HANDLE thread;
    CONTEXT context;
    BOOL context_ok = FALSE;
    DWORD stack[16];
    DWORD bytes_read = 0;
    DWORD index;
    DWORD access_type = 0xffffffff;
    DWORD fault_address = 0;
    DWORD parameter_count = exception->ExceptionRecord.NumberParameters;
    memset(&context, 0, sizeof(context));
    memset(stack, 0, sizeof(stack));
    {
        typedef HANDLE (WINAPI *OPEN_THREAD_PROC)(DWORD, BOOL, DWORD);
        OPEN_THREAD_PROC open_thread = (OPEN_THREAD_PROC)GetProcAddress(
                GetModuleHandleA("KERNEL32.DLL"), "OpenThread");
        thread = open_thread ? open_thread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                FALSE, tid) : 0;
    }
    if (thread) {
        context.ContextFlags = CONTEXT_FULL;
        context_ok = GetThreadContext(thread, &context);
        CloseHandle(thread);
    }
    if (exception->ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        parameter_count >= 2) {
        access_type = (DWORD)exception->ExceptionRecord.ExceptionInformation[0];
        fault_address = (DWORD)exception->ExceptionRecord.ExceptionInformation[1];
    }
    if (context_ok && context.Esp)
        ReadProcessMemory(process, (LPCVOID)context.Esp, stack,
                sizeof(stack), &bytes_read);

    fprintf(out, "{\"type\":\"exception_context\",\"tid\":%lu,\"context_read\":%s,\"eip\":\"0x%08lx\",\"esp\":\"0x%08lx\",\"ebp\":\"0x%08lx\",\"eax\":\"0x%08lx\",\"ebx\":\"0x%08lx\",\"ecx\":\"0x%08lx\",\"edx\":\"0x%08lx\",\"esi\":\"0x%08lx\",\"edi\":\"0x%08lx\",\"access_type\":%lu,\"fault_address\":\"0x%08lx\",\"stack_bytes_read\":%lu,\"stack_words\":[",
            (unsigned long)tid, context_ok ? "true" : "false",
            (unsigned long)context.Eip, (unsigned long)context.Esp,
            (unsigned long)context.Ebp, (unsigned long)context.Eax,
            (unsigned long)context.Ebx, (unsigned long)context.Ecx,
            (unsigned long)context.Edx, (unsigned long)context.Esi,
            (unsigned long)context.Edi, (unsigned long)access_type,
            (unsigned long)fault_address, (unsigned long)bytes_read);
    for (index = 0; index < bytes_read / sizeof(DWORD) && index < 16; ++index) {
        if (index)
            fputc(',', out);
        fprintf(out, "\"0x%08lx\"", (unsigned long)stack[index]);
    }
    fprintf(out, "]}\n");
    fflush(out);
}

typedef struct window_snapshot_context {
    FILE *out;
    DWORD pid;
    HWND parent;
    const char *kind;
    DWORD count;
} WINDOW_SNAPSHOT_CONTEXT;

static void snapshot_one_window(HWND window, WINDOW_SNAPSHOT_CONTEXT *context)
{
    DWORD window_pid = 0;
    DWORD thread_id;
    RECT rect;
    char class_name[256];
    char title[1024];
    BOOL visible;
    int control_id;
    LRESULT message_result = 0;
    BOOL queried_text = FALSE;
    DWORD style;
    DWORD ex_style;
    HMENU menu;
    UINT windowed_state = 0xffffffff;
    char menu_text[256];
    BOOL windowed_present;

    thread_id = GetWindowThreadProcessId(window, &window_pid);
    if (window_pid != context->pid)
        return;

    class_name[0] = 0;
    title[0] = 0;
    GetClassNameA(window, class_name, sizeof(class_name));
    GetWindowTextA(window, title, sizeof(title));
    visible = IsWindowVisible(window);
    if (!title[0]) {
        /* GetWindowText cannot retrieve control text across processes. A
         * bounded, read-only WM_GETTEXT lets us capture dialog/static text
         * without risking an unbounded wait on an unresponsive window. */
        queried_text = SendMessageTimeoutA(window, WM_GETTEXT,
                (WPARAM)sizeof(title), (LPARAM)title,
                SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, &message_result) != 0;
        title[sizeof(title) - 1] = 0;
    }
    control_id = GetDlgCtrlID(window);
    style = (DWORD)GetWindowLongA(window, GWL_STYLE);
    ex_style = (DWORD)GetWindowLongA(window, GWL_EXSTYLE);
    menu = GetMenu(window);
    menu_text[0] = 0;
    windowed_present = find_menu_command(menu, HERCULES_WINDOWED_COMMAND,
            menu_text, sizeof(menu_text));
    if (menu)
        windowed_state = GetMenuState(menu, HERCULES_WINDOWED_COMMAND, MF_BYCOMMAND);
    if (!GetWindowRect(window, &rect))
        SetRectEmpty(&rect);

    fprintf(context->out,
            "{\"type\":\"window\",\"kind\":\"%s\",\"hwnd\":\"0x%08lx\",\"pid\":%lu,\"tid\":%lu,\"class\":",
            context->kind, (unsigned long)window, (unsigned long)window_pid,
            (unsigned long)thread_id);
    json_string(context->out, class_name);
    fprintf(context->out, ",\"text\":");
    json_string(context->out, title);
    fprintf(context->out,
            ",\"text_query_sent\":%s,\"text_query_result\":%ld,\"visible\":%s,\"control_id\":%d,\"rect\":[%ld,%ld,%ld,%ld],\"style\":\"0x%08lx\",\"ex_style\":\"0x%08lx\",\"menu_available\":%s,\"windowed_command_present\":%s,\"windowed_command_text\":",
            queried_text ? "true" : "false", (long)message_result,
            visible ? "true" : "false", control_id,
            (long)rect.left, (long)rect.top, (long)rect.right, (long)rect.bottom,
            (unsigned long)style, (unsigned long)ex_style,
            menu ? "true" : "false", windowed_present ? "true" : "false");
    json_string(context->out, menu_text);
    if (windowed_state == 0xffffffff)
        fprintf(context->out, ",\"windowed_menu_state\":null,\"parent_hwnd\":\"0x%08lx\"}\n",
                (unsigned long)context->parent);
    else
        fprintf(context->out, ",\"windowed_menu_state\":%u,\"windowed_checked\":%s,\"windowed_enabled\":%s,\"parent_hwnd\":\"0x%08lx\"}\n",
                windowed_state, (windowed_state & MF_CHECKED) ? "true" : "false",
                (windowed_state & (MF_DISABLED | MF_GRAYED)) ? "false" : "true",
                (unsigned long)context->parent);
    ++context->count;
}

static BOOL CALLBACK snapshot_child_window(HWND window, LPARAM value)
{
    WINDOW_SNAPSHOT_CONTEXT *context = (WINDOW_SNAPSHOT_CONTEXT *)value;
    snapshot_one_window(window, context);
    return TRUE;
}

static BOOL CALLBACK snapshot_top_window(HWND window, LPARAM value)
{
    WINDOW_SNAPSHOT_CONTEXT *context = (WINDOW_SNAPSHOT_CONTEXT *)value;
    WINDOW_SNAPSHOT_CONTEXT child_context;
    snapshot_one_window(window, context);
    child_context = *context;
    child_context.parent = window;
    child_context.kind = "child";
    EnumChildWindows(window, snapshot_child_window, (LPARAM)&child_context);
    context->count += child_context.count - context->count;
    return TRUE;
}

static void snapshot_process_windows(FILE *out, DWORD pid)
{
    WINDOW_SNAPSHOT_CONTEXT context;
    memset(&context, 0, sizeof(context));
    context.out = out;
    context.pid = pid;
    context.kind = "top_level";
    fprintf(out, "{\"type\":\"windows_snapshot_begin\",\"pid\":%lu}\n",
            (unsigned long)pid);
    fflush(out);
    EnumWindows(snapshot_top_window, (LPARAM)&context);
    if (g_main_window && IsWindow(g_main_window)) {
        log_window_state(out, g_main_window, "timeout_snapshot");
        capture_window_client(out, g_main_window, g_report_path, "timeout");
    }
    fprintf(out, "{\"type\":\"windows_snapshot_end\",\"pid\":%lu,\"window_count\":%lu}\n",
            (unsigned long)pid, (unsigned long)context.count);
    fflush(out);
}

static void log_process_virtualization(FILE *out, HANDLE process, DWORD pid)
{
    HANDLE token = 0;
    DWORD allowed = 0;
    DWORD enabled = 0;
    DWORD returned = 0;
    BOOL opened;
    BOOL allowed_ok = FALSE;
    BOOL enabled_ok = FALSE;
    opened = OpenProcessToken(process, TOKEN_QUERY, &token);
    if (opened) {
        allowed_ok = GetTokenInformation(token, (TOKEN_INFORMATION_CLASS)23,
                &allowed, sizeof(allowed), &returned);
        returned = 0;
        enabled_ok = GetTokenInformation(token, (TOKEN_INFORMATION_CLASS)24,
                &enabled, sizeof(enabled), &returned);
        CloseHandle(token);
    }
    fprintf(out,
            "{\"type\":\"token_virtualization\",\"pid\":%lu,\"token_opened\":%s,\"allowed_query_succeeded\":%s,\"allowed\":%s,\"enabled_query_succeeded\":%s,\"enabled\":%s}\n",
            (unsigned long)pid, opened ? "true" : "false",
            allowed_ok ? "true" : "false", allowed ? "true" : "false",
            enabled_ok ? "true" : "false", enabled ? "true" : "false");
    fflush(out);
}

static BOOL read_remote_exact(HANDLE process, DWORD address, void *buffer,
                              DWORD byte_count, DWORD *bytes_read_out)
{
    DWORD bytes_read = 0;
    BOOL ok = ReadProcessMemory(process, (LPCVOID)address, buffer,
            byte_count, &bytes_read);
    if (bytes_read_out)
        *bytes_read_out = bytes_read;
    return ok && bytes_read == byte_count;
}

static void log_title_process_pool_snapshot(FILE *out, HANDLE process,
                                            DWORD pid, DWORD elapsed,
                                            const char *phase)
{
    DWORD pool_address = 0;
    DWORD pool_words[16][6];
    DWORD bytes_read = 0;
    BOOL read_ok = FALSE;
    DWORD i;
    if (!out || pid != g_target_pid)
        return;
    memset(pool_words, 0, sizeof(pool_words));
    if (g_title_base && g_title_pool_rva <= 0xffffffff - g_title_base) {
        pool_address = g_title_base + g_title_pool_rva;
        read_ok = read_remote_exact(process, pool_address, pool_words,
                sizeof(pool_words), &bytes_read);
    }
    fprintf(out, "{\"type\":\"title_process_pool_snapshot\",\"phase\":");
    json_string(out, phase);
    fprintf(out, ",\"pid\":%lu,\"elapsed_ms\":%lu,\"module_base\":\"0x%08lx\",\"pool_rva\":\"0x%08lx\",\"pool_address\":\"0x%08lx\",\"read_ok\":%s,\"bytes_read\":%lu,\"entry_size\":24,\"entry_count\":16,\"entries\":[",
            (unsigned long)pid, (unsigned long)elapsed,
            (unsigned long)g_title_base, (unsigned long)g_title_pool_rva,
            (unsigned long)pool_address, read_ok ? "true" : "false",
            (unsigned long)bytes_read);
    if (read_ok) {
        for (i = 0; i < 16; ++i) {
            if (i)
                fprintf(out, ",");
            fprintf(out, "{\"index\":%lu,\"fn\":\"0x%08lx\",\"state\":\"0x%08lx\",\"delay\":\"0x%08lx\",\"next\":\"0x%08lx\",\"word_10\":\"0x%08lx\",\"word_14\":\"0x%08lx\"}",
                    (unsigned long)i, (unsigned long)pool_words[i][0],
                    (unsigned long)pool_words[i][1], (unsigned long)pool_words[i][2],
                    (unsigned long)pool_words[i][3], (unsigned long)pool_words[i][4],
                    (unsigned long)pool_words[i][5]);
        }
    }
    fprintf(out, "]}\n");
    fflush(out);
}

static void log_remote_byte(FILE *out, HANDLE process, const char *name,
                            DWORD address)
{
    BYTE value = 0;
    DWORD bytes_read = 0;
    BOOL ok = read_remote_exact(process, address, &value, sizeof(value), &bytes_read);
    fprintf(out, "\"%s\":{\"address\":\"0x%08lx\",\"read_ok\":%s,\"bytes_read\":%lu,\"value\":\"0x%02x\"}",
            name, (unsigned long)address, ok ? "true" : "false",
            (unsigned long)bytes_read, (unsigned int)value);
}

static void log_remote_dword(FILE *out, HANDLE process, const char *name,
                             DWORD address)
{
    DWORD value = 0;
    DWORD bytes_read = 0;
    BOOL ok = read_remote_exact(process, address, &value, sizeof(value), &bytes_read);
    fprintf(out, "\"%s\":{\"address\":\"0x%08lx\",\"read_ok\":%s,\"bytes_read\":%lu,\"value\":\"0x%08lx\"}",
            name, (unsigned long)address, ok ? "true" : "false",
            (unsigned long)bytes_read, (unsigned long)value);
}

static void log_host_state_snapshot(FILE *out, HANDLE process, DWORD pid,
                                    DWORD tid, DWORD start_tick,
                                    const char *phase)
{
    DWORD engine_context = 0;
    DWORD bytes_read = 0;
    BOOL context_pointer_ok;
    static const DWORD context_offsets[] = {0x3, 0x6, 0x30, 0x128};
    static const DWORD movie_flag_addresses[] = {
        0x0091c44c, 0x00ba0238, 0x00ba0230, 0x00ba0228
    };
    static const char *movie_flag_names[] = {
        "movie_flags_91c44c", "movie_flags_ba0238",
        "movie_flags_ba0230", "movie_flags_ba0228"
    };
    HANDLE thread = 0;
    DWORD suspend_result = 0xffffffff;
    DWORD resume_result = 0xffffffff;
    DWORD open_thread_error = 0;
    BOOL thread_context_ok = FALSE;
    BOOL suspended = FALSE;
    CONTEXT thread_context;
    DWORD stack[16];
    DWORD stack_bytes = 0;
    DWORD i;
    DWORD elapsed = GetTickCount() - start_tick;

    if (!out || pid != g_target_pid)
        return;
    memset(&thread_context, 0, sizeof(thread_context));
    memset(stack, 0, sizeof(stack));
    context_pointer_ok = read_remote_exact(process, 0x004361bc,
            &engine_context, sizeof(engine_context), &bytes_read);

    fprintf(out, "{\"type\":\"host_state_snapshot\",\"phase\":");
    json_string(out, phase);
    fprintf(out, ",\"pid\":%lu,\"elapsed_ms\":%lu,\"host\":{",
            (unsigned long)pid, (unsigned long)elapsed);
    log_remote_byte(out, process, "requested_fullscreen", 0x0096e3d0);
    fprintf(out, ",");
    log_remote_byte(out, process, "current_mode", 0x00ba020c);
    fprintf(out, ",\"engine_context_pointer\":{\"address\":\"0x004361bc\",\"read_ok\":%s,\"bytes_read\":%lu,\"value\":\"0x%08lx\",\"fields\":{",
            context_pointer_ok ? "true" : "false", (unsigned long)bytes_read,
            (unsigned long)engine_context);
    for (i = 0; i < sizeof(context_offsets) / sizeof(context_offsets[0]); ++i) {
        char field_name[32];
        DWORD field_value = 0;
        DWORD field_bytes = 0;
        BOOL field_ok = FALSE;
        sprintf(field_name, "offset_%03lx", (unsigned long)context_offsets[i]);
        if (context_pointer_ok && engine_context <= 0xffffffff - context_offsets[i])
            field_ok = read_remote_exact(process, engine_context + context_offsets[i],
                    &field_value, sizeof(field_value), &field_bytes);
        if (i)
            fprintf(out, ",");
        fprintf(out, "\"%s\":{\"offset\":\"0x%lx\",\"read_ok\":%s,\"bytes_read\":%lu,\"value\":\"0x%08lx\"}",
                field_name, (unsigned long)context_offsets[i],
                field_ok ? "true" : "false", (unsigned long)field_bytes,
                (unsigned long)field_value);
    }
    fprintf(out, "}},\"movie_flags\":{");
    for (i = 0; i < sizeof(movie_flag_addresses) / sizeof(movie_flag_addresses[0]); ++i) {
        if (i)
            fprintf(out, ",");
        log_remote_dword(out, process, movie_flag_names[i], movie_flag_addresses[i]);
    }
    fprintf(out, "},\"main_thread\":{\"tid\":%lu,\"open_ok\":",
            (unsigned long)tid);
    if (tid == 0)
        tid = g_main_thread_id;
    if (tid == 0) {
        fprintf(out, "false,\"suspend_succeeded\":false,\"context_ok\":false,\"resume_called\":false}}");
    } else {
        typedef HANDLE (WINAPI *OPEN_THREAD_PROC)(DWORD, BOOL, DWORD);
        OPEN_THREAD_PROC open_thread = (OPEN_THREAD_PROC)GetProcAddress(
                GetModuleHandleA("KERNEL32.DLL"), "OpenThread");
        SetLastError(0);
        if (open_thread)
            thread = open_thread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                    THREAD_QUERY_INFORMATION, FALSE, tid);
        if (!thread)
            open_thread_error = GetLastError();
        fprintf(out, "%s,\"open_error\":%lu,\"suspend_succeeded\":",
                thread ? "true" : "false", (unsigned long)open_thread_error);
        if (thread) {
            suspend_result = SuspendThread(thread);
            suspended = suspend_result != 0xffffffff;
            fprintf(out, "%s,\"suspend_previous_count\":%lu,\"context_ok\":",
                    suspended ? "true" : "false", (unsigned long)suspend_result);
            if (suspended) {
                thread_context.ContextFlags = CONTEXT_FULL;
                thread_context_ok = GetThreadContext(thread, &thread_context);
            }
            fprintf(out, "%s", thread_context_ok ? "true" : "false");
            if (thread_context_ok) {
                read_remote_exact(process, thread_context.Esp, stack,
                        sizeof(stack), &stack_bytes);
                fprintf(out, ",\"eip\":\"0x%08lx\",\"esp\":\"0x%08lx\",\"ebp\":\"0x%08lx\",\"eax\":\"0x%08lx\",\"ebx\":\"0x%08lx\",\"ecx\":\"0x%08lx\",\"edx\":\"0x%08lx\",\"esi\":\"0x%08lx\",\"edi\":\"0x%08lx\",\"stack_bytes_read\":%lu,\"stack\":[",
                        (unsigned long)thread_context.Eip,
                        (unsigned long)thread_context.Esp,
                        (unsigned long)thread_context.Ebp,
                        (unsigned long)thread_context.Eax,
                        (unsigned long)thread_context.Ebx,
                        (unsigned long)thread_context.Ecx,
                        (unsigned long)thread_context.Edx,
                        (unsigned long)thread_context.Esi,
                        (unsigned long)thread_context.Edi,
                        (unsigned long)stack_bytes);
                for (i = 0; i < stack_bytes / sizeof(DWORD) && i < 16; ++i) {
                    if (i)
                        fprintf(out, ",");
                    fprintf(out, "\"0x%08lx\"", (unsigned long)stack[i]);
                }
                fprintf(out, "]");
            }
            if (suspended)
                resume_result = ResumeThread(thread);
            fprintf(out, ",\"resume_called\":%s,\"resume_previous_count\":%lu",
                    suspended ? "true" : "false", (unsigned long)resume_result);
            CloseHandle(thread);
        } else {
            fprintf(out, "false,\"suspend_succeeded\":false,\"context_ok\":false,\"resume_called\":false");
        }
        fprintf(out, "}}");
    }
    fprintf(out, "}\n");
    fflush(out);
    log_title_process_pool_snapshot(out, process, pid, elapsed, phase);
}

static int launch_debugged(const char *host, const char *cwd,
                           const char *report_path, DWORD timeout_ms,
                           BOOL windowed, DWORD title_pool_rva)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION process_info;
    DEBUG_EVENT event;
    FILE *out;
    char command_line[4096];
    DWORD start_tick, exit_code = STILL_ACTIVE, last_error = 0;
    DWORD pid = 0;
    BOOL created = FALSE, timed_out = FALSE, exited = FALSE;
    BOOL loader_breakpoint_pending = FALSE;
    DWORD reason = 0;

    out = fopen(report_path, "wb");
    if (!out) {
        fprintf(stderr, "cannot create report: %s\n", report_path);
        return 2;
    }

    memset(&startup, 0, sizeof(startup));
    memset(&process_info, 0, sizeof(process_info));
    g_main_window = 0;
    g_windowed_state_reported = FALSE;
    g_windowed_command_verified = FALSE;
    g_windowed_post_count = 0;
    g_target_pid = 0;
    g_main_thread_id = process_info.dwThreadId;
    g_title_base = 0;
    g_title_pool_rva = title_pool_rva;
    lstrcpynA(g_report_path, report_path, sizeof(g_report_path));
    g_report_path[sizeof(g_report_path) - 1] = 0;
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = windowed ? SW_SHOWNORMAL : SW_HIDE;
    if (strlen(host) + 3 >= sizeof(command_line)) {
        fprintf(out, "{\"type\":\"error\",\"reason\":\"host path too long\"}\n");
        fclose(out);
        return 2;
    }
    sprintf(command_line, "\"%s\"", host);

    created = CreateProcessA(host, command_line, 0, 0, FALSE,
            DEBUG_ONLY_THIS_PROCESS, 0, cwd, &startup, &process_info);
    if (!created) {
        last_error = GetLastError();
        fprintf(out, "{\"type\":\"create_process_failed\",\"error\":%lu,\"host\":",
                (unsigned long)last_error);
        json_string(out, host);
        fprintf(out, ",\"cwd\":");
        json_string(out, cwd);
        fprintf(out, "}\n");
        fflush(out);
        fclose(out);
        return 3;
    }

    pid = process_info.dwProcessId;
    g_target_pid = pid;
    g_main_thread_id = process_info.dwThreadId;
    start_tick = GetTickCount();
    fprintf(out, "{\"type\":\"start\",\"pid\":%lu,\"timeout_ms\":%lu,\"host\":",
            (unsigned long)pid, (unsigned long)timeout_ms);
    json_string(out, host);
    fprintf(out, ",\"cwd\":");
    json_string(out, cwd);
    fprintf(out, ",\"debug_mode\":\"DEBUG_ONLY_THIS_PROCESS\",\"show_window\":\"%s\",\"windowed_request_enabled\":%s}\n",
            windowed ? "SW_SHOWNORMAL" : "SW_HIDE",
            windowed ? "true" : "false");
    fflush(out);
    log_process_virtualization(out, process_info.hProcess, pid);

    while (!exited) {
        DWORD elapsed = GetTickCount() - start_tick;
        DWORD wait_ms;
        DWORD continue_status = DBG_CONTINUE;

        if (!timed_out && elapsed >= timeout_ms) {
            timed_out = TRUE;
            reason = 1;
        if (windowed)
                poll_for_windowed_menu(out, process_info.hProcess, pid, report_path);
            fprintf(out, "{\"type\":\"timeout\",\"elapsed_ms\":%lu,\"action\":\"TerminateProcess(own PID only)\"}\n",
                    (unsigned long)elapsed);
            fflush(out);
            log_host_state_snapshot(out, process_info.hProcess, pid,
                    g_main_thread_id, start_tick, "pre_terminate");
            snapshot_process_windows(out, pid);
            /* This is the only termination call; DEBUG_ONLY_THIS_PROCESS means
             * processes spawned by the game are outside this debugger. */
            if (!TerminateProcess(process_info.hProcess, 0xC0DE)) {
                last_error = GetLastError();
                fprintf(out, "{\"type\":\"terminate_process_failed\",\"error\":%lu}\n",
                        (unsigned long)last_error);
                fflush(out);
            }
        }

        if (timed_out) {
            DWORD after_timeout = GetTickCount() - start_tick - timeout_ms;
            if (after_timeout >= 5000) {
                reason = 2;
                if (WaitForSingleObject(process_info.hProcess, 0) == WAIT_OBJECT_0)
                    exited = TRUE;
                else
                    break;
            }
            wait_ms = 100;
        } else {
            DWORD remaining = timeout_ms - elapsed;
            wait_ms = remaining > 1000 ? 1000 : remaining;
        }

        memset(&event, 0, sizeof(event));
        if (!WaitForDebugEvent(&event, wait_ms)) {
            DWORD wait_error = GetLastError();
            if (wait_error == ERROR_SEM_TIMEOUT) {
                if (windowed)
                    poll_for_windowed_menu(out, process_info.hProcess, pid, report_path);
                log_host_state_snapshot(out, process_info.hProcess, pid,
                        g_main_thread_id, start_tick, "periodic");
                continue;
            }
            last_error = wait_error;
            fprintf(out, "{\"type\":\"wait_for_debug_event_failed\",\"error\":%lu}\n",
                    (unsigned long)last_error);
            fflush(out);
            reason = 3;
            break;
        }

        switch (event.dwDebugEventCode) {
        case CREATE_PROCESS_DEBUG_EVENT:
            loader_breakpoint_pending = TRUE;
            log_file_record(out, "create_process", event.dwProcessId, event.dwThreadId,
                    event.u.CreateProcessInfo.lpBaseOfImage,
                    event.u.CreateProcessInfo.lpImageName,
                    event.u.CreateProcessInfo.fUnicode,
                    event.u.CreateProcessInfo.hFile, process_info.hProcess);
            if (event.u.CreateProcessInfo.hFile &&
                event.u.CreateProcessInfo.hFile != INVALID_HANDLE_VALUE)
                CloseHandle(event.u.CreateProcessInfo.hFile);
            if (event.u.CreateProcessInfo.hProcess &&
                event.u.CreateProcessInfo.hProcess != process_info.hProcess)
                CloseHandle(event.u.CreateProcessInfo.hProcess);
            if (event.u.CreateProcessInfo.hThread &&
                event.u.CreateProcessInfo.hThread != process_info.hThread)
                CloseHandle(event.u.CreateProcessInfo.hThread);
            break;

        case CREATE_THREAD_DEBUG_EVENT:
            fprintf(out, "{\"type\":\"create_thread\",\"pid\":%lu,\"tid\":%lu,\"start\":\"0x%08lx\"}\n",
                    (unsigned long)event.dwProcessId, (unsigned long)event.dwThreadId,
                    (unsigned long)event.u.CreateThread.lpStartAddress);
            if (event.u.CreateThread.hThread)
                CloseHandle(event.u.CreateThread.hThread);
            break;

        case EXIT_THREAD_DEBUG_EVENT:
            fprintf(out, "{\"type\":\"exit_thread\",\"pid\":%lu,\"tid\":%lu,\"exit_code\":%lu}\n",
                    (unsigned long)event.dwProcessId, (unsigned long)event.dwThreadId,
                    (unsigned long)event.u.ExitThread.dwExitCode);
            break;

        case LOAD_DLL_DEBUG_EVENT:
            {
                char loaded_image_name[1024];
                char *leaf;
                loaded_image_name[0] = 0;
                read_remote_image_name(process_info.hProcess,
                        event.u.LoadDll.lpImageName,
                        event.u.LoadDll.fUnicode,
                        loaded_image_name, sizeof(loaded_image_name));
                leaf = strrchr(loaded_image_name, '\\');
                leaf = leaf ? leaf + 1 : loaded_image_name;
                if (_stricmp(leaf, "TITLE.DLL") == 0) {
                    g_title_base = (DWORD)event.u.LoadDll.lpBaseOfDll;
                    fprintf(out, "{\"type\":\"title_module\",\"pid\":%lu,\"base\":\"0x%08lx\",\"image_name\":",
                            (unsigned long)event.dwProcessId, (unsigned long)g_title_base);
                    json_string(out, loaded_image_name);
                    fprintf(out, "}\n");
                    fflush(out);
                }
            }
            log_file_record(out, "load_dll", event.dwProcessId, event.dwThreadId,
                    event.u.LoadDll.lpBaseOfDll, event.u.LoadDll.lpImageName,
                    event.u.LoadDll.fUnicode, event.u.LoadDll.hFile,
                    process_info.hProcess);
            if (event.u.LoadDll.hFile && event.u.LoadDll.hFile != INVALID_HANDLE_VALUE)
                CloseHandle(event.u.LoadDll.hFile);
            break;

        case UNLOAD_DLL_DEBUG_EVENT:
            if ((DWORD)event.u.UnloadDll.lpBaseOfDll == g_title_base)
                g_title_base = 0;
            fprintf(out, "{\"type\":\"unload_dll\",\"pid\":%lu,\"tid\":%lu,\"base\":\"0x%08lx\"}\n",
                    (unsigned long)event.dwProcessId, (unsigned long)event.dwThreadId,
                    (unsigned long)event.u.UnloadDll.lpBaseOfDll);
            break;

        case EXCEPTION_DEBUG_EVENT: {
            DWORD code = event.u.Exception.ExceptionRecord.ExceptionCode;
            BOOL first_chance = event.u.Exception.dwFirstChance != 0;
            BOOL handled_loader_breakpoint = FALSE;
            if (code == EXCEPTION_BREAKPOINT && first_chance && loader_breakpoint_pending) {
                /* Consume only the first OS loader breakpoint so startup can
                 * proceed. Every later application exception remains visible. */
                loader_breakpoint_pending = FALSE;
                handled_loader_breakpoint = TRUE;
                continue_status = DBG_CONTINUE;
            } else {
                continue_status = DBG_EXCEPTION_NOT_HANDLED;
            }
            fprintf(out, "{\"type\":\"exception\",\"pid\":%lu,\"tid\":%lu,\"code\":\"0x%08lx\",\"name\":\"%s\",\"address\":\"0x%08lx\",\"first_chance\":%s,\"probe_handled_loader_breakpoint\":%s,\"continued_as\":\"%s\"}\n",
                    (unsigned long)event.dwProcessId, (unsigned long)event.dwThreadId,
                    (unsigned long)code, exception_name(code),
                    (unsigned long)event.u.Exception.ExceptionRecord.ExceptionAddress,
                    first_chance ? "true" : "false",
                    handled_loader_breakpoint ? "true" : "false",
                    continue_status == DBG_CONTINUE ? "DBG_CONTINUE" : "DBG_EXCEPTION_NOT_HANDLED");
            fflush(out);
            if (code == EXCEPTION_ACCESS_VIOLATION && first_chance)
                log_exception_context(out, process_info.hProcess,
                        event.dwThreadId, &event.u.Exception);
            break;
        }

        case OUTPUT_DEBUG_STRING_EVENT: {
            char message[1024];
            DWORD read_count = 0;
            DWORD bytes = event.u.DebugString.nDebugStringLength;
            DWORD cap = bytes < (DWORD)(sizeof(message) - 1) ? bytes : (DWORD)(sizeof(message) - 1);
            memset(message, 0, sizeof(message));
            if (cap && ReadProcessMemory(process_info.hProcess,
                    event.u.DebugString.lpDebugStringData, message, cap, &read_count))
                message[read_count < (DWORD)sizeof(message) ? read_count : (DWORD)(sizeof(message) - 1)] = 0;
            fprintf(out, "{\"type\":\"output_debug_string\",\"pid\":%lu,\"tid\":%lu,\"unicode\":%s,\"text\":",
                    (unsigned long)event.dwProcessId, (unsigned long)event.dwThreadId,
                    event.u.DebugString.fUnicode ? "true" : "false");
            json_string(out, message);
            fprintf(out, "}\n");
            break;
        }

        case RIP_EVENT:
            fprintf(out, "{\"type\":\"rip\",\"pid\":%lu,\"tid\":%lu,\"error\":%lu,\"type_code\":%lu}\n",
                    (unsigned long)event.dwProcessId, (unsigned long)event.dwThreadId,
                    (unsigned long)event.u.RipInfo.dwError,
                    (unsigned long)event.u.RipInfo.dwType);
            break;

        case EXIT_PROCESS_DEBUG_EVENT:
            exit_code = event.u.ExitProcess.dwExitCode;
            fprintf(out, "{\"type\":\"exit_process\",\"pid\":%lu,\"exit_code\":%lu}\n",
                    (unsigned long)event.dwProcessId, (unsigned long)exit_code);
            exited = TRUE;
            break;

        default:
            fprintf(out, "{\"type\":\"other_debug_event\",\"code\":%lu,\"pid\":%lu,\"tid\":%lu}\n",
                    (unsigned long)event.dwDebugEventCode,
                    (unsigned long)event.dwProcessId, (unsigned long)event.dwThreadId);
            break;
        }

        if (windowed && !timed_out)
            poll_for_windowed_menu(out, process_info.hProcess, pid, report_path);

        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, continue_status)) {
            last_error = GetLastError();
            fprintf(out, "{\"type\":\"continue_debug_event_failed\",\"error\":%lu}\n",
                    (unsigned long)last_error);
            fflush(out);
            reason = 4;
            break;
        }
    }

    if (WaitForSingleObject(process_info.hProcess, 0) == WAIT_OBJECT_0) {
        if (GetExitCodeProcess(process_info.hProcess, &exit_code))
            exited = TRUE;
    } else if (timed_out) {
        /* Safety bound: do not terminate anything other than this exact PID. */
        TerminateProcess(process_info.hProcess, 0xC0DE);
        WaitForSingleObject(process_info.hProcess, 3000);
        GetExitCodeProcess(process_info.hProcess, &exit_code);
    }

    fprintf(out, "{\"type\":\"summary\",\"pid\":%lu,\"created\":true,\"timed_out\":%s,\"exited\":%s,\"exit_code\":%lu,\"reason_code\":%lu,\"last_error\":%lu}\n",
            (unsigned long)pid, timed_out ? "true" : "false",
            exited ? "true" : "false", (unsigned long)exit_code,
            (unsigned long)reason, (unsigned long)last_error);
    fflush(out);
    fclose(out);
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    return reason == 0 && exited ? 0 : 1;
}

int main(int argc, char **argv)
{
    unsigned long timeout_ms;
    unsigned long pool_rva = 0;
    char *end;
    BOOL windowed = FALSE;
    int arg_index;
    if (argc < 5) {
        fprintf(stderr, "usage: host_debugger_probe.exe <host.exe> <working-directory> <report.jsonl> <timeout-ms> [--windowed|--hidden] [--title-pool-rva hex]\n");
        return 2;
    }
    for (arg_index = 5; arg_index < argc; ++arg_index) {
        if (strcmp(argv[arg_index], "--windowed") == 0) {
            windowed = TRUE;
        } else if (strcmp(argv[arg_index], "--hidden") == 0) {
            windowed = FALSE;
        } else if (strcmp(argv[arg_index], "--title-pool-rva") == 0 &&
                   arg_index + 1 < argc) {
            pool_rva = strtoul(argv[++arg_index], &end, 0);
            if (!argv[arg_index][0] || *end || pool_rva == 0 || pool_rva > 0xffffffff) {
                fprintf(stderr, "title pool RVA must be a nonzero 32-bit integer\n");
                return 2;
            }
        } else {
            fprintf(stderr, "unsupported probe option: %s\n", argv[arg_index]);
            return 2;
        }
    }
    timeout_ms = strtoul(argv[4], &end, 10);
    if (!argv[4][0] || *end || timeout_ms < 1000 || timeout_ms > 600000) {
        fprintf(stderr, "timeout must be between 1000 and 600000 milliseconds\n");
        return 2;
    }
    return launch_debugged(argv[1], argv[2], argv[3], (DWORD)timeout_ms,
            windowed, (DWORD)pool_rva);
}
