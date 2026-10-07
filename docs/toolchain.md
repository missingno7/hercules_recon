# Historical toolchain and calibration

VC5 is the best-supported compiler family. The current tested configuration is
the unmodified Visual C++ 5.0 RTM compiler with `/O2 /Gy`. SP2 also reproduces the
same four accepted functions, so the exact original compiler revision is **not
identified**. `/Gy` supplies unambiguous per-function COFF boundaries for this
pilot; matching those contributions does not establish the original project
setting or translation-unit boundaries.

## Measured evidence

All four PC modules record PE linker version **5.0**, with timestamps on
1997-08-20. The DLLs import only KERNEL32; HERCULES.EXE imports the Windows UI,
DirectDraw, DirectSound and WinMM APIs. None imports a CRT/MFC runtime DLL in the
analyzed import table. The EXE contains `AfxWnd42s`, `AfxControlBar42s`,
`AfxMDIFrame42s`, `AfxFrameOrView42s`, and `AfxOleControl42s` class strings, plus
C++ RTTI descriptors for `CWinApp`, `CWnd`, and `CException`. Together these
strongly support static MFC 4.2-family code; the exact library revision remains
unproved. All four modules contain Microsoft Visual C++ runtime error strings.
TITLE's TLS-based random-number implementation at RVA `0x1d7b0` uses the Microsoft
CRT recurrence `214013*x + 2531011`, extracting bits 16 through 30. Direct library
comparison now identifies the **static multithreaded CRT / LIBCMT (`/MT`) family**:
VC5 RTM and SP2 `mt_obj/rand.obj` reproduce its entire 48-byte contribution
apart from the single `__getptd` REL32 operand. The resolved destination,
RVA `0x1daf0`, also agrees with both libraries' 128-byte `__getptd` contribution
at every nonrelocation byte. Its imported GetLastError/TlsGetValue/TlsSetValue/
GetCurrentThreadId/SetLastError operands independently agree with the PE import
table. The corresponding 32-byte `__initptd` at RVA `0x1dad0` agrees apart from
its exception-table pointer. Single-threaded `LIBC` rand has 35 nonrelocation
byte differences and a global seed instead of per-thread state.

These three complete code contributions and their relocation records do not
distinguish RTM from SP2; the full archive-member hashes differ. This establishes
the runtime configuration without claiming an exact library patch revision or
rewriting CRT code as game source. Internal/data symbol identities remain
fingerprint inferences; no normalized library comparison grants FUNCTION_MATCH.
Hashes, relocation records, and extraction procedure are in `evidence/crt.json`.
Other addresses and strings are recorded in `evidence/source_map.json` and
`docs/architecture.md`.

