"""The diagnostic no-compile preflight must defer file data owners to the full gate."""
import unittest
import reference_preflight


class FileDataPreflightTests(unittest.TestCase):
    def test_file_data_requires_full_verifier(self):
        rec=dict(kind='data',data_profile='file-data-owner-v1',references=[])
        self.assertEqual(reference_preflight._complex_binding_reasons(dict(symbols={'_table':rec})),
            ['_table: file data owner semantics require the full relocated verifier'])

    def test_legacy_data_diagnostics_preserved(self):
        for rec in (dict(kind='data'),dict(kind='data',data_profile='relocated-data-v1')):
            self.assertEqual(reference_preflight._complex_binding_reasons(dict(symbols={'_table':rec})),[])


if __name__=='__main__':unittest.main()
