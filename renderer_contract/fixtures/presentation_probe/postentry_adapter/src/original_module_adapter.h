#ifndef HERCULES_ORIGINAL_MODULE_ADAPTER_H
#define HERCULES_ORIGINAL_MODULE_ADAPTER_H

#include <windows.h>
#include "hercules_host_abi.h"

#define OMA_TABLE_WORDS ENGINE_TABLE_WORD_COUNT
#define OMA_TABLE_BYTES (OMA_TABLE_WORDS * 4UL)
#define OMA_SLOT89 HERCULES_SLOT_SUBMIT
#define OMA_SLOT89_OFFSET (OMA_SLOT89 * 4UL)
#define OMA_TABLE_SIGNATURE ENGINE_TABLE_SIGNATURE
#define OMA_HOST_SOURCE_TABLE_RVA HERCULES_HOST_TABLE_RVA
#define OMA_HOST_POINTER_CELL_RVA HERCULES_HOST_TABLE_POINTER_RVA

#define OMA_HOST_RETAIL_SHA256 \
    "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
#define OMA_HOST_NOCD_SHA256 \
    "b3ab3ee9e5c973bb245d8eed795e8e7065d7dfc371e412e602b2059af894a551"

typedef struct OmaModuleProfileV1 {
    const char *module_name;
    const char *module_sha256;
    DWORD preferred_base;
    DWORD local_table_rva;
    DWORD copy_instruction_rva;
    DWORD copy_post_eip_rva;
    DWORD submit_wrapper_rva;
    DWORD submit_callee_return_rva;
    DWORD submit_caller_call_rva;
    DWORD submit_caller_return_rva;
    const char *submit_wrapper_sha256;
} OmaModuleProfileV1;

typedef enum OmaStopResultV1 {
    OMA_STOP_INVALID = 0,
    OMA_STOP_SLOT89_WRITE_AFTER_COPY = 1,
    OMA_STOP_SLOT89_CALLEE_RETURNED = 2,
    OMA_STOP_SUBMIT_WRAPPER_RETURNED = 3,
    OMA_STOP_TRACE_ENTRY_OBSERVED = 4
} OmaStopResultV1;

typedef enum OmaProbePhaseV1 {
    OMA_PHASE_INVALID = 0,
    OMA_PHASE_COPY_ARMED = 1,
    OMA_PHASE_COPY_VALIDATED = 2,
    OMA_PHASE_POSTCOPY_ARMED = 3,
    OMA_PHASE_SLOT89_CLOBBERED = 4,
    OMA_PHASE_CALLEE_RETURN_OBSERVED = 5,
    OMA_PHASE_WRAPPER_COMPLETED = 6
} OmaProbePhaseV1;

typedef struct OmaProbeStateV1 {
    OmaProbePhaseV1 phase;
    DWORD slot89_value_at_copy;
    DWORD callee_raw_eax;
} OmaProbeStateV1;

typedef struct OmaTableSnapshotV1 {
    DWORD source_address;
    DWORD source_table[OMA_TABLE_WORDS];
    DWORD local_table[OMA_TABLE_WORDS];
} OmaTableSnapshotV1;

const OmaModuleProfileV1 *OmaFindProfile(const char *module_name);
int OmaStartCopyObservation(CONTEXT *context, DWORD module_base,
                            const OmaModuleProfileV1 *profile,
                            OmaProbeStateV1 *state);
int OmaAdvanceAfterValidatedCopy(CONTEXT *context, DWORD module_base,
                                 DWORD host_base, DWORD clone_base,
                                 DWORD clone_size,
                                 DWORD pointer_cell_value,
                                 DWORD local_table_address,
                                 const OmaModuleProfileV1 *profile,
                                 const DWORD *source_table,
                                 const DWORD *local_table,
                                 DWORD expected_slot89_target,
                                 OmaProbeStateV1 *state);
