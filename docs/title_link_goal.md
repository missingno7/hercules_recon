# Long-run goal: TITLE.DLL by natural link

Set 2026-10-09 after the leaf harvest exhausted the relocation-free pool.

## Why this goal

The raw gate rejects any contribution carrying an object relocation: every call,
global access, string or jump table. That is the large majority of first-party
code. Such functions can only reach raw equality inside a link that places every
symbol at its original address, without placement controls. The project needs one
module where that happens, and TITLE.DLL is the smallest measured candidate.

| Module | `.text` | Library (masked LIBCMT/MFC map) | First-party spans |
|---|---:|---:|---:|
| TITLE.DLL | 130,287 | 13,565 | 360 (116,656 bytes to `0x1d7b0`) |
| ENG3.DLL | 199,692 | 31,255 | 578 |
| ENG1.DLL | 306,908 | 23,669 | 1,180 |
| HERCULES.EXE | 183,742 | 97,074 | 230 (MFC-heavy, no base relocations) |

TITLE's first-party code is one block before LIBCMT (`rand` at `0x1d7b0` begins the
library), consistent with user objects linking first. It shares about 12.5 KB of
address-masked code with ENG1 (10 KB with ENG3), so engine-core work transfers.
The only export is `PC_DLLEngineMain` (`0xc1f0`), which copies the 250-word engine
interface to `0x1002bb40`. `.rdata` is 925 bytes and initialized `.data` 29 KB.
The 15 unreferenced empty functions retained in `.text` suggest that unreferenced
code was not stripped. That matters for reproducing the link.

## Stages

1. **Masked reconstruction (current).** Each first-party function is recovered in C
   until all non-relocation bytes are equal and every relocation implies one
   consistent address per symbol (`scripts/masked.py`). Rows are `CODEGEN_SIMILAR`
   with their implied `link_symbols`. Sources live in `calibration/title/` region
   files and are verified in place (`scripts/similar.py verify`). Names are
   mechanical: `title_<rva>`, `g_<rva>`, `g_engine_interface.slot_<off>`
   (`calibration/title/title_engine.h`).
2. **Symbol and data closure.** Merge the implied maps into one address table,
   reconcile object bases, and recover `.data`/`.rdata` contributions (strings,
   tables, initialized globals) and LIBCMT member identities.
3. **Natural link.** Determine object order and link flags from evidence, link with
   pinned LINK 5.00.7022 and LIBCMT, and measure function, section and image
   equality.
4. **Linked acceptance.** Expanding the raw gate to naturally linked contributions
   is a policy decision for the project owner. It will be proposed with measured
   evidence, never assumed. Until then, linked equality stays diagnostic.

## Prediction for stage 1, wave 1

At least half of the sub-1 KB first-party functions in wave-1 lanes reach masked
equality with ordinary C under RTM `/O2`. Falsifier: under a quarter, or exactness
needing flags or non-ordinary constructs. Functions of 1 KB or more (23 spans,
53,664 bytes) are deferred to integrator-led work.

## Tooling

- `scripts/library_map.py`: relocation-masked LIBCMT/MFC contribution map.
- `scripts/function_map.py MODULE`: first-party span leads with jump-table-aware starts.
- `scripts/leaf.py packet|data|check`: worker context; `check` reports masked equality.
- `scripts/similar.py stage|verify`: integrator staging and regression of masked rows.
- `work/title_lanes/BRIEF.md` (ignored scratch): the worker brief and lane lists.

## Finding: TITLE mixes C and C++ translation units

The most frequent near-miss, base/index operand order in `[global + index]` addresses, is a
front-end effect. In a minimal sweep of nine ordinary spellings, VC5's C++ front end always
uses the loaded global pointer as the SIB base and the C front end never does. Functions
`0x8a50` and `0x8ed0` are masked-equal only as C++ (`.cpp` region files with an `extern "C"`
block to keep link names). Function `0x65e0` matches only as C: the C front end turns a
constant `strcpy` into immediate stores. Language is therefore a per-unit discriminator.
It also bears on translation-unit boundaries. Every compiler revision on hand (RTM, SP2,
NT5 SDK builds, SP3) behaves identically in both front ends.

## Finding: screen handlers read volatile context fields

TITLE's screen handlers begin with `mov cx,[ctx+0x5c]; mov dx,[ctx+0x5e]` and never use the
values. VC5 removes such reads in every non-volatile form tried: plain locals, struct copies,
inline helpers, constructors, references and macros, in C and C++. Declaring the two context
fields `volatile` reproduces the sequence exactly, which is consistent with host-updated input
state. This is the only sanctioned use of `volatile` in TITLE reconstruction. Its evidence is
the unremovable dead load itself; it is not used to steer register allocation. The out-handle
these handlers keep in the caller's argument area is an ordinary local. VC5 places it there
itself when the request is a single typed pointer parameter. `0x128e0` and `0x14300` are
masked-equal with this shape.

