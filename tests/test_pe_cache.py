"""Every construction reads current bytes; cached parser state cannot leak."""
import concurrent.futures
import os
from pathlib import Path
import pickle
import struct
import sys
import threading
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import pe
import match
from test_matching import CODE, make_coff, make_pe, workspace_temporary_directory


class PECacheTests(unittest.TestCase):
    def setUp(self):
        directory = workspace_temporary_directory('pe-cache-')
        self.addCleanup(directory.cleanup)
        self.path = Path(directory.name) / 'fixture.dll'
        make_pe(self.path, base_relocation=True)
        self.addCleanup(patch.stopall)
        patch.dict(os.environ, {'HERCULES_PE_CACHE': '1'}).start()
        pe._PARSES.clear()
        self.addCleanup(pe._PARSES.clear)

    def test_each_construction_reads_and_hashes_current_bytes(self):
        original = Path.read_bytes
        reads = []
        def read(path):
            reads.append(path)
            return original(path)
        with patch.object(Path, 'read_bytes', read), \
                patch.object(pe.pefile.PE, '__init__', autospec=True, side_effect=pe.pefile.PE.__init__) as parser:
            first = pe.PE(self.path)
            second = pe.PE(self.path.resolve())
        self.assertEqual(reads, [self.path, self.path.resolve()])
        self.assertEqual(parser.call_count, 1)
        self.assertIsNot(first.pe, second.pe)
        self.assertIsNot(type(first.pe), type(second.pe))
        self.assertIsNot(first.sections, second.sections)

    def test_changed_bytes_same_path_size_and_timestamp_require_a_new_parse(self):
        before = self.path.stat()
        with patch.object(pe.pefile.PE, '__init__', autospec=True, side_effect=pe.pefile.PE.__init__) as parser:
            first = pe.PE(self.path)
            data = bytearray(self.path.read_bytes())
            struct.pack_into('<I', data, 0x98 + 28, 0x500000)
            self.path.write_bytes(data)
            os.utime(self.path, ns=(before.st_atime_ns, before.st_mtime_ns))
            second = pe.PE(self.path)
        self.assertEqual(parser.call_count, 2)
        self.assertIsNot(first.pe, second.pe)
        self.assertEqual(first.image_base, 0x400000)
        self.assertEqual(second.image_base, 0x500000)
        self.assertEqual(self.path.stat().st_size, before.st_size)
        self.assertEqual(self.path.stat().st_mtime_ns, before.st_mtime_ns)

    def test_invalid_replacement_never_reuses_old_parse(self):
        before = self.path.read_bytes()
        pe.PE(self.path)
        self.path.write_bytes(b'not a PE image')
        with self.assertRaises(pe.pefile.PEFormatError):
            pe.PE(self.path)
        self.path.write_bytes(before)
        self.assertEqual(pe.PE(self.path).data, before)

    def test_deleted_file_cannot_be_served_from_cache(self):
        pe.PE(self.path)
        self.path.unlink()
        with self.assertRaises(FileNotFoundError):
            pe.PE(self.path)

    def test_equal_bytes_at_different_paths_are_separate_keys(self):
        other = self.path.with_name('other.dll')
        other.write_bytes(self.path.read_bytes())
        with patch.object(pe.pefile.PE, '__init__', autospec=True, side_effect=pe.pefile.PE.__init__) as parser:
            first, second = pe.PE(self.path), pe.PE(other)
        self.assertEqual(parser.call_count, 2)
        self.assertIsNot(first.pe, second.pe)

    def test_attributes_and_nested_relocation_records_are_read_only(self):
        image = pe.PE(self.path)
        for obj, attribute in ((image.pe.OPTIONAL_HEADER, 'ImageBase'),
                               (image.pe.FILE_HEADER, 'Machine'),
                               (image.pe.sections[0], 'VirtualAddress'),
                               (image.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5], 'Size'),
                               (image.pe.DIRECTORY_ENTRY_BASERELOC[0].entries[0], 'rva')):
            with self.subTest(attribute=attribute):
                before = getattr(obj, attribute)
                with self.assertRaises(AttributeError):
                    setattr(obj, attribute, 0)
                with self.assertRaises(AttributeError):
                    delattr(obj, attribute)
                self.assertEqual(getattr(obj, attribute), before)
        with self.assertRaises(AttributeError):
            image.pe.sections.clear()
        with self.assertRaises((AttributeError, TypeError)):
            vars(image.pe.OPTIONAL_HEADER)
        self.assertEqual(pe.PE(self.path).image_base, 0x400000)

    def test_dictionary_metadata_and_its_nested_values_are_private(self):
        original = pe.pefile.PE.__init__
        def initialize(instance, *args, **kwargs):
            original(instance, *args, **kwargs)
            instance.RICH_HEADER = {'values': [1, 2]}
        with patch.object(pe.pefile.PE, '__init__', initialize):
            first = pe.PE(self.path)
        first.pe.RICH_HEADER['values'].append(3)
        first.metadata()['rich_header']['values'].clear()
        self.assertEqual(pe.PE(self.path).pe.RICH_HEADER, {'values': [1, 2]})

    def test_adapter_sections_and_metadata_are_private(self):
        first = pe.PE(self.path)
        expected = first.sections[0]['virtual_address']
        first.sections[0]['virtual_address'] = 0
        second = pe.PE(self.path)
        self.assertEqual(second.sections[0]['virtual_address'], expected)
        second.metadata()['sections'][0]['virtual_address'] = 7
        self.assertEqual(pe.PE(self.path).sections[0]['virtual_address'], expected)

    def test_review_object_setattr_cannot_hide_a_target_relocation(self):
        obj = self.path.with_suffix('.obj')
        make_coff(obj)
        image = pe.PE(self.path)
        before = match.compare(obj, '_sample', self.path, 0x1000, len(CODE))
        self.assertFalse(before['exact'])
        relocation = image.pe.DIRECTORY_ENTRY_BASERELOC[0].entries[0]
        with self.assertRaises((AttributeError, TypeError)):
            object.__setattr__(relocation, 'type', 0)
        cached = match.compare(obj, '_sample', self.path, 0x1000, len(CODE))
        with patch.dict(os.environ, {'HERCULES_PE_CACHE': '0'}):
            uncached = match.compare(obj, '_sample', self.path, 0x1000, len(CODE))
        self.assertEqual(before, cached)
        self.assertEqual(cached, uncached)
        self.assertFalse(cached['exact'])

    def test_records_have_no_shared_raw_parser_or_snapshot_handles(self):
        image = pe.PE(self.path)
        for record in (image.pe, image.pe.OPTIONAL_HEADER,
                       image.pe.DIRECTORY_ENTRY_BASERELOC[0].entries[0]):
            self.assertIsInstance(record, tuple)
            for name in ('_value', '_snapshot', '__dict__'):
                self.assertFalse(hasattr(record, name))
            with self.assertRaises((AttributeError, TypeError)):
                object.__setattr__(record, '_0', b'changed bytes')
        private = image.pe.sections[0].pe
        private.OPTIONAL_HEADER.ImageBase = 0x900000
        private.RICH_HEADER = {'values': [0xDEADBEEF]}
        self.assertEqual(pe.PE(self.path).image_base, 0x400000)
        self.assertIsNone(pe.PE(self.path).metadata()['rich_header'])

    def test_review_record_class_mutation_cannot_hide_target_relocation(self):
        obj = self.path.with_suffix('.obj')
        make_coff(obj)
        first = pe.PE(self.path)
        relocation = first.pe.DIRECTORY_ENTRY_BASERELOC[0].entries[0]
        before = match.compare(obj, '_sample', self.path, 0x1000, len(CODE))
        self.assertFalse(before['exact'])
        # Deliberately overwrite the descriptor, exactly as the third review did.
        type(relocation).type = 0
        self.assertEqual(relocation.type, 0)
        second = pe.PE(self.path)
        later = second.pe.DIRECTORY_ENTRY_BASERELOC[0].entries[0]
        self.assertIsNot(type(relocation), type(later))
        self.assertEqual(later.type, pe.pefile.RELOCATION_TYPE['IMAGE_REL_BASED_HIGHLOW'])
        cached = match.compare(obj, '_sample', self.path, 0x1000, len(CODE))
        with patch.dict(os.environ, {'HERCULES_PE_CACHE': '0'}):
            uncached = match.compare(obj, '_sample', self.path, 0x1000, len(CODE))
        self.assertEqual(before, cached)
        self.assertEqual(cached, uncached)
        self.assertFalse(cached['exact'])
        self.assertEqual(first.data, self.path.read_bytes())

    def test_cache_contains_only_builtin_immutable_data_and_views_have_builtin_base(self):
        first, second = pe.PE(self.path), pe.PE(self.path)
        def immutable(value):
            self.assertIn(type(value), (type(None), bool, int, float, str, bytes, tuple))
            if type(value) is tuple:
                for item in value:
                    immutable(item)
        for snapshot in pe._PARSES.values():
            immutable(snapshot)
        for image in (first, second):
            for record in (image.pe, image.pe.OPTIONAL_HEADER,
                           image.pe.DIRECTORY_ENTRY_BASERELOC[0].entries[0]):
                self.assertEqual(type(record).__bases__, (tuple,))
        type(first.pe).__getattr__ = lambda *args: 0
        type(first.pe.OPTIONAL_HEADER).ImageBase = 0
        type(first.pe.sections[0]).VirtualAddress = 0
        self.assertIsNot(type(first.pe.sections), type(second.pe.sections))
        self.assertEqual(type(first.pe.sections).__bases__, (tuple,))
        type(first.pe.sections).__iter__ = lambda self: iter(())
        self.assertEqual(list(first.pe.sections), [])
        self.assertEqual(second.image_base, 0x400000)
        self.assertEqual(pe.PE(self.path).pe.OPTIONAL_HEADER.ImageBase, 0x400000)
        self.assertEqual(second.pe.sections[0].VirtualAddress, 0x1000)
        self.assertGreater(len(list(second.pe.sections)), 0)

    def test_mutating_method_and_mutable_method_result_do_not_leak(self):
        image = pe.PE(self.path)
        expected = image.read_rva(0x2000, 4)
        self.assertTrue(image.pe.set_dword_at_rva(0x2000, 0xDEADBEEF))
        private_section = image.pe.get_section_by_rva(0x2000)
        private_section.VirtualAddress = 0
        self.assertEqual(image.read_rva(0x2000, 4), expected)
        second = pe.PE(self.path)
        self.assertEqual(second.pe.get_data(0x2000, 4), expected)
        self.assertEqual(second.sections[1]['virtual_address'], 0x2000)

    def test_getters_and_rebasing_cannot_modify_shared_pefile_internals(self):
        image = pe.PE(self.path)
        snapshot = next(iter(pe._PARSES.values()))
        before = pickle.dumps(snapshot, protocol=5)
        self.assertEqual(image.pe.get_rva_from_offset(0x400), 0x1000)
        image.pe.get_data(0x1000, 4)
        image.pe.sections[0].get_data()
        image.pe.get_memory_mapped_image()
        image.pe.get_memory_mapped_image(ImageBase=0x500000)
        image.metadata()
        self.assertEqual(pickle.dumps(snapshot, protocol=5), before)
        self.assertEqual(pe.PE(self.path).pe.OPTIONAL_HEADER.ImageBase, 0x400000)

    def test_concurrent_cold_constructions_coalesce_one_parse(self):
        barrier = threading.Barrier(8, timeout=5)
        def construct(_):
            barrier.wait()
            return pe.PE(self.path)
        with patch.object(pe.pefile.PE, '__init__', autospec=True, side_effect=pe.pefile.PE.__init__) as parser:
            with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
                results = list(pool.map(construct, range(8)))
        self.assertEqual(parser.call_count, 1)
        self.assertEqual(len({id(result.pe) for result in results}), 8)
        self.assertEqual(len({type(result.pe) for result in results}), 8)

    def test_cache_bound_and_eviction_preserve_correctness(self):
        expected = self.path.read_bytes()
        for index in range(17):
            path = self.path.with_name(str(index) + '.dll')
            path.write_bytes(expected)
            pe.PE(path)
        self.assertEqual(len(pe._PARSES), 16)
        with patch.object(pe.pefile.PE, '__init__', autospec=True, side_effect=pe.pefile.PE.__init__) as parser:
            self.assertEqual(pe.PE(self.path.with_name('0.dll')).data, expected)
        self.assertEqual(parser.call_count, 1)

    def test_serial_baseline_switch_never_shares_mutable_parses(self):
        with patch.dict(os.environ, {'HERCULES_PE_CACHE': '0'}):
            first, second = pe.PE(self.path), pe.PE(self.path)
        first.pe.OPTIONAL_HEADER.ImageBase = 0
        self.assertEqual(second.pe.OPTIONAL_HEADER.ImageBase, 0x400000)
        self.assertFalse(pe._PARSES)


if __name__ == '__main__':
    unittest.main()
