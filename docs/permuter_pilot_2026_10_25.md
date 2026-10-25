# Targeted cpp_permuter pilot

The pilot recovered two perturbed STRICT controls and closed two real instruction
residuals, but accepted **0 new functions / 0 bytes**. Stop the searches here;
the private candidates need reference proof, not more source variants. Do not
deploy a standing permutation campaign or another comparator. Canonical source,
flags, recovery state and originals stayed unchanged. Base: `ba9fabc`.

## Tool and adaptation

Inspected and built the unmodified
[cpp_permuter](https://github.com/CriminalRETeam/cpp_permuter/tree/59a4fea7d20aaae4d58786aa1a0425e9251199ef)
at `59a4fea7d20aaae4d58786aa1a0425e9251199ef` with existing VS 2022 MSVC
19.44.35225.0. Upstream targets VC6; the controls establish only the tested
Hercules VC5 profiles, not general compiler support.

- Source archive SHA-256: `686ed6f23df5025d1736d1248f9fd55c84992b1212a0dde900fd37f9d5335ae6`.
- Executable SHA-256: `632027ad7dec583031d123da6b8eb2ae5cb124f2793618fbd1ce892e79577b5e`.
- Executable: `C:\tools\cpp_permuter-59a4fea7d20aaae4d58786aa1a0425e9251199ef\build\Release\cpp_permuter.exe`.

Its `--score-cmd` bypasses target-object disassembly: no synthetic target object,
LLVM installation or executable link is needed. The disposable adapter
`work/permuter/bridge.py` calls existing `match.compile_source` and
`match.compare` / `masked.masked_compare` against the PE, checking frozen inputs,
complete contribution sizes, peer function bytes/relocations and relocation
symbol/type/addend multiplicities. Masked score zero is diagnostic; existing
fresh strict/reference/promotion gates remain the only acceptance path.

The tool preserves the unpreprocessed TU and mirrors its include context. These
pilots copied only the selected TU and required local headers, kept pinned SDK/MFC
inputs, original `.c`/`.cpp` selection and flags, and never patched objects.
Use explicit reviewed passes, exhaustive depth 1, one job and a hard compile cap.
The base compile is additional to `--max-candidates`. Defaults use random search
and all passes. `move_stmt` assumes memory accesses do not alias: exclude it.
Keep type/cast changes, global-definition switches, ABI changes, read caching,
dummy/deleted statements and helper rewrites disabled. Even declaration ordering
requires scalar/nonvolatile/constructor/overloaded-operator review.

Existing `residual.py`, `refinery_forms.py` and object-only objdiff already provide
localization, bounded families and instruction display. Unlike `refinery.py`, the
pilot did not build a semantic executable for every reviewed scalar/lifetime
alternative. Reviewed current
[reccmp](https://github.com/isledecomp/reccmp/tree/b673616608c9aecc6117f99f2fba7d7ed6510cca)
PE/parser/asm/function comparison code; its linked-PE/PDB workflow and normalized
instruction scoring add nothing needed for this object-to-PE pilot. No reccmp
dependency was installed.

## Results and cost

| Function | Extent | Instruction-byte differences | Target-lane CL calls | Result |
| --- | ---: | ---: | ---: | --- |
| `title.make_colour`, C++ RAW control | 64 B | 19 → 0 | 2 | `invert_if`, one generated candidate |
| `eng3.within_radius_1140`, C RAW control | 80 B | 48 → 0 | 3 | `reorder_saves`, two evaluated candidates |
| `exe.options_joystick_bind_83f0` | 352 B | 8 → 0 masked | 2 | Manual grouped edit; tool `--base-only`, not an automated search win |
| `exe.options_keyboard_bind_8dd0` | 608 B | 3 → 0 masked | 3 | One explicit two-arm `PERM_GENERAL` choice |

Lane counts include the tool base; genuine lanes also include a separate fresh
canonical baseline. Both controls then passed independent fresh RAW recompilation
and every TU peer contribution check. Both genuine candidates preserve all peer
contributions and typed relocation multiplicities, but are not STRICT matches.

Total **22 historical compiler calls**: 10 control preparation, 5 control search,
2 independent control verification and 5 genuine-target calls. Preparation
includes three transformations folded to unchanged code and one failed compile
from an omitted scratch header. A sandboxed launch failed before CL started and
was retried elevated; it is excluded from compiler executions. Token totals were
unavailable. New accepted functions and bytes per 100 focused calls are **zero**;
recovering existing controls is not new coverage. Four bounded Luna workers and
one integrator handled audit, controls, target diagnosis and keyboard evaluation;
no Sol escalation was needed. No source integration/promotion cost was incurred.

Keyboard: move only `char text[2]` immediately before `if (label == 0)`, retaining
the two stores and call order. This corrects stack-offset bytes 529, 533 and 539.
Canonical source hash:
`33615bb65358bf98d81d76d2bf00fd664da0189ef069f790821fc40262b983de`.
Joystick: initialize scalar `bit` before `count`, keep the same loop, and reverse
the operands of all six pointer-equality comparisons. This fixes two zeroing
bytes and six CMP encodings. Stock depth-1 swapping handles them individually;
reasoning supplied the coupled edit without a 64-way search. Canonical hash:
`34d70850ab83179d4adadcbf4e08ade757f947efac0691607c7d855294cfed82`.

## Stop and next evidence

Do not repeat these body searches for the recorded source/flags. Joystick needs
a reviewed map for 14 references. Existing MFC/import/CRT identities are reusable;
independent identity/payload/extent proof for `None\0`, ` %d\0` and the measured
state/empty-text views remains. Do not promote inferred addresses or invent full
storage owners from measured prefixes. Keyboard needs its 43-reference audit.
Neither private candidate was installed as a second source implementation.

Explicit small alternative sets work with VC5 and can close instruction residues
cheaply; this pilot does not prove higher accepted STRICT throughput. Keep the
tool optional and the adapter disposable until a case can complete acceptance.
Scratch receipts/configs/diffs/objects are indexed under `work/permuter/controls/`,
`targets/joystick/`, `keyboard/` and `reference_readiness/`. No routing system,
persisted search cache or matching authority was added.
