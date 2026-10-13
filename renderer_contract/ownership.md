# Public renderer state ownership audit

This package reruns the static candidate inventory at the AC60 submission seam. It pins the four original images, the public closure dependency list, the checked-in `recovery.json` function inventory, and public renderer evidence. It does not read `work/renderer_baseline` or ignored worker receipts. `tests/audit_state_ownership.py` writes a single-line JSON report to the requested `--output` path and emits a compact JSON success record on stdout.

The prediction was that AC60 packet scratch and page state would have direct absolute references only inside the reviewed bounded closure, while host lock/surface/frame state would also have host-side references; the engine DLLs would use their copied callback table instead of absolute references to those cells. A confirmed function-rooted reference outside the closure, or a confirmed direct DLL reference, would falsify that prediction. Section-wide linear-sweep operands remain candidates because code/data alignment is not established by linear decoding. The audit does not resolve pointer aliases, so a downstream user of an AC60-derived pointer would still falsify a scratch-private interpretation.

## Run

Run from the checkout root:

```powershell
& 'C:\msys64\mingw64\bin\python.exe' renderer_contract\tests\audit_state_ownership.py `
  --original-root 'D:\Prog\hercules_recon\work\renderer_interface_repo' `
  --output 'work\renderer_phase4\state_ownership\report.json'
```

The tested command locates the original EXE and DLLs by searching the checkout and its ancestors for `work/discs/pc_install/HERCULES.EXE` and `assets/pc/HERCULES/{ENG1,ENG3,TITLE}.DLL`. On a standalone checkout, supply `--exe`, `--eng1`, `--eng3`, and `--title` explicitly. The output folder is created if needed.

For this private package validation, the same source was run once with the `work/renderer_phase4/state_ownership/package/audit.py` path and the package-local `report.json` output. The script rejects an `--output` path outside the resolved checkout `work` directory.

The script requires Capstone 5.0.7 and imports `EXPECTED_EXE_SHA256`/`sha256` from the public `renderer_contract/tests/oracle_common.py`; `scripts/pe.py` reads the images without loading or executing them. It verifies the original image hashes, public evidence hashes, `recovery.json` hash, and each bounded closure span hash.

For each file-backed executable section, the script records how far ordinary Capstone decoding proceeds, then performs a full linear sweep with skipdata enabled. Every undecodable byte becomes a `.byte` pseudo-op; pseudo-op operands are skipped, and the exact contiguous gap RVAs, decoded/skipped byte counts, and end offset are recorded. It also restarts bounded sweeps at every matching `recovery.json` function start and at each public renderer-closure span start. These function-rooted spans are alignment anchors; the inventory is partial, so unanchored section bytes are still reported separately. Direct absolute memory references and address-immediate candidates are kept distinct, and outside-closure sites retain their instruction, scan source, and access. Any instruction-like decoding of embedded data remains a candidate, not proof of actual control flow.

## Current finding

The previous first-stop sweep decoded only 21,486 EXE, 10,964 ENG1, 12,680 ENG3, and 15,763 TITLE instructions before stopping at the first invalid byte in each `.text` section. The corrected full sweeps end at each section boundary and account for all file-backed bytes:

| Image | `.text` bytes | First strict stop | Valid instructions | Skipped bytes / gaps |
| --- | ---: | --- | ---: | --- |
| HERCULES.EXE | 183,808 | `0x1124A` | 65,375 | 4 bytes / 4 gaps |
| ENG1.DLL | 307,200 | `0x97F8` | 100,212 | 23 bytes / 12 gaps |
| ENG3.DLL | 200,192 | `0xB2E1` | 63,421 | 6 bytes / 6 gaps |
| TITLE.DLL | 130,560 | `0xD714` | 40,168 | 16 bytes / 16 gaps |

