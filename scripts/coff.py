"""Minimal i386 COFF object reader, with explicit relocation reporting."""
import struct
from pathlib import Path


class COFF:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        d = self.data
        machine, count, _, symptr, nsyms, optsize, _ = struct.unpack_from("<HHIIIHH", d)
        if machine != 0x14c or optsize:
            raise ValueError("Expected a plain i386 COFF object")
        strptr = symptr + 18 * nsyms
        def name(raw):
            if raw[:4] == b"\0" * 4:
                offset = struct.unpack_from("<I", raw, 4)[0]
                start = strptr + offset
                return d[start:d.index(b"\0", start)].decode("ascii")
            return raw.rstrip(b"\0").decode("ascii")
        self.symbols = {}
        i = 0
        while i < nsyms:
            offset = symptr + i * 18
            raw, value, section, typ, storage, auxcount = struct.unpack_from("<8sIhHBB", d, offset)
            self.symbols[i] = {"name": name(raw), "value": value, "section": section,
                               "type": typ, "storage": storage,
                               "aux": [d[offset+18*(a+1):offset+18*(a+2)] for a in range(auxcount)]}
            i += 1 + auxcount
        self.sections = []
        for i in range(count):
            fields = struct.unpack_from("<8sIIIIIIHHI", d, 20 + 40 * i)
            raw, _, _, size, ptr, relptr, _, nrel, _, flags = fields
            if (ptr and ptr + size > len(d)) or (size and flags & 0x20 and not ptr):
                raise ValueError("Truncated or missing COFF section bytes")
            if nrel and (not relptr or relptr + 10 * nrel > len(d)):
                raise ValueError("Truncated COFF relocations")
            relocs = []
            for j in range(nrel):
                offset, symbol, typ = struct.unpack_from("<IIH", d, relptr + 10 * j)
                relocs.append({"offset": offset, "symbol": self.symbols[symbol]["name"], "type": typ})
            self.sections.append({"name": name(raw), "data": d[ptr:ptr+size] if ptr else b"", "size": size,
                                  "relocations": relocs, "flags": flags})

    def function(self, name):
        choices = [s for s in self.symbols.values() if s["name"] == name and s["section"] > 0]
        if len(choices) != 1:
            raise ValueError(f"Expected one defined symbol {name}, found {len(choices)}")
        symbol = choices[0]
        section = self.sections[symbol["section"] - 1]
        if not symbol["type"] & 0x20 or section["flags"] & 0x20000020 != 0x20000020:
            raise ValueError("Expected a function symbol in executable code")
        # /Gy gives an independently delimited COMDAT code contribution. Reject
        # ambiguous shared text instead of guessing boundaries from RET bytes.
        funcs = [s for s in self.symbols.values() if s["section"] == symbol["section"] and s["type"] & 0x20]
        if not section["flags"] & 0x1000 or symbol["value"] != 0 or len(funcs) != 1:
            raise ValueError("Function must occupy one /Gy COMDAT code section at offset zero")
        return section
