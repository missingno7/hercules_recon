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
