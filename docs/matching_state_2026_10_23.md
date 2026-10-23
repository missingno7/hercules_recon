# Matching audit and unblocking, 2026-10-23

Fresh verification preserves **132 FUNCTION_MATCH rows / 8,304 bytes**. The
masked regression gate preserves **449 rows in 73 sources**, with zero failures.
The ledger contains 452 CODEGEN_SIMILAR rows; three legacy diagnostics are outside
that gate. Nothing was promoted, and `recovery.json` was not edited in this audit.
The pre-existing scheduler storage/source and recovery edits were preserved.

| Module | Exact rows | Exact bytes | CODEGEN_SIMILAR rows | RECONSTRUCTED rows |
|---|---:|---:|---:|---:|
| TITLE.DLL | 6 | 352 | 333 | 1 |
| ENG1.DLL | 76 | 5,632 | 39 | 1 |
| ENG3.DLL | 25 | 1,712 | 32 | 4 |
| HERCULES.EXE | 25 | 608 | 48 | 0 |

These are matching-state totals, not behavioral source coverage or whole-image
equality. The current branch also contains substantial behavioral reconstruction.
See `docs/eng1_behavioral.md` for that separate graph and its explicit missing
providers; no behavioral status was converted to a match.

## Blockers resolved

**Repeated compilation.** `match.py verify` now compiles each source/ordered-flags
group once within a fresh run, retaining every complete raw comparison. It freezes
all source proofs, conservative transitive literal includes, target identities and
the toolchain manifest, checks object stability and revalidates the pinned tools.
Macro includes, forced includes, response files, PCHs and unsupported flag modes
fail closed rather than receive incomplete provenance. No cross-run cache exists.
Candidate and promotion paths keep their existing gates.

The old pass used 132 compiler calls; the new one uses 30 (77.3% fewer). All 132
raw results and complete parsed section/symbol/auxiliary/relocation inventories
agree across the before/after builds. Object timestamps are excluded from that
cross-build comparison. Observed wall times were 12.1 and 7.2 seconds, respectively;
this is one measurement, not a general performance guarantee.

**Stale source-closure diagnosis.** The recovery-only TITLE graph still has 39
sources, 21 unresolved code names and 75 unresolved data names. The newer explicit
source graph already supplies those dependencies: a fresh private build compiles
all 362 inventoried entries from 44 sources and links naturally with no unresolved
symbols, duplicate providers or placeholders. Its six accepted TITLE functions
remain raw-exact in the linked DLL. This closes that graph's source dependencies,
not native data extent/ownership or original placement.

**Mixed-status units.** `title_match_diagnostic.py` inventories every emitted
section and function in that graph, measures all 340 recorded rows, and retains
22 unrecorded leads with explicitly provisional target spans. Failed members stay
visible alongside matching peers. It uses the existing raw/masked comparators,
validates build/source/object identities, and never writes recovery state.
This avoids reconstructing already-present providers or requiring every member
to match before examining its actual complete source unit.

**Instrumentation mistaken for codegen residue.** The default behavioral build
inserts optional tracing into scheduler/movie functions. Its recorded peers
`05fb0`, `05db0`, `05e40` and `09980` have real byte differences caused by those
insertions. Eight configuration edits are now explicitly tagged `diagnostic_only`.
The new `--matching-diagnostic` profile omits only those tagged insertions, retaining
ownership/type corrections, real providers and the default behavioral profile.
It requires a separate output directory from the preserved default runtime build.

The predicted result holds: all four codegen differences disappear; all six exact
peers and natural-link closure survive. Across 340 recorded rows, 339 are now
masked-equal and the existing decimal formatter remains different. Four peers
(`03050`, `066f0`, `19570`, `195f0`) have changed symbol maps despite matching masked
bytes. These require explicit identity/ABI review; names are not normalized away.
The profile still contains support routines and provisional source. It is not a
claim to have reproduced historical compiler inputs or a strict natural link.

**Unsupported ownership claims.** `unit_bounds.py` now labels its results as
conditional shared-storage grouping. External explicit-zero C definitions can
occupy BSS and have cross-TU users. Its legacy address window is not a proved
communal boundary. `function_map.py` also exposes its 16-byte discovery filter,
which cannot establish completeness or original `/Gy` usage.

## Remaining concrete work

The complete source graph provides a useful current baseline. Its primitive unit
contains 24 emitted functions and preserves all 18 recorded masked peers.
`186e0` emits 704 bytes with two nonrelocation differences in this context. This
is fresh current-graph evidence; the old private full-unit candidate is absent
from this checkout and its older measurement was not reproduced.

All 22 unrecorded leads remain non-equal. Current wrong-sized leads include
`54f0` (1376 versus 1408), `a450` (7440 versus 7408), and the instrumented entry
provider (320 versus the provisional 128-byte span). The recorded formatter is
still 240 versus 224. Old claims that `9c90` and `186e0` are necessarily wrong-sized
are stale for this graph. Complete measurements, including binding conflicts, are
in the private diagnostic; compact values are in the evidence companion.

Two existing 16-byte calibration functions, `016e0` and `01dd0`, also compare raw
equal. They remain CODEGEN_SIMILAR: the initial promotion gate cannot adopt their
mixed/data-bearing units. Do not split their context or bypass that gate to raise
the count. Extending acceptance for independently reviewed complete units and
actual naturally linked contributions is a separate proof obligation.

Next priorities are the four binding-map reviews, the provisional data/extent
reviews needed by the complete graph, and discriminating experiments on its
remaining source differences. Preserve the exact and masked peers. Do not resume
old syntax, declaration-count or compiler-version sweeps without a new discriminator.

## Reproduction and validation

The Store `python` alias is inaccessible in this sandbox. The working interpreter
here is `C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe`;
the project still loads its existing analysis packages from `C:\tools\hercules\python`.
Historical compiler execution requires the approved unsandboxed path on this host
after the documented WinError 623 launch failure. No tools were downloaded or patched.

With that interpreter selected as `python`:

```powershell
python scripts/match.py verify --report work/check/strict.json
python scripts/similar.py verify
python scripts/title_behavioral.py --matching-diagnostic --output work/check/title
python scripts/title_match_diagnostic.py work/check/title/report.json
python -m unittest discover -s tests -p 'test_*.py'
```

All 90 Python tests pass, including mutation/stale-artifact rejection, mixed
outcomes and trace-profile selection. No game or original binary was executed.
All 12 manifest-listed assets match their expected sizes/hashes, and all four PC
oracles pass the matching checks. The inventory command correctly fails on an
additional unregistered European PSX BIN; its identity is recorded in the evidence
companion. Neither that file nor the asset manifest was changed. Establish its
provenance before using it as evidence.

The audit applied ai_decomp's coherent-unit/proof-scope lessons and Blupi's fresh
per-TU verification and protected-peer workflow. Blupi's SP3 and host-CRT findings
do not justify changing Hercules's toolchain. Exact reference commits, commands,
private report hashes, counts and scope limits are recorded in
`evidence/matching_state_2026_10_23.json`.
