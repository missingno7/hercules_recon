# Hercules process exploration — 2026-10-11

The exploration found credible improvements in build representation, verification efficiency and work ordering. It did **not** find evidence justifying a global compiler, service-pack or optimization change. Routine matching should stay paused until the first process tasks below are implemented and measured.

This is an audit of commit `9306aa0288d6b064f8a555ca8acf7b28e401c6f1` **plus its preexisting working-tree changes** to `title_proc.cpp`, eight recovery proofs, boundary metadata and the TITLE scheduler evidence. Those changes were frozen, not adopted or altered by this exploration. `recovery.json` remains the only recovery authority. No new function, source promotion or strict-byte gain is claimed.

Four independent Phase A investigations covered compiler archaeology, object/link architecture, residuals and verification. The supervisor consolidated them before selecting two Phase B hypotheses, capped at twelve historical compiler invocations and zero linker invocations. Private measurements, objects and verbose inventories remain under `work/process_audit_53/`; the compact evidence companion records their identities and commands.

## A. Verified facts

1. **VC5 is the best-supported family, rather than a uniquely identified revision.** Current RTM and SP2 driver/frontend/backend/linker/library pins agree with the manifest. Their CL drivers are identical while their backends differ. Retained CRT controls reproduce complete `rand`, `__getptd` and `__initptd` contributions outside actual relocations under both libraries; single-threaded `LIBC` rand has 35 nonrelocation differences. This establishes the tested LIBCMT family, not every original TU's runtime switch or exact library revision. The original PE linker field is 5.0; it does not identify CL or the precise LINK build. See `docs/toolchain.md`, `evidence/crt.json` and the compiler audit.

2. **`/O2` enables `/Gy` in the pinned historical driver.** Direct inspection of `cl.exe` finds the O2 option-table pointers at file offsets `0x5660/0x5664` and its expansion ending in `-Gy`. Removing the wrapper's explicit `/Gy` while keeping `/O2` is not a no-Gy experiment. `/O1` also includes Gy in that table; `/Ox` does not. That observation alone establishes neither native flags nor how an overriding option is parsed.

3. **There is positive regional evidence against one universal optimized-function profile.** Eight corresponding engine FX intervals total 5,343 bytes per engine and agree outside independently identified reference operands. Their stack loops, reloads, EBP frames and naturally unaligned internal entries form a coherent low-optimization signature. Retained `/Od` plain-text fragments reproduce complete native code and literals in both engines: the prefix/runtime controls cover 1,127 and 942 bytes, and the four-function tail covers 345 text bytes plus 71 literal bytes. This proves compatible generation within those bounds, not exact original `/Od`, full TU ownership or placement. EBP alone would not establish optimization.

4. **The default acceptance representation is intentionally narrower than the target.** `compile_source` inserts `/Gy`; `COFF.function` requires a single function at offset zero in one COMDAT. The raw gate also requires zero object/native relocations. The initial promotion path supports self-contained C and rejects ordinary data contributions. `masked_unit_compare` separately handles complete plain text with explicit function offsets and mapped initialized data. It remains diagnostic. There is no production naturally linked raw-image acceptance path.

5. **Two older architectural premises are unsound as universal rules.** `function_map.py` filters unreviewed starts to 16-byte boundaries, so its aligned census cannot identify native `/Gy`. `unit_bounds.py` assumes ordinary C BSS means file-static storage, but the project's explicit-zero external C controls already falsify that premise. Shared BSS users are conditional TU clues, not a universal same-object proof. The older communal-tail threshold `0x2b370` is also superseded by evidence of ordinary LIBCMT BSS beginning at `0x2b3d8`.

6. **The current and old TITLE build inventories are different experiments.** The current canonical graph has 39 sources, 340 functions and 96 unresolved names: 21 code and 75 data. It produces no image. The old private graph has 362 spans and 32 objects, includes draft source, and uses a forced image for layout. Equal extents in that old image were not raw equality: independent reads find 5 differing `.rdata` bytes, 238 `.idata` bytes and 789 bytes in its corresponding library block. The forced file hash is `e891dbdde439573b8024e13fdd31c0ee5117de3603d4dffcfa4c719635d9a3a7`. Its ordinal/member-relative layout remains useful evidence, with this narrower claim.

