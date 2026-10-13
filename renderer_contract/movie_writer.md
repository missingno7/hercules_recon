# TITLE movie frame writer: static contract candidate

The pinned direct route is TITLE `0x9980` → wrapper `0xC9E0` → interface slot
148 → HERC `0x5450`. TITLE supplies a movie identifier and one raw control
word. HERC opens a numbered ETV resource, obtains its frame storage and
dimensions, decodes each frame, then copies indexed bytes into the currently
locked CPU-visible view. This is one statically observed writer path; it does
not establish exclusive screen ownership or a complete runtime route inventory.

The pre-family prediction is preserved unchanged in
`fixtures/movie_writer/prediction.json`. It was **partly falsified**: it
predicted that TITLE supplied frame and palette pointers. The pinned TITLE
caller forwards its first formal and fourth raw stack formal, while HERC opens
the resource and supplies the pixel pointer, dimensions, and optional palette
pointer itself. The prediction's falsifier and original hash provenance remain
in the fixture and receipt.

## Static caller and resource path

TITLE `0x9980` calls `0xC9E0` at `0x9A25`. The first value comes from TITLE's
first formal; the second is loaded from entry stack offset `+0x10` after the
function's two saved registers. The wrapper pushes both unchanged in logical
order and calls the cell at `0x2BD90`, offset `0x250` from the interface base
`0x2BB40`. The pinned HERC source-table cell `0x36408` contains VA `0x405450`,
the slot-148 target. These facts come from the pinned caller, wrapper, and
table bytes; they do not depend on callback-name labels.

HERC `0x5450` uses its first incoming value to format the original literal
`e:\data\%d.etv` and passes the second later to internal helper `0x1F60`. The
new static audit does not infer a complete callback prototype or semantics for
that second value. The older external file
`evidence/movie_callback_protocol_2026_10_11.json` labels it
`input_abort_mask`; that label is **historical provenance only**. The
candidate records that file's path and SHA-256 as historical context but
never reads or requires it. Its own finding is only the raw stack/dataflow
shape.

The formatted path is not opened unchanged. At `0x547D`, `0x5450` passes the
same local buffer to HERC helper `0x5350`. The complete helper span
`0x5350+0xFB` is pinned. It overwrites buffer byte zero with the preferred
byte at global VA `0x91C444` (`0x53F0`), probes the path through internal
`0x4113D0`, and can then overwrite byte zero with entries from candidate table
`0x91C018` (`0x541E`). The candidate scan is gated by global byte `0x436634`;
it builds candidates from the `GetLogicalDrives` mask and calls
`GetDriveTypeA`, retaining the raw return value `5`. HERC `0x3140` receives
that same buffer pointer and passes it to `KERNEL32!CreateFileA` without an
observed intervening rewrite.

The preferred byte's source and the ultimately selected drive are unresolved.
The original host path lookup/open is a resource service to preserve; this
static candidate does not reconstruct filesystem behavior or execute native
filesystem calls. The format literal `e:` therefore does not establish a
runtime `E:` path.

At `0x54D8`, HERC calls `0x3140` with the post-`0x5350` path, sound-device
global `0xB9E440`, and output slots for pixel pointer, width, and height. The
bounded helper imports `KERNEL32!CreateFileA` and `ReadFile`, reads `0x348` bytes,
checks magic `0x0A565445`, and publishes pixel address `0x601B88`, width from
header `+4`, height from header `+8`, and frame count from header `+0xC`. Its
open path clears `0x14000` bytes at that address; that clear length is not a
proven maximum frame capacity.

For a frame step, `0x5450` calls `0x37A0` with a local optional-palette output
slot. `0x37A0` either clears the slot or points it at current chunk `+5`, then
calls internal helper `0x21E0` with destination `0x601B88`. The pixel pointer
is borrowed/reused global storage; no per-frame allocation or explicit lease
is observed. The complete decoder/container bounds and safe maximum extent
remain unresolved.

## Frame copy and palette order

