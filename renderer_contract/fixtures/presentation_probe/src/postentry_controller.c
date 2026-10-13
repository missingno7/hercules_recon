#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "postentry_shared.h"
#include "postentry_layout.h"

#define POSTENTRY_TIMEOUT_MS 12000UL
#define POSTENTRY_CHILD_TIMEOUT_EXIT 0x504f5354UL
#define POSTENTRY_MAX_THREADS 16UL
#define WAIT_TIMEOUT_ERROR 121UL

typedef DWORD (WINAPI *GetFinalPathNameByHandleAFn)(HANDLE, LPSTR, DWORD,
                                                     DWORD);
typedef BOOL (WINAPI *DebugSetProcessKillOnExitFn)(BOOL);

typedef struct PostentryThreadV1 {
    DWORD id;
    HANDLE handle;
} PostentryThreadV1;

static HANDLE g_process;
static HANDLE g_initial_thread;
static DWORD g_process_id;
static DWORD g_initial_thread_id;
static int g_pending_event;
static DWORD g_pending_process_id;
static DWORD g_pending_thread_id;
static int g_exit_event_drained;
static int g_kill_on_exit_mode;
static int g_process_created;
static int g_seen_create_process;
static int g_proxy_loaded;
static int g_engine_loaded;
static int g_cold_copy_verified;
static int g_warm_copy_verified;
static int g_forwarded_render_verified;
static int g_timeout_observed;
static int g_invalid_signature_observed;
static DWORD g_host_base;
static DWORD g_proxy_base;
static DWORD g_engine_base;
static DWORD g_proxy_size;
static DWORD g_engine_size;
static DWORD g_engine_copy_address;
static DWORD g_engine_warm_address;
static DWORD g_engine_render_address;
static DWORD g_test_mode;
static char g_host_path[MAX_PATH];
static char g_proxy_path[MAX_PATH];
static char g_engine_path[MAX_PATH];
static PostentryThreadV1 g_threads[POSTENTRY_MAX_THREADS];
static DWORD g_thread_count;

static int arm_thread(PostentryThreadV1 *thread, DWORD enabled_mask);

static void print_state(const char *state)
{
    printf("STATE=%s\n", state);
    fflush(stdout);
}

static int full_path(const char *input, char *output, DWORD capacity)
{
    DWORD length = GetFullPathNameA(input, capacity, output, 0);
    return length != 0 && length < capacity;
}

static void normalize_path(char *path)
{
    if (strncmp(path, "\\\\?\\", 4) == 0)
        memmove(path, path + 4, strlen(path + 4) + 1);
}

static int final_path_from_file(HANDLE file, char *path, DWORD capacity)
{
    HMODULE kernel = GetModuleHandleA("KERNEL32.DLL");
    FARPROC address = kernel == 0 ? 0 : GetProcAddress(
        kernel, "GetFinalPathNameByHandleA");
    GetFinalPathNameByHandleAFn get_final_path;
    DWORD length;
    if (address == 0 || file == 0 || file == INVALID_HANDLE_VALUE)
        return 0;
    get_final_path = (GetFinalPathNameByHandleAFn)address;
    length = get_final_path(file, path, capacity, 0UL);
    if (length == 0 || length >= capacity)
        return 0;
    path[length] = '\0';
    normalize_path(path);
    return 1;
}

static int path_matches_file(HANDLE file, const char *expected)
{
    char actual[MAX_PATH];
    return final_path_from_file(file, actual, sizeof(actual)) &&
           _stricmp(actual, expected) == 0;
}

static int read_remote(DWORD address, void *buffer, DWORD length)
{
    DWORD bytes_read = 0UL;
    return ReadProcessMemory(g_process, (LPCVOID)address, buffer, length,
                             &bytes_read) && bytes_read == length;
}

static int verify_remote_pe(DWORD base, DWORD expected_size)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    if (!read_remote(base, &dos, sizeof(dos)) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 ||
        dos.e_lfanew > 0x1000 ||
        !read_remote(base + (DWORD)dos.e_lfanew, &nt, sizeof(nt)))
        return 0;
    return nt.Signature == IMAGE_NT_SIGNATURE &&
        nt.FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
        nt.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC &&
        nt.OptionalHeader.SizeOfImage == expected_size;
}

