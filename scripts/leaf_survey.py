"""Diagnostic leaf survey: spans with no calls, absolute references or base relocations.

Function starts are direct call targets plus code pointers. Spans run to the next
start, so they are leads, not reviewed extents. Spans whose bytes occur in a
pinned static library code contribution are reported as library code; they must
be identified, not reconstructed. Output is a disposable work/ report.
"""
import argparse
import json
import sqlite3
from pathlib import Path

from env import ROOT
from match import DEFAULT_TOOLCHAIN, verify_toolchain
from crt_copy_evidence import members
from coff import COFF
from pe import PE

LIBRARIES = ('lib/libcmt.lib', 'mfc/lib/nafxcw.lib')
ABSOLUTE = ('linear_call', 'linear_memory', 'linear_immediate', 'pe_highlow_pointer')


def library_code(toolchain):
    """Map code bytes of every COMDAT/plain text contribution to its library member."""
    found = []
    for rel in LIBRARIES:
        path = toolchain / rel
        if not path.is_file():
            continue
        for payload in members(path.read_bytes()):
            if payload[:2] != b'\x4c\x01':
                continue
            tmp = ROOT / 'work/leaf_survey/member.obj'
            tmp.parent.mkdir(parents=True, exist_ok=True)
            tmp.write_bytes(payload)
            try:
                coff = COFF(tmp)
            except (ValueError, IndexError, UnicodeDecodeError):
                continue
            for number, section in enumerate(coff.sections, 1):
                if section['flags'] & 0x20 and len(section['data']) >= 8:
                    names = [s['name'] for s in coff.symbols.values()
                             if s['section'] == number and s['storage'] == 2 and s['value'] == 0]
                    found.append((rel + ':' + (names[0] if names else '?'), section['data']))
    return found


def survey(db_path, toolchain):
    verify_toolchain(toolchain)
    db = sqlite3.connect(db_path)
    state = json.loads((ROOT / 'recovery.json').read_text())
    rows = {}
    for row in state['functions']:
        rows.setdefault(Path(row['target']).name.upper(), {})[int(row['rva'], 16)] = (row['id'], row['status'])
    libs = library_code(toolchain)
    report, payloads = [], []
    for name, path in db.execute('select name, path from modules'):
        pe = PE(ROOT / path)
        relocs = {e.rva for b in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', []) for e in b.entries if e.type}
        text = [s for s in pe.sections if s['characteristics'] & 0x20000000]
        lo = min(s['virtual_address'] for s in text)
        hi = max(s['virtual_address'] + min(s['virtual_size'], s['raw_size']) for s in text)
        starts = sorted({t for (t,) in db.execute(
            "select distinct target_rva from refs where module=? and kind in ('linear_call','pe_highlow_pointer')",
            (name,)) if lo <= t < hi and (t & 15) == 0})
        for i, start in enumerate(starts):
            end = starts[i + 1] if i + 1 < len(starts) else hi
            if any(start <= r < end for r in relocs):
                continue
            refs = db.execute('select count(*) from refs where module=? and source_rva>=? and source_rva<? '
                              'and target_rva>0 and kind in (%s)' % ','.join('?' * len(ABSOLUTE)), (name, start, end, *ABSOLUTE)).fetchone()[0]
            if refs:
                continue
            outside = db.execute("select count(*) from refs where module=? and source_rva>=? and source_rva<? "
                                 "and kind='linear_branch' and (target_rva<? or target_rva>=?)",
                                 (name, start, end, start, end)).fetchone()[0]
            if outside:
                continue
            data = pe.read_rva(start, end - start)
            body = data.rstrip(b'\x90')
            library = sorted({rel for rel, code in libs if code.rstrip(b'\x90') == body or
                              (len(code) >= 16 and body.startswith(code.rstrip(b'\x90')))})
            existing = rows.get(name.upper(), {}).get(start)
            report.append(dict(module=name, rva=hex(start), size=end - start, body_size=len(body),
                               library=library, existing=existing))
            payloads.append(data)
    groups = {}
    for row, data in zip(report, payloads):
        groups.setdefault(data, []).append(row['module'] + ':' + row['rva'])
    for row, data in zip(report, payloads):
        row['identical'] = [x for x in groups[data] if x != row['module'] + ':' + row['rva']]
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--db', type=Path, default=ROOT / 'work/archaeology.sqlite')
    parser.add_argument('--output', type=Path, default=ROOT / 'work/leaf_survey/leaves.json')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('Diagnostic outputs belong in ignored work/')
    report = survey(args.db, DEFAULT_TOOLCHAIN)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=1) + '\n')
    open_rows = [r for r in report if not r['library'] and not r['existing']]
    print(f'{len({r["module"] + r["rva"] for r in open_rows if not any(i < r["module"] + ":" + r["rva"] for i in r["identical"])})} distinct open; ', end='')
    print(f'{len(report)} leaf spans; {sum(1 for r in report if r["library"])} library; '
          f'{sum(1 for r in report if r["existing"])} in recovery.json; {len(open_rows)} open '
          f'({sum(r["size"] for r in open_rows)} bytes); report: {output}')


if __name__ == '__main__':
    main()