The skipdata totals are Capstone `.byte` pseudo-ops; for example, the EXE sweep emits 65,375 valid instructions plus 4 skipped-byte pseudo-ops. The report records each exact section gap and the exclusive end RVA. It also uses the pinned recovery inventory (73 EXE, 116 ENG1, 61 ENG3, 340 TITLE spans) and 19 hash-verified renderer-closure spans as alignment anchors. The recovery inventory covers only part of the executable bytes; unanchored ranges are explicit in the report.

For the named globals, the corrected section-wide candidate counts did not add or remove references compared with the earlier truncated run. The hash-verified closure-rooted sweep independently found the same internal candidate counts: 32 at `A90944`, 8 at `A90948`, 296 in `A909B0..A90A13`, 5/7/5 at `A8E6F4/F8/FC`, and 3/15 at the F3 outputs. Outside-closure candidates remain 3 at `BA01FC`, 1 at `BA01F0`, 2 each at `BA0210/214`, and 3 at `43A590`. The recovery-rooted sweep confirms a subset of the surface and dirty-flag sites; source classifications and outside sites are in [state_ownership.json](evidence/state_ownership.json). The compact receipt omits repeated per-target details and unanchored-range lists; rerunning the script generates those full details under work/. ENG1/ENG3/TITLE produced no candidate direct references to these targets in either sweep mode.

These are candidate-inventory counts, not whole-control-flow proof. Host display/surface cells have independently supported host-service roles. AC60 workspace and page cells have no outside candidate in this inventory, but remain private-state candidates until computed aliases and callback-derived consumers are excluded. Keep observed staged writes reproducible across the seam while that alias question remains open.

## Callback slot 80 and BD80

The public callback-table evidence binds slot `+0x140` to AC00 and records AC00's direct call to BD80. The public closure records AC60's own direct call to BD80 at RVA `0xB2E2`. The package also pins the original AC00 wrapper bytes `[0xAC00,0xAC08)` and the BD80 gate body `[0xBD80,0xBD96)`, and verifies the AC60 call-site bytes.

AC00 executes `call BD80; xor eax,eax; ret`. It prepares no explicit arguments and does not read or adjust caller stack arguments; the caller's argument shape and the callback's stack interpretation remain unresolved. AC00 adds a return frame: if BD80 dispatches its callback, the callback returns to AC00 at `0xAC05`, after which AC00 clears EAX and returns. AC60 calls BD80 directly, so the callback continuation is `0xB2E7`. BD80 checks callback target `[0xA8E780]` and gate `[0xA8E788]`, then tail-jumps through the target only when both checks pass. AC00 and BD80 therefore share the callback gate but are not ABI-identical substitutes.

The pinned original is mapped at `0x400000` and the public native-dispatch receipt records no base relocations, so an adapter can statically resolve the original gate at `image_base + 0xBD80` (`0x40BD80`) without depending on a DLL's copied callback table. Installing or invoking that address from a host adapter remains unobserved.

## Public evidence and inventory pins

The report verifies and records these checked-in inputs before scanning:

| Evidence | SHA-256 |
| --- | --- |
| `renderer_contract/evidence/closure.json` | `2142cb1d0c7a49c9fe9ebd3bd186099583431755205efc3022d9a29a7cab9e30` |
| `renderer_contract/evidence/compatibility.json` | `82b8b183ad54d629fef5bd977ca8ac579c3708af36879b18c8536303a983c61d` |
| `renderer_contract/evidence/frame.json` | `b81f7e1f86bb8b3976ed666261d741000b2e7e400d35d18be6ce0068ce69bb5b` |
| `renderer_contract/evidence/presentation.json` | `c98085c9f39b8c532817d90f5617067517a1157a8ede56042d778fde6c2d829a` |
| `renderer_contract/evidence/textures.json` | `8743c94b086765d4d4dff50633b31d953506b2dc0184de10b8a357bbd399767a` |
| `renderer_contract/evidence/native_dispatch.json` | `966617e243816275b70e7080396a75c69a09fd1afc26c01ed001a464c10b9151` |
| `recovery.json` function inventory | `d6858a378f8360ed713b6786e7dc14c09d728d12afa8b759afd0544c01efd597` |
