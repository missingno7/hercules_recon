# Actor-pair animation recovery

Three missing ENG1 functions now have real C providers and RELOCATED acceptance:
`1a280` (48 bytes), `1a2b0` (224 bytes), and `1a390` (80 bytes). The ledger has
487 accepted entries / 49,024 bytes: 137 RAW, 307 RELOCATED / 34,816 bytes,
and 43 retained linked rows. All 484 earlier accepted records remain identical.

The experiment asked whether other native clients of the existing complete
192-byte WORD owner at `55478` identified missing source. Initialization selects
its byte+48 or byte+72 program root; update reads its bounded ten-WORD table at
byte+96. The complete independently located owner is reused without adding data.
The HIGHLOW census is a search aid, not proof of exhaustive references or capacity.

`actor_pair_animation.c` preserves the measured behavior:

- Release tests the first actor's kind, removes it, then reloads the second root
  after the first removal. It adds no null-pointer guard.
- Initialization reloads collision coordinates, preserves low-WORD arithmetic and
  32-bit subtraction wrapping, runs reset and WORD interpretation, then reads
  the resulting flags and the latest collision root.
- Update zero-extends the pool WORD, subtracts 351, clamps to 0..9, and reads the
  existing table. Only an interpreter result of exactly one invokes release.

The four pointer cells use actual existing data-only owners. The setup producer
at `15090` independently establishes the two release roots; earlier independent
population witnesses establish the normal-pool and collision roots. Owner source,
flags, complete four-byte COMMON objects, headers and instruction identities are
pinned. Reset, WORD interpretation and removal have separately reviewed entry
witnesses outside these three callers. This accepts caller references, not the
unaccepted helpers' entire dependency graphs. Thirteen wrong-target controls reject.

The old `eng1_1a280` consumer uses an explicit same-entry function alias. Its
complete compiler sections, relocations and symbol inventory agree after that
sole undefined-name map. The physical cdecl protocol is verified; historical
source spelling, callback return type and translation-unit identity remain unknown.

```text
python scripts/actor_pair_animation_differential.py
```

The isolated Unicorn composition passes 4,428 native/source cases: 12 release,
1,152 initialization and 3,264 update. Real reset instructions and the composed
release execute; WORD interpretation and middle removal are explicit mutation
models. Cases cover live root changes, actor/root aliases, arithmetic boundaries,
volatile-register clobbers and interpreter return discrimination. Ordered writes,
observed reads, complete guarded actor memory, calls, roots, stack and registers
agree. Raw program pointers are checked before role normalization. These finite
cases do not prove modeled service semantics, pool capacities or gameplay closure.
No original OS module starts and no original instruction or file is patched.

The genuine source graph has 248 units, 965 mapped entries, 1,042 external function
symbols and 726 unresolved dependencies, four fewer than before. All 247 previous
units / 1,213 function contributions retain their complete sections and symbol
records after the one reviewed name map; FILE metadata is checked separately.
There are no duplicate or generated providers. The genuine link still fails with
unresolved-symbol exit 25; no runnable ENG1 DLL is claimed.

All 446 strict proofs, 43 linked checks, 321 Python tests and 1,551 supported
masked diagnostics pass. The existing 86 broader linked provenance drifts remain.
Frozen references, runtime evidence, bindings and the final audit hashes are in
`evidence/actor_pair_animation_*_2026_10_25.json`. Full matching remains unfinished.
