# Whole-image consistency tier (CONSISTENT, design 2026-11-25)

Status: **proposed**. Adversarial review rvCON (GPT-6.2 Sol xhigh, 2026-11-26):
- Round 1 found 12 holes and 3 gaps.
- Round 2 found 10 holes and 3 gaps.
- Round 3 found 8 holes and 3 gaps.
- Round 4 found 5 holes, 1 gap and 1 documentation issue.

The regression suites are `tests/test_consistency_review.py`, `tests/test_consistency_round2.py` and
`tests/test_consistency_round3.py` and `tests/test_consistency_round4.py`.

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
     base-relocation set touching the contribution. Four-byte relocation fields must be in bounds and
     disjoint, including for the fixed-base EXE.
   - A reference into the row's own contribution must land inside it. An own function symbol must
     resolve to its entry.
   - An own-section DIR32 must mark a supported absolute operand. Arithmetic-only references require
     a contribution-local label or a section symbol. Named functions retain their scoped identities.
   - For a DLL, a reachable flat `jmp dword ptr [reg*4 + disp32]` can identify an inline table. Its
     disp32 must have an own-section DIR32 giving the start of a contiguous run of four-byte local
     DIR32 cells with matching native fixups. Each cell must target a decoded code boundary, and the
     run must lie outside the decoded code. Unsupported instruction operands and other unclassified
     fields fail collection.
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
   - A binding name gets its linkage from the bound row's own compiled unit. A file-local name is
     scoped to its object. Contradictory accepted records fail their key.
   - The rows that observe the map are the module's instruction-equal rows whose operand forms are
     supported. Refused rows and conflicting keys are listed in the report. The map is claimed over
     the rows that survive, not over every byte-equal row.
3. **Injective, non-overlapping storage.**
   - No two data keys share or overlap storage.
   - Accepted data pin intervals and every named or ordinal PE IAT slot are reserved whether or not
     any row references them. A pin with unresolved linkage or contradictory records reserves each
     recorded interval under an opaque identity. Symbolic attribution requires resolved linkage.
   - An ordinary pin overlapping an IAT slot fails. The PE slot retains its canonical import identity.
     The grade-B cap applies to collisions between two accepted pin intervals.
   - A verified alias and its scoped owner share placement, definitions and dependency checks. Every
     pin in the group must agree on the base. Only identical intervals qualify for the overlap
     exception. An accepted `source_alias` record identifies the compile-renaming owner, and that
     owner's compiled profile establishes its linkage. Unresolved ownership and matching spelling
     alone do not qualify.
   - A use or definition that extends a pinned object beyond its accepted interval is a separate,
     non-authoritative extent and must not overlap anything.
   - Declared sizes come from CodeView or the unambiguous listing rule below. Accepted sizes and verified access widths also bound
     storage. A direct store proves its full width. The verifier recognizes x87 and first-operand
     stores by mnemonic. A read proves one byte because VC5 widens narrow loads.
   - A direct access before an object's base fails.
