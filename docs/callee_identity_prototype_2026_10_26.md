# Native callee identity: restricted own-body method (review proposal)

Question, before implementation: can the exact source symbol be tied to a native
entry without using any pending caller operand? Yes, narrowly: compile that
symbol's own canonical body, locate its complete /Gy contribution independently,
and close its outgoing references from previously accepted evidence. A caller
vote or a name containing an RVA does not establish that connection.

## Admission classes

(a) RESTRICT. Exact COFF symbol, canonical source and ledger-recorded ordered
profile, fresh pinned compiler, immutable include closure; complete one-function
/Gy section including alignment; complete equality outside supported relocation
operands; exactly one location in all executable file-backed bytes of that same
module, not just one queue destination; independently delimited native entry and
extent; every outgoing reference must already be independently bindable. For this
prototype the automatic subset is relocation-free bodies only. This is deliberately
narrower than masked uniqueness: wrong globals/call targets can preserve masked
shape. Reject duplicate shapes and competing symbol claims. Require at least one
decoded direct CALL witness inside an accepted contribution or outside EVERY
pending ledger interval. The witness is entry corroboration, not the source of S.
Reject a witness inside any pending contribution even if it is not in the queue.
Do not use a pending callee's own bytes as a separate future witness: own-body
comparison is the explicit proof premise, not a witness borrowed by other rows.
ABI is equal to the compiled body including all RET immediates and stack accesses;
reject decorated cleanup conflicts, undecoded code, thunks and escaping branches.
No original source spelling or semantic completeness claim follows.

(b) RESTRICT to individually authored native identity reviews; no bulk admission.
Mechanically pin module, source/include/profile and complete native span hashes;
check entry/boundary, complete decode, instruction boundaries/CFG, stack argument
reads, RET cleanup, saved-register and return-register behavior, direct and indirect
transfers and all data/reference operands. A human must map every changed region,
branch predicate, state read/write, integer width/sign, fault path, call ordering
and reload to source semantics and justify the name/alias independently. Similarity
percent, a matching ABI, or finite tests do not suffice. Different CFG rejects the
automatic class; a future manual review may explain an equivalent CFG, but is not
provided by this tool. The ENG3 RNG review is a manual precedent, not a threshold.

(c) REJECT as standalone name evidence. Two independently compiled callers rule
out a one-caller operand transcription error if the other is omitted from the
proof. They do not rule out a systematically wrong shared declaration/name,
cloned source, swapped functions, shared wrong prototypes, or two pending callers
mutually certifying themselves. Cross-module votes must use each module's own
entry. An already accepted caller's independently pinned FUNCTION binding can be
reused as that existing evidence, not upgraded from masked caller agreement.

(d) REJECT as standalone name evidence. Prologue, padding, RET, stack agreement
and outside callers establish at most an entry and ABI. They do not name it.
The supplied function maps are diagnostic and use recorded extents themselves;
they are candidate delimiters, never independently authoritative identities.
Require own complete contribution plus native RET/NOP end and a safe direct caller.

## Special cases and controls

TITLE-named symbols are strings, not addresses: search ENG1's own PE for ENG1,
ENG3's for ENG3. Never copy a TITLE RVA. CRT/MFC/library contributions use the
pinned RTM library COFF name and complete member with resolved relocations, not a
reconstructed label; library_map is diagnostic (its own source documents the
ENG3 0x25040 / LIBCMT __scalb masked false positive). This prototype defers library
symbols instead of relabeling them. No thunk chasing, mid-function entry,
tail-merge or nonzero addend inference: those require a separate reviewed alias
contract. Two source symbols at one entry reject absent an independently reviewed
alias; this implementation provides no alias exception.

Original-backed counterexample search: full raw complete map spans already have
duplicates ENG1 {0x1000,0xc720,0x22180}, ENG3 {0x1000,0x2390}, TITLE
{0x16e0,0x1dd0}. Thus even unmasked own bytes are not inherently unique. Scan all
executable offsets, not only map starts, to detect ambiguous and interior shapes.
Further counterexample measurements and exact input hashes accompany report.json.
No negative search proves a universal absence of systematically wrong names.

## Outputs and integration gate

One new module, no verifier/ledger edits. Scan the fixed queue, compile a bounded
number of canonical source/profile groups and reuse one immutable object within
the run. A prior census without an object plus matching source/include/profile,
toolchain and object hashes cannot be reused; none are present in this worktree.
Every symbol receives a rejection/deferred reason, including budget-deferred.
Emit existing function_identities fields as proposals, plus reasons and counts.
Queue implied destinations are validation-only: they neither locate E nor select
between candidate bodies. Uses are counted both as relocation operands and as
unique (row,symbol) blocker uses. No acceptance claim.