Project-owner ruling (2026-10-09): the `volatile` declarations of the two context fields at
`+0x5c` and `+0x5e` are approved as a narrowly scoped, evidence-supported reconstruction
hypothesis. The original performs reads whose results are unused, and no non-volatile
construct reproduces them under the pinned compiler. This is not proof of the original
declaration. The exception does not generalize, and `volatile` must never be used to
influence register allocation.

## Finding: callee-saved priority follows live source references

This is the B6 allocation probe, recorded in `evidence/title_stage1.json`. A cut-down pair of
the 0x10670 lead has the same shape but swaps the roles of `buttons` and `out`. One added live
reference flips the pair. Weights count source references before the compiler merges identical
tails. References in unreachable code don't count. References in a switch `default` count for
less. References inside an `if` count fully. Translation-unit context has no effect.

Three sibling handlers use three different role permutations in the original. Under this rule,
the original 0x10670 must carry more live `buttons` weight than its two visible tests provide.
The investigation stopped at its bound with that as the re-entry condition.

## Adopted from blood2_recon: translation-unit symbol count

blood2_recon measured that VC5 C++ code generation depends on how many symbols the TU declares
(`../blood2_recon/docs/object_residue.md`). The affected code is register choice, operand order
and scheduling, and function size stays the same. One declaration counts 1, a prototype with a
named parameter counts 2, and each opened file counts 1. `scripts/count_scan.py` ports their
diagnostic count scan. Results are in `evidence/title_stage1.json` (`context_count_scan`):

- **The tested C near-misses are insensitive.** B1 in 0x1de0, B2, B6 in 0x10670 and the
  others keep their residue at every count, so they are source-form questions. C code is not
  immune, though: staged 0x17bf0 is count-sensitive, so any header change needs a full
  masked verify. Its "C-only" language claim was withdrawn for that reason.
- **C++ units are sensitive.** In 0x8770 each `[base+index]` order flips at a count threshold,
  so C++ near-misses must be judged at a realistic count. The real headers set that count.
- **Staged C++ rows are robust.** All 19 masked-equal C++ rows hold for every count in 0..255.

Rules carried over:

- Padding is a diagnostic only, never a fix.
- Equal size with only register or operand-order differences means "source up to TU context",
  not a match.
- Promote a context change only with real evidence, such as a real header or include.

Other blood2 principles that apply to the natural link:

- **Link order.** msdev links in `.dsp` order, which is usually alphabetical by file name.
- **Identical COMDAT folding.** One address can carry several names, so callee identity is a set
  of names.
- **`/YX`.** It shifts the count of C++ TUs.
- **Rebuild check.** A second clean build must reproduce the objects and the image byte for byte.

## First data unit: the file-name lists (`title_files.c`)

The pointer lists at 0x22288 and their strings now live in one C data unit with four arrays:
engine paths per level, screen files, sequence files and language files. The users index
these arrays instead of 16 per-entry pseudo-globals. Evidence is in `evidence/title_closure.json`
(`name_tables.experiment`).

- **Proven by bytes:** the unit reproduces the 4064-byte object exactly when linked, both
  alone and in the full diagnostic link. That covers every pointer slot by relative target and
  every string, NULL entry and padding byte. Every user still matches masked.
- **Proven by measured linker behaviour:**
  - Each object contributes its ordinary data first, then its string literals.
  - Literals of one initializer are emitted in reverse order.
  - The lists' strings follow the lists directly, after the trace strings of code up to 0x5bc0.

  So the lists are a separate object, linked before the unit starting at 0x5e90.
- **Hypothesis:** the split into four arrays, their names, and whether the object also owns code
  without literals.

`calibration/title/data_units.json` lists data-only units and their link position.
`scripts/title_link.py` links in code order. It reports natural function RVAs, section sizes and
each data unit's placement and linked bytes. The forced image used for layout is never accepted.

## Unit boundaries from data order (2026-10-09)

Per object, LINK emits ordinary data first and the object's literals last. So a literal block
ends an object, and per-object .bss blocks follow link order. `scripts/object_map.py` segments
.data this way. It can't see data that no code references, so its segments are leads.

The measured boundaries are recorded in `evidence/title_closure.json` (`units`):

- **After 0x5c60.** The sound-effect unit's .bss and literals end here.
- **0x5cc0 to 0x6350 is a C++ process scheduler (`title_proc.cpp`).** Two of its .bss variables
  are read across the whole DLL. Only 2 of 525 per-object .bss addresses are shared like that,
  and a C unit could provide them only as communals. The unit defines its six used variables.
- **From 0x6350 is a C unit (`u06350.c`, 0x65e0 is C-only).** It reaches at least to 0x6a50.

Rule measured along the way: within one object, the .bss order of C statics and C++ globals
follows the identifiers only, by a function not yet identified. The scheduler keeps 4 of its 6
variables at their original relative offsets. Three unreferenced 4-byte slots are not invented.

