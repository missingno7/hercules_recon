# Clamp, animation and sine clients, 2026-10-25

Four unchanged ENG1 caller contributions gain complete RELOCATED proofs / 720
bytes. Recovery.json now records 442 distinct accepted entries / 44,288 bytes:
137 RAW, 262 RELOCATED / 30,080 bytes, and 43 retained linked rows. The previous
438 accepted records are identical. No gameplay source, storage definition,
alias header, build configuration or matching-verifier policy changed.

| ENG1 RVA | Bytes | Existing source in calibration/eng1/ | Boundary reviewed |
| --- | ---: | --- | --- |
| 0x117b0 | 32 | level_unit_10120.c | Signed collision-owner x clamp |
| 0x34570 | 224 | status_panel_32b10.c | RNG/position-dependent frame selection |
| 0x398b0 | 272 | player_unit_36030.c | Sine projection, damage and lifetime reload |
| 0x3a550 | 192 | player_scene_3a040.c | Owner-qualified orbit update and collision test |

The question was whether complete existing data owners and independently known
helper entries explain all remaining references without changing these callers.
Fresh canonical compilation and the full instruction/reference gate answer yes.
Sixteen distinct wrong-reference controls reject. Diagnostic inventory metadata
for the four affected units is refreshed; it does not grant peer acceptance.

The pointer cells reuse the frozen explicit data-only owner proofs at 0x72860
and 0x7217c. The sine reference uses all 2,048 bytes of the existing initialized
`calibration/title/title_sine.c:_g_243e0` owner. A fresh compilation reproduces
the complete payload, without relocations, and a full native search finds its
unique occurrence at 0x5a848. The earlier full-table audit remains pinned. The
indexed sine payload is file-backed data; this does not extend BSS support.

The frame selector's own existing RAW proof establishes its 0x292f0 entry. The
removal helper's own fresh 432-byte diagnostic and two native direct-call
witnesses outside these selected callers establish its 0x27260 entry. The other
five helper entry identities reuse their separate body/source evidence. Removal,
interpreter, damage, bounds and host-RNG dependency chains remain incomplete;
their entry identities do not transfer acceptance to those helpers.

`python scripts/pointer_clients_differential.py` runs 2,117 unpatched original/
source comparisons in Unicorn:

| Caller | Cases |
| --- | ---: |
| 0x117b0 | 8 |
| 0x34570 | 771 |
| 0x398b0 | 1,094 |
| 0x3a550 | 244 |

The fixture exercises signed clamp boundaries, RNG thresholds and out-of-range
returns, screen/z boundaries, all 1,024 sine indices, low-word angle wrapping,
owner kind/magic rejection, post-interpreter state and root changes, and counter
changes during modeled damage. The state-four animation path safely returns
with a null collision pointer and never invokes RNG. Every compared case agrees
on the full synthetic data region, ordered guest stores, root/table read traces
and service call order/arguments. It also preserves the argument frame, return
stack and nonvolatile registers.

The actual frame selector runs as real compiled source. RNG, script interpreters,
bounds, damage and removal are explicitly modeled and intercepted before their
fixture entries execute. Their modeled effects stress reload/order behavior;
they do not establish complete helper semantics or native storage capacities.
No original OS module or game startup occurs.

The private fixture removes unrelated bodies while retaining declarations and
source context. Before linking, every selected complete COFF contribution must
equal the fresh full canonical unit, including its relocations and padding.
The fixture uses the real initialized sine owner and typed pointer-owner wrappers.
The historical linker uses `/OPT:NOICF` so modeled services retain distinct entry
addresses; uniqueness is asserted. Inputs are snapshotted before compilation and
rechecked after building and executing. All fixture files stay in ignored work/.
Fixture extraction and finite comparisons grant no acceptance on their own.

The journaled promotion passes all 401 strict proofs and 43 retained linked
function checks. The existing 86 broader linked provenance drifts remain. See
the frozen references, runtime report and bindings in
`evidence/pointer_clients_{references,runtime,bindings}_2026_10_25.json`, and the
final census/validation in `evidence/pointer_clients_progress_2026_10_25.json`.
Protect those accepted inputs before any later source or tool changes. Full
matching, source dependency closure and runtime completeness remain separate.
