# Reconstruction work and compact context

`recovery.json` remains the only recovery authority. The sequence is evidence,
private scratch, fresh compiler/oracle measurement, and the existing journaled
promotion. No model opinion or context receipt grants `FUNCTION_MATCH`.

Use **Luna xhigh** for normal reconstruction in a reasonably stable frame. Use
**Sol high** when the explanation itself is uncertain or resolving one shared
question can unlock several functions. Hand its falsifiable hypotheses back to
Luna. A failed candidate, function size or ugly diff is not an escalation rule.
There is no scheduler, difficulty score, retry ladder or routine Sol review.

Choose the largest *interpretable* common context. Label ownership as proven TU,
inferred region, or function task; convenience does not prove historical ownership.
Compile broadly once, freeze exact contributions, investigate shared causes once,
and shrink to local families when common representation is stable. Relocation-only
equals belong to the dependency/linkage problem, not an algebraic source search.

For a recovered dependency family, attempt a natural historical link before
reopening relocation-only equals. Use an explicit list of actual source providers,
identified library inputs and frozen contribution receipts. For the frame family:

```
python scripts/dependency_link.py --plan evidence/frame_link_plan.json --output work/frame_dependency_links
```

This freshly compiles the selected units, rejects stale source/header/specification
receipts and changed oracles, checks complete recorded contributions, then links
without production stubs, symbol aliases or placement controls. Unresolved symbols
are an expected diagnostic outcome. The report is disposable; it grants no recovery
state. Review whether each error denotes a missing body, inconsistent name or
prototype, missing data contribution, library dependency, or incorrect unit owner.
Correct identity only with address and ABI evidence, then independently compile
the canonical result and verify affected semantic pipelines and accepted matches.
Coalescing two names for one cell does not provide its storage. Successful linkage
does not prove original module layout. See `docs/frame_dependency_binding.md` for
the first measured use and remaining blockers.

## Start and resume

A reconstruction worktree can use the primary checkout's read-only originals
and derived archaeology without copying them or changing the primary recovery
state. For the ENG1 source graph, run:

```
python scripts/eng1_behavioral.py --original-root D:/Prog/hercules_recon --output work/eng1_source_graph
```

Source, configuration and `recovery.json` come from the calling worktree; the
original image, archaeology database and library map come from the named root.
The database is opened read-only. A failed natural link remains a dependency
diagnostic, and fail-on-entry placeholders do not make a playable engine.

The data-provider audit accepts the same read-only original root:

```
python scripts/eng1_data_evidence.py --original-root D:/Prog/hercules_recon --output work/eng1_data_verification/report.json
```

Keep the manifest's relative target identity unchanged. The override changes
only where the pinned oracle is read; function references still bind against
the local recovery rows and the original hash. Substituting an absolute path
inside the manifest would change the identity used for those bindings.

Fresh strict verification can read the same pinned originals directly:

```
python scripts/match.py verify --original-root D:/Prog/hercules_recon --report work/strict_verification.json
```

This changes only target lookup. Canonical source proof checks, fresh compilation,
complete contribution comparison and the rejection of object/native relocations
remain the existing strict gate. It writes no recovery state.

The current original-host frame recovery supplies the ordinary static MFC map:
30 entries, its zero sentinel and 32 pointer relocations over 752 bytes. Scalar
fields, order and source-symbol/native-target correspondence are measured;
relocated data is not raw object equality or a module-layout match. All 63 virtual
slots, the 24-byte runtime descriptor, eight complete class contributions and
both SYSKEY contributions survive the nonvirtual declaration extension.

The WM_ERASEBKGND callback at EXE `0x9900` now passes journaled `FUNCTION_MATCH`
for all 16 bytes, including alignment. All 133 accepted functions pass fresh raw
verification. Three command-UI callbacks at `0x9a90`, `0x9bf0` and `0x9c10` use
existing state owners and remain `RECONSTRUCTED`: 17 directed cases / 19 virtual
Enable events agree, but each complete contribution retains a relocation.
The signed byte, full DWORD and zero-test distinctions are preserved. The
offline fixture models the Enable return in a code hook without writing endpoint
opcodes; it checks all 188 receiver bytes and rejects other execution or writes
outside the bounded stack. It does not execute MFC or a Windows message loop.

```
python scripts/host_frame_message_map_data.py --output work/frame_map_replay
python scripts/host_frame_command_ui_differential.py --original-root D:/Prog/hercules_recon --output work/frame_ui_replay
python scripts/host_system_keys_differential.py --output work/frame_keys_replay
python scripts/host_frame_precreate_differential.py --original-root D:/Prog/hercules_recon --output work/frame_precreate_replay
```

The current natural selected host graph compiles 124 actual source units with
166 recorded EXE functions and preserves 26 accepted EXE functions. It has no
duplicate providers and 17 unresolved dependencies: five frame
callbacks and 12 data providers. Including the real map replaces one missing
map symbol with its actual missing callback bodies. These counts are diagnostics,
not whole-game coverage or a runnable build. The known natural MFC/CRT allocator
selection discrepancy still needs genuine dependency evidence. The isolated
PreCreateWindow fixture deliberately omits the production map unit; its sole
missing map dependency does not describe the complete production source graph.
See `evidence/host_frame_message_map_recovery.json`,
`evidence/host_frame_class_recovery.json` and
`evidence/host_current_source_graph.json`. Earlier receipts retain their original
header and graph contexts. Reconstruction remains the current phase; renderer
implementation is deferred.

Six further frame callbacks now have ordinary historical C++ definitions: activation,
client sizing, system commands, nonclient mouse/menu entry and help. The existing
receiver declarations, class layout, virtual table and message map are unchanged.
In 67 guarded native/source cases, these handlers execute the recovered platform
and audio descendants. Four identified MFC library methods are linked unchanged
and modeled at entry; this does not execute a Windows message loop. An incorrect
system-command mask is rejected by a directed negative control. Five complete
contributions are relocation-normalized equals; activation retains seven other
byte differences. All six fail raw equality, so none is a new FUNCTION_MATCH.
The one-byte maximized flag has measured loader-zero backing; larger storage and
original ownership remain unproved. See `evidence/host_frame_messages_recovery.json`.

