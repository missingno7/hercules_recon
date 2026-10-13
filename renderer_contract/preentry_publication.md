# Original-host pre-entry publication

Proof level: **HOST_INTEGRATION_OBSERVED**, limited to loader selection and
data-only table publication before the original EXE entry instruction. This
does not establish engine execution, production forwarding or rendering.

The pinned unmodified EXE has SHA-256
`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`.
The [source-built fixture](fixtures/preentry_probe/README.md) copies it into a
new ignored directory alongside a diagnostic `ddraw.dll`. Its process-attach
handler shallow-copies the existing 250-word interface, replaces submission
slot 89 with an unmistakable non-callable marker, and publishes the clone
through the original data pointer cell at RVA `0x365a0`. Other words retain
their original values, including word 1's shared context pointer.

An x86 debugger arms an execution hardware breakpoint at PE entry RVA
`0x11ea0`. It accepts the stop only with EIP and exception address at that
instruction, DR0 at the same address, DR6 B0 set and DR7 configured for the
execution breakpoint. It inspects memory while stopped, then terminates the
child and drains its exit event. It neither patches EXE instructions nor
continues the original entry instruction. Separate source-built synthetic
tests verify normal stopping and failure cleanup before breakpoint arming.

On the tested Windows 11 machine, the diagnostic sibling DLL was selected and
the clone was published before entry. The original table and file-backed
`.text` bytes remained unchanged; all 249 unselected words were preserved,
and `DirectDrawCreate` had not been called. The [receipt](evidence/preentry_deployment.json)
records executable, helper, source and runtime identities and the bounded
observations. No original game files are modified or included in the package.

This removes the early-publication scheduling uncertainty for this tested
deployment. A future production proxy still needs correct system DirectDraw
forwarding outside complex loader-lock work, callback lifetime management and
actual engine-copy/presentation verification. The diagnostic marker is never
called and is unsuitable for continued execution. The fixture deliberately
terminates at entry; its DLL is not an installable renderer or forwarding proxy.

The three original DLLs' cold 250-word copies are established separately by
the [interface audit](interface_publication.md). Combining that static evidence
with publication before host entry supports the recommended bootstrap
architecture; it does not substitute for observing a real engine initialization.
