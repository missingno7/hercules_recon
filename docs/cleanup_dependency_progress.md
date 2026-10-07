# Cleanup dependency continuation

The long-run goal remains active. This checkpoint recovers ordinary C for two
actor-removal contributions and the token-release contribution. It does not add
a `FUNCTION_MATCH`: the accepted total remains **39 instances / 4,336 bytes**, and
`recovery.json` is unchanged. Real globals and callees still prevent natural link
closure.

| Contribution | ENG1 RVA | Complete bytes | Diagnostic result |
|---|---|---:|---|
| Actor removal | `0x27260` | 432 | All bytes equal after relocation normalization; 22 COFF relocations |
| Actor removal variant | `0x27860` | 320 | All bytes equal after relocation normalization; 15 COFF relocations |
| Token release | `0x5750` | 48 | All bytes equal after relocation normalization; two COFF relocations |
| Acquire action group | `0x2afe0` | 80 | All bytes equal after relocation normalization; four COFF relocations |
| Release action group | `0x2b370` | 128 | All bytes equal after relocation normalization; five COFF relocations |
| Retire action group | `0x2b030` | 64 | All bytes equal after relocation normalization; three COFF relocations |
| Append normal action | `0x2b070` | 96 | All bytes equal after relocation normalization; three COFF relocations |
| Append alternate action | `0x2b0d0` | 96 | All bytes equal after relocation normalization; three COFF relocations |
| Forward host allocation | `0x2c3c0` | 32 | All bytes equal after relocation normalization; one COFF relocation |
| Allocate action groups | `0x2af80` | 96 | 14 normalized differences; initial guard/table-base topology differs |
| Recursive actor clear | `0x55b0` | 80 | Previously explained relocation-only peer remains unchanged |
| Resource state release | `0x51d0` | 320 | 120 normalized byte differences; 21 matching CFG blocks; nine audited relocations |

The removal boundaries include the complete natural padding: 418 reachable bytes
and 14 NOPs for the first routine, 310 and 10 for the variant. The PC instruction
`TEST AH,8` checks render flag `0x800`. The first candidate mistakenly used
`0x80000`; high instruction similarity concealed that behavioral error. The
durable fixture tests all 32 one-bit flags, callback order, callback-visible kind,
null owners and sidecar mutation. Literal zero clears on known-null paths generate
the desired output without pointer narrowing.
An explicit one-constant negative control restores only the wrong `0x80000` mask;
the widened fixture rejects it with 11 failures. This confirms that the fixture
distinguishes the diagnosed error, beyond passing the selected candidate.

`calibration/remove_actor.c` and `calibration/cleanup_helpers_writers.c` remain
experimental source. Their reviewed specifications and complete-output receipts
are indexed by `scripts/region.py packet remove_actor_region` and
`scripts/region.py packet cleanup_resource_writers`. Descriptive names and region
grouping do not establish original names, declarations or translation units.

## Writer-backed representation

`evidence/resource_writer_layout.json` records compact PC evidence. The physical
resource table starts at RVA `0x5cee0`, with 81 records of 20 bytes; the older
`0x5ceec` view names its reference-buffer member at +12. State is at +16.
Allocated reference rows and the 32 token rows are paired shorts, not the invented
first-pass structures. The 256 link rows have a four-bit occupied mask, a descriptor
word and four successor keys. The packed key's low nibble is an occupancy mask,
including composite masks; a signed map selects the successor slot.

The corrected resource source preserves the observed key-before-state read,
independent count-zero and state-zero predicates, map read before mask mutation,
and distinct row +0/+4 references. These recover the target size, topology and
addresses but do not settle register/index lifetime. Native `int` indices and a
six-word row declaration produced identical complete outputs to the selected
baseline. A signed local key with unsigned packed-group decoding changed output
but did not recover the target allocation. These families are exhausted pending
independent new source, TU or compiler-context evidence. Preserve the two equal
peer outputs rather than repeating spelling grids.

The diagnostic mapper now audits external physical bases against the **actual
COFF DIR32 member addends**. Every observed base-plus-addend must correspond to an
encoded original operand. Tests reject missing members, wrong bases and
unreferenced symbols. It does not mask operands, change objects or alter
`match.py` / `promote.py`; relocation-normalized equality remains diagnostic.

