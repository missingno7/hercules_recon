# Actor pool and resource data dependencies

The accepted total remains **40 instances / 4,352 bytes**. Four real actor
dependencies and resource data definitions are now measured; none passed a new
function raw gate.

The middle allocator occupies `ENG1:0x275d0..0x27720` (336 bytes). It scans
236-byte records by kind word at +0x2e, selecting a partition with bit 0x8000,
then wrapping to its lower bound. It writes the selected cursor without reserving
the slot. Its ordinary-C candidate is 352 bytes with a different CFG. The three
80-byte reset targets process normal, middle and alternate pools at strides
308, 236 and 148, using runtime counts; those counts do not establish capacities.

Direct PC rereading corrected the reset behavior: every iteration reloads the
pool base and applies the accumulated stride. The first candidate cached the
base. Both versions capture the current actor for the removal and subsequent
initializer/clear; only the next iteration uses a changed base. The corrected
source passes 39 callback-mutation checks across all three pools and both helper
outcomes. The cached-base negative control fails twelve corresponding assertions.
The original 26 allocation/reset cases remain registered and pass against the
correction. Its reset contributions are 96 versus 80 bytes and remain nonexact;
the larger result is retained because the earlier source omitted PC behavior.
All six allocator/initializer peers keep identical complete bytes and relocations.

Reader-backed views for scalar/flags at +0x4c/+0x50/+0x54 and actor pointers at
+0x5c/+0x60 retain the 148/236/308-byte layouts and all five previous contribution
identities. The 262,408-case initializer/allocation fixture passed. Those views
improve source consistency but do not explain the code-generation residuals;
original names/declarations and inheritance remain unproved.

Resource allocation evidence now distinguishes the prefix from the complete
blob. Prefix bytes are `4*(entry_count+1)`; the actual request is
`((file_length+prefix)&0xfffff800)+0x1000`, with 32-bit arithmetic and flag 0x20.
Both loaders write file bytes at `buffer+prefix`; metadata begins at
`buffer+prefix+4`. The paired-short prefix view remains valid. Its previous
allocation-size description was incorrect; no capacity or slack purpose follows
from the corrected formula. Release at `0x51d0` has no calls, and the static audit
found no new supported release-source discriminator.

`calibration/resource_data.c` supplies reader-compatible ordinary-C definitions:
81 physical 20-byte resource rows with local strings, the 16-byte signed map,
and 128/3,072-byte token/link zero-storage views. Fresh historical compilation
verifies all nonrelocated row fields, all 81 actual string referents including
NUL, the map bytes and common-symbol extents without patching an object.
`python scripts/resource_data_evidence.py` reproduces this audit. Leading link
extent is not a physical-capacity or original-declaration claim.

Row zero is supported as a shared one-byte empty-string COMDAT, rather than an
assumed mutable buffer. A 2,034-byte historical `getqloc.obj` data contribution
matches its nonpointer fields and all 186 string referents; two empty aliases
use the same decorated COMDAT symbol emitted by the candidate. The game reads
this empty string through a formatter into separate storage. No writer was
established. Final alias addresses and module layout remain unproved.

The consistent reverse-copy prototype takes void pointers and a U32 word count.
Rounding is `(S32)(byte_count+3UL)>>2`: unsigned wrap precedes signed interpretation.
It preserves frozen-v2 code, passes 1,772 behavior checks and eleven historical
compiler boundary cases, and needs no positive count assumption or large buffers.
The count unit now shares the full 48-byte state/two-member union; shrink uses
the pinned `string.h` declaration. Both declaration-only changes preserve code.

`python scripts/host_pool_full_linkage.py` freshly links thirteen reconstructed
functions, seven observed strings and the 48-byte zero-filled state with no
unresolved externals, aliases, wrappers, padding or placement flags. All twelve
experimental pre-link identities remain unchanged; the accepted diagnostic is
still exact. The explicitly supplied pinned LIBC copy provider is an experiment
choice, not proof of the original CRT variant. Only the diagnostic's linked
function bytes equal the target; this is dependency evidence, not module closure.

The original game files remain immutable, generated outputs stay ignored, and
`recovery.json` remains the only function acceptance authority. The long-run goal
continues through the remaining source/control and real module dependencies.

Final verification: all **40 raw matches**, **29 semantic suites / 3,251,611
checks**, **30 Python tests**, and **12 immutable original-file hashes** pass.
The cached-base negative control fails its twelve expected assertions. Source
line-ending normalization was followed by fresh contribution checks and a full
natural relink; no accepted source or recovery state was changed.