4. **Definitions equal the image.** Every definition of a key among all compiled recorded profiles (every
   data symbol, external or file-local, whether or not the profile's rows are equal) and the data units
   must equal the native bytes at the base.
   - In an initialized definition, the literal bytes must match. The four-byte pointer fields must
     be disjoint and in bounds. For a relocated image, the fields must match all touching native
     fixups exactly.
   - Zero definitions: no touching fixup, and zero storage or zero file bytes.
   - Build each source and flags profile once for comparison, then compile it with `/Z7` for CodeView
     metadata. A separate owner entry for the same profile shares these builds. Use the ordinary COFF
     object for all code, data bytes, fixups, and symbol bounds.
   - Bind each C11 `S_GDATA32` or `S_LDATA32` record through its matching offset and section COFF
     relocations. Both objects must identify the same unique linkage symbol. The type supplies the
     size for scalars, qualified types, pointers, enums, arrays including nested arrays, and complete
     structs, unions and classes. Array records specify the total in bytes, including when VC5
     references a forward declaration for the element. A standalone forward declaration has no size.
     The declared size must fit before the next symbol or section end. A COMMON definition must
     agree exactly with its ordinary COFF size.
   - When CodeView and an exact listing both supply a size, they must agree. An unresolved or
     invalid data record leaves its definition unsupported. If a data record's relocations
     cannot identify any symbol, the profile's sidecar cannot establish which symbols lack a
     record, so no definition in that profile falls back to the listing.
   - For a symbol without a data record, the ordinary profile's `/Fa` listing may prove the
     extent if all operands are recognized and the object ends without a trailing `ORG`. Blank lines, comments and visibility directives preserve the
     active object. An internal `ORG` followed by data contributes its stated bytes. A trailing
     `ORG` can encode either alignment or real zeros and leaves the listing extent unproven.
   - A key defined twice in one unit fails.
   - File-backed data needs a definition or an accepted binding.
   - Code uses and placed data pins seed definition discovery. Each batch uses a stable placement
     snapshot. A source publishes equations after its local checks and rejects all its exports
     if any pointer contradicts an authoritative base or has an invalid form. A late placement
     conflict quarantines the source and replays discovery without its equations. Locally viable
     paths hidden by ambiguity reserve possible storage against weaker claims. They supply no
     witnesses. A failed target invalidates its dependents transitively.
   - Function pointers in data need an instruction-equal callee entry and no addend.
5. **Callees.** A function key's recorded rows (identity scoped like keys) decide its address:
   - several addresses fail;
   - the callee must be instruction-equal at the referenced entry, in code;
   - otherwise an accepted *function* binding must name that entry.

   An unrecorded callee, or an address recorded under another name, fails.
6. **Grade.**
   - Accepted rows vote only when they pass every rule above themselves. The report lists tier failures
     and collection refusals separately.
   - **A:** every key in the row's closure is content-proven or used by at least two live rows. The
     closure is its direct keys plus the data reached through payload pointers.
     - Content proof: a function, an import, an accepted binding, or a definition with at least 8
       compared literal bytes of more than 2 distinct values. Pointer addends don't count.
     - One pair of live rows must directly use the key, differ in native bytes and masked bodies, and
       have disjoint native extents. The same pair must satisfy all three conditions. No third row's
       extent enters that comparison.
     - Compute the greatest fixed point over the eligible rows, including accepted rows, by retesting
       the pair requirement as rows leave the strong set.
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

## CodeView-only baseline measurements

The fresh report compiled 557 distinct profiles with eight workers. It made 1,113 VC5 calls and
took 93.649 s, compared with 565 calls and 35.978 s for the listing-based baseline. One
unrecorded candidate provider's ordinary build failed in both runs. Both runs collected the
same 2,133 code rows and found zero accepted address contradictions.

See [the CodeView-only comparison](../cv4/COMPARISON.md) for baseline grades and defects. The [Round 4 comparison](../cv4/round4/COMPARISON.md) covers that verifier.

## Round 3 strict pre-integration snapshot (2026-11-26)

| Pending rows | Grade A | Grade B |
|---|---:|---:|
| ENG1 | 15 rows / 2,398 B | 24 / 5,730 B |
| ENG3 | 9 / 1,758 B | 11 / 3,778 B |
| TITLE | 30 / 5,360 B | 78 / 23,360 B |
| EXE (no fixups) | 53 / 3,536 B | 41 / 7,840 B |

Both fresh runs reported zero accepted address contradictions. The revised collector refused 36
accepted rows with unsupported own-section DIR32 fields. Of the 1,447 accepted rows evaluated, 174
failed a tier rule and were ineligible to vote. The 22 accepted pin collisions stayed the same.
See [the strict collector receipt](../COMPARISON.md) in the repository's `work/velocity_q/fixCON/` lane for each grade change.

The type and symbol record layouts follow [Microsoft's CodeView definitions](https://github.com/microsoft/microsoft-pdb/blob/master/include/cvinfo.h).

## Round 5 measurements

The fresh baseline and revised reports were byte-identical. Grade A stayed at 115, grade B at
164, and accepted address contradictions at zero. See [the Round 5 comparison](COMPARISON.md).

## Round 4 measurements

The revised report collected 2,133 code rows, including 1,483 accepted rows, with zero accepted
address contradictions. The proved listing fallback restored 384 of the 390 previously unsupported
extents. The 115 grade-A and 164 grade-B rows match the original listing report row for row.
See [the Round 4 comparison](../cv4/round4/COMPARISON.md) for those row and defect changes.

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
  that disagree with the map and use-level overlaps labeled by row and reference;
- definition defects;
- accepted-name collisions.

`python scripts/consistency.py check` fails on any new entry or any accepted-row address contradiction.
Address disagreements are checked independently of other defects and retain their classification through
every level of pointer dependencies. A comparator address contradiction also survives a collection
refusal and fails `check` for an accepted row. A conflict is repaid by giving each object one name and one owner (as pop2 did) or by fixing the
definition, never by editing the ledger.

## Commands

    python scripts/consistency.py report --output work/consistency/report.json
    python scripts/consistency.py check
    python -m unittest discover -s tests -p "test_consistency*.py"