After a positive frame step, HERC calls the surface lock helper `0x6FF0`. If
the lock byte `0xBA01F0` is set and destination base `0xBA0210` is non-null,
the function copies the frame from origin `(0,0)`. Source row bytes are the
file width; row count is the file height; destination pitch is `0xBA0214`.
When pitch equals width, the function copies `width * height` bytes
contiguously. Otherwise, each row copies `width` bytes from `source + y*width`
to `destination + y*0xBA0214`. The bytes are copied directly to the indexed
CPU-visible view: the pinned callback contains no call to remap helper
`0x70C0`, scaling, source offset, clipping check, or comparison against the
surface bounds. Exact safe bounds for file-provided dimensions are unresolved.

On the successful-copy route the order is copy → dirty byte `0x43A590 = 1` →
unlock `0x7140` → present `0x6620`. A missing lock or null view skips the copy
and dirty store but still reaches unlock and present. This callback writes the
CPU-visible view directly and bypasses packet slot 89/AC60. That establishes
independence from that packet route for this callback, not a separate
composition layer or sole ownership of the view.

If the optional palette pointer is present, `0x5450` consumes 256 entries of
three bytes each through `0x6780`, then calls `0x69A0(0)` before it locks and
copies the frame. The caller saves the current palette through `0x6810` before
playback, restores the saved triples through `0x6780` in cleanup, and calls
`0x69A0(1)`. The pinned `0x69A0` route first unlocks (`0x7140`), clears black
(`0x6EE0`), presents (`0x6620`), and then applies the mode-specific palette.
Therefore both movie palette transitions can also clear and present the host
view. No palette-pointer lease or cross-thread serialization is proven. The
message-pump path in `0x5450` leaves reentrant callbacks possible.

The bounded file/timing helper calls include `CreateFileA`, `ReadFile`,
`CreateThread`, `Sleep`, `CloseHandle`, `timeGetTime`, and `timeBeginPeriod`.
The decode call observed in this route is internal HERC `0x21E0`. This package
does not inspect or execute a video codec, MCI session, ETV file, window, GDI,
DirectDraw surface, sound device, or display. It makes no pixel or playback
claim.

## Evidence and reproduction

The runner produces a compact static receipt after validating the complete
original image hashes and exact code/data spans, including the new `0x5350`
path resolver pins. The earlier candidate receipt predates those pins and must
be regenerated before it is used for this report. The runner's only input
files are the explicitly supplied originals and prediction JSON; the receipt
output must be a new `.json` file under the supplied repository's `work`
directory. The script creates missing output parents there. It rejects
redirected paths, `assets` paths, output/input overlap, and an existing output.
It has no dependency on the original installation path, its private candidate
directory, or the historical protocol file.

Pinned images:

- `HERCULES.EXE`: `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`
- `TITLE.DLL`: `b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b`

The receipt pins HERC spans `0x3140+0x2a0`, `0x37A0+944`, `0x5350+0xFB`,
`0x5450+752`, `0x6810+0x39`, `0x69A0+178`, and callback table `0x361B8+1000`; TITLE spans
`0x9980+0xC0` and `0xC9E0+0x14`. It records selected observed instruction
bytes and imported endpoints, without embedding full function disassemblies.

After copying this package into the repository's top-level `renderer_contract`
directory, run the following from the repository root. It creates
`work/movie_writer-rerun.json` if needed; choose another fresh output filename
for each later run because the audit intentionally refuses to overwrite.

```powershell
$repo = (Resolve-Path '.').Path
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py -B "$repo\renderer_contract\tests\audit_movie_writer.py" `
  --repo-root $repo `
  --exe "$repo\work\discs\pc_install\HERCULES.EXE" `
  --title-dll "$repo\work\discs\pc_install\TITLE.DLL" `
  --prediction-json "$repo\renderer_contract\fixtures\movie_writer\prediction.json" `
  --analysis-deps 'C:\tools\hercules\python' `
  --output "$repo\work\movie_writer-rerun.json"
```

Python, Capstone, and pefile are analysis tools only; the receipt records the
versions used. No original is executed or modified.
