# Motion/contact response and fixed-degree helpers, 2026-10-25

Four missing ENG1 bodies now have ordinary C source and complete relocated
acceptance: `2bc50` (208 bytes), `2bd20` (416), `2bec0` (48) and `2bef0` (64).
They add 736 bytes. The ledger contains 408 accepted entries / 39,296 bytes:
136 RAW rows, 229 RELOCATED rows and 43 retained linked rows. All 404 earlier
accepted records remain identical.

The question was whether this small missing dependency family could be recovered
without changing accepted motion code or substituting mathematical equivalents.
`src/eng1/behavioral/contact_motion.c` preserves the native distinction between
calling `advance_object_motion` at `2bc50` and inlining its three-axis update at
`2bd20`. Each axis uses DWORD wrap, a lower-zero clamp followed by the original
upper clamp, then signed-direction position updates. A negative upper limit can
replace the lower clamp. Zero y-speed swaps the x/y acceleration/deceleration
roles and clears the y direction before checking contact.

Only a zero y-direction invokes the contact service. The damped entry calls
`29e20`, then on success halves y-speed and the x/z limits. The other entry calls
`29f50` and halves only y-speed. Both reload mutable actor state after the call,
set y direction to `ff`, transfer acceleration/deceleration roles, and return
one or three depending on the resulting speed. The contact services themselves
remain unaccepted; their entry identities are established by their own freshly
compiled bodies, bounded native review and independent call anchors outside
the four new functions. This does not prove their scene/grid dependencies.

The initial damped source had 73 non-reference byte differences. Expressing
the independent damping stage before response state assignments reduced that
to six. Initializing the result before the final z-limit store removed the last
six. These are measured C scheduling choices, with no assembly or instruction
patching. The other three contributions matched on their initial C control.

`src/shared/fixed_angle.c` deliberately keeps two different phase orders.
`2bec0` computes `((angle / 45 + 256) >> 9) & 1023`; `2bef0` computes
`((angle / 45 >> 9) + 256) & 1023`. Division is signed and truncates toward zero.
Both return twice the signed-WORD sine-table sample. The first expression's
addition uses explicit DWORD wrap. The complete 2,048-byte initialized sine
owner and unique native payload identity come from the preceding angle audit.

`scripts/contact_motion_differential.py` passes 1,550 isolated original/source
cases: 268 per contact entry and 507 per fixed-angle entry. The original accepted
motion body executes normally. The two contact services are modeled with zero,
positive and negative results, optionally changing position, speeds, limits,
directions and both acceleration roles. Callback snapshots, complete fixture
memory, full return values, cdecl frames, ESP and callee-saved registers agree.
Execution remains inside selected contributions, the accepted motion body and
intercepted services; writes remain inside actor storage or the stack. No OS
module load, initializer or original host/game startup occurs. These finite
fixtures do not establish universal or whole-game equivalence.

Five wrong-reference controls reject with zero non-reference differences. All
367 strict proofs and 43 legacy linked function checks pass; the existing 86
broader linked provenance drifts remain. All 218 Python tests and 1,625 remaining
supported masked rows in 199 sources pass. Two obsolete diagnostic contact
placeholders were retired; their verified physical ABI aliases target the real
source, without adding the entire spatial-effects unit to the graph.
The genuine ENG1 graph gains four
mapped providers (961 / 1,231 diagnostic spans) and closes three unresolved names
(744 remain), with zero duplicate providers or generated placeholders. It still
does not link.

Evidence is in `evidence/motion_contacts_recovery_2026_10_25.json`,
`evidence/motion_contacts_bindings_2026_10_25.json`,
`evidence/motion_contacts_runtime_2026_10_25.json` and the frozen final receipt
`evidence/motion_contacts_progress_2026_10_25.json`. Recovery.json alone grants
matching acceptance. The next dependency question is the `20b50` grid query
behind the contact services. Its existing source is explicitly known incomplete:
four finite native cases cover the scan order, while row/header/stream capacities
remain unproved. Entry identification does not settle those ownership questions
or turn that guarded source into a matching contribution.
