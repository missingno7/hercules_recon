# Release dependencies and engine callback binding

This continuation works on the real dependencies of the earlier natural-link
interval. The PC binaries remain the oracle and the object acceptance gate is
unchanged. Experimental diagnostic equality does not establish linked layout.

## Actor release repair

The preserved first `release_actor` hypothesis was semantically wrong. At ENG1
`0x27e20`, sidecar flags `0x100` and `0x80` control independent copy predicates.
Every nonblocked sidecar then reaches child/buffer/attachment detachment, actor
kind clearing, sidecar high-bit clearing and actor clearing. The earlier C nested
the predicates and incorrectly reached `remove_actor` on other combinations.

The special-kind byte is sign extended and compared as a 16-bit word. This matters
for profiles `0xff80..0xffff`; a promoted unsigned-short versus signed-char
comparison gives different behavior. The second predicate reloads actor profile,
and cleanup reads sidecar flags again after callbacks. These details preserve
aliasing and callback-visible state observed in the PC instruction sequence.

`calibration/release_actor_repaired.c` reproduces the **complete 304-byte
contribution diagnostically**, with zero normalized differences, 98 matching
instructions, the same 17-block CFG and all ten direct-call destinations. Strict
raw equality remains false: 40 raw differences, 13 object relocations and three
target base relocations. It receives no recovery promotion.

The widened target-derived harness passes **2,244,629 checks**, covering all
16-bit profiles, flag combinations, negative signed-byte values, callback state,
sidecar offsets and overlapping actor/sidecar views. The same harness reports
**1,120,002 failures** against the old hypothesis. The old twelve-case probe is
retained as historical evidence but removed from registered validation. Finite
tests support the repair; complete diagnostic instruction equality is separate
evidence, and neither replaces raw promotion.

Sol high performed this structural diagnosis: two object builds and two semantic
builds, all successful. No local source search remains justified once diagnostics
are equal. The integrator independently recompiled the preserved source and ran
both positive and negative semantic controls. Requested model/effort are known;
runtime identity, tokens and cost are unavailable. `evidence/release_repair.json`
and the immutable region receipt preserve commands, identities and the resume
recipe against the unchanged failed baseline.

## Data needed by natural linkage

The tag table at ENG1 `0x716c0` contains a signed count followed directly by
32-bit actor pointer slots. The referenced initialization callback at `0x1010`
increments count before storing the actor at `base + 4*count`. Its writes also
set tag `0x2a`, flags `0x81` and render flags zero. This is additional data-shape
evidence; it does not prove the array's declared capacity. Adjacent table addresses
alone cannot establish an original declaration boundary.

`0x6fed8` is a selected actor pointer used as a default insertion anchor. The
selection functions clear it, scan candidate actors and store a selected actor;
it is not established as a persistent scene-list head. Both globals occupy
virtual `.data` beyond file-backed bytes. Original data contributions and grouping
remain unresolved. `evidence/link_data_dependencies.json` records instruction
references, reviewed code extents and reproducing queries.

## Callback interface and helper reconstruction

The cells at ENG1 `0x71098`, `0x710a0` and `0x712a4` are slots `0x5e`, `0x60`
and `0xe1` of the interface block starting at `0x70f20`. On its initial nonnull-input
path, `PC_DLLEngineMain` copies **250 dwords** from its first argument, then checks
the copied signature `0x5ac00cac`. ENG3
and TITLE corroborate the same signature and copy length at their exports.

The original CD EXE resolves that named export through `GetProcAddress`, then
passes the table selected by the pointer at VA `0x4365a0`. Its static value points
to VA `0x4361b8`; slots `0x5e`/`0x60` reference host VA `0x40be30`, while `0xe1`
references `0x40c140`. This identifies the static initializer chain. Indexed code
has no direct writes to those source slots, but mutation through computed aliases
has not been excluded and no original image was executed. The `0x40c140` wrapper
forwards a dword plus `-1` to `0x40fa10`, caller-cleans eight bytes and zeroes AX;
its complete prototype and callee meaning remain open. These callbacks must not
be replaced by assumed CRT or Win32 routines.

The original `0x40be30` body is empty: RET with natural NOP alignment to the next
independent entry at `0x40be40`. Ordinary empty C freshly reproduces the complete
**16-byte contribution**, without relocations. The unchanged promotion gate
accepted `exe.empty_host_callback` and freshly verified all **39 accepted
instances**. Totals are **30 source functions / 4,336 mapped bytes**. The target
is the verified original CD EXE; the installed no-CD executable was not changed.
Function equality does not establish parameter types, original names, source
ownership, runtime table immutability or whole-image layout.

Luna xhigh traced the initialization and compiled one bounded empty-body probe.
`scripts/interface_evidence.py` rechecks four immutable images, the table words,
export addresses and thirteen reviewed code spans. Full disassembly remains in
ignored scratch; `evidence/engine_interface_binding.json` retains compact findings.

The separate Luna cleanup pass reconstructed **448 bytes in three functions**:

| Function | ENG1 RVA | Full bytes | First-pass result |
|---|---|---:|---|
| Resource state release | `0x51d0` | 320 | Source/codegen mismatch; normalized differences remain |
| Recursive actor clear | `0x55b0` | 80 | Relocation-only diagnostic equality |
| Token reference release | `0x5750` | 48 | Source/codegen mismatch; return ABI unresolved |

All first-pass sizes and ordered CFGs match; that does not prove the two residual
sources are correct. The 38-check fixtures cover field preservation, recursive
call order, captured flags and sample table updates. A combined table/cache/return
rewrite grew resource release by 32 bytes and worsened output. A separate explicit
pointer-return test preserved peers and passed 26 synthetic checks but worsened
token CFG/equality. All four observed callers discard EAX, so the table address
left in EAX on the active path does not settle its source declaration. Both
families stop pending new field-writer, table-layout or caller evidence.

The worker's metadata mislabeled its requested configuration as Sol high and
reused a report filename. The integrator preserved the private receipts, recorded
the actual requested Luna xhigh dispatch, and recompiled the frozen first-pass
source for a new durable receipt. The later pointer-return experiment also changed
its call-order test stub; the preserved first-pass fixture restores the matching
void declaration. No acceptance decision used those model labels or mutable logs.

## Validation and next boundary

The accepted no-op adds one 16-byte match; actor release and recursive clear add
**384 bytes of newly explained diagnostic-equal source**. The two helper source
residuals, real global data contributions, `remove_actor` and other true callees
still prevent natural module closure. The earlier relocation-only regions remain
frozen. A supported declaration/layout or return-use discriminator is more useful
than further algebraic spelling changes.

The new release suite replaces the shallow historical probe. Full registered
semantic validation now covers twelve suites, plus the explicit negative control;
**2,844,687 checks pass** across those suites. The separate negative control fails
as expected. **26 Python tests**, **12 immutable original hashes**, all 39 fresh
promotion comparisons and the four-image static interface recheck pass. Previous
recovery rows are byte-for-byte unchanged.

Worker discovery used four Sol compiler invocations for release diagnosis, one
Luna invocation for the host callback, and nine Luna invocations across the helper
first pass and two bounded follow-ups. Integration replay and full regression
compiles are additional. One integration fixture compile failed because its stub
retained the rejected return signature; restoring the first-pass void signature
fixed it. Unknown model-only wall time, tokens and costs stay null. These outcomes
do not establish comparative model economics from one continuation.
