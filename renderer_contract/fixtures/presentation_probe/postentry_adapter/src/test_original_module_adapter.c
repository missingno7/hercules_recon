#include <stdio.h>
#include <string.h>

#include "original_module_adapter.h"

static int g_checks;
static int g_failures;

static DWORD WINAPI held_thread(LPVOID argument)
{
    (void)argument;
    Sleep(INFINITE);
    return 0UL;
}

static void check(int condition, const char *name)
{
    ++g_checks;
    if (condition)
        printf("PASS %s\n", name);
    else {
        ++g_failures;
        printf("FAIL %s\n", name);
    }
}

static void make_table(DWORD *table, DWORD slot89)
{
    DWORD index;
    for (index = 0; index < OMA_TABLE_WORDS; ++index)
        table[index] = 0x67000000UL + index;
    table[0] = OMA_TABLE_SIGNATURE;
    table[OMA_SLOT89] = slot89;
}

static void make_copy_context(CONTEXT *context,
                              const OmaModuleProfileV1 *profile,
                              DWORD host_base, DWORD module_base,
                              DWORD source_base)
{
    memset(context, 0, sizeof(*context));
    context->Eip = module_base + profile->copy_post_eip_rva;
    context->Esi = source_base + OMA_TABLE_BYTES;
    context->Edi = module_base + profile->local_table_rva + OMA_TABLE_BYTES;
    context->Ecx = 0UL;
    context->EFlags = 0x202UL;
    context->Dr0 = context->Eip;
    context->Dr6 = 1UL;
    context->Dr7 = 1UL;
    (void)host_base;
}

