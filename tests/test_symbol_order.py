import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from symbol_order import bucket, emission_order  # noqa: E402


class SymbolOrderTests(unittest.TestCase):
    """Measured VC5 orders (pinned RTM compiler) that the recovered rule must reproduce."""

    def test_cpp_globals(self):
        names = ['v_first', 'v_second', 'v_array', 'v_bytes', 'v_third', 'v_short', 'v_last']
        self.assertEqual(emission_order(names),
                         ['v_bytes', 'v_first', 'v_array', 'v_second', 'v_short', 'v_last', 'v_third'])

    def test_c_communal_group(self):
        names = ['g_2cbe0', 'g_2cbe4', 'g_2cbe8', 'g_2cbec', 'g_2cbf0', 'g_2cbf4', 'g_2cc00', 'g_2cc04',
                 'g_2cc08', 'g_2cc0c', 'g_2cc20', 'g_2cc60', 'g_2cc64', 'g_2cc68']
        self.assertEqual(emission_order(names),
                         ['g_2cbf0', 'g_2cbe4', 'g_2cbf4', 'g_2cbe8', 'g_2cbe0', 'g_2cbec', 'g_2cc20', 'g_2cc08',
                          'g_2cc00', 'g_2cc04', 'g_2cc60', 'g_2cc64', 'g_2cc68', 'g_2cc0c'])

    def test_digit_names_and_bucket_ties(self):
        names = [f'v{i}' for i in range(40)]
        observed = ['v10', 'v11', 'v12', 'v13', 'v20', 'v14', 'v21', 'v15', 'v22', 'v16', 'v23', 'v17', 'v30', 'v24',
                    'v18', 'v31', 'v25', 'v19', 'v32', 'v26', 'v33', 'v27', 'v34', 'v28', 'v35', 'v29', 'v36', 'v37',
                    'v38', 'v39', 'v0', 'v1', 'v2', 'v3', 'v4', 'v5', 'v6', 'v7', 'v8', 'v9']
        self.assertEqual(emission_order(names), observed)
        self.assertEqual(bucket('v20'), bucket('v14'))  # tie: the later-defined v20 comes first


if __name__ == '__main__':
    unittest.main()
