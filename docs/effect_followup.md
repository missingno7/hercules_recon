# Effect-region closure and first natural-link probe

This continuation promotes the four raw-equal blind-region functions and removes
the remaining three source/code-generation differences. The twelve-function,
816-byte effect region now has **four raw equals (192 bytes)** and **eight
relocation-only diagnostic equals (624 bytes)**. This is not a linked module match.
The original macro/refinery sources and recorded first-pass results are preserved.

## Targeted reasoning after classification

The [refinery experiment](refinery_experiment.md) left three hard cases for a fresh
source-level diagnosis. Re-reading their instruction ordering exposed small,
ordinary C differences. This follow-up is not additional blind validation.

| Function | Diagnosis and retained source form | Normalized differing bytes | Tested variants |
|---|---|---:|---:|
| `tick_particle` | Move the independent y update before toggling/testing ticks | 36 → 0 / 48 | 1 |
| `initialize_sound_effect` | Clear phase/ticks separately before multiplying scale | 23 → 0 / 96 | 1 |
| `maybe_start_effect` | Put the zero-delay path first and decrement in `else` | 94 → 0 / 112 | 1 |

Each first candidate succeeds. Bounds were three movement/condition forms, four
zero-store/scale forms, and two equivalent zero predicates; remaining forms were
not compiled after equality. These are meaningful source-order/branch changes,
not arbitrary allocation tricks. They preserve callback order and the values
visible to callbacks. The outer branch change repairs the mismatched ordered CFG.

Each search freshly compiles the full preserved source with VC5 RTM `/O2 /Gy`,
checks all other contributions remain unchanged, and separately compiles/runs
the existing 3,392-check effect harness. Three baselines plus three variants cost
six target builds and six semantic builds per replay. The retained changes are
then combined with the already verified burst zero-store change, compiled once
more, and checked as a full region. All twelve normalized contributions agree.
The replay script fails if that recorded result regresses.

An additional reconstructed-code harness runs 461,145 checks. It covers every
16-bit tick and scale value, unchanged fields, the object state observed by
release/sound callbacks, and delay/busy/blocked/random combinations. Random-call
side effects on the blocked flag verify the relevant read/call ordering. No
original executable or DLL is loaded. Finite testing plus reviewed transformations
supports these forms; this is not a general formal equivalence proof.

`scripts/effect_followup.py` contains the families and reconstruction recipe;
[compact evidence](../evidence/effect_followup.json) pins candidate hashes,
comparison results, and tool/test identities. Scratch source, objects, and full
logs remain under ignored `work/effect_followup/`.

## Four canonical promotions

The following functions were isolated in `src/shared/effect_state.c` without
changing their compiled contributions:

| Function | ENG1 RVA | Complete contribution |
|---|---:|---:|
| `initialize_backdrop` | `0x216b0` | 64 bytes |
| `initialize_particle` | `0x216f0` | 32 bytes |
| `wrapped_delta` | `0x217e0` | 48 bytes |
| `initialize_burst` | `0x21910` | 48 bytes |

A canonical-only harness adds 17,153 checks, including preservation of all other
struct bytes, signed-short scale narrowing, fractional fixed-point coordinates,
and wrapped-distance boundaries. Complete COFF object coverage and fresh raw
equality passed before promotion. The unchanged normal gate then freshly verified
all 36 accepted instances and committed source/proofs together.

`recovery.json` now records **36 accepted function instances, 27 source functions,
and 4,176 mapped bytes**: +4 instances / 192 accepted bytes. These were previously
experimental raw matches, so promotion is not counted as a new matching discovery.
The three new diagnostic equals add 256 bytes to the linkage queue and receive
no `FUNCTION_MATCH` status.

## Natural linkage: actual output, unresolved layout

`scripts/linkage_probe.py` extracts only reconstructed `unlink_12c` and
`hide_and_unlink`, freshly compiles them, and uses pinned LINK 5.00.7022 to emit a
normal DLL with both functions exported. It uses `/DLL /NOENTRY /NODEFAULTLIB
/INCREMENTAL:NO /OPT:NOREF`; these are experimental isolation flags, not claimed
original linker flags. No `/BASE`, `/ORDER`, forced section placement, artificial
padding, stubs, original-binary dispatch, or patched relocations are used.
The generated image is inspected as data and never loaded.

| Actual linked contribution | Result against original complete bytes |
|---|---|
| `unlink_12c` at RVA `0x1000` | Raw equal, 64/64 bytes |
| `hide_and_unlink` at RVA `0x1040` | Two differing bytes, offsets 14 and 15, within the REL32 operand |

The actual linked call resolves to the reconstructed callee. No PE base
relocations remain in this minimal image. Nevertheless, natural caller–callee
separation is 64 bytes versus the original 512 bytes. That distinction is the
measured reason relocation-normalized equality was insufficient.

The original interval contains 448 bytes of real contributions between the two:
attachment detach (64), child detach (64), buffer detach (48), scene insertion
(128), scene unlink (80), and tag lookup (64). Recovering that contribution set
and its real dependencies is the next evidence-based linkage step. `find_tag`
still has source differences; release/data dependencies are not closed. Recreating
the 448-byte distance with padding or dummy functions would prove nothing and is
not used. A complete linked-image acceptance path remains a separate future
review; the current relocation-free object gate has not changed.

[Link evidence](../evidence/linkage_probe.json) records both complete function
hashes, the emitted call destination, input/command identities, and the actual
image/map hashes. Whole image/map hashes contain linker timestamps and may differ
between replays; compare the recorded contribution hashes and layout metrics.
No whole-binary equality is claimed.

## Verification and next work

All 36 accepted function instances passed the normal fresh-compile gate. All seven
registered semantic suites passed (120,473 checks), plus the 461,145-check combined
follow-up harness. The 21 Python gate/metric tests and hashes of all twelve
immutable originals passed. Existing macro and blind baseline files are unchanged.

These results support the selective-reasoning step: a structural/hard label does
not necessarily imply a difficult function, and a concrete source-order diagnosis
can eliminate the tail cheaply. Three successes following manual diagnosis are
not an independent estimate of automatic search throughput. The earlier negative
calibration findings remain valid.

Next priority is natural linkage closure with real dependencies and contribution
boundaries, while preserving the frozen effect region. Avoid another broad search
over its eight relocation-only functions. The tag lookup and release dependencies
are better targets for new semantic/context evidence than repeated high-similarity
algebraic variants.

```powershell
python scripts/effect_followup.py
python scripts/linkage_probe.py
python scripts/test_pilot.py --suite all
python scripts/match.py verify --report build/verification.json
python -m unittest discover -s tests -p 'test_*.py'
python scripts/inventory.py --verify
```
