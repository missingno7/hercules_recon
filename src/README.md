# Reconstructed game source

This is the current game reconstruction, organized by original module. Matching
and incomplete functions live together in their real compiler units. A folder
name does not confer a proof status; `recovery.json` records that per function.

| Directory | Ownership |
| --- | --- |
| `pc/` | HERCULES.EXE: Windows host, application, input, audio, movie and archive code |
| `title/` | TITLE.DLL: title/menu engine and its data |
| `eng1/` | ENG1.DLL: gameplay engine, actors, world state and its data |
| `eng3/` | ENG3.DLL: secondary engine and its data |
| `shared/` | Reused engine logic, common interfaces, layouts and data owners |

The former `src/eng1/behavioral/` and game source under `calibration/` have been
consolidated here. Those names described earlier work streams, not a separate
implementation or a lower matching tier. Accepted strict matches were present
in both. There is one current source path for each retained unit.

Use the derived lookup instead of inferring status from a filename:

```text
python scripts/source_status.py
python scripts/source_status.py actor_pair_animation.c
python scripts/source_status.py eng1.native_code_target_001a280
python scripts/source_status.py calibration/eng1/actor_unit_282e0.cpp
```

The last form translates a historical path for inspection. It does not make an
old path a build alias. `FUNCTION_MATCH` is the legacy name for RAW acceptance;
`FUNCTION_MATCH_RELOCATED` is exact code with independently verified references;
`LINKED_MATCH` retains the documented linked proof. `CODEGEN_SIMILAR` is diagnostic
and `RECONSTRUCTED` grants no matching acceptance. Headers/data owners may have no
function rows. A unit can contain several statuses.

Active build selection is under [`config/`](../config/README.md). It may apply
reviewed omissions or aliases to retained compiler units; do not compile every
file by glob. Preserve unit boundaries, declaration ordering and compiler flags
until original-backed experiments establish a safe change. Address-based names
remain where a better semantic name has not been established.

Superseded probes are under `prototypes/reconstruction/`; other host/renderer
experiments remain in `prototypes/` and `dropin_renderer/`. Generated candidates
and diagnostic bodies belong in ignored `work/` or `candidates/`.

See [layout migration and proof preservation](../docs/source_layout.md) and
[matching definitions](../docs/matching.md). None of this establishes a complete
gameplay implementation or a runnable ENG1 build.
