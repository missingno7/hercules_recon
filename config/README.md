# Active build and analysis configuration

| Path | Purpose |
| --- | --- |
| `eng1/sources.json` | Genuine ENG1 source selection, reviewed adaptations/omissions and aliases |
| `eng1/incomplete.json` | Explicit unresolved/provisional dependency inventory; generated bodies stay out of genuine builds |
| `title/sources.json` | TITLE source additions and source-graph settings |
| `title/link_plan.json` | Reviewed historical natural-link plan used by `scripts/linked.py` |
| `title/data_units.json` | TITLE data-owner selection and ordering evidence |
| `research/` | Current path versions of bounded compiler experiment recipes |

`recovery.json` at the repository root remains the sole function-acceptance
authority. These files select inputs or describe missing work; they do not grant
matches or prove gameplay completeness. Modules without a reviewed full link plan
must not be advertised as source-complete.

Earlier `evidence/*_behavioral_build.json` and related experiment receipts remain
historical provenance. Current runners read this directory. See
[the source guide](../src/README.md) and [the migration review](../docs/source_layout.md).