```
python scripts/host_frame_messages_differential.py --output work/frame_messages_replay
```

The palette-query, palette-change and cursor callbacks now also have ordinary
historical C++ definitions over the unchanged receiver. In 46 guarded cases they
execute the actual existing palette and presentation routines. Null/child window
handling, current MSG.message, live handle reloads, full return values and the
fullscreen BYTE zero test are preserved. Identified static MFC methods and OS/COM
effects are modeled boundaries. All three full contributions are diagnostic equals
with raw relocation differences; they remain CODEGEN_SIMILAR. See
`evidence/host_frame_palette_cursor_recovery.json`.

```
python scripts/host_frame_palette_cursor_differential.py --output work/frame_palette_cursor_replay
```

The window-resize helper and its three resolution callers now preserve the
original edge metrics, signed work-area minima and live HWND/control-site reads.
All four complete contributions are diagnostic equals. In 30 guarded cases the
actual unmodified MFC ShowWindow and SetWindowPos bodies execute; only Windows
and control-site endpoint effects are modeled. A negative case distinguishes
edge metrics 45/46 from size-frame metrics 32/33. The shared header adds one
nonvirtual helper declaration. All 69 sections in its seven prior consumers retain
their bytes and resolved relocation targets; four generated local exception-label
names change without moving their targets. All 133 strict matches and the complete
message map survive. See `evidence/host_frame_resize_recovery.json`.

```
python scripts/host_frame_resize_differential.py --output work/frame_resize_replay
```

The paint callback now has an ordinary automatic CPaintDC reconstruction. In
27 guarded normal-path cases it executes eight unmodified MFC bodies, the actual
CRT EH prolog and existing presentation code. Captured paint HWND, complete paint
object/PAINTSTRUCT, handle-map cleanup and parent exception-chain restoration
agree. Its 48-byte source does **not** reproduce the original 96-byte callback's
inactive outer exception registration; every in-call trace retains that measured
difference. A C++-linkage diagnostic activates cleanup and is not adopted merely
because its size is 96 bytes. The function remains RECONSTRUCTED; exception
dispatch and unwinding are unverified. All 133 exact peers and 78 existing frame
consumer sections survive. See `evidence/host_frame_paint_recovery.json`.

```
python scripts/host_frame_paint_differential.py --output work/frame_paint_replay
```

The loader now has an independent source provider for four observed DWORD cells
and the pinned SDK's 64-byte `jmp_buf` representation. All 80 source bytes have
the original virtual loader-zero backing and no code or relocations. This does
not prove their original owner, adjacency, placement or complete capacity.
The dispatcher's actual `__setjmp3(env, 0)` path saves a 40-byte prefix; the
remaining 24 SDK unwind-data bytes stay untouched in all nine directed cases.
The original CRT also contains a bounded optional six-DWORD copy fitting the
64-byte SDK layout; the dispatcher does not exercise those optional arguments.
The source differential uses the real canonical storage object and a separate
native-address fixture, while retaining synthetic path/error buffers, exports,
API effects and the existing exception-head precondition. Path/error capacity,
actual initialized host-table ownership and real SEH behavior remain unresolved.
Function sources and all recovery rows remain unchanged. See
`evidence/host_engine_storage_recovery.json` and run:

```
python scripts/host_engine_storage_data.py --output work/host_loader_storage_replay
python scripts/host_dispatch_differential.py --output work/host_dispatch_differential/loader_storage_replay.json
```

The original bootstrap at `0x1ff0` now has ordinary source for its complete
208-byte contribution. It clears the 6,964-byte context prefix and the current
configured arena, calls the real `save_arena` source, retains post-callee context
reloads, writes the observed startup fields, seeds the identified CRT `srand`,
and passes the current buffer/name to the still unresolved engine-switch routine.
The existing 32-byte pool-save contribution is now recorded alongside it.
Both remain `CODEGEN_SIMILAR`: full relocation diagnostics agree, raw bytes fail.
The shared headers and existing pool/diagnostic function sources are unchanged.

```
python scripts/host_context_bootstrap_differential.py --output work/context_bootstrap_replay
```

Twelve directed cases compare all three context buffers, complete pool/table
fixture bytes, the full original 3 MiB arena, zero and non-DWORD byte counts,
guards, caller stack/nonvolatile registers and bounded execution/writes. Only
the bootstrap and actual pool-save code execute. Setup, seed and switch effects
are modeled at their call boundaries; mutation cases distinguish live reloads
from cached views without proving those callee implementations. The fixture
uses the actual pool metadata, four native format literals and already exact
diagnostic provider; inert main and other linked code fail on entry. Its zero
table, filename capacity and alternate context preconditions remain test inputs.
The pool base is the independently proved `0xba29c0`, not the unrelated data at
`0x43a630`; the write whitelist caught the initial private fixture mistake.
See `evidence/host_context_bootstrap_recovery.json` for the source, CRT identity,
127 literal bytes, failures and receipts. All 133 accepted functions remain
exact. Supplying the real initializer/pool unit exposes its actual missing
setup/switch/name/buffer dependencies; a larger missing-symbol count is not a
reason to restore an opaque initializer or substitute production stubs.

The original engine-switch caller at `0x4040` now has source for all 144 bytes,
including its final alignment byte. It preserves the ignored zero-leaf argument,
conditional context allocation free, pool-zero bulk cleanup, resource callback
clears, empty callback, callback-one reset, DWORD writes at context+`0x108` and
`0x10c`, selector arguments and EAX0 return. The accepted zero/empty/diagnostic
leaves and existing pool/callback providers remain unchanged. Seven previously
unrecorded caller/provider functions are now measured recovery rows; the count
helper already had a row and is reused. Three rows have full relocation
diagnostic equality and four remain reconstructed with code differences. None
gets exact acceptance from finite semantic agreement.

```
python scripts/host_engine_switch_differential.py --output work/engine_switch_replay
```

