# Matching workflow and proof limits

`recovery.json` is the authoritative function state. It holds reviewed target
hashes, RVAs, extents, provisional names, source locations, compiler flags and
accepted proof hashes. Generated logs and objects are disposable. No whole
module is reconstructed yet.

## Initial measured pilot

| ID | Shape | Compared bytes | Initial result |
|---|---|---:|---|
| title.unlink_12c | Conditional list/struct pointer updates | 64 | FUNCTION_MATCH |
| title.unlink_0e4 | Flag guard, list update and clearing | 80 | FUNCTION_MATCH |
| title.make_colour | Clamp, shifts, integer packing and flags | 64 | FUNCTION_MATCH |
| title.sentinel_count | Sentinel loop and count arithmetic | 32 | FUNCTION_MATCH |
| title.loop_calls | 18 iterations, two calls per iteration | 48 | CODEGEN_SIMILAR |
| title.switch_calls | Switch, merged tails, eight calls | 80 | CODEGEN_SIMILAR |
| title.decimal_digits | Three signed divisions, sprite writes, flag branches | 224 | RECONSTRUCTED |

The exact group contains 209 instruction bytes plus 31 naturally emitted NOP
alignment bytes. Both the complete function body and padding are compared; no
bytes are stripped or normalized. Reviewed call references, complete control
flow and following aligned entries support these extents. The object must
contain one unambiguous `/Gy` COMDAT contribution per function, with no object
or target base relocations. Target bytes must lie inside executable PE code.

Accepted source currently shares `src/title/pilot.c` to preserve the measured
translation-unit context. This small calibration grouping is **not** a recovered
historical filename. Partial structures expose proven offsets and name unknown
regions explicitly. Their default packing fits these accessed fields, but the
pilot does not discriminate all original packing options. In `make_colour`, the
apparently redundant lower clamp remains because this ordinary clamp expression
reproduces the observed VC5 output; no volatile/assembly or byte insertion is used.

The two call-bearing cases resolve independently verified symbolic REL32 targets
only inside a diagnostic comparison buffer. That buffer is never written as
reconstructed code. They cannot pass the exact gate until a real reconstructed
link supplies the right addresses without forced placement. The larger decimal
routine currently emits 240 bytes instead of 224; source expression, allocation
and scheduling differences remain open. Its source is a calibration hypothesis,
not accepted reconstruction.

## Commands

```powershell
python scripts/archaeology.py function title.unlink_12c
python scripts/match.py candidate title.unlink_12c --source candidates/my/pilot.c
python scripts/match.py verify --report build/verification.json
python scripts/calibrate_control.py
python scripts/test_pilot.py
python -m unittest discover -s tests -p 'test_*.py'
```

`match.py candidate` always compiles fresh with a pinned historical compiler.
It reports actual byte differences, mismatch intervals and unresolved relocations.
`match.py object ID --obj build/.../candidate.obj` is an inspection command;
prebuilt objects never promote state. Full reports must stay under `work/` or
`build/`. `calibrate_control.py` compares two toolchains and eight flag sets;
its historical measurement is in `evidence/toolchain_calibration.json`.

Promotion requires reviewed source and reviewed target context in `recovery.json`:

```powershell
python scripts/promote.py title.unlink_12c --source candidates/my/pilot.c --verify-only
python scripts/promote.py title.unlink_12c --source candidates/my/pilot.c
python scripts/promote.py --recover
```

The gate accepts self-contained C units under `src/` in this initial version.
It freezes the candidate, verifies **all** established matches, checks every
emitted function against accepted/requested rows for that source, and rejects
unreviewed data contributions. It then updates source and state under an OS
lock with a rollback journal. Canonical edits during verification are rejected.
An interrupted transaction must be recovered before another promotion. The
journal refuses to overwrite subsequent unrelated edits. Source validity still
requires human/agent review: byte equality alone is not permission to insert
copied code, arbitrary assembly, or original-binary dispatch.

`FUNCTION_MATCH` proves fresh raw function contribution equality under the
recorded environment. It does not prove historical names, whole-image bytes,
library closure, linker layout, original source text or a uniquely identified
compiler revision. The next calibration work should resolve the larger arithmetic
case, identify library members, and reconstruct a naturally linked caller/callee
group before expanding the acceptance gate to relocatable code and C++ units.

Initial verification on 2026-10-07 passed 64 reconstructed-code semantic checks,
13 acceptance/recovery tests, all four fresh exact comparisons, complete hashes
of both compiler trees, and hashes of all 12 supplied original files. The
six-function differential matrix contains 96 measured function/configuration
comparisons; it records rejected configurations as well as successes.
