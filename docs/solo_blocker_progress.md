# Solo blocker investigation, 2026-10-11

This increment fixes a compiler-diagnostic defect and narrows two TITLE blockers.
It adds no strict or masked code matches. Canonical game source and recovery state
remain as they were at the start of this increment. All experiments are private;
the historical compiler, flags and acceptance rules remain unchanged.

## Implemented diagnostic fix

`leaf.py diff` previously replaced every numeric operand in an instruction that
contained a relocation. For example, two stores to a relocated address with
different immediate values could receive the same comparison key. Its zero
displacement substitution also contained a control character instead of a
register backreference. The existing byte gate still rejected such differences;
the defect affected the explanation presented during compiler research.

The shared decoder in `residual.py` now normalizes only the encoded, supported
four-byte relocation field. Other immediates, displacements and index scales
remain visible. `leaf.py check` and the residual path also reject source/oracle
mutation during compilation. No second acceptance verifier was introduced.

The new `leaf.py residual` command reuses the full raw and relocation-binding
comparisons, then groups nearby residual byte spans into bounded instruction
windows. Complete windows can identify instruction ordering or register-operand
differences. Partial decoding stays explicit. Linear decoding may include inline
data; these classifications prove neither behavior nor source identity.

Pilot command from the repository root, using the configured analysis Python:

```powershell
python scripts/leaf.py residual TITLE.DLL 0xa450 7408 _title_0a450 candidates/wa450/u09a40_ctx.c --islands 1 --report work/solo_reload/residual_pilot.json
```

The 7,408-byte draft has six residual bytes, no binding conflicts, and one
seven-byte window at offsets `0xff0..0xff7`. It contains exactly two reversed
instructions: native `xor edi,edi; mov esi,15`, candidate `mov esi,15; xor edi,edi`.
The next `mov ebp,1` agrees. The compact report exposes the actual question
without mixing it with thousands of relocation bytes or the inline switch data.

## Compiler families measured

**Join reloads — 8,320-byte member ceiling.** One fresh unchanged 0xa450 compile
reproduced its six-byte residue. Six call-argument controls and six store controls
were each freshly compiled twice. Arguments alone use immediates and do not
exercise qualified constant webs. Store controls do; the two-predecessor case
rematerializes constants in the order 1,15,0 into EBX,ESI,EDI, while their earlier
loads map 15,0,1 to ESI,EDI,EBX. Final register ranking alone is insufficient to
explain that observed order. Repeated complete function bytes and ordered
relocations agree for every control; no whole-object timestamp claim is made.

Six ordinary target-derived join/tail forms were then measured. Explicit default,
named join and inverted guard forms retain 7,408 bytes but increase the residue
from six to sixteen bytes. Duplicating the pending-input tail emits 7,824 bytes.
The two canonical u09a40 peers retain raw bytes and ordered relocations in every
trial. None is eligible for staging. Keep the original six-byte draft. Reentry
requires a control that isolates incoming-edge constant survival and piece order
while retaining the observed allocation, or genuine new source/context evidence.
The other member, 0x18bf0, was not modified or newly measured in this increment.

**Decimal formatter — 224-byte target, 16-byte size excess.** Fresh native/draft
comparison corrects the old arithmetic-only diagnosis: the draft's 10 product is
already positive; its 100 product uses a negative chain. Native also keeps value
in EBX and loads the units pointer late, while the baseline keeps units in EBX
and value in EBP. Native first-flag result uses EBP. These additional allocation
and scheduling differences prevent treating a product rewrite as the whole fix.

Four minimal arithmetic controls show that isolated signed and unsigned products
both emit positive 100 chains. Mixed-sign products inside the formatter are
inert. Explicit flag locals can change value's allocation to EBX, but introduce
other allocation/scheduling differences. Bitfield views, conditional flag
assignments, chained quotient stores and early remainder capture also fail the
raw gate. All fifteen formatter trials remain 240 bytes. No candidate replaces
the existing reconstruction. Reentry requires a cut-down quotient/store/flag
control explaining why the units pointer's range is selected, or real type/TU
evidence; do not repeat the arithmetic spelling family.

## Validation and next priorities

Fresh historical regression passes **113 strict rows / 7,840 bytes** and
**342 masked rows / 81,184 bytes in 30 sources**, with no failures. All **54 Python
tests** pass, including relocation-field precision, truncated decode handling,
bounded output, and source/oracle mutation refusal. Match bytes include padding;
masked equality is diagnostic. Compact evidence and private report identities
are in `evidence/solo_blockers_2026_10_11.json`.

Next work should complete real TITLE contributions/data ownership for the natural
link, or pursue the narrowed constant-piece and units-range controls. The four
wrong-size drafts remain real layout blockers. Avoid another declaration-count,
store-permutation or arithmetic-spelling sweep. The new diagnostics apply across
TITLE, ENG1, ENG3 and the EXE without transferring a compiler conclusion from
Blood II or adding a scheduler.
