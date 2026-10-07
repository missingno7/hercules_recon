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
| Recursive actor clear | `0x55b0` | 80 | Previously explained relocation-only peer remains unchanged |
| Resource state release | `0x51d0` | 320 | 120 normalized byte differences; 21 matching CFG blocks; nine audited relocations |

The removal boundaries include the complete natural padding: 418 reachable bytes
and 14 NOPs for the first routine, 310 and 10 for the variant. The PC instruction
`TEST AH,8` checks render flag `0x800`. The first candidate mistakenly used
`0x80000`; high instruction similarity concealed that behavioral error. The
durable fixture tests all 32 one-bit flags, callback order, callback-visible kind,
null owners and sidecar mutation. Literal zero clears on known-null paths generate
the desired output without pointer narrowing.

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
All **15 semantic suites / 2,855,759 checks**, **28 Python tests**, **12 immutable
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

The next real edge is `release_pending_action` at ENG1 RVA `0x2b370`, called with
Actor +0x120. Its registry and free-slot helper need actual recovery and writer
evidence. They are not substitutes for unresolved dependencies and cannot be
closed with placeholders or placement tricks.
