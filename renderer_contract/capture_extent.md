# Command observation span

The first modern consumer needs to own each packet before a synchronous callback
can change future memory. Following `RendererDecodedPacket.minimum_size` alone
leaves command `0x01` incomplete: that field deliberately records AC60's 16-byte
direct-access envelope, while the inline rectangle passed to A8F0 occupies
`+0x10..+0x17`. The new C89 `renderer_packet_capture_size` exposes 24 bytes for
this command, using the authoritative `HrcCommand01WireView`; other packet
minima and all decoder statuses retain their previous meaning.

Prediction recorded in `tests/test_original_command01_extent.py`: a 16-byte
packet can enter A8F0, but its rectangle reads cross the readable boundary;
22 bytes still lack the final height WORD, and the complete 24-byte zero-height
case returns through the unchanged no-device path. A return for either shorter
case or a failure for the complete case falsifies this bounded prediction.

The fresh CPU fixture confirms a source-Y read at packet offset 18 from RVA
`0xA8F7`, then a height read at offset 22 from `0xAA6E`. The 16-byte and 22-byte
cases fault at those boundaries respectively. The 24-byte case completes and
reloads the live link at `0xACA2`. This establishes a transitive observation
requirement for the controlled case, not original producer allocation capacity,
general malformed-input handling, positive-area copying, or pixels.

The modern consumer requests this span, rejects truncated observations, and
passes owned bytes to the existing decoder/state/staging helpers. Its offline
callback runs before the shared walker reloads the current link. Entry-time
observations remain distinct from actual consumption; passive slot-89 wrapping
alone cannot observe each original internal consumption boundary.

Run from this checkout with the pinned retail EXE, read-only:

```powershell
& $py renderer_contract/tests/test_original_command01_extent.py `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --receipt work/dropin_validation/command01_extent.json
```

The compact provenance is in `evidence/command01_capture_extent.json`. The
full report remains in ignored work. No original game process or historical
function acceptance is involved.
