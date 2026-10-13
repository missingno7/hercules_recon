# ENG1 archived-resource geometry fixture

This fixture executes a bounded original x86 path through the archived FOREST
`MAP_PSX.BIN`, then compiles the measured point/face emitter path and the
shared integer projection reference with the pinned VC5 toolchain. It is a
controlled finite fixture: the archived mesh, face, UV, material, and lookup
bytes are original; the selected row/camera context and bounded packet/OT
backing are controlled inputs. It is not a live game or actor-identity proof.

Run from a clean checkout with the explicit bundled Python runtime, matching
read-only original files, and a fresh directory below `<repo>/work`:

```powershell
$repo = (Resolve-Path .).Path
$python = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$exe = 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE'
$eng1 = 'D:\Prog\hercules_recon\work\discs\pc_install\ENG1.DLL'
$archive = 'D:\Prog\hercules_recon\assets\pc\HERCULES\HERCULES.FS'
$out = Join-Path $repo 'work\eng1-real-resource-replay'
& $python -B renderer_contract\fixtures\eng1_real_mesh\run_native_real_map.py --repo-root $repo --exe $exe --eng1 $eng1 --archive $archive --output $out
if ($LASTEXITCODE -ne 0) { throw 'native trace failed' }
& $python -B renderer_contract\fixtures\eng1_real_mesh\make_real_manifest.py --repo-root $repo --output $out
if ($LASTEXITCODE -ne 0) { throw 'manifest generation failed' }
& $python -B renderer_contract\fixtures\eng1_real_mesh\run_source_compare.py --repo-root $repo --exe $exe --eng1 $eng1 --archive $archive --output $out
if ($LASTEXITCODE -ne 0) { throw 'VC5 source comparison failed' }
& $python -B renderer_contract\fixtures\eng1_real_mesh\test_path_guards.py --repo-root $repo --existing-output $out
```

The first command creates a fresh output directory with a local ownership
marker. Later commands require that marker and reject existing compiler-output
subdirectories. All extracted map bytes, the original 1,000-byte host transport,
the generated case header, native reports, object/executable, and logs stay
inside the output directory. The scripts reject output paths outside the
repository's `work` tree, including `assets`, and reject reparse-point paths.

The accepted comparison covers three observed FT4 resource calls and 481 C
assertions. It independently derives point inputs from packed map bytes, runs
`hercules_projection_eval` on each captured B6F0 point/config, compares camera
and screen results with the native callbacks, then emits and checks faces,
packet bytes/stride, and OT insertion. `2E3B0` preparation and live camera
setup remain native observations supplied as a data seam; C does not reconstruct
them. No `FUNCTION_MATCH`, general map coverage, frame-capacity, live actor, or
whole-game initialization claim is made.
