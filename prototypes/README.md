# Prototypes (tracked, non-canonical)

This directory holds reproducible experiment sources and compact receipts. The
ignored `experiments/`, `candidates/` and `work/` stay for scratch.
Use [project direction](../docs/project_direction.md) to decide whether another
experiment resolves a current reconstruction question. These are historical
tools and bounded seams, not separate long-term products or an automatic queue.

Nothing here is reconstruction source or acceptance evidence. It never
modifies `recovery.json`, `src/` or `calibration/`. Findings move into
`renderer_contract/` only after a byte-level audit re-derives them.

| Directory | What it is |
| --- | --- |
| [rea_eval/](rea_eval/README.md) | Measured evaluation of REA (rea-agents 6.5.0 + Ghidra 12.1) on the pinned originals |
| [host_shim/](host_shim/README.md) | x86 host-interface shim DLL with a backend ABI, a synthetic harness and an original-AC60 differential |

## Outcome of the 2026-10-15 experiment

1. **REA works** in this environment as an isolated, pinned install, with no agent configuration changes.
   - Unseeded, it misses 88 of 165 host callback targets, including AC60.
   - With seeds generated from repository evidence, it recognizes all 165, and its AC60 analysis matches our contract (body, 11 callees, gate and traversal).
   - It mislabels signatures and calling conventions, and raw output is very large (1.57 MB for one function).
2. **New knowledge.** Host `0x79A0` is the application's MFC `OnIdle` override (vtable `0x42E1D8`, slot 26), driven by `CWinThread::Run`'s idle loop.
   - `InitInstance` ends with the direct DirectDraw/engine-load call `0x5E10`.
   - The engine frame runs on the UI thread under an `__except` whose handler is fatal (`exit(-123)`).
   - The REA session supplied the lead; [audit_idle_owner.py](../renderer_contract/tests/audit_idle_owner.py) proves it from bytes.
3. **DLL architecture.** The smallest reliable design is the existing app-local `ddraw.dll` proxy (bootstrap and presentation) hosting a table-clone shim (render commands).
   - The shim compiles, relays 13 slots ABI-exactly, and isolates backend faults.
   - It passes 346 synthetic checks and 9 checks against the original AC60 bytes.
   - Full frame ownership still requires taking over every host-internal writer. Interception of slot 89 alone is not enough.

## Next steps

- **Complete, static scope:** the [frame-pacing audit](../renderer_contract/frame_pacing.md) establishes `5F20`'s 33 ms step, catch-up and reset coupling.
- **Complete, synthetic scope:** the [combined app-local `ddraw.dll`](../dropin_renderer/docs/combined_dll.md) builds from the existing forwarder and shim and passes fresh integration/regression checks. Native rendering and original-process deployment remain open.
- **Gated:** any live run needs the AGENTS.md authorization gate.
