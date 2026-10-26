"""Opt-in native callee identity proposals. No acceptance or caller-derived identity.

Automatic class a0 is deliberately relocation-free. See docs/callee_identity_prototype_2026_10_26.md
for the review proposal; work output is never binding evidence.
"""
import argparse
from collections import defaultdict
import hashlib
import json
import os
from pathlib import Path
import re

from env import ROOT
from coff import COFF
from pe import PE
import match


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def number(value):
    return int(value, 0) if isinstance(value, str) else value


def instructions(code, address):
    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    result = list(decoder.disasm(code, address))
    if sum(i.size for i in result) != len(code):
        raise ValueError('incomplete decode')
    return result


def abi(code, address, symbol):
    """Narrow leaf-body check, not a general x86 calling-convention inference."""
    from capstone.x86 import X86_OP_IMM
    decoded = instructions(code, address)
    body = list(decoded)
    while body and body[-1].mnemonic == 'nop':
        body.pop()
    if not body or body[-1].mnemonic != 'ret':
        raise ValueError('thunk/tail transfer or missing RET')
    returns = set()
    boundaries = {i.address for i in body}
    for i in body:
        if i.mnemonic == 'ret':
            returns.add(i.operands[0].imm if i.operands else 0)
        elif i.mnemonic == 'call':
            raise ValueError('callee calls need independent outgoing reference closure')
        elif i.mnemonic.startswith('j') or i.mnemonic.startswith('loop'):
            if len(i.operands) != 1 or i.operands[0].type != X86_OP_IMM or i.operands[0].imm not in boundaries:
                raise ValueError('escaping/mid-instruction branch or thunk')
    decorated = re.fullmatch(r'_[^@]+@(\d+)', symbol)
    expected = int(decorated[1]) if decorated else 0
    if not symbol.startswith('_') or symbol.startswith('__') or returns != {expected}:
        raise ValueError('unsupported ABI or decorated cleanup mismatch')
    return dict(ret_immediates=sorted(returns), convention='stdcall' if decorated else 'cdecl')


def locate(pe, code):
    """Search every executable offset, including unindexed/interior collisions."""
    found = []
    for s in pe.sections:
        if not s['characteristics'] & 0x20000000:
            continue
        base = s['virtual_address']
        raw = pe.read_rva(base, min(s['virtual_size'], s['raw_size']))
        at = raw.find(code)
        while at >= 0:
            found.append(base + at)
            at = raw.find(code, at + 1)
    return found


def safe_witnesses(pe, rows):
    """Only accepted native contributions may witness an entry; never queue votes."""
    result = defaultdict(list)
    accepted = match.accepted_rows(rows)
    pending = [r for r in rows if r['id'] not in {a['id'] for a in accepted}]
    for row in accepted:
        if row['target_sha256'] != hashlib.sha256(pe.data).hexdigest():
            continue
        start, size = number(row['rva']), row['size']
        try:
            decoded = instructions(pe.read_rva(start, size), start)
        except ValueError:
            continue
        for i in decoded:
            if len(i.bytes) != 5 or i.bytes[0] != 0xe8:
                continue
            if any(number(p['rva']) < i.address + 5 and i.address < number(p['rva']) + p['size'] for p in pending):
                continue
            target = i.operands[0].imm
            result[target].append(dict(row=row['id'], rva=i.address, size=5,
                                      sha256=hashlib.sha256(bytes(i.bytes)).hexdigest()))
    return result


def admit(symbol, code, relocations, pe, entries, witnesses, claimed=None, expected=None, native_spans=()):
    """No expected destination is used to locate or disambiguate an identity."""
    if not code:
        raise ValueError('empty own body')
    if relocations:
        raise ValueError('outgoing relocations require independent closure (not automatic a0)')
    candidates = locate(pe, code)
    if len(candidates) != 1:
        raise ValueError('own body is absent or ambiguous across executable offsets')
    rva = candidates[0]
    if expected is not None and rva != expected:
        raise ValueError('independently located entry disagrees with requested entry')
    if entries.get(rva) != len(code):
        raise ValueError('mid-function target or complete native extent differs')
    if any(start < rva + len(code) and rva < start + size and
           (start, size) != (rva, len(code)) for start, size in native_spans):
        raise ValueError('mid-function/overlapping recorded native extent')
    shape = abi(code, rva, symbol)
    if not witnesses.get(rva):
        raise ValueError('no accepted outside direct caller; pending-only callers cannot witness')
    if claimed is not None and set(claimed.get(rva, ())) != {symbol}:
        raise ValueError('two symbols claim one entry; independent alias review required')
    return dict(rva=rva, size=len(code), abi=shape, witness=witnesses[rva][0])


