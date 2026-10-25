# Typed worklist compiler probe

A private typed reconstruction now reproduces all 144 bytes of ENG1 `27c40`
after relocation operands are masked, including the thirteen trailing NOPs.
Normal `270c0` retains two nonreference differences; middle `27720` retains
seventeen. None is accepted by this experiment. `recovery.json` remains the
authority, with 479 accepted entries / 48,304 bytes.

The question was whether the existing coordinate-factory field view, grouped
coordinate assignments and byte-width flag arithmetic explain the three large
worklist contributions. They reduce the private unguarded C control's
544/400/288-byte extents to the native 272/192/144-byte extents. Extent equality
alone does not establish instruction equality or historical source identity.

| Private control | Normal residual bytes | Middle | Alternate |
| --- | ---: | ---: | ---: |
| Initial typed view and grouped stores | 204 | 150 | 91 |
| Write record flags through the WORD field | 12 | 116 | 58 |
| Native owner/store grouping and masks at table uses | 2 | 17 | 0 |

The WORD `flags |= 0x8000` operation compiles to the original single high-byte
memory OR. An explicit high-byte union operation instead compiles to separate
load/OR/store instructions. Keeping the entry kind as a WORD and applying
`& 0x0fff` at the middle/alternate table uses removes a redundant full16 cleanup.
Widening the entry kind itself gives different code and was rejected.

The original loads the indexed group owner before storing actor field `38`.
Grouping the relative-offset assignment before the group assignment lets this
compiler schedule those independent actor stores as observed. A separate owner
pointer temporary changes earlier register scheduling. These measured source
forms preserve the intended arithmetic; they do not prove historical declarations
or arbitrary alias equivalence.

The normal residual is confined to two register choices in its inherited-flag
ORs. Middle additionally differs in group-load/push scheduling and equivalent
scale-one SIB operand order. Signed-byte/DWORD controls, unsigned mask suffixes
and explicit flag temporaries reproduce the same residual. Repeating those
controls requires new native/source-context evidence. Do not introduce assembly,
instruction patches or guessed padding to erase the differences.

Replay the frozen source recipe, freshly compile and run the 108 isolated cases:

```text
python scripts/worklist_typed_probe.py --output work/worklist_typed_replay
```

`--compile-only` skips runtime comparison. The tool verifies its recipe baseline
and oracle hashes, materializes C++ under ignored `work/`, and uses the existing
pinned historical compiler and Unicorn harness. The fixture uses `/OPT:NOREF`
only to retain the unused intercepted guard-model endpoints required by that
harness. No original OS module is loaded, and no native instruction is patched.
The recipe can also be materialized with `scripts/region.py materialize`.

All 108 cases agree in complete synthetic game storage, ordered writes,
descriptor reads, helper/callback entries, interventions and return contracts.
Their native results and case definitions are identical to the previous live-read
review. They cover group0 and synthetic descriptor/callback banks; callbacks are
models. They do not prove capacities, arbitrary aliases or callback bodies.
The private typed candidate omits diagnostic guards. Separately, the unchanged
production C providers still pass all nine actual source-prefix guard controls.

Matching remains open even for alternate: the recovered `g_kind_info[1]` is only
an observed prefix, and the full16/low12 factory callbacks fall outside the frozen
`rooted-callback-v1` verifier. No binding or indirect-transfer rule changed, and
the guarded production providers were not replaced. Fresh verification preserves
all 438 strict proofs with identical instruction/reference/context hashes;
all 321 Python tests pass.

The materializable source recipe is
`evidence/worklist_typed_recipe_2026_10_25.json`. Measurements, input hashes,
control output identities and regression checks are in
`evidence/worklist_typed_progress_2026_10_25.json`. The earlier
[live-read review](worklist_live_reads_2026_10_25.md) retains its original dated scope.
