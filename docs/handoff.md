# Current checkpoint, 2026-10-23

Latest follow-up: the user permits documented layout constraints and measured
steering when logic is preserved. The [constrained TITLE pilot](constrained_title_2026_10_23.md)
changes function order only: 25 complete linked functions / 2,112 bytes compare
raw-equal, with 275/362 functions at original RVAs. Three call-only functions
(256 bytes, eight references) have additional independent callee checks. All 43
units preserve their emitted sections and non-FILE symbols. These are diagnostic
results; the authoritative 134 FUNCTION_MATCH / 8,336-byte total is unchanged.

Read [the latest matching progress](title_matching_progress_2026_10_23.md) and
[the preceding audit](matching_state_2026_10_23.md) before the historical handoffs.
Fresh gates preserve **134 exact rows / 8,336 bytes**, with 447 masked rows passing
the supported verifier. The two additions are `title.016e0` and `title.01dd0`;
their mixed units remain intact. Strict verification uses 32 fresh compilations.
The complete TITLE diagnostic links 362 game entries from 43 sources without our
tracing provider. Its DLL is 181,760 bytes versus the original 180,736, but is not
exact: 326 functions differ only in reference operands, 20 have other byte
differences and three have different sizes. Thirteen linked functions compare
raw-equal (only two at original RVAs); these observations grant no further status.
The four existing binding-view discrepancies remain explicit. Next work should
recover real data/code contributions and ownership, with bounded experiments on
the three size mismatches; do not sweep source spelling for relocation-only rows.
`recovery.json` remains the sole matching authority.

# Hercules reconstruction handoff, 2026-10-10 (TITLE.DLL stage-1 convergence)

**Historical checkpoint.** Counts, priorities, compiler hypotheses and owner
rulings below describe this dated investigation. They are not current project
instructions or acceptance state. Start with [project direction](project_direction.md),
[the current contract](../AGENTS.md), [workflow](workflow.md) and `recovery.json`.
Declaration/name steering and automatic push instructions below are not standing
authorization for new work. Retain outcomes while reviewing their hypotheses
against later evidence.

The latest work concentrated on TITLE.DLL. Claude integrated. General-purpose and Haiku 5.5 lanes did
bounded grinding in ignored `candidates/` directories. The 2026-10-09 checkpoint below still describes the
other modules.

## Authoritative checkpoint

- `recovery.json` SHA-256 `8fc21ee815b97f12c3164954138c1ca1767e4ee900f2c666380395573988602e`
  at commit `7fba07e` or later.
- Gates:
  - `python scripts/match.py verify`: 113 exact rows (FUNCTION_MATCH, raw bytes).
  - `python scripts/similar.py verify`: 331 masked rows in 29 sources, 0 failures.
- TITLE.DLL:
  - 6 FUNCTION_MATCH, 333 CODEGEN_SIMILAR (masked, diagnostic only) and 1 RECONSTRUCTED (0x1a2c0, 16 bytes
    too long).
  - Coverage: 70.0% of first-party code bytes. 22 functions are uncovered.
- Other modules:
  - ENG1.DLL: 73 exact.
  - ENG3.DLL: 23 exact and 1 masked.
  - HERCULES.EXE: 11 exact.
  - These are essentially untouched beyond the leaf harvest.

## Where things are recorded

- `evidence/title_stage1.json` holds:
  - `open_functions`: residue per open function;
  - `best_candidates_2026_10_10`: the best scratch draft for each of the 22 open functions;
  - per-lane findings: `lane_*` and `diag_*` keys;
  - the compiler models: `register_model_2026_10_10`, `rotation_study` (including `RR_model_2026_10_10`,
    `R4_resolved_order`, `R8_two_increments`) and `scheduler_study`;
  - `tu_count_phase_2026_10_10`;
  - the volatile and header rulings.
- `evidence/title_closure.json` holds the unit extents and data units, `full_link_2026_10_10` (the full
  natural-link assembly) and `unit_title_objcfg`.
- `docs/title_link_goal.md` has the narrative, including the sections "Levers and rulings from the tail work"
  and "Register model and TU-count phase".

## Owner rulings in force

- **Layout names:** variable names may be chosen so that VC5's identifier hash reproduces the .bss order.
  Document each such choice.
- **Real system headers as TU context:** allowed, documented and decided per unit. Measured: headers act only
  as phases of a declaration-count tie-break (none = full + 2, lean = full + 13, mod 32). A header that makes a
  unit match records context, not the original header's identity. Re-decide once each unit is complete.
  Current choices:
  - u01de0.c and u02b70.c: full `<windows.h>`;
  - u06350.c: `WIN32_LEAN_AND_MEAN`.