Lane documents may refer only to a new immutable evidence snapshot and existing
accepted pinned records. Never cite this living design, queue/maps, or work files
as binding evidence. A work binding document is a proposal container, not evidence.
Integrator must review design, controls, original-backed measurements and evidence
snapshot before ANY lane use. Existing match.py references is diagnostic only.

## Measured result at base 96de7df

This is a partial prototype, not a deployable bulk identity campaign. Nineteen
fresh source/profile builds evaluated at least one profile for 127 of 399
module/symbol subjects. Eighty subjects had only budget-deferred profiles; 192
had no exact-symbol body/profile lead. Twelve evaluated subjects also have an
uncompiled alternative profile. These are scheduling/search limits, not claims
that their identities are impossible. No a0 identity was admitted: 0 symbols,
0 of 783 row/symbol uses, and 0 rows. There are 1,166 relocation operands in that
783-use planning set. The initial 16-profile diagnostic found 12 unique masked
body/profile/module combinations (11 symbols), all with outgoing references.
Some additionally need indirect-call review. No outgoing-closure extension was
implemented; the diagnostic established its missing prerequisites.

The original-backed controls also found same-entry multiple reconstruction names
(e.g. ENG1 0x1000: _dbg_printf / _engine_diagnostic; ENG1 0x4010:
_eng1_04010 / _title_03100). These are not claims about original source spelling.
Multi-TU names number 102/56/52 in ENG1/ENG3/TITLE; agreement alone still allows
uniform renaming. RET-zero leaf candidates number 184/96/44, demonstrating that
an entry/ABI description cannot name a function. The compiled _title_03050 shape
locates at ENG1 0x3f60, not its TITLE-like name 0x3050, but unclosed outgoing
references prevent admission. The pinned RNG manual NEAR review still matches
its native/source hashes and records 36 different nonreference bytes; it cannot
be replaced with a similarity rule. library_map.py documents a native masked
false positive at ENG3 0x25040 for LIBCMT __scalb.

No lane binding document, immutable binding evidence snapshot or references
comparison was manufactured: there are no qualifying positive rows. The request
for at least ten ENG1/ENG3/TITLE sample references therefore remains unmet. This
is the main open deliverable, not a successful bulk-method coverage claim.

Final focused tests: 15 pass, including an end-to-end synthetic scanner emission
and every requested rejection control. Full unittest discovery produced a
468-test OK summary before the final overlap/input-freeze guards and two focused
tests were added; those final additions passed the focused suite. It was not
repeated because its 35 compiler launches plus 19 experiments consumed 54/60.
The PowerShell capture shell returned 1 on expected argparse stderr from fixture
tests; the unittest log itself contains an unambiguous 468-test OK summary.
Tests exercise promotion code only in synthetic fixture roots; no canonical
promotion command or recovery mutation was performed.

Decisions are deterministic for frozen inputs; fresh compiler receipts retain
random scratch paths. Identity compile metadata uses timestamp-independent COFF
unit inventory and include hashes. Existing census objects were absent; recorded
census JSON alone was not treated as a reusable object or binding evidence.
The existing accepted-record transfer experiment found 460 readable pinned rows,
201 module/symbol records, and zero of the 399 blocker identities. Other records
failed the existing snapshot guard (390 outside-root historical paths, five
changed manifest pins); none were silently normalized or repaired.

All work outputs are disposable measurements. This design is method documentation,
not binding evidence. Integrator review and a broader independently closed class
(a) are needed before lane use. The prototype does not implement a reviewed
manual NEAR schema, full caller argument/type analysis, or alias/thunk identities.

## Integrator audit of accepted callee identities (same day)

`python scripts/identity_audit.py` (compiler-free) masks every 4-byte operand field of
each callee span bound by an accepted proof and searches its module for the same
masked shape. 241 of 324 identity records are unique. Of the 83 duplicates, 74
callees are themselves accepted rows, whose own references are closed. One
(`_eng3_slot_2ac`, ENG3 0x1ffa0) is confirmed by its own slot operand (root+0x2ac;
its twin at 0x1f8e0 uses +0x20). The remaining **8 HERCULES.EXE identities** are
not closed under this method: CGamePlayPage, COptionsSheet, CHerculesApp and
CHostRestartDialog destructors, `$E121`, two host callback setters and operator
delete at 0x11b00. Their masked twins differ only in relocated operands such as
vtables, `_free` and callback cells. **14 accepted rows** use them. Closing
those outgoing references, or narrowing the rows' claims, is the open action.
