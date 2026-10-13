#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hercules_host_abi.h"
#include "probe_diag.h"
#include "probe_layout.h"

#define MAX_PATH_TEXT 2048
#define DEBUG_TIMEOUT_MS 20000UL
#define CHILD_EXIT_CODE 0x50424531UL
#define PREENTRY_ERROR_SEM_TIMEOUT 121UL
#define PREENTRY_INVALID_SET_FILE_POINTER ((DWORD)0xFFFFFFFFUL)

static DWORD g_process_id;
static DWORD g_thread_id;
static HANDLE g_process;
static HANDLE g_thread;
static int g_pending_event;
static int g_seen_create_process;
static int g_resumed_initial_thread;
static int g_initial_breakpoint_count;
static int g_entry_stopped;
static DWORD g_entry_eip;
static DWORD g_entry_exception_address;
static DWORD g_entry_dr0;
static DWORD g_entry_dr6;
static DWORD g_entry_dr7;
static int g_hardware_breakpoint_confirmed;
static int g_failure_stop_mode;
static int g_kill_on_exit_mode;
static int g_exit_process_event_drained;

typedef BOOL (WINAPI *DebugSetProcessKillOnExitFn)(BOOL kill_on_exit);

static int enable_kill_on_debugger_exit(void)
{
    HMODULE kernel32 = GetModuleHandleA("KERNEL32.DLL");
    FARPROC address = kernel32 == 0 ? 0 :
        GetProcAddress(kernel32, "DebugSetProcessKillOnExit");
    if (address != 0) {
        DebugSetProcessKillOnExitFn set_kill_on_exit =
            (DebugSetProcessKillOnExitFn)address;
        g_kill_on_exit_mode = set_kill_on_exit(TRUE) ? 1 : 2;
        return 1;
    }
    /* Windows' debugger default is to terminate debuggees on debugger exit. */
    g_kill_on_exit_mode = 2;
    return 1;
}

static void close_event_handles(const DEBUG_EVENT *event)
{
    if (event->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
        if (event->u.CreateProcessInfo.hFile != 0)
            CloseHandle(event->u.CreateProcessInfo.hFile);
    } else if (event->dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
        if (event->u.LoadDll.hFile != 0)
            CloseHandle(event->u.LoadDll.hFile);
    } else if (event->dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
        if (event->u.CreateThread.hThread != 0)
            CloseHandle(event->u.CreateThread.hThread);
    }
}

static int read_remote(HANDLE process, DWORD address, void *buffer,
                       DWORD length)
{
    DWORD read_count = 0;
    return ReadProcessMemory(process, (LPCVOID)address, buffer, length,
                             &read_count) && read_count == length;
}

static int read_file_at(HANDLE file, DWORD offset, void *buffer, DWORD length)
{
    DWORD read_count = 0;
    SetLastError(NO_ERROR);
    if (SetFilePointer(file, (LONG)offset, 0, FILE_BEGIN) == PREENTRY_INVALID_SET_FILE_POINTER &&
        GetLastError() != NO_ERROR)
        return 0;
    return ReadFile(file, buffer, length, &read_count, 0) &&
           read_count == length;
}

static int full_path(const char *path, char *out, DWORD capacity)
{
    DWORD size = GetFullPathNameA(path, capacity, out, 0);
    return size != 0 && size < capacity;
}