7. **Repeated identical compilations are real.** Current strict verification has 132 rows but only 30 source/flag groups. A historical 121-object strict receipt has 28 identical-input groups, with all complete parsed contributions repeating; that receipt contains 93 repeated same-input compilations. This is an invocation count, not a measured token or wall-time saving. The masked verifier already groups its sources, and frame diagnostics already reuse freshly pinned objects within one experiment.

8. **Some “compiler failures” were source errors or incomplete controls.** Examples include the movie opener's frame-count field, reversed raster `memcpy` operands, graphics branch order and explicit full SDK clear ranges. The recent reduced HRESULT MRE did not preserve the required EBX-zero live range; its negative result cannot establish a compiler law. TITLE call-boundary controls reproducibly failed their prediction but did not exclude unknown original context.

## B. Open assumptions and discriminators

| Assumption | Current limit | Useful discriminator |
|---|---|---|
| Exact RTM/SP2 or another VC5 revision | The initial anchors and selected retained controls do not distinguish them; some older all-version claims lack per-arm source/object receipts | Independently selected native anchors with a stable reconstructed full context and a predicted revision-specific difference; repeat across multiple controls before changing a profile |
| One compiler/optimization profile per DLL | Packed engine fragments already contradict the default representation's universality | A region inventory of real entries, contiguous contributions, literal/data ownership and independently matching controls; no global flag sweep |
| Original C/C++ language | Local language effects are measured, but some historical “C-only” claims changed with declaration context | Real headers, definitions, ABI/decorations and peer contributions in a coherent unit; a synthetic language success is not source identity |
| Original `/Gy`, packing, inlining, CPU or FP switches | Alignment, SDK struct sizes and current `/O2` compatibility do not establish these settings globally | Native plain/COMDAT-compatible full units, real call-versus-inline helpers and multiple independently selected packing/FP anchors |
| TU boundaries inferred from BSS/literals | External explicit-zero BSS can cross TUs; COMDAT literal coalescing and missing data complicate ownership | Defining-object storage classes, complete noncode contributions, literal payloads, reference identities and actual link roots together |
| Original object/library order and REF/ICF flags | Code-order sorting and NOREF/NOENTRY are diagnostic recipes; unused functions do not prove NOREF | Frozen real-provider inventory with root/export evidence and ordinary link measurements, keeping private and canonical graphs distinct |
| Full build reproducibility/equality | Contribution repeatability excludes COFF timestamps and paths; forced image shape is not whole-file equality | Two clean natural builds with recorded executable/data/import/export/resource/header identities; timestamps/debug metadata need explicit treatment before any whole-file claim |

No current experiment proves that a different compiler, backend, global flags or build architecture would generate more correct game code.

## C. Root-cause map and byte scope

The following is a **disjoint partition of all four native executable sections**, including runtime/library code and inline data, excluding file-alignment tails. A sampled family is a known investigation interval, not an established causal diagnosis. Unclassified bytes are not automatically missing game source.

| Module | Native code | Strict | Relocation-only | Other recorded | Additional sampled intervals | Unclassified here |
|---|---:|---:|---:|---:|---:|---:|
| TITLE | 130,287 | 352 | 80,928 | 352 | 26,896 | 21,759 |
| ENG1 | 306,908 | 5,632 | 2,368 | 272 | 5,903 | 292,733 |
| ENG3 | 199,692 | 1,712 | 2,080 | 816 | 6,639 | 188,445 |
| EXE | 183,742 | 608 | 4,544 | 0 | 4,160 | 174,430 |
| **Total** | **820,629** | **8,304** | **89,920** | **1,440** | **43,598** | **677,367** |

The 29 reading-list families contain 69 member mentions but only 66 unique module/RVA members. Their full interval union is 45,998 bytes; 2,400 bytes overlap already recorded categories and are removed from the additional-sample column. The private residual audit stores half-open intervals and verifies nonoverlap and totals. Its first-party prefix heuristics are secondary leads, not an exhaustive function census.

