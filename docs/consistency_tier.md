# Whole-image consistency tier (CONSISTENT, design 2026-11-25)

Status: **proposed**. Adversarial review rvCON (GPT-6.2 Sol xhigh, 2026-11-26):
- round 1 found 12 holes and 3 gaps;
- round 2 found 10 holes and 3 gaps.

All are fixed or stated below. Their 53 reproducers are regression tests:
`tests/test_consistency_review.py` and `tests/test_consistency_round2.py`.

This is a separate and weaker tier:
- it never changes `recovery.json`;
- it never counts as FUNCTION_MATCH or FUNCTION_MATCH_RELOCATED;
- it is always reported separately.

`scripts/consistency.py` recomputes it from fresh compiles on every run and stores no result as acceptance.

## Why a separate tier

On 2026-11-25, 671 rows / 211,527 bytes compiled instruction-equal but were not accepted. The RELOCATED
gate needs one independent anchor per reference outside the compared caller, and most of these rows lack
it for one or more data cells. The sibling projects check one symbol-to-address map across the whole
image instead:
- blood2 `engverify` and blupi `cmpfunc`, the VC5 siblings;
- pop2, with `CODE_MATCH` plus a shrink-only link-consistency ledger. It then reached a complete
  whole-image link within five days.

This tier adopts that check without weakening the existing gates.

## The claim

For each module separately, a CONSISTENT row satisfies all of the following.

1. **Code.**
   - The row's complete fresh `/Gy` contribution is instruction-equal to the native extent
     (`masked._compare_section`). In a DLL this includes that the DIR32 field set equals the native
     base-relocation set.
   - A reference into the row's own contribution must land inside it; an own function symbol only at
     its entry.
   - A REL32 field must name a function symbol and sit in a branch. An own-section REL32 must also be a
     branch.
   - A named own-function reference records the row's entry as a use of its key, so it must agree with
     any pin.
   - Function and import references carry no addend.
   - An import slot is only read directly as flat memory: no index register, no store, no segment
     override.
   - Segment-overridden operands and FPU state saves are refused.
2. **One map.**
   - Every reference resolves to a base (target minus addend).
   - Externals are keyed per module; file-local names per source, flags tuple and name.
   - A key is one kind of object everywhere: function, data or import. Kinds come from code uses,
     payload pointers, definitions, pins and recorded rows together. Each key has one base.
   - Canonical values:
     - a function's entry is its function pin, else its recorded rows, else its uses;
     - an import's value is its IAT slot, named or ordinal;
     - data takes its pin, else its single observed base.
   - An accepted binding fixes the base, and then only disagreeing uses fail. A wrong payload pointer
     fails its own table, not the pinned target.
   - A binding name gets its linkage from the bound row's own compiled unit: a file-local name is
     scoped to its object. Contradictory accepted records fail their key.
   - The rows that observe the map are the module's instruction-equal rows whose operand forms are
     supported. Refused rows and conflicting keys are listed in the report. The map is claimed over
     the rows that survive, not over every byte-equal row.
3. **Injective, non-overlapping storage.**
   - No two data keys share or overlap storage.
   - Accepted extents and every IAT slot (named or ordinal) are reserved under their own identities,
     even when used.
   - Overlapping accepted pin intervals are listed as collisions, and every user of one is at most
     grade B.
   - Exception: equal intervals of names that an accepted `source_alias` record makes aliases of one
     owner. These are the reviewed compile-renaming aliases, one object under two names, and are not
     collisions.
   - A use or definition that extends a pinned object beyond its accepted interval is a separate,
     non-authoritative extent and must not overlap anything.
   - Extents are lower bounds: the exact compiler-listed size (`/Fa`, the only addition to the recorded
     profile), COMMON sizes and accepted sizes. A direct store proves its full width; x87 and
     first-operand stores are recognized by mnemonic. A read proves one byte, because VC5 widens narrow
     loads.
   - A direct access before an object's base fails.
