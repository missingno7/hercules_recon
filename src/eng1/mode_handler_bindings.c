/* The setup table retains the recovered RVA-based external names from
 * setup_shared_state.h.  The player-mode implementation owns the same entry
 * points under its source names; these no-argument cdecl forwarders connect
 * the table to those real bodies without duplicating their behavior.
 *
 * Dispatch evidence: evidence/eng1_setup_source.json records the indirect
 * call at 0x1555b as cdecl, zero physical arguments, discarded return value.
 * setup_handler_assignments.inc maps each alias below to its matching RVA.
 */
extern void eng1_16840(void);
extern void eng1_16b40(void);
extern void eng1_16da0(void);
extern void eng1_16f40(void);
extern void eng1_171b0(void);
extern void eng1_17400(void);
extern void eng1_177a0(void);
extern void eng1_178f0(void);
extern void eng1_17a90(void);
extern void eng1_17c30(void);
extern void eng1_17c70(void);
extern void eng1_17cd0(void);
extern void eng1_17eb0(void);
extern void eng1_18240(void);
extern void eng1_182c0(void);
extern void eng1_18550(void);
extern void eng1_18770(void);
extern void eng1_18830(void);
extern void eng1_379e0(void);

void __cdecl eng1_native_code_target_0016840(void) { eng1_16840(); }
void __cdecl eng1_native_code_target_0016b40(void) { eng1_16b40(); }
void __cdecl eng1_native_code_target_0016da0(void) { eng1_16da0(); }
void __cdecl eng1_native_code_target_0016f40(void) { eng1_16f40(); }
void __cdecl eng1_native_code_target_00171b0(void) { eng1_171b0(); }
void __cdecl eng1_native_code_target_0017400(void) { eng1_17400(); }
void __cdecl eng1_native_code_target_00177a0(void) { eng1_177a0(); }
void __cdecl eng1_native_code_target_00178f0(void) { eng1_178f0(); }
void __cdecl eng1_native_code_target_0017a90(void) { eng1_17a90(); }
void __cdecl eng1_native_code_target_0017c30(void) { eng1_17c30(); }
void __cdecl eng1_native_code_target_0017c70(void) { eng1_17c70(); }
void __cdecl eng1_native_code_target_0017cd0(void) { eng1_17cd0(); }
void __cdecl eng1_native_code_target_0017eb0(void) { eng1_17eb0(); }
void __cdecl eng1_native_code_target_0018240(void) { eng1_18240(); }
void __cdecl eng1_native_code_target_00182c0(void) { eng1_182c0(); }
void __cdecl eng1_native_code_target_0018550(void) { eng1_18550(); }
void __cdecl eng1_native_code_target_0018770(void) { eng1_18770(); }
void __cdecl eng1_native_code_target_0018830(void) { eng1_18830(); }
void __cdecl eng1_native_code_target_00379e0(void) { eng1_379e0(); }