The PE field is specifically a linker version, as documented by
[Microsoft's PE format specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format).
It neither proves CL's version nor identifies the linker build. RTM LINK
5.00.7022 is the initial linker candidate; no whole-DLL link match is claimed.
Image/file alignment is 4096/512 in TITLE.DLL. Its first-party code, library
contributions, and padding must be separated before reconstructing whole-image
layout. The CD/installed binary distinction is tracked in the evidence manifest;
the three engine/title DLL identities agree, while the installed EXE has a
one-byte alteration and is not interchangeable with the original CD EXE.

### Differential code generation

Run `python scripts/calibrate_control.py` to regenerate the diagnostic matrix in
`build/toolchain-calibration.json`; the initial result is preserved compactly in
`evidence/toolchain_calibration.json`. It freshly compiles two small source units
under both acquired toolchains. No oracle bytes enter compilation.

| Flags, in addition to `/c /Gy` | RTM exact pilot functions | SP2 exact pilot functions | Call/switch diagnostics equal after REL32 resolution |
| --- | ---: | ---: | ---: |
| `/O2` | 4/4 | 4/4 | 2/2 |
| `/O1` | 0/4 | 0/4 | 0/2 |
| `/O2 /Og-` | 0/4 | 0/4 | 0/2 |
| `/O2 /G5` | 4/4 | 4/4 | 2/2 |
| `/O2 /G6` | 3/4 | 3/4 | 2/2 |
| `/Ox` | 4/4 | 4/4 | 2/2 |
| `/O2 /Ob0` | 4/4 | 4/4 | 2/2 |
| `/O2 /Op` | 4/4 | 4/4 | 2/2 |

The four relocation-free functions cover pointer-field updates with conditional
control flow (two variants), bounded integer/bit-field arithmetic, and a sentinel
loop. Their full contributions total 240 bytes, including compiler-generated
NOP padding that also occurs in the oracle. `/G6` changes 17 bytes of the
64-byte `make_colour` contribution; the other three remain exact.

The later shared-engine milestone adds exact three-axis motion and fixed-point
vector arithmetic under the same pinned RTM `/O2 /Gy` configuration. ENG3's
vector variant remains two shift-immediate bytes away under both RTM and SP2;
see `docs/shared_code.md`. This expands the tested code shapes without proving
the original compiler patch level or complete translation-unit context.

The two diagnostic functions in `calibration/title_control.c` cover an 18-step
loop with two calls per iteration (RVA `0x4a10`, 48-byte contribution) and a
switch with eight calls (RVA `0x6430`, 80-byte contribution). Every external
symbol has a recorded direct-call destination independently checked against the
oracle. All remaining differences under `/O2` are those REL32 operands. The
diagnostic resolves them only in a comparison buffer; it writes no executable
and **does not award FUNCTION_MATCH**. There is no proven reconstructed callee
closure or natural linked placement yet.

The switch's separate source blocks for case zero and default contain the same
cleanup sequence. This plausible source form causes VC5 to merge their tails
while retaining the oracle's otherwise redundant case-zero test. Combining the
two source cases, or using early returns, changes code generation. This is an
observed source-structure dependency, not proof of the original source spelling.

These results support optimized VC5 code generation and distinguish some flag
families. They do not distinguish RTM from SP2, `/O2` from `/Ox`, default CPU
tuning from `/G5`, or the tested inlining/floating-point controls. No floating-point
pilot, struct-packing proof, exception/RTTI configuration, exact CRT/MFC library
revision, PCH reconstruction, or original SDK selection is proven. The separate
CRT fingerprint comparison establishes TITLE's static multithreaded family.

## Acquired tools and reproducible setup

The pre-existing `C:\tools` tree was inspected first. It contained Ghidra and
several DOS-era tools but no VC compiler. `tools.zip` contained Pascal/LZEXE
material. The VC5 download locations were found through the user-supplied
[Blood II toolchain research](references.md).

| Local ID | Driver CL | C front end | C++ front end | Back end C2 | LINK |
| --- | --- | --- | --- | --- | --- |
| `msvc5_rtm` | 11.00.7022 | 11.00 | 11.00 | 11.00 | 5.00.7022 |
| `msvc5_sp2` | 11.00.7022 | 11.00.7113 | 11.00.7149 | 11.00.7153 | 5.02.7132 |

The unchanged CL driver is identical in both trees; its banner alone cannot
identify the compiler back end. Paths, archive URLs, immutable source commits,
archive SHA-256, important file identities, and complete installed-tree digests
are in `toolchains/manifest.json`.

```powershell
.\scripts\setup_toolchain.ps1               # install missing pinned candidates
.\scripts\setup_toolchain.ps1 -VerifyOnly   # verify every installed compiler file
```

Setup extracts into `C:\tools\hercules\<repository>-<commit>`. It rejects a
wrong archive hash and a modified existing tree. The sole layout adjustment is
an unchanged copy of `redist/msvcp50.dll` into `bin`, needed by the linker.
No installer, registry change, compiler patch, or binary modification is used.

Compile with an explicit tool path and a process-local environment:

```powershell
$tc = 'C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953'
$env:PATH = "$tc\bin;$env:PATH"
$env:INCLUDE = "$tc\include"
$env:LIB = "$tc\lib"
# scripts/match.py sets these itself and clears ambient CL/_CL_ options.
```

Do not add `redist` to PATH: its historical MSVCRT must not shadow the operating
system runtime. On this host the Codex restricted Windows process environment
fails to start these old 32-bit tools with an illegal-DLL-relocation error;
the same untouched tools compile successfully in an approved unsandboxed
process. This is a host execution limitation, not a reason to alter the tools.

## Analysis dependencies

Analysis is separate from historical compilation. Python 3.12.10 was already
installed; Capstone 5.0.7 and pefile 2024.8.26 are installed under
`C:\tools\hercules\python`. Their 64 and 12 hashed RECORD entries were verified.
The Windows x64 wheel hashes published by PyPI are locked in
`toolchains/analysis-requirements.txt`; measured key file hashes are in the
manifest. Reinstall with:

```powershell
python -m pip install --only-binary=:all: --require-hashes `
  --target C:\tools\hercules\python -r toolchains\analysis-requirements.txt
```

Both acquired VC5 trees already contain `ddraw.h`, `dsound.h`, `d3d.h`,
`d3dcaps.h`, `d3dtypes.h`, `d3drm.h`, and `dplay.h`, plus DirectDraw, DirectSound,
Direct3DRM, and DirectPlay import libraries. Their DirectDraw header declares
IDirectDraw2/IDirectDrawSurface2. `dinput.h`, `dinput.lib`, and `dxguid.lib` are
absent from these trees. Identities are in `evidence/crt.json`; this inventory
does not establish an SDK release or test the headers against Hercules source.

Ghidra's existing installations remain available for deeper analysis but are not
part of the compiler or acceptance environment. Further SDK/runtime acquisition
should follow a concrete unresolved import, header, or library fingerprint
instead of assuming a version from the game's release year.
