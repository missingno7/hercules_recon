# Bounded presentation-forwarding fixture

This source-only fixture combines the established pre-entry 250-DWORD table
publication boundary with stable passive wrappers for table slots 81 and 89
and a deferred `DirectDrawCreate` forwarder. Its process-attach path performs
only image/table guards, the fixed-size clone, and pointer publication. The
wrappers record raw arguments and callback boundaries in a fixed-size buffer;
they do not traverse ordering-table packets or touch display state.

From the repository root, the default command builds with the manifest-pinned
VC5 RTM tools and runs only the source-built fake backend, self-module guard,
and forwarded-self-export guard:

```powershell
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py -B renderer_contract/fixtures/presentation_probe/build_probe.py `
  --repo-root . `
  --output-dir work/presentation_probe_synthetic
```

The default mode makes no operating-system DirectDraw call and starts no
original executable or engine. Its receipt includes source, helper, toolchain,
command, and output hashes. Output must be a new or empty descendant of
`work/`; the builder checks the lexical and resolved locations and rejects
symlinks and Windows junctions on the output path.

The real-system test is a separate explicit mode. Use it only for a reviewed
fresh source/API receipt; do not repeat the historical first call just to
relabel its preserved path result. The exact command is:

```powershell
& $py -B renderer_contract/fixtures/presentation_probe/build_probe.py `
  --repo-root . `
  --output-dir work/presentation_probe_system `
  --run-api `
  --timeout-seconds 10
```

That mode starts only a source-built x86 probe, calls
`DirectDrawCreate(NULL, &object, NULL)` once, records the HRESULT and object,
and invokes `IUnknown::Release` once for any non-null object. In that same
process it records the requested full path, `GetModuleFileNameA` module path,
read-only final path of the opened module file, and PE identity read from the
mapped image. The builder pins the physical x86 system DLL's version, size,
SHA-256, PE identity, and Windows build before the run and checks the file hash
again afterward. The probe does not create a window, set cooperative level,
create surfaces or palettes, request a display mode, or enter gameplay.

`DirectDrawCreate` attempts to initialize the object for the active display
driver, so that explicit call may initialize DirectDraw/driver code or allocate
transient system resources. No claim is made that the driver runtime has no
internal side effects. Microsoft documents the API contract at
[DirectDrawCreate](https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreate)
and object release at
[IUnknown::Release](https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-release).

The first real-system receipt remains preserved verbatim at
`../../evidence/presentation_forwarding_first_raw.json`. Its raw status is
`UNEXPECTED_FORWARD_MODULE_PATH`; retain that status and its logical path. A
separate earlier metadata-only process resolved the path and matched the
module/export identity across processes. That is explicitly cross-process
evidence, not same-process identity proof. The new explicit mode above records
all of those path and mapped-image observations in one process.

The source-built API result proves only that this candidate forwarded to the
pinned system export in that diagnostic process. Forwarding from original
Hercules, engine initialization, a displayed frame, renderer takeover, and
shutdown restoration remain unresolved. See
[`presentation_takeover.md`](../../presentation_takeover.md) for the display
ownership boundary.
