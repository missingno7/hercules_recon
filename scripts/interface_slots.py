"""Reproduce the fixed-slot review from static PC bytes; never execute an image.

The catalog supplies independently reviewed entry extents and annotated physical
call stack effects (not inferred prototypes). All reads, returns, tails, callers,
wrapper forms, loader copies and address audits are remeasured here.
"""
import argparse
import copy
import hashlib
import json
import struct
from collections import Counter

from env import ROOT
from pe import PE
import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG
from fixed_slot import REVIEW, ROOT_SYMBOL, wrapper_shape

CATALOG = 'evidence/interface_slot_catalog_2026_10_26.json'
IMAGES = {
    'HERCULES.EXE': ('work/discs/pc_install/HERCULES.EXE', '587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'),
    'ENG1.DLL': ('assets/pc/HERCULES/ENG1.DLL', '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'),
    'ENG3.DLL': ('assets/pc/HERCULES/ENG3.DLL', 'aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7'),
    'TITLE.DLL': ('assets/pc/HERCULES/TITLE.DLL', 'b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b'),
}
ROOTS = {'ENG1.DLL': 0x70f20, 'ENG3.DLL': 0x407a0, 'TITLE.DLL': 0x2bb40}
TABLE, POINTER, TABLE_SIZE = 0x361b8, 0x365a0, 1000


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def write_json(path, document):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(document, indent=2) + '\n', encoding='utf-8', newline='\n')


def decoder():
    d = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32); d.detail = True
    return d


def code_inventory(pe, entries):
    # Skip undecodable islands, but never silently stop auditing a section.
    d = decoder(); d.skipdata = True
    result = []
    for s in pe.sections:
        if not s['characteristics'] & 0x20000000: continue
        start = s['virtual_address']; end = start + min(s['raw_size'], s['virtual_size'])
        boundaries = sorted({start, end} | {at for at in entries if start < at < end})
        for low, high in zip(boundaries, boundaries[1:]):
            result.extend(i for i in d.disasm(pe.read_rva(low, high - low), low) if i.id)
    return result


def audit(pe, code, start, length):
    low, high = pe.image_base + start, pe.image_base + start + length
    fields, writes, immediates, refs = set(), [], [], 0
    for i in code:
        for op in i.operands:
            if op.type == X86_OP_MEM:
                value = op.mem.disp & 0xffffffff
                if low <= value < high:
                    refs += 1
                    fields.add(i.address + i.disp_offset)
                    if op.access & cs.CS_AC_WRITE:
                        writes.append(i.address)
                    if op.mem.base or op.mem.index:
                        raise ValueError('Indexed alias into audited table')
            elif op.type == X86_OP_IMM and low <= op.imm < high:
                refs += 1; fields.add(i.address + i.imm_offset); immediates.append(i.address)
    raw_refs = []
    for s in pe.sections:
        raw = pe.read_rva(s['virtual_address'], s['raw_size'])
        raw_refs.extend(s['virtual_address'] + n for n in range(len(raw) - 3)
                        if low <= struct.unpack_from('<I', raw, n)[0] < high)
    return dict(decoded_address_references=refs, raw_unaligned_address_occurrences=len(raw_refs),
                raw_occurrences_not_operands=sorted(set(raw_refs) - fields),
                direct_writes=sorted(set(writes)), address_immediates=sorted(set(immediates)))