Eleven guarded comparisons execute the actual free, bulk, coalescing,
compaction, count, reverse-copy and callback code. Whole heap/owner/pool/callback
and three context buffers agree for owned, pinned and zero-owner-content blocks,
including a real compaction move. Nullable selector arguments and synthetic
context interventions check caller reload/forwarding behavior. Only the selector
return is modeled; deliberately nonzero EAX at that boundary still yields caller
EAX0. This does not prove DLL loading, nonlocal transfer or a complete switch.
The modeled selector initially folded with the identical real RET callback;
normal fixture-only `/OPT:NOICF` separates those boundaries. It establishes no
original linker flag or module layout. The actual 37-byte compaction format is
an independent literal-prefix provider. See
`evidence/host_engine_switch_recovery.json` for source/oracle/tool hashes and
scope. The one-DWORD zero-leaf callsite view does not settle its original common
source prototype or whole-program type agreement; its accepted no-argument source
was preserved. All 637 previous rows and all 133 exact functions remain intact.

The actual setup at `0x10690`, resource reset at `0x10760`, input registration
caller at `0x4180`, notification at `0xac00` and slot reset at `0xa1a0` now have
source in one coherent working family. The ordinary `unsigned char local[32]
= {0}` hypothesis reproduces the observed 32-byte reservation and 31-byte clear
without padding, volatile or assembly steering. Full 208/96/32/16-byte relocation
diagnostics agree; the ordinary slot loop remains 96 versus 112 bytes. The existing
pool initializer is also recorded with its remaining code differences.

```
python scripts/host_setup_differential.py --output work/setup_replay
```

Eleven original/source cases execute real pool initialization, exact leaves,
callback reset/invocation, packet registration and the existing virtual/archive
file-close chain. They compare unsigned BYTE9, two complete context buffers,
the pool/arena, default copies, packet pointer, all 16 public handles and 1024
logical positions, call order, ABI/guards and the unused stack clear. A synthetic
post-leaf context/default intervention checks caller reloads without claiming
that the actual leaf mutates them. No close return or effect is modeled.

The original A350 is the already recovered `virtual_file_close`, calling the
existing `archive_close` at 51F0. Setup now uses their actual signed public count
and handle owners. The shrinking-count guard can leave a later occupied handle
untouched during ascending reset; dense and sparse cases reproduce that original
behavior. Negative and zero counts reject every close. The final reset count is
zero even when entries survive. This behavior is preserved for later matching.

The original `4180` registers pointers `ba2a40` and `ba2a00` with two `0x22`
arguments; it is not the unrelated file-prepare draft. The execution whitelist
caught that initial private binding error. Production buffer capacities remain
unknown, so both symbols stay external. The existing packet owner consumes only
the first pointer; the common original prototype is not settled by its one-arg
source and this four-arg caller view. The notification's duplicate old draft
body was removed; the remaining close-state complete code and relocations are
preserved. All old accepted function sources and shared context headers remain
unchanged. Setup owns a measured 32-byte initialized working prefix and DWORD
first flag. The former separate count/slot definitions duplicated 68 bytes of
existing public-file storage and have been removed. Fresh COFF/native data checks
reuse its unique 4/64-byte owners and the 4096-byte logical-position prefix.
Original aggregate, translation-unit ownership, complete extent and placement
remain unproved. All five setup code contributions are unchanged; only the three
proven relocation identities were rebound.
See `evidence/host_file_close_integration.json` for current evidence and
`evidence/host_setup_recovery.json` for the first recovery context. All 650 rows
retain their statuses and proofs; six evidence notes now point to the integration.
All 133 accepted functions remain exact. The real close path resolves the former
opaque A350 dependency; the natural build still produces no game executable.

The CD interval service at `0x9cd0` now supplies the former opaque timing
dependency. Its first ordinary C hypothesis reproduces the full 144-byte
contribution after relocation normalization; 23 raw bytes differ and eight
relocations remain. It is `CODEGEN_SIMILAR`, with no exact promotion. All 650
previous rows are identical, and all 133 exact functions pass fresh verification.

```
python scripts/host_cd_timer_differential.py --output work/cd_timer_replay
```

Twenty-seven bounded cases reuse the six existing MCI DWORD owners and check
zero guards, packed byte arithmetic, unsigned threshold equality/reversed ranges,
clock wrap, ignored upper bits and API-boundary mutations. Duration uses snapshots
before the tick call; previous tick and restart arguments reload after it. The
playing flag is cleared before restart, whose distinct modeled effects survive
return. Time and play `0x9e60` remain test boundaries. No real MCI operation runs.
See `evidence/host_cd_timer_recovery.json`. The natural graph now contains 96
units and 130 recorded EXE functions; the new body exposes its actual missing
playback dependency, leaving 39 missing providers and no executable.

The playback service at `0x9e60` now replaces that missing dependency. Its
ordinary C source is 240 versus the original 256 bytes and remains
`RECONSTRUCTED`; no exact or normalized full-code match is claimed. It reuses
the existing registry, MCI and timing-reset owners and adds no data storage.

```
python scripts/host_cd_play_differential.py --output work/cd_play_replay
```

Thirty-five guarded comparisons execute the actual playback, cleanup and
readiness providers, including the timer-to-play path. They cover exact BYTE1
enable gating, device-query failures, the two attempts and both failure sleeps,
shared start-track packing, API mutations and final tick/reset writes. MCI,
clock, Sleep and identified CRT formatting/ASCII comparison remain modeled
boundaries; no device or complete CRT behavior is proved. The unmodified RTM
`sprintf` and `strcmpi`/`stricmp` library members have full normalized code
identity at their original PC extents. No library source was rewritten.
See `evidence/host_cd_play_recovery.json`. All 651 prior statuses/proofs remain
unchanged; one descriptive preceding-RET typo in the timer row is corrected to
`9CCE`. All 133 accepted functions still match. The natural graph now has 97
units, 131 recorded EXE functions and 38 missing providers, with no executable.

Startup CD check `0xa0b0` and validation `0x10520` now have source. Their
first ordinary C hypotheses reproduce the full 112/368-byte contributions
after relocation normalization. Raw bytes differ, so both are `CODEGEN_SIMILAR`.

```
python scripts/host_cd_startup_differential.py --output work/cd_startup_replay
```