## Verification and next dependency

Fresh pinned VC5 compilation reproduces both removal contributions and both
cleanup peers over their full spans. All **39 accepted matches remain raw exact**.
All **18 semantic suites / 2,855,928 checks**, **28 Python tests**, **12 immutable
original hashes**, and the four-image / 13-span interface recheck pass. The new
resource fixture includes 8,420 checks over both chains, all nonzero four-bit
masks, residency states, absent buffers and legal interior pointers for signed
reference loads. Such negative fixture indices do not prove production validity.
No original game image was loaded or executed.

Static interface documentation also now states the observed ENG1 order correctly:
the 250-dword copy precedes signature validation, so invalid input can already
modify the destination table.

Luna xhigh handled bounded reconstruction and compiler hypotheses; Sol high
diagnosed representation and remaining lifetime uncertainty. Private handoffs
record requested configurations, source/object hashes and build counts. Actual
model identity, model-only time, tokens and cost are unknown; no economics claim
follows from this checkpoint.

The next real edge, `release_pending_action` at ENG1 RVA `0x2b370`, is now
explained together with its free-slot helper at `0x2afe0`: another 208 bytes of
relocation-only equality. The first child is at +4, with count at offset zero.
Initializing the one-based loop counter before the positive-count guard explains
the remaining control-flow difference. The 86-check durable fixture exercises
callbacks changing the group count, registry slot and registry count independently.
Fixture array dimensions do not establish production capacity. The actual registry
allocation and initialization writers are the next dependencies; placeholders or
placement tricks cannot close them.
The selected 72-byte level record supplies the per-group limit from +0x14 and the
outer registry count from +0x16. Eight non-sentinel records include outer counts up
to 20 and limits up to 30. Selecting a count-20 record requires 20 valid slots for
that execution; exact physical capacity remains unproved. Pointer-slot population
requires the cross-image chain below. The sentinel reset at `0x27a39` writes pointed group counts, not the
table pointers. See `evidence/action_registry_writer_layout.json`.

The adjacent retirement and two append helpers add another **256 bytes** of full
relocation-normalized equality. Retirement writes through the loaded table entry.
Both appenders share a failure epilogue, call the appropriate real actor factory,
select the low-nine-bit callback row, then reload/increment the group count and
store the actor. Their fixture covers factory arguments, failure/limit paths,
callback-before-append visibility and callback count mutation. No original actor
factory or callback implementation was substituted into the reconstruction.

The pointer-population uncertainty above is now narrowed by
`evidence/registry_population.json`: ENG1 initializer `0x2af80` passes each pointer
slot address and exactly `4 * append_limit` requested bytes to wrapper `0x2c3c0`.
That wrapper forwards three arguments through interface slot `0x1e`; the reviewed
default host table binds it to EXE `0x4570`, which writes the allocated pointer
through argument one. With flags zero, its failure path writes NULL. ENG1 checks
the result and writes `-1` to each successful group. Its direct caller ignores
initializer failure before calling the later reset; successful allocation is not
guaranteed by this static chain. Physical table capacity, child payload bounds and
runtime callback mutation remain unproved. Do not alter the requested size or
append limit to repair a suspected bounds issue during matching.

`python scripts/registry_population_evidence.py` rechecks both immutable images,
six reviewed code spans, the 250-dword table hash and default callback cell. The
next reconstruction chunk is the real allocator initializer and forwarding
wrapper. Existing code-equal contributions stay frozen while those dependencies
are recovered.

That initializer/wrapper chunk is now preserved in
`calibration/registry_allocation.c`. The wrapper is a full 32-byte diagnostic equal.
The 96-byte initializer retains 14 normalized differences: the selected ordinary
loop loads the table base before the initial guard and merges the zero-entry path
differently. Its 50-check fixture passes exact request/flags/output-slot checks,
live count and limit mutations, and partial failure behavior. Four bounded source
families stopped at their falsifiers; a new structural discriminator is needed
before reopening that residual. The wrapper stays frozen.

Across this long-run checkpoint, **nine newly explained functions / 1,296 bytes**
are full-span relocation-only diagnostic equals. The strict accepted total stays
39; global data, host allocator/pool lifecycle, real actor factories and the two
source residuals still prevent natural module closure. The goal remains active.
