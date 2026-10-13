# Early table-publication loader surrogate

This fixture compares early publication in a source-built DLL's
`DLL_PROCESS_ATTACH` with publication deferred to the fixture's mock
`DirectDrawCreate` call. Both fixture EXEs import a sibling source-built
`ddraw.dll` and contain a synthetic 250-DWORD table with signature
`0x5ac00cac`. The early DLL uses the existing
`engine_table_clone_selected` helper to replace only fixture slot 89 with
`0xf17e0089`; the deferred case waits until after the EXE constructor's first
copy. The constructor observes the replacement in the early case and the
original word in the deferred case. Full table and source-preservation checks
pass, and both mock calls return `E_NOTIMPL` with a null output pointer.

This proves table-copy timing only inside the source-built fixture. It does not
launch Hercules, use original executable pointers, create a DirectDraw/COM
object, resolve a genuine system implementation, or establish product-DLL
safety. It also does not settle Hercules' independent guarded event/tick path
(`0x79a0 -> 0x79fc -> 0x5f20 -> 0x5ec0`) that can read the current callback
pointer and dispatch without traversing `DirectDrawCreate`; host scheduling and
reentrancy remain unknown. Thus the fixture supports early data publication as
a candidate worth evaluating, but does not show that the candidate wins the
first possible host dispatch.

The fixture intentionally performs only its bounded accessor lookup, table
clone, and pointer publication during attach. Microsoft documents that
`DllMain` runs under the loader lock, recommends minimal initialization, and
warns against direct or indirect `LoadLibrary` calls there. This experiment is
not a recommendation to load or forward system DirectDraw in `DllMain`; keep
real DLL loading/forwarding out of the attach path.
[Microsoft: Dynamic-Link Library Best Practices](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices).

## Reproduction

From the renderer checkout root, build with the pinned VC5 toolchain verified
against `toolchains/manifest.json`. Output must be a new or empty directory
under `work/`:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' `
  renderer_contract\fixtures\early_loader_probe\build_and_run.py `
  --repo-root . `
  --output-dir work\early_loader_probe\replay
```

The builder uses `renderer_contract/hercules_host_abi.c` and the existing
`engine_table_clone_selected` contract helper, plus the fixture sources under
[`src`](src). The DX6 DirectDraw import library is link-time-only; the mock
export always returns `E_NOTIMPL`. The replay receipt under
`work/early_loader_probe/replay/receipt.json` contains the verified compiler
and import-library identities, source hashes, commands, case outputs, and
build hashes. Generated EXE/DLL hashes are intentionally not repeated here or
checked in as evidence; use the receipt from a fresh run.

The compact package evidence is
[`early_loader_surrogate.json`](../../evidence/early_loader_surrogate.json).
That file records the accepted fixture result and its limits; it is not
Hercules host-integration evidence. Original host ordering is separately
reported in [`bootstrap_order.md`](../../bootstrap_order.md), which identifies
the DirectDraw HRESULT branch as logging-only and the independent event/tick
route.

**Prediction:** in this fixture, attach-time publication is visible to the
EXE global constructor's first copy, while mock-API publication is too late.

**Falsifier:** reject the fixture result if the early constructor fails to see
slot 89's selected value, the deferred constructor sees it before the API call,
any unselected/source word changes, or the mock API does not return
`E_NOTIMPL` with a null output pointer.
