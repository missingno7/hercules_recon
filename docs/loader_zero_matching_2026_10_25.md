# Explicit loader-zero reference matching, 2026-10-25

Three functions gained FUNCTION_MATCH_RELOCATED acceptance, covering 384 complete
contribution bytes and twelve references:

| Entry | Bytes | References |
| --- | ---: | ---: |
| ENG1 `19a10`, X motion | 144 | 5 |
| ENG1 `19aa0`, Y motion | 160 | 5 |
| ENG1 `c8c0`, actor initialization | 80 | 2 |

The ledger now contains 383 unique accepted rows / 35,248 bytes: 135 RAW,
205 RELOCATED and 43 retained linked rows. Two linked rows also retain raw proofs
and must not be counted twice. All 380 earlier accepted records remain identical.

## Storage proof

The four-byte pool-pointer owner was audited before support was added. New `bss`
bindings name its actual source/flags/COFF symbol and an independently reviewed
native anchor. Verification recompiles the complete COMMON owner on every run,
pins all its source/header inputs, and checks the exact declared extent. The
native object must lie wholly in a unique writable, non-executable zero tail
inside the loaded image, with no loader fixups or data-directory overlap.
Unreviewed TLS/load-config effects reject the proof.

The anchor is one complete file-backed instruction outside the compared and
selected caller ranges. Its hash, absolute operand and exact original HIGHLOW
fixup must identify the same owner base. A same-zero neighboring cell fails that
identity check. Direct memory accesses must fit the complete owner; indexed
accesses, aliases, one-past pointers and indirect control transfers remain
unsupported for this storage kind. Ordinary `data` records retain their
file-backed payload check. RAW and mixed-unit adoption rules remain unchanged.

Owner profiles are shared only within one verification run, through the same
pinned compiler. Every owner object and compiler input is checked again before
publication. Source/context changes, ambiguous COMMON definitions, partial
owners, overlapping sections and loader-managed storage fail closed.

Twenty-one adversarial tests were added; all 204 Python tests pass. They include positive synthetic proofs,
wrong-cell and stale-header cases, unchanged rejection by ordinary data records,
access-width/index checks, fresh builds on repeated runs, and owner-object
mutation immediately before publication. Fixture PE/COFF files are synthetic.

## Independent targets and regression

The two axis continuations were independently reconstructed and reviewed at
their own native boundary entries. The X reference preserves the original
comparison/store asymmetry. These reference bodies are not production providers:
header/table ownership and capacity remain unreviewed, and their compiled bytes
still differ. Their original/native body roles and 565 finite isolated/composed
cases establish the reviewed entry identities; caller relocation values did not
select them. Other services remain modeled and no game/host process was launched.

For `c8c0`, the frame helper at `29270` was freshly recompiled and compared over
all 80 bytes under its existing RAW proof. The caller retains flag writes,
frame advance, direction/render updates and the live pool-Z read in original order.
Actual wrong-neighbor and wrong-callee controls both reject the hypothesis.

Journaled promotion checks 342 RAW/RELOCATED proofs and all 43 older linked function
checks. The existing 86 broader linked provenance drifts remain unchanged.
The remaining 1,637 supported diagnostic rows pass in 200 sources. This does not
establish dependency closure, source identity, complete BSS support or whole-game
behavior. The active full-matching goal remains incomplete.

Frozen bindings and identity evidence are in
`evidence/loader_zero_axis_bindings_2026_10_25.json`,
`evidence/loader_zero_axis_references_2026_10_25.json`,
`evidence/loader_zero_pool_caller_bindings_2026_10_25.json`, and
`evidence/loader_zero_pool_caller_2026_10_25.json`.
Final counts and report hashes are in
`evidence/loader_zero_matching_2026_10_25.json`. Recovery.json alone grants acceptance.