def queue_symbols(queue):
    result = defaultdict(list)
    for row in queue['rows']:
        for blocker in row['blockers']:
            if blocker['kind'] in ('function_unknown', 'function_near'):
                result[(row['module'], blocker['symbol'])].append(row['id'])
    return dict(result)


def profiles_for(symbol, sources, profiles):
    if not re.fullmatch(r'_[A-Za-z][A-Za-z0-9_]*(?:@\d+)?', symbol):
        return []
    name = symbol[1:].split('@')[0]
    pattern = re.compile(r'\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{')
    # A textual lead only: COFF.function must subsequently prove the exact name.
    return sorted({(path, tuple(flags)) for path, text in sources.items() if pattern.search(text)
                   for flags in profiles.get(path, [])})


def scan(queue_path, map_dir, output, max_compiles=20):
    queue = json.loads(queue_path.read_bytes())
    ledger = json.loads((ROOT / 'recovery.json').read_bytes())['functions']
    subjects = queue_symbols(queue)
    profiles = defaultdict(list)
    for row in ledger:
        if row.get('source') and 'flags' in row:
            profiles[row['source']].append(row['flags'])
    sources = {p.relative_to(ROOT).as_posix(): p.read_text(errors='replace')
               for p in sorted((ROOT / 'src').rglob('*')) if p.suffix in ('.c', '.cpp')}
    choices = {key: profiles_for(key[1], sources, profiles) for key in subjects}
    demand = defaultdict(set)
    for key, values in choices.items():
        for value in values:
            demand[value].add(key)
    # Deterministic breadth per compile; counts are work scheduling, never proof.
    ordered = sorted(demand, key=lambda p: (-len(demand[p]), p))
    objects, errors, inputs, calls = {}, {}, {}, 0
    os.environ.setdefault('HERCULES_COMPILE_METRICS_DIR', 'work/compiler_events')
    for profile in ordered[:max_compiles]:
        source, flags = profile
        try:
            context = match.compile_inputs(ROOT / source, flags, match.DEFAULT_TOOLCHAIN)
            calls += 1
            obj, command = match.compile_source(ROOT / source, list(flags))
            if context != match.compile_inputs(ROOT / source, flags, match.DEFAULT_TOOLCHAIN):
                raise ValueError('compiler input closure changed')
            objects[profile] = COFF(obj)
            inputs[profile] = dict(object=str(obj), object_sha256=sha(obj), command=command,
                                   context=match.context_proof(context), unit_inventory_sha256=match.unit_inventory(obj))
        except (ValueError, RuntimeError, OSError) as exc:
            errors[profile] = str(exc)
    images, entries, witnesses = {}, {}, {}
    for module in sorted({k[0] for k in subjects}):
        row = next(r for r in ledger if Path(r['target']).name.upper() == module)
        pe = PE(ROOT / row['target'])
        if hashlib.sha256(pe.data).hexdigest() != row['target_sha256']:
            raise ValueError('original hash mismatch: ' + module)
        images[module] = (pe, row['target'], row['target_sha256'])
        document = json.loads((map_dir / ('function_map_' + module.lower() + '.json')).read_bytes())
        if document['module'].upper() != module:
            raise ValueError('map module mismatch')
        entries[module] = {number(r['rva']): r['size'] for r in document['functions']}
        witnesses[module] = safe_witnesses(pe, [r for r in ledger if r['target'] == row['target']])
    # Include non-queue defined symbols in selected objects as ambiguity controls.
    claims = defaultdict(lambda: defaultdict(set))
    for module, (pe, _, _) in images.items():
        relevant = {p for key, values in choices.items() if key[0] == module for p in values if p in objects}
        for profile in sorted(relevant):
            coff = objects[profile]
            for s in coff.symbols.values():
                if s['section'] <= 0 or not s['type'] & 0x20:
                    continue
                try:
                    section = coff.function(s['name'])
                except ValueError:
                    continue
                if section['relocations']:
                    continue
                for rva in locate(pe, section['data']):
                    claims[module][rva].add(s['name'])
    for profile, receipt in inputs.items():
        context = match.compile_inputs(ROOT / profile[0], profile[1], match.DEFAULT_TOOLCHAIN)
        if match.context_proof(context) != receipt['context'] or sha(receipt['object']) != receipt['object_sha256']:
            raise ValueError('frozen compiler inputs/object changed during scan')
    records, reasons = {}, []
    for key, uses in sorted(subjects.items()):
        module, symbol = key
        pe, target, target_hash = images[module]
        attempts, good = [], []
        for profile in choices[key]:
            if profile not in objects:
                attempts.append(dict(source=profile[0], flags=list(profile[1]),
                                     reason=errors.get(profile, 'compiler budget deferred')))
                continue
            try:
                section = objects[profile].function(symbol)
                result = admit(symbol, section['data'], section['relocations'], pe, entries[module],
                               witnesses[module], claims[module], native_spans=[
                                   (number(r['rva']),r['size']) for r in ledger if r['target']==target])
                good.append((profile, result))
            except ValueError as exc:
                attempts.append(dict(source=profile[0], flags=list(profile[1]), reason=str(exc)))
        identities = {(r['rva'], r['size']) for _, r in good}
        if len(identities) == 1:
            profile, result = good[0]
            identifier = module + ':' + symbol
            records[identifier] = dict(symbol=symbol, rva=result['rva'], size=result['size'], target=target,
                target_sha256=target_hash, source=profile[0], source_sha256=sha(ROOT / profile[0]),
                origin='a0: unique complete relocation-free own body; accepted native outside caller; proposal only',
                flags=list(profile[1]), abi=result['abi'], witness=result['witness'],
                native_sha256=hashlib.sha256(pe.read_rva(result['rva'], result['size'])).hexdigest(),
                compile={k:inputs[profile][k] for k in ('context','unit_inventory_sha256')}, rows=uses)
        else:
            reasons.append(dict(module=module, symbol=symbol, rows=uses,
                                reason='conflicting own-body profiles' if good else 'no admissible own body',
                                attempts=attempts or [dict(reason='no canonical exact-symbol body/profile lead')]))
    admitted = {(Path(r['target']).name, r['symbol']) for r in records.values()}
    touched = {id for key in admitted for id in subjects[key]}
    ready = [r['id'] for r in queue['rows'] if r['blockers'] and
             all((r['module'], b['symbol']) in admitted for b in r['blockers'])]
    stats = dict(symbols=len(subjects), blocker_uses=sum(map(len, subjects.values())),
                 rel32_operands=sum(x['kind'] in ('function_unknown', 'function_near')
                                   for r in queue['rows'] for x in r['refs']),
                 admitted_symbols=len(records), admitted_blocker_uses=sum(len(subjects[k]) for k in admitted),
                 touched_rows=len(touched), all_blockers_covered_rows=ready, compiler_calls=calls,
                 compiled_profiles=len(objects), candidate_profiles=len(ordered))
    result = dict(schema=1, scope='Review proposals only; not acceptance or lane authorization',
                  queue_sha256=sha(queue_path), recovery_sha256=sha(ROOT / 'recovery.json'),
                  function_identities=records, rejected=reasons, stats=stats,
                  compiled_inputs=[dict(source=p[0], flags=list(p[1]), **receipt)
                                   for p,receipt in sorted(inputs.items())],
                  inputs={m:dict(path=t,sha256=h) for m,(_,t,h) in images.items()})
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=1) + '\n', encoding='utf-8')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--queue', type=Path, default=ROOT / 'work/inputs/queue.json')
    parser.add_argument('--maps', type=Path, default=ROOT / 'work/inputs/function_maps')
    parser.add_argument('--output', type=Path, default=ROOT / 'work/callee_sol/scan.json')
    parser.add_argument('--max-compiles', type=int, default=20)
    args = parser.parse_args()
    if not 0 <= args.max_compiles <= 60:
        parser.error('max-compiles must be between 0 and 60')
    if not args.output.resolve().is_relative_to((ROOT / 'work').resolve()):
        parser.error('proposal output must be under work/')
    print(json.dumps(scan(args.queue, args.maps, args.output, args.max_compiles)['stats'], indent=1))


if __name__ == '__main__':
    main()
