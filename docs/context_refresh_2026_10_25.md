# Preserve accepted functions while correcting their source unit

`scripts/reprove.py` handles a reviewed correction to unaccepted functions in a
source unit that already has relocated accepted peers. It does not accept new
functions, change bindings, targets, extents or flags, or relax RAW/mixed-unit
adoption rules. Its first use corrects two callback argument orders while
preserving accepted `1e00`.

Supply a schema-1 review under evidence/ naming the source, exact before/after
SHA-256 hashes, changed COFF function names, reason and native evidence. Put the
candidate under candidates/ and generated report under work/:

    python scripts/reprove.py --review evidence/review.json --candidate candidates/unit.c --report work/reproof.json

`--verify-only` runs the same checks and restores source/state. Function acceptance
still uses `scripts/promote.py`; refresh is not an alternate acceptance route.

The tool takes the existing promotion lock, freshly verifies every strict proof,
and journals the temporary source edit. Every accepted function is freshly
compiled again against the same original bytes and independent binding map.
Complete compiled hashes, target hashes and categories must equal the baseline.
Diagnostic peers in the edited unit retain byte identity and symbol maps.

The exact reviewed function set must account for every changed contribution.
Adding/removing functions or changing other functions, data, COMMON extents,
imports or linkage directives rejects this implementation. Only relocated peers
in one canonical source/compile profile are supported. RAW peers and header
migrations remain unsupported.

Original reference manifests stay immutable. A descendant changes the reviewed
source hash and pins its parent plus review; binding records/hashes stay identical.
Every accepted row sharing that manifest is reproved. Source, objects, compiler
inputs, review, manifests, targets and state are rechecked before publication.
Failure restores source and state; inactive descendant proposals can remain.

Fourteen tests cover contribution inventories, references, data/COMMON/import
preservation, immutable old metadata, stale baselines, RAW rejection, fresh checks
and rollback. Even an exact new result rejects if its compiled bytes differ from
the old proof. The actual correction passed all 345 then-existing strict proofs
both before and after the edit, plus linked checks, without changing acceptance.
