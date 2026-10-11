# TITLE data closure increment, 2026-10-10

This increment brought the canonical diagnostic link to **105 unresolved symbols instead of 107**,
with no duplicate definitions. `calibration/title/u0cc90.c` defines the selected
password wheel's initial value and the password/level table. Its complete emitted
1,016-byte initialized data contribution equals TITLE `0x25a28..0x25e1f`, both in
a fresh object and in the naturally linked diagnostic layout. This proves the
recovered interval's bytes, including four naturally emitted alignment bytes;
it does not prove original object extent, native placement or whole-image equality.

The table has **42 records of six 32-bit integers**: four sprite codes, a result
code and difficulty. It contains 36 ordinary records and six special codes.
The prior `43 * 6` declaration extended into the separately referenced trace
literal at `0x25e20`. A negative control with 43 declared rows emits 24 extra zero
bytes even though all existing function peers remain equal. Code equality alone
would therefore miss the wrong data extent.

The native search still makes 43 iterations, and its loop remains unchanged.
On the extra iteration it compares the literal's first word, `1734692128`, with
a zero-extended sprite frame minus `0x1e3`, whose range is `[-483,65052]`. That
comparison always rejects. The recovered C array does not make this access valid
C behavior; at this milestone the missing `0xd910` literal was not yet restored beside it.
No complete runtime or behavior-exact claim is made for this partial unit.

All **13 existing unit functions / 5,952 bytes** retain their masked identities
and implied symbol maps. Fresh regression also passes all **113 strict instances
/ 7,840 bytes** and **331 masked rows / 80,928 bytes**. Only the thirteen masked
rows' source-proof hashes and evidence references changed; statuses, target
extents and code identities are unchanged. `data_units.json` registers this
measured partial interval for future link diagnostics. Durable evidence and exact
report hashes are in `evidence/title_password_data.json`.

The separate r04d80 ownership audit rejected another tempting data addition.
The old note's ranges total 160 bytes, not 152; its second alleged int table
begins with `cpt(%d)` and padding, and neither table-like range has an identified
direct PC user. Their types and ownership remain unresolved. No globals were
invented to fill those gaps.

The independent ENG3 lane established two duplicated dispatch loops at `0x13a0`
and `0x13e0`, paired vector writers and common callback ABI. A single shared
ordinary-C template produces matching peer shapes but still fails both targets:
30 non-relocation differences and shifted relocation offsets. The new bounded
blocker is pointer anchoring at the count word versus the first slot. Evidence
and the re-entry condition are in `evidence/eng3_dispatch_siblings.json`.

Next recover a genuine missing TITLE contribution, preferably `0xd910`'s frame
tables and referenced literal in its real unit context. Do not retry exhausted
scratch-register families merely because the data table now exists. The ENG3
family needs an independent accepted counted-vector anchor before further source
spelling trials. Generated objects, listings and recipes stay in private scratch.

```powershell
python scripts/title_link.py
python scripts/similar.py verify
python scripts/match.py verify --report work/title_data/raw_regression.json
```

The subsequent 2026-10-11 increment is in `docs/frame_and_interface_progress.md`:
the frame/trace interval grows to 1,492 exact data bytes, and eleven ENG3
interface wrappers add 256 masked code bytes. First-call/motion ownership remains
unresolved after a failed natural-link alignment prediction.
