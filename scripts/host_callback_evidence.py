"""Recheck the static host-to-TITLE arena-reset callback binding."""
import json
import struct

from env import ROOT
from match import digest
from pe import PE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32


def instruction(image, rva, mnemonic, operands):
    row = next(Cs(CS_ARCH_X86, CS_MODE_32).disasm(image.read_rva(rva, 16), image.image_base + rva))
    if row.mnemonic != mnemonic or row.op_str != operands:
        raise ValueError(f'Changed callback instruction at {rva:#x}')


def verify():
    evidence = json.loads((ROOT / 'evidence/host_arena_reset_experiment.json').read_text())
    host_path = ROOT / evidence['oracle']['path']
    if digest(host_path.read_bytes()) != evidence['oracle']['sha256']:
        raise ValueError('Changed host oracle')
    host = PE(host_path)
    title_path = ROOT / 'assets/pc/HERCULES/TITLE.DLL'
    if digest(title_path.read_bytes()) != evidence['engine_table_binding']['module_sha256']:
        raise ValueError('Changed TITLE oracle')
    title = PE(title_path)
    table = evidence['source_table']
    if digest(host.read_rva(int(table['rva'], 0), table['bytes'])) != table['sha256']:
        raise ValueError('Changed host interface table')
    for rva, value in ((0x3625c, 0x404470), (0x365a0, 0x4361b8)):
        if struct.unpack('<I', host.read_rva(rva, 4))[0] != value:
            raise ValueError('Changed callback table word')
    sites = [
        (host, 0x5ef3, 'mov', 'edx, dword ptr [0x4365a0]'),
        (host, 0x5f01, 'push', 'ecx'),
        (host, 0x5f02, 'push', 'edx'),
        (host, 0x5f03, 'call', 'eax'),
        (host, 0x5f05, 'add', 'esp, 8'),
        (title, 0xc203, 'mov', 'ecx, 0xfa'),
        (title, 0xc208, 'mov', 'esi, edx'),
        (title, 0xc20a, 'mov', 'edi, 0x1002bb40'),
        (title, 0xc20f, 'rep movsd', 'dword ptr es:[edi], dword ptr [esi]'),
        (title, 0xc4b0, 'mov', 'eax, dword ptr [esp + 4]'),
        (title, 0xc4b4, 'push', 'eax'),
        (title, 0xc4b5, 'call', 'dword ptr [0x1002bbe4]'),
        (title, 0xc4bb, 'add', 'esp, 4'),
        (title, 0xc4be, 'ret', ''),
        (title, 0x6017, 'call', '0x1000c4b0'),
        (title, 0x601c, 'add', 'esp, 4'),
        (title, 0x5fb6, 'xor', 'ebx, ebx'),
        (title, 0x5fde, 'push', 'ebx'),
    ]
    for image, rva, mnemonic, operands in sites:
        instruction(image, rva, mnemonic, operands)
    if 0x2bb40 + 4 * 0x29 != 0x2bbe4:
        raise ValueError('Incorrect copied slot offset')
    result = dict(images=2, table_words=250, instructions=len(sites),
                  scope='Static default table binding and forwarding ABI only; runtime alias mutation and execution outcomes unproved.')
    print(json.dumps(result, indent=2))
    return result


if __name__ == '__main__':
    verify()
