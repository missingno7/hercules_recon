/* Non-invasive ENG1 integration probe, based on the repository's TITLE host
 * probe contract. It observes one supplied host under DEBUG_ONLY_THIS_PROCESS;
 * it never writes target memory or patches target code. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOWED_COMMAND 32801
#define REQUESTED_MODE_ADDRESS 0x0096e3d0UL
#define CURRENT_MODE_ADDRESS   0x00ba020cUL

static DWORD g_eng1_base;
static DWORD g_entry_rva;
static DWORD g_interface_rva;
static DWORD g_first_use_rva;
static BOOL g_first_table_logged;
static BOOL g_setup_logged;
static BOOL g_alt_enter_sent;
static BOOL g_size_command_sent;
static BOOL g_window_ready_logged;
static BOOL g_activation_requested;
static HWND g_main_window;
static BOOL g_enter_game;
static BOOL g_return_key_owned;
static DWORD g_return_key_tick, g_next_return_tick, g_probe_start_tick;
static unsigned int g_return_pulses;
typedef struct probe_state_cell {
    char name[64];
    DWORD rva, width;
} PROBE_STATE_CELL;
static PROBE_STATE_CELL g_state_cells[32];
static unsigned int g_state_cell_count;

/* SendInput is newer than the pinned VC5 headers. Resolve the OS API, with its
 * actual 28-byte PE32 input layout. No target memory or game code is changed. */
typedef struct probe_key_input {
    WORD key, scan;
    DWORD flags, time, extra;
} PROBE_KEY_INPUT;
typedef struct probe_input {
    DWORD type;
    union { PROBE_KEY_INPUT key; BYTE largest_input[24]; } value;
} PROBE_INPUT;
typedef UINT (WINAPI *PROBE_SEND_INPUT)(UINT, const PROBE_INPUT *, int);
typedef char probe_input_requires_28_bytes[(sizeof(PROBE_INPUT)==28)?1:-1];

static BOOL send_return(BOOL release)
{
    PROBE_INPUT input;
    PROBE_SEND_INPUT send_input = (PROBE_SEND_INPUT)GetProcAddress(
        GetModuleHandleA("USER32.DLL"), "SendInput");
    if (!send_input) return FALSE;
    memset(&input,0,sizeof(input));
    input.type=1;
    input.value.key.key=VK_RETURN;
    input.value.key.scan=0x1c;
    input.value.key.flags=release?2:0;
    return send_input(1,&input,sizeof(input))==1;
}

static void poll_new_game_input(FILE *out,DWORD pid)
{
    DWORD now=GetTickCount(),foreground_pid=0;
    BOOL result;
    if (g_return_key_owned && (g_eng1_base || now-g_return_key_tick>=200UL)) {
        result=send_return(TRUE);
        if (result) g_return_key_owned=FALSE;
        fprintf(out,"{\"type\":\"new_game_return_up\",\"sent\":%s,\"elapsed_ms\":%lu}\n",
                result?"true":"false",(unsigned long)(now-g_probe_start_tick));
        fflush(out);
        g_next_return_tick=now+3000UL;
        return;
    }
    if (!g_enter_game || g_return_key_owned || g_eng1_base ||
        !g_window_ready_logged || g_return_pulses>=6 ||
        now-g_probe_start_tick<12000UL || (LONG)(now-g_next_return_tick)<0)
        return;
    GetWindowThreadProcessId(GetForegroundWindow(),&foreground_pid);
    if (foreground_pid!=pid || (GetAsyncKeyState(VK_RETURN)&0x8000) ||
        (GetAsyncKeyState(VK_MENU)&0x8000) ||
        (GetAsyncKeyState(VK_CONTROL)&0x8000) ||
        (GetAsyncKeyState(VK_SHIFT)&0x8000)) return;
    result=send_return(FALSE);
    if (result) {
        g_return_key_owned=TRUE;
        g_return_key_tick=now;
        ++g_return_pulses;
    } else g_next_return_tick=now+3000UL;
    fprintf(out,"{\"type\":\"new_game_return_down\",\"sent\":%s,\"ordinal\":%u,\"foreground_pid\":%lu,\"foreground_is_target\":true,\"elapsed_ms\":%lu}\n",
            result?"true":"false",g_return_pulses,(unsigned long)foreground_pid,
            (unsigned long)(now-g_probe_start_tick));
    fflush(out);
}