static int find_sibling_ddraw(DWORD pid, const char *exe_path,
                              char *module_path, DWORD capacity,
                              DWORD *module_base, DWORD *module_size)
{
    HANDLE snapshot;
    MODULEENTRY32 entry;
    char expected[MAX_PATH_TEXT];
    int found = 0;
    char *slash;

    if (!full_path(exe_path, expected, sizeof(expected)))
        return 0;
    slash = strrchr(expected, '\\');
    if (slash == 0)
        return 0;
    slash[1] = '\0';
    if (strlen(expected) + strlen("ddraw.dll") >= sizeof(expected))
        return 0;
    strcat(expected, "ddraw.dll");

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;
    memset(&entry, 0, sizeof(entry));
    entry.dwSize = sizeof(entry);
    if (Module32First(snapshot, &entry)) {
        do {
            const char *name = strrchr(entry.szModule, '\\');
            name = name == 0 ? entry.szModule : name + 1;
            if (_stricmp(name, "ddraw.dll") == 0) {
                if (full_path(entry.szExePath, module_path, capacity)) {
                    *module_base = (DWORD)entry.modBaseAddr;
                    *module_size = entry.modBaseSize;
                    found = _stricmp(module_path, expected) == 0;
                }
                break;
            }
        } while (Module32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}

static int compare_host_text(HANDLE process, const char *exe_path,
                             DWORD image_base)
{
    HANDLE file;
    BYTE *file_bytes;
    BYTE *memory_bytes;
    int equal = 0;

    file = CreateFileA(exe_path, GENERIC_READ, FILE_SHARE_READ, 0,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return 0;
    file_bytes = (BYTE *)malloc(HOST_TEXT_RAW_SIZE);
    memory_bytes = (BYTE *)malloc(HOST_TEXT_RAW_SIZE);
    if (file_bytes != 0 && memory_bytes != 0 &&
        read_file_at(file, HOST_TEXT_RAW_OFFSET, file_bytes,
                     HOST_TEXT_RAW_SIZE) &&
        read_remote(process, image_base + HOST_TEXT_RVA, memory_bytes,
                    HOST_TEXT_RAW_SIZE))
        equal = memcmp(file_bytes, memory_bytes, HOST_TEXT_RAW_SIZE) == 0;
    if (file_bytes != 0)
        free(file_bytes);
    if (memory_bytes != 0)
        free(memory_bytes);
    CloseHandle(file);
    return equal;
}

static int compare_original_table(HANDLE process, const char *exe_path,
                                  DWORD image_base, DWORD *source_words)
{
    HANDLE file;
    DWORD file_words[ENGINE_TABLE_WORD_COUNT];
    int result = 0;
    file = CreateFileA(exe_path, GENERIC_READ, FILE_SHARE_READ, 0,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return 0;
    if (read_file_at(file, HOST_TABLE_RAW_OFFSET, file_words,
                     sizeof(file_words)) &&
        read_remote(process, image_base + HOST_TABLE_RVA, source_words,
                    sizeof(file_words)))
        result = memcmp(file_words, source_words, sizeof(file_words)) == 0;
    CloseHandle(file);
    return result;
}

static int inspect_host_at_entry(const char *exe_path, DWORD image_base,
                                 DWORD thread_id)
{
    char ddraw_path[MAX_PATH_TEXT];
    DWORD ddraw_base = 0;
    DWORD ddraw_size = 0;
    ProbeDiagnosticV1 diag;
    DWORD current_table_pointer = 0;
    DWORD source_words[ENGINE_TABLE_WORD_COUNT];
    DWORD clone_words[ENGINE_TABLE_WORD_COUNT];
    DWORD i;
    int unselected_preserved = 1;
    int source_unchanged;
    int text_unchanged;
    int pointer_published;
    int clone_in_module;
    int diagnostic_valid;

    if (!find_sibling_ddraw(g_process_id, exe_path, ddraw_path,
                            sizeof(ddraw_path), &ddraw_base, &ddraw_size)) {
        printf("APP_LOCAL_DDRAW_SELECTED=0\n");
        return 0;
    }
    if (!read_remote(g_process, ddraw_base + PROBE_DIAGNOSTIC_RVA,
                     &diag, sizeof(diag)))
        return 0;
    if (!read_remote(g_process, image_base + HOST_TABLE_POINTER_RVA,
                     &current_table_pointer, sizeof(current_table_pointer)))
        return 0;
    if (!read_remote(g_process, image_base + HOST_TABLE_RVA, source_words,
                     sizeof(source_words)))
        return 0;
    if (!read_remote(g_process, diag.clone_pointer, clone_words,
                     sizeof(clone_words)))
        return 0;

    diagnostic_valid = diag.magic == PREENTRY_DIAG_MAGIC &&
        diag.state == PREENTRY_DIAG_STATE_PUBLISHED &&
        diag.process_attach_calls == 1UL &&
        diag.module_base == image_base &&
        diag.source_pointer_cell == image_base + HOST_TABLE_POINTER_RVA &&
        diag.source_pointer_before == image_base + HOST_TABLE_RVA &&
        diag.source_pointer_after == diag.clone_pointer &&
        diag.clone_status == ENGINE_TABLE_CLONE_OK;
    pointer_published = current_table_pointer == diag.clone_pointer;
    clone_in_module = ddraw_size >= sizeof(clone_words) &&
        diag.clone_pointer >= ddraw_base &&
        diag.clone_pointer - ddraw_base <= ddraw_size - sizeof(clone_words);
    source_unchanged = compare_original_table(g_process, exe_path,
                                               image_base, source_words);
    text_unchanged = compare_host_text(g_process, exe_path, image_base);
    for (i = 0; i < ENGINE_TABLE_WORD_COUNT; ++i) {
        if (i == HERCULES_SLOT_SUBMIT) {
            if (clone_words[i] != 0xf17e0089UL)
                unselected_preserved = 0;
        } else if (clone_words[i] != source_words[i]) {
            unselected_preserved = 0;
        }
    }

    printf("APP_LOCAL_DDRAW_SELECTED=1\n");
    printf("DDRAW_PATH=%s\n", ddraw_path);
    printf("DDRAW_BASE=0x%08lx\n", (unsigned long)ddraw_base);
    printf("DDRAW_SIZE=0x%08lx\n", (unsigned long)ddraw_size);
    printf("ATTACH_STATE=0x%08lx\n", (unsigned long)diag.state);
    printf("ATTACH_CALLS=%lu\n", diag.process_attach_calls);
    printf("TABLE_POINTER_CELL=0x%08lx\n", (unsigned long)(image_base + HOST_TABLE_POINTER_RVA));
    printf("TABLE_POINTER_CURRENT=0x%08lx\n", (unsigned long)current_table_pointer);
    printf("SOURCE_TABLE=0x%08lx\n", (unsigned long)(image_base + HOST_TABLE_RVA));
    printf("CLONE_TABLE=0x%08lx\n", (unsigned long)diag.clone_pointer);
    printf("CLONE_IN_DDRAW_MODULE=%d\n", clone_in_module);
    printf("SOURCE_WORD1=0x%08lx\n", (unsigned long)source_words[1]);
    printf("CLONE_WORD1=0x%08lx\n", (unsigned long)clone_words[1]);
    printf("SOURCE_SLOT89=0x%08lx\n", (unsigned long)source_words[89]);
    printf("CLONE_SLOT89=0x%08lx\n", (unsigned long)clone_words[89]);
    printf("DIRECTDRAWCREATE_CALLS=%lu\n", diag.directdrawcreate_calls);
    printf("DIAGNOSTIC_VALID=%d\n", diagnostic_valid);
    printf("POINTER_PUBLISHED=%d\n", pointer_published);
    printf("SOURCE_TABLE_UNCHANGED=%d\n", source_unchanged);
    printf("CLONE_UNSELECTED_PRESERVED=%d\n", unselected_preserved);
    printf("EXE_TEXT_UNCHANGED=%d\n", text_unchanged);

    return diagnostic_valid && pointer_published && clone_in_module && source_unchanged &&
        unselected_preserved && text_unchanged &&
        diag.source_word0 == ENGINE_TABLE_SIGNATURE &&
        diag.source_word1 == source_words[1] &&
        diag.source_slot89 == source_words[89] &&
        diag.clone_word0 == clone_words[0] &&
        diag.clone_word1 == clone_words[1] &&
        diag.clone_slot89 == clone_words[89] &&
        source_words[0] == ENGINE_TABLE_SIGNATURE &&
        clone_words[0] == ENGINE_TABLE_SIGNATURE &&
        clone_words[1] == source_words[1] &&
        clone_words[89] == 0xf17e0089UL &&
        diag.directdrawcreate_calls == 0UL;
}

static DWORD wait_for_terminated_debuggee(DWORD timeout_ms)
{
    DWORD start = GetTickCount();
    while ((DWORD)(GetTickCount() - start) < timeout_ms) {
        DEBUG_EVENT event;
        DWORD remaining = timeout_ms - (DWORD)(GetTickCount() - start);
        DWORD slice = remaining > 250UL ? 250UL : remaining;
        if (WaitForDebugEvent(&event, slice)) {
            g_pending_event = 1;
            g_thread_id = event.dwThreadId;
            close_event_handles(&event);
            if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId,
                                    DBG_CONTINUE))
                return WAIT_FAILED;
            g_pending_event = 0;
            if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
                g_exit_process_event_drained = 1;
                if (WaitForSingleObject(g_process, 1000UL) == WAIT_OBJECT_0)
                    return WAIT_OBJECT_0;
            }
        } else if (GetLastError() != PREENTRY_ERROR_SEM_TIMEOUT) {
            return WAIT_FAILED;
        } else if (WaitForSingleObject(g_process, 0) == WAIT_OBJECT_0) {
            return WAIT_OBJECT_0;
        }
    }
    return WaitForSingleObject(g_process, 0);
}

static DWORD kill_child(void)
{
    DWORD wait_result;
    if (g_process != 0) {
        BOOL terminate_accepted;
        int already_terminal;
        printf("KILL_REQUESTED=1\n");
        fflush(stdout);
        terminate_accepted = TerminateProcess(g_process, CHILD_EXIT_CODE);
        already_terminal = WaitForSingleObject(g_process, 0) == WAIT_OBJECT_0;
        printf("TERMINATE_PROCESS_ACCEPTED=%d\n", terminate_accepted != 0);
        printf("PROCESS_ALREADY_TERMINAL=%d\n", already_terminal);
        fflush(stdout);
        if (!terminate_accepted && !already_terminal)
            return WAIT_FAILED;
        if (g_pending_event) {
            printf("CONTINUING_PENDING_EVENT=%lu,%lu\n",
                   (unsigned long)g_process_id, (unsigned long)g_thread_id);
            fflush(stdout);
            if (!ContinueDebugEvent(g_process_id, g_thread_id, DBG_CONTINUE))
                return WAIT_FAILED;
            g_pending_event = 0;
        }
        wait_result = wait_for_terminated_debuggee(5000UL);
        printf("KILL_WAIT_RESULT=%lu\n", (unsigned long)wait_result);
        fflush(stdout);
        if (wait_result != WAIT_OBJECT_0) {
            terminate_accepted = TerminateProcess(g_process, CHILD_EXIT_CODE);
            already_terminal = WaitForSingleObject(g_process, 0) == WAIT_OBJECT_0;
            if (!terminate_accepted && !already_terminal)
                return WAIT_FAILED;
            wait_result = wait_for_terminated_debuggee(5000UL);
        }
        return wait_result;
    }
    return WAIT_OBJECT_0;
}

static int run_debugged_child(const char *exe_path, const char *work_dir,
                              DWORD entry_rva, int host_mode,
                              int failure_stop_mode)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION process_info;
    DEBUG_EVENT event;
    char command_line[MAX_PATH_TEXT * 2];
    DWORD process_image_base = 0;
    DWORD start_tick;
    BOOL created;
    int result = 0;
    g_failure_stop_mode = failure_stop_mode;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    memset(&process_info, 0, sizeof(process_info));
    if (strlen(exe_path) + 4 >= sizeof(command_line))
        return 0;
    sprintf(command_line, "\"%s\"", exe_path);
    created = CreateProcessA(exe_path, command_line, 0, 0, FALSE,
        DEBUG_ONLY_THIS_PROCESS | CREATE_SUSPENDED, 0, work_dir,
        &startup, &process_info);
    if (!created) {
        printf("CREATE_PROCESS_ERROR=%lu\n", (unsigned long)GetLastError());
        return 0;
    }
    g_process = process_info.hProcess;
    g_thread = process_info.hThread;
    g_process_id = process_info.dwProcessId;
    g_thread_id = process_info.dwThreadId;
    if (!enable_kill_on_debugger_exit()) {
        printf("KILL_ON_EXIT_SETUP_FAILED=1\n");
        goto done;
    }
    printf("KILL_ON_EXIT_MODE=%d\n", g_kill_on_exit_mode);
    start_tick = GetTickCount();
    /* CREATE_SUSPENDED supplies the initial suspend count; release that count
     * to let the debugger receive CREATE_PROCESS_DEBUG_EVENT. DEBUG_ONLY_THIS_PROCESS
     * holds the debuggee at that event before any user entrypoint instruction. */
    {
        DWORD previous_suspend_count = ResumeThread(process_info.hThread);
        printf("CREATE_SUSPENDED_RELEASE_COUNT=%lu\n",
               (unsigned long)previous_suspend_count);
        fflush(stdout);
        if (previous_suspend_count != 1UL)
            goto done;
        g_resumed_initial_thread = 1;
    }

    while ((DWORD)(GetTickCount() - start_tick) < DEBUG_TIMEOUT_MS) {
        DWORD remaining = DEBUG_TIMEOUT_MS - (DWORD)(GetTickCount() - start_tick);
        DWORD wait_ms = remaining > 1000UL ? 1000UL : remaining;
        if (!WaitForDebugEvent(&event, wait_ms)) {
            DWORD error = GetLastError();
            if (error == PREENTRY_ERROR_SEM_TIMEOUT)
                continue;
            printf("WAIT_FOR_DEBUG_EVENT_ERROR=%lu\n", (unsigned long)error);
            goto done;
        }
        g_pending_event = 1;
        g_thread_id = event.dwThreadId;
        printf("DEBUG_EVENT=%lu TID=%lu\n",
               (unsigned long)event.dwDebugEventCode,
               (unsigned long)event.dwThreadId);
        fflush(stdout);

        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            CONTEXT context;
            if (g_seen_create_process) {
                close_event_handles(&event);
                goto done;
            }
            g_seen_create_process = 1;
            process_image_base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            if (process_image_base != HOST_IMAGE_BASE || event.dwThreadId != process_info.dwThreadId) {
                close_event_handles(&event);
                goto done;
            }
            if (failure_stop_mode) {
                printf("FAILURE_INJECTED_BEFORE_BREAKPOINT=1\n");
                fflush(stdout);
                close_event_handles(&event);
                goto done;
            }
            memset(&context, 0, sizeof(context));
            context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
            if (!GetThreadContext(process_info.hThread, &context)) {
                close_event_handles(&event);
                goto done;
            }
            context.Dr0 = process_image_base + entry_rva;
            context.Dr6 = 0;
            context.Dr7 = (context.Dr7 & ~0x000f0003UL) | 0x00000001UL;
            if (!SetThreadContext(process_info.hThread, &context)) {
                close_event_handles(&event);
                goto done;
            }
            printf("BREAKPOINT_ARMED=0x%08lx\n",
                   (unsigned long)context.Dr0);
            fflush(stdout);
            close_event_handles(&event);
            if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId,
                                    DBG_CONTINUE)) {
                goto done;
            }
            g_pending_event = 0;
            printf("INITIAL_DEBUG_EVENT_CONTINUED=1\n");
            fflush(stdout);
            continue;
        }

        if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            DWORD code = event.u.Exception.ExceptionRecord.ExceptionCode;
            DWORD exception_address = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            printf("EXCEPTION=0x%08lx ADDRESS=0x%08lx\n",
                   (unsigned long)code, (unsigned long)exception_address);
            fflush(stdout);
            if (code == EXCEPTION_SINGLE_STEP) {
                CONTEXT context;
                memset(&context, 0, sizeof(context));
                context.ContextFlags = CONTEXT_CONTROL | CONTEXT_DEBUG_REGISTERS;
                if (event.dwThreadId != process_info.dwThreadId ||
                    !GetThreadContext(process_info.hThread, &context)) {
                    close_event_handles(&event);
                    goto done;
                }
                g_entry_eip = context.Eip;
                g_entry_exception_address = exception_address;
                g_entry_dr0 = context.Dr0;
                g_entry_dr6 = context.Dr6;
                g_entry_dr7 = context.Dr7;
                if (context.Eip != process_image_base + entry_rva ||
                    exception_address != process_image_base + entry_rva) {
                    close_event_handles(&event);
                    goto done;
                }
                g_entry_stopped = 1;
                g_hardware_breakpoint_confirmed =
                    (context.Dr6 & 1UL) != 0 &&
                    context.Dr0 == process_image_base + entry_rva &&
                    (context.Dr7 & 0x000f000fUL) == 0x00000001UL;
                printf("BREAKPOINT_DR0=0x%08lx\n", (unsigned long)context.Dr0);
                printf("BREAKPOINT_DR6=0x%08lx\n", (unsigned long)context.Dr6);
                printf("BREAKPOINT_DR7=0x%08lx\n", (unsigned long)context.Dr7);
                printf("BREAKPOINT_HW_B0=%d\n", (context.Dr6 & 1UL) != 0);
                printf("BREAKPOINT_EXEC_CONFIG=%d\n",
                       (context.Dr0 == process_image_base + entry_rva &&
                        (context.Dr7 & 0x000f000fUL) == 1UL));
                printf("HARDWARE_BP_CONFIRMED=%d\n",
                       g_hardware_breakpoint_confirmed);
                fflush(stdout);
                printf("ENTRY_STOP_CONFIRMED=0x%08lx\n",
                       (unsigned long)context.Eip);
                fflush(stdout);
                if (!g_hardware_breakpoint_confirmed)
                    result = 0;
                else if (host_mode)
                    result = inspect_host_at_entry(exe_path, process_image_base,
                                                   event.dwThreadId);
                else
                    result = 1;
                close_event_handles(&event);
                goto done;
            }
            if (code == EXCEPTION_BREAKPOINT &&
                g_initial_breakpoint_count == 0 &&
                exception_address != process_image_base + entry_rva) {
                ++g_initial_breakpoint_count;
                close_event_handles(&event);
                if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId,
                                        DBG_CONTINUE)) {
                    goto done;
                }
                g_pending_event = 0;
                continue;
            }
            printf("UNEXPECTED_EXCEPTION=0x%08lx\n", (unsigned long)code);
            close_event_handles(&event);
            goto done;
        }

        if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            close_event_handles(&event);
            if (ContinueDebugEvent(event.dwProcessId, event.dwThreadId,
                                   DBG_CONTINUE)) {
                g_pending_event = 0;
                g_exit_process_event_drained = 1;
            }
            goto done;
        }

        close_event_handles(&event);
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId,
                                DBG_CONTINUE)) {
            goto done;
        }
        g_pending_event = 0;
    }
    printf("DEBUG_TIMEOUT_MS=%lu\n", (unsigned long)DEBUG_TIMEOUT_MS);

