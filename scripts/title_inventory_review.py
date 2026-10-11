"""Verify compact native entry-identity evidence without accepting byte extents."""
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM
from pe import PE
from title_integration import ROOT, load_report


def verify():
    evidence = json.loads((ROOT / 'evidence/title_behavioral_inventory.json').read_text())
    original = PE(ROOT / 'assets/pc/HERCULES/TITLE.DLL')
    if hashlib.sha256(original.data).hexdigest() != evidence['target_sha256']:
        raise ValueError('Original TITLE identity changed')
    build, _, _ = load_report()
    flagged = {row['rva'] for row in build['functions'] if row['extent_review_required']}
    entries = evidence['entries']
    if len({row['rva'] for row in entries}) != len(entries) or {row['rva'] for row in entries} != flagged:
        raise ValueError('Reviewed entries differ from current inventory warnings')
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    low = int(evidence['first_party_text_range']['start_rva'], 0)
    high = int(evidence['first_party_text_range']['end_exclusive_rva'], 0)
    for row in entries:
        rva = int(row['rva'], 0)
        if not low <= rva < high or rva % 16:
            raise ValueError('Entry outside reviewed aligned first-party range: ' + row['rva'])
        prefix = original.read_rva(rva, 16)
        if hashlib.sha256(prefix).hexdigest() != row['entry_prefix_sha256'] or prefix[0] == 0x90:
            raise ValueError('Native entry prefix changed or begins on padding: ' + row['rva'])
        name = row['symbol'].lstrip('_')
        source = (ROOT / row['source']).read_text()
        pattern = r'(?m)^\s*(?:static\s+)?[A-Za-z_][A-Za-z_0-9 \t\*]*\b' + re.escape(name) + r'\s*\([^;{}]*?\)\s*\{'
        if not re.search(pattern, source):
            raise ValueError('Reconstructed function definition missing: ' + name)
        external = 0
        for reference in row['references']:
            site = int(reference['site'], 0)
            if reference['kind'] == 'data':
                if struct.unpack('<I', original.read_rva(site, 4))[0] != original.image_base + rva:
                    raise ValueError('Native data-pointer evidence changed: ' + hex(site))
                external += 1
            else:
                instruction = next(md.disasm(original.read_rva(site, 16), original.image_base + site), None)
                if instruction is None or instruction.mnemonic != reference['instruction'] or not any(
                        operand.type == X86_OP_IMM and operand.imm == original.image_base + rva
                        for operand in instruction.operands):
                    raise ValueError('Native code-reference evidence changed: ' + hex(site))
                external += not rva <= site < rva + row['mapped_span_size']
        if not external:
            raise ValueError('No external native reference supports entry: ' + row['rva'])
    print(json.dumps(dict(verified_entry_identities=len(entries), exact_extents_accepted=0,
                          matching_state_changed=False)))
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(verify())
    except (OSError, ValueError, KeyError) as error:
        print('inventory review error:', error, file=sys.stderr)
        raise SystemExit(2)