static int enable_kill_on_exit(void)
{
    HMODULE kernel = GetModuleHandleA("KERNEL32.DLL");
    FARPROC address = kernel == 0 ? 0 : GetProcAddress(
        kernel, "DebugSetProcessKillOnExit");
    DebugSetProcessKillOnExitFn set_kill;
    if (address == 0) {
        g_kill_on_exit_mode = 2;
        return 1;
    }
    set_kill = (DebugSetProcessKillOnExitFn)address;
    g_kill_on_exit_mode = set_kill(TRUE) ? 1 : 0;
    return g_kill_on_exit_mode != 0;
}

static void close_event_handles(const DEBUG_EVENT *event)
{
    if (event->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
        if (event->u.CreateProcessInfo.hFile != 0)
            CloseHandle(event->u.CreateProcessInfo.hFile);
        if (event->u.CreateProcessInfo.hProcess != 0)
            CloseHandle(event->u.CreateProcessInfo.hProcess);
        if (event->u.CreateProcessInfo.hThread != 0)
            CloseHandle(event->u.CreateProcessInfo.hThread);
    } else if (event->dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
        if (event->u.LoadDll.hFile != 0)
            CloseHandle(event->u.LoadDll.hFile);
    } else if (event->dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
        if (event->u.CreateThread.hThread != 0)
            CloseHandle(event->u.CreateThread.hThread);
    }
}

static PostentryThreadV1 *find_thread(DWORD thread_id)
{
    DWORD index;
    for (index = 0; index < g_thread_count; ++index) {
        if (g_threads[index].id == thread_id)
            return &g_threads[index];
    }
    return 0;
}

static int add_thread(DWORD thread_id, HANDLE source_handle)
{
    PostentryThreadV1 *record = find_thread(thread_id);
    HANDLE duplicate = 0;
    if (record != 0)
        return 1;
    if (g_thread_count >= POSTENTRY_MAX_THREADS || source_handle == 0)
        return 0;
    if (!DuplicateHandle(GetCurrentProcess(), source_handle,
                         GetCurrentProcess(), &duplicate, 0, FALSE,
                         DUPLICATE_SAME_ACCESS))
        return 0;
    g_threads[g_thread_count].id = thread_id;
    g_threads[g_thread_count].handle = duplicate;
    ++g_thread_count;
    if (g_engine_loaded && !arm_thread(&g_threads[g_thread_count - 1], 0x15UL))
        return 0;
    return 1;
}

static int arm_thread(PostentryThreadV1 *thread, DWORD enabled_mask)
{
    CONTEXT context;
    DWORD dr7_mask = 0x000f00ffUL;
    if (thread == 0 || thread->handle == 0)
        return 0;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(thread->handle, &context))
        return 0;
    context.Dr0 = g_engine_copy_address;
    context.Dr1 = g_engine_warm_address;
    context.Dr2 = g_engine_render_address;
    context.Dr6 = 0UL;
    context.Dr7 = (context.Dr7 & ~dr7_mask) | (enabled_mask & 0xffUL);
    return SetThreadContext(thread->handle, &context) != 0;
}

static void clear_thread_handles(void)
{
    DWORD index;
    for (index = 0; index < g_thread_count; ++index) {
        if (g_threads[index].handle != 0) {
            CloseHandle(g_threads[index].handle);
            g_threads[index].handle = 0;
        }
    }
    g_thread_count = 0UL;
}