def loader_chain(images, inventories):
    host = images['HERCULES.EXE']
    if struct.unpack('<I', host.read_rva(POINTER, 4))[0] != host.image_base + TABLE:
        raise ValueError('Static table pointer differs')
    # Exact instructions establish GetProcAddress -> saved entry -> CALL EAX,
    # and unmodified source-cell load -> PUSH EDX (first argument at offset zero).
    expected = {0x5c75: 'ff159c58ba00', 0x5c7b: 'a368c49100',
                0x5c89: '8b1568c49100', 0x5c8f: '6868684300', 0x5c94: '52', 0x5c95: 'ff15ac58ba00',
                0x5c9d: 'a35cc49100', 0x5eea: 'a15cc49100',
                0x5ef3: '8b15a0654300', 0x5ef9: '33c9', 0x5efb: '8a0dfc01ba00',
                0x5f01: '51', 0x5f02: '52', 0x5f03: 'ffd0', 0x5f05: '83c408'}
    for at, raw in expected.items():
        if host.read_rva(at, len(bytes.fromhex(raw))).hex() != raw:
            raise ValueError('Loader argument/entry chain differs')
    if host.read_rva(0x36868, 17) != b'PC_DLLEngineMain\0':
        raise ValueError('Wrong export name')
    imports = {i.address: i.name for m in host.pe.DIRECTORY_ENTRY_IMPORT for i in m.imports}
    if imports.get(0xba58ac) != b'GetProcAddress' or imports.get(0xba589c) != b'LoadLibraryA':
        raise ValueError('Wrong loader IAT identities')
    modules = {}
    for name, root in ROOTS.items():
        pe = images[name]
        entry = next(e.address for e in pe.pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name == b'PC_DLLEngineMain')
        prefix = pe.read_rva(entry, 0x80)
        guard = struct.unpack_from('<I', prefix, 1)[0] - pe.image_base
        if (prefix[0:1] != b'\xa1' or prefix[5:11] != bytes.fromhex('5685c0577452') or
                prefix[11:26] != bytes.fromhex('8b54240c85d27463b9fa0000008bf2') or
                prefix[26:33] != b'\xbf' + struct.pack('<I', pe.image_base + root) + b'\xf3\xa5' or
                prefix[33:43] != b'\x81\x3d' + struct.pack('<I', pe.image_base + root) + bytes.fromhex('ac0cc05a') or
                prefix[79:91] != bytes.fromhex('33c083c40885c0a3') + struct.pack('<I', pe.image_base + guard) or
                struct.unpack('<I', pe.read_rva(guard, 4))[0] != 1):
            raise ValueError('DLL first-use copy chain differs: ' + name)
        if not any(s['virtual_address'] + s['raw_size'] <= root and root + 1000 <= s['virtual_address'] + s['virtual_size'] for s in pe.sections):
            raise ValueError('Copied root is not loader zero fill')
        mutation = audit(pe, inventories[name], root, 1000)
        if mutation['direct_writes'] or mutation['raw_occurrences_not_operands'] or mutation['address_immediates'] != [entry + 26]:
            raise ValueError('Additional root writer/alias: ' + name)
        modules[name] = dict(root_rva=root, export_rva=entry, first_use_guard_rva=guard,
                             guard_initial_value=1, source_argument=1, source_offset=0,
                             source_setup_rva=entry + 24, count_setup_rva=entry + 19,
                             destination_setup_rva=entry + 26, copy_rva=entry + 31,
                             copy_dwords=250, copy_before_signature_validation=True,
                             entry_size=128, entry_sha256=digest(prefix), mutation_audit=mutation)
    mutation = audit(host, inventories['HERCULES.EXE'], TABLE, 1004)
    if mutation['direct_writes'] or mutation['address_immediates'] or mutation['raw_occurrences_not_operands'] != [POINTER]:
        raise ValueError('Additional static table writer/alias')
    return modules, dict(table_rva=TABLE, table_size=1000, table_sha256=digest(host.read_rva(TABLE, 1000)),
                         pointer_cell_rva=POINTER, source_offset=0, dispatch_rva=0x5f03,
                         loader_spans=[dict(rva=at, size=size, sha256=digest(host.read_rva(at, size)))
                                       for at, size in [(0x5c20, 176), (0x5cd0, 528), (0x5eea, 30)]],
                         mutation_audit=mutation)


