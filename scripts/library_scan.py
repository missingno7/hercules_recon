"""Library attribution audit (diagnostic; outputs under work/libaudit). See evidence/library_attribution_2026_10_20.json.

Locates every code section of every member of candidate historical libraries in
each module's .text, masking only relocation fields. Same-section relocations are
resolved and checked. Results are pickled per library revision.
"""
import json, pickle, struct, sys, hashlib
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / 'scripts'))
import env  # noqa
import pefile

OUT = REPO / 'work/libaudit'
MODULES = {'HERCULES.EXE': REPO / 'work/discs/pc_install/HERCULES.EXE',
           'ENG1.DLL': REPO / 'assets/pc/HERCULES/ENG1.DLL',
           'ENG3.DLL': REPO / 'assets/pc/HERCULES/ENG3.DLL',
           'TITLE.DLL': REPO / 'assets/pc/HERCULES/TITLE.DLL'}
RTM = Path(r'C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953')
SP2 = Path(r'C:\tools\hercules\msvc500sp2-4ebf02022705b4c9e9108d3ed3f286ed80ba2ed9')
SP3 = Path(r'C:\tools\msvc5_sp3\VC')
VC6 = Path(r'C:\tools\msvc6_rtm\VC98')
LIBS = {
    'rtm/libcmt': RTM / 'lib/libcmt.lib', 'sp2/libcmt': SP2 / 'lib/libcmt.lib',
    'sp3/libcmt': SP3 / 'LIB/LIBCMT.LIB', 'vc6/libcmt': VC6 / 'LIB/LIBCMT.LIB',
    'rtm/libc': RTM / 'lib/libc.lib',
    'rtm/libcpmt': RTM / 'lib/libcpmt.lib', 'rtm/libcimt': RTM / 'lib/libcimt.lib',
    'rtm/nafxcw': RTM / 'mfc/lib/nafxcw.lib', 'sp2/nafxcw': SP2 / 'mfc/lib/nafxcw.lib',
    'sp3/nafxcw': SP3 / 'MFC/LIB/NAFXCW.LIB', 'vc6/nafxcw': VC6 / 'MFC/LIB/NAFXCW.LIB',
    'rtm/uuid': RTM / 'lib/uuid.lib',
    'rtm/ctl3d32s': RTM / 'lib/ctl3d32s.lib', 'sp2/ctl3d32s': SP2 / 'lib/ctl3d32s.lib', 'sp3/ctl3d32s': SP3 / 'LIB/CTL3D32S.LIB', 'vc6/ctl3d32s': VC6 / 'LIB/CTL3D32S.LIB',
}


def members(data):
    assert data[:8] == b'!<arch>\n'
    pos, longnames = 8, b''
    while pos < len(data):
        h = data[pos:pos + 60]
        size = int(h[48:58]); name = h[:16].rstrip()
        payload = data[pos + 60:pos + 60 + size]
        if name == b'//':
            longnames = payload
        elif name != b'/':
            if name.startswith(b'/') and name[1:].isdigit():
                o = int(name[1:]); e = longnames.find(b'\0', o)
                e2 = longnames.find(b'\n', o)
                ends = [x for x in (e, e2) if x >= 0]
                name = longnames[o:min(ends) if ends else len(longnames)]
            yield name.rstrip(b'/').decode('latin1'), payload
        pos += 60 + size + (size & 1)


def coff_sections(obj):
    if obj[:2] != b'\x4c\x01':
        return
    _, count, _, symptr, nsyms, optsize, _ = struct.unpack_from('<HHIIIHH', obj)
    if optsize:
        return
    strptr = symptr + 18 * nsyms
    def nm(raw):
        if raw[:4] == b'\0\0\0\0':
            o = strptr + struct.unpack_from('<I', raw, 4)[0]
            return obj[o:obj.index(b'\0', o)].decode('latin1')
        return raw.rstrip(b'\0').decode('latin1')
    syms, i = {}, 0
    while i < nsyms:
        off = symptr + 18 * i
        raw, value, sec, typ, storage, aux = struct.unpack_from('<8sIhHBB', obj, off)
        syms[i] = (nm(raw), value, sec, typ, storage)
        i += 1 + aux
    for n in range(count):
        raw, _, _, size, ptr, relptr, _, nrel, _, flags = struct.unpack_from('<8sIIIIIIHHI', obj, 20 + 40 * n)
        if not flags & 0x20 or not ptr or not size:
            continue
        data = obj[ptr:ptr + size]
        relocs = []
        for j in range(nrel):
            o, s, t = struct.unpack_from('<IIH', obj, relptr + 10 * j)
            relocs.append((o, syms.get(s, ('?', 0, 0, 0, 0)), t))
        names = sorted({s[0] for s in syms.values() if s[2] == n + 1 and s[4] in (2, 3) and s[0] and not s[0].startswith('.') and not s[0].startswith('$')},
                       key=lambda x: (not x.startswith('_') and not x.startswith('?'), x))
        publics = [s[0] for s in syms.values() if s[2] == n + 1 and s[4] == 2]
        yield dict(section=n + 1, data=data, relocs=relocs, names=names, publics=publics,
                   comdat=bool(flags & 0x1000), local_syms={s[0]: s[1] for s in syms.values() if s[2] == n + 1})


