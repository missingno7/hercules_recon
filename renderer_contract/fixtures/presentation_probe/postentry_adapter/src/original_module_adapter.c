#include <string.h>

#include "original_module_adapter.h"

#define OMA_COPY_BP_DR7 0x00000001UL
#define OMA_MONITOR_BP_DR7 0x00d00054UL
#define OMA_DR7_MONITOR_MASK 0xffff00ffUL
#define OMA_DR7_COPY_MASK 0xffff00ffUL

static const OmaModuleProfileV1 g_profiles[] = {
    { "ENG1.DLL",
      "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
      0x10000000UL, 0x00070f20UL, 0x0002c19fUL, 0x0002c1a1UL,
      0x0002c620UL, 0x0002c62bUL, 0x00020616UL, 0x0002061bUL,
      "56974b1cae70836a100d47ef3cde2793206aaae329bd71894283ed9dbbfbc1bb" },
    { "ENG3.DLL",
      "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
      0x10000000UL, 0x000407a0UL, 0x0001f84fUL, 0x0001f851UL,
      0x0001fc30UL, 0x0001fc3bUL, 0x0001aeafUL, 0x0001aeb4UL,
      "60df4fbb3ef8da7836d7cc270fae184e9ec6da51fddc6eeffb1f8955f304214e" },
    { "TITLE.DLL",
      "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
      0x10000000UL, 0x0002bb40UL, 0x0000c20fUL, 0x0000c211UL,
      0x0000c6b0UL, 0x0000c6bbUL, 0x00006312UL, 0x00006317UL,
      "aaf98bd8e9b93be18c2442d2c7c392a4b9fa05f16d8a6882aa5fb7585dde7b7a" }
};

static int address_add(DWORD base, DWORD offset, DWORD *result)
{
    if (result == 0 || base > 0xffffffffUL - offset)
        return 0;
    *result = base + offset;
    return 1;
}

static int address_in_range(DWORD address, DWORD range_base,
                            DWORD range_size, DWORD length)
{
    DWORD range_end;
    DWORD address_end;
    if (!address_add(range_base, range_size, &range_end) ||
        !address_add(address, length, &address_end))
        return 0;
    return address >= range_base && address_end <= range_end;
}

const OmaModuleProfileV1 *OmaFindProfile(const char *module_name)
{
    DWORD index;
    if (module_name == 0)
        return 0;
    for (index = 0; index < sizeof(g_profiles) / sizeof(g_profiles[0]);
         ++index) {
        if (_stricmp(module_name, g_profiles[index].module_name) == 0)
            return &g_profiles[index];
    }
    return 0;
}

int OmaStartCopyObservation(CONTEXT *context, DWORD module_base,
                            const OmaModuleProfileV1 *profile,
                            OmaProbeStateV1 *state)
{
    if (context == 0 || profile == 0 || state == 0 ||
        state->phase != OMA_PHASE_INVALID)
        return 0;
    if (!OmaConfigureCopyBreakpoint(context, module_base, profile))
        return 0;
    state->phase = OMA_PHASE_COPY_ARMED;
    state->slot89_value_at_copy = 0UL;
    state->callee_raw_eax = 0UL;
    return 1;
}

int OmaAdvanceAfterValidatedCopy(CONTEXT *context, DWORD module_base,
                                 DWORD host_base, DWORD clone_base,
                                 DWORD clone_size,
                                 DWORD pointer_cell_value,
                                 DWORD local_table_address,
                                 const OmaModuleProfileV1 *profile,
                                 const DWORD *source_table,
                                 const DWORD *local_table,
                                 DWORD expected_slot89_target,
                                 OmaProbeStateV1 *state)
{
    DWORD expected_local;
    if (context == 0 || profile == 0 || source_table == 0 ||
        local_table == 0 || state == 0 ||
        state->phase != OMA_PHASE_COPY_ARMED ||
        !address_add(module_base, profile->local_table_rva,
                     &expected_local) ||
        local_table_address != expected_local ||
        !OmaValidateCopyStop(context, host_base, module_base, clone_base,
                             clone_size, pointer_cell_value, profile,
                             source_table, local_table,
                             expected_slot89_target))
        return 0;
    if (!OmaConfigurePostCopyWatchpoints(context, module_base,
                                         local_table_address, profile))
        return 0;
    state->slot89_value_at_copy = local_table[OMA_SLOT89];
    state->callee_raw_eax = 0UL;
    state->phase = OMA_PHASE_POSTCOPY_ARMED;
    return 1;
}

