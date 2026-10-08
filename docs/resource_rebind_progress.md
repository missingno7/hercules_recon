# Resource reclamation and real owner relocation

Six new ordinary C functions cover 928 target bytes: the 80-byte reclaim caller,
two interface bridges (48 bytes), and three blob/row rebuild routines (800 bytes).
The caller and two bridges are relocation-normalized equals. None is a new raw
match. The existing six interface bridges preserve their complete bytes and
ordered relocations after extraction of `calibration/engine_interface.h`.

Reclaim calls the real compact bridge once for mode zero, or repeats until a
nonzero result for nonzero mode. It then reads the live movable root owner at
`0x70ed4`; a nonnull root triggers blob rebuild followed by the row rebuild.
The independent row-owner table at `0x71e40[category*8+row]` remains distinct.
Allocator calls receive each owner's slot address. Row data is not assumed to
belong to the root blob.

The blob parser uses six unsigned header words and packed field-relative DWORD
offsets rounded down to four-byte units. Group setup retains the signed selector
comparison, including equality at the upper boundary, signed counts, cumulative
low-word stores and exact reset prefixes. Row rebuild consumes the separately
allocated block and signed widths/counts. Original format names, complete types,
capacities and supported input ranges remain unproved. Its fixture passes eight
checks. All three functions remain nonexact.

Integration uses one observed native 250-word interface declaration across the
real separately linked engine/resource units. This shared context produces a
432-byte blob contribution, versus 448 in the private eight-byte-prefix build,
although its function body was not edited. The old complete compiler argv and
included-header hash were not retained, so a single-variable causal experiment
is not established. The two helpers retain their byte and
relocation identities. Both blob versions are nonexact; this is a measured
source-context change, not preservation or recovered original type evidence.
The compiler explanation remains unresolved, and no declaration grid is used
to chase the target. Source, header and output hashes are recorded in
`evidence/resource_rebind_measurements.json`.

The integration fixture links 23 real reconstructed host/engine/resource
functions as separate translation units. Synthetic in-memory blobs and storage
are explicit boundaries. A test-only selector adapter converts the current
provisional engine signed selector to the host unsigned declaration; reclaim
physically supplies zero. It does not establish the original ABI signedness.
Real allocation/free/compaction relocates both the blob and independent row
block through their actual owner slots. The real reclaim and rebuild callees
then consume the new pointers. All 19 checks pass. A negative control that
rebuilds the blob before compaction fails the stale-pointer check; the unchanged
production composition passes again. No original binary or game data runs.

The diagnostic map auditor now recognizes actual outgoing unconditional tail
jumps, which the reclaim's final rebuild call needs. Local and conditional
branches do not identify external callees. The added test rejects an incorrect
tail destination. Strict raw comparison and the acceptance gate are unchanged.

Thirteen additional archive CRT dependencies, covering 2,160 bytes, are
identified against pinned VC5 RTM LIBCMT members. All nonrelocation bytes agree,
and encoded operands, repeated symbol bases and section-local destinations are
checked. `_openfile` includes ten DWORD jump destinations and a 74-byte case map;
the verifier treats them as data and checks their real dispatcher. This is
library identity evidence, not function acceptance or complete CRT selection.
`python scripts/archive_crt_evidence.py` reproduces the result from immutable
library/oracle hashes. No CRT source is rewritten.

Current verification passes 46 semantic suites with 3,647,788 checks, 31 Python
tests and 12 immutable-file hashes. All 41 accepted functions (4,368 bytes) remain
raw equal after fresh historical compilation. `recovery.json` is unchanged.
The new regions are indexed for disposable packets and frozen until new
evidence or a new discriminator supports another family.

The full engine dispatcher is now bounded at `0x20300..0x20670` (880 bytes), with
57 call sites and 52 distinct callees. It samples the entry state, retains a
post-load loop target and reads live context flags independently. Its compact
static contract identifies real dependencies rather than replacing dispatch
with a stub. Its control handler and remaining library dependencies are next
evidence tasks; whole-module source and natural link closure remain open.
