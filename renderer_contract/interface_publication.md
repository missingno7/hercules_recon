# Early interface-table clone coherence

## Result

The reviewed static evidence found no source-prefix or source-pointer mutation in the existing direct-address scans or this bounded alias/dataflow audit. The source prefix is a file-backed `.data` initializer. Added checks found no code immediate in the prefix/pointer-cell address interval, no register-based memory operand with a displacement numerically in that interval, and no other file-backed DWORD alias; the sole non-text value pointing into the prefix is the pointer cell itself at RVA `0x365A0`. This supports word-content coherence within the checked scope, not process-wide immutability.

Word 1 is a pointer to shared mutable host state. Its initial value `0x00615C98` targets the EXE's zero-fill `.data` tail. The host callback at RVA `0x1BD0` reads the source cell at `0x4361BC` (`0x1C03`), then writes the caller's low word to `[context+0x138]` at `0x1C0E`. A table clone copies the pointer, not the pointed-to state, so preserving word 1 keeps the same context alias; replacing it would change the shared-state target. The callback's observed context read is from the original source cell, not an argument-derived clone.

This is a bounded static coherence result, not a process-wide immutability guarantee. The `.data` section is writable, and computed aliases, external writers, and process scheduling are outside the scan. The unresolved `0x79A0` event path cautions against treating DirectDrawCreate return as the only pre-copy boundary for a strategy that defers publication until then. That ordering question neither proves nor invalidates a hypothetical `DLL_PROCESS_ATTACH` publication; attach/load timing is outside this audit.

## Original values and copy direction

The source prefix is RVA `0x361B8`, VA `0x4361B8`, 1,000 bytes, SHA-256 `52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8`. Its adjacent pointer cell at RVA `0x365A0` is file-backed and initially contains `0x004361B8` (four-byte SHA-256 `808fbb8b89f159778e8ec1322e9c7a53c570dfe17e6e565367f97a9bee2a0e3b`). The compact receipt records the full-table hash and only selected initial values needed to interpret data and context ownership; it omits the other initial DWORDs.

Selected values show the distinct data and callback roles: word 0 is signature `0x5AC00CAC`; word 1 is context pointer `0x00615C98`; word 2 is pointer `0x006177D0` into zero-fill `.data`; word 3 is `0x00300000` outside the image; word 5 is pointer `0x00437480` into file-backed `.data`; and slot 19 is host text RVA `0x1BD0`. The host's fixed-address reads of slots 1, 2, 3, and 5 are retained as raw data contracts; the values of slots 2, 3, and 5 are not semantically reinterpreted here.

Each original module's pinned `PC_DLLEngineMain` entry shows `mov esi, edx`, loads its own local table root into EDI, then executes `rep movsd dword ptr es:[edi], dword ptr [esi]` with ECX `0xFA`; signature validation follows the copy. The destinations are ENG1 RVA `0x70F20`, ENG3 RVA `0x407A0`, and TITLE RVA `0x2BB40`. Their complete entry-span hashes and copy windows are included in the receipt. This is the direction source-to-module-local storage; the bounded entry spans contain no memory write through ESI.

The host dispatcher separately loads `[0x4365A0]` into EDX at RVA `0x5EF3`, pushes that pointer, then calls the resolved engine export at `0x5F03`. No memory destination or repeated-copy destination uses EDX in that bounded sequence. Existing bootstrap evidence shows DirectDraw setup before the dispatcher on the ordinary direct path and a separate `0x79A0` route whose invocation order remains open. This is a publication-timing caveat only; it does not answer whether a `DLL_PROCESS_ATTACH` hook runs before the host reads this cell.

## Static scan boundary

The runner verifies the four public-manifest original hashes, source-table and pointer-cell hashes, and relevant public evidence inputs. It hash-pins its bounded dispatcher and context-writer spans against [upload_domain_bounds.json](evidence/upload_domain_bounds.json). It scans the complete file-backed EXE `.text` stream for immediate address aliases into the table/pointer interval and register-based memory displacements numerically in that interval; it scans non-text file-backed DWORD values for aliases into the same interval. It also rechecks the bounded dispatcher dataflow, each module's exact `REP MOVSD` copy direction/local destination, and the word-1 context writer. The previous direct-absolute write counts remain sourced from [compatibility.json](evidence/compatibility.json), [loader.json](evidence/loader.json), and [upload_domain_bounds.json](evidence/upload_domain_bounds.json), avoiding a second report of the same direct scan.

The checked original images match the public manifest: HERCULES.EXE `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`; ENG1.DLL `01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757`; ENG3.DLL `aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7`; TITLE.DLL `b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b`.

Prediction: no host write or bounded alias route will change a source word after an early clone. Falsifier: a derived store or repeated-copy destination reaches any prefix byte or the pointer cell, or a later host writer changes the source pointer/value before another cold module copy. Current result: no such route in the bounded static scan; the word-1 pointee is mutable, while its pointer value remains unchanged in the observed source initializer and writer path.

## Reproduction

Run from the renderer checkout root with the repository's existing analysis dependencies:

```powershell
$env:HERCULES_PYTHON_DEPS='C:\tools\hercules\python'
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' renderer_contract\tests\audit_interface_publication.py --repo-root . --output work\replay\interface_publication.json
```

Without explicit image options, the runner searches the checkout and its ancestors for the pinned `work/discs/pc_install` and `assets/pc/HERCULES` originals. The `--exe`, `--eng1`, `--eng3`, and `--title` options can override those paths. The runner writes only under `work/` and does not launch the host, proxy, or game. It uses the public PE reader and Capstone already configured in the repository environment.

The public receipt is [interface_publication.json](evidence/interface_publication.json). The conditional ordinary-startup timing inference is separate in [startup_import.md](startup_import.md).
