# Host callback metadata

## Finding

The `.rdata` words at RVAs `0x2E5CC`, `0x2E62C`, and `0x2E644` are the `pfn` fields of three `CMainFrame` message-map entries. Their entry records begin at RVAs `0x2E5B8`, `0x2E618`, and `0x2E630`; in each 24-byte row, `pfn` is field six at offset `+0x14`. They are message handlers, not virtual-method slots.

The local pinned MFC declarations support the field and signature names below. Separately, original EXE code and data show the runtime-class getters, map getters, object factories, constructors, vptr stores, and vtable slots. This is static metadata evidence only; it does not demonstrate that MFC invoked a handler or scheduled any callback.

| Handler RVA | Entry RVA | `nMessage` | `nCode` | `nID..nLastID` | `nSig` | `pfn` |
|---:|---:|---|---:|---:|---|---:|
| `0x9820` | `0x2E5B8` | `0x001C` `WM_ACTIVATEAPP` | `0` | `0..0` | `0x0E` `AfxSig_vww` — `void (UINT, UINT)` | `0x409820` |
| `0x9840` | `0x2E618` | `0x0111` `WM_COMMAND` | `0` | `0xE146..0xE146` | `0x0C` `AfxSig_vv` — `void (void)` | `0x409840` |
| `0x9870` | `0x2E630` | `0x0005` `WM_SIZE` | `0` | `0..0` | `0x11` `AfxSig_vwii` — `void (UINT, int, int)` | `0x409870` |

The message names and values come from the pinned `winuser.h`; the `AfxSig` names and declarations come from the pinned `afxmsg_.h`. The custom command ID `0xE146` is left numeric because no application resource definition was established.

## Original object and vtable links

The host's `CRuntimeClass` record at `0x2E568` contains the original class-name string `CMainFrame`, object size `0xBC`, factory pointer `0x409420`, and base-class record pointer at `0x430F80` (`CFrameWnd`). Its map descriptor at `0x2E580` contains base-map pointer `0x430B70` and entries pointer `0x42E588`. The 31-row array, including its zero sentinel, occupies 744 bytes and hashes to `43a94caa61152bd42a7b89f40ec1ee38cb7f87276ea2bbc4be0ac66621b32807`.

The factory at RVA `0x9420` directly calls constructor `0x94A0`. The constructor stores vtable base VA `0x42E870` at RVA `0x94A8`. In that vtable, `GetRuntimeClass` is slot 0 and `GetMessageMap` is slot 12 (byte offset `0x30`). Their six-byte bodies return the runtime-class address `0x42E568` and map descriptor address `0x42E580`, respectively. Thus the three callback addresses are located in message-map rows, while the map getter itself is a virtual method at slot 12.

Two neighboring objects independently match this metadata shape: `CJoystickPage : CPropertyPage` (runtime record `0x2E290`, map `0x2E2A8`, entries `0x2E2B0`, vtable `0x2E310`; factory `0x8090` calls constructor `0x8100`, which stores that vtable at `0x81D9`) and `CKeyboardPage : CPropertyPage` (runtime record `0x2E408`, map `0x2E420`, entries `0x2E428`, vtable `0x2E470`; factory `0x8A30` calls constructor `0x8AA0`, which stores that vtable at `0x8BC1`). Their vtables also place `GetRuntimeClass` at slot 0 and `GetMessageMap` at slot 12. Full span hashes are in [callback_metadata.json](evidence/callback_metadata.json).

The installed VC5 MFC headers declare `CRuntimeClass` fields in the observed order, `AFX_MSGMAP` as `{pBaseMap, lpEntries}` for the static-MFC form, and `AFX_MSGMAP_ENTRY` as `{nMessage,nCode,nID,nLastID,nSig,pfn}`. The EXE descriptors point to file-backed `.rdata` base-map data and 24-byte rows. The report uses the headers to name and interpret those fields; it does not infer exact source spelling or MFC runtime behavior.

## `0x79A0` owner: application `OnIdle` override

Resolved (2026-10-15). The earlier bounded scan looked only for references into `0x2E210..0x2E27F`; that span is the tail (slots 14..40) of the application object's vtable, whose base `0x42E1D8` is referenced by the constructor. An REA/Ghidra session supplied the lead (`prototypes/rea_eval`); every fact below is re-derived from bytes by [audit_idle_owner.py](tests/audit_idle_owner.py), receipt [idle_owner.json](evidence/idle_owner.json):

