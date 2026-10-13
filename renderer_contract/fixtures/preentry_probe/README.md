# Pre-entry app-local DirectDraw probe

This source-only fixture tests one bounded startup condition for the pinned
32-bit Hercules executable: whether its `DDRAW.dll` import resolves to an
app-local sibling and whether a 250-word engine table pointer can be published
before the executable's first entrypoint instruction.

The controller starts an isolated byte-identical copy suspended, arms an x86
hardware execution breakpoint at the PE entrypoint, and terminates the child
while the breakpoint event is still pending. It does not patch executable
code. The diagnostic DLL copies the host's 250-DWORD source table into static
storage, changes slot 89 to marker `0xf17e0089`, and publishes the clone via
the host pointer cell at RVA `0x365a0`. Its attach work is limited to this
data copy and publication. `DirectDrawCreate` returns `E_NOTIMPL`; the stub
does not forward to system DirectDraw or initialize COM, a window, or a GPU.

A bounded run on this Windows 11 environment selected the sibling `ddraw.dll`
at base `0x18000000`. Before the first instruction at host EIP `0x00411ea0`,
the host pointer cell at `0x004365a0` referenced clone address `0x18007798`.
The source table and executable `.text` remained unchanged; clone word 1
matched the source, only slot 89 differed, `DirectDrawCreate` had not been
called, and the controller confirmed child termination and drained its exit
event. This is evidence for this pinned executable, directory layout, and
Windows loader environment only. DLL resolution and timing remain
deployment-dependent.

The source uses the repository's existing `renderer_contract/hercules_host_abi`
files and the manifest-pinned `msvc5_rtm` toolchain. Keep this package's
relative `src` directory intact when copying it into the repository. From the
renderer checkout root, run the source-only build first:

```powershell
$probe = 'renderer_contract/fixtures/preentry_probe/build_probe.py'
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' `
  $probe --build-only --repo-root . `
  --original-exe 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE' `
  --output-dir work\replay\preentry_probe
```

`--build-only` builds the controller and stub and runs two synthetic-target
smokes. Its receipt pins all source and output hashes. `--run-target` is a
separate explicit mode; it requires that receipt, verifies the current source
and build-output hashes, makes a byte-identical copy under the output
directory, and requires exact result fields including the hardware-breakpoint
registers and drained exit event. Output paths must resolve below checkout
`work` and stay outside the original executable and assets ancestry.

The fixture is not a production proxy or renderer implementation. It does not
provide forwarding behavior, COM services, surface preservation, movie
presentation, or compatibility with other deployments. The measured result
establishes that early data-only publication is technically possible for the
observed import and process startup; it does not establish that replacing
DirectDraw services is safe.

After the build passes, repeat the same command with `--run-target` instead
of `--build-only` and the same output directory. The run directory must not
already contain an `isolated` child. The recorded [deployment evidence](../../evidence/preentry_deployment.json)
pins the final public sources and both synthetic smoke results as well as the
bounded original-host result.