def mask_of(c):
    m = bytearray(len(c['data']))
    for o, _, t in c['relocs']:
        w = 2 if t in (10,) else 4
        for k in range(o, min(o + w, len(m))):
            m[k] = 1
    return bytes(m)


def best_anchor(data, mask):
    best, run, start = (0, 0), 0, 0
    for i, mm in enumerate(mask):
        if mm:
            run = 0
            continue
        if run == 0:
            start = i
        run += 1
        if run > best[1]:
            best = (start, run)
    s, l = best
    return s, min(l, 48)


def load_images():
    images = {}
    for name, path in MODULES.items():
        pe = pefile.PE(str(path), fast_load=False)
        t = next(s for s in pe.sections if s.Characteristics & 0x20000000)
        size = min(t.Misc_VirtualSize, t.SizeOfRawData)
        images[name] = dict(base=t.VirtualAddress, text=t.get_data()[:size], ib=pe.OPTIONAL_HEADER.ImageBase,
                            sha=hashlib.sha256(path.read_bytes()).hexdigest())
    return images


def scan(tag, path, images):
    rows = []
    blob = path.read_bytes()
    for mname, payload in members(blob):
        try:
            secs = list(coff_sections(payload))
        except Exception:
            continue
        for c in secs:
            data = c['data']
            if len(data) < 6:
                continue
            mask = mask_of(c)
            a, alen = best_anchor(data, mask)
            if alen < 6:
                continue
            key = data[a:a + alen]
            for mod, im in images.items():
                text, base, ib = im['text'], im['base'], im['ib']
                p = text.find(key)
                while p >= 0:
                    org = p - a
                    if org >= 0 and org + len(data) <= len(text) and all(mm or text[org + i] == b for i, (b, mm) in enumerate(zip(data, mask))):
                        # check same-section relocations
                        ok, checked, ext = True, 0, []
                        for o, sym, t in c['relocs']:
                            sname, sval, ssec = sym[0], sym[1], sym[2]
                            field = int.from_bytes(text[org + o:org + o + 4], 'little')
                            if t == 20:
                                dest = (base + org + o + 4 + field - (int.from_bytes(data[o:o+4], 'little'))) & 0xffffffff
                                if ssec == c['section']:
                                    checked += 1
                                    ok &= dest == base + org + sval
                                else:
                                    ext.append((sname, dest))
                            elif t == 6:
                                dest = (field - int.from_bytes(data[o:o+4], 'little') - ib) & 0xffffffff
                                if ssec == c['section']:
                                    checked += 1
                                    ok &= dest == base + org + sval
                                else:
                                    ext.append((sname, dest))
                        rows.append(dict(module=mod, rva=base + org, size=len(data), lib=tag, member=mname,
                                         section=c['section'], names=c['names'][:6], publics=c['publics'][:6],
                                         relocs=len(c['relocs']), fixed=len(data) - sum(mask), anchor=alen,
                                         internal_checked=checked, internal_ok=ok, ext=ext,
                                         sha=hashlib.sha1(data).hexdigest(), msha=hashlib.sha1(bytes(b if not mm else 0 for b, mm in zip(data, mask))).hexdigest()))
                    p = text.find(key, p + 1)
    return rows


def main():
    images = load_images()
    tags = sys.argv[1:] or list(LIBS)
    for tag in tags:
        rows = scan(tag, LIBS[tag], images)
        (OUT / 'raw').mkdir(exist_ok=True, parents=True)
        (OUT / 'raw' / (tag.replace('/', '_') + '.pkl')).write_bytes(pickle.dumps(rows))
        print(tag, len(rows), {m: sum(1 for r in rows if r['module'] == m) for m in MODULES}, flush=True)


if __name__ == '__main__':
    main()
