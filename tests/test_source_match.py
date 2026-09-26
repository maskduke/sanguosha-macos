"""BASE-02 content evidence: mismatches may never be hidden by match totals."""
import hashlib
import tempfile
import unittest
from pathlib import Path

from tools.source_match import compare_resources, parse_git_tree


def blob(data):
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


class SourceMatchTests(unittest.TestCase):
    def test_parses_nul_terminated_paths_without_splitting_spaces_or_tabs(self):
        data = b"100644 blob " + b"a" * 40 + b"\tlua/name with\ttab.lua\0"
        self.assertEqual(parse_git_tree(data), {"lua/name with\ttab.lua": "a" * 40})

    def test_rejects_tree_without_complete_record(self):
        with self.assertRaises(ValueError):
            parse_git_tree(b"100644 blob " + b"a" * 40 + b"\ttruncated")

    def test_reports_all_four_categories_without_discarding_extra_files(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "same").write_bytes(b"match")
            (root / "changed").write_bytes(b"local")
            (root / "local-only").write_bytes(b"packaged")
            result = compare_resources(root, {"same": blob(b"match"),
                                               "changed": blob(b"upstream"),
                                               "upstream-only": blob(b"not packaged")})
            self.assertEqual(result["matched"], ["same"])
            self.assertEqual(result["changed"], ["changed"])
            self.assertEqual(result["local_only"], ["local-only"])
            self.assertEqual(result["repository_only"], ["upstream-only"])

    def test_identical_name_does_not_count_as_content_match(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "config.lua").write_text('version="20120715"')
            result = compare_resources(root, {"config.lua": blob(b'new rules')})
            self.assertEqual(result["matched"], [])
            self.assertEqual(result["changed"], ["config.lua"])

    def test_rejects_symlinks_in_resource_comparison_instead_of_reading_outside(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "link").symlink_to("/etc/hosts")
            with self.assertRaises(ValueError):
                compare_resources(root, {})
