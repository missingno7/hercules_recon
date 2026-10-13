# ENG1 mesh association fixture: pre-experiment prediction

Scope: execute unchanged original ENG1.DLL mesh producer 0x2DF20 with one finite synthetic object/mesh input, real point emitters, actual HERCULES.EXE point-input/transform/project/output/GT4 packet callbacks where their dependencies are bounded, then observe packet/OT writes. No renderer implementation, gameplay, object identity inference, or whole-module claim.

Prediction: each GT4 packet corner will copy projected XY from the reusable point scratch record selected by that face's encoded vertex index. Reordered and repeated face indices will therefore reorder/repeat packet XY even where distinct source points project to identical XY. Packet insertion will prepend into the bucket selected from the original face/material data; emitted camera/projected coordinates must be associated by execution order and explicit memory provenance, not coordinate matching.

Falsifier: any packet corner receives XY from another source index or from a coordinate-only lookup, the native instructions show a different face-index mapping, no point-to-corner provenance can be established from actual stores/loads, or the output order-table destination/tag/link contradicts the native stores under the synthetic input. Any unbounded unknown external callback or memory read fails the fixture visibly.

Discriminator: use four source vertices with distinct input coordinates, two points deliberately projecting to identical screen XY, and face records with nonsequential order plus repeated indices; compare point scratch writes, face index reads, packet XY stores, and bucket-head/link writes by address and instruction order.