## Units consolidated so far

`scripts/unit_bounds.py` turns per-object .bss sharing into minimal unit intervals. A C
file-static ties its users to one object, and one object's .bss is contiguous.

- **Sequence screens:** `seq1.c` .. `seq10.c` hold ten units, each a handler and two helpers.
  The handlers load SEQ1, SEQ10, SEQ2..SEQ9 in code order, which is alphabetical file order.
- **Other units:**
  - `u197f0.c` (0x197f0..0x1c430);
  - `u0ff70.c` (0xff70..0x10fa0);
  - `u0cc90.c` (0xcc90..0xd6c0);
  - `u09a40.c` (0x9a40..0xc140);
  - `u06350.c` (0x6350..0x6840);
  - `u06bb0.c` (0x6bb0..0x71e0);
  - the C++ scheduler `title_proc.cpp`.
- **The engine unit `u0c1f0.c`.** It holds the export and 117 interface-slot wrappers, and it
  tentatively defines the engine interface.
- **The C++ object-system unit `u08140.cpp`.** It defines the .bss block it shares with
  `u06bb0.c`. Per-object variables read by other units need a C++ definer.
- **Statics:** each unit's private .bss variables are file statics. Per-object .bss starts at
  0x290e8.
- **Shared types:**
  - `title_proc.h`: the process record, where a handler's argument is its process;
  - `title_screen.h`: the screen context and the sprite object, whose x and y are signed 16.16.

The diagnostic link has gone from 433 unresolved symbols to 108 (34 functions and 74 data),
with no duplicates. `title_link.py` reports in-object offsets for initialized data, per-object
.bss and communal data separately.

**In-object order (L1, rule recovered).** VC5 orders the uninitialized variables of one object
by the bucket of the c1 identifier hash: `h = (h >> 4) + 4*h + c`, folded `h ^ (h >> 16)`, 1024
buckets. Within a bucket, the later-defined name comes first. `scripts/symbol_order.py`
implements it and `tests/test_symbol_order.py` covers it; it is exact on every probe.
Initialized data keeps definition order. Reproducing the original in-object order now depends
only on the identifier names, which is a naming-policy decision.

## Shared records and data units (2026-10-09, continued)

Records recovered from cross-file evidence (details in `evidence/title_closure.json`):

- **Display double buffer (`title_gpu.h`).** `g_2cc08` holds two 0x1478-byte buffers and
  `g_2cc04` the one being drawn. Every touched field recurs 0x1478 bytes apart. The layout is
  the PSX libgpu one: DRAWENV (0x5c), DISPENV (0x14), and a 0x500-entry ordering table at +0x70.
  Five per-file views were replaced. `r17ad0.c` keeps two unused declarations because 0x17bf0
  is TU-count sensitive.
- **Object record (`title_screen.h`).** The 0x134-byte pooled object is now the union of eleven
  per-file views. All their rows stay masked equal, so the conflicting signedness in the old
  views was not load-bearing.
- **Not an aggregate.** The communal gap before 0x2cc00 also admits unreferenced tentative
  definitions. The would-be fields have unrelated meanings, so they stay separate globals.

New data units:

| Unit | Content | Linked result |
|---|---|---|
| `title_levels.c` (data-only C) | Unreferenced level table at 0x234b0; its literals are the first copies of the strings r16110's engine table points to | 424 bytes, 36 pointers |
| `u06350.c` | Icon palette and pixels, plus the unit's literals | 324 bytes |
| `u09a40.c` | Button maps and eleven frame tables | 740 bytes |

Every measured data unit now links byte-equal. `title_link.py` compares pointer slots by place:
the same offset in any measured unit, or the same reconstructed function. Consecutive units
now differ only by missing contributions, such as 0x5fb0's two literals.

Functions retried in their real unit context:

- **Now masked equal:** 0x5cc0 and 0x5e90 in the C++ scheduler.
- **Characterized and stopped:**
  - 0x5db0 (30 bytes): argument push placement.
  - 0x5fb0 (B2, 7 bytes).
  - 0x66f0 (10 bytes): it is C in its unit, which supersedes the "needs C++" note.
  - 0x9a40 and 0xa020: the frame-table state machines keep a per-case register rotation. It
    is insensitive to flags, language, TU count, prototypes and loop spelling.

## Preliminary closure (stage 2 groundwork)

`scripts/closure.py` maps the data addresses implied by masked rows back to their functions.
Uninitialized data up to about `0x2b360` follows code order, consistent with per-object
contributions in link order. From about `0x2b370` it is shared and out of order, consistent
with communal C globals placed after all objects. The engine interface and the ordering
tables sit there. Screen-handler groups own contiguous `.bss` blocks. These are hypotheses
for unit boundaries and link order, recorded with alternatives in
`evidence/title_closure.json`. Two interior references to interface fields were replaced by
the fields themselves, and the symbol check now rejects that pattern.
