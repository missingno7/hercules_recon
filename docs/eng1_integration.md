# ENG1 host integration preparation

This workflow adds hash-checked runtime staging and a compact VC5 x86
debug-event probe. It does not change `assets/`, `HERCULES.EXE`, original
`ENG3.DLL`, original `TITLE.DLL`, or the stable source-built TITLE DLL. The
runtime receives a no-CD copy of the host, original ENG3 and data files, source
TITLE (`0dde33225d04557aa81e3870674ea9b08edb7faa38ef3f6db978e08fe54ab775`),
and only a successful report-bound source ENG1 build. Unknown runtime files and
unrecorded ENG1 files are refused.

The ENG1 linker is currently incomplete. The commands below are prepared for
future use; the one-shot host launch is deliberately not part of the current
static checks.

```powershell
python scripts/eng1_integration.py build-probe
python scripts/eng1_integration.py prepare
python scripts/eng1_integration.py inspect
python scripts/eng1_integration.py attach
python scripts/eng1_integration.py inspect
```

After `attach` and `inspect` accept a successful ENG1 report,
`launch-once` can perform one bounded `DEBUG_ONLY_THIS_PROCESS`
observation (5â€“120 seconds). Each invocation records a new attempt before launch; repeated observations
remain separate, finite runs. The probe logs the actual ENG1
module base, source entry RVA/address from the verified link map, observed
copied interface-table words and first-use state, first-chance and second-chance
exceptions, host mode bytes, window caption/menu/client size, and process exit.
It sends only the observed Alt+Enter key messages and menu command 32801 after
checking host mode state; it sends no gameplay input and never edits target code
or process memory. A logged entry address is a map calculation at a loaded
module base; function execution itself is not trapped.

The runtime manifest records source and file hashes.
`work/eng1_behavioral/integration/probe_build/build_report.json` records the
pinned compiler and fresh probe compile/link commands.

Current verification is limited to Python syntax checking and a fresh static
VC5 compile/link of the probe. It does not compile ENG1 or launch the game.
