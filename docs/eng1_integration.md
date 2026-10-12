# ENG1 original-host integration

The original host has loaded the transitional source-built ENG1 DLL and copied
the expected 250-DWORD interface with signature 0x5ac00cac and first-use gate
change from 1 to 0. The latest windowed run stops at explicit missing RVA 0x27d40.
Initialization, a gameplay frame and normal engine shutdown remain unverified.

Earlier runs stopped at 32b10 and 274e0, now implemented. Runtime testing then
exposed a packed-header cursor error in 20670: source used root+0x26 while
native uses root+0x24. The correction passes 46 original-x86 checks on the full
shipped FOREST map and gets past the invalid row read in 20830. Artifact-bound
observations and limits are in evidence/eng1_runtime_validation.json and
evidence/eng1_resource_cursor_correction.json.

The descriptor scan at 3910 and link initialization at 39b0 are now implemented
and pass finite original-x86 comparisons. The scan fixture also caught a real
cleanup discrepancy at 34d0: residency state selects destruction; entry count
does not. Corrected windowed runs advanced through 3910, 39b0, 33e0 and 4ca0 before reaching
setup15090, player dispatcher15d00, query30010 and the current constructor dependency stop. Cold-frame command writers also pass 14 native cases,
but the runtime has not reached their frame submission yet.

Preparation uses locally verified installer files and the stable source TITLE
build/report required by scripts/title_integration.py. Source TITLE retains SHA
0dde33225d04557aa81e3870674ea9b08edb7faa38ef3f6db978e08fe54ab775.
Original data, ENG3, and assets/ remain immutable. The authorized one-byte
no-CD change applies only to a disposable EXE copy and is hash-checked.
Original binaries, extracted maps and generated builds are ignored.

Reproduce a diagnostic build and one bounded integration attempt:

```powershell
python scripts/eng1_behavioral.py
python scripts/eng1_provisional.py
python scripts/eng1_behavioral.py --include-incomplete --output work/eng1_behavioral/transition_build
python scripts/eng1_data_evidence.py
python scripts/test_eng1_behavioral.py
python scripts/eng1_resource_cursor_differential.py
python scripts/eng1_worklist_factory_differential.py
python scripts/eng1_descriptor_resource_differential.py
python scripts/eng1_bounded_differential.py resource-links
python scripts/eng1_bounded_differential.py command-writers
python scripts/eng1_bounded_differential.py descriptor-reload
python scripts/eng1_bounded_differential.py dialog-frame
python scripts/eng1_bounded_differential.py simulation-root
python scripts/eng1_bounded_differential.py query-decoder
python scripts/eng1_bounded_differential.py grid-scan
python scripts/eng1_bounded_differential.py extent-producer
python scripts/eng1_bounded_differential.py actor-bounds
python scripts/eng1_bounded_differential.py grid-query
python scripts/eng1_bounded_differential.py index-query
python scripts/eng1_bounded_differential.py actor-update
python scripts/eng1_bounded_differential.py query
python scripts/eng1_bounded_differential.py player-setup
python scripts/eng1_bounded_differential.py setup-leaves
python scripts/eng1_bounded_differential.py simulation-helpers
python scripts/eng1_bounded_differential.py payload-dispatch
python scripts/eng1_resource_initial_data.py
python tests/test_eng1_data_evidence_contract.py
python scripts/eng1_integration.py build-probe
python scripts/eng1_integration.py prepare --runtime work/eng1_behavioral/integration/experiment
python scripts/eng1_integration.py attach --runtime work/eng1_behavioral/integration/experiment --report work/eng1_behavioral/transition_build/report.json
python scripts/eng1_integration.py inspect --runtime work/eng1_behavioral/integration/experiment
python scripts/eng1_integration.py launch-once --runtime work/eng1_behavioral/integration/experiment --report work/eng1_behavioral/transition_build/report.json --enter-game --timeout-ms 45000
```

The first genuine build currently fails to link and records missing symbols.
The optional profile explicitly supplies fail-on-entry diagnostics; those
definitions never return success and do not count as reconstructed functions.
Every entry records its original RVA in the centralized incomplete manifest.
The final engine must replace reachable incomplete behavior with real source.

Choose a fresh runtime directory for each different DLL artifact. Attach checks
PE32, the sole PC_DLLEngineMain export at ordinal 1, absence of original-engine
imports, source/context hashes, DLL/map hashes and mapped interface addresses.
Unknown or unrecorded runtime files are refused. Preparation does not launch.

The source-built observer uses DEBUG_ONLY_THIS_PROCESS with a 5 to 120 second limit.
It verifies native mode bytes and window caption/menu/client size, and sends
the observed Alt+Enter and window-size commands. With --enter-game, it sends
at most six real Enter pulses only while its own PID is foreground and windowed
and modifier keys are released. It releases its key and stops new pulses when
ENG1 loads. Omit that flag for passive observation.

The probe reads loaded paths, the full interface, selected mapped source cells,
exception parameters, x86 registers and a bounded stack. It never patches code
or writes target memory. The first incomplete operation or access violation
ends only the owned test process after capture; this abort is not normal shutdown.
Each run has a separate UTC/UUID directory under work/eng1_behavioral/integration/runs/.

Reduce an actual recorded attempt to a compact receipt:

```powershell
python scripts/eng1_runtime_receipt.py work/eng1_behavioral/integration/runs/RUN_ID/report.json
```

The receipt verifies recorded DLL/map identity and distinguishes historical
build context from the current checkout. It does not rerun the game or promote
recovery state. Original oracle code executes only in separate differential
fixtures; the replacement never dispatches into original ENG1.

The optional `--capture-call 0x4ca0` observer mode is reserved for the reviewed
source caller. It derives the return site from fresh COFF relocation and linked
CALL bytes, scans at most 512 stack DWORDs after an actual exception, and reads
three arguments plus at most 128 input bytes. This is a bounded return-address
candidate, not an entry breakpoint or general stack unwind. The compact receipt
keeps only capture hashes and provenance; extracted input remains in `work/`.
The payload differential runs six synthetic cases without any extracted data.
Add `--captured-payload work/eng1_behavioral/differential/payload_dispatch_4ca0/actual_payload.bin`
when the reviewed private capture is available to reproduce its seventh case.