Forty-nine guarded cases execute the real caller, validator and readiness code.
Validation clears the existing gate and checks exactly two tracks: the first
must be non-audio, the second audio with a parsed minute field of 34. Seconds
and frames are parsed but unchecked. Command/response buffer aliasing and one
reused escaped local are preserved. The caller retains readiness from before
validation, retries every dialog result except cancel2, and does not clear the
gate on cancel. MCI/dialog and identified CRT boundaries remain modeled; no
actual CD/device or whole startup proof follows.
See `evidence/host_cd_startup_recovery.json`. All 652 prior rows are identical,
and all 133 exact functions still pass. Existing29-byte state owners are reused
without new storage. The matching oracle remains the original baseline, not
the installed no-CD variant. The natural graph now has 98 units, 133 recorded
EXE functions and 37 missing providers, with no executable.

Graphics startup `0x72a0` and mode callback `0x7170` now have ordinary
historical-SDK C++ source. The startup retains eight normalized differences
over its full 544 bytes and stays `RECONSTRUCTED`. Callback48 normalizes but
retains a relocation, so it is `CODEGEN_SIMILAR`. All 654 previous rows remain
identical, and all 133 exact functions pass fresh verification.

```
python scripts/host_graphics_startup_differential.py --output work/graphics_startup_replay
```

Twenty-nine guarded cases compare the full8192-byte reciprocal prefix, whole
1032/1068-byte palette/bitmap allocations including extra SDK entries, existing
47-byte state and actual synchronous mode callbacks. They check normal-path
lock code, error branches, live device/window reloads and callback stdcall ABI.
Platform/malloc effects and table71A0 remain modeled; fatal cases stop at call
entry. No display or process runs. `malloc` is the identified CRT10B20 member,
not the separate frame factory's still unresolved operator-new selection.

In that first context table71A0 stayed missing because its 8192-byte remap write overlaps existing
WINDOWPLACEMENT/RECT views and 192 bytes of another lookup. Recover their real
shared backing before declaring production tables. See
`evidence/host_graphics_startup_recovery.json`. Including the real startup,
DirectDraw import library and already measured lock-byte provider produces a
natural graph of101 units/135 recorded EXE functions with36 missing providers,
no duplicates and no executable.

The shared table/window backing is now recovered as a working SDK aggregate.
Loader68C0 and the pinned palette member establish31persisted remap rows.
Initializer71A0 writes32identity rows, not zeros: its last row spans64window
bytes and192average-table bytes. The full-byte test caught the initial zero-fill
interpretation; it was corrected from native MOV AL and the existing oracle.

```
python scripts/host_graphics_tables_differential.py --output work/graphics_tables_replay
```

Two distinct memory seeds produce equality over all335680generated bytes,
including the ramp overwrite and all four blend formulas. Palette writer/refresh
are still modeled leaves. Existing WINDOWPLACEMENT/RECT now use proved member
views; their standalone60-byte definitions are removed. The784-byte window
consumer survives modulo exactly two data member offsets; other initialized/BSS
semantics remain unchanged. Source71A0 has full code differences and stays
`RECONSTRUCTED`. This ordinary representation preserves the native overlap but
does not prove the original aggregate, allocation, TU or linker placement.
All 133exact functions and29graphics-startup cases remain valid. See
`evidence/host_graphics_tables_recovery.json`. The natural graph now has103units,
136recordedEXE functions and36missing providers;71A0 is resolved and the actual
palette setter6780 is exposed. No game executable is produced.

Palette writer6780 and RGB getter6810 now have SDK source with complete
144/64-byte normalized equality; data relocations prevent raw matching.

```
python scripts/host_palette_entries_differential.py --output work/palette_entries_replay
```

Twenty guarded comparisons cover endpoint0/255 overrides,low16/BYTE truncation,
the green caller-slot byte mutation,aliased outputs and live global pointer
reloads. The table-to256writer chain executes actually,including grayscale RGB,
palette flags4 and bitmap reserved0. Only refresh69A0 remains modeled. A shared
typed caller declaration preserves every byte/relocation of table256; both
335680-byte table regressions still pass. Original common prototype/source/TU
and arbitrary invalid-pointer domains remain unproved. See
`evidence/host_palette_entries_recovery.json`. All657prior rows and133exact
functions remain valid. The natural graph now has104units,138recordedEXE
functions and35missing providers,with no executable.

Cached-color fill at `0x6ee0` now supplies the missing palette refresh dependency,
with complete 272-byte normalized equality. Its palette search at `0x6850` has
source but retains code differences. No exact promotion follows.

```
python scripts/host_color_fill_differential.py --output work/color_fill_replay
```

Twenty-five guarded cases compare all 16 cache bytes, 76,800 bitmap bytes and
100-byte fullscreen effect descriptors, including locked calls, cache hits,
low-byte conversion, null targets and ignored Blt failure. Search uses fixed RGB,
retains the first equal minimum and stops on zero distance. The first recurrence
hypothesis was falsified: the loop includes the red reload, and gray 255 selects
255 rather than 170. Source was corrected from that discriminator.
Only COM Blt remains modeled. All 659 prior rows and 133 exact functions remain
intact. See `evidence/host_color_fill_recovery.json`. The natural graph now has
106 units, 140 recorded EXE functions and 34 missing providers, with no executable.

The journaled promotion gate now supports `.c` and `.cpp` destinations under
`src/`. A C++ candidate must already occupy its exact canonical path. The gate
freshly compiles both frozen C++ and that canonical path, using only the recorded
flags for the canonical proof. Complete raw equality, zero object/native code
relocations, reviewed object contributions, and fresh verification of every
accepted peer remain mandatory. Transitive literal-quoted repository headers
are observed through verification and the transaction; their hashes are context,
not function acceptance. Conditional directives are scanned conservatively;
macro includes are not expanded. SDK/MFC inputs use the pinned toolchain.

```
python scripts/verify_cpp_promotion.py --original-root D:/Prog/hercules_recon --output work/cpp_promotion_replay
```

The isolated controls cover exact acceptance, mismatches, extra code/data,
language and destination errors, accepted-peer regressions, direct/nested header
mutation, transaction commit/rollback, eight historical C rows and pinned target
lookup. They do not change the real recovery state. See
`evidence/cpp_promotion_gate.json` for commands, provenance and the actual erasure
promotion. Do not use header hashes or relocation diagnostics to waive the raw
gate, and do not rewrite relocation-only equals to force native link placement.

