# Remaining verification boundaries

| Target | Established | Remaining |
|---|---|---|
| Passive capture | Shared submission callback, 250-word native prefix copies, per-module OT views, checked packet decoder | Actual bootstrap selection/timing; representative reachable commands; borrowed lifetime and live reload after callbacks |
| Faithful GPU packet rendering | Distinct indexed pixel kernels, finite sprite clipping, rectangle dispatch, startup LUT aliasing, forward VRAM move order | Polygon edge/rounding/clipping rules; complete reachable command coverage; resource-table lifetime order; callback-driven state changes and full buffer ownership |
| Original window integration | Host HWND, DIB/DirectDraw surface, palette, lock/unlock/present/movie paths | Bounded coexistence observation; surface loss/restoration; synchronization with preserved writers |
| Full presentation replacement | Separate palette/movie/remap/clear writers identified | All writer ownership and mode transitions; complete replacement of presentation services |
| Enhanced 3D | Projected XY/UV and earlier camera/projection snapshot identified | Per-vertex association across clipping and modules; depth/perspective/normal information absent from final packets |

Optional 3D enhancements do not block basic capture. Whole-EXE reconstruction
and complete software-rasterizer reimplementation are unnecessary. The next
high-leverage fixture is the native polygon edge/clip contract, followed by
resource-driven LUT ordering and bounded notification effects. Bootstrap
installation and actual host/GPU output remain unobserved. Unsupported packets
remain visibly rejected, never silently treated as drawing success.
