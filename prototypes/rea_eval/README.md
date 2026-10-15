# REA evaluation (experimental, 2026-10-15)

The later [ENG1 camera follow-up](../../docs/eng1_camera_bounds.md) uses this
existing client/provider to recover a real missing dependency and correct its
caller's output extent. REA's inferred ABI was rejected using x86 evidence.
The historical calibration below remains evidence for its original inputs;
the follow-up adds source and finite tests without changing strict acceptance.
The [camera-chain follow-up](../../docs/eng1_camera_chain.md) reuses the saved
`26730` analysis for bounded smoothing recovery and corrects a camera step error.

This directory evaluates whether [REA](https://github.com/morluto/rea), the
`rea-agents` npm package (an MCP server/CLI over Ghidra, Hopper, IDA and
others), helps Hercules reconstruction. REA output is **evidence only**. Nothing
here touches `recovery.json`, `src/`, `calibration/`, or acceptance.

## Setup (reproducible, isolated)

| Item | Value |
| --- | --- |
| Package | `rea-agents` 6.5.0 (npm, published 2026-10-14; 6.x shipped daily) |
| Tarball SHA-256 | `531717a06e92ca651aab52691190f916fac044272c01f875e22c5e293218e699` |
| Install | `npm pack` + `npm install --ignore-scripts` into `C:\tools\rea-agents-6.5.0` (173 packages, 39 s) |
| Agent config | untouched: `rea setup` not run; `rea doctor` reports every agent registration absent |
| Provider | existing `C:\tools\ghidra_12.1_PUBLIC` via `GHIDRA_INSTALL_DIR`; JDK 25, Node 24.18 |
| Windows status | REA labels Windows Ghidra "P0 / experimental"; it worked for PE x86 EXE and DLL |

REA copies every target into a private temp runtime as an ephemeral,
source-immutable Ghidra import. Originals were additionally copied to
`work/` before use.

Reproduce everything (5 Ghidra sessions, about 3.5 min):

```powershell
python prototypes/rea_eval/run_probe.py --rea C:\tools\rea-agents-6.5.0\node_modules\rea-agents\scripts\rea.mjs `
  --ghidra C:\tools\ghidra_12.1_PUBLIC --exe work\discs\pc_install\HERCULES.EXE `
  --eng1 work\discs\pc_install\ENG1.DLL --output work\rea_probe
```

- `rea_mcp.mjs` is a 60-line stdio MCP client. It keeps one Ghidra session alive for a whole query plan.
- `make_seeds.py` converts repository knowledge into an REA seed file (`REA_GHIDRA_SEED_FILE`). Its sources are the 250-word callback table read from the original bytes and the EXE entries in `recovery.json`.
- The compact result is [evidence/rea_probe_summary.json](evidence/rea_probe_summary.json).

## Results

### 1. Calibration on host callback 89 (`AC60`)

| Question | REA/Ghidra | Repository evidence | Verdict |
| --- | --- | --- | --- |
| Is `0x40AC60` a function? | **No** (unseeded) | yes, slot 89 submit | REA blind without seeds |
| Callback-table targets recognized | **77 / 165** unseeded; 165 / 165 seeded | 165 | default analysis misses data-only roots |
| AC60 body | `0x40AC60..0x40B2EB` | `0xAC60..0xB2F0` (with padding) | agrees |
| AC60 callees | 11, exactly the documented set | 11 | agrees |
| Indirect calls in AC60 | none | none | agrees |
| Pseudocode semantics | BA01FC gate, live link reload, E1/01/F3, tail order | `closure.md`, `packets.md` | agrees |
| Calling convention | `__stdcall`, low confidence | cdecl, caller-cleaned | **wrong label** |

Missed without seeds: AC60 itself, every geometry slot (100–121), palette 145,
movie 148, text 149 and the notification setters. These are reached only
through the data table, which default auto-analysis does not treat as code
roots. **Feeding our evidence in as seeds is mandatory.** REA supports it
cleanly: seeds are committed into the analysis profile digest.

### 2. Unresolved target: the owner of callback `0x79A0`

`renderer_contract/callback_metadata.md` recorded `0x79A0` as an *unowned*
pointer in an unreferenced 28-DWORD `.rdata` span. That made the scheduling of
the engine tick (`79A0 → 5F20 → 5EC0 → export`) an open question.

What REA contributed:
- Running cheap xref queries over a wider window, `0x42E100..0x42E2A0` (40 queries, about 1 ms each), returned one reference to `0x42E1D8` from `0x4075EA`.
- Message-map and scope-table references at `0x42E170`, `0x42E280` and `0x42E290`.
- Instruction listings showing `0x79A0` is `__thiscall`/`RET 4`. It chains to `0x425886`, whose body is MFC `CWinApp::OnIdle`.

The decisive step was analyst reasoning. The old span is the *tail* of the
application vtable `0x42E1D8`, and slot 26 is `OnIdle` in the MFC 4.2 layout.

Result:
- `0x79A0` is `CHerculesApp::OnIdle`, called from `CWinThread::Run`'s `PeekMessage` idle loop at `0x425A56`.
- `InitInstance` (`0x7670`) ends with the `0x5E10` DirectDraw/engine-load call.
- The tick runs under an `__except` that turns any fault into `"Exception %08lX at Address %08lX"` and `exit(-123)`.

Independently verified from bytes, without REA, by
[audit_idle_owner.py](../../renderer_contract/tests/audit_idle_owner.py)
(15/15 checks, 0.33 s). The receipt is
[idle_owner.json](../../renderer_contract/evidence/idle_owner.json), and
`callback_metadata.md` is updated.

### 3. Cross-check on ENG1 (independent engine)

| Check | Result |
| --- | --- |
| Copied-table slots referenced | 122 |
| References per slot | exactly one each (data words 0, 1, 4, 5 excepted) |
| Documented sites for 80/81/89/129/131/143 | all match |
| Slot 92 | only its adapter site `0x2C645`, so no computed alias was found by a second engine either; corroboration, no new information |
| Our recovered ENG1 starts that Ghidra recognizes | 74 / 120 |

### 4. Reliability problems observed

- Pseudocode signatures are unreliable for this binary. `0x79A0` is shown as `void FUN_004079a0(void)`, dropping `this`, `lCount` and the BOOL result. AC60 is labeled `__stdcall`. Use instructions, not signatures.
- If a numeric selector is not a procedure, REA silently retries it as a *file offset*. The resulting error ("File offset 0x4075d0 has no loaded source mapping") hides the real cause.
- `get_call_graph` takes `forward`/`backward`, not `callees`/`callers`. Schemas are large (207 tools), so read them before calling.

## Cost and usability

| Aspect | Measurement |
| --- | --- |
| Session start (import + auto-analysis) | 28–36 s for the EXE; 44–46 s for ENG1 |
| CLI | re-imports on every command (about 30 s per call) |
| MCP session | 1–600 ms per query after the import |
| Envelope overhead | every response carries about 3.5–4 KB of profile/limitations; a one-address `xrefs` answer is 3,981 bytes |
| `analyze_function(AC60)` | **1.57 MB** (value-flow 633 KB, outgoing refs 173 KB); `procedure_pseudo_code` is 8.5 KB |
| `list_procedures` | 332 KB |
| Our equivalent byte audit | 0.33 s, about 5 KB receipt |

Raw MCP output is unusable as direct agent context. It needs a filtering
client (as here) or `inspect_analysis_view` paging.

## Assessment

| Criterion | Verdict |
| --- | --- |
| New findings | One real one (the 0x79A0 owner). REA made a wide xref sweep cheap; our own tools could have found it with a wider operand scan. |
| Correctness | Calls, xrefs and bodies were correct everywhere checked. Signatures and calling conventions were wrong or missing. |
| Blocker solved | Partly. It resolves the frame-scheduling owner (bootstrap open question 1). Packet, texture and presentation blockers are untouched. |
| Context efficiency | Poor raw; acceptable through a filtering client |
| Setup | Low: one npm install, existing Ghidra |
| Reuse across projects | Good: generic PE/ELF/DOS support plus seeds |
| Versus existing tools | `scripts/archaeology.py` + Capstone audits stay authoritative and faster for exact byte facts. REA adds a persistent reference database, a decompiler perspective and value-flow traces. |

**Recommendation:** keep REA as an *optional, pinned analysis provider* for
exploration. Do not make it a dependency of any audit or acceptance gate.
1. Pin the version and keep `--ignore-scripts`; never run `rea setup` on the shared host.
2. Always seed from `make_seeds.py`. Extend it to all four modules' table wrappers and `recovery.json` entries.
3. Use it for wide "who references X" sweeps and second-opinion pseudocode on large functions before strict matching.
4. Re-derive every claim with a byte-level audit before it enters `renderer_contract/` or `docs/`, as done here.