OmaStopResultV1 OmaObservePostCopyEvent(
    const CONTEXT *context, DWORD module_base, DWORD local_table_address,
    const OmaModuleProfileV1 *profile, DWORD slot89_value_now,
    OmaProbeStateV1 *state)
{
    DWORD raw_eax = 0UL;
    int callee_return_seen;
    OmaStopResultV1 result;
    if (state == 0 ||
        (state->phase != OMA_PHASE_POSTCOPY_ARMED &&
         state->phase != OMA_PHASE_CALLEE_RETURN_OBSERVED))
        return OMA_STOP_INVALID;
    callee_return_seen =
        state->phase == OMA_PHASE_CALLEE_RETURN_OBSERVED;
    result = OmaClassifyPostCopyStop(
        context, module_base, local_table_address, profile,
        state->slot89_value_at_copy, slot89_value_now,
        callee_return_seen, state->callee_raw_eax, &raw_eax);
    if (result == OMA_STOP_SLOT89_WRITE_AFTER_COPY) {
        state->phase = OMA_PHASE_SLOT89_CLOBBERED;
        return result;
    }
    if (result == OMA_STOP_SLOT89_CALLEE_RETURNED) {
        state->callee_raw_eax = raw_eax;
        state->phase = OMA_PHASE_CALLEE_RETURN_OBSERVED;
        return result;
    }
    if (result == OMA_STOP_SUBMIT_WRAPPER_RETURNED) {
        state->phase = OMA_PHASE_WRAPPER_COMPLETED;
        return result;
    }
    return OMA_STOP_INVALID;
}

int OmaAcknowledgePostCopyEvent(CONTEXT *context,
                                OmaStopResultV1 observed_result)
{
    if (context == 0 || observed_result == OMA_STOP_INVALID)
        return 0;
    context->Dr6 = 0UL;
    return 1;
}

int OmaStartSyntheticCopyObservation(CONTEXT *context, DWORD copy_eip,
                                     OmaProbeStateV1 *state)
{
    if (context == 0 || state == 0 || copy_eip == 0UL ||
        state->phase != OMA_PHASE_INVALID)
        return 0;
    context->Dr0 = copy_eip;
    context->Dr1 = 0UL;
    context->Dr2 = 0UL;
    context->Dr3 = 0UL;
    context->Dr6 = 0UL;
    context->Dr7 = (context->Dr7 & ~OMA_DR7_COPY_MASK) | 1UL;
    state->phase = OMA_PHASE_COPY_ARMED;
    state->slot89_value_at_copy = 0UL;
    state->callee_raw_eax = 0UL;
    return 1;
}

int OmaMarkSyntheticCopyValidated(const CONTEXT *context, DWORD module_base,
                                  DWORD copy_post_eip_rva,
                                  const DWORD *published_clone,
                                  const DWORD *local_table,
                                  DWORD expected_slot89_target,
                                  OmaProbeStateV1 *state)
{
    DWORD expected_eip;
    DWORD index;
    if (context == 0 || published_clone == 0 || local_table == 0 ||
        state == 0 || state->phase != OMA_PHASE_COPY_ARMED ||
        expected_slot89_target == 0UL ||
        !address_add(module_base, copy_post_eip_rva, &expected_eip) ||
        context->Eip != expected_eip || (context->Dr6 & 0x0fUL) != 1UL ||
        context->Dr0 != expected_eip ||
        (context->Dr7 & OMA_DR7_COPY_MASK) != 1UL ||
        published_clone[0] != OMA_TABLE_SIGNATURE ||
        published_clone[OMA_SLOT89] != expected_slot89_target ||
        local_table[OMA_SLOT89] != expected_slot89_target)
        return 0;
    for (index = 0; index < OMA_TABLE_WORDS; ++index) {
        if (published_clone[index] != local_table[index])
            return 0;
    }
    state->slot89_value_at_copy = local_table[OMA_SLOT89];
    state->callee_raw_eax = 0UL;
    state->phase = OMA_PHASE_COPY_VALIDATED;
    return 1;
}