class NativeABI:
    def __init__(self, host, code, effects):
        self.host, self.code, self.effects = host, {i.address: i for i in code}, effects
        self.pop_cache = {}
        for at, row in effects.items():
            if digest(host.read_rva(row['window_rva'], row['window_size'])) != row['window_sha256']:
                raise ValueError('Reviewed stack-effect witness changed: ' + at)

    def pop(self, entry, active=()):
        """Enumerate native RET immediates across local branches and tail chains."""
        if entry in self.pop_cache:
            return self.pop_cache[entry]
        if entry in active:
            raise ValueError('Recursive native tail')
        seen, pending, values = set(), [entry], set()
        while pending:
            at = pending.pop()
            if at in seen:
                continue
            seen.add(at)
            if len(seen) > 4096 or at not in self.code:
                raise ValueError('Unbounded native RET inventory')
            i = self.code[at]
            if i.mnemonic == 'ret':
                values.add(i.operands[0].imm if i.operands else 0); continue
            if i.group(cs.CS_GRP_JUMP):
                if i.operands[0].type != X86_OP_IMM:
                    raise ValueError('Computed native tail')
                pending.append(i.operands[0].imm)
                if i.mnemonic == 'jmp':
                    continue
            pending.append(at + i.size)
        if len(values) != 1:
            raise ValueError('Ambiguous native RET')
        self.pop_cache[entry] = next(iter(values))
        return self.pop_cache[entry]

    def measure(self, entry, extent, owned):
        reads, writes, returns, own_returns, tails, unresolved, controls = {}, {}, set(), set(), {}, set(), {}
        pending, seen = [(entry, 0, None)], set()
        while pending:
            at, esp, ebp = pending.pop(); state = (at, esp, ebp)
            if state in seen:
                continue
            seen.add(state)
            if len(seen) > 20000 or at not in self.code or not entry <= at < entry + extent:
                unresolved.add('CFG leaves independent extent'); continue
            i = self.code[at]; ops = i.operands
            for op in ops:
                if op.type != X86_OP_MEM or i.mnemonic == 'lea':
                    continue
                m = op.mem; base = i.reg_name(m.base)
                if base not in ('esp', 'ebp'):
                    continue
                delta = esp if base == 'esp' else ebp
                # EBP is also an ordinary callee-saved register in /Oy code.
                if base == 'ebp' and delta is None: continue
                if delta is None or m.index or m.segment:
                    unresolved.add('unknown stack address at ' + hex(at)); continue
                offset = delta + m.disp
                if offset >= 4:
                    row = dict(rva=at, entry_esp_offset=offset, size=op.size,
                               argument_word=(offset + op.size - 1) // 4,
                               instruction_sha256=digest(bytes(i.bytes)))
                    if op.access & cs.CS_AC_READ: reads[(at, offset)] = row
                    if op.access & cs.CS_AC_WRITE: writes[(at, offset)] = row
            if i.mnemonic == 'ret':
                returns.add(ops[0].imm if ops else 0)
                own_returns.add(ops[0].imm if ops else 0)
                if esp != 0: unresolved.add('unbalanced return at ' + hex(at))
                continue
            if i.group(cs.CS_GRP_CALL):
                override = self.effects.get(hex(at))
                try:
                    pop = override['pop'] if override else self.pop(ops[0].imm) if ops[0].type == X86_OP_IMM else None
                    if pop is None: raise ValueError('unreviewed indirect call')
                    esp += pop
                    controls[at] = dict(rva=at, pop=pop, basis=override['basis'] if override else 'native RET inventory')
                except ValueError:
                    unresolved.add('unknown call stack effect at ' + hex(at))
                    continue
            elif i.mnemonic == 'push': esp -= 4
            elif i.mnemonic == 'pop':
                if ops[0].type != X86_OP_REG or i.reg_name(ops[0].reg) == 'esp':
                    unresolved.add('unsupported POP at ' + hex(at)); continue
                if i.reg_name(ops[0].reg) == 'ebp': ebp = None
                esp += 4
            elif len(ops) >= 2 and ops[0].type == X86_OP_REG and ops[0].access & cs.CS_AC_WRITE:
                reg = i.reg_name(ops[0].reg)
                if reg in ('esp', 'ebp'):
                    if i.mnemonic in ('add', 'sub') and reg == 'esp' and ops[1].type == X86_OP_IMM:
                        esp += ops[1].imm * (1 if i.mnemonic == 'add' else -1)
                    elif i.mnemonic == 'mov' and ops[1].type == X86_OP_REG and i.reg_name(ops[1].reg) in ('esp', 'ebp'):
                        value = esp if i.reg_name(ops[1].reg) == 'esp' else ebp
                        if value is None: unresolved.add('unknown frame at ' + hex(at)); continue
                        if reg == 'esp': esp = value
                        else: ebp = value
                    elif i.mnemonic == 'lea' and reg == 'esp' and ops[1].type == X86_OP_MEM and i.reg_name(ops[1].mem.base) == 'esp' and not ops[1].mem.index:
                        esp += ops[1].mem.disp
                    elif reg == 'ebp':
                        ebp = None
                    else:
                        unresolved.add('unsupported stack update at ' + hex(at)); continue
            if i.group(cs.CS_GRP_JUMP):
                if ops[0].type != X86_OP_IMM:
                    unresolved.add('computed tail at ' + hex(at)); continue
                dest = ops[0].imm
                if not entry <= dest < entry + extent:
                    tails[at] = dict(rva=at, target_rva=dest, entry_esp_delta=esp)
                    if esp != 0: unresolved.add('unbalanced tail at ' + hex(at))
                    try: returns.add(self.pop(dest))
                    except ValueError: unresolved.add('unresolved native tail at ' + hex(at))
                else: pending.append((dest, esp, ebp))
                if i.mnemonic == 'jmp': continue
            pending.append((at + i.size, esp, ebp))
        if not owned: unresolved.add('no independent ledger entry/extent')
        if not returns: unresolved.add('no closed native return')
        own_code = [i for at,i in self.code.items() if entry <= at < entry + extent]
        return dict(stack_reads=sorted(reads.values(), key=lambda x:(x['rva'], x['entry_esp_offset'])),
                    stack_writes=sorted(writes.values(), key=lambda x:(x['rva'], x['entry_esp_offset'])),
                    argument_words=sorted({x['argument_word'] for x in reads.values()}),
                    ret_immediates=sorted({i.operands[0].imm if i.operands else 0 for i in own_code if i.mnemonic == 'ret'}),
                    effective_ret_immediates=sorted(returns), tail_transfers=list(tails.values()),
                    native_direct_tail_forms=[dict(rva=i.address, target_rva=i.operands[0].imm)
                        for i in own_code if i.mnemonic == 'jmp' and i.operands[0].type == X86_OP_IMM and
                        not entry <= i.operands[0].imm < entry + extent],
                    call_stack_effects=list(controls.values()), unresolved=sorted(unresolved))


