# Remaining verification boundaries

| Target | Established | Remaining |
|---|---|---|
| Passive capture | Shared submission callback, 250-word native prefix copies, per-module OT views, checked packet decoder | Actual bootstrap selection/timing; representative reachable commands; borrowed lifetime and live reload after callbacks |
| Faithful GPU packet rendering | Distinct indexed pixel kernels, finite sprite/triangle/quad/fill geometry, E680 shade/blend/zero-source paths, startup LUT aliasing, forward VRAM move order | Broader coordinate domains; complete reachable command coverage; resource-transition values/timing; callback-driven state changes and full buffer ownership |
| Original window integration | Host HWND, DIB/DirectDraw surface, palette, lock/unlock/present/movie paths; static IsLost/Restore branches | Live coexistence and loss/restoration observations; synchronization with preserved writers |
| Full presentation replacement | Separate palette/movie/remap/clear writers identified | All writer ownership and mode transitions; complete replacement of presentation services |
| Enhanced 3D | Projected XY/UV and earlier camera/projection snapshot identified | Per-vertex association across clipping and modules; depth/perspective/normal information absent from final packets |

Optional 3D enhancements do not block basic capture. Whole-EXE reconstruction
and complete software-rasterizer reimplementation are unnecessary. The next
verification boundaries are wider coordinate domains beyond the five proven
skewed quads, shared-state ownership, reachable commands and resource-driven
rectangle/LUT values. The fixed ENG1/ENG3 high-band upload is now bounded to
128 KiB; this establishes a write interval, not a general texture allocation.
Bootstrap
installation and actual host/GPU output remain unobserved. Unsupported packets
remain visibly rejected, never silently treated as drawing success.
