"""Read-only PE evidence adapter. Never loads or executes an inspected image."""
from __future__ import annotations

import datetime
import hashlib
import os
from collections import OrderedDict
from operator import itemgetter
from threading import RLock
from pathlib import Path

try:
    import env  # optional project dependency path setup
except ImportError:
    pass
import pefile


def _text(value):
    return value.decode("ascii", "replace") if isinstance(value, bytes) else value


def _private_record(self):
    value = pefile.PE(data=self[0], fast_load=False)
    for attribute, key in self[1]:
        value = getattr(value, key) if attribute else value[key]
    return value


def _record_attribute(self, name):
    if name.startswith('_') or name not in self[2]:
        raise AttributeError(name)
    for field, value in self[3]:
        if field == name:
            return value
    # Methods/back-references run on a private parse, including lazy writes.
    return getattr(_private_record(self), name)


def _record_string(self):
    return str(_private_record(self))


def _record_repr(self):
    return '<private PE record>'


def _freeze_parse(value, trail=(), memo=None):
    """Cache only builtin immutable primitives; never records or record classes."""
    if value is None or isinstance(value, (bool, int, float, str, bytes)):
        return value
    if memo is None:
        memo = {}
    key = id(value)
    if key not in memo:
        def freeze(item, part):
            return _freeze_parse(item, trail + (part,), memo)
        if isinstance(value, dict):
            result = (2, tuple((key, freeze(item, (False, key)))
                               for key, item in value.items()))
        elif isinstance(value, (list, tuple)):
            result = (0 if isinstance(value, list) else 1,
                      tuple(freeze(item, (False, index)) for index, item in enumerate(value)))
        else:
            names = tuple(name for name in sorted(dir(value)) if not name.startswith('_'))
            fields = {name: getattr(value, name) for name in names}
            dictionaries = tuple((name, freeze(item, (True, name)))
                                 for name, item in fields.items() if isinstance(item, dict))
            fields = {name: item for name, item in fields.items()
                      if not callable(item) and not isinstance(item, (dict, pefile.PE))}
            result = (3, trail, names, tuple(fields), dictionaries,
                      tuple(freeze(item, (True, name)) for name, item in fields.items()))
        memo[key] = result
    return memo[key]


def _consumer_view(snapshot, data):
    # Every construction owns its types, descriptors and containers. In
    # particular, type(record).type = 0 cannot change any later construction.
    types, memo = {}, {}
    def iterate(self):
        return (view(item) for item in tuple.__iter__(self))
    def item(self, index):
        value = tuple.__getitem__(self, index)
        return tuple(view(part) for part in value) if isinstance(index, slice) else view(value)
    sequence = type('_PESequence', (tuple,), dict(__slots__=(), __iter__=iterate, __getitem__=item))
    def field(index):
        return property(lambda self: view(self[index]))
    def view(facts, dictionary=False):
        if type(facts) is not tuple:
            return facts
        key = (id(facts), dictionary)
        if key not in memo:
            kind = facts[0]
            if kind in (0, 1):
                if dictionary:
                    values = (view(item, True) for item in facts[1])
                    result = list(values) if kind == 0 else tuple(values)
                else:
                    result = sequence(facts[1])
            elif kind == 2:
                result = {key: view(item, True) for key, item in facts[1]}
            else:
                _, trail, names, shape, dictionaries, fields = facts
                structured = tuple(type(item) is tuple for item in fields)
                type_key = shape, structured
                if type_key not in types:
                    attributes = dict(__slots__=(), _private=_private_record,
                                      __getattr__=_record_attribute, __str__=_record_string,
                                      __repr__=_record_repr)
                    attributes.update((name, field(index + 4) if structured[index] else property(itemgetter(index + 4)))
                                      for index, name in enumerate(shape))
                    # No shared Python base class or generated namedtuple base:
                    # the only base reachable through a record is builtin tuple.
                    types[type_key] = type('_PERecord', (tuple,), attributes)
                result = types[type_key]((data, trail, names,
                    tuple((name, view(item, True)) for name, item in dictionaries),
                    *fields))
            memo[key] = result
        return memo[key]
    return view(snapshot)


# Process-local only; each constructor hashes its freshly read bytes. Values
# contain only None/bool/int/float/str/bytes/tuple, with no shared parser handles.
_PARSES = OrderedDict()
_PARSE_LOCK = RLock()


def _parsed_image(path, data):
    key = (path.resolve(), hashlib.sha256(data).digest())
    if os.environ.get('HERCULES_PE_CACHE') == '0':
        return pefile.PE(data=data, fast_load=False)
    with _PARSE_LOCK:
        if key not in _PARSES:
            _PARSES[key] = _freeze_parse(pefile.PE(data=data, fast_load=False))
        _PARSES.move_to_end(key)
        if len(_PARSES) > 16:
            _PARSES.popitem(last=False)
        snapshot = _PARSES[key]
    return _consumer_view(snapshot, data)

class PE:
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        self.pe = _parsed_image(self.path, self.data)
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
