# Host pool and actor factory continuation

The accepted total is **40 instances / 4,352 bytes**. The new match is the host
diagnostic at immutable CD EXE RVA `0x63c0`: its observed body is RET followed by
15 natural NOPs. An ordinary empty variadic C function reproduces all 16 bytes
without relocations. The unchanged promotion gate freshly compiled all 40
functions; every previous recovery row remains unchanged. The original name,
prototype, return type and TU/library identity are unproved. The actual empty body
was recovered; it was not invented to satisfy allocator dependencies.

## Host pool evidence and first source

`evidence/host_pool_layout.json` records four pools, an allocated two-word header
and owner-slot tracking. Word zero stores a total dword count in its low 30 bits,
with free and pinned flags above it; allocated word one stores the caller's
pointer-slot address. A residual free block can be one word. Allocation scans the
whole selected pool, chooses the **last fitting block**, and splits from its high
end. The default interface supplies a static three-megabyte arena. These are
project/platform pool operations; no CRT/MFC identity was established.

The five-function ordinary-C experiment in `calibration/host_pool.c` preserves
request rounding, flag behavior, owner updates, initialization markers, splitting
and coalescing in the historical x86 fixture. It remains experimental:

| Function | Full target/candidate bytes | Normalized differences |
|---|---:|---:|
| Save arena | 32 / 32 | 0; two unresolved relocations |
| Initialize pool | 96 / 96 | 59 |
| Allocate | 288 / 288 | 264 |
| Free | 112 / 112 | 80 |
| Coalesce | 96 / 80 | 72 |

The 57-check pool fixture covers all selectors, last fit, high-end and exact
splits, one-word free blocks, request rounding/wrap, pinned and preserved-output
flags, failure, free and owner clearing. Two initial fixture mistakes were
corrected: failure diagnostics were undercounted, and writing the union's
`owner_slot` member overwrote the one-word block's descriptor. The source stayed
unchanged through those corrections. A single explicit masked-stride variant
then emitted an extra AND and worsened coalesce code generation; that family is
stopped. Descriptor-scaled traversal reflects historical 32-bit address behavior,
not a portable-language or original-declaration proof. No capacity or bounds fix
was applied to the game algorithm.

The fixed-base host EXE has no PE base relocations in these spans. The diagnostic
mapper now collects reviewed encoded absolute memory operands and MOV/PUSH address
literals inside mapped image sections. Register-relative offsets, arithmetic and
comparison literals, and unmapped values are excluded. Literal reference evidence
does not prove pointer semantics. Actual COFF member addends are still audited;
no operand is masked, no object is patched, and raw acceptance is unchanged.

## Actor factories

The allocator consumers inspect only kind bit `0x8000`; record-based factories
define BX without establishing its register's upper half. This supports a shared
16-bit `ActorKind` hypothesis. The original full-32-bit mock assertion imposed
semantics absent from the real consumers and was replaced with checks of every
low-word value, bit-15 pool selection, field flags and call order.

`calibration/actor_factories.c` now reproduces both complete sizes, three-block
CFGs and helper call order. The remaining four bytes in normal creation and two
in alternate creation are scale-one SIB base/index reversals on commutative
descriptor addresses. They remain actual byte differences; neither function is
accepted. The fixture passes **131,090 checks**, including all 65,536 kind values
through both factories, coordinate extremes/copies, descriptors, allocation/init
order and failures. Its synthetic array dimensions do not establish real table
capacity. The family stops pending independent source abstraction/TU evidence,
rather than pointer-addition spelling changes.

One separate explicit empty-return allocator-initializer hypothesis was tested
once and failed: it duplicated the first call and grew to 112 bytes. The previous
96-byte residual stays frozen. Earlier reasoning that its empty return restores
only EDI was incorrect: all target paths restore both EDI and ESI, with different
epilogue ordering. This correction is recorded with the failed discriminator.

## Verification and remaining work

Fresh integration compiles reproduce the pool and factory measurements. All
**20 semantic suites / 2,987,075 checks**, **30 Python tests**, and the immutable
original checks pass. `python scripts/host_pool_evidence.py` rechecks 21 reviewed
code spans and the default host table across two images. No original image was
executed. Source and proof remain separate: `recovery.json` alone records accepted
functions, while region packets preserve experimental recipes and failed-family
limits.

Natural module closure remains incomplete. The allocator/free/coalesce control
forms, pool compaction/reallocation lifecycle, actor pool providers and initializers,
real data/link contributions and existing cleanup residuals still need recovery.
Equal contributions remain frozen. The long-run goal stays active.
