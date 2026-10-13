# Primary BD80 notification target

The static images do not identify a non-null primary callback target. The host publishes the setter `HERCULES.EXE+0xBDA0` in source-table slot 92. Its one-argument cdecl setter stores the supplied 32-bit callback address in `A8E780` and returns the previous address in EAX. Its only direct cell write is at `BDA9`. Startup helper `BDF0` sets shared gate `A8E788=1` and clears both primary `A8E780` and secondary `A8E784`; HERC calls it from `0x106E2` in the `0x10690` initialization routine. The gate-disable helper `BDE0` stores zero to the shared gate.

A full raw `.text` skipdata sweep found the local slot-92 adapter and every direct caller in the three shipped DLLs. Every direct call is an unregister/clear call with a pending `push 0` argument, then caller cleanup by `add esp,4`:

| Module | Adapter calls | Result |
| --- | --- | --- |
| ENG1.DLL | `0x632A`, `0x1F22C` | NULL |
| ENG3.DLL | `0xB1D1`, `0x1A46C` | NULL |
| TITLE.DLL | `0x1D4E2` | NULL |

The slot-92 adapter cells are `ENG1+0x71090`, `ENG3+0x40910`, and `TITLE+0x2BCB0`; each has one direct code access, inside its adapter. The HERC source-table slot 92 at RVA `0x36328` points to BDA0. No direct HERC call to BDA0, non-null direct setter argument, exported TITLE adapter, or other direct slot-92 access was found. This supports only the fixed direct paths: a callback is not installed by those observed calls. It does not prove A8E780 stays null at runtime, because computed table aliases, indirect callers, and runtime code generation are outside this scan.

BD80 checks the primary pointer and shared gate, then tail-jumps through EAX. It does not add arguments or normalize the caller's stack; ECX contains the gate value when control transfers. There is no statically fixed callback body or callback ABI beyond this forwarding boundary. Therefore no callback write set can be assigned: a runtime-installed target could mutate OT nodes, texture pages/LUTs, or renderer globals, but there is no evidence that the shipped runtime installs such a callback or that one actually performs those writes. Keep the synchronous callback boundary intact and reload the live OT link after it; in the AC60/A8F0 path, the link is read again after notification.

The secondary callback at A8E784/BDB0 is a separate path. HERC's native secondary registration helper installs `0x401D30`; it is not the primary BD80 callback and must not be used to infer primary ownership.

The first prediction was falsified: an older strict linear decode missed later wrapper/caller sites. The corrected scan covers all raw `.text` bytes with Capstone skipdata: HERC 183,808 bytes (65,375 decoded instructions, 4 skipdata bytes), ENG1 307,200 (100,212; 23), ENG3 200,192 (63,421; 6), and TITLE 130,560 (40,168; 16). This remains a linear sweep, so byte coverage is complete but instruction boundaries in embedded data and computed aliases are not fully proven.

Run the [public audit](tests/audit_callback_target.py):

```powershell
python renderer_contract/tests/audit_callback_target.py --original-root D:\Prog\hercules_recon --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE --output work/replay/notification_target.json
```

The audit verifies four image identities, complete raw-section byte coverage,
and fourteen deterministic gates, including twelve bounded decodes with
skipdata disabled. It writes only below this checkout's work directory.
[The receipt](evidence/notification_target.json) records fixed direct paths;
computed aliases and runtime callback installation remain unresolved.