| Class | Representative evidence | Interpretation and scope limit |
|---|---|---|
| Wrong semantics | EXE `3140` and historical `a8f0` recipe | Wrong field or copy direction; not compiler-only negatives. No module-wide count follows from these examples |
| Wrong instruction graph | FX `6005/af65`, EXE `6a60/6ff0`, TITLE `186e0` | Source branch topology, reload lifetime and full memset changed outputs. Some examples are now masked-equal and must leave codegen queues |
| Compiler/profile representation | Packed FX family, 5,343 bytes per engine | Strong bounded profile evidence, overlapping other FX classes; not 10,686 bytes of forecast strict gain |
| TU/header/owner uncertainty | TITLE primitive/scheduler units; EXE BYTE controls | Real contextual dependencies exist; exact historical context and broad affected scope remain unknown |
| Register allocation/local scheduling | TITLE `a450/18bf0/1c500/95d0/94c0`, paired engine resets, FX stack slots | Observed register/offset symptoms. No universal allocator rule or global flag explanation established |
| Incomplete dependencies/contributions | TITLE 96 unresolved; EXE selected 63-source graph has six; selected frame graph has 113 | Different scoped build graphs, not additive missing-function counts. A missing provider has no honest strict-byte forecast until supplied and naturally linked |
| Relocation-only/layout | 449 rows / 89,920 bytes | The recorded codegen already passes its diagnostic gate. Recover providers, original contributions and natural layout rather than resynthesizing these functions |

Class tags overlap, so their representative spans are **not summed**. The disjoint table above is the accounting total. Constant-lifetime, scratch-rotation, frame-slot and source-form families retain their frozen negatives and explicit reentry conditions. Identical source spellings collapse by full output identity, not similarity score. Unbound historical attempt-count narratives are not comparable experiment counts.

## D. Experimental findings

Phase A independently rechecked selected retained compiler/library/unit objects and native intervals; it did not compile them. The initial compiler matrix remains prior experimental evidence rather than a new replay. The supervisor's ledger reconciliation found 1,031 scoped symbol-map keys with no multi-address conflicts; this checks existing map consistency, not actual linked placement, literal contents or ABI completeness.

### D1. Fresh source-group verification

Prediction was frozen before compilation in `work/process_audit_53/grouped/prediction.json`, SHA-256 `9fb03c1f47972b8b2a5ba341e2bf11f3dd69037de061d0998c5cf8ab8df475ef`. The fixed, header-free canonical controls were `src/shared/motion.c` and `src/shared/frame_state.c`.

Four fresh pinned RTM `/Gy /O2` compilations produced repeatable complete ordered section, symbol, auxiliary and relocation identities. All 32 raw checks across 16 distinct accepted rows / 2,288 target bytes equal the existing accepted source, compiled and target hashes. Recovery, sources, full oracles and manifest-listed tool inputs were hashed before/after; the test did not rely on cached `verify_toolchain` for the final pins.

This establishes that the selected rows do not require separate compilations for their raw comparisons. Generalization needs complete compile-input keys and regression tests. The potential strict-pass count changes from 132 to 30 compiler calls, **77.3% fewer invocations**, with no claimed wall-time, token or strict-byte gain. No production verifier or cross-run acceptance cache was introduced.