The resource-action cluster now supplies source for ENG1 `0x28290`, `0x128a0`
and `0x12910` using the existing shared owners. Its 518 finite comparisons pass,
the measured data providers remain equal, and all 132 accepted functions remain
raw-exact. These three rows remain `RECONSTRUCTED`; relocation-excluded diagnostic
similarity is not acceptance. The genuine ENG1 graph compiles 397 functions but
still has 532 unresolved symbols. Removing the `0x28290` fail marker does not
establish a new runtime frontier or playable gameplay. See
`evidence/eng1_resource_action_masks.json` for the proof boundary and commands.

The ENG1 color-transition initializer at `0x1eba0` now binds its existing callback
name to real source and three independent measured storage prefixes. Five cases
and 178 checks pass, including unequal table values and complete node/state/timer
bytes. The data audit passes for 59 providers, 923 fields and 4,787 pointer cells;
all 132 accepted functions remain exact. The current genuine graph has 398
compiled functions, 531 unresolved symbols and no duplicate providers. The
initializer remains `RECONSTRUCTED`: its complete raw contribution differs.
Following the native target/current load lifetime reduced its ordinary C output
from 96 to 80 bytes. The complete 80-byte relocation diagnostic now agrees, with
all eleven operand locations consistent; the raw gate still rejects those
relocations. This does not prove native module layout or linking.
The broader startup and selected-color producer remain incomplete. See
`evidence/eng1_color_transition_init.json` and run:

```
python scripts/eng1_bounded_differential.py color-transition-init --native-eng1 D:/Prog/hercules_recon/assets/pc/HERCULES/ENG1.DLL
```

`python scripts/region.py packet <region>` derives a small HOT packet from the
explicit index in `evidence/region_index.json`, selected WARM receipts, and current
`recovery.json`. `--more` adds reviewed target spans. COLD disassembly, verbose logs
and superseded variants stay in ignored scratch and are opened only for a specific
question. The index is a reading list, not task status or a second recovery ledger.

Packets identify source/context, unresolved functions, selected frontier, tried
families, blockers, re-entry evidence, next discriminator and ungated work. Changed
baseline/oracle/flags/symbol context invalidates old receipts. Canonical closure is
overlaid from recovery state each time; fully closed regions expose no frontier.
`unresolved` and `canonical_in_region` describe strict closure. The separate
`recorded_non_strict_functions` list exposes existing source, recovery status,
proof scope and extent agreement; `unrecorded_functions` means absent from
`recovery.json`, and may still have private drafts. Use these lists to avoid
duplicating recovered source while preserving every strict pending function.
For header-using regions, `context_files` lists the bounded transitive repository
dependencies. Their hashes participate in context identity and are captured
before/after fresh compilation. Receipt writing rejects missing/stale hashes;
an existing-object analysis cannot establish current header provenance. Historical
specs without this list retain their old identity. These hashes are not a compiler
or acceptance cache; SDK/toolchain inputs remain pinned by the toolchain manifest.
Workers own disjoint `work/<lane>` and `candidates/<lane>` paths. One integrator
publishes compact durable evidence and promotes source; workers never edit shared
status logs or accepted state.

## A bounded experiment

Write metadata with region/lane, family, prediction, falsifier, model/effort,
capability and, when available, elapsed time/build count. Unknown tokens/cost stay
null. Before a family, run:

```
python scripts/region.py check --spec SPEC --baseline BASE --meta META --prior RECEIPT
```

Published negative evidence is also checked. An exhausted/falsified/blocked family
requires new evidence, a new discriminator, or changed context. This is a guard
against accidental replay, not an automatic judge of a hypothesis's quality.

Compile with the existing pinned tools and run the relevant semantic harness.
Record the resulting report/object without another exploratory build:

```
python scripts/region.py record --spec SPEC --baseline BASE --source CANDIDATE --report REPORT --meta META --parent RECEIPT
```

This rechecks full raw contributions and source/object/oracle identities. It does
not prove compile freshness independently of the reported experiment, cache proof,
or replace fresh promotion. A receipt retains a baseline-bound source patch,
compact measurements, provenance and unknown economics explicitly. Resume with
`materialize --receipt RECEIPT --baseline BASE --output candidates/LANE/new.c`.
Stale source recipes fail rather than silently applying elsewhere.
Recipe source identities use UTF-8/LF text; the actual compiler-input byte hash is
recorded separately. Restored text is always freshly compiled when verifying it.

Output identities include complete function bytes and ordered relocation names,
types and offsets, plus non-code contributions for the measured-region identity.
They exclude object timestamps and paths. Local label renumbering may cause a
conservative false distinction; equality is never manufactured by masking operands.
Equivalent outputs share a representative; regressing raw-exact peers are excluded.
A small diagnostic Pareto display retains useful alternatives; it does not route
models, predict difficulty, or change the acceptance decision. Receipts and compact
failed-family conclusions survive; no destructive scratch garbage collector runs.

Keep successful sources and reproducible recipes, not every candidate spelling.
Measure progress in accepted bytes/functions and explained blockers, not variant
counts. See `docs/workflow_experiment.md` for the actual transfer and production run.

For a derived cross-module byte view, run `scripts/convergence.py` with explicit
module function maps. It overlays `recovery.json` on the shared research reading
list in `evidence/convergence_families.json`; untracked span leads may already have
drafts. Byte ceilings are not gain forecasts, difficulty scores or routing state.
See `docs/blood2_transfer_plan.md` for the audit, independent source/compiler
workstreams and the reproducible constant-lifetime pilot. The existing strict
promotion and negative-family reentry rules still apply.

Use `python scripts/leaf.py residual MODULE RVA SIZE SYMBOL SOURCE --islands 1`
to inspect the first grouped non-relocation mismatch without an entire listing.
Add `--report work/LANE/residual.json` for raw/masked measurements and provenance.
It recompiles with the pinned toolchain, retains binding conflicts, and explains
complete instruction windows (ordering, register operands, or other differences).
Linear decoding can include inline data; these labels never establish behavior or
acceptance. `diff` now normalizes only the encoded relocation operand, preserving
other constants and index scales in the same instruction. See
`docs/solo_blocker_progress.md` for the measured pilot and stopped source families.