int OmaArmSyntheticPostCopy(CONTEXT *context, DWORD local_slot89_address,
                            DWORD first_trace_commit_address,
                            DWORD chain_caller_return_address,
                            DWORD submit_caller_return_address,
                            OmaProbeStateV1 *slot81_state,
                            OmaProbeStateV1 *slot89_state)
{
    if (context == 0 || slot81_state == 0 || slot89_state == 0 ||
        slot81_state == slot89_state ||
        slot81_state->phase != OMA_PHASE_COPY_VALIDATED ||
        slot89_state->phase != OMA_PHASE_COPY_VALIDATED ||
        (local_slot89_address & 3UL) != 0UL ||
        (first_trace_commit_address & 3UL) != 0UL ||
        local_slot89_address == 0UL || first_trace_commit_address == 0UL ||
        chain_caller_return_address == 0UL ||
        submit_caller_return_address == 0UL ||
        !OmaConfigureSyntheticPostCopyContext(
            context, local_slot89_address, first_trace_commit_address,
            chain_caller_return_address, submit_caller_return_address))
        return 0;
    slot81_state->phase = OMA_PHASE_POSTCOPY_ARMED;
    slot89_state->phase = OMA_PHASE_POSTCOPY_ARMED;
    return 1;
}

int OmaConfigureSyntheticPostCopyContext(
    CONTEXT *context, DWORD local_slot89_address,
    DWORD trace_commit_address, DWORD chain_caller_return_address,
    DWORD submit_caller_return_address)
{
    if (context == 0 || (local_slot89_address & 3UL) != 0UL ||
        (trace_commit_address & 3UL) != 0UL ||
        local_slot89_address == 0UL || trace_commit_address == 0UL ||
        chain_caller_return_address == 0UL ||
        submit_caller_return_address == 0UL)
        return 0;
    context->Dr0 = chain_caller_return_address;
    context->Dr1 = local_slot89_address;
    context->Dr2 = trace_commit_address;
    context->Dr3 = submit_caller_return_address;
    context->Dr6 = 0UL;
    context->Dr7 = (context->Dr7 & ~OMA_DR7_MONITOR_MASK) |
                   0x0dd00055UL;
    return 1;
}

int OmaMoveSyntheticTraceWatchpoint(CONTEXT *context,
                                    DWORD trace_commit_address)
{
    if (context == 0 || trace_commit_address == 0UL ||
        (trace_commit_address & 3UL) != 0UL)
        return 0;
    context->Dr2 = trace_commit_address;
    context->Dr6 = 0UL;
    return 1;
}

OmaStopResultV1 OmaObserveSyntheticTraceCommit(
    const CONTEXT *context, DWORD trace_commit_address,
    DWORD slot89_value_now, DWORD raw_eax, int is_return_event,
    OmaProbeStateV1 *state)
{
    if (context == 0 || state == 0 ||
        state->phase != OMA_PHASE_POSTCOPY_ARMED ||
        context->Dr2 != trace_commit_address ||
        (context->Dr6 & 0x0fUL) != 4UL ||
        slot89_value_now != state->slot89_value_at_copy)
        return OMA_STOP_INVALID;
    if (!is_return_event)
        return OMA_STOP_TRACE_ENTRY_OBSERVED;
    state->callee_raw_eax = raw_eax;
    state->phase = OMA_PHASE_CALLEE_RETURN_OBSERVED;
    return OMA_STOP_SLOT89_CALLEE_RETURNED;
}

OmaStopResultV1 OmaObserveSyntheticCallerReturn(
    const CONTEXT *context, DWORD caller_return_address, DWORD dr_bit,
    DWORD slot89_value_now, OmaProbeStateV1 *state)
{
    if (context == 0 || state == 0 ||
        state->phase != OMA_PHASE_CALLEE_RETURN_OBSERVED ||
        context->Eip != caller_return_address ||
        (context->Dr6 & 0x0fUL) != dr_bit ||
        slot89_value_now != state->slot89_value_at_copy ||
        (dr_bit != 1UL && dr_bit != 8UL))
        return OMA_STOP_INVALID;
    state->phase = OMA_PHASE_WRAPPER_COMPLETED;
    return OMA_STOP_SUBMIT_WRAPPER_RETURNED;
}

int OmaValidateImageIdentity(const char *host_sha256,
                             const char *module_name,
                             const char *module_sha256,
                             const OmaModuleProfileV1 **profile_out)
{
    const OmaModuleProfileV1 *profile;
    int host_matches;
    if (profile_out != 0)
        *profile_out = 0;
    if (host_sha256 == 0 || module_name == 0 || module_sha256 == 0)
        return 0;
    host_matches = _stricmp(host_sha256, OMA_HOST_RETAIL_SHA256) == 0 ||
                   _stricmp(host_sha256, OMA_HOST_NOCD_SHA256) == 0;
    if (!host_matches)
        return 0;
    profile = OmaFindProfile(module_name);
    if (profile == 0 ||
        _stricmp(module_sha256, profile->module_sha256) != 0)
        return 0;
    if (profile_out != 0)
        *profile_out = profile;
    return 1;
}