Reproduce in the current retained workspace:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' work/process_audit_53/grouped_probe.py --prepare
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' work/process_audit_53/grouped_probe.py
```

The result records each exact CL invocation, source/oracle/tool hash and complete contribution identity. COFF timestamps are excluded from contribution repeatability; whole object-file identity is not claimed.

### D2. Object framing controls

The fixed native FX file-tail source is SHA-256 `d7f2a5f5d940cd5ded4c473b315bd45a4ab4ef3bed9a8efa5083c72d9f4f2042`. Controls are ENG1 `8330..83d8` and ENG3 `d1d7..d27f`, two functions at offsets `0/163`, eleven references, complete 168-byte text and 71-byte literals. The independent optimized control is unchanged `src/title/pilot.c`.

The predeclared arms were `/Od` versus `/Od /Gy` on the FX source and `/O2` versus `/O2 /Gy-` on the pilot, with two repetitions planned per arm. The FX arms both repeated successfully. The ordinary-text arm retained its complete native 168-byte text/71-byte literals and coherent bindings in both engines; the Gy arm changed function packaging while preserving the function bodies, references and literal data. This explains why compatible generation can require a full-unit representation; it does not identify the original full TU or recover native linked placement.

The optimized implicit `/O2` arm repeated with four COMDATs and all eight existing pilot rows raw-exact. The first `/O2 /Gy-` invocation emitted `Command line warning D4002 : ignoring unknown option '/G-'` and retained COMDAT packaging. The disabling-control prediction was falsified; the second repetition was not run. This is a negative option-control result, **not an optimization improvement or a demonstrated new body-generation effect**. No unsupported replacement option or `/Ox` sweep followed.

Seven framing invocations plus four grouped-verification invocations used **eleven of the twelve allowed builds**. There were zero linker invocations and zero source variants. Complete contributions, inventories, references, bodies and data were compared; body-local framing diagnostics never became sliced-function acceptance. The framing prediction SHA-256 is `5a7c6411286020dad7a69cacde85d9a4f7bd92d25968cbb4ca1cc96f9d86753f`. Exact commands, identities and the stopped-arm log are retained in `work/process_audit_53/framing/` and indexed by the compact evidence companion.

Reproduce the same bounded framing test in the retained workspace with the pinned toolchain and fixed source identities:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' work/process_audit_53/framing/run.py --prepare
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' work/process_audit_53/framing/run.py --run
```

The runner's nonzero exit at the unsupported-switch falsifier is the expected recorded negative. Reproduction depends on the preserved private FX source and prediction, whose hashes are recorded; this is not advertised as a clean-checkout production build command. The compact evidence includes all eleven original CL commands independently of these runners.

No service-pack sweep, declaration-padding search, register spelling permutation, new source variant, original execution or fresh natural link was authorized. The two families stop on unsupported options, changed outcomes or repeatability failure. There is no fallback flag grid.

Existing synthetic adversarial tests were also run: all ten full-unit tests and thirteen raw/promotion tests passed. They test full inventories, internal targets, relocations, literal payloads, unsupported storage, input mutation and refusal to slice extra code; no historical compilation is involved. Final hashes of all 764 frozen repository files and all four native oracles were unchanged. This was a selected audit verification, not a fresh rebuild of all 132 strict or 449 masked rows.

## E. Highest-leverage changes

| Rank | Change | Evidence and plausible impact | Cost / risk |
|---|---|---|---|
| 1 | Fresh compilation once per complete immutable source/profile group within a verification run | D1 plus historical repeated-output receipts; current count bound 132→30. Preserves every raw row comparison | Small; low risk with frozen transitive inputs, object inventories and mutation tests. No persistent acceptance cache |
| 2 | Separate full source-unit inventory from per-member recovery status | `similar.check_rows` requires every emitted function to be masked-equal. A TU containing 13 equal peers and a failed member cannot be represented there. Isolating TITLE `4fe0` changed a prior 160-byte contextual output to 176 bytes | Moderate; strong representational need, unknown gain. Preserve all prior proof obligations and exact complete inventory; failed members remain honestly non-strict |
| 3 | Explicit diagnostic build profiles for real packed units and actual module startup/library context | Complete FX units reproduce naturally; O2 implies Gy; EXE WINDOWS/LIBCMT/MFC evidence resolves a false `_main` dependency created by a different diagnostic link recipe | Moderate; no global defaults change. Keep real production hypotheses separate from NOENTRY/forced experiments |
| 4 | TITLE source/data closure in jointly verified coherent units, followed by natural linking | 80,928 masked bytes and 96 real unresolved names provide a concrete target. Old private whole inventory is evidence, not canonical closure | Substantial source work; preserve existing peers. Potential is conditional, not a forecast that these bytes automatically become strict |
| 5 | Correct the boundary/ownership reading tools and pin complete header context | Alignment filtering and universal BSS inference bias investigations; missing SDK/header identities complicate reuse | Small/moderate; derive leads only, preserve recovery authority, avoid mass reclassification |
| 6 | Add a separate natural raw-linked verifier when genuine builds close | Current raw gate cannot represent relocated or packed code; whole-image claims need actual raw sections/addresses and provenance | Moderate/high; keep raw equality intact and forbid forced images, aliases, stubs or placement controls from acceptance |

