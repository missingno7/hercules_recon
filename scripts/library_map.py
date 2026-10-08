"""Diagnostic map of static-library code contributions inside the PC images.

Every code section of every member of the pinned libraries is searched in each
image's executable section. Relocation fields of the library contribution are
masked; every other byte must be equal. A located contribution is identified
library code, not reconstruction, and is never a FUNCTION_MATCH. Output is a
disposable report under work/.
"""
import argparse
import json
from pathlib import Path

from env import ROOT
from match import DEFAULT_TOOLCHAIN, verify_toolchain
from crt_copy_evidence import members
from coff import COFF
from pe import PE

LIBRARIES = ('lib/libcmt.lib', 'mfc/lib/nafxcw.lib')
MODULES = {'HERCULES.EXE': 'work/discs/pc_install/HERCULES.EXE', 'ENG1.DLL': 'assets/pc/HERCULES/ENG1.DLL',
           'ENG3.DLL': 'assets/pc/HERCULES/ENG3.DLL', 'TITLE.DLL': 'assets/pc/HERCULES/TITLE.DLL'}
ANCHOR = 12


def contributions(toolchain):
    scratch = ROOT / 'work/library_map/member.obj'
    scratch.parent.mkdir(parents=True, exist_ok=True)
    for rel in LIBRARIES:
        for index, payload in enumerate(members((toolchain / rel).read_bytes())):
            if payload[:2] != b'\x4c\x01':
                continue
            scratch.write_bytes(payload)
            try:
                coff = COFF(scratch)
            except (ValueError, IndexError, UnicodeDecodeError, KeyError):
                continue
            for number, section in enumerate(coff.sections, 1):
                data = section['data']
                if not section['flags'] & 0x20 or len(data) < 16:
                    continue
                mask = bytearray(len(data))
                for r in section['relocations']:
                    width = 4 if r['type'] in (6, 7, 20) else 2
                    for i in range(r['offset'], min(r['offset'] + width, len(data))):
                        mask[i] = 1
                names = [s['name'] for s in coff.symbols.values()
                         if s['section'] == number and s['storage'] in (2, 3) and s['type'] & 0x20]
                yield dict(library=rel, member=index, names=names, data=data, mask=bytes(mask),
                           relocations=len(section['relocations']))


def anchor(contribution):
    data, mask = contribution['data'], contribution['mask']
    run = 0
    for i, m in enumerate(mask):
        run = run + 1 if not m else 0
        if run == ANCHOR:
            return i - ANCHOR + 1
    return None


def locate(text, base, contribution):
    data, mask = contribution['data'], contribution['mask']
    start = anchor(contribution)
    if start is None:
        return []
    key = data[start:start + ANCHOR]
    found, position = [], text.find(key)
    while position >= 0:
        origin = position - start
        if origin >= 0 and origin + len(data) <= len(text) and all(
                m or text[origin + i] == b for i, (b, m) in enumerate(zip(data, mask))):
            found.append(base + origin)
        position = text.find(key, position + 1)
    return found


def build(toolchain):
    verify_toolchain(toolchain)
    images = {}
    for name, path in MODULES.items():
        pe = PE(ROOT / path)
        text = next(s for s in pe.sections if s['characteristics'] & 0x20000000)
        images[name] = (text['virtual_address'], pe.read_rva(text['virtual_address'],
                                                             min(text['virtual_size'], text['raw_size'])))
    report = {name: [] for name in MODULES}
    for c in contributions(toolchain):
        for name, (base, text) in images.items():
            for rva in locate(text, base, c):
                report[name].append(dict(rva=rva, size=len(c['data']), library=c['library'], member=c['member'],
                                         names=c['names'][:4], relocations=c['relocations']))
    summary = {}
    for name, rows in report.items():
        rows.sort(key=lambda r: (r['rva'], -r['size']))
        covered = set()
        for r in rows:
            covered.update(range(r['rva'], r['rva'] + r['size']))
        summary[name] = dict(contributions=len(rows), covered_bytes=len(covered), text_bytes=len(images[name][1]))
        for r in rows:
            r['rva'] = hex(r['rva'])
    return summary, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/library_map/library_map.json')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('Diagnostic outputs belong in ignored work/')
    summary, report = build(DEFAULT_TOOLCHAIN)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(dict(summary=summary, contributions=report), indent=1) + '\n', encoding='utf-8')
    print(json.dumps(summary, indent=1))


if __name__ == '__main__':
    main()
