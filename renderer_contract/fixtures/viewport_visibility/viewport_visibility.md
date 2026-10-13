# ENG3 viewport and visibility boundary

This static audit pins one original PC ENG3 parent/child route and its callback bindings. It does not execute the original images or claim rendered output. The rerunnable checker is `renderer_contract/tests/audit_viewport_visibility.py`; its compact receipt is `renderer_contract/evidence/viewport_visibility.json`.

`ENG3.DLL+0x22E40` calls the parent predicate at `0x22E4B`. The predicate at `0x24A80` reads the signed high WORD of the object DWORD at `arg+0`, arithmetic-shifts it right seven, and reads a WORD index at `arg+0x38`. The index is zero-extended to 32 bits; the decoded LEA sequence computes `base + 12*index` using 32-bit address arithmetic. The base pointer is loaded from ENG3 VA `0x1004078C`; its runtime value is unknown. The predicate makes signed 32-bit comparisons against record DWORDs at offsets `+4` and `+8`, accepting `lower <= coordinate < upper`. The record values, units, contents, and initialization are unresolved; this does not establish a bounds structure or its purpose.

An in-range parent reaches the fully pinned helper `0x24BB0`, whose complete body is `[0x24BB0,0x24F4B)` (0x39B bytes, 258 decoded instructions, final RET at `0x24F4A`). Its branch-specific callback sites are:

- `0x24CB4` and `0x24E36` call wrapper `0x1FF40`, which indexes slot 164; `0x24CC6` and `0x24E48` call `0x1FF60`, which indexes slot 165.
- `0x24CD3` and `0x24E55` call wrapper `0x1FD20`, slot 108; `0x24CE0` and `0x24E62` call `0x1FD30`, slot 109.
- The common tail calls `0x1FCA0` at `0x24F33` (slot 101) and thunk `0x1FD00` at `0x24F3B` (slot 106).

The slot numbering follows the absolute copied-table cell operands and base VA `0x100407A0`: `0x10040A34` is slot 165. The source HERC callback table maps slot 165 to `0xBC30`; slot 166 maps to `0xA340` but does not appear at the pinned `0x24BB0` call sites. Slots 108 and 109 map to `0xB880` and `0xB8F0`; 101 and 106 map to `0xB6F0` and `0xB920`. All these targets and the full 1000-byte source table are hash-pinned by the audit. The ENG3 wrapper operands establish slot indices; they do not establish runtime table initialization or call-time global values.

After `0x24BB0`, `0x24A80` calls slot 114 to write projected WORD X/Y at object offsets `+0x0C/+0x0E`, then slot 121 to write a depth WORD at `+0x10`. It sign-extends that WORD, subtracts `0x20` in 32-bit arithmetic, and accepts when the unsigned result is at most `0x620`, equivalent for a signed WORD input to depth `0x20..0x640` inclusive. On rejection, `0x22E40` sets bit `0x08` at object byte `+0x1F` and returns before traversing children. The projected X/Y writes are not compared to a screen rectangle in this predicate.

For a child, `0x24770` prepares the high WORDs of three object DWORDs, uses slots 101 and 106 for the transform/projection route, then slots 114 and 121 for output. It accepts only when the resulting signed depth WORD at child offset `+4` is strictly greater than `0x40`. `0x22E40` skips the later `0xA410`/`0x8DA0` renderer calls for a false result. The shared renderer span contains a direct call to FT4 producer `0x231A0`. These predicates therefore reject some content before packet emission; scaling packets after emission cannot restore those rejected objects. No projected screen-X/Y rectangle test is established by these pinned parent/child gates.

The same narrow selected spans contain no direct calls to ENG3 setter wrappers `0x1FFF0`/`0x20010` for slots 174/175, nor indirect calls through their copied-table cells. This is a local absence result for the six pinned ENG3 functions, not a whole-module scan. HERC `0xB970` stores two DWORD center arguments at `A8E74C/A8E750`; `0xB990` stores one DWORD focal argument at `A8E754`. The selected path's actual center/focal producer and runtime values remain unknown.

The known native presentation size is 320x240. 160x120 is only its half-size; this trace does not establish that either value is passed as the runtime projection center. The Phase 14 projection fixture's center `[320,240]` and focal `256` are synthetic inputs. The indexed bounds' semantic relationship to a camera frustum, level/sector limits, or another domain remains unresolved, so this evidence does not prescribe which bounds a wider view should change.

Prediction and falsifier are recorded in `fixtures/viewport_visibility/prediction_falsifier.json`. The static trace supports the prediction that pre-emission gates can discard content before packet scaling, but the bound table's meaning and projection setup remain open. To close those unknowns, first identify byte-backed writers of ENG3 VA `0x1004078C` and actual callers/arguments for slots 174/175. A later native discriminator would require fully evidenced runtime table state and original callback bytes; any unknown initializer or callable dependency must fail the fixture rather than become a success stub.

## Rerun

From the repository root, use the supplied hash-matching PC originals and a new output path under `work/`:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' renderer_contract/tests/audit_viewport_visibility.py `
  --repo-root . `
  --exe "D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE" `
  --eng3 "D:\Prog\hercules_recon\assets\pc\HERCULES\ENG3.DLL" `
  --output "work\viewport_visibility\viewport_visibility.json"
```

The output must be a fresh `.json` file whose lexical and resolved paths remain under `<repo-root>/work`; the checker fails on hash, byte-coverage, callback-site, slot-map, threshold, or output-guard mismatch.