static int verify_tables(DWORD expected_copy_count, int marker_kind)
{
    SyntheticHostStateV1 host;
    SyntheticEngineStateV1 engine;
    PresentationProbeV1 *probe;
    renderer_u32 index;
    DWORD pointer_cell_address = g_host_base + POSTENTRY_HOST_STATE_RVA +
        (DWORD)offsetof(SyntheticHostStateV1, pointer_cell);
    DWORD source_address = g_host_base + POSTENTRY_HOST_STATE_RVA +
        (DWORD)offsetof(SyntheticHostStateV1, source_table);
    DWORD clone_address = g_host_base + POSTENTRY_HOST_STATE_RVA +
        (DWORD)offsetof(SyntheticHostStateV1, clone_table);
    int valid = 1;

    if (!read_remote(g_host_base + POSTENTRY_HOST_STATE_RVA, &host,
                     sizeof(host)) ||
        !read_remote(g_engine_base + POSTENTRY_ENGINE_STATE_RVA, &engine,
                     sizeof(engine)))
        return 0;
    if (host.magic != POSTENTRY_HOST_MAGIC ||
        host.candidate_base != g_proxy_base ||
        host.engine_base != g_engine_base ||
        host.state != POSTENTRY_HOST_STATE_ENGINE_LOADED ||
        host.publish_status != ENGINE_TABLE_CLONE_OK ||
        host.pointer_cell != clone_address ||
        host.probe_state_pointer < g_proxy_base ||
        host.probe_state_pointer - g_proxy_base >= g_proxy_size ||
        engine.magic != POSTENTRY_ENGINE_MAGIC ||
        engine.copy_count != expected_copy_count)
        return 0;

    if ((marker_kind == 1 &&
         (engine.after_copy_hits != 0UL || engine.after_warm_copy_hits != 0UL)) ||
        (marker_kind == 2 &&
         (engine.after_copy_hits != 1UL || engine.after_warm_copy_hits != 0UL)) ||
        (marker_kind == 3 &&
         (engine.after_copy_hits != 1UL ||
          engine.after_warm_copy_hits != (expected_copy_count == 2UL ? 1UL : 0UL))))
        return 0;

    if (host.pointer_cell != clone_address ||
        host.source_table[0] != ENGINE_TABLE_SIGNATURE ||
        host.source_table[HERCULES_SLOT_CHAIN_INIT] < g_host_base ||
        host.source_table[HERCULES_SLOT_CHAIN_INIT] - g_host_base >=
            POSTENTRY_HOST_IMAGE_SIZE ||
        host.source_table[HERCULES_SLOT_SUBMIT] < g_host_base ||
        host.source_table[HERCULES_SLOT_SUBMIT] - g_host_base >=
            POSTENTRY_HOST_IMAGE_SIZE)
        valid = 0;

    for (index = 0; index < ENGINE_TABLE_WORD_COUNT; ++index) {
        renderer_u32 expected = 0xa3000000UL + index;
        if (index == 0UL)
            expected = ENGINE_TABLE_SIGNATURE;
        else if (index == HERCULES_SLOT_CHAIN_INIT)
            expected = host.source_table[index];
        else if (index == HERCULES_SLOT_SUBMIT)
            expected = host.source_table[index];
        if (host.source_table[index] != expected)
            valid = 0;
        if (index == HERCULES_SLOT_CHAIN_INIT ||
            index == HERCULES_SLOT_SUBMIT) {
            if (host.clone_table[index] == host.source_table[index] ||
                host.clone_table[index] < g_proxy_base ||
                host.clone_table[index] - g_proxy_base >= g_proxy_size)
                valid = 0;
        } else if (host.clone_table[index] != host.source_table[index]) {
            valid = 0;
        }
        if (engine.local_table[index] != host.clone_table[index])
            valid = 0;
    }

    probe = (PresentationProbeV1 *)malloc(sizeof(*probe));
    if (probe == 0)
        return 0;
    if (!read_remote(host.probe_state_pointer, probe, sizeof(*probe))) {
        free(probe);
        return 0;
    }
    if (probe->magic != PRESENTATION_PROBE_MAGIC ||
        probe->state != PRESENTATION_STATE_OTHER_IMAGE ||
        probe->source_pointer_cell != pointer_cell_address ||
        probe->source_pointer_before != source_address ||
        probe->source_pointer_after != clone_address ||
        probe->clone_pointer != clone_address ||
        probe->original_chain_init != host.source_table[HERCULES_SLOT_CHAIN_INIT] ||
        probe->original_submit != host.source_table[HERCULES_SLOT_SUBMIT] ||
        probe->directdrawcreate_calls != 0UL)
        valid = 0;
    free(probe);
    return valid;
}