- The static initializer at `0x7660` passes object `0x96E428` to the constructor chain; constructor `0x75E0` calls the CWinApp constructor `0x2BAC5` and stores vptr `0x42E1D8` at `0x75EA`. The base constructor stores vptr `0x4309F4`.
- The derived vtable differs from the base in exactly slots 1, 12, 22, 26 and 28. Slot 12 (`0x75D0`) returns the app message map `0x42E170` (`{pBaseMap=0x430958, lpEntries=0x42E178}`; ON_COMMAND rows for `0x800E`, `0xE100`, `0xE101`). This matches the slot-12 `GetMessageMap` of the three classes above.
- Base slot 26 is `0x25886`, whose body is MFC `CWinApp::OnIdle(LONG)`: `m_pDocManager` at `+0x80`, result `lCount < 1`, `ret 4`. The override `0x79A0` is `__thiscall` with `ret 4` and ends by calling `0x25886(0)` with the saved `this`. In the VC5 MFC 4.2 layout, slot 22 is `InitInstance` (`0x7670`), 26 `OnIdle` (`0x79A0`) and 28 `ExitInstance` (`0x7960`).
- `CWinThread::Run` (`0x25A22`, base slot 23 → `0x257B6` → `0x25A22`) calls `[vtbl+0x68](lIdleCount++)` at `0x25A56` while `PeekMessageA(..., PM_NOREMOVE)` finds no message. Of the six `call [reg+0x68]` sites in `.text`, only this one passes the single stack argument `OnIdle` pops. `0x64CE` calls slot 26 of the COM object in `0x96E3EC` after `TextOutA` (the `IDirectDrawSurface::ReleaseDC` slot). `0x118BF` is CRT exception machinery. `0x241A3`/`0x2B6B8` push two words and `0x2A465` pushes none.
- `0x79A0` runs its tick (`5A50`, `6BB0`, then `5F20` when `BA0238 != 0 && BA0228 == 0`) inside `__try`. Its scope table `0x42E280` is `{-1, filter 0x7A1D, handler 0x7A27}`. The handler formats `"Exception %08lX at Address %08lX"` and calls fatal service `0x10460` (MessageBox when windowed, `WM_CLOSE`, CRT exit `-123`). When the gates are closed it blocks in `WaitMessage`, then returns `CWinApp::OnIdle(0)`, which is TRUE.
- `InitInstance` (`0x7670`) ends with `UpdateWindow` then `call 0x5E10`, the direct DirectDraw/engine-load route, before returning.

Consequence: the engine frame (export call, AC60 rasterization and present) executes synchronously on the UI thread inside MFC idle processing, bracketed by OnIdle's `__except`. A fault anywhere below that call, including host callbacks and any interposed code, becomes the fatal exit unless contained. This is a static dispatch result. It does not prove the run-time frame cadence, `5F20`'s timing policy, or that no other code path invokes the slot.

## Provenance and replay

The runner consumes only the public [bootstrap-gates receipt](evidence/bootstrap_gates.json) and its public audit source. It pins all four original module hashes, verifies the four target code spans against that receipt, and emits the metadata receipt without original-byte dumps or full disassembly. It writes only under the checkout's `work/` directory and rejects output inside the original image directory.

Header provenance is pinned in the JSON receipt. The installed local package directory is `C:\tools\hercules\msvc500sp2-4ebf02022705b4c9e9108d3ed3f286ed80ba2ed9`; its README (SHA-256 `7641c6d9f64e3b14da391954528faf93d5f79e798418506639f7ac408feda213`) identifies Microsoft Visual C++ 5.00 product version `5.00.7022`, driver `11.00.7022`, linker `5.02.7132`, and the `archaic-msvc/msvc500` source. The specific headers are pinned by full SHA-256: `afx.h` `3ca0caeb968ceb2c99b81251dc29f7ca5ad275358633ff3f514b4fcd08a72902`, `afxwin.h` `87603d834fcaa1d21b94ebc241f26164ab7698898b00ea774f249196540a7087`, `afxmsg_.h` `9b590d7b43f3aed98491ed1bf8b7466e5faf6193e5d357284c320e8a0033fb67`, and Windows `winuser.h` `df89c70f2c4392643f25d8b7e70d19b2052041d3bf4e2cb08f671ee6483a20ea`.

From a checkout containing `renderer_contract` and `scripts/pe.py`:

```powershell
$env:HERCULES_PYTHON_DEPS='C:\tools\hercules\python'
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' renderer_contract\tests\audit_callback_metadata.py --repo-root . --image-dir D:\Prog\hercules_recon\work\discs\pc_install --output work\renderer_callback_metadata\callback_metadata.json
```

The runner may also discover the checkout from its own canonical location; `--original-root` is an alias for `--repo-root`. The package receipt is a compact successful replay from the public bootstrap evidence. No process launch or gameplay test is involved.