static int test_one_profile(const OmaModuleProfileV1 *profile)
{
    DWORD host_base = 0x00400000UL;
    DWORD module_base = 0x12000000UL;
    DWORD clone_base = 0x50000000UL;
    DWORD clone_size = 0x00010000UL;
    DWORD source_base = clone_base + 0x1000UL;
    DWORD callback_target = clone_base + 0x2000UL;
    DWORD local_address = module_base + profile->local_table_rva;
    DWORD host_pointer_cell_value = source_base;
    DWORD original_source_address = host_base + 0x361b8UL;
    DWORD source_table[OMA_TABLE_WORDS];
    DWORD local_table[OMA_TABLE_WORDS];
    OmaTableSnapshotV1 snapshot;
    CONTEXT context;
    CONTEXT threads[3];
    HANDLE os_threads[3];
    OmaProbeStateV1 probe_state;
    DWORD raw_eax = 0UL;
    DWORD index;
    int ok;

    make_table(source_table, callback_target);
    memcpy(local_table, source_table, sizeof(local_table));

    {
        DWORD captured_source = (DWORD)source_table;
        DWORD captured_local = (DWORD)local_table;
        DWORD captured_host_base = (DWORD)&captured_source -
            OMA_HOST_POINTER_CELL_RVA;
        DWORD captured_module_base = captured_local -
            profile->local_table_rva;
        CONTEXT captured_context;
        int captured = OmaCaptureStoppedCopyTables(
            GetCurrentProcess(), captured_host_base, captured_module_base,
            profile, &snapshot);
        check(captured,
              "profile-bound stopped-memory reader captures pointer cell and both tables");
        make_copy_context(&captured_context, profile, captured_host_base,
                          captured_module_base, captured_source);
        check(captured && OmaValidateCopyStop(
                  &captured_context, captured_host_base, captured_module_base,
                  captured_source, OMA_TABLE_BYTES, captured_source,
                  profile,
                  snapshot.source_table, snapshot.local_table,
                  callback_target),
              "synthetic ReadProcessMemory snapshot passes full-copy validation");
    }

    make_copy_context(&context, profile, host_base, module_base,
                      source_base);
    ok = OmaValidateCopyStop(&context, host_base, module_base,
                             clone_base, clone_size,
                             host_pointer_cell_value,
                             profile,
                             source_table, local_table, callback_target);
    check(ok, "all 250 source and local words validate at exact post-copy EIP");

    local_table[249] ^= 1UL;
    check(!OmaValidateCopyStop(&context, host_base, module_base,
                               clone_base, clone_size,
                               host_pointer_cell_value,
                               profile,
                               source_table, local_table, callback_target),
          "copy mismatch at word 249 is rejected");
    local_table[249] ^= 1UL;

    context.Eip += 1UL;
    check(!OmaValidateCopyStop(&context, host_base, module_base,
                               clone_base, clone_size,
                               host_pointer_cell_value,
                               profile,
                               source_table, local_table, callback_target),
          "post-copy EIP off by one is rejected");
    context.Eip -= 1UL;

    context.Edi += 4UL;
    check(!OmaValidateCopyStop(&context, host_base, module_base,
                               clone_base, clone_size,
                               host_pointer_cell_value,
                               profile,
                               source_table, local_table, callback_target),
          "wrong local-table destination is rejected");
    context.Edi -= 4UL;

    context.Esi += 4UL;
    check(!OmaValidateCopyStop(&context, host_base, module_base,
                               clone_base, clone_size,
                               host_pointer_cell_value,
                               profile,
                               source_table, local_table, callback_target),
          "wrong source-table pointer is rejected");
    context.Esi -= 4UL;

    check(!OmaValidateCopyStop(&context, host_base, module_base,
                               clone_base, clone_size,
                               original_source_address,
                               profile,
                               source_table, local_table, callback_target),
          "uncloned original host source is rejected");

    check(OmaConfigureCopyBreakpoint(&context, module_base, profile) &&
          context.Dr0 == module_base + profile->copy_post_eip_rva &&
          context.Dr7 == 1UL && context.Dr1 == 0UL &&
          context.Dr2 == 0UL && context.Dr6 == 0UL,
          "per-thread copy execution breakpoint has exact DR0 state");

    for (index = 0; index < 3UL; ++index) {
        memset(&threads[index], 0, sizeof(threads[index]));
        threads[index].Dr7 = 0x5a5a5a5aUL;
        check(OmaConfigurePostCopyWatchpoints(&threads[index], module_base,
                                              local_address, profile) &&
              OmaValidatePostCopyWatchpoints(&threads[index], module_base,
                                             local_address, profile),
              "each extant thread gets slot89 write and wrapper-return BPs");
    }
    threads[1].Dr1 += 4UL;
    check(!OmaValidatePostCopyWatchpoints(&threads[1], module_base,
                                         local_address, profile),
          "thread with a wrong slot89 watch address is rejected");
    threads[1].Dr1 -= 4UL;

    for (index = 0; index < 3UL; ++index)
        os_threads[index] = CreateThread(0, 0, held_thread, 0,
                                          CREATE_SUSPENDED, 0);
    ok = 1;
    for (index = 0; index < 3UL; ++index) {
        CONTEXT observed;
        if (os_threads[index] == 0 ||
            !OmaArmThreadCopyBreakpoint(os_threads[index], module_base,
                                        profile)) {
            ok = 0;
            continue;
        }
        memset(&observed, 0, sizeof(observed));
        observed.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(os_threads[index], &observed) ||
            observed.Dr0 != module_base + profile->copy_post_eip_rva ||
            (observed.Dr7 & 0x0fff00ffUL) != 1UL ||
            !OmaArmThreadPostCopyWatchpoints(os_threads[index], module_base,
                                             local_address, profile)) {
            ok = 0;
            continue;
        }
        memset(&observed, 0, sizeof(observed));
        observed.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(os_threads[index], &observed) ||
            !OmaValidatePostCopyWatchpoints(&observed, module_base,
                                            local_address, profile))
            ok = 0;
    }
    check(ok, "Win32 thread API arms and verifies all per-thread DR states");
    for (index = 0; index < 3UL; ++index) {
        if (os_threads[index] != 0) {
            TerminateThread(os_threads[index], 0UL);
            WaitForSingleObject(os_threads[index], 1000UL);
            CloseHandle(os_threads[index]);
        }
    }

    context = threads[0];
    context.Dr6 = 2UL;
    context.Eip = module_base + profile->submit_wrapper_rva + 1UL;
    check(OmaClassifyPostCopyStop(&context, module_base, local_address,
                                  profile, callback_target, callback_target,
                                  0, 0UL, &raw_eax) ==
              OMA_STOP_SLOT89_WRITE_AFTER_COPY,
          "slot89 data write watch is a stop even when the value is unchanged");

    context.Dr6 = 4UL;
    context.Eip = module_base + profile->submit_callee_return_rva;
    context.Eax = 0xdeadbeefUL;
    check(OmaClassifyPostCopyStop(&context, module_base, local_address,
                                  profile, callback_target, callback_target,
                                  0, 0UL, &raw_eax) ==
              OMA_STOP_SLOT89_CALLEE_RETURNED &&
          raw_eax == 0xdeadbeefUL,
          "exact callee-return EIP records physical EAX before wrapper cleanup");

    context.Dr6 = 8UL;
    context.Eip = module_base + profile->submit_caller_return_rva;
    check(OmaClassifyPostCopyStop(&context, module_base, local_address,
                                  profile, callback_target, callback_target,
                                  0, 0UL, &raw_eax) == OMA_STOP_INVALID,
          "caller-return stop before observed callee return is rejected");
    check(OmaClassifyPostCopyStop(&context, module_base, local_address,
                                  profile, callback_target, callback_target,
                                  1, 0xdeadbeefUL, &raw_eax) ==
              OMA_STOP_SUBMIT_WRAPPER_RETURNED &&
          raw_eax == 0xdeadbeefUL,
          "caller return after callee return validates wrapper completion");

    context.Eip += 1UL;
    check(OmaClassifyPostCopyStop(&context, module_base, local_address,
                                  profile, callback_target, callback_target,
                                  1, 0xdeadbeefUL, &raw_eax) ==
              OMA_STOP_INVALID,
          "wrong wrapper return EIP is rejected");
    context.Eip -= 1UL;
    check(OmaClassifyPostCopyStop(&context, module_base, local_address,
                                  profile, callback_target,
                                  callback_target ^ 1UL, 1,
                                  0xdeadbeefUL, &raw_eax) ==
              OMA_STOP_INVALID,
          "slot89 change before wrapper return is rejected");
    context.Eax ^= 1UL;
    check(OmaClassifyPostCopyStop(&context, module_base, local_address,
                                  profile, callback_target, callback_target,
                                  1, 0xdeadbeefUL, &raw_eax) ==
              OMA_STOP_INVALID,
          "wrapper completion that changes physical EAX is rejected");
    context.Eax ^= 1UL;

    context.Dr6 = 0UL;
    check(OmaClassifyPostCopyStop(&context, module_base, local_address,
                                  profile, callback_target, callback_target,
                                  1, 0xdeadbeefUL, &raw_eax) ==
              OMA_STOP_INVALID,
          "unattributed stop without a hardware breakpoint is rejected");

    memset(&probe_state, 0, sizeof(probe_state));
    memset(&context, 0, sizeof(context));
    context.EFlags = 0x202UL;
    check(OmaStartCopyObservation(&context, module_base, profile,
                                  &probe_state) &&
          probe_state.phase == OMA_PHASE_COPY_ARMED &&
          context.Dr0 == module_base + profile->copy_post_eip_rva &&
          context.Dr1 == 0UL && context.Dr2 == 0UL && context.Dr3 == 0UL,
          "state machine starts execute-aftercopy only; slot89 is not watched mid-REP");
    make_copy_context(&context, profile, host_base, module_base, source_base);
    check(OmaAdvanceAfterValidatedCopy(
              &context, module_base, host_base, clone_base, clone_size,
              host_pointer_cell_value, local_address, profile, source_table,
              local_table, callback_target, &probe_state) &&
          probe_state.phase == OMA_PHASE_POSTCOPY_ARMED &&
          probe_state.slot89_value_at_copy == callback_target &&
          OmaValidatePostCopyWatchpoints(&context, module_base,
                                         local_address, profile),
          "post-copy phase arms slot89/clobber, callee, and caller stops only after the full 250-word snapshot passes");

    context.Dr6 = 8UL;
    context.Eip = module_base + profile->submit_caller_return_rva;
    check(OmaObservePostCopyEvent(&context, module_base, local_address,
                                  profile, callback_target, &probe_state) ==
              OMA_STOP_INVALID &&
          probe_state.phase == OMA_PHASE_POSTCOPY_ARMED,
          "caller return cannot be relabeled complete before a callee-return event");
    context.Dr6 = 4UL;
    context.Eip = module_base + profile->submit_callee_return_rva;
    context.Eax = 0x13579bdfUL;
    check(OmaObservePostCopyEvent(&context, module_base, local_address,
                                  profile, callback_target, &probe_state) ==
              OMA_STOP_SLOT89_CALLEE_RETURNED &&
          probe_state.phase == OMA_PHASE_CALLEE_RETURN_OBSERVED &&
          probe_state.callee_raw_eax == 0x13579bdfUL,
          "callee return is explicitly recorded as before wrapper cleanup/RET with raw EAX saved");
    check(OmaAcknowledgePostCopyEvent(&context,
                                      OMA_STOP_SLOT89_CALLEE_RETURNED) &&
          context.Dr6 == 0UL,
          "accepted DR2 event clears sticky DR6 before wrapper continuation");
    context.Dr6 = 8UL;
    context.Eip = module_base + profile->submit_caller_return_rva;
    check(OmaObservePostCopyEvent(&context, module_base, local_address,
                                  profile, callback_target, &probe_state) ==
              OMA_STOP_SUBMIT_WRAPPER_RETURNED &&
          probe_state.phase == OMA_PHASE_WRAPPER_COMPLETED,
          "only the caller-return stop after callee return marks wrapper complete");
    check(OmaAcknowledgePostCopyEvent(&context,
                                      OMA_STOP_SUBMIT_WRAPPER_RETURNED) &&
          context.Dr6 == 0UL,
          "accepted DR3 event clears sticky DR6 before controller resumes or reaps");
    check(OmaObservePostCopyEvent(&context, module_base, local_address,
                                  profile, callback_target, &probe_state) ==
              OMA_STOP_INVALID,
          "completed wrapper state is terminal and cannot count a second completion");

    memset(&probe_state, 0, sizeof(probe_state));
    memset(&context, 0, sizeof(context));
    context.EFlags = 0x202UL;
    OmaStartCopyObservation(&context, module_base, profile, &probe_state);
    make_copy_context(&context, profile, host_base, module_base, source_base);
    check(OmaAdvanceAfterValidatedCopy(
              &context, module_base, host_base, clone_base, clone_size,
              host_pointer_cell_value, local_address, profile, source_table,
              local_table, callback_target, &probe_state),
          "second chronology fixture reaches post-copy armed state");
    context.Dr6 = 2UL;
    context.Eip = module_base + profile->submit_wrapper_rva + 1UL;
    check(OmaObservePostCopyEvent(&context, module_base, local_address,
                                  profile, callback_target, &probe_state) ==
              OMA_STOP_SLOT89_WRITE_AFTER_COPY &&
          probe_state.phase == OMA_PHASE_SLOT89_CLOBBERED,
          "a post-copy slot89 write is a terminal clobber event regardless of observed value");
    context.Dr6 = 4UL;
    context.Eip = module_base + profile->submit_callee_return_rva;
    check(OmaObservePostCopyEvent(&context, module_base, local_address,
                                  profile, callback_target, &probe_state) ==
              OMA_STOP_INVALID,
          "slot89 clobber chronology cannot later be promoted to a successful callback");
    return g_failures == 0;
}

