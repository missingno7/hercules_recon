# Upload producer domain bounds

This bounded static audit finds two distinct cases. ENG3's split-upload object
state is initialized to zero, then an event command can replace it from a signed
16-bit resource operand before the split consumer reads its low WORD. TITLE's two
observed context-y reads are each preceded in the same local code path by a setter
call whose second argument is `0x100`. Loaded-resource RECT values remain
unbounded by inspected evidence. These findings do not establish runtime event
contents or scheduler order; the same initial TITLE/HERC context pointer is
directly proven below.

The prediction was that ENG3 state and resource RECT words might be data-driven,
while TITLE might have a local fixed y writer. The falsifier was a complete local
range check, a TITLE read without an earlier fixed setter, or an enforced resource
descriptor range. The bounded evidence partly supports that prediction: ENG3 has
an unchecked signed-WORD assignment; TITLE's two local reads follow the `0x100`
setter; no descriptor range schema was established. The prediction and falsifier are retained in the published receipt.

ENG3 constructor code at RVA `0x78f0` seeds object+`0x4c` with zero and stores
the resource-derived data pointer at +`0x64`. The event interpreter reads its
event word from event-record+2, sign-extends it, and writes it to object+`0x4c`
for command `-0x20` (jump-table slot RVA `0x1df74`). The split consumer at
`0x7c5b` uses the value twice:

| Rectangle | RECT word +0 | RECT word +2 | RECT word +4 | RECT word +6 |
|---|---:|---|---:|---|
| First | 127 | `(uint16_t)(state + 0x200)` | 127 | `(uint16_t)(0x100 - state)` |
| Second | 127 | `(uint16_t)state` | 127 | 256 |

After both upload calls, it assigns `state = (state + object[0x21]) & 0xff`.
That post-upload byte mask does not constrain the current upload. The command
operand can encode any signed 16-bit value, which covers every low-WORD pattern
when consumed, but actual resource commands and whether a particular event runs
before a particular draw remain unresolved. Thus state zero would make the first
rectangle's y equal 512 if the split consumer runs before another event changes
the seed; it is a conditional producer case, not proof of runtime order. Values
outside the A710 address helper's proven y range 512..1023 have no address
formula claim here. The table lists raw 16-bit RECT fields; it does not assign
pixel or inclusive/exclusive dimension semantics to width and height.

The split consumer is reached from the object-list path at ENG3 RVA `0x8c07`.
The separate event interpreter has many direct callers and stores the same
object-relative field, but the static evidence does not bind those callers to
the same object instance or establish event/draw chronology. The already-proven
fixed high-y callers are the ENG1 cold-start panel and ENG3 startup panel; each
passes the literal `{x=0,y=512,w=128,h=512}` in the phase-5
[producer receipt](texture_producer_domains.md).

For TITLE, the copied interface table's slot+`0x4c` wrapper at RVA `0xc370`
forwards only its first two caller arguments. HERCULES.EXE callback slot 19 is
the table dword at RVA `0x36204`, pointing to host RVA `0x1bd0`. At the host
store to context+`0x138`, the exact prologue and stack movement resolve
`[ESP+0x20]` to host-entry ESP+`0x14`: the original caller's second argument.
The HERC caller at RVA `0x5f03` passes its static interface-table pointer from
VA `0x4365a0`, initially `0x4361b8`, into TITLE Entry. The table word1 at
`0x4361bc` is initially `0x615c98`; HERC host RVA `0x1bd0` rereads that exact
word1 cell as its context pointer. TITLE Entry copies 1000 bytes from the passed
table to `0x1002bb40`, so its word1 at `0x1002bb44` receives the same initial
`0x615c98` pointer. The copied slot+`0x4c` also points to HERC RVA `0x1bd0`.
The file-backed HERC `.text` linear sweep finds no direct absolute write to
`0x4361bc`; indirect writes are outside this bounded scan. This pins the same
initial context pointer for the identified TITLE calls, with that residual
indirect-write caveat.

TITLE calls this setter with `(0x140, 0x100, -4, -4, 0x100)` at RVAs `0x16fff`
and `0x173a7`, then reads context+`0x138` at `0x17032` and `0x173e0` and builds
a RECT with raw fields `{x=0x390,w=0x50,h=0xc8}` before calling its rectangle
upload route. Under the pinned initial table pointer, each local sequence sets
and reads the same context field, yielding y=`0x100` for these calls.
The third setter call at `0x1641f` passes second argument zero and has no later
context+`0x138` read in that local sequence. The third setter does not supply an
observed upload y.

ENG1 and ENG3 also contain calls to their slot+`0x4c` wrappers with pointer
literals as the second argument. Examples are ENG1 RVAs `0x35de`, `0xe7e3`,
`0xec4b` and ENG3 RVAs `0x86ae`, `0x1a95d`. If those calls use the same host
callback and context, the host stores the literal's low WORD. They do not
establish that a TITLE context read follows those module calls or upload uses
their values.

ENG3's resource-rectangle parser copies four WORD fields from loaded records
into the upload RECT and uses signed width/height in source-offset arithmetic.
The inspected native path has no x/y clipping or RECT range check. The ENG1
behavioral source draft shows the same four-word descriptor pattern, but neither
source nor parser evidence identifies actual resource values. No resource bytes
were extracted, and no finite descriptor range is claimed.

The machine-readable receipt records all original spans as explicit
`{module,rva,size,sha256,label}` rows in [`evidence/upload_domain_bounds.json`](evidence/upload_domain_bounds.json).
The verifier pins the four original images, public PE helper hashes, source/evidence
identities, direct call/read/write sites, and file-backed instruction spans. Its
Capstone sweep covers each file-backed `.text` linear stream only; overlapping
streams, computed calls, non-file-backed code, runtime state and resource values
remain outside scope.

## Reproduction

Run from PowerShell with the checkout that contains `scripts/pe.py` and
`renderer_contract`. Original inputs are read-only; the receipt must stay under
that checkout's `work` directory.

```powershell
$checkout = 'D:\Prog\hercules_recon\work\renderer_interface_repo'
$python = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$env:PYTHONDONTWRITEBYTECODE = '1'
& $python -B (Join-Path $checkout 'renderer_contract\tests\audit_upload_domain_bounds.py') `
  --original-root $checkout `
  --exe 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE' `
  --image-dir 'D:\Prog\hercules_recon\assets\pc\HERCULES' `
  --output (Join-Path $checkout 'work\replay\upload_domain_bounds.json')
```

This audit is read-only against the original images. It does not execute the
game or extract resources and writes only the bounded JSON receipt under `work`.
