"""Aggregate implied link symbols of masked rows into one address table (diagnostic).

Checks: one symbol implies one address across all rows; a name that encodes an
address (title_01a30, g_2bb40, ..._1a30) agrees with its implied address; distinct
names sharing one address are reported for review (aliases are not merged).
Definitions come from the rows themselves (symbol at rva).
"""
import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

from env import ROOT

ENCODED = re.compile(r'_([0-9a-f]{4,5})$')
re_short = re.compile(r'^(title|g)_[0-9a-f]{1,4}$')


def build(module):
    state = json.loads((ROOT / 'recovery.json').read_text())
    rows = [r for r in state['functions'] if Path(r['target']).name.upper() == module.upper()]
    implied = defaultdict(set)
    users = defaultdict(list)
    defined = {}
    for r in rows:
        defined[r['symbol']] = int(r['rva'], 16)
        for name, address in r.get('link_symbols', {}).items():
            implied[name].add(int(address, 16))
            users[name].append(r['id'])
    problems = []
    for name, addresses in implied.items():
        if len(addresses) > 1:
            problems.append(dict(kind='symbol at several addresses', symbol=name,
                                 addresses=sorted(hex(a) for a in addresses), users=users[name][:6]))
        m = ENCODED.search(name.split('@')[0])
        if m and len(addresses) == 1 and int(m.group(1), 16) != next(iter(addresses)):
            problems.append(dict(kind='name/address disagreement', symbol=name,
                                 address=hex(next(iter(addresses))), users=users[name][:6]))
        stem = name.lstrip('_').split('@')[0]
        if stem.startswith(('title_', 'g_')) and re_short.search(stem):
            problems.append(dict(kind='address name not 5 hex digits', symbol=name, users=users[name][:6]))
        if name in defined and defined[name] not in addresses:
            problems.append(dict(kind='reference disagrees with definition', symbol=name,
                                 defined=hex(defined[name]), implied=sorted(hex(a) for a in addresses)))
    by_address = defaultdict(set)
    for name, addresses in implied.items():
        for a in addresses:
            by_address[a].add(name)
    for name, a in defined.items():
        by_address[a].add(name)
    aliases = {hex(a): sorted(n) for a, n in by_address.items() if len(n) > 1}
    table = {hex(a): sorted(n) for a, n in sorted(by_address.items())}
    unresolved_functions = sorted(n for n in implied if n.startswith('_title_') and n not in defined)
    return dict(module=module, symbols=len(implied), defined=len(defined), problems=problems,
                aliases=aliases, unresolved_title_functions=unresolved_functions, table=table)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module', nargs='?', default='TITLE.DLL')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    report = build(args.module)
    output = (args.output or ROOT / f'work/symbol_table/{args.module.lower()}.json').resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('Diagnostic outputs belong in ignored work/')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    print(f"{report['symbols']} referenced symbols, {report['defined']} defined rows, "
          f"{len(report['problems'])} problems, {len(report['aliases'])} shared addresses, "
          f"{len(report['unresolved_title_functions'])} referenced title functions not yet reconstructed")
    for p in report['problems'][:20]:
        print('  ', p)


if __name__ == '__main__':
    main()
