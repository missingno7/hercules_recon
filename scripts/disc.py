"""Read ISO9660 logical files from CUE/BIN evidence without mounting or changing it.

Supports a single BIN with a MODE1/2352, MODE2/2352 or MODE1/2048 first
data track. CD-XA Form 2 sectors are not ISO 2048-byte file data: their
2352-byte sectors are preserved as a separate .xa-raw file when encountered.
The JSON index labels the logical view and the raw companion explicitly.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def cue_info(cue: Path) -> dict:
    content = cue.read_text(encoding="utf-8-sig")
    filenames = re.findall(r'^\s*FILE\s+"(.+?)"\s+BINARY\s*$', content, re.M | re.I)
    if len(filenames) != 1:
        raise ValueError("Only one BIN per CUE is supported")
    tracks = []
    for match in re.finditer(r'^\s*TRACK\s+(\d+)\s+(\S+)(.*?)(?=^\s*TRACK|\Z)', content, re.M | re.S | re.I):
        idx = re.search(r'INDEX\s+01\s+(\d+):(\d+):(\d+)', match[3])
        if not idx:
            raise ValueError("Track missing INDEX 01")
        minute, second, frame = map(int, idx.groups())
        tracks.append({"number": int(match[1]), "mode": match[2].upper(),
                       "start_sector": (minute * 60 + second) * 75 + frame})
    if not tracks or tracks[0]["mode"] not in {"MODE1/2352", "MODE2/2352", "MODE1/2048"}:
        raise ValueError("Unsupported first track mode")
    binary = (cue.parent / filenames[0]).resolve()
    # A malicious CUE must not turn an inventory into arbitrary file access.
    if binary.parent != cue.parent.resolve():
        raise ValueError("BIN must be beside its CUE")
    return {"cue": cue, "binary": binary, "tracks": tracks}


class Disc:
    def __init__(self, cue: Path):
        self.info = cue_info(cue)
        self.track = self.info["tracks"][0]
        self.stride = int(self.track["mode"].split("/")[1])
        self.handle = self.info["binary"].open("rb")
        self.sector_limit = (self.info["tracks"][1]["start_sector"]
                             if len(self.info["tracks"]) > 1
                             else self.info["binary"].stat().st_size // self.stride)
        self.handle.seek((self.track["start_sector"] + 16) * self.stride)
        volume = self.sector(16)[0]
        if volume[:7] != b"\x01CD001\x01":
            raise ValueError("Primary volume descriptor not found at sector 16")
        if struct.unpack_from("<H", volume, 128)[0] != 2048:
            raise ValueError("Only 2048-byte ISO logical blocks supported")
        self.volume = {"system_id": volume[8:40].decode("ascii").strip(),
                       "volume_id": volume[40:72].decode("ascii").strip(),
                       "volume_sectors": struct.unpack_from("<I", volume, 80)[0],
                       "creation_time": volume[813:830].decode("ascii", errors="replace"),
                       "modification_time": volume[830:847].decode("ascii", errors="replace")}
        self.root = self.record(volume[156:156 + volume[156]])

    def close(self):
        self.handle.close()

    def sector(self, lba: int) -> tuple[bytes, bytes, bool]:
        position = self.track["start_sector"] + lba
        if lba < 0 or position >= self.sector_limit:
            raise ValueError(f"Data-track LBA out of bounds: {lba}")
        self.handle.seek(position * self.stride)
        raw = self.handle.read(self.stride)
        if len(raw) != self.stride:
            raise ValueError("Truncated sector")
        if self.stride == 2048:
            return raw, raw, False
        if raw[:12] != b"\x00" + b"\xff" * 10 + b"\x00":
            raise ValueError(f"Bad CD sector sync at {lba}")
        mode = raw[15]
        if mode == 1:
            return raw[16:2064], raw, False
        if mode == 2:
            if raw[16:20] != raw[20:24]:
                raise ValueError(f"CD-XA subheader copies differ at {lba}")
            return raw[24:2072], raw, bool(raw[18] & 0x20)
        raise ValueError(f"Unsupported CD sector mode {mode} at {lba}")

    @staticmethod
    def record(data: bytes) -> dict:
        size = data[32]
        return {"lba": struct.unpack_from("<I", data, 2)[0],
                "size": struct.unpack_from("<I", data, 10)[0],
                "flags": data[25], "name": data[33:33 + size].decode("ascii"),
                "recording_time_hex": data[18:25].hex()}

    def read(self, entry: dict) -> bytes:
        if entry["flags"] & 0x80:
            raise ValueError("Multi-extent ISO files are unsupported")
        chunks = []
        for offset in range((entry["size"] + 2047) // 2048):
            data, _, form2 = self.sector(entry["lba"] + offset)
            if form2:
                raise ValueError("Form 2 sector in ISO directory")
            chunks.append(data)
        return b"".join(chunks)[:entry["size"]]

    def walk(self, entry: dict | None = None, parent: str = "", seen=None):
        if entry is None:
            entry = self.root
        if seen is None:
            seen = set()
        if entry["lba"] in seen:
            raise ValueError("Cyclic directory tree")
        seen.add(entry["lba"])
        data = self.read(entry)
        offset = 0
        while offset < len(data):
            length = data[offset]
            if length == 0:
                offset = (offset // 2048 + 1) * 2048
                continue
            child = self.record(data[offset:offset + length])
            offset += length
            if child["name"] in {"\x00", "\x01"}:
                continue
            name = child["name"].split(";")[0].rstrip(".")
            if name in {"", ".", ".."} or "/" in name or "\\" in name or ":" in name:
                raise ValueError("Unsafe ISO file name")
            child["path"] = parent + name
            if child["flags"] & 2:
                yield from self.walk(child, child["path"] + "/", seen)
            else:
                yield child

    def extract(self, entry: dict, destination: Path) -> dict:
        if entry["flags"] & 0x80:
            raise ValueError("Multi-extent ISO files are unsupported")
        target = destination / entry["path"]
        if not target.resolve().is_relative_to(destination.resolve()):
            raise ValueError("Extraction target escapes destination")
        target.parent.mkdir(parents=True, exist_ok=True)
        # First scan makes XA preservation streaming while avoiding a raw copy
        # for ordinary files. The logical view is never labelled a faithful XA file.
        sectors = (entry["size"] + 2047) // 2048
        form2_count = sum(self.sector(entry["lba"] + i)[2] for i in range(sectors))
        raw_target = target.with_name(target.name + ".xa-raw") if form2_count else None
        raw_handle = raw_target.open("wb") if raw_target else None
        digest = hashlib.sha256()
        xa_content_digest = hashlib.sha256()
        remaining = entry["size"]
        try:
            with target.open("wb") as output:
                for i in range(sectors):
                    data, raw, form2 = self.sector(entry["lba"] + i)
                    data = data[:min(remaining, 2048)]
                    output.write(data)
                    digest.update(data)
                    remaining -= len(data)
                    if raw_handle:
                        raw_handle.write(raw)
                        # Include XA subheaders (channel/interleave metadata) and
                        # all user bytes; exclude sector address and EDC/ECC.
                        xa_content_digest.update(raw[16:2348] if form2 else raw[16:2072])
        finally:
            if raw_handle:
                raw_handle.close()
        result = {**entry, "sha256": digest.hexdigest(), "form2_sectors": form2_count,
                  "extraction": "2048-byte logical view; raw XA sectors preserved" if form2_count else "ISO logical file"}
        if raw_target:
            result["raw_sector_companion"] = raw_target.relative_to(destination).as_posix()
            result["raw_sector_sha256"] = sha256_file(raw_target)
            result["xa_subheader_and_user_data_sha256"] = xa_content_digest.hexdigest()
        return result


def psx_exe(path: Path, root: Path) -> dict:
    data = path.read_bytes()
    keys = ("entry_pc", "initial_gp", "load_address", "load_size", "data_address",
            "data_size", "bss_address", "bss_size", "initial_sp", "sp_offset")
    fields = dict(zip(keys, struct.unpack_from("<10I", data, 0x10)))
    if data[:8] != b"PS-X EXE" or len(data) != 2048 + fields["load_size"]:
        raise ValueError(f"Bad PS-X EXE header: {path}")
    strings = []
    for match in re.finditer(rb"[ -~]{5,}", data):
        value = match.group().decode("ascii")
        if value.startswith("$Id:") or value in {"\\SRC1\\ENGINE\\", "\\SRC3\\ENGINE\\"}:
            address = fields["load_address"] + match.start() - 2048
            pointer = struct.pack("<I", address)
            strings.append({"file_offset": hex(match.start()),
                            "address": hex(address), "value": value,
                            "aligned_absolute_pointer_refs": [hex(i) for i in range(2048, len(data) - 3, 4)
                                                               if data[i:i + 4] == pointer]})
    return {"path": path.relative_to(root).as_posix(), "size": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
            "header": {k: hex(v) for k, v in fields.items()},
            "license_marker": data[0x4c:0x4c + 80].split(b"\0")[0].decode("ascii"),
            "provenance_strings": strings}


def unchanged_spans(a: bytes, b: bytes) -> list[dict]:
    """Unique 64-byte anchors, word-aligned; these spans are NOT functions."""
    def anchors(data):
        seen = {}
        for i in range(2048, len(data) - 63, 4):
            key = data[i:i + 64]
            seen[key] = i if key not in seen else None
        return seen
    ax, bx = anchors(a), anchors(b)
    pairs = sorted((offset, bx[key]) for key, offset in ax.items()
                   if offset is not None and bx.get(key) is not None)
    spans = []
    for left, right in pairs:
        if spans and left <= spans[-1][0] + spans[-1][2] and right - left == spans[-1][1] - spans[-1][0]:
            spans[-1][2] = left + 64 - spans[-1][0]
        else:
            spans.append([left, right, 64])
    return [{"original_file_offset": hex(x), "rerelease_file_offset": hex(y), "size": n}
            for x, y, n in sorted(spans, key=lambda v: (-v[2], v[0]))[:8]]


def level_table(data: bytes, address_to_offset, offset_to_address) -> dict:
    """Recover the observed {level-name, development-path, engine-name} table."""
    first = data.index(b"PLAYROOM\0")
    pointer = struct.pack("<I", offset_to_address(first))
    def string(address):
        offset = address_to_offset(address)
        if not 0 <= offset < len(data):
            raise ValueError("Pointer outside file")
        end = data.index(b"\0", offset, min(offset + 128, len(data)))
        return data[offset:end].decode("ascii")
    for i in range(0, len(data) - 11, 4):
        if data[i:i + 4] != pointer:
            continue
        rows = []
        for pos in range(i, min(i + 64 * 12, len(data) - 11), 12):
            try:
                name, path, engine = map(string, struct.unpack_from("<3I", data, pos))
            except (ValueError, UnicodeDecodeError):
                break
            if path not in {"\\SRC1\\ENGINE\\", "\\SRC3\\ENGINE\\"} or engine not in {"ENGINE1", "ENGINE3"}:
                break
            rows.append({"level": name, "development_path": path, "engine": engine})
        if len(rows) == 12:
            return {"file_offset": hex(i), "address": hex(offset_to_address(i)),
                    "record_size": 12, "fields": ["level_name_pointer", "development_path_pointer", "engine_name_pointer"],
                    "rows": rows}
    raise ValueError("Expected 12-record level/engine table not found")


def compare_psx(roots: list[Path]) -> dict:
    reports = [json.loads(root.with_suffix(".json").read_text(encoding="utf-8")) for root in roots]
    files = [{entry["path"]: entry for entry in report["files"]} for report in reports]
    revisions = []
    for root, report in zip(roots, reports):
        exes = []
        for item in report["files"]:
            path = root / item["path"]
            with path.open("rb") as handle:
                if handle.read(8) == b"PS-X EXE":
                    exes.append(psx_exe(path, root))
        revisions.append({key: report[key] for key in ("cue", "cue_sha256", "image_size", "image_sha256", "volume")}
                         | {"file_count": len(report["files"]),
                            "xa_form2_files": sum(bool(f.get("form2_sectors")) for f in report["files"]),
                            "system_cnf": (root / "SYSTEM.CNF").read_text(encoding="ascii"),
                            "executables": exes})
    common = files[0].keys() & files[1].keys()
    unchanged = [p for p in common if files[0][p]["sha256"] == files[1][p]["sha256"]]
    def payload_digest(entry):
        if entry.get("form2_sectors"):
            return entry["xa_subheader_and_user_data_sha256"]
        return entry["sha256"]
    same_payload = [p for p in common if payload_digest(files[0][p]) == payload_digest(files[1][p])]
    changed = [{"path": p, "original_size": files[0][p]["size"], "rerelease_size": files[1][p]["size"]}
               for p in sorted(common - set(unchanged))]
    executable_comparisons = []
    for i, name in enumerate(("BOOT", "EX/ENGINE1", "EX/ENGINE3", "EX/TITLE")):
        paths = [next(x["path"] for x in rev["executables"] if x["path"].startswith("SLUS"))
                 if i == 0 else name for rev in revisions]
        a, b = [(root / path).read_bytes() for root, path in zip(roots, paths)]
        executable_comparisons.append({"module": name, "same_file_offset_equal_bytes": sum(x == y for x, y in zip(a, b)),
                                       "compared_bytes": min(len(a), len(b)),
                                       "largest_unchanged_raw_spans": unchanged_spans(a, b)})
    project = Path(__file__).resolve().parents[1]
    relationships = []
    for pc, psx in (("TITLE.DLL", "TITLE"), ("ENG1.DLL", "ENGINE1"), ("ENG3.DLL", "ENGINE3")):
        pc_data = (project / "assets/pc/HERCULES" / pc).read_bytes()
        psx_data = (roots[0] / "EX" / psx).read_bytes()
        shared = set(re.findall(rb"[ -~]{5,}", pc_data)) & set(re.findall(rb"[ -~]{5,}", psx_data))
        useful = sorted(s for s in shared if re.search(rb"[A-Z]{4}", s) and b"ABCDE" not in s)
        relationships.append({"pc_module": pc, "psx_module": "EX/" + psx,
                              "shared_readable_strings": [s.decode("ascii") for s in useful],
                              "status": "structural and string evidence; functions not yet mapped"})
    from pe import PE
    pc_title = PE(project / "assets/pc/HERCULES/TITLE.DLL")
    pc_table = level_table(pc_title.data, lambda x: pc_title.rva_to_offset(x - pc_title.image_base),
                          lambda x: pc_title.image_base + pc_title.offset_to_rva(x))
    psx_tables = []
    for root in roots:
        data = (root / "EX/TITLE").read_bytes()
        load = struct.unpack_from("<I", data, 0x18)[0]
        psx_tables.append(level_table(data, lambda x: x - load + 2048, lambda x: x + load - 2048))
    if not pc_table["rows"] == psx_tables[0]["rows"] == psx_tables[1]["rows"]:
        raise ValueError("Level/engine table contents unexpectedly differ")
    table_evidence = {"pc": {k: v for k, v in pc_table.items() if k != "rows"},
                      "psx_original": {k: v for k, v in psx_tables[0].items() if k != "rows"},
                      "psx_rerelease": {k: v for k, v in psx_tables[1].items() if k != "rows"},
                      "identical_decoded_rows": pc_table["rows"]}
    return {"schema": 1, "role": "secondary semantic evidence; PC binaries are the matching targets",
            "revisions": dict(zip(("usa_original", "usa_rerelease"), revisions)),
            "comparison": {"common_paths": len(common), "identical_logical_files": len(unchanged),
                           "identical_file_payloads": len(same_payload),
                           "logical_equal_but_xa_payload_different": sorted(set(unchanged) - set(same_payload)),
                           "changed_logical_files": changed,
                           "only_original": sorted(files[0].keys() - files[1].keys()),
                           "only_rerelease": sorted(files[1].keys() - files[0].keys()),
                           "xa_note": "Logical-file comparisons use 2048-byte views. Full XA comparisons hash each sector's 8-byte subheader plus 2048 (Form 1) or 2324 (Form 2) user bytes, excluding address and EDC/ECC. Raw sector companions preserve full sectors.",
                           "executables": executable_comparisons},
            "pc_relationships": relationships,
            "shared_level_engine_table": table_evidence,
            "limitations": ["No PSX function boundaries or cross-architecture exact correspondences are asserted.",
                            "Unchanged raw spans can include code, data, and padding.",
                            "ISO labels and dates are embedded evidence, not independently verified release dates.",
                            "Sector EDC/ECC and external disc-database hashes have not been validated."]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cue", type=Path, nargs="?")
    parser.add_argument("--extract", type=Path)
    parser.add_argument("--compare-psx", type=Path, nargs=2, metavar=("ORIGINAL_ROOT", "RERELEASE_ROOT"))
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    asset_root = Path(__file__).resolve().parents[1] / "assets"
    for output in (args.extract, args.report):
        if output and output.resolve().is_relative_to(asset_root):
            raise ValueError("Output must not modify immutable assets")
    if args.compare_psx:
        if args.cue or args.extract:
            parser.error("--compare-psx cannot be combined with CUE extraction")
        report = compare_psx(args.compare_psx)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({"report": str(args.report), "identical_logical_files": report["comparison"]["identical_logical_files"]}))
        return
    if args.cue is None:
        parser.error("CUE path or --compare-psx is required")
    disc = Disc(args.cue)
    try:
        if args.extract and args.extract.resolve().is_relative_to(args.cue.resolve().parent):
            raise ValueError("Never extract beside or inside original evidence")
        files = list(disc.walk())
        if args.extract:
            files = [disc.extract(entry, args.extract) for entry in files]
        report = {"schema": 1, "cue": str(args.cue), "cue_sha256": sha256_file(args.cue),
                  "image": str(disc.info["binary"]),
                  "image_size": disc.info["binary"].stat().st_size,
                  "image_sha256": sha256_file(disc.info["binary"]),
                  "tracks": disc.info["tracks"], "volume": disc.volume, "files": files}
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({"report": str(args.report), "files": len(files),
                          "logical_bytes": sum(f["size"] for f in files),
                          "xa_files": sum(bool(f.get("form2_sectors")) for f in files)}))
    finally:
        disc.close()


if __name__ == "__main__":
    main()
