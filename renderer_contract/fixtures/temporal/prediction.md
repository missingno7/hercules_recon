# Prediction: bounded native camera-history invocation

Before running native code, my prediction is that original ENG1 camera routine `0x2fb70`, invoked with `g_camera_flags_70d10 & 0x80 == 0` and `g_camera_iterations_70cfd == 0`, will:

1. write current camera Y DWORD `0x70ca4` to prior-Y cell `0x70cc8`, then current X DWORD `0x70ca0` to prior-X cell `0x70cc4`;
2. skip the follow loop and its actor/collision references, leave current X/Y/Z unchanged, update the derived camera grid cells, and return inside its own pinned body;
3. on a second controlled invocation with different seeded current X/Y/Z, overwrite the prior-X/Y cells with that invocation's current X/Y. This is a test of the original routine's entry/snapshot path under controlled inputs, not a pair of simulation updates or evidence of object identity.

The prediction is falsified if the exact writes or order differ, current XYZ change with zero iterations, execution calls an out-of-body helper or leaves the pinned body, any data dependency beyond the mapped camera fields is reached, or the second prior-X/Y values are not the second invocation's seeded current values.