`python scripts/archive_storage.py --output work/archive_storage/report.json`
freshly checks the canonical archive scalar/string and public-handle providers
against durable field evidence. It verifies initialized values, writable data
class, supported loader-zero extents, exact public definitions, and stable inputs.
This is a data dependency diagnostic, never function or whole-image acceptance.
It also checks the fatal service's observed four-byte HWND and two byte flags
when their canonical provider/evidence is present. Those six bytes establish
access-width storage only; original aggregate extent, ownership and placement
remain open. It rejects a missing half of the provider/evidence pair.

The same verifier checks registry providers when their canonical proof is present:
49 writable initialized string bytes and the 72-byte loader-zero protocol prefix.
The prefix is not proof of the original record's full extent. The host context
pointer belongs to `EngineInterface.context_004`; leave the actual host table
external until its genuine initializer and dependencies are recovered.

Write durable text as LF before hashing it, as required by `.gitattributes`.
For example, write UTF-8 serialized bytes with `Path.write_bytes`, or explicitly
set `newline="\n"` when writing text. Before a checkpoint, compare source SHA-256
proofs with the actual staged blob bytes. Historical scratch receipts keep their
original identities; migrate a durable receipt only for its explicitly named,
unchanged artifact when normalizing text. Do not replace historical source
snapshots with the current source hash.

Graphics storage uses the same fresh field verifier. Its 27 observed cells
contribute one initialized byte and 150 loader-zero bytes; compiler alignment,
heap targets and original aggregate/TU/placement remain outside that proof.

Do not infer a DWORD source object from a DWORD load followed by an FF mask.
Pinned RTM BYTE controls show that a same-TU one-byte definition can emit that
form with ordinary alignment, while an external BYTE uses a byte load. Recover
the actual minimum object and unit context, then run the complete function/data
gates. Do not add DWORD storage, unions or pointer casts just to force the load.
See `evidence/byte_load_context_control_2026_10_11.json`.

The field gate also checks six observed sound/thread/priority cells: 24 zero
bytes with no compiler padding. Real SDK creators and save/restore users support
the working pointer, HANDLE, DWORD, int and BOOL views. This establishes no
pointed-to object, independent original declaration, aggregate, TU or placement.
See `evidence/audio_storage_adoption_2026_10_11.json`.

The field verifier now checks the supported 4,800-pointer sound-bank prefix,
two actual DirectSound pointer cells, a shared byte flag, six MCI state fields,
and the writable stop command. The native 4,800-DWORD clear and creation/duplicate
consumers prove the bank prefix; the neighboring 24-WORD cache and gaps remain
excluded. No pointed-to objects, original full capacity, aggregate/TU or native
placement follow from these providers.

Keep legacy negative receipts separate when their measured input identity is
missing. When new caller/type/unit evidence permits work, freeze and freshly
measure the current ordinary source before changing it. ENG1 44740 illustrates
why: the old 23-byte/722-attempt report lacks source/object/command/target binding,
while the current frozen source independently matches all 128 bytes. Its exact
cause remains unknown. Normal FPO metadata relocations in `.debug$F` are outside
the function-scoped raw gate; do not turn a whole-object diagnostic into a new
acceptance requirement. Existing promotion still checks the full code contribution
and every prior strict function.

The same field gate checks the canonical `EngineInterface` provider against two
module-specific loader-zero ranges. Its 1,000-byte object is compiled separately
into each engine image; it never supplies the initialized EXE host table. Engine
prefix bytes are reported by module, separately from the EXE data totals. Context
and allocator cells inside the prefix must use actual structure members rather
than separate overlapping definitions. A diagnostic link that resolves the table
symbol does not establish native placement, entry/startup or whole-image equality.

Archive directory and logical-position prefixes now use ordinary explicit-zero
definitions under the same field gate: 12,288 and 4,096 observed bytes. The full
table's native next-key probe crosses into the first position word; storage
acceptance does not prove that initializer's source representation or safety for
arbitrary input. Do not add a 1,025th directory row or force an aggregate/adjacency
to hide this issue. The scratch buffer remains external because no actual bound
has been recovered. See `evidence/archive_array_prefixes_adoption_2026_10_11.json`.

Use `function_map.py MODULE --entry-evidence EVIDENCE --output work/LANE/map.json`
for reviewed initialized pointer-table entries that the reference index missed.
The evidence binds the complete oracle, initialized table bytes, entry bytes and
actual pointer cells; recorded function extents still take precedence. These are
diagnostic map leads, never recovery state or relaxed function acceptance. The
host interface pilot fixes the previously concatenated BDD0/BDE0 spans and yields
two new strict callback leaves. See
`evidence/host_interface_entry_seeds_2026_10_11.json`.

Archive open/seek, normalizer and initializer now live in their canonical units
without duplicate read/hash/shutdown definitions. The existing providers supply
those functions; current region specs follow this partition while old receipts
keep their original identities. Compare local COFF labels by proven own-function
offsets when reviewing source preservation: label spelling alone is conservative
diagnostic metadata, not a reason to alter the actual matching gate. The old and
new normalizer switch labels both point to offset 392.

The reconstructed-source fixtures link the actual separate providers and use the
actual int hash and void shutdown views. Shutdown tests assert the observed
cleared-stream invariant; no return-value guarantee follows from callers that
discard EAX. This corrects the fixture's old assumption, not the matching rules.

For SDK clear mismatches, transfer the controlled experiment, not a rule from
another compiler. Hercules RTM controls independently show aggregate zeroing can
elide overwritten fields while ordinary exact-size memset preserves full ranges.
F3D0 also demonstrates a real escaped variable reused across sequential decoder,
sample-copy and duplicate phases. These reproduce specific frame/clear patterns;
the complete function still fails and is not partially promoted. See
`evidence/sdk_cursor_initializer_controls_2026_10_11.json`.