done:
    if (g_process != 0) {
        DWORD child_wait;
        printf("CLEANUP_BEGIN=1\n");
        fflush(stdout);
        if (WaitForSingleObject(g_process, 0) == WAIT_TIMEOUT)
            child_wait = kill_child();
        else
            child_wait = WAIT_OBJECT_0;
        if (child_wait != WAIT_OBJECT_0) {
            result = 0;
        }
        printf("CHILD_TERMINATED=%d\n", child_wait == WAIT_OBJECT_0);
        printf("EXIT_PROCESS_EVENT_DRAINED=%d\n", g_exit_process_event_drained);
        if (g_failure_stop_mode && child_wait == WAIT_OBJECT_0 &&
            !g_entry_stopped)
            result = 1;
        CloseHandle(g_thread);
        CloseHandle(g_process);
        g_process = 0;
        g_thread = 0;
    }
    printf("BREAKPOINT_EIP=0x%08lx\n", (unsigned long)g_entry_eip);
    printf("BREAKPOINT_EXCEPTION_ADDRESS=0x%08lx\n",
           (unsigned long)g_entry_exception_address);
    printf("BREAKPOINT_DR0=0x%08lx\n", (unsigned long)g_entry_dr0);
    printf("BREAKPOINT_DR6=0x%08lx\n", (unsigned long)g_entry_dr6);
    printf("BREAKPOINT_DR7=0x%08lx\n", (unsigned long)g_entry_dr7);
    printf("HARDWARE_BP_CONFIRMED=%d\n", g_hardware_breakpoint_confirmed);
    printf("STOPPED_BEFORE_ENTRY=%d\n", g_entry_stopped);
    printf("FAILURE_PATH_NO_ENTRY=%d\n",
           g_failure_stop_mode && !g_entry_stopped);
    printf("INITIAL_BREAKPOINT_EVENTS=%d\n", g_initial_breakpoint_count);
    printf("INITIAL_THREAD_RESUMED=%d\n", g_resumed_initial_thread);
    printf("RESULT=%s\n", result ? "PASS" : "FAIL");
    return result;
}