def derive():
    catalog = json.loads((ROOT / CATALOG).read_bytes())
    images = {name: PE(ROOT / path) for name, (path, _) in IMAGES.items()}
    for name, (_, sha) in IMAGES.items():
        if digest(images[name].data) != sha: raise ValueError('Changed original: ' + name)
    inventories = {name: code_inventory(pe, catalog['code_entries'][name]) for name, pe in images.items()}
    modules, loader = loader_chain(images, inventories)
    host = images['HERCULES.EXE']; native = NativeABI(host, inventories['HERCULES.EXE'], catalog['call_stack_effects'])
    providers = {}
    for text, row in catalog['providers'].items():
        entry = int(text, 0)
        abi = native.measure(entry, row['extent'], bool(row['id']))
        callers = [i.address for i in inventories['HERCULES.EXE'] if
                   (i.group(cs.CS_GRP_CALL) or i.mnemonic == 'jmp') and i.operands[0].type == X86_OP_IMM and
                   i.operands[0].imm == entry and not entry <= i.address < entry + row['extent']]
        providers[entry] = dict(id=row['id'], extent=row['extent'], native_sha256=digest(host.read_rva(entry, row['extent'])),
                                abi=abi, independent_EXE_callers=callers)
    slots, wrappers = {}, {}
    for row in catalog['wrappers']:
        name = row['target'].split('/')[-1].upper(); pe = images[name]; at = int(row['rva'], 0)
        shape = wrapper_shape(pe.read_rva(at, row['size']), at)
        slot = shape.pop('address') - pe.image_base - ROOTS[name]
        if not 0 <= slot <= 996 or slot % 4: raise ValueError('Invalid wrapper slot')
        provider = struct.unpack('<I', host.read_rva(TABLE + slot, 4))[0] - host.image_base
        own = providers[provider]; abi = own['abi']
        verdict = 'unresolved' if abi['unresolved'] or abi['effective_ret_immediates'] != [0] else 'agree'
        if shape['transfer'] == 'call' and max(abi['argument_words'], default=0) > shape['pushed_dwords']: verdict = 'conflict'
        record = slots.setdefault(hex(slot), dict(slot_offset=slot, static_cell_rva=TABLE + slot, provider_rva=provider,
                        provider_id=own['id'], provider_extent=own['extent'], provider_sha256=own['native_sha256'],
                        provider_abi=abi, independent_EXE_caller_count=len(own['independent_EXE_callers']),
                        independent_EXE_callers=own['independent_EXE_callers'], modules={}))
        module = record['modules'].setdefault(name, dict(verdict=verdict, forms=[]))
        if module['verdict'] != verdict: raise ValueError('Mixed module/slot ABI verdict')
        form = {key: shape[key] for key in ('transfer', 'pushed_dwords', 'cleanup_bytes')}
        if form not in module['forms']: module['forms'].append(form)
        wrappers[row['id']] = dict(module=name, rva=at, size=row['size'], slot=slot, verdict=verdict, **shape)
    counts = Counter(w['verdict'] for w in wrappers.values())
    return dict(schema=1, profile='fixed-slot-v1',
                question='Does this statically copied slot preserve the measured native provider stack boundary?',
                scope='Physical stack compatibility only; not C prototypes, types, semantics, dynamic callback behavior or arbitrary computed writes. Static bytes only; no original execution.',
                reproduction='python scripts/interface_slots.py review',
                derivation_inputs={CATALOG:digest((ROOT / CATALOG).read_bytes()),
                                   'scripts/interface_slots.py':digest((ROOT / 'scripts/interface_slots.py').read_bytes())},
                inputs={name:dict(path=path, sha256=sha) for name,(path,sha) in IMAGES.items()},
                modules=modules, loader=loader, slots=dict(sorted(slots.items(), key=lambda x:int(x[0],0))),
                wrappers=wrappers, counts=dict(total=len(wrappers), **counts))