Loop-weighted literal comparisons in disjoint branches can affect RTM constant
qualification and register class. A fixed two-arm control changes only a sibling
comparison from 6 to 7: reset 6 changes from EBX to ESI. Neither form produces
TITLE's two independent scratch-register pieces. Preserve the actual post-call
state reload; an else-if changes the native sequential conditions. A register
class change alone does not justify another full target build.

Read-only asset formats can distinguish fields without proving their original
source owner. All 559 reviewed PC ESF headers support independent bytes at +12
and +13 and a low-WORD rate at +8. A same-TU mixed-prefix sound control reproduces
wide initial loads and later byte loads, but the complete function still fails.
Its shifted 387-byte suffix is diagnostic only. Keep the prior 393-residual
recipe; the new seven-byte prefix difference is a row-address/index-context
question, not permission to insert bytes or promote an aligned suffix. See
`evidence/constant_header_controls_2026_10_11.json`.

On the current Windows host, a sandboxed historical `cl.exe`/`link.exe` launch
can fail in CreateProcess with WinError 623 before the compiler starts. Treat
this as a launch failure, not a compiled negative or a reason to change flags,
environment, DLLs or pinned tools. Run the same frozen command through approved
escalated execution and record the unsuccessful launch separately from actual
object builds. The working Python interpreter may also differ from the blocked
Windows Store alias; resolve an available interpreter before the experiment.

Pass the exact COFF symbol to leaf comparison commands: ordinary x86 C cdecl
names have a leading underscore. A symbol lookup failure after compilation does
not invalidate the fresh object. Inspect its symbols and compare that same object
with the corrected name; record the wrapper failure separately from actual
compiler builds rather than recompiling unchanged source.

Compiler-generated `$S` ordinals can change when a genuine header/member binding
changes. A private preservation review may establish the same named source
object through identical storage class, BSS section/value/type/addend and full
storage bytes. Do not blindly normalize names or relax the native matching gate.
The TITLE member pilot preserves all 32 complete functions / 8,816 bytes and
removes two overlapping pseudo-globals from the actual source graph. See
`evidence/title_interface_member_rebind_2026_10_11.json`.

For an inferred plain-COFF region, use the read-only
`masked.masked_unit_compare(obj, target, rva, size, expected_functions)` API.
`expected_functions` must name every defined function and its native-derived
offset from the proposed region start. It requires one complete non-COMDAT code
section and compares the whole contribution; it does not extract function slices.
`masked_equal` describes code only, while `unit_equal` additionally requires all
initialized sections to have consistent inferred bases and complete raw equality,
with no omitted storage. Neither result accepts a function, TU or native layout.
Keep genuine-prefix uncertainty explicit and leave normal staging/promotion
unchanged. See `evidence/fx_unit_convergence_2026_10_11.json` for a pinned example.

The original indexed bitmap copy at EXE `0x6530` now has ordinary historical
source, preserving float32 ratios, x87 accumulation and forward byte copies.
Its actual CRT `__ftol` is identified as libcmt member 602: all 39 bytes equal
the native helper and the independently selected, normally linked library body.
It is library code, not a rewritten recovered game function.

```
python scripts/host_bitmap_copy_differential.py --output work/bitmap_copy_replay
```

Twenty-six cases compare whole 393,216-byte arenas, guards, ABI and FPU state,
including overlap, row padding, a 320x240 frame and five control-word settings.
No conversion effect is modeled. The source remains `RECONSTRUCTED`: the complete
240-byte contribution retains five diagnostic differences in scheduling the
column clear around the first conversion call, plus four relocations. A bounded
source-order discriminator reduced nine differences to five; no spelling grid
was used. All 133 accepted functions remain raw exact. This removes the actual
bitmap-copy dependency from display-mode transition; it does not run a display
or game. See `evidence/host_bitmap_copy_recovery.json`.

The private `0x5a50` platform-state draft has a new semantic discriminator:
the native function reloads mode/type/depth after callbacks, while the old draft
retained snapshots. A focused correction passes 22 caller-only comparisons and
four mutation cases reject the old draft. All callback effects are modeled, so
this establishes caller behavior under those preconditions, not real callee
behavior. The corrected extent is 464 bytes but 147 diagnostic differences
remain. Shared cell/callee bindings and cache/literal auditing are still pending;
no canonical source or recovery row was adopted. Preserve the reload finding for
that integration. See `evidence/host_platform_sync_reload_discriminator.json`.

The actual CD unpause `0x9db0` and resume `0x9f60` now have canonical source
using the existing MCI and registry owners. The former has full 96-byte
relocation diagnostic equality; the latter has the full 336-byte extent with
143 diagnostic differences. Neither is raw exact.

```
python scripts/host_cd_resume_differential.py --output work/cd_resume_replay
```

Eighty-eight guarded native/source cases execute actual pause, unpause, resume,
readiness, playback, cleanup and timer bodies. The 35 prior playback/timer cases
remain included. Tests preserve live state after callbacks, tick wrap, failed
commands and the SDK WORD-minute packing, whose high bits overlap the second
field. Zero start/already-playing resume leaves paused state unchanged, while
disabled or failed-readiness resume still adjusts a live paused tick. Only OS
and identified CRT boundaries are modeled. All 133 accepted matches remain raw
exact; the selected host graph has no duplicate providers and still has 33 real
missing dependencies. See `evidence/host_cd_resume_recovery.json`.

The actual DirectSound channel stop `0xfa10`, pause `0xffb0`, resume `0x10030`
and bank restore `0xf760` now have canonical SDK-based source. Three complete
contributions have relocation diagnostic equality after a single signed-loop
discriminator; stop retains 177 diagnostic differences. None is raw exact.

```
python scripts/host_sound_channels_differential.py --output work/sound_channels_replay
```

Forty-four comparisons execute actual stop-all, channel control, polling and
bank release. Tests compare complete bank/channel/registry/loop state and the
synthetic COM arena, with post-COM reload mutations, signed-short slot truncation,
paused guards, ignored failures, cursor/loop restoration and boundary bank 1199.
Restore requires status exactly 2; mixed status 6 does not qualify. COM effects and
the still unrecovered F3D0 initializer alone are modeled. Its production symbol
remains unresolved. The new 4-byte pause cell and 1200-byte loop prefix have fresh
BSS and native virtual-zero backing; existing state providers are reused.
All 133 accepted functions remain raw exact. The real FA10 dependency is resolved
and the real F3D0 dependency exposed, leaving 33 unresolved symbols in the selected
110-unit graph. See `evidence/host_sound_channels_recovery.json`.

