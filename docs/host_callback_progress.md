# Host callback reconstruction

The accepted total remains **40 instances / 4,352 bytes**. This continuation
recovers missing source dependencies and corrects two target-behavior errors in
experimental source. No new function passed the raw gate.

The arena reset is callback slot `0x29` in the 250-DWORD host interface. HERC
reads the pointer immediately following the table and passes that table base to
the resolved engine entry. TITLE copies the table to `0x2bb40`; its wrapper at
`0xc4b0` calls copied slot `0x29`, forwarding one caller-cleaned argument. The
observed direct caller passes zero. The callback's nonzero case interprets its
argument as an absolute replacement arena start, adjusts saved bytes by the
address delta, and rebuilds pool-zero bounds/header before coalescing.
`scripts/host_callback_evidence.py` rechecks the table words and critical
instructions without executing either original image.

| Experimental source | Full target / candidate bytes | Normalized differences |
|---|---:|---:|
| Arena reset | 160 / 160 | 18, entry register/dataflow sequence |
| Shrink/relocate | 192 / 192 | 162, control/source residual |
| Corrected bulk free | 112 / 112 | 2, still unaccepted |

The arena fixture passes 18 checks for zero and absolute-start replacement,
snapshot-derived header size, preserved end, and callback/diagnostic arguments.
Its three-block CFG, 40 decoded instructions and direct calls match; this does
not resolve the remaining bytes or prove an original declaration.

Shrink retains an ordinary external call to the identified historical CRT copy
implementation. Its public `memcpy`/`memmove` name and runtime variant remain
indistinguishable. The production candidate does not contain a replacement CRT.
The test fixture alone supplies a byte-copy model to observe arguments and order.
Null and equal-size paths return zero; growth returns the old payload; strict
shrink relocates the header upward, updates the separately captured owner slot,
coalesces and returns the old payload value. Callers inspected here discard EAX;
the source prototype remains a hypothesis.

A one-word remainder reveals the actual PC sequencing: the new descriptor
overwrites old header word one before either transfer path reads that word.
Both paths therefore transfer descriptor bits into the new owner word. The
originally captured owner-slot address still receives the new payload pointer.
The first owner-only candidate incorrectly reused the cached slot value.
Correcting that single source read preserves the target edge and passes 222
checks. A frozen-v1 negative control fails exactly two assertions about that same
owner-word transfer. No gameplay repair or original execution was involved.

Bulk free had an extra owner-slot-address NULL guard. PC loads word one,
dereferences it immediately, then tests only its content. Removing that guard
reproduces the target's 35-instruction, eight-block topology. The existing fixture
uses a valid slot address with zero content and remains unchanged: 34 checks pass.
The compaction wrapper and compactor retain identical complete bytes and ordered
relocations. The compactor's mixed word-plus-current-byte budget is also directly
observed and remains intact.

`python scripts/host_pool_linkage.py` naturally links six pool routines and the
real accepted no-op diagnostic with four observed strings and the inferred
48-byte zero-filled state view. All six freshly compiled pre-link contributions
retain their published identities; strings and virtual zero-fill are checked.
There are no unresolved externals, stubs, original dispatch, object patches,
padding or placement flags. The linked addresses differ from the original:
dependency resolution is proved, original TU/data grouping and image layout are
not. This experiment does not grant `FUNCTION_MATCH`.

The failed first shrink candidate, corrected source, predictions, fixtures,
compiler accounting and compact receipts are retained. Natural module closure
still needs the remaining pool control/code-generation questions, real module
layout and other cleanup/resource dependencies. The long-run goal stays active.

Fresh integration and the final regression pass preserve all **40 raw matches**.
All **26 semantic suites / 3,251,535 checks**, **30 Python tests**, and **12 immutable
original-file hashes** pass. The negative control fails its two expected assertions.
The static callback recheck covers 250 table words and 18 critical instructions;
the existing pool evidence verifier rechecks 35 reviewed spans across two images.
No original executable or DLL was loaded or run.
