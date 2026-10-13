# Prediction and falsifier v4: three original parent-factor operands

This prediction follows the prior recorded falsifiers. The selected record's `+8` WORD controls the generated orientation; object `+8` maps to camera X in the zero-angle setup; record byte `+0xF=5` produced the predicted slot-4 scratch vector `(0,0,640)`, but camera Z stayed at 1 because the controlled parent input used the wrong three values for HERC slot 165.

Prediction written before this run:

- Original ENG3 `0x204C0` writes `[0,10,20]`. Coordinate `9` must return `0xffffffff` before `0x24BB0` and before HERC transform/projection callbacks.
- At coordinate `10`, use object DWORDs `+0=(10*128)<<16`, `+4=0`, `+8=0`; selected 16-byte record byte `+0xF=5`, with other bytes zero. ENG3 `0x24BF7/0x24CED/0x24D01` predict slot-4 scratch WORDs `(0,0,640)`. The original HERC slot-4 target is `0x00BA25C0`; original slot 101 reads those bytes after the helper writes them.
- For the controlled parent buffer at ENG3 `0x10039518`, set its first three DWORD operands at offsets `+0/+4/+8` to Q12 `4096` each, leaving all later bytes zero. This follows the exact reads in original HERC slot 165 (`0xBC30`), which repeatedly uses only those three DWORD addresses across its nine WORD stores. This predicts it will preserve all nine local orientation WORDs generated for the zero-angle case. The expected transform is camera Z `640`; original slot 121 then writes depth WORD `160`; ENG3 `0x24A80` returns through the pinned success tail `0x24AEF..0x24AF4` with a value other than `0xffffffff`.
- Angles/translations remain zero; center/focal remain `(0,0)`/`256` through original ENG3 wrappers and original HERC slots 174/175. The original complete callback prefix and file-backed slot-5 table are used unchanged.

Falsifier: coordinate `9` fails to reject early; coordinate `10` fails to write/read `(0,0,640)` via the original slot-4 pointer target, fails to produce camera Z `640` and depth `160`, fails to return through the success tail, or sees an input write, unbacked image read, invalid access, or unapproved code address.

This proves only this finite controlled decision. The three parent DWORD operands are identified from HERC `0xBC30` loads; they are not promoted to a recovered matrix structure or runtime transform state. Resource semantics, caller state, live visibility, and packet emission remain open.

## Evidence basis

ENG3 pins: `0x24A80 [0x24A80,0x24AEF)`, adjacent success tail `0x24AEF [0x24AEF,0x24AF4)`, and helper `0x24BB0 [0x24BB0,0x24F4B)`. HERC slot 165 target `0xBC30 [0xBC30,0xBCCE)` is pinned by the existing source runner. The four successful epilogue bytes and all helper/callback bytes are checked before execution.
