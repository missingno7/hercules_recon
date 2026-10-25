# Action appenders and explicit callback proof, 2026-10-25

ENG1 `append_normal_action` at `2b070` and `append_kind_action` at `2b0d0`
have complete 96-byte compiler contributions with independently verified
references. They use `src/eng1/behavioral/action_group_writers.c`, retaining the
existing native-backed count/entry overlay and preserving all instruction bytes.
The unsigned32 increment explicitly preserves count wrap after the callback.
Retirement moves with its two neighbors but remains diagnostic: its indexed
registry dependency still lacks a complete independently proved native extent.

The current ledger has 477 accepted distinct entries / 47,840 bytes: 137 RAW,
297 RELOCATED / 33,632 bytes, and 43 retained linked rows. All 475 earlier accepted
records remain identical. Matching, source dependencies and callback behavior
remain distinct claims.

## Why a separate callback proof was required

The existing relocated gate correctly rejected these functions' indirect calls.
A complete scalar root identifies storage, without authorizing indirect dispatch.
The new `scripts/callback_transfer.py` implements only an explicit reviewed
`rooted-callback-v1` record; absent a record, the old rejection remains.

The profile proves one exact dispatch expression: absolute load from an already
verified complete four-byte BSS root, low 9-bit register mask, one DWORD register
argument, stride 8 or 12, slot-zero memory CALL and four-byte caller cleanup.
Stride 12 requires the actual x3 LEA. Root, index and argument registers are
disjoint, with no intervening clobber, extra argument or unclassified instruction.
A count reload may occur before cleanup. Direct branches or local-address/table
references into the window reject. The complete caller instruction/reference
comparison remains unchanged around this additional check.

Independent native windows `cabb..cad1` and `40418..4042b` corroborate the two
root/mask/stride/argument/call/cleanup shapes with different register allocation.
Each window is pinned by hash and exactly one native HIGHLOW root fixup; neither
is inside either appender. Verification rejects witnesses inside any pending
caller. This establishes the dispatch expression rather than the set of possible
dynamic callees, table capacities or callback behavior.

All 434 prior strict proofs were compiled before and after the mode addition.
Their instruction, target, binding, evidence and compile-context hashes remain
identical. Twenty new adversarial tests exercise malformed/unused certificates,
wrong roots/masks/strides/slots/provenance, clobbers, extra arguments, cleanup,
incoming direct/local edges, witness identity/fixups/independence, owner extent,
ordinary byte differences, and normal fresh publication/regression. The full
Python suite has 321 passing tests. Existing RAW and default unsupported-reference
policies are preserved; the two explicit proof windows supply the new evidence.

## Independent references and composed behavior

The existing `frame_data.c` objects at 71370/7139c are complete DWORD callback
address carriers; 71394 is a complete WORD limit. Independent selected-row
configuration stores identify all three, outside the appenders. Fresh COMMON
and native loader-zero checks corroborate them. The census finds 125 direct
overlapping scalar accesses. Two additional numeric 71370 references are exclusive
end pointers for the preceding 20-DWORD command table, independently decoded
from its two loops; they are not callback-cell reads. Computed aliases and
pointee capacities remain outside this census.

Both coordinate factories have earlier accepted entry proofs, checked freshly
and corroborated by other native call witnesses. Six wrong symbol bindings and
ten missing/wrong callback certificates reject. These identities do not come
from the caller's implied symbol addresses.

`python scripts/action_appenders_differential.py` passes 776 guarded native/source
cases. The real coordinate factories, allocators and full normal/middle/base
initializer chain execute; 632 indirect callbacks use an explicit mutation model
and clobber volatile registers. Cases cover signed count versus unsigned WORD
limit, allocation failure, both pool partitions, high kind bits, callback changes
to count, INT_MAX/INT_MIN wrap, slot-zero count overwrite, and 16 group/actor alias
cases. Reads, ordered stores, entry/callback order, return values, stack,
surrounding guards and synthetic memory agree. These finite modeled callback
cases establish no original callback bodies or native allocation capacities.

Only three unaccepted diagnostics change source providers. Their original rows
and the unchanged old-unit inventory are retained in the review. The runtime
source plan replaces that unit with exactly one new provider for each function.
The other 242 source units retain their full parsed sections/relocations and
non-FILE symbol inventories, with FILE metadata validated separately. All three
new integrated contributions retain complete instruction equality.

The promotion passes 436 strict proofs and 43 linked function checks. The remaining
supported masked rows pass too; 86 existing broader linked provenance drifts
remain. The source graph still has 962 mapped providers, 1,039 emitted external
function symbols, 736 unresolved symbols and zero duplicates/placeholders. It
does not link or establish a complete playable game.

Frozen evidence is in `evidence/action_appender_{references,bindings,runtime}_2026_10_25.json`
and `evidence/rooted_callback_mode_2026_10_25.json`. The current census and hashes
are in `evidence/action_appender_progress_2026_10_25.json`.
