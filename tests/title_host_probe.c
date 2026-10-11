/*
 * 32-bit Win32 debug-event sampler for one disposable Hercules process.
 * The probe launches only its supplied host with DEBUG_ONLY_THIS_PROCESS,
 * hides its startup window, and never changes the debuggee's memory or code.
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

static int launch_debugged(const char *host, const char *cwd,
                           const char *report_path, DWORD timeout_ms)
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
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
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
    start_tick = GetTickCount();
    fprintf(out, "{\"type\":\"start\",\"pid\":%lu,\"timeout_ms\":%lu,\"host\":",
            (unsigned long)pid, (unsigned long)timeout_ms);
    json_string(out, host);
    fprintf(out, ",\"cwd\":");
    json_string(out, cwd);
    fprintf(out, ",\"debug_mode\":\"DEBUG_ONLY_THIS_PROCESS\",\"show_window\":\"SW_HIDE\"}\n");
    fflush(out);

    while (!exited) {
        DWORD elapsed = GetTickCount() - start_tick;
        DWORD wait_ms;
        DWORD continue_status = DBG_CONTINUE;

        if (!timed_out && elapsed >= timeout_ms) {
            timed_out = TRUE;
            reason = 1;
            fprintf(out, "{\"type\":\"timeout\",\"elapsed_ms\":%lu,\"action\":\"TerminateProcess(own PID only)\"}\n",
                    (unsigned long)elapsed);
            fflush(out);
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
            if (wait_error == ERROR_SEM_TIMEOUT)
                continue;
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
            log_file_record(out, "load_dll", event.dwProcessId, event.dwThreadId,
                    event.u.LoadDll.lpBaseOfDll, event.u.LoadDll.lpImageName,
                    event.u.LoadDll.fUnicode, event.u.LoadDll.hFile,
                    process_info.hProcess);
            if (event.u.LoadDll.hFile && event.u.LoadDll.hFile != INVALID_HANDLE_VALUE)
                CloseHandle(event.u.LoadDll.hFile);
            break;

        case UNLOAD_DLL_DEBUG_EVENT:
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
    char *end;
    if (argc != 5) {
        fprintf(stderr, "usage: host_debugger_probe.exe <host.exe> <working-directory> <report.jsonl> <timeout-ms>\n");
        return 2;
    }
    timeout_ms = strtoul(argv[4], &end, 10);
    if (!argv[4][0] || *end || timeout_ms < 1000 || timeout_ms > 600000) {
        fprintf(stderr, "timeout must be between 1000 and 600000 milliseconds\n");
        return 2;
    }
    return launch_debugged(argv[1], argv[2], argv[3], (DWORD)timeout_ms);
}
