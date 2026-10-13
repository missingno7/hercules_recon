# ENG3 bounded interval source seam

This candidate contains a readable C model of the ENG3 indexed-interval seam, a C assertion harness, and a standalone source/native replay. It does not model the complete `0x204C0` producer, `0x24A80` handler, live level bounds, or a successful transform. The prediction and falsifier were recorded before this replay in `prediction_falsifier.md` (SHA-256 `14fff49fc73936c17a4e80860f2d01c82a862251875fc83352656f2b698454cd`).

`eng3_visibility_seam.c` represents three measured operations: a 12-byte indexed row with endpoint DWORDs at `+4/+8`; an unsigned 32-bit `lower + extent` endpoint write that wraps modulo `2^32`; and the `0x24A80` coordinate/predicate calculation. The object X input is read as a signed high WORD of the DWORD at `+0`, then arithmetic-shifted by seven. Its WORD index is zero-extended and addresses `base + 12*index`; signed endpoint DWORDs accept `lower <= coordinate < upper`. The C endpoint helper intentionally does not write selector `+0`.

The C test harness passes with both GCC and pinned MSVC 5.0 RTM. It checks the normal row bytes, all four encoded coordinates through decode plus interval decision, the signed `0xff80` high-WORD case, and endpoint wrap/sign interpretation. In the original code, ENG3 `0x204C0` completed its controlled zero-count path in 318 instructions and emitted `000000000a00000014000000` for lower 10 / extent 10. A second bounded writer call emitted `00000000f8ffff7f08000080` for lower `0x7ffffff8` / extent `0x10`, confirming the endpoint add wraps before the `+8` store. The +0 selector was controlled as zero for this byte-for-byte normal comparison; selector production remains outside the C helper.

With that original row installed, original `0x24A80` returned `EAX=0xffffffff` before `0x24BB0` for coordinates 9 and 20. Coordinates 10 and 19 entered `0x24BB0`, then stopped at the first unresolved data access: a WORD read at ENG3 `0x10024C49` through global `0x100407B4`, whose controlled value is null. The supplementary negative coordinate `-1` also rejected before `0x24BB0`. No unknown callee or synthetic scene/trig data is accepted as a successful result.

This verifies the bounded source seam against the original writer bytes and pre-transform branch decisions. It does not establish live bounds contents or units, runtime level-table initialization, success of the positive transform tail, or full Gate 4 visibility behavior. This candidate does not call `0x23720`, `0x8DA0`, or a packet producer; it proves no screen-coordinate mutation, clipped vertices, or accepted-to-emission completion.

## Reproduction

From the repository root, provide the pinned originals and a fresh output beneath `work/`:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' `
  renderer_contract/fixtures/eng3_visibility_source/run_eng3_visibility_source.py `
  --repo-root . `
  --exe 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE' `
  --eng3 'D:\Prog\hercules_recon\work\discs\pc_install\ENG3.DLL' `
  --output 'work\eng3_visibility_source\receipt.json'
```

The measured compiler identities were GCC 12.2.0 (binary SHA-256 `c5005ba5380351c10743e9ca53ef9632164c54be3bd6b93324795114e4589a7d`) and pinned MSVC 5.0 RTM `cl.exe` (SHA-256 `bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb`; installed bundle tree SHA-256 `29fc5aea67aa10a8135d65f8f371f7456bfc10ec8ef36dc0050ca5406bf9d92e`). The runner checks full-file SHA-256 (`HERCULES.EXE` `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`; `ENG3.DLL` `aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7`) and the exact writer, predicate, and transform byte spans. It uses the pinned Python PE reader and Unicorn dependencies at `C:\tools\hercules\python` and `C:\tools\hercules\renderer-oracle-python`. Both C builds run in a temporary directory under `work/`, which is removed after execution. The VC5 compiler path, compiler hash, and manifest bundle identity are pinned in the runner; its include/library paths come from that same bundle. `--output` must be a fresh `.json` path both lexically and physically under this repository's `work/`, outside each input's parent directory. The receipt reports the two positive cases as unresolved, not as transform success.

The measured private receipt for this candidate is `work/renderer_phase19/eng3_visibility/runs/source_comparison_v3.json`.
