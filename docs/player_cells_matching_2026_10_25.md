# Player state owners and callers, 2026-10-25

Five missing complete player-state owners now have native-backed source in
`src/eng1/behavioral/player_state_cells.{c,h}`. Twenty-six unchanged callers gain
complete RELOCATED proofs / 2,576 bytes. Recovery.json now records 468 distinct
accepted entries / 46,864 bytes: 137 RAW, 288 RELOCATED / 32,656 bytes, and 43
retained linked rows. All 442 previous accepted records are identical.

The question was whether the native writers and complete direct access widths
support individual state cells, without constructing guessed pointees or using
the candidate callers' implied addresses as identity evidence.

| Cell RVA | Source owner | View | Direct root operands | Writes |
| --- | --- | --- | ---: | ---: |
| 0x707dc | g_player_glow_707dc | Opaque actor pointer | 19 | 1 |
| 0x707e4 | g_burst_origin_707e4 | Opaque actor pointer | 48 | 1 |
| 0x707e8 | g_player_cruise_707e8 | Signed DWORD | 25 | 8 |
| 0x707f8 | g_player_flag_707f8 | Signed DWORD | 19 | 10 |
| 0x707fc | g_player_trail_707fc | Opaque actor pointer | 17 | 1 |

All 128 original HIGHLOW-root operands are classified from their indexed native
function entries. Every overlapping classified use addresses the exact root,
without a base/index register, and accesses four bytes. No searched root operand
is unclassified. This does not exhaust computed aliases or adjacent bases outside
the searched interval; the census is not a proof of an original C declaration.

The independent setup at 0x374c0 reads the middle-pool pointer and computes
offsets 0x2c4 and 0x3b0 before storing glow/trail. Those are the observed third
and fourth 0xec-stride positions; neither allocation capacity nor pointee
lifetime follows. The burst initializer at 0x3aef0 stores its actor argument.
The cruise writer at 0x3beb4 stores 0x40000; other native arithmetic subtracts
0x1999 and clamps a negative result. Flag writers clear or add/subtract DWORD
amounts; the scrolling consumer uses signed positive/negative tests. The identity
anchors lie outside every selected caller.

The new owner emits exactly five four-byte COMMON definitions, no functions and
no executable storage. Pointer/long widths are asserted in its header. It
supplies no pointee arrays, runtime initialization policy or placeholder logic.
Each cell lies wholly in the writable, non-executable native loader-zero tail,
outside loader-managed directories/fixups under the existing strict checks.
The source-build plan adds only this owner; existing source, headers, aliases
and flags remain unchanged. Matching verifier policy is unchanged.

The accepted callers are 0x372b0, 0x38310, 0x38740, 0x387f0, 0x38950, 0x38b00,
0x38d70, 0x38e30, 0x39ca0, 0x3a700, 0x3a770, 0x3a7a0, 0x3a8e0, 0x3a950,
0x3aa70, 0x3ab50, 0x3af20, 0x3b050, 0x3b080, 0x3b0b0, 0x3b0e0, 0x3b110,
0x3b140, 0x3b170, 0x3bc20 and 0x3c330. Their full canonical units are compiled
freshly. All instruction/padding bytes and every reference pass the existing
gate; 81 distinct wrong-reference controls reject. Ten helper entry identities
reuse separately pinned evidence and fresh own-body compilation. Entry identity
does not accept unproved helper bodies or establish their dependency closure.

`python scripts/player_cells_differential.py` performs 2,418 guarded original/
source comparisons, comprising 3,066 function invocations and 324 three-step
flag-producer/scroll-consumer sequences. Every selected entry has at least 64
isolated cases. Boundary cases cover signed flags, odd negative division by two,
motion and angle wrapping, scroll clamp thresholds, frame-list loop markers,
and root/state changes during modeled services. They compare complete synthetic
data, all six cell values, ordered guest stores, cell read traces and service
order/arguments. Argument frames, return stack and nonvolatile registers are
checked on each invocation.

The real advance/select/reset frame helpers and the actual empty entry execute.
RNG, interpreters, removal, camera bounds and 0x39d40 are explicitly modeled.
Their effects stress reloads and ordering, including stores after removal and
cruise reloads around RNG. These finite cases do not establish full helper
behavior, actor capacities or complete game execution. No original OS module
or game startup is performed.

Private extraction retains declarations and requires all 26 selected complete
COFF contributions to equal the fresh canonical objects before ordinary linking.
The fixture links the actual new state owner and the typed normal-pool adapter.
Modeled entries remain distinct with `/OPT:NOICF`; uniqueness and immutable
compile/execution inputs are checked. Private builds and case transcripts stay
under ignored work/; the compact canonical runtime receipt pins their result.

The full promotion passes 427 strict proofs and all 43 retained linked function
checks. The existing 86 broader linked provenance drifts remain. Frozen references,
bindings and runtime evidence are `evidence/player_cells_*_2026_10_25.json`.
The final progress receipt records the source-build comparison and census.
Protect all accepted inputs before later edits; function acceptance does not
establish whole-game matching, source completeness or runtime completeness.
