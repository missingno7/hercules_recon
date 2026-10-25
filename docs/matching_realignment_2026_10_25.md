# Matching realignment: measured census and pilot

Latest measured continuation: [529 accepted entries / 54,784 bytes](#reference-reuse-continuation), with 629 ENG1 unresolved names. Earlier sections retain their dated baseline/checkpoint figures.

Snapshot: `a29d71a814cce19f0ae59486c767e257c58af777`; `recovery.json` SHA-256 `bebc1468846c1ef12f4e870d46fb017d853ee80883643b751a774a50ee947795`.

Fresh recorded census: 275/275 profiles compiled; 2,237/2,237 rows classified; zero unsupported rows. 254 profiles (2,201 rows) had matching prior source hashes; 21 profiles (36 rows) had no prior proof hash. Source and target guards passed. The initial sandbox attempt failed before compiler launch (WinError 623); the same census then succeeded once with elevated execution.

## Accepted unique extents by target module

| Target | RAW rows / bytes | RELOCATED rows / bytes | LINKED rows / bytes | Unique accepted rows / bytes | `.text` bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| HERCULES.EXE | 28 / 656 | 25 / 896 | 0 / 0 | 53 / 1,552 | 183,742 |
| TITLE.DLL | 6 / 352 | 0 / 0 | 43 / 5,360 | 49 / 5,712 | 130,287 |
| ENG1.DLL | 78 / 6,128 | 224 / 25,168 | 0 / 0 | 302 / 31,296 | 306,908 |
| ENG3.DLL | 25 / 1,712 | 58 / 8,752 | 0 / 0 | 83 / 10,464 | 199,692 |

Total is 487 distinct accepted ledger rows / 49,024 bytes across 820,629 file-backed executable `.text` bytes. Intervals do not overlap. Two TITLE LINKED rows retain earlier RAW proofs. The root-owned strict verifier passed 446 proof checks; a separate fresh 43-row LINKED verification passed.

## Fresh compilation categories

`Reference-only` means only supported COFF relocation fields differ; no destination binding follows from it. Counts below subtract all currently accepted RAW, RELOCATED, and LINKED statuses.

| Target | Exact, unaccepted | Reference-only, unaccepted | Nonreference differences | Of these, <=16 differing bytes | Extent differences |
| --- | ---: | ---: | ---: | ---: | ---: |
| HERCULES.EXE | 8 / 192 B | 271 / 30,816 B | 58 / 24,064 B | 20 / 6,000 B | 13 / 4,304 B |
| TITLE.DLL | 0 / 0 B | 296 / 84,448 B | 17 / 26,496 B | 1 / 704 B | 0 / 0 B |
| ENG1.DLL | 4 / 341 B | 557 / 122,851 B | 54 / 34,176 B | 8 / 3,648 B | 0 / 0 B |
| ENG3.DLL | 7 / 533 B | 418 / 99,930 B | 47 / 29,712 B | 13 / 6,992 B | 0 / 0 B |

Across targets: 19 exact-but-unaccepted / 1,066 B; 1,542 reference-only-but-unaccepted / 338,045 B; 176 nonreference rows / 114,448 B (42 within 16 differing bytes, covering 17,344 B with 108 mismatch bytes); and 13 EXE extent differences (4,304 target bytes vs. 3,984 compiled). The 1,889 total reference-only rows also include 307 accepted RELOCATED and 40 accepted LINKED rows.

## Text coverage and attribution

| Target | File-backed `.text` bytes | Ledger extent union (raw bytes) | CRT/MFC member union (raw bytes) | Arithmetic unclassified residual | Fresh map leads |
| --- | ---: | ---: | ---: | ---: | ---: |
| HERCULES.EXE | 183,742 | 60,928 | 97,074 | 26,076 | 8 / 3,136 B |
| TITLE.DLL | 130,287 | 116,656 | 13,565 | 354 | 0 / 0 B |
| ENG1.DLL | 306,908 | 188,664 | 23,669 | 95,279 | 313 / 95,208 B |
| ENG3.DLL | 199,692 | 140,639 | 31,255 | 28,278 | 74 / 27,825 B |

Pinned-library search found 1,674 contribution hits and 165,563 unique member bytes across `libcmt.lib` and `nafxcw.lib`; the latter appears only in HERCULES.EXE. Recovery extents and library spans overlap by 1,808 B. Their union covers 670,642 B; the remaining 149,987 B is unclassified `.text`, not a missing-function estimate.

Fresh function maps use the current library scan and filter same-entry size mismatches and spans crossing recorded extents. ENG1 reports 1,231 heuristic inventory spans separately from 965 compiled provider RVAs/mapped provider addresses and 1,042 external function symbols; these counts do not share a game-byte denominator. TITLE has 362 reviewed source-graph function entries. Remaining leads are heuristic extents, not accepted functions; ENG1 had one size-mismatched start and one lead inside a RECONSTRUCTED span, both excluded. The map cutoff uses a 75% trailing-library coverage heuristic.

The library search identifies candidate pinned CRT/MFC members but does not exhaustively distinguish compiler helpers or assembly-origin code. The stronger existing [RTM library audit](../evidence/library_attribution_2026_10_20.json) also covers CTL3D32S and attributes 114,185 strong member bytes inside the EXE's 119,678-byte library block; the ordinary map above covers only 97,074 bytes there. Neither map supplies a complete first-party denominator. Original assembly and compiler helpers remain explicitly unclassified where there is no independent attribution. No global semantic-completeness counter exists.

## Accepted source bytes and library-map intersections

The 487 accepted rows / 49,024 B remain the official accepted ledger total. Comparing those extents with the union of current pinned-library spans finds 445 rows / 45,120 B with no known `libcmt.lib` or `nafxcw.lib` overlap, and 42 rows / 3,904 B that touch mapped spans. The latter contain 688 B of signature overlap: 12 wholly covered rows / 208 B and 30 partially overlapping rows / 3,696 B of whole extents (480 B of partial overlap). Treat these as **uncertain library-signature collisions**, not confirmed vendor members; short generic RET/NOP contributions can collide. No bytes are deducted from accepted coverage on this basis, and ledger bytes are not advertised as a fully attributed game-source denominator.

| Target | Accepted rows / bytes | No mapped-library overlap | Uncertain touching rows / whole extents | Signature-overlap bytes |
| --- | ---: | ---: | ---: | ---: |
| HERCULES.EXE | 53 / 1,552 | 48 / 1,456 B | 5 / 96 B | 80 B |
| TITLE.DLL | 49 / 5,712 | 42 / 5,024 B | 7 / 688 B | 112 B |
| ENG1.DLL | 302 / 31,296 | 282 / 29,344 B | 20 / 1,952 B | 320 B |
| ENG3.DLL | 83 / 10,464 | 73 / 9,296 B | 10 / 1,168 B | 176 B |

The archive scan does not prove that the original linker selected a library member for an overlapping game function, so no accepted row is classified as confirmed vendor code by signature matching alone.

## Source graph and codegen queue

The source tree has 625 current source/header files carrying 2237 ledger rows. Source-path counts are `pc` 187 files / 403 rows / 53 accepted; `title` 61 / 440 / 58; `eng1` 312 / 882 / 262; `eng3` 21 / 443 / 64; `shared` 44 / 69 / 50. These are provider-path counts, not target-module coverage.

TITLE source graph receipt is hash-current: 38 sources, inventory/recorded/compiled function counts 362/362/362, link exit 0, zero unresolved and zero duplicate function rows (four duplicate string providers remain represented); its reviewed natural-link plan has 36 objects. This closes the selected graph only; it does not prove whole-DLL equality or complete game semantics.

The ENG1 baseline had 248 sources, 965 mapped providers, 1,042 external function symbols, 726 unresolved names (431 function / 295 untyped), no duplicate providers/placeholders, and 78 RAW peers preserved. Link exit 25; `source_complete=false`, `playable=false`. The source/config changes and final graph are reported below. EXE and ENG3 have no reviewed complete link plan.

The 19 exact diagnostics are not 19 promotion-ready functions: the two five-byte `/Od` sound slices lack independently bounded contributions, the EXE `comdat_63d0` is an MFC/header slot, and several units contain reconstructed peers that the existing-unit gate deliberately does not adopt. No such row was relabeled to bypass those obligations.

## The worker loop

Use one current source tree and the existing tools. Before another experiment:

1. Read the current ledger row and source, entry-decoded original instructions, callers/callees, compiler profile, cross-module twins, and the negative findings. Use `region.py packet` when indexed, otherwise `leaf.py packet`.
2. Classify the first blocker: semantics/control flow, signedness/width, access/store order, ABI, reference/layout identity, frontend/TU context, allocation/scheduling, boundary, or vendor code. An old residual is not a current measurement.
3. Read the fresh census group once. `census.py --recorded --jobs 2` now retains each diagnostic object's path, command and complete hash, with mutation rejection. Reuse it for inspection; promotion still recompiles independently. Reference-only candidates go to owner/reference review without another C-body sweep.
4. If computation differs, correct it first and use the existing focused differential or `leaf.py residual` to inspect the first meaningful difference. If computation agrees, state one predicted instruction effect and a falsifier before a bounded natural source transformation. Stop repeated outputs and exhausted families.
5. Run the selected unit/reference gate and complete contribution/peer checks privately. Submit source identity, native identity, bindings, positive/negative results, compile count, and exact remaining evidence. Only the integrator publishes through `similar.py`/`promote.py` and verifies all accepted categories at integration boundaries.

No semantic-IR framework was needed for these pilots: their first blockers were reference ownership and an inventory bug. The existing residual and differential tools remain the semantic diagnostic path. Behavioral agreement never supplies a matching verdict.

Luna xhigh workers owned disjoint TITLE, ENG1 dependency, ENG1 reference, ENG3/EXE, census, and small tool/test tasks. Their private compiler budgets were 12â€“35 calls, with explicit separate allowances for one full census and the baseline/final module graphs. A single integrator owned configuration, binding publication, recovery state and commits. Sol high inspected the exhausted compiler question, then audited the shared interface owner; repetitive caller application returned to Luna. There is no new routing system or second ledger.

The source-only RAW adoption fix counts unique emitted symbols across target modules and the already pinned co-defined inventory. It checks every target row, every supported masked peer, complete COFF contributions, source/include context and full object inventory. Existing RELOCATED peers retain their full independent reference checks and unchanged records. Missing/extra/duplicate contributions, inconsistent helper inventories, changed references, and RECONSTRUCTED peers still reject. Pinned helpers and data gain no acceptance.

## What transferred from the reference projects

Current implementations were inspected at pinned revisions:

- [Rayman `tools/match.py`](https://github.com/missingno7/rayman_recon/blob/d1c9d9716c0c4001c8c4efdfde23e9bb7ad8df04/tools/match.py) keeps exact instructions, bound references and layout separate; [`tools/difftest/report.py`](https://github.com/missingno7/rayman_recon/blob/d1c9d9716c0c4001c8c4efdfde23e9bb7ad8df04/tools/difftest/report.py) reports the first memory/call/return divergence without granting STRICT. Its committed status reports 1,581 STRICT functions / 290,301 bytes, including 109 original assembly entries; that is not a fresh Hercules-style verification.
- [Blupi's current context probe](https://github.com/missingno7/blupi_recon/blob/3dc368c15ba4a01832348f3261d6e4df6ccbd78a/tools/ctxsweep_tu.py) is diagnostic. Its current context revisit records 40 successful controls and no new STRICT function. This supports bounded causal probes, not declaration floods or importing SP3 rules into RTM.
- [Blood II's object locator](https://github.com/missingno7/blood2_recon/blob/77f088e6c773ca43d91276530fbc4547e6b302a5/scripts/objmatch.py) masks references without proving their targets; its [VC5 mechanisms](https://github.com/missingno7/blood2_recon/blob/77f088e6c773ca43d91276530fbc4547e6b302a5/docs/vc5_codegen_map.md) are hypotheses until reproduced in Hercules.
- [ai_decomp's current consolidation](https://github.com/missingno7/ai_decomp/blob/7c508ff8a1082275d79b12fdcae468ed45e579e5/docs/consolidation-2026-10.md) removed the broad research machinery after it produced no STRICT gain. The useful inheritance is the compact matching skill and decision-changing evidence.

Hercules keeps its pinned RTM compiler, C/C++ contexts, COFF/PE decoder, static CRT/MFC attribution, module identities and existing acceptance gates. No external compiler rule is claimed transferred merely because these sources describe it.

## Initial pilot results

| Workstream | Unique functions promoted | Complete bytes | New verified reference uses | Focused compiler calls |
| --- | ---: | ---: | ---: | ---: |
| Unchanged RAW units, ENG1/ENG3 | 3 RAW | 560 | 0 | 0; reused the common fresh census |
| ENG1 existing direct callers | 8 RELOCATED | 992 | 27 | 9 |
| TITLE interface-field clients | 4 RELOCATED | 528 | 22 | 5, including the Sol owner audit |
| ENG1 collision dependency | 0; one new NEAR provider | 208 source bytes, unaccepted | 0 accepted | 11, including the existing-provider alias probe |

The RAW entries are `eng1.02b30`, `eng3.shared_07f10`, and `eng3.29e50`.
The ENG1 reference batch is `00e470`, `00e560`, `00ee70`, `00eee0`, `2a3d0`,
`2a460`, `2a660`, and `3c1e0`. TITLE adds `18f80`, `19040`, `194e0`, and `196c0`.
All 487 prior accepted records remain unchanged. No game body was retuned to
obtain these 15 acceptances. Complete compiler padding remains included.

TITLE's real COMMON owner is 1,000 bytes at `2bb40`. Native startup independently
copies 250 DWORDs into it; the last wrapper reads its final DWORD at offset 996.
The existing verifier accepts bounded direct DWORD accesses at +16/+20 without
any gate extension. Wrong base, wrong extent and a dependent anchor reject.
The independently reviewed `0c840` entry closes the additional two callers;
its one-DWORD cdecl ABI is checked outside all four pending callers. This proves
the owner and references, without accepting interface dispatch wrappers or
claiming pointee storage or service behavior.

ENG1 now selects the genuine [collision query](../src/eng1/collision_query.c)
at `200e0`. It calls the real bounds provider once, preserves its early failure,
signed WORD normalization and mutation of the passed area, and checks all six
separating planes. Its complete contribution is 208 bytes with 47 nonreference
differences; it is **NEAR and has no accepted ledger row**. Twenty-four finite
original/source Unicorn cases agree with the bounds service modeled identically.
This is source/dependency progress, not a matching promotion or universal
behavior proof. The already present `266a0` provider also gains a reviewed
legacy spelling alias, without replacing its implementation.

The final genuine ENG1 build compiles 249 units and supplies 966 mapped provider
RVAs / 1,043 external function symbols. Unresolved names fall **726â†’723**:
428 function and 295 untyped symbols. Only `_resolve_collision`, `_query_collision`
and `_eng1_200e0` disappear; no unresolved name or duplicate provider is added.
All 79 current ENG1 RAW rows survive, and no generated placeholder is selected.
The incomplete inventory has 558 entries, including 388 unimplemented-function
entries; it is not an exhaustive count of the remaining 723 symbols.
The link still deliberately fails with exit 25 and produces no standalone DLL.
The graph's compiler/source/config inputs remain identical after the subsequent
status-only promotion; accepted counts come from the current ledger separately.

| Target, after the initial pilot | RAW rows / bytes | RELOCATED rows / bytes | LINKED rows / bytes | Unique accepted rows / bytes |
| --- | ---: | ---: | ---: | ---: |
| HERCULES.EXE | 28 / 656 | 25 / 896 | 0 | 53 / 1,552 |
| TITLE.DLL | 6 / 352 | 4 / 528 | 43 / 5,360 | 53 / 6,240 |
| ENG1.DLL | 79 / 6,368 | 232 / 26,160 | 0 | 311 / 32,528 |
| ENG3.DLL | 27 / 2,032 | 58 / 8,752 | 0 | 85 / 10,784 |

Initial checkpoint: **502 unique accepted entries / 51,104 bytes**, up 15 / 2,080 bytes:
140 RAW, 319 RELOCATED, and the same 43 LINKED entries. The two retained prior RAW
proofs are checked separately without double-counting their LINKED rows.
These are target-entry/contribution counts, not a source-identity or whole-image claim.

## Efficiency and verification

The focused reference harvest promoted 12 of 12 selected functions in 14 compiler
invocations: **85.7 functions / 10,857 accepted bytes per 100 focused calls**.
The nine-call ENG1 cost includes a redundant three-call positive receipt rerun;
it is counted, not hidden. The dependency pilot spent 11 calls for one genuine
source provider plus one existing-provider binding, resolving three names.
The exhausted `186e0` question spent zero compiler calls: its current two-byte
sx/x0 load-order residue already survived 462 earlier declaration placements,
types, expression orders and header/count controls. No new discriminator was
found, so another sweep was rejected.

The common baseline and protection costs are separate: 275 census profiles,
116 initial STRICT profiles, 36 initial legacy-link objects, 248 initial ENG1
profiles; RAW promotion 117 STRICT + 36 LINKED; relocated promotion 118 STRICT
+ 36 LINKED; final STRICT 118. Two 249-profile final ENG1 builds are counted:
the second pins a corrected diagnostic inventory exactly. Three full regression
suite runs used eight real compiler controls each; the census object-reuse
control used one compiler call. Including focused work, this is **1,648 historical
compiler invocations** for this checkpoint, or 0.91 new accepted entries /
126.2 accepted bytes per 100 calls. Failed sandbox process launches are separate
environment failures, not candidate compiles. Model token usage was not exposed.

The earlier refinery's four hard controls used 70 development variants plus seven
baselines, each with a semantic compilation, and accepted none of those four.
Its blind region also had successful leaves, so this is not an apples-to-apples
speedup claim. This pilot demonstrates accepted yield and dependency closure from
reference-first selection; sustained throughput still needs measurement over
further batches. Do not repeat the common census or every module build after an
unsuccessful local hypothesis.

Both initial promotions passed the separate 43-row LINKED gate with zero failures; its
86 existing source/image provenance drifts remain unchanged. The final STRICT
gate at that checkpoint checks all 461 RAW/RELOCATED obligations with zero failures. All 333
repository tests pass. Original assets and binaries were comparison inputs only;
no original host/module process was launched.

## Next queue, ordered by concrete remaining proof

1. **Completed in the continuation below:** apply the complete-interface-owner review to `eng1.shared_318d0`,
   `eng1.shared_31990`, `eng1.shared_31e30`, and `eng3.shared_247e0` (624 bytes).
   Independently establish each module's native copy base, real source owner,
   terminal extent and actual helper entries. TITLE-labelled symbols must not
   imply the same numeric RVA in a different module; the old wrong-helper
   substitution remains rejected. This is a bounded follow-up, not assumed acceptance.
2. Audit the remaining direct-call-only, one-identity-blocked families, such as
   ENG1 `113c0`/`27410` and ENG3 `27d70`/`280b0`â†’`01460`, from callee-own source,
   native entry and outside callers. Stop immediately on missing independent ABI
   or owner evidence rather than retuning already equal callers.
3. Resolve actual ENG1 data/provider ownership among the remaining 428 function
   and 295 untyped names. Reuse established providers first, as for `266a0`;
   prioritize families with multiple real consumers and explicit native contracts.
4. Keep `200e0` as one canonical NEAR implementation. Reopen only for genuine TU,
   frontend or allocation evidence; direct/negated delta, sequential OR and the
   accepted-neighbor plane ordering already emit the same residue. Do not run a
   six-plane permutation grid.
5. The EXE GPU/GTE exact leaves remain useful after their real mixed-unit
   ownership is reviewed; RECONSTRUCTED peers are still a gate boundary. Exclude
   `comdat_63d0`, unsupported five-byte sound slices and ambiguous RET/NOP shims
   from the cheap game-code queue. Do not reopen `186e0` or ENG3's relative-vector
   algebraic sweep without the new evidence specified in their negative records.

## Scaled continuation: real zero-section owners

The first queued transfer closed all four functions, adding **624 bytes and 24
verified references**. The first probe found the real shared owner emits one
1,000-byte `.bss` section, rather than COMMON. Its initializer and source remain
unchanged. The explicit `zero-section-v1` profile proves this complete data-only
C object through its exact section extent, unique strong symbol, strict section
flags/auxiliaries and independent native copy/zero-tail identity. It requires
no payload, relocations, additional storage, functions or aliases. The default
COMMON check and all instruction/reference comparisons remain intact.

Luna audited the two module-specific bases and nine actual helper entries;
Sol supplied the bounded owner mechanism and controls. All 27 outside-call
witnesses were checked directly against immutable original bytes. TITLE-labelled
COFF names bind to each engine's own helper entries, not TITLE RVAs. Wrong-module
callee, wrong-base/extent, extra-storage and hidden COMDAT/section-flag controls
reject. These are shared-source function proofs and add no module dependency
closure; the ENG1 source build excludes cross-module identity rows.

| Target, current | RAW rows / bytes | RELOCATED rows / bytes | LINKED rows / bytes | Unique accepted rows / bytes |
| --- | ---: | ---: | ---: | ---: |
| HERCULES.EXE | 28 / 656 | 25 / 896 | 0 | 53 / 1,552 |
| TITLE.DLL | 6 / 352 | 4 / 528 | 43 / 5,360 | 53 / 6,240 |
| ENG1.DLL | 79 / 6,368 | 235 / 26,640 | 0 | 314 / 33,008 |
| ENG3.DLL | 27 / 2,032 | 59 / 8,896 | 0 | 86 / 10,928 |

Current total: **506 accepted entries / 51,728 bytes**, up **19 / 2,704 bytes**
from the baseline: 140 RAW, 323 RELOCATED and 43 LINKED. The continuation preserves
all 502 preceding accepted records unchanged. Its fresh protected promotion
passes **465 RAW/RELOCATED obligations in 119 compiler calls**, plus the same
43 legacy LINKED rows with zero failures and 86 unchanged provenance drifts.
All **356 tests pass**, including 44 focused zero-storage tests.

The continuation adds six private compiler calls (four failed preflights and
two positive profiles), eight compiler controls in the final regression suite,
and 119 + 36 protected promotion builds. The cumulative count is **1,817 compiler
invocations**: 1.05 new accepted entries / 148.8 accepted bytes per 100 calls.
Reference-directed work alone is 16 accepted entries / 2,144 bytes in 20 calls.
The measured source/dependency result stays 966 mapped ENG1 providers and 723
unresolved names; the new 208-byte query remains NEAR. Continue with queue items
2â€“5, retaining their stated evidence and reentry limits.

## Scratch files

- `recorded.json`: fresh profile/row report.
- `library_map.json`: fresh pinned-library attribution scan.
- `function_map_*.json`: current heuristic spans.
- `convergence_with_maps.json`: filtered lead counts and annotated families.
- `summary.json`: machine-readable metrics and input hashes.

## Reference reuse continuation

From `569597f` the current ledger gains **23 RELOCATED functions / 3,056 bytes**
and **116 verified reference uses**. All 506 preceding accepted records and all
524 records at the intermediate checkpoint remain unchanged. No comparison rule,
function body or compiler setting changed to obtain these matches. Five new
independent identities were established: three ENG1 callees, TITLE's exact
12-byte screen/projection owner, and the pinned MFC DDX_Control entry. The MFC
body supplies identity evidence only; accepted EXE bytes belong to four original
option-dialog methods.

| Target, current | RAW rows / bytes | RELOCATED rows / bytes | LINKED rows / bytes | Unique accepted rows / bytes |
| --- | ---: | ---: | ---: | ---: |
| HERCULES.EXE | 28 / 656 | 29 / 1,424 | 0 | 57 / 2,080 |
| TITLE.DLL | 6 / 352 | 6 / 800 | 43 / 5,360 | 55 / 6,512 |
| ENG1.DLL | 79 / 6,368 | 249 / 28,592 | 0 | 328 / 34,960 |
| ENG3.DLL | 27 / 2,032 | 62 / 9,200 | 0 | 89 / 11,232 |

Total **529 / 54,784 bytes**: 140 RAW, 346 RELOCATED, 43 LINKED. This is
**42 functions / 5,760 bytes above the mission's regenerated 487-row baseline**.
Fresh final promotion passes **488 RAW/RELOCATED obligations in 120 compiler
calls**, plus all 43 LINKED rows (zero failures, the same 86 provenance drifts).
Both full 356-test runs pass. Original binaries/assets remain unchanged.

The 275 recorded census source/flag profiles and target hashes remain current;
their unchanged measurements now leave 16 exact-but-unaccepted, 1,503 reference-only,
176 nonreference-difference and 13 extent-difference rows. These are diagnostic
categories, including vendor/uncertain contributions. No second 275-profile
census was needed. There is still no complete attributed game-code denominator.
Two new canonical NEAR providers outside the ledger are tracked separately.

The full **249-unit ENG1 source build** resolves **94 names to existing exact
providers**: 91 callback-pointer spellings plus three direct aliases. All 249
fresh objects preserve section bytes, symbol metadata and relocation offsets/types
after those 94 name substitutions; 54 generated `.file` paths are separately
normalized to their actual compiler input directories. This source-graph comparison
does not alter any STRICT gate. All 79 RAW peers pass. The graph has **629 unresolved
names (334 function / 295 untyped)**, zero duplicate providers, no generated stubs,
966 mapped provider RVAs and 1,043 function symbols. Link exit 25 remains expected;
there is no standalone source-linked ENG1 DLL. Removing 92 duplicate diagnostic
placeholder rows leaves 466 incomplete entries, including 296 unimplemented functions.
Later matching-only status promotions preserve this graph's source/config profiles.

ENG3's canonical `rng_state_01010.c` recovers the actual two 32-bit state cells
initialized to 123/456 and their native update order. Its object supplies the
previously undefined symbol in an existing caller. The complete 64-byte contribution
still has 36 nonreference differences: **NEAR**, not accepted matching. The 1,726
finite arithmetic cases exclude zero limits and are not whole-module behavioral proof.
No complete ENG3 dependency/link graph is claimed.

| Bounded workstream | Accepted functions / bytes | Focused compiler calls | Other result |
| --- | ---: | ---: | --- |
| Luna ENG1 direct references | 5 / 544 | 3 | Three new callee identities |
| Luna TITLE owner harvest | 2 / 272 | 6 | Complete 12-byte native range; wrong base/extent reject |
| Luna EXE DDX family | 4 / 528 | 2 | Unique pinned MFC public/body/ABI identity |
| Luna known-reference reuse | 12 / 1,712 | 17 | Existing identities; fresh caller/owner gates |
| Luna ENG1 dependencies | 0 | 4 + 249 full-build | 94 names closed; no new bodies counted |
| Luna ENG3 source recovery | 0 | 6 | One genuine 64-byte NEAR provider |
| Sol interface blocker | 0 | 3 | Falsified a tempting 97-function acceptance route |

Focused matching yields **23 functions / 3,056 bytes in 28 compiler calls**
(82.1 functions / 10,914 bytes per 100 calls), sustaining the preceding focused
16 / 2,144 in 20 calls. Including the baseline, two protected promotions/LINKED
checks, full source build, both complete test runs and unsuccessful probes costs
**779 historical compiler invocations** (2.95 functions / 392 bytes per 100).
Each test run includes 29 real compiler launches, rather than only its eight added
pointer-type controls. Earlier all-in totals used narrower fixture accounting;
they do not establish an end-to-end speed comparison. Token/currency cost is unavailable.

The useful loop is now demonstrated twice: reuse hash-current evidence for triage,
classify the first actual blocker, copy only previously independent identity metadata,
audit new owner/callee evidence outside the pending callers, run the focused fresh
gate, then batch journaled integration and all-category protection. Luna xhigh owns
bounded disjoint source/identity tasks and private scratch. Sol high gets one shared
causal question. The integrator alone publishes bindings, config, ledger and commits.
No routing framework, permuter campaign or new comparison capability was added here.

Sol's three-compile control stops **97 instruction-equal wrappers / 2,160 bytes**.
The existing rooted-callback profile cannot represent their fixed-member CALL/JMP.
More seriously, ENG1 `2c310` forwards two DWORDs while static host slot `+0x4c`
points to a five-argument provider; the bounded native fixture demonstrates the
argument mismatch. A tail jump to the static empty callback proves only that
fixture. Acceptance needs independent live-table lifetime/mutation and provider ABI
evidence; a blanket BSS indirect-call exception would be unsound. The gates stay intact.

Next work, in payoff order:

1. TITLE `03050`: independently establish `03100`, `03420`, `03740`, `039b0`
   identities/ABI. Its complete caller instructions and screen-state owner already
   agree; spend no further compiles on the unchanged caller body.
2. ENG1 `27410` (64 bytes) and `27450`: four real consumer units each. Recover
   the missing helper semantics with original-backed controls and try matching immediately.
3. ENG1 `26de0` and `1aba0`: five consumer units each; assign one bounded coherent
   provider family, with source ownership before compiler steering.
4. ENG3 `01460`: recover the actual 416-byte helper and independent Actor ABI to
   unlock `27d70`/`280b0`. Reuse the new RNG provider where its symbol is required.
5. Keep the 97 wrappers deferred at the recorded table/ABI evidence boundary.
   Exclude exhausted TITLE `186e0` declaration sweeps, ENG1 `200e0` delta/plane
   permutations and ENG3 relative-vector algebra/frontend sweeps without new evidence.

Reproducible receipts: [continuation metrics](../evidence/strict_continuation_progress_2026_10_25.json),
[callback aliases](../evidence/eng1_callback_aliases_2026_10_25.json),
[direct aliases](../evidence/eng1_direct_aliases_2026_10_25.json),
[RNG source evidence](../evidence/eng3_rng_provider_2026_10_25.json), and
[interface blocker](../evidence/interface_wrapper_blocker_2026_10_25.json).
`recovery.json` remains the only matching authority.