static int verify_render(void)
{
    SyntheticHostStateV1 host;
    SyntheticEngineStateV1 engine;
    PresentationProbeV1 *probe;
    PresentationTraceEventV1 *events;
    renderer_u32 index;
    int valid = 1;

    if (!verify_tables(g_test_mode == 2UL ? 2UL : 1UL, 3))
        return 0;
    if (!read_remote(g_host_base + POSTENTRY_HOST_STATE_RVA, &host,
                     sizeof(host)) ||
        !read_remote(g_engine_base + POSTENTRY_ENGINE_STATE_RVA, &engine,
                     sizeof(engine)))
        return 0;
    probe = (PresentationProbeV1 *)malloc(sizeof(*probe));
    if (probe == 0)
        return 0;
    if (!read_remote(host.probe_state_pointer, probe, sizeof(*probe))) {
        free(probe);
        return 0;
    }
    if (engine.render_complete != 1UL || engine.after_render_hits != 0UL ||
        engine.callback_thread_id != host.worker_thread_id ||
        engine.slot81_result != 0x11223344UL ||
        host.callback81_calls != 1UL || host.callback89_calls != 1UL ||
        host.callback81_first != 0x81810001UL ||
        host.callback81_count != 0x81000001UL ||
        host.callback81_return != 0x11223344UL ||
        host.callback89_first != 0x89890001UL ||
        host.callback89_raw_eax != 0x55667788UL ||
        probe->slot81_entered != 1UL || probe->slot81_completed != 1UL ||
        probe->slot89_entered != 1UL || probe->slot89_completed != 1UL ||
        probe->active_wrapper_calls != 0UL ||
        probe->max_active_wrapper_calls < 1UL ||
        probe->trace_count != 4UL || probe->trace_dropped != 0UL)
        valid = 0;

    events = probe->trace;
    for (index = 0; index < 4UL; ++index) {
        if (events[index].sequence != index + 1UL ||
            events[index].committed_sequence != (LONG)(index + 1UL) ||
            events[index].thread_id != host.worker_thread_id)
            valid = 0;
    }
    if (events[0].kind != PRESENTATION_EVENT_CHAIN_INIT ||
        events[0].arg0 != 0x81810001UL ||
        events[0].arg1 != 0x81000001UL ||
        events[0].target != host.source_table[HERCULES_SLOT_CHAIN_INIT] ||
        events[1].kind != PRESENTATION_EVENT_CHAIN_INIT_RETURN ||
        events[1].arg0 != 0x81810001UL ||
        events[1].arg1 != 0x81000001UL ||
        events[1].arg2 != 0x11223344UL ||
        events[1].target != host.source_table[HERCULES_SLOT_CHAIN_INIT] ||
        events[2].kind != PRESENTATION_EVENT_SUBMIT ||
        events[2].arg0 != 0x89890001UL ||
        events[2].target != host.source_table[HERCULES_SLOT_SUBMIT] ||
        events[3].kind != PRESENTATION_EVENT_SUBMIT_RETURN ||
        events[3].arg0 != 0x89890001UL ||
        events[3].arg1 != 0x55667788UL ||
        events[3].target != host.source_table[HERCULES_SLOT_SUBMIT])
        valid = 0;
    free(probe);
    return valid;
}

static int continue_pending(DWORD status)
{
    if (!g_pending_event)
        return 1;
    if (!ContinueDebugEvent(g_pending_process_id, g_pending_thread_id, status))
        return 0;
    g_pending_event = 0;
    return 1;
}

static int drain_exit(DWORD timeout_ms)
{
    DWORD start = GetTickCount();
    while ((DWORD)(GetTickCount() - start) < timeout_ms) {
        DEBUG_EVENT event;
        DWORD remaining = timeout_ms - (DWORD)(GetTickCount() - start);
        DWORD wait = remaining > 250UL ? 250UL : remaining;
        if (!WaitForDebugEvent(&event, wait)) {
            if (GetLastError() == WAIT_TIMEOUT_ERROR)
                continue;
            return 0;
        }
        g_pending_event = 1;
        g_pending_process_id = event.dwProcessId;
        g_pending_thread_id = event.dwThreadId;
        if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
            g_exit_event_drained = 1;
        close_event_handles(&event);
        if (!continue_pending(DBG_CONTINUE))
            return 0;
        if (g_exit_event_drained)
            return WaitForSingleObject(g_process, 1000UL) == WAIT_OBJECT_0;
    }
    return 0;
}

static int terminate_and_reap(DWORD exit_code)
{
    DWORD process_state;
    BOOL terminate_accepted = FALSE;
    if (g_process == 0)
        return 1;
    process_state = WaitForSingleObject(g_process, 0UL);
    if (process_state != WAIT_OBJECT_0) {
        terminate_accepted = TerminateProcess(g_process, exit_code);
        if (!terminate_accepted &&
            WaitForSingleObject(g_process, 0UL) != WAIT_OBJECT_0)
            return 0;
    }
    printf("TERMINATE_PROCESS_ACCEPTED=%d\n",
           terminate_accepted != 0 || process_state == WAIT_OBJECT_0);
    fflush(stdout);
    if (!continue_pending(DBG_CONTINUE))
        return 0;
    if (!drain_exit(5000UL))
        return 0;
    printf("CHILD_TERMINATED=1\n");
    printf("EXIT_PROCESS_EVENT_DRAINED=%d\n", g_exit_event_drained);
    printf("KILL_ON_EXIT_MODE=%d\n", g_kill_on_exit_mode);
    fflush(stdout);
    return g_exit_event_drained &&
        WaitForSingleObject(g_process, 0UL) == WAIT_OBJECT_0;
}

