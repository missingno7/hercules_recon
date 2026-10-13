#ifndef HERCULES_PREENTRY_PROBE_DIAG_H
#define HERCULES_PREENTRY_PROBE_DIAG_H

typedef unsigned long probe_u32;

typedef struct ProbeDiagnosticV1 {
    probe_u32 magic;
    probe_u32 state;
    probe_u32 process_attach_calls;
    probe_u32 module_base;
    probe_u32 source_pointer_cell;
    probe_u32 source_pointer_before;
    probe_u32 source_pointer_after;
    probe_u32 source_word0;
    probe_u32 source_word1;
    probe_u32 source_slot89;
    probe_u32 clone_pointer;
    probe_u32 clone_word0;
    probe_u32 clone_word1;
    probe_u32 clone_slot89;
    probe_u32 clone_status;
    probe_u32 directdrawcreate_calls;
} ProbeDiagnosticV1;

#define PREENTRY_DIAG_MAGIC 0x50424531UL
#define PREENTRY_DIAG_STATE_INITIAL 0UL
#define PREENTRY_DIAG_STATE_ATTACHED 1UL
#define PREENTRY_DIAG_STATE_PUBLISHED 2UL
#define PREENTRY_DIAG_STATE_SOURCE_MISMATCH 0xE001UL
#define PREENTRY_DIAG_STATE_CLONE_FAILED 0xE002UL

#endif