- **volatile:** allowed per variable with codegen evidence and owner approval. Approved: the context fields
  +0x5c/+0x5e, `g_25a28` (0xd720) and `g_2a244` (0x5fb0). Not approved: `TitleObject.unknown_022` (0x54f0
  stays a documented hypothesis; the shared qualification breaks 3 rows).
- **Language:** new units are C; use C++ only with evidence. Push after every verified commit.

## Measured compiler models (use these before guessing)

- **Callee-saved priority:**
  - W(v) = sum over blocks of refs x freq x N(B), where N(B) is the number of distinct candidates in the block.
  - Structured loops weigh 2^(depth+1).
  - Assignment is greedy: ranges with calls get esi, edi, ebx, ebp; call-free ranges try eax, ecx, edx first.
  - A byte candidate takes ebx.
  - Known gap: statement order inside a block can decide a contested register.
- **Scratch registers:** round-robin eax -> ecx -> edx. Call arguments are allocated right to left.
- **Loop lever:** a 999-marker loop written as `while (T[++g] == 999) { ...; g++; }` (two separate +1 steps)
  gives later loops the original register pair. The first loop in layout never gets it (open).
- **Other natural levers that worked:**
  - region-local cursors for heavy loop pointers (0x3da0);
  - writing results straight into arrays instead of locals (0x193e0);
  - passing a value through an existing pointer variable (0x2b70);
  - a duplicated tail written once (0x10fe0);
  - a goto-built loop to avoid loop rotation (0x3da0);
  - one loop writing two arrays (0x1b150);
  - a byte loop instead of memset (0x66f0).

## Full natural link (diagnostic, candidates/wlink)

- With all 362 functions present, 57 land at their original RVA. The only shifts come from four draft size
  errors: 0x54f0 (-32), 0x9c90 (-16), 0x186e0 (-16) and 0x1a2c0 (+16).
- **Link-flag evidence:** `/OPT:REF /OPT:NOICF /EXPORT:PC_DLLEngineMain` plus OLDNAMES reproduce .rdata,
  .idata and the library block exactly.
- **Canonical steps already taken:**
  - u02b70.c is in address order;
  - the data unit `calibration/title/title_objcfg.c` (0x26770..0x269c8, 600/600 bytes) is added.
- **Blockers:**
  - unowned initialized data, some of it unreferenced, which must not be defined as placeholders;
  - communal owners and order;
  - TU-count phases that shift when units are completed;
  - the six exact rows that still live outside their units;
  - type conflicts between drafts.

## Suggested next steps

1. Fix the four wrong-size drafts and re-run the full assembly. Many masked rows may then become raw-exact in
   one natural link.
2. Reconcile and complete units, then re-decide each unit's header/phase context on the complete unit.
3. Resolve data ownership as functions are recovered. Use `scripts/layout_names.py` for the .bss and communal
   order.
4. Start ENG1/ENG3/EXE with the same toolchain, models and workflow.

---

# Hercules reconstruction handoff, 2026-10-09 (updated after the leaf harvest)

The project moved to a new host (user `jiriv`). Originals, pinned VC5 RTM/SP2 trees
(`setup_toolchain.ps1 -VerifyOnly`) and analysis packages verified unchanged; VC5
runs there without sandbox escalation. Python is the WindowsApps CPython 3.11.9
(see README). No Codex workers were used for this chunk; Claude integrated, with
Haiku 5.5 lanes for bounded candidate grinding.

## Authoritative checkpoint

- `recovery.json`: 113 accepted instances. Its SHA-256 is
  `4bb3ea675b42893bfb0302bf883de16548d021a3c3d19a0c9c162fcc74ce4ce5`.
- All 113 passed fresh historical compilation and the unchanged complete raw-byte
  gate. The leaf harvest added 69 ([docs/leaf_harvest.md](leaf_harvest.md),
  `evidence/leaf_harvest.json`). The frame-family state below is unchanged: its
  link diagnostic still reports 8 objects and 114 unresolved names.
- The primary EXE oracle remains `work/discs/pc_install/HERCULES.EXE`, SHA-256
  `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`.
  The installed asset EXE has the user-confirmed one-byte no-CD modification.
- Original files remain immutable; the last inventory check verified all 12.
  No original executable or DLL was loaded or run.
- `scripts/dependency_link.py` now resolves `--output` before the linker runs in
  its lane, so the relative command below works as documented.