4. **Definitions equal the image.** Every definition of a key among all compiled recorded profiles (every
   data symbol, external or file-local, whether or not the profile's rows are equal) and the data units
   must equal the native bytes at the base.
   - Initialized definitions: literal bytes equal, and every native fixup that touches the object is
     exactly at a pointer field of the definition.
   - Zero definitions: no touching fixup, and zero storage or zero file bytes.
   - A listing form the parser does not recognize makes the definition fail. This includes any
     unrecognized line inside an object and an ORG operand that is not one complete number.
   - A key defined twice in one unit fails.
   - File-backed data needs a definition or an accepted binding.
   - Pointers inside a placed definition bind their targets. A failing target fails every definition
     that points to it, transitively.
   - Function pointers in data need an instruction-equal callee entry and no addend.
5. **Callees.** A function key's recorded rows (identity scoped like keys) decide its address:
   - several addresses fail;
   - the callee must be instruction-equal at the referenced entry, in code;
   - otherwise an accepted *function* binding must name that entry.

   An unrecorded callee, or an address recorded under another name, fails.
6. **Grade.**
   - Accepted rows vote only when they pass every rule above themselves. The report lists the ones that
     fail.
   - **A:** every key in the row's closure is content-proven or used by at least two live rows. The
     closure is its direct keys plus the data reached through payload pointers.
     - Content proof: a function, an import, an accepted binding, or a definition with at least 8
       compared literal bytes of more than 2 distinct values. Pointer addends don't count.
     - Witnesses: two rows count as two only if their native bytes differ, their masked bodies differ
       and their extents do not overlap.
     - This is a greatest fixpoint over all live rows, accepted ones included: a witness must itself
       stay strong.
   - **B:** everything else that passes.
   - HERCULES.EXE has no base relocations, so which absolute fields are references comes from the
     candidate's own relocation inventory. An integer literal and an address with the same value cannot
     be told apart. EXE rows are always reported separately as "no fixups", and their grade is
     conditional on that inventory.

## What it does not claim

- **Original names.** Swapping two names consistently everywhere is invisible; names stay labels.
- **Original data layout, TU boundaries or link order.** Only the placement the code already uses is
  checked.
- **Capacity of indexed storage.** Indexed accesses prove no extent.
- **The definition inventory is the recorded profiles plus data units.**
  - Owners named by accepted bindings are compiled with their recorded flags.
  - Other unrecorded files of a module's source folder are compiled with `/O2` as candidate evidence.

  The report lists each unit, its origin and any compile failure.
- **Anything about rows that are not instruction-equal.**

## Measured state (2026-11-26, after the round-2 fixes)

| Pending rows | Grade A | Grade B |
|---|---:|---:|
| ENG1 | 21 rows / 4,878 B | 29 / 9,682 B |
| ENG3 | 9 / 1,758 B | 13 / 5,938 B |
| TITLE | 32 / 5,472 B | 81 / 25,120 B |
| EXE (no fixups) | 53 / 3,536 B | 42 / 8,256 B |

- **Calibration.** All 1,483 accepted rows were evaluated under the same rules, with 0 address
  contradictions. 41 accepted rows fail some tier rule (mostly callees without a row) and do not vote.
  22 pairs of accepted pin intervals overlap, for example a data owner whose accepted extent runs into a
  string literal. They are listed as collisions for review.
- **Mutation power.** One reference of a live row was mutated: retargeted to another symbol, shifted by
  2-16 bytes, or renamed. All 160 of 160 mutations were rejected in grade A rows, and 318 of 320 overall.
  Every miss renamed a grade-B single-witness symbol at its own address: a label change, not a wrong
  reference.
- **Source defects found that instruction equality hides:**
  - `eng1.dispatch_local_258e0` walks `g_actor_resources` 16 bytes low; the PSX `SDBASE` struct has
    `Status` at +0x10;
  - a typo, `TTRAIN training` against native `TTRAIN raining`;
  - nine EXE strings declared as oversized arrays;
  - shared units whose data differs per module (`_g_26110`).

## Why pending rows fail (2026-11-26)

| Reason | Rows |
|---|---:|
| Storage shared with another name | 120 |
| Callee without a ledger row (library members, unsourced code) | 120 |
| File-backed data with no definition | 93 |
| Callee not instruction-equal | 88 |
| Callee recorded under another name | 32 |

## Ledger

`evidence/consistency_debt.json` holds three shrink-only lists:
- name conflicts: several addresses, shared storage, callees under another name, kind conflicts, pointers
  that disagree with the map;
- definition defects;
- accepted-name collisions.

`python scripts/consistency.py check` fails on any new entry or any accepted-row address contradiction.
A conflict is repaid by giving each object one name and one owner (as pop2 did) or by fixing the
definition, never by editing the ledger.

## Commands

    python scripts/consistency.py report --output work/consistency/report.json
    python scripts/consistency.py check
    python -m unittest discover -s tests -p "test_consistency*.py"