The DirectSound fixture now binds its cleanup exception flag to the proven
`0xb9e450` cell, rather than the earlier incorrect `0xb9e448` shadow. Five bank-824
release/restore cases distinguish zero, one and high-bit flag states. A deliberate
negative run with the old address fails the release-count assertion. All seven
executed game contributions remain byte-for-byte unchanged across fresh fixture
builds; no recovery row changed. Earlier 39-case receipts retain their historical
limits. See the correction in `evidence/host_sound_channels_recovery.json`.

The original nibble decoder `0x2410` now has ordinary C source and the complete
16/89-DWORD numeric lookup tables. Fresh initialized object and fixture bytes
equal all 420 original table bytes; original constness/TU/layout is unproved.

```
python scripts/host_sound_decoder_differential.py --output work/sound_decoder_replay
```

Thirty-nine whole-arena cases execute actual native/source decoding with no
modeled decoder operations. They cover every nibble, saturation, count boundaries,
state seeding and input/output/state aliases. Preserve the native low-WORD
zero extension and final signed predictor OR, which can overwrite high state
bits. The source remains `RECONSTRUCTED`: its 256-byte contribution differs from
the 288-byte native contribution. All 133 accepted functions remain raw exact.
This supplies an actual sample-decoding dependency for F3D0 validation, without
reopening its exhausted compiler families. The selected host link still has 33
unresolved dependencies. See `evidence/host_sound_decoder_recovery.json`.

The original sound-bank constructor `0xf3d0` now has canonical source rebound
from the preserved shared-cursor/memset recipe. Its complete 832-byte contribution
retains 393 diagnostic differences; no exhausted compiler family was reopened.

```
python scripts/host_sound_bank_differential.py --output work/sound_bank_replay
```

Twenty-nine cases execute actual construction, nibble decoding and bank release.
They compare whole header/counter/bank/loop state, synthetic PCM scratch and COM
heap, descriptor/format bytes and ordered events. Coverage includes both PCM
widths, odd lengths, split/empty locks, failed/null creation, ignored HRESULTs,
duplication failure cleanup and bank 824 protection. Archive/format/fatal/COM
effects remain modeled. A separate 44-case caller replay preserves all seven
complete game contributions after adopting a common working void declaration.
The new 16-byte header and 2400-byte counter prefix have fresh/native zero backing,
and full native literals agree. Production scratch remains external; 1MiB fixture
backing is not a capacity proof. The natural link resolves F3D0 and exposes this
real storage dependency, retaining 33 missing symbols without stubs. All 133
accepted matches remain raw exact. See `evidence/host_sound_bank_recovery.json`.

The sound scratch base now has a measured one-MiB accessed-prefix provider.
This follows new actual-asset evidence: 250 of 559 PC sound headers require one
packed byte more than the constructor reads, and the decoder consumes the final
byte at offset `0xfffff`. Real member 184 proves that changing only that byte
changes the final PCM sample from 23 to 559 or -589. Member 139 does not read it.

```
python scripts/host_sound_scratch_data.py --output work/sound_scratch_data_replay
python scripts/host_sound_bank_differential.py --output work/sound_bank_prefix_replay
```

The fresh provider has exactly 1 MiB of BSS, no code/relocations and complete native
virtual-zero backing. All 559 header access bounds fit; 33 constructor cases, 44 caller
cases and all 133 exact matches pass. Existing game code contributions are unchanged.
The natural link resolves this real storage dependency and now has 32 missing
providers. Original total allocation,type/TU/layout,indirect ownership closure
and arbitrary-header/COM safety remain unproved. Earlier fixture-only receipts
retain their historical scope. See `evidence/host_sound_scratch_recovery.json`.

The original platform-state reconciliation at `0x5a50` now has canonical source
bound to existing input/pending,registry and audio/display owners. It executes
the actual eight platform callbacks and eight descendants,including restoration,
construction,decoding,release,CD readiness and priority transitions.

```
python scripts/host_platform_integrated_differential.py --output work/platform_integrated_replay
```

Thirty-seven whole-state/heap/event comparisons pass. They retain the native
cache early-return behavior even with dirty flags, signed-byte comparisons,
full-DWORD fields, post-callback reloads and final live cache capture. An ignored
negative copy restoring the old snapshots skips sound pause after the controlled
COMStop mutation;native and corrected source execute it. Only OS/COM/CRT/archive
boundaries are modeled. Fresh initialized cache bytes and complete native title
literals agree; new dirty BYTEs have native zero backing. Source 464 retains 147
diagnostic differences and remains `RECONSTRUCTED`. All 133 accepted matches remain
raw exact. The selected 117-unit host graph resolves this real dependency and
has 31 missing providers. Earlier caller-only receipts retain their historical
scope. See `evidence/host_platform_sync_recovery.json`.

The actual sound-startup service `0xfdf0` and primary-format helper `0xfd50` now
have canonical SDK-based source. Their complete 336/128-byte contributions retain
140/11 diagnostic differences and remain `RECONSTRUCTED`.

```
python scripts/host_sound_startup_differential.py --output work/sound_startup_replay
```

Twenty-five comparisons execute actual startup, format, gain and the identified
39-byte CRT conversion helper. All 16384 volume entries agree with an independent
60-digit decimal reference across five x87 control-word settings; no logarithm or
conversion is modeled. Tests preserve selective resets and untouched neighbors,
the lowWORD duplicated into both gains, ignored HRESULTs, live device reload and
the format helper's saved prior-rate return. Only DirectSound/COM effects are
modeled. Four measured zero-backed data prefixes and the existing pinned
DirectSound import library supply genuine dependencies. All 133 accepted matches
remain raw exact. The selected 119-unit graph resolves FDF0 and now has 30 missing
providers. See `evidence/host_sound_startup_recovery.json`.
