# Archive and virtual-handle dependencies

Thirteen ordinary-C dependencies covering **1,728 target bytes** are now measured:
four public-handle adapters, two position leaves, three archive backends,
initialization/shutdown, and normalization/hash. The accepted total remains
**41 instances / 4,368 bytes**. No new raw equality or module closure is claimed.

The four adapters and two position leaves match their complete contributions
after relocation normalization: **432 bytes across six functions**. Their source
is frozen. Public handles are one-based slot indices containing archive handles.
Validation uses the signed active count and an occupied slot. Open scans 16 slots;
seek reloads the slot after the backend call; close preserves the backend result,
reloads the count, and clears its captured slot. It does not repair the hole left
when a low public handle closes. The adapter fixture passes 2,528 checks; the
position fixture passes 23, including signed values and handles 1/1024.

Backend open uses unsigned keys and the target's unusual pivot order: its upper
half computes the next midpoint using the old lower bound before replacing that
bound with the old pivot. Read captures a position for physical `fseek`, reloads
the logical position afterward for clamping, and reloads again after `fread` for
advancement. A one-line cached-position negative control fails the expected
assertion; the corrected fixture passes 30. These hooks test ABI/read order and
do not assert that ordinary CRT calls mutate these globals. Seek preserves
unsigned clamping, including negative offsets clamping to the logical end.
All three backends retain substantial byte differences; no spelling grid follows.

Initialization is `0x5020..0x5110` (240 bytes); shutdown is `0x5110..0x5130`
(32 bytes). Splitting at `0x5120` cuts the shutdown body. The initializer clears
1024 leading directory rows and position words, opens `hercules.fs`, reads one
0x3000-byte block, counts leading nonzero keys, and checks strict unsigned ordering.
Shutdown closes a nonnull stream, clears it, and retains the observed close result
or zero. The original prototypes remain unproved. The 20-check fixture passes;
both complete contributions remain nonexact.

Normalization uses one shared scratch object, copying at offset 100 and then
rewriting/prepending within it. The source preserves aliasing, drive rewrites,
`O:FOO` losing its `F`, and short `C:` input scanning stale bytes after its new NUL.
The returned offset matters alongside string content. Scratch capacity remains
unproved. The real six-entry switch table occupies `0x4fbc..0x4fd4`; the preceding
two bytes are alignment and the following twelve are NOPs. Full raw comparison
retains that table; no linear CFG claim is made through its data. The candidate is
416 versus 464 bytes. Hashing sign-extends bytes into four cycling eight-bit lanes,
wraps at 32 bits, and adds byte count. Its 64-byte contribution has three actual
differences and no relocations. It remains unaccepted. The paired fixture passes
27 checks, and this source family is frozen.

The integration fixture links all thirteen reconstructed functions as separate
translation units with historical stdio. It creates a synthetic archive only in
a fresh ignored test directory. Fatal UI/termination and initial storage are
explicit fixture boundaries. Its 21 checks preserve two composition effects:
seek does not change the last-read-handle cache, so an interleaved same-handle read
can consume bytes at another file's physical stream position; closing a low handle
can leave an occupied higher slot outside the reduced active count. These effects
are predicted by the PC instructions and verified in reconstructed code. No
original executable, DLL, or game archive is run.

Data closure remains a real question. The initializer's next-key probe at the end
of the leading directory overlaps the first position word. Positions end at the
normalizer scratch base, whose offset-100 view aliases the same backing object.
Neither an independent 1025th row nor a fictional scratch capacity is introduced
to satisfy a linker. Physical adjacency does not prove the original aggregate or
declaration. The six host file-service contracts now expose the next 1,152-byte
slice, with callback arity, signed sector rounding, live cursor updates, and
completion/failure cleanup documented before another bounded family.

The fatal service's callee is historical CRT `exit`, not a formatter. Three pinned
LIBCMT `crt0dat.obj` contributions (256 bytes, 20 relocations) match after diagnostic
resolution. They will remain library code. This identifies selected members,
not the entire CRT, linker flags, or binary layout.

Verification passes **41 fresh raw matches**, **40 semantic suites / 3,647,669
checks**, **30 Python tests**, and **12 immutable original-file hashes**. Accepted
source and recovery state are unchanged; generated outputs and originals stay
ignored. Natural module closure and whole-binary equality remain open.


Current original-host continuation: the initializer at `0x5020` now uses its
global count as scan state. This reproduces the measured zero-before-scan and
final-count CPU stores that the local-count draft omitted. Nine fresh native
comparisons pass full measured directory/positions, stream/count, CRT/fatal
boundaries and ordered writes; the pinned archive prefix yields 821 rows.
The existing source lifecycle and composed pipeline fixtures pass 20 and 21
checks. The fresh 240-byte contribution still differs at 76 raw bytes, with
14 unmasked bytes and a relocation conflict, so the row is `RECONSTRUCTED`.
Earlier metrics above are historical. Current provenance and limits are in
`evidence/host_archive_initializer_recovery.json`.

The current native/source archive comparison executes the existing normalizer,
hash, search, virtual handle, seek/read and close code in 119 guarded cases.
It reads all 35 distinct MUSIC/SFX members reached through the recovered path
prefix, checks their complete bytes independently, and rejects six incorrect
variants. Interleaved seeks preserve the stale last-read cache; public handle
holes preserve validation against active count. These are now measured against
the original PC instructions, extending the earlier source-only composition test.

The connected loading runner passes 427 cases, including 23 original SFX and
12 original MUSIC members through actual archive code and actual pool allocation.
Correct bytes are not preloaded into the destination allocation. CRT stream
effects, formatting and sample-device entry remain modeled boundaries; synthetic
caller reload probes retain their explicit virtual I/O models. These experiments
execute in the offline instruction fixture, without starting the original game.

The three existing normalizer/open/seek source bodies are unchanged and newly
recorded as `RECONSTRUCTED`. They retain 289/93/85 diagnostic byte differences;
finite agreement grants no raw acceptance. All 780 previous recovery rows remain
identical, 28 prior game contributions retain complete bytes and relocations, and
all 134 accepted functions pass fresh raw verification. The natural selected host
graph has 149 units, 262 recorded EXE functions and the same seven missing data
providers. Production scratch/filename capacities and original source ownership
remain open. See `evidence/host_archive_pipeline_recovery.json`.

```
python scripts/host_archive_pipeline_differential.py --output work/archive_pipeline_replay
python scripts/host_music_differential.py --output work/archive_pipeline_callers_replay
```

This gives later matching work a connected behavioral constraint and frozen
contributions. It helps distinguish an algorithm error from remaining code
generation, dependency and layout questions without reopening exhausted source
spellings or treating relocation-normalized equality as an exact match.
