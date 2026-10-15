"""Emit an REA Ghidra seed file (rea-ghidra-seeds-v1) for the pinned host EXE.

Seeds come only from repository evidence plus the original bytes:
  * every host-.text word of the 250-DWORD engine table at RVA 0x361B8
    (named hcb<slot>_<rva>, with a role suffix where the slot is documented);
  * every recovery.json entry whose target is HERCULES.EXE (named by its id).
The EXE is read, never written. Usage:
  python make_seeds.py <HERCULES.EXE> <recovery.json> <out.tsv> [--table-only]
"""
import hashlib
import json
import struct
import sys

PINNED_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
IMAGE_BASE = 0x400000
TABLE_RVA = 0x361B8
TABLE_WORDS = 250

# Roles taken from renderer_contract/compatibility.md and hercules_host_abi.h.
SLOT_ROLES = {
    80: "frame_notify", 81: "chain_init", 89: "submit", 92: "primary_cb_exchange",
    100: "proj_pack", 101: "point_input", 102: "batch_point_input",
    103: "translation_snapshot", 106: "project", 107: "batch_project",
    108: "rotation", 109: "translation", 117: "ft4_screen_copy",
    129: "upload_rect", 130: "upload_page", 131: "move_rect", 143: "color_remap",
    145: "palette_resource", 148: "movie_display", 149: "text_out",
    160: "frame_header_copy", 174: "proj_center", 175: "proj_focal",
    240: "notify_disable", 249: "secondary_cb_exchange",
}


def sections(pe):
    peoff = struct.unpack_from("<I", pe, 0x3C)[0]
    nsec = struct.unpack_from("<H", pe, peoff + 6)[0]
    optsz = struct.unpack_from("<H", pe, peoff + 20)[0]
    base = peoff + 24 + optsz
    out = []
    for i in range(nsec):
        name, vsz, rva, rsz, roff = struct.unpack_from("<8sIIII", pe, base + 40 * i)
        out.append((name.rstrip(b"\0").decode(), rva, vsz, roff, rsz))
    return out


def main():
    exe, recovery, out = sys.argv[1:4]
    table_only = "--table-only" in sys.argv
    data = open(exe, "rb").read()
    digest = hashlib.sha256(data).hexdigest()
    if digest != PINNED_SHA256:
        sys.exit(f"unexpected EXE sha256 {digest}")
    secs = {s[0]: s for s in sections(data)}
    _, drva, _, droff, _ = secs[".data"]
    words = struct.unpack_from("<250I", data, droff + TABLE_RVA - drva)
    if words[0] != 0x5AC00CAC:
        sys.exit("table signature mismatch")
    _, trva, tvsz, _, _ = secs[".text"]
    lo, hi = IMAGE_BASE + trva, IMAGE_BASE + trva + tvsz
    seeds = {}
    for slot, va in enumerate(words):
        if lo <= va < hi and va not in seeds:
            role = SLOT_ROLES.get(slot)
            seeds[va] = f"hcb{slot}_{va - IMAGE_BASE:x}" + (f"_{role}" if role else "")
    if not table_only:
        for f in json.load(open(recovery, encoding="utf-8"))["functions"]:
            if f.get("target_sha256") == PINNED_SHA256:
                va = IMAGE_BASE + int(f["rva"], 16)
                seeds.setdefault(va, f["id"].replace(".", "_"))
    with open(out, "w", newline="\n") as fh:
        fh.write(f"# rea-ghidra-seeds-v1 for HERCULES.EXE {PINNED_SHA256}\n")
        for va in sorted(seeds):
            fh.write(f"0x{va:x}\tfunction\t{seeds[va]}\n")
    print(f"{len(seeds)} function seeds -> {out}")


if __name__ == "__main__":
    main()
