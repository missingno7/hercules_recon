# Windowed resource checkpoint

This checkpoint loads actual archived Hercules resources through the recovered
PC name hash, uploads their indexed pixels with the shared A710 mapping, consumes
two owned sprite commands, and shows CPU, GPU, or indexed differences in a
resizable SDL3 window. It is a development tool with a reproducible original-code
oracle. The original title ordering-table producer and gameplay are still open.

From this checkout in PowerShell:

```powershell
Set-Location D:\Prog\hercules_recon\work\renderer_interface_repo
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py -B dropin_renderer/tools/run_preview.py
```

The accepted local build is already in `work/game_checkpoint_build`. The launcher
checks its executable, source and tool identities and the archive SHA-256. If the
sources have changed, select a fresh build directory; it will build and run CTest
before opening the window:

```powershell
& $py -B dropin_renderer/tools/run_preview.py --build work/my-next-checkpoint
```

The launcher locates the original archive in the primary checkout through Git's
common directory. Another layout can pass `--archive <absolute HERCULES.FS>`.
It opens its own window and reads the archive; it never launches the original
Windows game, installs a proxy, or writes game files or configuration.

| Control | Result |
| --- | --- |
| C | Shared CPU output |
| G | D3D12/DXBC GPU output |
| D | Index differences: black means equality |
| Space | Cycle the three views |
| R | Reload the pinned resource members and repeat comparison |
| Esc | Close |
| Resize window | Preserve the 4:3 image with nearest-neighbor scaling |

Interactive mode reports an unavailable GPU and retains CPU viewing. Finite
`--smoke` verification requires GPU equality and fails if it cannot obtain it.
Screenshots must have a fresh path beneath this checkout's ignored `work/`.

```powershell
& $py -B dropin_renderer/tools/run_preview.py --smoke --screenshot work/my-preview.bmp
& $py -B dropin_renderer/tests/run_title_screen.py `
  --exe D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE `
  --archive D:/Prog/hercules_recon/assets/pc/HERCULES/HERCULES.FS `
  --cli work/game_checkpoint_build/title_resource_cli.exe `
  --gpu --output work/my-original-comparison
```

The window uses `MENUB.TGA`'s raw top 240 rows and `MAINMENU/MAP_PSX.CIF`'s
logical colors. Both resources and their separate native consumption are proven.
Their pairing at a particular original title-sequence moment is **not** proven.
The current blue/dark appearance is therefore a diagnostic observation, not an
accepted original title frame. The embedded TGA colormap is deliberately unused:
the PC TITLE helper at RVA 1DD0 is empty, and the host palette selector changes
the `.PAL` suffix to `.CIF` before opening the resource.

The test chain contains two E1 changes and two variable sprites, covering 128
and 192 pixels respectively. It does not claim that TITLE produced these packets.
They exercise the complete 320x240 indexed target, actual source rows, and pages
10/11 after the measured `{320,256,160,256}` word upload. The shared sprite slice
supports ordinary pages, copy/raw blend, fixed 8/16 or variable sizes, and the
measured bottom bound 240. UV spans that wrap pages and other bottom bounds fail
explicitly. FT4 copy/shaded/raw blend remain their prior separately proven slice.

Fresh verification passed 11 modern tests without skips, 25 portable tests with
each of VC5 and GCC x86, 132 historical raw matches, the four prior composition
comparisons, and all 14 prior indexed copy/blend cases. Original CPU/shared CPU/
GPU agree on the 76,800 indices; original/shared logical palettes agree on all
1,024 bytes. Resource identity, commands and receipts are pinned in
[game_checkpoint.json](game_checkpoint.json) and the shared
[resource evidence](../../renderer_contract/evidence/title_resource.json).

Continue by recovering TITLE's actual image/palette selection, transition state
and ordering-table construction, then compare a producer-generated frame. Keep
simulation, loader timing, palette mutation and host ownership observable before
extending raster coverage or connecting the renderer to the game's window.