static int arm_loaded_threads(void)
{
    DWORD index;
    for (index = 0; index < g_thread_count; ++index) {
        if (!arm_thread(&g_threads[index], 0x15UL))
            return 0;
    }
    return 1;
}

static int stop_at_marker(PostentryThreadV1 *thread, DWORD dr_bit,
                          DWORD expected_eip, DWORD remaining_mask,
                          const char *state)
{
    CONTEXT context;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_CONTROL | CONTEXT_DEBUG_REGISTERS;
    if (thread == 0 || !GetThreadContext(thread->handle, &context))
        return 0;
    printf("MARKER_EIP=0x%08lx DR6=0x%08lx DR7=0x%08lx\n",
           (unsigned long)context.Eip, (unsigned long)context.Dr6,
           (unsigned long)context.Dr7);
    if (context.Eip != expected_eip || (context.Dr6 & dr_bit) == 0UL ||
        (context.Dr7 & 0x000f00ffUL) !=
            (dr_bit == 1UL ? 0x00000015UL :
             dr_bit == 2UL ? 0x00000014UL : 0x00000010UL))
        return 0;
    context.Dr6 = 0UL;
    context.Dr7 = (context.Dr7 & ~0x000000ffUL) | remaining_mask;
    if (!SetThreadContext(thread->handle, &context))
        return 0;
    print_state(state);
    return 1;
}

static int read_host_on_exit(int invalid_signature, DWORD exit_code)
{
    SyntheticHostStateV1 host;
    if (!read_remote(g_host_base + POSTENTRY_HOST_STATE_RVA, &host,
                     sizeof(host)))
        return 0;
    if (!invalid_signature || host.magic != POSTENTRY_HOST_MAGIC ||
        host.state != POSTENTRY_HOST_STATE_BAD_SIGNATURE ||
        host.publish_status != ENGINE_TABLE_CLONE_BAD_SIGNATURE ||
        host.pointer_cell != g_host_base + POSTENTRY_HOST_STATE_RVA +
            (DWORD)offsetof(SyntheticHostStateV1, source_table) ||
        exit_code != 0x51UL || g_engine_loaded)
        return 0;
    g_invalid_signature_observed = 1;
    print_state("INVALID_SIGNATURE_REJECTED");
    return 1;
}

static int process_marker(DWORD thread_id, DWORD exception_address,
                          DWORD dr6)
{
    PostentryThreadV1 *thread = find_thread(thread_id);
    if (thread == 0)
        return 0;
    if ((dr6 & 1UL) != 0UL) {
        if (g_cold_copy_verified || exception_address != g_engine_copy_address ||
            !stop_at_marker(thread, 1UL, g_engine_copy_address,
                            g_test_mode == 2UL ? 0x14UL : 0x10UL,
                            "COLD_COPY_BREAKPOINT") ||
            !verify_tables(1UL, 1))
            return 0;
        g_cold_copy_verified = 1;
        printf("COLD_COPY_WORDS=250\nCOLD_COPY_MATCHES_PUBLISHED_CLONE=1\n");
        printf("SLOT81_WRAPPER_INSTALLED=1\nSLOT89_WRAPPER_INSTALLED=1\n");
        fflush(stdout);
        print_state("COLD_COPY_VERIFIED");
        return 1;
    }
    if ((dr6 & 2UL) != 0UL) {
        if (!g_cold_copy_verified || g_warm_copy_verified ||
            exception_address != g_engine_warm_address ||
            !stop_at_marker(thread, 2UL, g_engine_warm_address,
                            0x10UL,
                            "WARM_COPY_BREAKPOINT") ||
            !verify_tables(2UL, 2))
            return 0;
        g_warm_copy_verified = 1;
        printf("WARM_COPY_WORDS=250\nWARM_COPY_MATCHES_PUBLISHED_CLONE=1\n");
        fflush(stdout);
        print_state("WARM_COPY_VERIFIED");
        return 1;
    }
    if ((dr6 & 4UL) != 0UL) {
        if (!g_cold_copy_verified || g_forwarded_render_verified ||
            exception_address != g_engine_render_address ||
            !stop_at_marker(thread, 4UL, g_engine_render_address,
                            0UL,
                            "SYNTHETIC_CALLBACK_PAIR_BREAKPOINT") ||
            !verify_render())
            return 0;
        g_forwarded_render_verified = 1;
        printf("SYNTHETIC_CALLBACK_PAIR_COMPLETED=1\n");
        printf("HOST_CALLBACK81_CALLS=1\nHOST_CALLBACK89_CALLS=1\n");
        printf("WRAPPER81_COMPLETED=1\nWRAPPER89_COMPLETED=1\n");
        printf("SLOT89_RAW_EAX=0x55667788\nTRACE_EVENTS=4\nTRACE_DROPPED=0\n");
        fflush(stdout);
        print_state("FORWARDING_VERIFIED");
        return 1;
    }
    return 0;
}