def generate_bindings(review):
    # Preserve accepted native root/anchor fields; references are per caller.
    from fixed_slot import REVIEW_SHA256
    if digest((ROOT / REVIEW).read_bytes()) != REVIEW_SHA256: raise ValueError('Unpinned review')
    originals = ['evidence/interface_transfer_bindings_2026_10_25.json', 'evidence/title_interface_bindings_2026_10_25.json']
    documents = [json.loads((ROOT / p).read_bytes()) for p in originals]
    owners = {name:next(copy.deepcopy(f['symbols'][ROOT_SYMBOL]) for d in documents for f in d['functions'].values()
                        if ROOT_SYMBOL in f['symbols'] and f['symbols'][ROOT_SYMBOL]['rva'] == root) for name,root in ROOTS.items()}
    # TITLE's accepted record is its real exact-size COMMON owner; ENG1/ENG3 use
    # zero-section-v1. Both are reused verbatim.
    base = dict(schema=1, scope='Opt-in fixed-slot candidates only; no acceptance or recovery changes.',
                evidence_files={REVIEW:REVIEW_SHA256, CATALOG:digest((ROOT / CATALOG).read_bytes())}, functions={})
    # Reuse the accepted documents' pinned include/source evidence as well.
    for d in documents:
        base['evidence_files'].update({p:sha for p,sha in d['evidence_files'].items()
                                       if p not in {path for path,_ in IMAGES.values()}})
    conflicts = json.loads((ROOT / CATALOG).read_bytes())['source_slot_conflicts']
    for row in conflicts.values():
        if digest((ROOT / row['source']).read_bytes()) != row['source_sha256']:
            raise ValueError('Source slot-conflict review is stale')
    for name in ROOTS:
        document = copy.deepcopy(base)
        for id, row in review['wrappers'].items():
            if row['module'] != name or row['verdict'] != 'agree' or id in conflicts: continue
            owner = copy.deepcopy(owners[name]); owner['references'] = [dict(offset=row['offset'] + 2, type=6, addend=row['slot'])]
            proof = {key:row[key] for key in ('offset', 'slot', 'transfer', 'pushed_dwords', 'cleanup_bytes')}
            proof.update(profile='fixed-slot-v1', root=ROOT_SYMBOL, review_sha256=REVIEW_SHA256,
                         evidence=[REVIEW + '#slots.' + hex(row['slot'])])
            document['functions'][id] = dict(symbols={ROOT_SYMBOL:owner}, indirect_calls=[proof])
        write_json(ROOT / 'work/iface_bindings' / (name.split('.')[0].lower() + '.json'), document)
    return dict(review['counts'], source_slot_conflicts=len(conflicts), lane_ready=review['counts']['agree'] - len(conflicts),
                title_owner='accepted TITLE COMMON root record reused verbatim')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['review', 'check', 'bindings'])
    args = parser.parse_args()
    if args.command == 'bindings':
        review = json.loads((ROOT / REVIEW).read_bytes()); print(generate_bindings(review)); return
    review = derive()
    if args.command == 'check':
        if review != json.loads((ROOT / REVIEW).read_bytes()): raise ValueError('Review does not reproduce')
    else: write_json(ROOT / REVIEW, review)
    print(json.dumps(review['counts']))


if __name__ == '__main__': main()