## Leaf pool status

The relocation-free leaf pool is essentially exhausted. Nine genuine leaves stay open
with recorded families and re-entry conditions in `evidence/leaf_harvest.json`.
Do not rerun their falsified families without a new discriminator. The measured
TU-context effect at ENG1 `0x2a520` is the most useful new input for TU recovery.
New acceptance otherwise depends on the natural-link dependency work below.

## Earlier finished work (frame dependency binding)

`calibration/frame_bootstrap_family.c` now calls recovered configuration/defaults
and allocator/cancellation providers directly. Three count references share the
configuration writer's observed cell names. The seven identity mappings were
checked against the same ENG1 image hash and RVA; return declarations agree with
the recovered providers. Historical names and formal prototypes remain unproved.

Across eight freshly compiled source units and 46 function contributions, 44 full
output identities stayed unchanged. The two affected functions retained identical
code bytes and relocation offsets/types; only reviewed reference names changed.
Bootstrap is still relocation-only equal. Cleanup/setup retain their previous
non-reference differences and must not be promoted from similarity.

The synthetic bootstrap fixture now links the real configuration and engine-file
bridge units. It passed 193 checks, including selected configuration reaching
allocation sizes, callback-visible count changes, live-context defaults,
cancellation, cleanup and setup. Other dependencies/storage are observing fixtures.
Python verification passed 32 tests. The diagnostic's four negative experiments
rejected stale source, header, specification and oracle inputs before compilation.

The natural link went from 121 to 114 unique unresolved names. Four callable names
resolved to real definitions; three count names were coalesced but still lack
production storage. No duplicate definitions were found. The remaining selected
closure has 13 local calls and 101 data names without providers; these are name
counts, not distinct original storage objects. The natural link uses no fixture
stubs, aliases or placement controls and remains incomplete.

Durable evidence: `evidence/frame_dependency_binding.json`,
`evidence/regions/frame_bootstrap_bound.json`, `evidence/frame_link_plan.json`.
Reading context: `docs/frame_dependency_binding.md`, `docs/blood2_convergence_review.md`
and `docs/workflow.md`. Generated reports and worker handoff paths are recorded
with hashes in the evidence, but stay ignored under `work/` or `build/`.

## Resume with dependency recovery

Read `AGENTS.md`, then run from the repository root using the configured Python:

```
python scripts/region.py packet frame_bootstrap_family --more
python scripts/dependency_link.py --plan evidence/frame_link_plan.json --output work/frame_dependency_links
```

The second command freshly checks pinned tools, source/header/specification
receipts, oracles, frozen contributions and the accepted writer, then produces a
new ignored report. An unresolved link is an expected diagnostic outcome. Neither
command grants acceptance. Historical VC5 execution on this host requires the
existing process-permission escalation because the sandbox rejects its old DLL
layout; do not modify the toolchain.

First review the two remaining same-address data naming pairs:
`g_active_row_70d01` / `g_frame_byte_70d01`, and
`g_selected_width_70d12` / `g_frame_word_70d12`. Check target identity, access width,
caller behavior and full contributions before changing either name. Coalescing
names does not supply storage or prove the original aggregate.

Next choose a coherent missing dependency, preferably the `0x5710` initializer
and its token-protocol callees, using PC evidence to determine bodies and genuine
data ownership. The other unresolved local calls are `0x2f30`, `0x3270`, `0x50d0`,
`0x58b0`, `0x5dc0`, `0x15090`, `0x1f870`, `0x25620`, `0x262a0`, `0x2da90`,
`0x2f840` and `0x2fb70`. Distinguish source omitted from this bounded link plan
from source genuinely absent from the repository.

Record a prediction and falsifier before the next family. Keep private experiments
in ignored scratch, preserve exact peers and do not repeat exhausted source-form
work without a new discriminator. Recover actual data/library contributions;
do not invent empty production providers, pad declarations, steer placement, or
rewrite relocation-only equals to solve layout. Keep the existing Luna xhigh /
Sol high routing contract and raw acceptance gate.

For a changed source family, rerun its affected semantic pipeline and all accepted
matches. Broaden verification only when shared inputs or failures justify it:

```
python scripts/test_pilot.py --suite frame_bootstrap_family
python scripts/match.py verify --report work/resume/raw_regression.json
python scripts/inventory.py --verify
```

Original game files and pinned tools are local prerequisites and are intentionally
absent from Git. A fresh checkout can regenerate diagnostic reports once those
prerequisites are present; ignored scratch reports are not portable proof caches.
