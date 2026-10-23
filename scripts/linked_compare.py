"""Raw linked-image diagnostics. No relocation substitution and no acceptance."""
import struct

from match import digest
import capstone


def verify_direct_call_region(section, actual, expected, rva, native_starts, linked_starts):
    """Narrow proof for raw-equal, same-RVA code with only direct function fixups.

    Starts must come from independently reviewed function records, never inferred
    by solving the candidate's relocation operands. This does not certify callees.
    """
    size = section['size']
    if len(section['data']) != size or len(actual) != size or len(expected) != size or actual != expected:
        raise ValueError('Direct-call region requires complete literal equality')
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(actual, rva))
    if sum(i.size for i in instructions) != size:
        raise ValueError('Incomplete instruction decode')
    direct = {i.address-rva+1: i for i in instructions
              if i.size == 5 and i.bytes[0] in (0xe8, 0xe9)}
    covered, results = set(), []
    for rel in section['relocations']:
        off, name = rel['offset'], rel['symbol']
        if rel['type'] != 0x14 or off not in direct or off in covered:
            raise ValueError('Pilot admits only complete, unique direct REL32 calls/jumps')
        if name not in native_starts or name not in linked_starts:
            raise ValueError('Missing independent callee identity: ' + name)
        if struct.unpack_from('<i', section['data'], off)[0] != 0:
            raise ValueError('Pilot requires function-entry references without addends')
        destination = rva + off + 4 + struct.unpack_from('<i', actual, off)[0]
        if destination != native_starts[name] or destination != linked_starts[name]:
            raise ValueError('Callee identity or linked destination differs: ' + name)
        covered.add(off)
        results.append(dict(offset=off, symbol=name, destination_rva=hex(destination)))
    fields = {i for off in covered for i in range(off, off+4)}
    if any(actual[i] != section['data'][i] for i in range(size) if i not in fields):
        raise ValueError('Linked instructions differ from the compiled contribution')
    for instruction in instructions:
        if instruction.group(capstone.CS_GRP_CALL) or instruction.group(capstone.CS_GRP_JUMP):
            if not instruction.operands or instruction.operands[0].type != capstone.x86.X86_OP_IMM:
                raise ValueError('Indirect control transfer needs a separate contract')
            destination = instruction.operands[0].imm
            if not rva <= destination < rva+size and instruction.address-rva+1 not in covered:
                raise ValueError('Unaccounted external direct control transfer')
    return results


def byte_difference(actual, expected):
    return sum(actual[i:i+1] != expected[i:i+1] for i in range(max(len(actual), len(expected))))


def executable_bytes(pe, rva, size):
    if size <= 0 or not any(s['characteristics'] & 0x20000000 and
            s['virtual_address'] <= rva and rva + size <= s['virtual_address'] + min(s['virtual_size'], s['raw_size'])
            for s in pe.sections):
        raise ValueError('Linked/target contribution must fit wholly in file-backed executable code')
    return pe.read_rva(rva, size)