Measured gains in this audit are uncertainty reduction and repeated-check equivalence. No matching-byte gain was measured. No quantitative forecast of future convergence speed or exact-byte yield is warranted.

## F. Recommended reconstruction strategy

**TITLE:** Prioritize genuine missing code/data contributions, declarations and unit inventory over local source-form searches. Jointly compile complete inferred units, preserve every accepted peer and explicitly distinguish proved versus inferred TU boundaries. Retain negative near-exact functions as source hypotheses where effects are supported; do not require every sibling to match before investigating the unit's ordinary natural link. Audit actual REF/NOICF roots and member order after closure. Keep the 96-name canonical graph distinct from the 362-span private graph.

**ENG1:** Prioritize coherent source recovery and paired-PC structure/control evidence. Map unaligned entries and low-optimization regions explicitly; do not use the alignment-filtered prefix map as a complete census. Recover full plain-text/literal fragments and actual providers. Strict local refinement remains appropriate for a well-supported source/profile discriminator, not as the global queue.

**ENG3:** Use ENG1 as secondary same-PC evidence only where full control/data differences are understood. Keep library code attributed to libraries and incomplete actor views honest. Preserve exact peers and reconstructed near-exact implementations; frozen SIB/frame-slot families need new independent context before reopening. Source/TU closure has more leverage than repeatedly changing equivalent forms.

**HERCULES.EXE:** Maintain its genuine WINDOWS startup/MFC/CRT graph, real SDK declarations and pointer cells. Close the selected six dependencies while recognizing that this is a subset, not full application recovery. Remaining graphics/movie effects must be audited as source semantics and contexts before allocator work. Retain complete masked SDK functions while investigating dependencies and data; no original EXE execution or dispatch.

The strict-first objective remains. The work order should be **evidenced source/profile/context closure → natural contribution/link verification → exact refinement**, with direct strict wins taken when justified. Behavior support, masked codegen, raw function equality, unit layout and whole-image equality stay separate.

## G. Execution plan

1. Implement and adversarially test within-run source-group compilation, retaining all existing raw comparisons, target identities and complete input mutation checks. Run one full strict regression; report compiler calls and wall time independently.
2. Design an explicit mixed-status unit inventory prototype outside production: declare every emitted member and noncode contribution, enforce each existing raw/masked proof, and retain failed members' complete measurements without promotion. Pilot on the existing TITLE primitive context; do not tune its source.
3. Narrow `unit_bounds` and boundary-map claims, adding explicit defining-object storage evidence and reviewed unaligned entry inventories. Preserve heuristic and proved ranges separately.
4. Freeze a current TITLE build inventory with code, data, literal, library and root ownership. Reconcile the canonical 96 missing names once, using actual ABI/address evidence and real providers. Reuse pinned diagnostic objects only within the frozen experiment; fresh acceptance remains fresh.
5. Introduce module/region diagnostic profiles for the demonstrated packed FX contexts and genuine EXE GUI link. Compare full units and data, preserving current defaults for existing accepted source.
6. Once a genuine natural build closes, run two clean builds and a separate raw linked-contribution/section verifier. Report source closure, raw equality, layout and PE metadata separately. Reopen compiler-revision research only with a reproducible multi-control discriminator.

## H. Stop / continue decision

**Credible systemic process improvements were found; the current process is not yet near-optimal for natural byte-identical builds.** The strongest issues are an initial function-only representation being used beyond its scope, mixed-status TU exclusion, alignment/ownership inference bias, distinct graphs being conflated, and repeated same-input compilation. These have concrete evidence; they are not a claim that a global compiler switch will cure the remaining residues.

Stop routine syntax/constant/register grinding for now. Continue with the first two process pilots and TITLE source/data inventory closure. The highest unresolved causal uncertainty is **the real complete source/header/defining-object context of the remaining TITLE families**, followed by region-specific engine build profiles. Exact VC5 patch identification remains open but does not currently have a demonstrated broad discriminator worth a blind sweep.

Canonical game sources, recovery state, acceptance code and native oracles remain unchanged by this exploration. The existing full-reconstruction goal stays paused; this report does not complete it.