static int handle_load_dll(const DEBUG_EVENT *event)
{
    char path[MAX_PATH];
    DWORD base = (DWORD)event->u.LoadDll.lpBaseOfDll;
    if (event->u.LoadDll.hFile == 0 ||
        !final_path_from_file(event->u.LoadDll.hFile, path, sizeof(path)))
        return 0;
    if (_stricmp(path, g_proxy_path) == 0) {
        if (g_proxy_loaded || base == 0UL ||
            !verify_remote_pe(base, POSTENTRY_PROXY_IMAGE_SIZE))
            return 0;
        g_proxy_base = base;
        g_proxy_size = POSTENTRY_PROXY_IMAGE_SIZE;
        g_proxy_loaded = 1;
        printf("PROXY_LOAD_PATH=%s\nPROXY_LOAD_BASE=0x%08lx\n",
               path, (unsigned long)base);
        print_state("PROXY_LOADED");
        return 1;
    }
    if (_stricmp(path, g_engine_path) == 0) {
        if (!g_proxy_loaded || g_engine_loaded || base == 0UL ||
            !verify_remote_pe(base, POSTENTRY_ENGINE_IMAGE_SIZE))
            return 0;
        g_engine_base = base;
        g_engine_size = POSTENTRY_ENGINE_IMAGE_SIZE;
        g_engine_copy_address = base + POSTENTRY_ENGINE_AFTER_COPY_RVA;
        g_engine_warm_address = base + POSTENTRY_ENGINE_AFTER_WARM_RVA;
        g_engine_render_address = base + POSTENTRY_ENGINE_AFTER_RENDER_RVA;
        g_engine_loaded = 1;
        if (!arm_loaded_threads())
            return 0;
        printf("ENGINE_LOAD_PATH=%s\nENGINE_LOAD_BASE=0x%08lx\n",
               path, (unsigned long)base);
        printf("ENGINE_AFTER_COPY=0x%08lx\nENGINE_AFTER_WARM=0x%08lx\nENGINE_AFTER_RENDER=0x%08lx\n",
               (unsigned long)g_engine_copy_address,
               (unsigned long)g_engine_warm_address,
               (unsigned long)g_engine_render_address);
        print_state("ENGINE_LOADED_BREAKPOINTS_ARMED");
        return 1;
    }
    if (_stricmp(strrchr(path, '\\') == 0 ? path :
                 strrchr(path, '\\') + 1, "ddraw.dll") == 0 ||
        _stricmp(strrchr(path, '\\') == 0 ? path :
                 strrchr(path, '\\') + 1, "synthetic_engine.dll") == 0)
        return 0;
    return 1;
}

