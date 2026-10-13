# AC60 callback/live-packet capture

## Prediction and falsifier

Prediction: unchanged original AC60 executes command `0x01` through original A8F0 with a zero-height rectangle. A8F0 calls original BD80 before returning to AC60. If BD80's primary callback pointer and gate are enabled, the synchronous callback boundary can mutate the current command's link or a future E1 record; after return, AC60 reloads the link at RVA `0xACA2` and consumes the then-current packet state. The later AC60 tail reaches BD80 again and reads the gate again.

Falsifiers were: zero-height A8F0 skips BD80 or writes VRAM; the enabled endpoint is not reached with return PCs `0x40ABE9`/`0x40B2E7`; AC60 ignores the changed live link or future E1 field; or a gate cleared during the first service remains enabled at the later BD80 gate read.

## Result

All four finite fixtures passed. The baseline with the gate off consumed the predispatch chain unchanged. In the link-mutation case, the synthetic callback replaced `cmd01.next` at `0x10001020` from E1 future (`0x10001040`) to E1 alternate (`0x10001080`). On return, original AC60 reached `0x40ACA2`, read `0x10001080` from the current command, and the first consumed-stream difference was index 2: `e1_future` expected, `e1_alt` consumed. The native boundary is RVA `0xACB9`.

In the future-field case, the callback changed the DWORD at future E1 `+4` from `0xE10001BB` to `0xE1000042`, preserving the E1 tag byte. The packet order stayed the same; the earliest stream difference was index 2 in that DWORD. Original AC60 read `0xE1000042` at RVA `0xB19C`, then its unchanged stores at RVAs `0xB1B0`, `0xB1BC`, and `0xB1CA` wrote the corresponding state fields.

For both mutation cases, the callback endpoint was reached first with `ESP=0x2000FEBC`, stack top/return PC `0x40ABE9` (the A8F0 call continuation), and again with `ESP=0x2000FEEC`, stack top/return PC `0x40B2E7` (the AC60 tail continuation). These are exact observed stacks; the fixture does not infer an installed callback's signature or behavior. In the gate-disable case, the first service cleared `0xA8E788`; original BD80 loaded `1` at RVA `0xBD8F` for the first call and `0` for the final call, so the endpoint ran once. With the gate initially off, original BD80 was entered twice and loaded zero both times.

The command rectangle was signed WORDs `{5, 224, 8, 0}`, destination `{15, 240}`. Original A8F0 reached its call at RVA `0xABE4` on the zero-height branch; the write hook observed no A8F0 data/VRAM writes. `BA01FC=1` suppressed unrelated setup/presentation/device tails while preserving E1 handling, command-01, and AC60's final BD80.

## Provenance and limits

- Original `HERCULES.EXE`: SHA-256 `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`; fixed image base `0x400000`, no base relocations.
- Executed unchanged spans: AC60 RVA `0xAC60`, 1,680 bytes, SHA-256 `f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af`; A8F0 RVA `0xA8F0`, 771 bytes, SHA-256 `bc04260d2227d221b124bb397b3d7f1138eed53081654722e3f1acd52599196e`; BD80 RVA `0xBD80`, 22 bytes, SHA-256 `6a9578c5be0c980bc35b29642d3ac389e829a0c5666022353cb88c04f19db64f`.
- Unicorn `2.1.4`; native DLL SHA-256 `010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1`. Runtime provenance retains the pinned wheel URL and expected/downloaded wheel hash in `live_capture.json`.
- Private runner: `run_live_capture.py`, SHA-256 `40fdfcd734defcddde60dce2339d34f1b30d040fe7ff034e629741c9aad073fb`. It imports the existing public `original_ac60_mixed_runner.py` fixture API unchanged (SHA-256 `faf988fddd771bbfac0a1b17b28ccc83012896d75c84d99f22c4c3f3ead3c050`) and public `oracle_common.py`.
- Execution was bounded to 100,000 original instructions per case. Original code spans stayed unchanged; unknown PCs, invalid memory, unexpected writes, and writes to fixture packets fail. Only the three original E1 state DWORD cells and the explicit fixture globals changed in `.data`; mapped `.data` was restored byte-for-byte after each case. The working files and JSON receipt are under ignored `work/renderer_phase11/live_capture/`.

This is `BOUNDED_EQUIVALENT` evidence for four small synthetic chains and the synchronous live-mutation boundary only. The predispatch snapshot is passive observation, not a mutation or replacement for the native walk. A synthetic host endpoint applies selected fixture mutations and returns by RET semantics; no original callback target is identified. No pixels, positive-area A8F0 copy, actual GPU/hardware callback visibility, DirectDraw path, gameplay, source identity, or `FUNCTION_MATCH` is claimed. A future renderer must account for the observed live link/state read after notification if it supports equivalent mutable callback behavior; this fixture does not establish that shipped callbacks perform these mutations.

Reproduce from the repository root:

```powershell
& $pythonExe -B renderer_contract\tests\test_original_live_capture.py `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --original-root D:\Prog\hercules_recon\work\renderer_interface_repo `
  --deps C:\tools\hercules\renderer-oracle-python `
  --analysis-deps C:\tools\hercules\python `
  --receipt work\replay\live_capture.json
```



The public [receipt](evidence/native_live_capture.json) binds the copied runner and its output guards. The comparison is between explicit fixture trace expectations and original execution; no C implementation equivalence is claimed.