static void json_string(FILE *out, const char *text)
{
    const unsigned char *p = (const unsigned char *)text;
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

static BOOL read_remote(HANDLE process, DWORD address, void *buffer,
                        DWORD size)
{
    DWORD count = 0;
    return ReadProcessMemory(process, (LPCVOID)address, buffer, size, &count) &&
           count == size;
}

static void image_name(HANDLE process, LPVOID remote_name_slot,
                       BOOL unicode, char *out, DWORD out_size)
{
    DWORD remote_name = 0;
    DWORD count = 0;
    if (!out || !out_size)
        return;
    out[0] = 0;
    if (!remote_name_slot ||
        !ReadProcessMemory(process, remote_name_slot, &remote_name,
                           sizeof(remote_name), &count) ||
        count != sizeof(remote_name) || !remote_name)
        return;
    if (unicode) {
        WCHAR wide[1024];
        memset(wide, 0, sizeof(wide));
        if (ReadProcessMemory(process, (LPCVOID)remote_name, wide,
                              sizeof(wide) - sizeof(WCHAR), &count) && count) {
            wide[count / sizeof(WCHAR)] = 0;
            WideCharToMultiByte(CP_ACP, 0, wide, -1, out, (int)out_size,
                                0, 0);
            out[out_size - 1] = 0;
        }
    } else {
        if (ReadProcessMemory(process, (LPCVOID)remote_name, out,
                              out_size - 1, &count) && count < out_size)
            out[count] = 0;
    }
}

static void log_table(FILE *out, HANDLE process, const char *phase)
{
    DWORD words[250];
    DWORD first_use = 0xffffffffUL;
    DWORD signature;
    BOOL table_ok, first_ok;
    unsigned int i;
    BOOL callback_seen;
    if (!g_eng1_base)
        return;
    table_ok = read_remote(process, g_eng1_base + g_interface_rva,
                           words, sizeof(words));
    first_ok = read_remote(process, g_eng1_base + g_first_use_rva,
                           &first_use, sizeof(first_use));
    if (!table_ok)
        return;
    signature = words[0];
    callback_seen = words[0x164 / 4] != 0 || words[0x258 / 4] != 0 ||
                    words[0x144 / 4] != 0 || words[0x3e0 / 4] != 0;
    if (!strcmp(phase, "first_post_load")) {
        if (g_first_table_logged)
            return;
        g_first_table_logged = TRUE;
    } else if (!strcmp(phase, "setup_observed")) {
        if (g_setup_logged || signature != 0x5ac00cacUL || !first_ok ||
            first_use != 0 || !callback_seen)
            return;
        g_setup_logged = TRUE;
    }
    fprintf(out, "{\"type\":\"eng1_interface_snapshot\",\"phase\":");
    json_string(out, phase);
    fprintf(out, ",\"module_base\":\"0x%08lx\",\"table_rva\":\"0x%08lx\",\"table_read\":true,\"first_use_read\":%s,\"first_use\":\"0x%08lx\",\"signature\":\"0x%08lx\",\"callback_slots_nonzero\":%s,\"interface_words\":[",
            (unsigned long)g_eng1_base, (unsigned long)g_interface_rva,
            first_ok ? "true" : "false", (unsigned long)first_use,
            (unsigned long)signature, callback_seen ? "true" : "false");
    for (i = 0; i != 250; ++i) {
        if (i)
            fputc(',', out);
        fprintf(out, "\"0x%08lx\"", (unsigned long)words[i]);
    }
    fprintf(out, "]}\n");
    fflush(out);
}

static BOOL find_menu_command(HMENU menu, UINT command)
{
    int i, count;
    if (!menu)
        return FALSE;
    count = GetMenuItemCount(menu);
    for (i = 0; i < count; ++i) {
        HMENU child = GetSubMenu(menu, i);
        if (GetMenuItemID(menu, i) == command ||
            find_menu_command(child, command))
            return TRUE;
    }
    return FALSE;
}

typedef struct window_search {
    DWORD pid;
    HWND found;
} WINDOW_SEARCH;

static BOOL CALLBACK find_window(HWND window, LPARAM value)
{
    WINDOW_SEARCH *search = (WINDOW_SEARCH *)value;
    DWORD pid = 0;
    if (GetWindowThreadProcessId(window, &pid) && pid == search->pid &&
        IsWindowVisible(window)) {
        search->found = window;
        return FALSE;
    }
    return TRUE;
}

static void poll_window_mode(FILE *out, HANDLE process, DWORD pid,
                             BOOL target_running)
{
    WINDOW_SEARCH search;
    BYTE requested = 0xff, current = 0xff;
    BOOL requested_ok, current_ok, menu_present;
    DWORD style = 0;
    RECT client;
    char title[512];
    static BYTE last_requested = 0xfe, last_current = 0xfe;
    static LONG last_width = -1, last_height = -1;
    static DWORD last_style = 0xffffffffUL;
    static BOOL last_menu = FALSE;
    BOOL visible = FALSE, caption = FALSE;
    LONG width = -1, height = -1;
    BOOL changed;
    DWORD foreground_pid = 0;
    memset(&search, 0, sizeof(search));
    search.pid = pid;
    EnumWindows(find_window, (LPARAM)&search);
    if (search.found)
        g_main_window = search.found;
    requested_ok = read_remote(process, REQUESTED_MODE_ADDRESS,
                               &requested, sizeof(requested));
    current_ok = read_remote(process, CURRENT_MODE_ADDRESS,
                             &current, sizeof(current));
    menu_present = g_main_window && IsWindow(g_main_window) &&
                   find_menu_command(GetMenu(g_main_window), WINDOWED_COMMAND);
    title[0] = 0;
    if (g_main_window && IsWindow(g_main_window)) {
        style = (DWORD)GetWindowLongA(g_main_window, GWL_STYLE);
        visible = IsWindowVisible(g_main_window);
        caption = (style & WS_CAPTION) != 0 && (style & WS_POPUP) == 0;
        GetWindowTextA(g_main_window, title, sizeof(title));
        if (GetClientRect(g_main_window, &client)) {
            width = client.right - client.left;
            height = client.bottom - client.top;
        }
    }
    changed = requested != last_requested || current != last_current ||
              width != last_width || height != last_height ||
              style != last_style || menu_present != last_menu;
    if (changed) {
        fprintf(out, "{\"type\":\"host_window_state\",\"hwnd\":\"0x%08lx\",\"visible\":%s,\"title\":",
                (unsigned long)g_main_window, visible ? "true" : "false");
        json_string(out, title);
        fprintf(out, ",\"requested_read\":%s,\"requested_fullscreen\":%u,\"current_read\":%s,\"current_mode\":%u,\"caption_style\":%s,\"windowed_menu_32801\":%s,\"style\":\"0x%08lx\",\"client_width\":%ld,\"client_height\":%ld}\n",
                requested_ok ? "true" : "false", (unsigned int)requested,
                current_ok ? "true" : "false", (unsigned int)current,
                caption ? "true" : "false", menu_present ? "true" : "false",
                (unsigned long)style, width, height);
        fflush(out);
        last_requested = requested;
        last_current = current;
        last_width = width;
        last_height = height;
        last_style = style;
        last_menu = menu_present;
    }
    /* Fullscreen removes the menu: it cannot be a prerequisite for leaving it.
     * The enumerated HWND is already restricted to this debuggee's PID. */
    if (requested_ok && current_ok && requested == 1 && current == 1 &&
        g_main_window && !g_alt_enter_sent) {
        BOOL down_ok, up_ok;
        DWORD down_error, up_error;
        SetLastError(0);
        down_ok = PostMessageA(g_main_window, WM_SYSKEYDOWN, VK_RETURN,
                               (LPARAM)0x201c0001L);
        down_error = down_ok ? 0 : GetLastError();
        SetLastError(0);
        up_ok = PostMessageA(g_main_window, WM_SYSKEYUP, VK_RETURN,
                             (LPARAM)0xe01c0001L);
        up_error = up_ok ? 0 : GetLastError();
        g_alt_enter_sent = TRUE;
        fprintf(out, "{\"type\":\"native_alt_enter_request\",\"requested_fullscreen\":1,\"current_mode\":1,\"syskeydown\":\"0x201c0001\",\"syskeyup\":\"0xe01c0001\",\"keydown_posted\":%s,\"keydown_error\":%lu,\"keyup_posted\":%s,\"keyup_error\":%lu}\n",
                down_ok ? "true" : "false", (unsigned long)down_error,
                up_ok ? "true" : "false", (unsigned long)up_error);
        fflush(out);
    }
    if (!(requested_ok && current_ok && requested == 0 && current == 0 &&
          visible && caption && menu_present && width == 320 && height == 240))
        g_window_ready_logged = FALSE;
    if (requested_ok && current_ok && requested == 0 && current == 0 &&
        visible && caption && menu_present && g_main_window) {
        if (width != 320 || height != 240) {
            if (!g_size_command_sent) {
                BOOL posted;
                DWORD error;
                SetLastError(0);
                posted = PostMessageA(g_main_window, WM_COMMAND,
                                      WINDOWED_COMMAND, 0);
                error = posted ? 0 : GetLastError();
                g_size_command_sent = TRUE;
                fprintf(out, "{\"type\":\"windowed_size_request\",\"command\":32801,\"post_succeeded\":%s,\"error\":%lu,\"mode_fields_confirmed\":true,\"caption_confirmed\":true,\"menu_confirmed\":true}\n",
                        posted ? "true" : "false", (unsigned long)error);
                fflush(out);
            }
        } else if (!g_window_ready_logged) {
            fprintf(out, "{\"type\":\"windowed_mode_ready\",\"client_width\":320,\"client_height\":240,\"requested_fullscreen\":0,\"current_mode\":0,\"caption_style\":true,\"menu_32801\":true,\"gameplay_input_sent\":false}\n");
            fflush(out);
            g_window_ready_logged = TRUE;
            g_alt_enter_sent = FALSE;
            g_size_command_sent = FALSE;
        }
    }
    /* Hercules pauses when inactive. Request activation only while the target
     * runs, never while a debug event suspends its window thread. No input or
     * target-memory write is used. The actual foreground owner is recorded. */
    if (target_running && g_window_ready_logged && !g_activation_requested) {
        BOOL activated;
        GetWindowThreadProcessId(GetForegroundWindow(), &foreground_pid);
        activated = SetForegroundWindow(g_main_window);
        GetWindowThreadProcessId(GetForegroundWindow(), &foreground_pid);
        g_activation_requested = TRUE;
        fprintf(out, "{\"type\":\"host_activation_request\",\"set_foreground_return\":%s,\"foreground_pid\":%lu,\"foreground_is_target\":%s}\n",
                activated ? "true" : "false", (unsigned long)foreground_pid,
                foreground_pid == pid ? "true" : "false");
        fflush(out);
    }
}

static void log_exception_context(FILE *out,HANDLE process,DWORD tid)
{
    typedef HANDLE (WINAPI *PROBE_OPEN_THREAD)(DWORD,BOOL,DWORD);
    PROBE_OPEN_THREAD open_thread=(PROBE_OPEN_THREAD)GetProcAddress(
        GetModuleHandleA("KERNEL32.DLL"),"OpenThread");
    HANDLE thread=open_thread?open_thread(0x0008UL,FALSE,tid):0;
    CONTEXT context;
    DWORD stack[16],read=0;
    unsigned int i;
    BOOL got=FALSE;
    memset(&context,0,sizeof(context));
    context.ContextFlags=CONTEXT_FULL;
    if(thread)got=GetThreadContext(thread,&context);
    fprintf(out,"{\"type\":\"exception_context\",\"tid\":%lu,\"context_read\":%s",
        (unsigned long)tid,got?"true":"false");
    if(got) {
        fprintf(out,",\"eip\":\"0x%08lx\",\"esp\":\"0x%08lx\",\"ebp\":\"0x%08lx\",\"eax\":\"0x%08lx\",\"ebx\":\"0x%08lx\",\"ecx\":\"0x%08lx\",\"edx\":\"0x%08lx\",\"esi\":\"0x%08lx\",\"edi\":\"0x%08lx\",\"eflags\":\"0x%08lx\"",
            context.Eip,context.Esp,context.Ebp,context.Eax,context.Ebx,
            context.Ecx,context.Edx,context.Esi,context.Edi,context.EFlags);
        ReadProcessMemory(process,(LPCVOID)context.Esp,stack,sizeof(stack),&read);
        fprintf(out,",\"stack_words\":[");
        for(i=0;i<read/4;++i)fprintf(out,"%s\"0x%08lx\"",i?",":"",stack[i]);
        fprintf(out,"]");
    }
    fprintf(out,"}\n");
    if(thread)CloseHandle(thread);
    if(g_eng1_base)for(i=0;i<g_state_cell_count;++i) {
        DWORD value=0;
        BOOL ok=ReadProcessMemory(process,(LPCVOID)(g_eng1_base+g_state_cells[i].rva),
                                 &value,g_state_cells[i].width,&read);
        fprintf(out,"{\"type\":\"exception_source_state\",\"name\":");
        json_string(out,g_state_cells[i].name);
        fprintf(out,",\"linked_rva\":\"0x%08lx\",\"width\":%lu,\"read\":%s,\"value\":\"0x%08lx\"}\n",
            g_state_cells[i].rva,g_state_cells[i].width,
            ok&&read==g_state_cells[i].width?"true":"false",value);
    }
}

static const char *exception_name(DWORD code)
{
    switch (code) {
    case 0xe0484731UL: return "explicit_incomplete_behavior";
    case EXCEPTION_ACCESS_VIOLATION: return "access_violation";
    case EXCEPTION_BREAKPOINT: return "breakpoint";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "datatype_misalignment";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal_instruction";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer_divide_by_zero";
    case EXCEPTION_PRIV_INSTRUCTION: return "privileged_instruction";
    case EXCEPTION_STACK_OVERFLOW: return "stack_overflow";
    default: return "other";
    }
}

static int launch_debugged(const char *host, const char *cwd,
                           const char *events_path, DWORD timeout_ms)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION info;
    DEBUG_EVENT event;
    FILE *out;
    char command[4096];
    DWORD start_tick, exit_code = STILL_ACTIVE, last_error = 0;
    DWORD pid = 0, reason = 0;
    BOOL created, loader_breakpoint_pending = FALSE;
    BOOL timed_out = FALSE, exited = FALSE;

    out = fopen(events_path, "wb");
    if (!out) {
        fprintf(stderr, "cannot create event log: %s\n", events_path);
        return 2;
    }
    memset(&startup, 0, sizeof(startup));
    memset(&info, 0, sizeof(info));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    if (strlen(host) + 3 >= sizeof(command)) {
        fclose(out);
        return 2;
    }
    sprintf(command, "\"%s\"", host);
    created = CreateProcessA(host, command, 0, 0, FALSE,
                             DEBUG_ONLY_THIS_PROCESS, 0, cwd,
                             &startup, &info);
    if (!created) {
        last_error = GetLastError();
        fprintf(out, "{\"type\":\"create_process_failed\",\"error\":%lu}\n",
                (unsigned long)last_error);
        fclose(out);
        return 3;
    }
    pid = info.dwProcessId;
    start_tick = GetTickCount();
    g_probe_start_tick=start_tick;
    g_next_return_tick=start_tick+12000UL;
    fprintf(out, "{\"type\":\"start\",\"pid\":%lu,\"timeout_ms\":%lu,\"debug_mode\":\"DEBUG_ONLY_THIS_PROCESS\",\"windowed_requested\":true}\n",
            (unsigned long)pid, (unsigned long)timeout_ms);
    fflush(out);

    while (!exited) {
        DWORD elapsed = GetTickCount() - start_tick;
        DWORD wait_ms = 100;
        DWORD continue_status = DBG_CONTINUE;
        if (!timed_out && elapsed >= timeout_ms) {
            timed_out = TRUE;
            reason = 1;
            fprintf(out, "{\"type\":\"timeout\",\"elapsed_ms\":%lu,\"action\":\"TerminateProcess(own_PID_only)\"}\n",
                    (unsigned long)elapsed);
            fflush(out);
            if (!TerminateProcess(info.hProcess, 0xC0deUL)) {
                last_error = GetLastError();
                fprintf(out, "{\"type\":\"terminate_process_failed\",\"error\":%lu}\n",
                        (unsigned long)last_error);
                fflush(out);
            }
        }
        if (timed_out && elapsed > timeout_ms + 5000UL) {
            reason = 2;
            break;
        }
        if (!WaitForDebugEvent(&event, wait_ms)) {
            last_error = GetLastError();
            if (last_error == ERROR_SEM_TIMEOUT) {
                if (!timed_out) {
                    poll_window_mode(out, info.hProcess, pid, TRUE);
                    poll_new_game_input(out,pid);
                    if (g_eng1_base && !g_first_table_logged)
                        log_table(out, info.hProcess, "first_post_load");
                    else if (g_eng1_base && !g_setup_logged)
                        log_table(out, info.hProcess, "setup_observed");
                }
                continue;
            }
            reason = 3;
            fprintf(out, "{\"type\":\"wait_for_debug_event_failed\",\"error\":%lu}\n",
                    (unsigned long)last_error);
            fflush(out);
            break;
        }
        switch (event.dwDebugEventCode) {
        case CREATE_PROCESS_DEBUG_EVENT: {
            char name[1024];
            image_name(info.hProcess, event.u.CreateProcessInfo.lpImageName,
                       event.u.CreateProcessInfo.fUnicode, name, sizeof(name));
            loader_breakpoint_pending = TRUE;
            fprintf(out, "{\"type\":\"create_process\",\"pid\":%lu,\"base\":\"0x%08lx\",\"image_name\":",
                    (unsigned long)event.dwProcessId,
                    (unsigned long)event.u.CreateProcessInfo.lpBaseOfImage);
            json_string(out, name);
            fprintf(out, "}\n");
            if (event.u.CreateProcessInfo.hFile &&
                event.u.CreateProcessInfo.hFile != INVALID_HANDLE_VALUE)
                CloseHandle(event.u.CreateProcessInfo.hFile);
            if (event.u.CreateProcessInfo.hProcess &&
                event.u.CreateProcessInfo.hProcess != info.hProcess)
                CloseHandle(event.u.CreateProcessInfo.hProcess);
            if (event.u.CreateProcessInfo.hThread &&
                event.u.CreateProcessInfo.hThread != info.hThread)
                CloseHandle(event.u.CreateProcessInfo.hThread);
            break;
        }
        case LOAD_DLL_DEBUG_EVENT: {
            char name[1024], leaf[1024];
            char *slash;
            DWORD base = (DWORD)event.u.LoadDll.lpBaseOfDll;
            image_name(info.hProcess, event.u.LoadDll.lpImageName,
                       event.u.LoadDll.fUnicode, name, sizeof(name));
            lstrcpynA(leaf, name, sizeof(leaf));
            slash = strrchr(leaf, '\\');
            if (!slash)
                slash = strrchr(leaf, '/');
            slash = slash ? slash + 1 : leaf;
            if (_stricmp(slash, "ENG1.DLL") == 0) {
                g_eng1_base = base;
                fprintf(out, "{\"type\":\"eng1_module_load\",\"pid\":%lu,\"module_base\":\"0x%08lx\",\"image_name\":",
                        (unsigned long)pid, (unsigned long)base);
                json_string(out, name);
                fprintf(out, ",\"linked_entry_rva\":\"0x%08lx\",\"linked_entry_address\":\"0x%08lx\",\"interface_rva\":\"0x%08lx\",\"first_use_rva\":\"0x%08lx\"}\n",
                        (unsigned long)g_entry_rva,
                        (unsigned long)(base + g_entry_rva),
                        (unsigned long)g_interface_rva,
                        (unsigned long)g_first_use_rva);
            } else {
                fprintf(out, "{\"type\":\"load_dll\",\"pid\":%lu,\"module_base\":\"0x%08lx\",\"image_name\":",
                        (unsigned long)pid, (unsigned long)base);
                json_string(out, name);
                fprintf(out, "}\n");
            }
            fflush(out);
            if (event.u.LoadDll.hFile &&
                event.u.LoadDll.hFile != INVALID_HANDLE_VALUE)
                CloseHandle(event.u.LoadDll.hFile);
            break;
        }
        case UNLOAD_DLL_DEBUG_EVENT:
            fprintf(out, "{\"type\":\"unload_dll\",\"pid\":%lu,\"module_base\":\"0x%08lx\"}\n",
                    (unsigned long)pid,
                    (unsigned long)event.u.UnloadDll.lpBaseOfDll);
            if ((DWORD)event.u.UnloadDll.lpBaseOfDll == g_eng1_base)
                g_eng1_base = 0;
            break;
        case EXCEPTION_DEBUG_EVENT: {
            DWORD code = event.u.Exception.ExceptionRecord.ExceptionCode;
            BOOL first = event.u.Exception.dwFirstChance != 0;
            BOOL loader_breakpoint = code == EXCEPTION_BREAKPOINT && first &&
                                     loader_breakpoint_pending;
            if (loader_breakpoint) {
                loader_breakpoint_pending = FALSE;
                continue_status = DBG_CONTINUE;
            } else
                continue_status = DBG_EXCEPTION_NOT_HANDLED;
            fprintf(out, "{\"type\":\"exception\",\"pid\":%lu,\"tid\":%lu,\"code\":\"0x%08lx\",\"name\":",
                    (unsigned long)event.dwProcessId,
                    (unsigned long)event.dwThreadId, (unsigned long)code);
            json_string(out, exception_name(code));
            fprintf(out, ",\"address\":\"0x%08lx\",\"first_chance\":%s,\"loader_breakpoint\":%s,\"continued_as\":\"%s\"}\n",
                    (unsigned long)event.u.Exception.ExceptionRecord.ExceptionAddress,
                    first ? "true" : "false",
                    loader_breakpoint ? "true" : "false",
                    continue_status == DBG_CONTINUE ? "DBG_CONTINUE" :
                                                     "DBG_EXCEPTION_NOT_HANDLED");
            if (!loader_breakpoint) {
                DWORD i;
                fprintf(out,"{\"type\":\"exception_parameters\",\"values\":[");
                for(i=0;i<event.u.Exception.ExceptionRecord.NumberParameters;++i)
                    fprintf(out,"%s\"0x%08lx\"",i?",":"",event.u.Exception.ExceptionRecord.ExceptionInformation[i]);
                fprintf(out,"]}\n");
                log_exception_context(out,info.hProcess,event.dwThreadId);
            }
            if (code==0xe0484731UL &&
                event.u.Exception.ExceptionRecord.NumberParameters>=2) {
                char why[281];
                DWORD count=0;
                memset(why,0,sizeof(why));
                ReadProcessMemory(info.hProcess,(LPCVOID)event.u.Exception.ExceptionRecord.ExceptionInformation[1],why,sizeof(why)-1,&count);
                fprintf(out,"{\"type\":\"explicit_incomplete_entry\",\"original_rva\":\"0x%lx\",\"reason\":",
                        (unsigned long)event.u.Exception.ExceptionRecord.ExceptionInformation[0]);
                json_string(out,why);
                fprintf(out,"}\n");
                if (!g_setup_logged)
                    log_table(out,info.hProcess,"setup_observed");
                /* The host catches exceptions in a modal error dialog. Stop
                 * this owned test after recording the intentional boundary. */
                reason=6;
                TerminateProcess(info.hProcess,0xe0484731UL);
                fflush(out);
            }
            if(code==EXCEPTION_ACCESS_VIOLATION) {
                /* Capture the first real invalid access before the host turns
                 * it into a modal dialog; end only this owned finite test. */
                reason=7;
                TerminateProcess(info.hProcess,code);
                fflush(out);
            }
            break;
        }
        case OUTPUT_DEBUG_STRING_EVENT: {
            char message[1024];
            DWORD bytes = event.u.DebugString.nDebugStringLength;
            DWORD count = 0;
            if (bytes >= sizeof(message))
                bytes = sizeof(message) - 1;
            memset(message, 0, sizeof(message));
            if (bytes)
                ReadProcessMemory(info.hProcess,
                    event.u.DebugString.lpDebugStringData, message, bytes, &count);
            message[count < sizeof(message) ? count : sizeof(message) - 1] = 0;
            fprintf(out, "{\"type\":\"output_debug_string\",\"pid\":%lu,\"text\":",
                    (unsigned long)pid);
            json_string(out, message);
            fprintf(out, "}\n");
            break;
        }
        case EXIT_PROCESS_DEBUG_EVENT:
            exit_code = event.u.ExitProcess.dwExitCode;
            fprintf(out, "{\"type\":\"exit_process\",\"pid\":%lu,\"exit_code\":%lu}\n",
                    (unsigned long)pid, (unsigned long)exit_code);
            exited = TRUE;
            break;
        case EXIT_THREAD_DEBUG_EVENT:
            fprintf(out, "{\"type\":\"exit_thread\",\"pid\":%lu,\"tid\":%lu,\"exit_code\":%lu}\n",
                    (unsigned long)event.dwProcessId,
                    (unsigned long)event.dwThreadId,
                    (unsigned long)event.u.ExitThread.dwExitCode);
            break;
        case CREATE_THREAD_DEBUG_EVENT:
            if (event.u.CreateThread.hThread)
                CloseHandle(event.u.CreateThread.hThread);
            break;
        default:
            fprintf(out, "{\"type\":\"other_debug_event\",\"code\":%lu}\n",
                    (unsigned long)event.dwDebugEventCode);
            break;
        }
        if (!timed_out && !exited) {
            poll_window_mode(out, info.hProcess, pid, FALSE);
            if (g_eng1_base && !g_first_table_logged)
                log_table(out, info.hProcess, "first_post_load");
            else if (g_eng1_base && !g_setup_logged)
                log_table(out, info.hProcess, "setup_observed");
        }
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId,
                                continue_status)) {
            last_error = GetLastError();
            reason = 4;
            fprintf(out, "{\"type\":\"continue_debug_event_failed\",\"error\":%lu}\n",
                    (unsigned long)last_error);
            fflush(out);
            break;
        }
        if (!timed_out && !exited)
            poll_new_game_input(out,pid);
    }
    if (g_return_key_owned) {
        BOOL released=send_return(TRUE);
        fprintf(out,"{\"type\":\"new_game_return_cleanup\",\"sent\":%s}\n",released?"true":"false");
        g_return_key_owned=FALSE;
    }
    if (g_eng1_base)
        log_table(out, info.hProcess, "shutdown");
    if (!exited && WaitForSingleObject(info.hProcess, 0) != WAIT_OBJECT_0) {
        TerminateProcess(info.hProcess, 0xC0dfUL);
        WaitForSingleObject(info.hProcess, 1000);
        if (!reason)
            reason = 5;
    }
    fprintf(out, "{\"type\":\"summary\",\"pid\":%lu,\"exit_code\":%lu,\"exited\":%s,\"timed_out\":%s,\"windowed_mode_ready\":%s,\"eng1_setup_observed\":%s,\"reason\":%lu}\n",
            (unsigned long)pid, (unsigned long)exit_code,
            exited ? "true" : "false", timed_out ? "true" : "false",
            g_window_ready_logged ? "true" : "false",
            g_setup_logged ? "true" : "false", (unsigned long)reason);
    fflush(out);
    fclose(out);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);
    return reason ? 1 : 0;
}