static int run_synthetic(const char *host_argument,
                         const char *proxy_argument,
                         const char *engine_argument,
                         const char *mode)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION process_info;
    DEBUG_EVENT event;
    char command_line[4 * MAX_PATH];
    char work_directory[MAX_PATH];
    char *slash;
    DWORD start_tick;
    DWORD mode_number;
    int ok = 0;

    if (!full_path(host_argument, g_host_path, sizeof(g_host_path)) ||
        !full_path(proxy_argument, g_proxy_path, sizeof(g_proxy_path)) ||
        !full_path(engine_argument, g_engine_path, sizeof(g_engine_path)))
        return 0;
    if (lstrcmpiA(strrchr(g_host_path, '\\') == 0 ? g_host_path :
                  strrchr(g_host_path, '\\') + 1, "synthetic_host.exe") != 0 ||
        lstrcmpiA(strrchr(g_proxy_path, '\\') == 0 ? g_proxy_path :
                  strrchr(g_proxy_path, '\\') + 1, "ddraw.dll") != 0 ||
        lstrcmpiA(strrchr(g_engine_path, '\\') == 0 ? g_engine_path :
                  strrchr(g_engine_path, '\\') + 1,
                  "synthetic_engine.dll") != 0)
        return 0;
    if (lstrcmpiA(mode, "positive") == 0)
        mode_number = 1UL;
    else if (lstrcmpiA(mode, "coldwarm") == 0)
        mode_number = 2UL;
    else if (lstrcmpiA(mode, "timeout") == 0)
        mode_number = 3UL;
    else if (lstrcmpiA(mode, "invalidsig") == 0)
        mode_number = 4UL;
    else
        return 0;
    g_test_mode = mode_number;

    strcpy(work_directory, g_host_path);
    slash = strrchr(work_directory, '\\');
    if (slash == 0)
        return 0;
    slash[1] = '\0';
    sprintf(command_line, "\"%s\" \"%s\" \"%s\" %s",
            g_host_path, g_proxy_path, g_engine_path, mode);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    memset(&process_info, 0, sizeof(process_info));
    print_state("SYNTHETIC_CONTROLLER_READY");
    if (!CreateProcessA(g_host_path, command_line, 0, 0, FALSE,
                        DEBUG_ONLY_THIS_PROCESS | CREATE_SUSPENDED, 0,
                        work_directory, &startup, &process_info)) {
        printf("CREATE_PROCESS_ERROR=%lu\n", (unsigned long)GetLastError());
        return 0;
    }
    g_process = process_info.hProcess;
    g_initial_thread = process_info.hThread;
    g_process_id = process_info.dwProcessId;
    g_initial_thread_id = process_info.dwThreadId;
    g_process_created = 1;
    if (!enable_kill_on_exit() ||
        !add_thread(g_initial_thread_id, g_initial_thread))
        goto cleanup;
    if (ResumeThread(g_initial_thread) != 1UL)
        goto cleanup;
    start_tick = GetTickCount();

    for (;;) {
        DWORD elapsed = (DWORD)(GetTickCount() - start_tick);
        DWORD timeout = mode_number == 3UL ? 2500UL : POSTENTRY_TIMEOUT_MS;
        if (elapsed >= timeout) {
            g_timeout_observed = 1;
            print_state("CONTROLLER_TIMEOUT");
            break;
        }
        if (!WaitForDebugEvent(&event, 100UL)) {
            if (GetLastError() == WAIT_TIMEOUT_ERROR)
                continue;
            printf("WAIT_FOR_DEBUG_EVENT_ERROR=%lu\n",
                   (unsigned long)GetLastError());
            break;
        }
        g_pending_event = 1;
        g_pending_process_id = event.dwProcessId;
        g_pending_thread_id = event.dwThreadId;

        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            if (g_seen_create_process ||
                event.dwProcessId != g_process_id) {
                close_event_handles(&event);
                break;
            }
            g_host_base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            if (!path_matches_file(event.u.CreateProcessInfo.hFile,
                                   g_host_path) ||
                !verify_remote_pe(g_host_base, POSTENTRY_HOST_IMAGE_SIZE) ||
                !add_thread(event.dwThreadId,
                            event.u.CreateProcessInfo.hThread)) {
                close_event_handles(&event);
                break;
            }
            g_seen_create_process = 1;
            printf("HOST_LOAD_PATH=%s\nHOST_LOAD_BASE=0x%08lx\n",
                   g_host_path, (unsigned long)g_host_base);
            print_state("CHILD_CREATED");
            close_event_handles(&event);
            if (!continue_pending(DBG_CONTINUE))
                break;
            continue;
        }

        if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            if (!add_thread(event.dwThreadId,
                            event.u.CreateThread.hThread)) {
                close_event_handles(&event);
                break;
            }
            close_event_handles(&event);
            if (!continue_pending(DBG_CONTINUE))
                break;
            continue;
        }

        if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            int loaded = handle_load_dll(&event);
            close_event_handles(&event);
            if (!loaded || !continue_pending(DBG_CONTINUE))
                break;
            continue;
        }

        if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            DWORD code = event.u.Exception.ExceptionRecord.ExceptionCode;
            DWORD address = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if (code == EXCEPTION_SINGLE_STEP) {
                PostentryThreadV1 *thread = find_thread(event.dwThreadId);
                CONTEXT context;
                if (thread == 0) {
                    close_event_handles(&event);
                    break;
                }
                memset(&context, 0, sizeof(context));
                context.ContextFlags = CONTEXT_CONTROL | CONTEXT_DEBUG_REGISTERS;
                if (!GetThreadContext(thread->handle, &context) ||
                    context.Eip != address ||
                    !process_marker(event.dwThreadId, address, context.Dr6)) {
                    close_event_handles(&event);
                    break;
                }
                close_event_handles(&event);
                if (g_forwarded_render_verified) {
                    print_state("STOP_AFTER_ONE_SYNTHETIC_CALLBACK_PAIR");
                    ok = terminate_and_reap(POSTENTRY_CHILD_TIMEOUT_EXIT);
                    break;
                }
                if (!continue_pending(DBG_CONTINUE))
                    break;
                continue;
            }
            if (code == EXCEPTION_BREAKPOINT) {
                close_event_handles(&event);
                if (!continue_pending(DBG_CONTINUE))
                    break;
                continue;
            }
            printf("UNEXPECTED_EXCEPTION=0x%08lx ADDRESS=0x%08lx\n",
                   (unsigned long)code, (unsigned long)address);
            close_event_handles(&event);
            break;
        }

        if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            int valid_exit = 0;
            if (mode_number == 4UL)
                valid_exit = read_host_on_exit(
                    1, event.u.ExitProcess.dwExitCode);
            close_event_handles(&event);
            if (!continue_pending(DBG_CONTINUE))
                break;
            g_exit_event_drained = 1;
            if (valid_exit &&
                WaitForSingleObject(g_process, 1000UL) == WAIT_OBJECT_0) {
                printf("CHILD_TERMINATED=1\nEXIT_PROCESS_EVENT_DRAINED=1\n");
                printf("KILL_ON_EXIT_MODE=%d\n", g_kill_on_exit_mode);
                print_state("INVALID_SIGNATURE_REAPED");
                ok = 1;
            }
            break;
        }

        close_event_handles(&event);
        if (!continue_pending(DBG_CONTINUE))
            break;
    }

    if (mode_number == 3UL && g_timeout_observed &&
        g_cold_copy_verified && !g_forwarded_render_verified) {
        printf("TIMEOUT_AFTER_COLD_COPY=1\n");
        ok = terminate_and_reap(POSTENTRY_CHILD_TIMEOUT_EXIT);
        if (ok)
            print_state("TIMEOUT_CLEANUP_VERIFIED");
    } else if (!ok && g_process_created &&
               WaitForSingleObject(g_process, 0UL) != WAIT_OBJECT_0) {
        terminate_and_reap(POSTENTRY_CHILD_TIMEOUT_EXIT);
    }

