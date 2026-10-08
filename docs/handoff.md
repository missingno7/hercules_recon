# Hercules reconstruction handoff, 2026-10-09 (updated after the leaf harvest)

The project moved to a new host (user `jiriv`). Originals, pinned VC5 RTM/SP2 trees
(`setup_toolchain.ps1 -VerifyOnly`) and analysis packages verified unchanged; VC5
runs there without sandbox escalation. Python is the WindowsApps CPython 3.11.9
(see README). No Codex workers were used for this chunk; Claude integrated, with
Haiku 5.5 lanes for bounded candidate grinding.

## Authoritative checkpoint

- `recovery.json`: 113 accepted instances. Its SHA-256 is
  `e9c169bedbeb01536019d4cd223cd1a0505669b8f75d56f96eadef987069d639`.
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
