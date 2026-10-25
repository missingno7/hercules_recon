# Worklist live-read correction, 2026-10-25

The three worklist factories in `src/eng1/behavioral/worklist_factories.c`
now preserve the native asset reload boundaries. This is a semantic correction
to unaccepted source. The matching ledger remains unchanged at 479 accepted
entries / 48,304 bytes; no new matching acceptance is recorded.

ENG1 `270c0` reloads the worklist record's kind before each normal asset field,
with four separate reads of the selected root at `71388`. Its allocator, stored
actor kind and callback retain the kind captured at entry. The prior source
used that captured kind for every asset too and captured the asset root once.
Native `27720` and `27c40` retain their captured low12-bit kind but reload roots
`71390` and `713a4` between category and image stores. Their source also retained
one root. The correction preserves each family's distinct kind lifetime.

The original instructions are recorded by RVA, full instruction hash and size
in `evidence/worklist_live_reads_recovery_2026_10_25.json`. The normal sequence
includes kind loads at `2714e`, `2715e`, `27172`, `2718a`; root loads occur at
`27147`, `27162`, `27176`, `2718e`. Middle reloads at `277b3`/`277c3` and alternate
reloads at `27c95`/`27ca5` establish their two-read interfaces independently of
the C source's previous temporaries.

## Counterexamples and protection

The standard command is now:

    python scripts/eng1_worklist_factory_differential.py

It runs unpatched native instructions in Unicorn against the freshly compiled
source, then executes the existing source-only prefix-guard fixture. The command
no longer uses the older driver that loaded an original DLL through Windows.
Historical OS-loading receipts retain their original scope and identities.

The same final harness reproduces 54 failing cases with the preserved prior
source: 42 normal, six middle and six alternate. The corrected source passes
all 108 cases, including all 54 counterexamples. Case inventory and every native
result remain identical between those runs. No expected result is derived from
the corrected source. All 11 other helper source bodies remain unchanged.

Coverage comprises 52 normal, 28 middle and 28 alternate cases, including 12
allocation failures. Record/actor aliases expose self-changing normal kinds;
explicit fixture interventions change roots or kinds between actor stores.
The real matching allocators and normal/middle/base initializer chain execute.
The tests compare complete synthetic game storage, ordered stores, descriptor
reads, callback arguments/count, helper entries and return values. They check
ESP, preserved registers, return address and caller guards. Ordinary C callee
argument homes can be scratch space and are excluded from semantic equality.

Source guard models are transparent only for these synthetic unregistered
banks and group0. Callbacks are explicit models. Separately, all nine existing
source-only tests execute the actual prefix guards and keep their successes,
allocation failure, asset/callback/group boundary failures and unknown-pointer
behavior. Guard checks now occur at every live asset read; their limits and
failure mechanism are unchanged. Neither fixture establishes original capacities,
real callback behavior, OS startup or complete gameplay.

## Matching work still required

Fresh `/O2` compiler measurements remain non-equal. Corrected guarded
contributions are 672/464/352 bytes against native 272/192/144. A single private
control removes guard calls while preserving the corrected logic; its
544/400/288-byte contributions still differ. Objdiff confirms distinct compiled
contributions, without granting acceptance. Guard removal alone does not solve
matching; keep the production checks and recover the typed source/TU explanation.

The indexed `g_kind_info[1]` source remains an observed prefix rather than a
complete independently proved native extent. The dynamic callback expressions
also fall outside the explicit `rooted-callback-v1` profile. These obligations
remain open; the scalar/indirect-control gates were not changed.

All 438 strict proofs, 43 linked function checks, 321 Python tests and 1,556
supported masked diagnostics pass. The ledger and all accepted records remain
byte-identical. The source graph retains the same providers and unresolved set;
only these three runtime bodies and their evidence metadata change. Full
source-link closure and full matching remain incomplete.

Before/after runtime evidence is in
`evidence/worklist_live_reads_runtime_2026_10_25.json`; current verification hashes
and graph comparisons are in `evidence/worklist_live_reads_progress_2026_10_25.json`.