int main(int argc, char **argv)
{
    DWORD timeout_ms;
    if (argc < 8 || argc > 10) {
        fprintf(stderr, "usage: eng1_host_probe.exe HOST RUNTIME EVENTS TIMEOUT_MS ENTRY_RVA INTERFACE_RVA FIRST_USE_RVA\n");
        return 2;
    }
    timeout_ms = strtoul(argv[4], 0, 0);
    g_enter_game=argc>=9 && strcmp(argv[8],"enter-game")==0;
    if(argc==10) {
        FILE *states=fopen(argv[9],"r");
        if(!states)return 2;
        while(g_state_cell_count<32) {
            PROBE_STATE_CELL *cell=&g_state_cells[g_state_cell_count];
            if(fscanf(states,"%63s %lx %lu",cell->name,&cell->rva,&cell->width)!=3)break;
            if(cell->width!=1 && cell->width!=2 && cell->width!=4) {
                fclose(states);return 2;
            }
            ++g_state_cell_count;
        }
        fclose(states);
    }
    g_entry_rva = strtoul(argv[5], 0, 0);
    g_interface_rva = strtoul(argv[6], 0, 0);
    g_first_use_rva = strtoul(argv[7], 0, 0);
    if (!timeout_ms || timeout_ms > 120000UL) {
        fprintf(stderr, "timeout must be in 1..120000 ms\n");
        return 2;
    }
    return launch_debugged(argv[1], argv[2], argv[3], timeout_ms);
}