int OmaReadStoppedTables(HANDLE process, DWORD pointer_cell_address,
                         DWORD local_table_address,
                         OmaTableSnapshotV1 *snapshot)
{
    DWORD bytes_read = 0UL;
    DWORD source_address = 0UL;
    if (process == 0 || snapshot == 0 || pointer_cell_address == 0UL ||
        local_table_address == 0UL ||
        !ReadProcessMemory(process, (LPCVOID)pointer_cell_address,
                           &source_address, sizeof(source_address),
                           &bytes_read) || bytes_read != sizeof(source_address))
        return 0;
    if (!address_in_range(source_address, source_address,
                          OMA_TABLE_BYTES, OMA_TABLE_BYTES) ||
        !ReadProcessMemory(process, (LPCVOID)source_address,
                           snapshot->source_table, OMA_TABLE_BYTES,
                           &bytes_read) || bytes_read != OMA_TABLE_BYTES)
        return 0;
    if (!ReadProcessMemory(process, (LPCVOID)local_table_address,
                          snapshot->local_table, OMA_TABLE_BYTES,
                          &bytes_read) || bytes_read != OMA_TABLE_BYTES)
        return 0;
    snapshot->source_address = source_address;
    return 1;
}

int OmaCaptureStoppedCopyTables(HANDLE process, DWORD host_base,
                                DWORD module_base,
                                const OmaModuleProfileV1 *profile,
                                OmaTableSnapshotV1 *snapshot)
{
    DWORD pointer_cell_address;
    DWORD local_table_address;
    if (profile == 0 ||
        !address_add(host_base, OMA_HOST_POINTER_CELL_RVA,
                     &pointer_cell_address) ||
        !address_add(module_base, profile->local_table_rva,
                     &local_table_address))
        return 0;
    return OmaReadStoppedTables(process, pointer_cell_address,
                                local_table_address, snapshot);
}

int OmaConfigureCopyBreakpoint(CONTEXT *context, DWORD module_base,
                               const OmaModuleProfileV1 *profile)
{
    DWORD address;
    if (context == 0 || profile == 0 ||
        !address_add(module_base, profile->copy_post_eip_rva, &address))
        return 0;
    context->Dr0 = address;
    context->Dr1 = 0UL;
    context->Dr2 = 0UL;
    context->Dr3 = 0UL;
    context->Dr6 = 0UL;
    context->Dr7 = (context->Dr7 & ~OMA_DR7_COPY_MASK) | OMA_COPY_BP_DR7;
    return 1;
}

int OmaArmThreadCopyBreakpoint(HANDLE thread, DWORD module_base,
                               const OmaModuleProfileV1 *profile)
{
    CONTEXT context;
    if (thread == 0)
        return 0;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(thread, &context) ||
        !OmaConfigureCopyBreakpoint(&context, module_base, profile))
        return 0;
    return SetThreadContext(thread, &context) != 0;
}

int OmaValidateCopyStop(const CONTEXT *context, DWORD host_base,
                        DWORD module_base, DWORD clone_base,
                        DWORD clone_size, DWORD pointer_cell_value,
                        const OmaModuleProfileV1 *profile,
                        const DWORD *source_table,
                        const DWORD *local_table,
                        DWORD expected_slot89_target)
{
    DWORD expected_eip;
    DWORD expected_local;
    DWORD original_source_address;
    DWORD source_base;
    DWORD index;
    if (context == 0 || profile == 0 || source_table == 0 ||
        local_table == 0 || expected_slot89_target == 0UL ||
        !address_add(module_base, profile->copy_post_eip_rva,
                     &expected_eip) ||
        !address_add(module_base, profile->local_table_rva,
                     &expected_local) ||
        !address_add(host_base, OMA_HOST_SOURCE_TABLE_RVA,
                     &original_source_address))
        return 0;

    if (context->Eip != expected_eip ||
        (context->Dr6 & 0x0fUL) != 1UL ||
        (context->Dr7 & OMA_DR7_COPY_MASK) != OMA_COPY_BP_DR7 ||
        context->Dr0 != expected_eip || context->Dr1 != 0UL ||
        context->Dr2 != 0UL || context->Dr3 != 0UL ||
        context->Ecx != 0UL || (context->EFlags & 0x400UL) != 0UL ||
        context->Esi < OMA_TABLE_BYTES ||
        context->Edi < OMA_TABLE_BYTES)
        return 0;

    source_base = context->Esi - OMA_TABLE_BYTES;
    if (source_base != pointer_cell_value ||
        source_base == original_source_address ||
        !address_in_range(source_base, clone_base, clone_size,
                          OMA_TABLE_BYTES) ||
        context->Edi - OMA_TABLE_BYTES != expected_local ||
        host_base == 0UL)
        return 0;

    if (source_table[0] != OMA_TABLE_SIGNATURE ||
        source_table[OMA_SLOT89] != expected_slot89_target ||
        local_table[OMA_SLOT89] != expected_slot89_target)
        return 0;
    for (index = 0; index < OMA_TABLE_WORDS; ++index) {
        if (source_table[index] != local_table[index])
            return 0;
    }
    return 1;
}

