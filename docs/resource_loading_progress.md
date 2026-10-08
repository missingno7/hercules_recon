# Resource loading dependencies

The accepted total is now **41 instances / 4,368 bytes**. The separate ENG1
diagnostic at `0x1000` passed the fresh raw gate: an ordinary empty variadic
function emits the complete return plus 15 natural alignment NOPs. Promotion
freshly verified all 41 functions. Its original name, prototype and TU remain
unproved. Larger loading dependencies remain experimental; relocation-normalized
equality is diagnostic.

The 96-byte metadata reader at ENG1 `0x31080` matches after resolving its actual
resource and readiness references. It tests the full index argument for zero,
then sign-extends the low 16 bits for descriptor addressing. State 3 bypasses
readiness; other states require its success. The basis is the combined allocation
plus `4*entry_count+8`; a nonzero signed descriptor contributes an arithmetic
`>>8` relative offset. Its 43-check fixture covers boundary counts and indices,
state/helper behavior, zero descriptors, and positive/negative offsets. Neither
valid metadata capacity nor the complete file format follows from these reads.

Completion (`0x3810`, 32 bytes), failure (`0x3830`, 64 bytes), and conditional
reset (`0x3520`, 32 bytes) also match after relocation normalization. Reset changes
only state 0 to 1; completion changes only 2 to 3. Cancellation (`0x3870`) changes
1/2/3 to 4 and preserves other 16-bit states. For state 2 it clears runtime
callbacks before calling the host cancellation wrapper. The candidate merges a
branch that the target retains: 80 versus 96 bytes. The family is frozen pending
independent control/source evidence, not a case-label spelling search.

Failure rereads the global loading ID after cancellation and destruction. The
393,226-check fixture covers every 16-bit state and concrete ID mutations across
those callbacks. A cached-ID negative control fails the two expected assertions.
These mutations test ABI/read-order behavior; they are not claims about shipped
default callback behavior.

Destroy (`0x38d0`, 64 bytes) captures the indexed record, frees a nonnull buffer
through `0x2c2d0`, then clears the buffer, state, and auxiliary fields. The callback
sees the old values; its mutations to those fields are overwritten afterward.
All other fields are preserved. The 32-check fixture passes; raw and normalized
comparison each retain 49 differing bytes. `scripts/resource_destroy_evidence.py`
reproduces the full comparison with an independently evidenced physical table
base. The standard local map audit rejects that base because this target embeds
only member addresses. That limitation is explicit; the general auditor and raw
acceptance gate remain unchanged.

The shared runtime declaration is an observed 224-byte prefix, with signed sector
words at +0x9e/+0xa0 and completion/failure slots at +0xb0/+0xb4. It preserves all
four callback contribution identities. Its extent, names, and complete original
type remain unproved. Resource records remain compatible with the existing
20-byte data definitions.

Readiness (`0x3750`, 80 bytes) delegates state 2 to the availability scan and
returns its result. State 0 requests conditional reset and sets auxiliary 3;
other states return 0. The scan (`0x37a0`, 112 bytes) uses the difference of two
signed sector words times 2048. Below 4096 it returns before reading an indexed
resource. It searches entries `index+1` through `entry_count`, inclusive, for the
first nonzero signed `descriptor>>8` and compares that offset with available
bytes. The 20-check fixture includes a below-budget call with an out-of-range ID,
which proves the early path has no record dependency. The corrected source uses
the existing void reset declaration. Both functions remain nonexact: readiness
has 36 normalized differing bytes; the scan is 128 bytes with a different CFG.

The provider audit distinguishes captured request/prefix/ID values from later
global reads. A zero cached length is written through the live active-record
pointer after the length callback. The original record still supplies the
allocation output slot and later indexed state/auxiliary writes. Allocation is
retried once after reclamation, with the same request and flags. Async submission
returns -1 regardless of immediate host status; sync paths return 0 even when
loading fails. Only an upper signed ID bound is checked. No lower-bound repair,
extra capacity guard, or modernized error behavior is introduced. The runtime
context used for the tick and completion slot is captured after allocation and
row initialization, rather than at the entry-time guard.

The provider fixture passes 88 checks. Removing that post-allocation context
reload fails three assertions: the start tick, preservation of the old completion
slot, and installation in the new slot. The first candidate's expected values
were corrected from the PC's `0x3670` reload. The corrected candidate is 496 versus
528 bytes, with 471 raw and 473 normalized differing bytes and a different CFG.
This is a behavior correction and measured dependency, not an exact function.

The host file services use custom virtual archive handles. Adapters at
`0xa210/0xa280/0xa2f0/0xa350` call project backends at
`0x5130/0x51f0/0x5210/0x52c0/0x5340`; they are not CRT file functions. Directory
initialization reads `hercules.fs` into 12-byte key/offset/length rows and initializes
1024 logical positions. Handles are one-based. The actual lower I/O layer contains
eight identified complete VC5 LIBCMT sections totaling 880 bytes, with zero
normalized differences and 23 audited call relocations. This identifies selected
library contributions; it does not prove the complete CRT/link configuration.
Those routines will be supplied by historical libraries rather than rewritten.

The smallest new grounded archive functions are close (`0x51f0`, 32 bytes) and
position (`0x5340`, 16 bytes), sharing the logical-position data view. The coherent
file service and engine interface/reclamation functions remain missing. The
ordinary resource sources now expose these real dependencies without replacement
stubs, original-binary dispatch, or address placement tricks.

Next work recovers the readiness/provider implementations and their actual
interface/reclamation contributions. None of this proves historical TU ownership,
final addresses, original source names, or natural module closure.

Validation passes all 41 accepted raw contributions, 34 reconstructed-source
semantic suites / 3,645,020 checks, 30 Python tests, and 12 immutable original-file
hashes. The cached-ID and entry-context negative controls fail their two and three
expected assertions. Generated builds and original game files remain ignored.
