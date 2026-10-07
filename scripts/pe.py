"""Read-only PE evidence adapter. Never loads or executes an inspected image."""
from __future__ import annotations

import datetime
import hashlib
from pathlib import Path

try:
    import env  # optional project dependency path setup
except ImportError:
    pass
import pefile


def _text(value):
    return value.decode("ascii", "replace") if isinstance(value, bytes) else value


class PE:
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        self.pe = pefile.PE(data=self.data, fast_load=False)
        self.image_base = self.pe.OPTIONAL_HEADER.ImageBase
        self.sections = [dict(name=_text(s.Name.rstrip(b"\0")),
                              virtual_address=s.VirtualAddress,
                              virtual_size=s.Misc_VirtualSize,
                              raw_offset=s.PointerToRawData,
                              raw_size=s.SizeOfRawData,
                              characteristics=s.Characteristics)
                         for s in self.pe.sections]

    def rva_to_offset(self, rva):
        if rva < 0:
            raise ValueError("negative RVA")
        if rva < self.pe.OPTIONAL_HEADER.SizeOfHeaders:
            if rva >= len(self.data):
                raise ValueError(f"RVA {rva:#x} outside file")
            return rva
        for s in self.sections:
            delta = rva - s["virtual_address"]
            if 0 <= delta < s["raw_size"]:
                offset = s["raw_offset"] + delta
                if offset >= len(self.data):
                    break
                return offset
        raise ValueError(f"RVA {rva:#x} has no file-backed bytes")

    def offset_to_rva(self, offset):
        for s in self.sections:
            delta = offset - s["raw_offset"]
            if 0 <= delta < s["raw_size"]:
                return s["virtual_address"] + delta
        if 0 <= offset < self.pe.OPTIONAL_HEADER.SizeOfHeaders:
            return offset
        raise ValueError(f"file offset {offset:#x} outside mapped image")

    def read_rva(self, rva, size):
        if size < 0:
            raise ValueError("negative range size")
        offset = self.rva_to_offset(rva)
        if size and self.rva_to_offset(rva + size - 1) != offset + size - 1:
            raise ValueError("range crosses non-contiguous file-backed sections")
        result = self.data[offset:offset + size]
        if len(result) != size:
            raise ValueError("truncated range")
        return result

    def metadata(self):
        p = self.pe
        oh = p.OPTIONAL_HEADER
        imports = []
        for module in getattr(p, "DIRECTORY_ENTRY_IMPORT", []):
            imports.append({"dll": _text(module.dll), "symbols": [
                {"name": _text(i.name), "ordinal": i.ordinal,
                 "iat_rva": i.address - self.image_base}
                for i in module.imports]})
        exports = []
        for e in getattr(getattr(p, "DIRECTORY_ENTRY_EXPORT", None), "symbols", []):
            exports.append({"name": _text(e.name), "ordinal": e.ordinal,
                            "rva": e.address, "forwarder": _text(e.forwarder)})
        resources = []
        def visit(directory, names):
            for e in directory.entries:
                name = str(e.name) if e.name is not None else e.id
                if hasattr(e, "directory"):
                    visit(e.directory, names + [name])
                elif hasattr(e, "data"):
                    d = e.data.struct
                    resources.append({"path": names + [name], "rva": d.OffsetToData,
                                      "size": d.Size, "codepage": d.CodePage,
                                      "sha256": hashlib.sha256(p.get_data(d.OffsetToData, d.Size)).hexdigest()})
        if hasattr(p, "DIRECTORY_ENTRY_RESOURCE"):
            visit(p.DIRECTORY_ENTRY_RESOURCE, [])
        debug = []
        for d in getattr(p, "DIRECTORY_ENTRY_DEBUG", []):
            s = d.struct
            raw = self.data[s.PointerToRawData:s.PointerToRawData+s.SizeOfData]
            debug.append({"type": s.Type, "timestamp": s.TimeDateStamp,
                          "rva": s.AddressOfRawData, "file_offset": s.PointerToRawData,
                          "size": s.SizeOfData, "signature_hex": raw[:4].hex(),
                          "printable": "".join(chr(b) if 32 <= b < 127 else "." for b in raw[:256])})
        versions = []
        for group in getattr(p, "FileInfo", []):
            for info in group:
                for table in getattr(info, "StringTable", []):
                    versions.append({_text(k): _text(v) for k, v in table.entries.items()})
        timestamp = p.FILE_HEADER.TimeDateStamp
        return {"machine": p.FILE_HEADER.Machine, "characteristics": p.FILE_HEADER.Characteristics,
                "timestamp": timestamp,
                "timestamp_utc": datetime.datetime.fromtimestamp(timestamp, datetime.timezone.utc).isoformat(),
                "image_base": self.image_base, "entry_point_rva": oh.AddressOfEntryPoint,
                "linker_version": f"{oh.MajorLinkerVersion}.{oh.MinorLinkerVersion}",
                "subsystem": oh.Subsystem, "image_size": oh.SizeOfImage,
                "section_alignment": oh.SectionAlignment, "file_alignment": oh.FileAlignment,
                "stack_reserve": oh.SizeOfStackReserve, "heap_reserve": oh.SizeOfHeapReserve,
                "checksum": oh.CheckSum, "sections": self.sections,
                "imports": imports, "exports": exports, "resources": resources,
                "debug": debug, "version_info": versions,
                "coff_symbol_table_offset": p.FILE_HEADER.PointerToSymbolTable,
                "coff_symbol_count": p.FILE_HEADER.NumberOfSymbols,
                "rich_header": getattr(p, "RICH_HEADER", None),
                "relocation_count": sum(len(b.entries) for b in getattr(p, "DIRECTORY_ENTRY_BASERELOC", []))}