OmaStopResultV1 OmaObservePostCopyEvent(
    const CONTEXT *context, DWORD module_base, DWORD local_table_address,
    const OmaModuleProfileV1 *profile, DWORD slot89_value_now,
    OmaProbeStateV1 *state);
int OmaAcknowledgePostCopyEvent(CONTEXT *context,
                                OmaStopResultV1 observed_result);
int OmaMarkSyntheticCopyValidated(const CONTEXT *context, DWORD module_base,
                                  DWORD copy_post_eip_rva,
                                  const DWORD *published_clone,
                                  const DWORD *local_table,
                                  DWORD expected_slot89_target,
                                  OmaProbeStateV1 *state);
int OmaStartSyntheticCopyObservation(CONTEXT *context, DWORD copy_eip,
                                     OmaProbeStateV1 *state);
int OmaArmSyntheticPostCopy(CONTEXT *context, DWORD local_slot89_address,
                            DWORD first_trace_commit_address,
                            DWORD chain_caller_return_address,
                            DWORD submit_caller_return_address,
                            OmaProbeStateV1 *slot81_state,
                            OmaProbeStateV1 *slot89_state);
int OmaConfigureSyntheticPostCopyContext(
    CONTEXT *context, DWORD local_slot89_address,
    DWORD trace_commit_address, DWORD chain_caller_return_address,
    DWORD submit_caller_return_address);
int OmaMoveSyntheticTraceWatchpoint(CONTEXT *context,
                                    DWORD trace_commit_address);
OmaStopResultV1 OmaObserveSyntheticTraceCommit(
    const CONTEXT *context, DWORD trace_commit_address,
    DWORD slot89_value_now, DWORD raw_eax, int is_return_event,
    OmaProbeStateV1 *state);
OmaStopResultV1 OmaObserveSyntheticCallerReturn(
    const CONTEXT *context, DWORD caller_return_address, DWORD dr_bit,
    DWORD slot89_value_now, OmaProbeStateV1 *state);
int OmaValidateImageIdentity(const char *host_sha256,
                             const char *module_name,
                             const char *module_sha256,
                             const OmaModuleProfileV1 **profile_out);
int OmaReadStoppedTables(HANDLE process, DWORD pointer_cell_address,
                         DWORD local_table_address,
                         OmaTableSnapshotV1 *snapshot);
int OmaCaptureStoppedCopyTables(HANDLE process, DWORD host_base,
                                DWORD module_base,
                                const OmaModuleProfileV1 *profile,
                                OmaTableSnapshotV1 *snapshot);
int OmaConfigureCopyBreakpoint(CONTEXT *context, DWORD module_base,
                               const OmaModuleProfileV1 *profile);
int OmaArmThreadCopyBreakpoint(HANDLE thread, DWORD module_base,
                               const OmaModuleProfileV1 *profile);
int OmaValidateCopyStop(const CONTEXT *context, DWORD host_base,
                        DWORD module_base, DWORD clone_base,
                        DWORD clone_size, DWORD pointer_cell_value,
                        const OmaModuleProfileV1 *profile,
                        const DWORD *source_table,
                        const DWORD *local_table,
                        DWORD expected_slot89_target);
int OmaConfigurePostCopyWatchpoints(CONTEXT *context,
                                    DWORD module_base,
                                    DWORD local_table_address,
                                    const OmaModuleProfileV1 *profile);
int OmaArmThreadPostCopyWatchpoints(HANDLE thread, DWORD module_base,
                                    DWORD local_table_address,
                                    const OmaModuleProfileV1 *profile);
int OmaValidatePostCopyWatchpoints(const CONTEXT *context,
                                  DWORD module_base,
                                  DWORD local_table_address,
                                  const OmaModuleProfileV1 *profile);
OmaStopResultV1 OmaClassifyPostCopyStop(
    const CONTEXT *context, DWORD module_base, DWORD local_table_address,
    const OmaModuleProfileV1 *profile, DWORD slot89_value_before,
    DWORD slot89_value_now, int callee_return_seen,
    DWORD prior_raw_eax, DWORD *raw_eax_out);

#endif
