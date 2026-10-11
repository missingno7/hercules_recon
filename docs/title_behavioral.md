# Behavioral TITLE branch

The requested experiment lives on `codex/title-behavioral-reconstruction`, based
on matching commit `9306aa0288d6b064f8a555ca8acf7b28e401c6f1`. Matching source,
`recovery.json`, and existing proofs are retained. There were already uncommitted
matching changes in `title_proc.cpp` and `recovery.json`; their initial patch and
status were saved in ignored `work/title_behavioral/matching_start*` files.

This branch uses reconstructed behavior and ABI compatibility as its development
criteria. It does not change the requirements for `FUNCTION_MATCH`. The build
configuration is an explicit source graph and a collection of semantic contracts,
not a replacement matching ledger. Function coverage is derived from the current
inventory and `recovery.json` each time. Inventory spans that need boundary review
remain marked as leads.

## Build and entry validation

```powershell
python scripts/title_behavioral.py
python scripts/test_title_behavioral.py
```

The builder uses the already pinned 32-bit VC5 RTM compiler, linker and static C
runtime under `C:\tools\hercules`. Existing providers and flags come from
`recovery.json` and the data-unit list. Additional providers, ordinary ownership
corrections, and same-unit fragments are explicit in
`evidence/title_behavioral_build.json`. Only affected translation units are
materialized under ignored build output; the matching originals remain unchanged.
The report records original/header/compiler-input/object hashes and commands.
No forced unresolved link, fixed addresses, or original executable code is used.

The successful artifact is `work/title_behavioral/build/TITLE.DLL`. A failed link
removes an older DLL at that output path and produces `link.log` and `report.json`.
The builder checks x86 PE32 and the sole undecorated export
`PC_DLLEngineMain`, ordinal 1. Every function remains mapped to its original RVA;
new support routines such as tracing explicitly have no original RVA.

The export is cdecl with two arguments: a pointer to the 250-DWORD host interface
and unsigned flags. Native code checks the first-use cell at RVA `0x257e8`, copies
1,000 bytes before signature rejection, checks `0x5ac00cac`, calls the copied arena
service, and clears the first-use cell. Active calls invoke the host update
callback, reload the copied context, set context DWORD `+0x40` to `flags & 1`, then
execute the TITLE frame routine. The observed EXE caller discards EAX, so no useful
return value is promised. The first-use cell shares the actual movement-table
provider with the password sprite handler.

The source fixture uses test-only callback spies to check this protocol. It does
not prove that the complete startup, resources, rendering, or menus work. Optional
diagnostic tracing is enabled by `HERCULES_TITLE_TRACE` containing a writable file
path. It reports entry, interface copy, arena setup, host update and frame stages;
an unset variable produces no trace file.

## Evidence and incomplete behavior

Existing `FUNCTION_MATCH` and `CODEGEN_SIMILAR` records retain their original
meaning. Behavioral additions state contracts, dependencies, provenance and
uncertainty separately. Plausible source and codegen similarity are supporting
evidence, not comprehensive behavioral validation. Fresh regression passed all
132 strict instances and 449 codegen rows before new providers were integrated.

`evidence/title_behavioral_incomplete.json` records transitional placeholders and
known incomplete implementations. Any future placeholder must identify its RVA,
report execution and fail visibly rather than fabricate success. Missing bodies
and unresolved symbols are derived in the build report even when no placeholder
has been introduced.

The behavioral data provider supplies observed typed loader-zero cells and array
prefixes. Its evidence explains interior aliases and the shared three-int screen
center. Pair-list, pointer-cache and replay-list capacities are provisional;
neighbor boundaries do not prove the original complete object extents. Native
placement, aggregate ownership and universal input safety are not established.

The current build supplies all 362 entries in the first-party inventory
from 44 source units, with no unresolved symbols, duplicate providers, or
placeholders. Native references and source definitions support all 39 flagged
entry identities; their exact byte extents remain provisional. Verify that compact
evidence with `python scripts/title_inventory_review.py`. Six
previously accepted functions remain raw-byte exact in the linked DLL. Thirteen
functions have limited native behavioral fixtures; the rest retain their
existing matching/codegen evidence and unverified behavioral status.