int main(void)
{
    const OmaModuleProfileV1 *profile;
    DWORD source_table[OMA_TABLE_WORDS];
    DWORD local_table[OMA_TABLE_WORDS];
    DWORD host_base = 0x00400000UL;
    DWORD module_base = 0x12000000UL;
    DWORD clone_base = 0x50000000UL;
    DWORD source_base = clone_base + 0x1000UL;
    DWORD callback_target = clone_base + 0x2000UL;
    CONTEXT context;
    int ok = 1;

    check(OmaValidateImageIdentity(OMA_HOST_RETAIL_SHA256, "ENG1.DLL",
          "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
          &profile) && profile != 0 && profile->local_table_rva == 0x70f20UL,
          "retail host and pinned ENG1 identity accepted");
    check(OmaValidateImageIdentity(OMA_HOST_NOCD_SHA256, "ENG3.DLL",
          "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
          &profile) && profile != 0 && profile->local_table_rva == 0x407a0UL,
          "no-CD host and pinned ENG3 identity accepted");
    check(OmaValidateImageIdentity(OMA_HOST_RETAIL_SHA256, "TITLE.DLL",
          "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
          &profile) && profile != 0 && profile->local_table_rva == 0x2bb40UL,
          "retail host and pinned TITLE identity accepted");
    check(!OmaValidateImageIdentity("0000000000000000000000000000000000000000000000000000000000000000",
          "ENG1.DLL",
          "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
          &profile) && profile == 0,
          "unrecognized retail/no-CD host identity rejected");
    check(!OmaValidateImageIdentity(OMA_HOST_RETAIL_SHA256, "ENG1.DLL",
          "0000000000000000000000000000000000000000000000000000000000000000",
          &profile) && profile == 0,
          "module with wrong full-file hash rejected");
    check(!OmaValidateImageIdentity(OMA_HOST_RETAIL_SHA256, "ENG2.DLL",
          "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
          &profile) && profile == 0,
          "unexpected module basename rejected");

    profile = OmaFindProfile("eng1.dll");
    check(profile != 0 && profile->copy_instruction_rva == 0x2c19fUL &&
          profile->copy_post_eip_rva == 0x2c1a1UL &&
          profile->submit_callee_return_rva == 0x2c62bUL &&
          profile->submit_caller_call_rva == 0x20616UL &&
          profile->submit_caller_return_rva == 0x2061bUL,
          "ENG1 exact cold-copy and wrapper-return RVAs pinned");
    profile = OmaFindProfile("ENG3.DLL");
    check(profile != 0 && profile->copy_instruction_rva == 0x1f84fUL &&
          profile->copy_post_eip_rva == 0x1f851UL &&
          profile->submit_callee_return_rva == 0x1fc3bUL &&
          profile->submit_caller_call_rva == 0x1aeafUL &&
          profile->submit_caller_return_rva == 0x1aeb4UL,
          "ENG3 exact cold-copy and wrapper-return RVAs pinned");
    profile = OmaFindProfile("TITLE.DLL");
    check(profile != 0 && profile->copy_instruction_rva == 0xc20fUL &&
          profile->copy_post_eip_rva == 0xc211UL &&
          profile->submit_callee_return_rva == 0xc6bbUL &&
          profile->submit_caller_call_rva == 0x6312UL &&
          profile->submit_caller_return_rva == 0x6317UL,
          "TITLE exact cold-copy and wrapper-return RVAs pinned");

    profile = OmaFindProfile("ENG1.DLL");
    make_table(source_table, callback_target);
    memcpy(local_table, source_table, sizeof(local_table));
    make_copy_context(&context, profile, host_base, module_base, source_base);
    context.Dr7 = 0UL;
    check(!OmaValidateCopyStop(&context, host_base, module_base,
                               clone_base, 0x10000UL, source_base,
                               profile,
                               source_table, local_table, callback_target),
          "wrong breakpoint event identity is rejected");

    profile = OmaFindProfile("ENG1.DLL");
    test_one_profile(profile);
    profile = OmaFindProfile("ENG3.DLL");
    test_one_profile(profile);
    profile = OmaFindProfile("TITLE.DLL");
    test_one_profile(profile);

    printf("checks=%d failures=%d\n", g_checks, g_failures);
    if (g_failures != 0) {
        printf("ORIGINAL_TARGET=NOTRUN\nRESULT=FAIL\n");
        return 1;
    }
    printf("ORIGINAL_TARGET=NOTRUN\nRESULT=PASS\n");
    return 0;
}