int main(int argc, char **argv)
{
    int host_mode;
    int failure_stop_mode;
    DWORD entry_rva;
    char *end = 0;
    if (argc < 2 ||
        (strcmp(argv[1], "--host") == 0 && argc != 4) ||
        (strcmp(argv[1], "--smoke") == 0 && argc != 5) ||
        (strcmp(argv[1], "--failure-smoke") == 0 && argc != 4) ||
        (strcmp(argv[1], "--host") != 0 && strcmp(argv[1], "--smoke") != 0 &&
         strcmp(argv[1], "--failure-smoke") != 0)) {
        fprintf(stderr, "usage: debug_controller.exe --host <exe> <workdir>\n");
        fprintf(stderr, "   or: debug_controller.exe --smoke <exe> <workdir> <entry-rva>\n");
        fprintf(stderr, "   or: debug_controller.exe --failure-smoke <exe> <workdir>\n");
        return 2;
    }
    host_mode = strcmp(argv[1], "--host") == 0;
    failure_stop_mode = strcmp(argv[1], "--failure-smoke") == 0;
    entry_rva = HOST_ENTRY_RVA;
    if (strcmp(argv[1], "--smoke") == 0)
        entry_rva = (DWORD)strtoul(argv[4], &end, 0);
    if (strcmp(argv[1], "--smoke") == 0 && (end == argv[4] || *end != '\0'))
        return 2;
    if (!run_debugged_child(argv[2], argv[3], entry_rva, host_mode,
                            failure_stop_mode))
        return 1;
    return 0;
}