Actual isolated host runs reach the clear title menu and password screen. Real
keyboard input navigates to Passwords and returns to the menu; F2 opens the
original host Options dialog and Cancel closes it. Animations run, but matching
wall-clock screenshots to identical animation frames is not yet deterministic.
Start Game, followed by a native Escape movie skip, unloads TITLE and loads the
original ENG1.DLL; both original and replacement runs reach its forest gameplay
scene. Save/load, changed settings, reinitialization and ENG3 transitions need
broader validation. Compact measurements are in `evidence/title_behavioral_runtime.json`
and `evidence/title_behavioral_ui.json`;
fresh detailed logs and screenshots remain ignored under `work/`.

The user subsequently reported that manual testing appeared to work well, while
explicitly leaving coverage non-exhaustive, and requested no further manual game
runs. That observation is recorded separately from measured fixtures. The tested
DLL is retained unchanged. An attempted ENG3 baseline input experiment was
inconclusive because its private controller could release a key before the
expected logical edge; that controller is not adopted as a canonical test.

`python scripts/title_menu_differential.py` compares the linked menu callback
against native RVA `0x7510` in a separate fixture process. Twenty cases cover
navigation, skipped selection 2, both confirmation bits, held versus rising-edge
input, repeat suppression, and selection 4 enabled by the high context flag.
Each compares 21,752 bytes of affected state and normalized ordered callbacks;
an explicit assertion checks Exit slot `0x238` invocation. Busy-state animation,
sound playback, and real host teardown are outside this fixture's scope.
The earlier apparent source Exit difference coincided with the normal 900-tick
idle demo; it does not establish an Exit dispatch mismatch.

## Original host provenance and integration

The unmodified installer host is `work/discs/pc_install/HERCULES.EXE`, SHA-256
`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`.
The supplied installed asset EXE has a pre-existing one-byte no-CD modification.
It must not be described as the unmodified installer host. ENG1.DLL, ENG3.DLL and
game data match the installer copies. Scratch runtime copies exclude the original
TITLE.DLL and verify the preserved EXE/other DLL hashes.

DLL loading, export resolution and rejected-entry smoke tests precede an actual
game launch. Neither a successful link nor a load fixture proves that the EXE
reaches the title screen. Original-game results must record host/DLL hashes,
execution evidence, earliest failure, and reproduction commands. CD checks and
display limitations remain host-environment questions until execution provides
evidence. The separately authorized EXE staging patch below is recorded
independently of reconstruction and matching evidence.

Reproduce the native surface and linked-DLL comparisons with:

```powershell
python scripts/title_differential.py
python scripts/title_linked_differential.py
python scripts/title_password_differential.py
python scripts/title_render_differential.py
python scripts/title_menu_differential.py
python scripts/title_inventory_review.py
python scripts/title_integration.py prepare
python scripts/title_integration.py attach
python scripts/title_integration.py build-fixture
python scripts/title_integration.py fixture
python scripts/title_integration.py build-debugger
python scripts/title_integration.py debug-launch --title-trace --timeout-ms 45000
python scripts/title_integration.py prepare-baseline
python scripts/title_integration.py debug-baseline --timeout-ms 45000
```
The debugger handles the initial OS loader breakpoint and passes later exceptions
to application handlers; it does not modify code or process memory.

The user authorized a final no-CD staging step after compilation and verification.
`python scripts/title_integration.py prepare --no-cd --runtime work/title_behavioral/integration/runtime_nocd`
copies the verified installer EXE, changes only file offset `0x94b0` from `53`
to `c3`, and requires the result to equal the supplied no-CD EXE hash. It never
changes assets or compilation/oracle outputs. The runtime manifest records the
host identity and exact patch. The menu label `320x240 Window` is misleading:
command `32801` only resizes the window. Startup requests fullscreen regardless
of an earlier resize request. Native Alt+Enter switches the mode; verify both
requested and current fullscreen bytes are zero and the caption/menu restored,
then use `32801` to choose 320x240. A 330x293 popup with no menu is still in the
fullscreen rendering path. Modern-PC integration must verify mode, not just the
window size. `prepare-baseline` creates a separate guarded original-TITLE copy
for comparison; the replacement has no runtime dependency on that copy.