cleanup:
    if (g_process_created && g_process != 0 &&
        WaitForSingleObject(g_process, 0UL) != WAIT_OBJECT_0)
        terminate_and_reap(POSTENTRY_CHILD_TIMEOUT_EXIT);
    if (g_process != 0)
        CloseHandle(g_process);
    if (g_initial_thread != 0)
        CloseHandle(g_initial_thread);
    clear_thread_handles();
    g_process = 0;
    g_initial_thread = 0;
    return ok;
}

int main(int argc, char **argv)
{
    int result;
    if (argc >= 2 && lstrcmpiA(argv[1], "--original") == 0) {
        print_state("ORIGINAL_GATE_BLOCKED");
        printf("ORIGINAL_TARGET=NOTRUN\n");
        printf("REGISTRY_PROFILE_ISOLATION=UNPROVEN\n");
        printf("WINDOWED_STARTUP_AND_MODE_RESTORE=UNPROVEN\n");
        printf("ROOT_AUTHORIZATION=REQUIRED\n");
        printf("ORIGINAL_LAUNCH_CODE=DISABLED\n");
        printf("RESULT=BLOCKED\n");
        return 3;
    }
    if (argc != 6 || lstrcmpiA(argv[1], "--synthetic") != 0) {
        printf("USAGE=postentry_controller --synthetic host.exe ddraw.dll synthetic_engine.dll positive|invalidsig|coldwarm|timeout\n");
        return 2;
    }
    result = run_synthetic(argv[2], argv[3], argv[4], argv[5]);
    if (result) {
        printf("RESULT=PASS\n");
        return 0;
    }
    printf("RESULT=FAIL\n");
    return 1;
}