int OmaConfigurePostCopyWatchpoints(CONTEXT *context,
                                    DWORD module_base,
                                    DWORD local_table_address,
                                    const OmaModuleProfileV1 *profile)
{
    DWORD slot89_address;
    DWORD callee_return;
    DWORD caller_return;
    if (context == 0 || profile == 0 ||
        (local_table_address & 3UL) != 0UL ||
        !address_add(local_table_address, OMA_SLOT89_OFFSET,
                     &slot89_address) ||
        !address_add(module_base, profile->submit_callee_return_rva,
                     &callee_return) ||
        !address_add(module_base, profile->submit_caller_return_rva,
                     &caller_return))
        return 0;

    context->Dr0 = 0UL;
    context->Dr1 = slot89_address;
    context->Dr2 = callee_return;
    context->Dr3 = caller_return;
    context->Dr6 = 0UL;
    context->Dr7 = (context->Dr7 & ~OMA_DR7_MONITOR_MASK) |
                   OMA_MONITOR_BP_DR7;
    return 1;
}

int OmaArmThreadPostCopyWatchpoints(HANDLE thread, DWORD module_base,
                                    DWORD local_table_address,
                                    const OmaModuleProfileV1 *profile)
{
    CONTEXT context;
    if (thread == 0)
        return 0;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(thread, &context) ||
        !OmaConfigurePostCopyWatchpoints(&context, module_base,
                                         local_table_address, profile))
        return 0;
    return SetThreadContext(thread, &context) != 0;
}

int OmaValidatePostCopyWatchpoints(const CONTEXT *context,
                                  DWORD module_base,
                                  DWORD local_table_address,
                                  const OmaModuleProfileV1 *profile)
{
    DWORD slot89_address;
    DWORD callee_return;
    DWORD caller_return;
    if (context == 0 || profile == 0 ||
        !address_add(local_table_address, OMA_SLOT89_OFFSET,
                     &slot89_address) ||
        !address_add(module_base, profile->submit_callee_return_rva,
                     &callee_return) ||
        !address_add(module_base, profile->submit_caller_return_rva,
                     &caller_return))
        return 0;
    return context->Dr0 == 0UL && context->Dr1 == slot89_address &&
        context->Dr2 == callee_return && context->Dr3 == caller_return &&
        (context->Dr7 & OMA_DR7_MONITOR_MASK) == OMA_MONITOR_BP_DR7;
}

OmaStopResultV1 OmaClassifyPostCopyStop(
    const CONTEXT *context, DWORD module_base, DWORD local_table_address,
    const OmaModuleProfileV1 *profile, DWORD slot89_value_before,
    DWORD slot89_value_now, int callee_return_seen,
    DWORD prior_raw_eax, DWORD *raw_eax_out)
{
    DWORD callee_return;
    DWORD caller_return;
    if (raw_eax_out != 0)
        *raw_eax_out = 0UL;
    if (!OmaValidatePostCopyWatchpoints(context, module_base,
                                       local_table_address, profile) ||
        !address_add(module_base, profile->submit_callee_return_rva,
                     &callee_return) ||
        !address_add(module_base, profile->submit_caller_return_rva,
                     &caller_return))
        return OMA_STOP_INVALID;
    if ((context->Dr6 & 0x0fUL) == 0x02UL)
        return OMA_STOP_SLOT89_WRITE_AFTER_COPY;
    if ((context->Dr6 & 0x0fUL) == 0x04UL &&
        context->Eip == callee_return &&
        slot89_value_now == slot89_value_before) {
        if (raw_eax_out != 0)
            *raw_eax_out = context->Eax;
        return OMA_STOP_SLOT89_CALLEE_RETURNED;
    }
    if ((context->Dr6 & 0x0fUL) != 0x08UL ||
        context->Eip != caller_return || !callee_return_seen ||
        context->Eax != prior_raw_eax ||
        slot89_value_now != slot89_value_before)
        return OMA_STOP_INVALID;
    if (raw_eax_out != 0)
        *raw_eax_out = context->Eax;
    return OMA_STOP_SUBMIT_WRAPPER_RETURNED;
}