def compare_function(section, linked, original, linked_rva, original_rva, target_size, next_entry=None):
    """Compare the complete emitted contribution, never a target-sized prefix."""
    size = section['size']
    if len(section['data']) != size:
        raise ValueError('Incomplete object contribution')
    if next_entry is not None and linked_rva < next_entry < linked_rva + size:
        raise ValueError('Complete object extent overlaps another linked function')
    actual = executable_bytes(linked, linked_rva, size)
    expected = executable_bytes(original, original_rva, target_size)
    fields, supported = set(), True
    for relocation in section['relocations']:
        offset = relocation['offset']
        if offset < 0 or offset + 4 > size or fields.intersection(range(offset, offset + 4)):
            raise ValueError('Invalid or overlapping relocation operand')
        fields.update(range(offset, offset + 4))
        supported &= relocation['type'] in (6, 0x14)
    different = {i for i in range(max(size, target_size)) if actual[i:i+1] != expected[i:i+1]}
    object_changed = [i for i in range(size) if i not in fields and actual[i] != section['data'][i]]
    if object_changed:
        raise ValueError('Linked bytes disagree with object outside relocations; mapping or linker transformation needs review')
    raw_equal = actual == expected
    outside = different - fields
    category = ('raw equal' if raw_equal else 'size differs' if size != target_size else
                'reference operands only' if not outside and supported else 'code bytes differ')
    references = []
    # Corresponding operand offsets are meaningful only when the rest agrees.
    if size == target_size and not outside and supported:
        for rel in section['relocations']:
            off, kind = rel['offset'], rel['type']
            if kind == 6:
                native = (struct.unpack_from('<I', expected, off)[0] - original.image_base) & 0xffffffff
                ours = (struct.unpack_from('<I', actual, off)[0] - linked.image_base) & 0xffffffff
            else:
                native = (original_rva + off + 4 + struct.unpack_from('<i', expected, off)[0]) & 0xffffffff
                ours = (linked_rva + off + 4 + struct.unpack_from('<i', actual, off)[0]) & 0xffffffff
            destination = next((s['name'] for s in original.sections if s['virtual_address'] <= native <
                                s['virtual_address'] + max(s['virtual_size'], s['raw_size'])), 'outside image')
            references.append(dict(offset=off, symbol=rel['symbol'], type=kind, native_destination_rva=hex(native),
                                   linked_destination_rva=hex(ours), destination_section=destination,
                                   destination_delta=ours-native, operand_bytes_equal=actual[off:off+4] == expected[off:off+4]))
    return dict(scope='Actual complete linked contribution at its mapped location; diagnostic only',
                linked_rva=hex(linked_rva), original_rva=hex(original_rva), placement_delta=linked_rva-original_rva,
                same_rva=linked_rva == original_rva, compiled_size=size, target_size=target_size,
                raw_equal=raw_equal, raw_equal_at_original_rva=raw_equal and linked_rva == original_rva,
                different_bytes=len(different), different_reference_bytes=len(different & fields),
                different_nonreference_bytes=len(outside), category=category, references=references,
                linked_sha256=digest(actual), target_sha256=digest(expected))


def image_comparison(linked, original):
    """Whole file and complete section payloads; offsets/metadata are reported separately."""
    sections = []
    ours = {s['name']: s for s in linked.sections}
    native = {s['name']: s for s in original.sections}
    if len(ours) != len(linked.sections) or len(native) != len(original.sections):
        raise ValueError('Ambiguous duplicate section names')
    for name in sorted(ours.keys() | native.keys()):
        a, b = ours.get(name), native.get(name)
        actual = linked.data[a['raw_offset']:a['raw_offset']+a['raw_size']] if a else b''
        expected = original.data[b['raw_offset']:b['raw_offset']+b['raw_size']] if b else b''
        sections.append(dict(name=name, linked=a, original=b, raw_equal=actual == expected,
                             different_bytes=byte_difference(actual, expected),
                             linked_sha256=digest(actual), original_sha256=digest(expected)))
    header_a = linked.data[:linked.pe.OPTIONAL_HEADER.SizeOfHeaders]
    header_b = original.data[:original.pe.OPTIONAL_HEADER.SizeOfHeaders]
    meta_a, meta_b = linked.metadata(), original.metadata()
    fields = ('image_base', 'entry_point_rva', 'timestamp', 'linker_version', 'image_size',
              'subsystem', 'characteristics', 'stack_reserve', 'heap_reserve', 'checksum')
    return dict(scope='Raw file and section-relative payload comparison; no byte normalization',
                raw_file_equal=linked.data == original.data, linked_file_size=len(linked.data),
                original_file_size=len(original.data), same_file_offset_different_bytes=byte_difference(linked.data, original.data),
                headers_different_bytes=byte_difference(header_a, header_b), sections=sections,
                metadata_differences={f: dict(linked=meta_a[f], original=meta_b[f]) for f in fields if meta_a[f] != meta_b[f]},
                imports=dict(linked=meta_a['imports'], original=meta_b['imports']),
                exports=dict(linked=meta_a['exports'], original=meta_b['exports']))
