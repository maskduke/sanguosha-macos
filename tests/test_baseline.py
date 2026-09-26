"""BASE-01 regression tests; fixtures are isolated miniature application bundles."""
import hashlib
import tempfile
import unittest
from pathlib import Path

from tools.baseline import compare_manifests, inventory, snapshot


class BaselineTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="qs-baseline-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.app = self.root / "原版 Game.app"
        self.app.mkdir()
        (self.app / "empty").mkdir()
        (self.app / "data.bin").write_bytes(b"\x00\xff\n")
        (self.app / "run").write_text("executable\n")
        (self.app / "run").chmod(0o755)

    def test_records_content_empty_directory_and_executable_mode(self):
        result = inventory(self.app)
        self.assertEqual(result["data.bin"]["sha256"], hashlib.sha256(b"\x00\xff\n").hexdigest())
        self.assertEqual(result["data.bin"]["size"], 3)
        self.assertEqual(result["empty"]["type"], "directory")
        self.assertEqual(result["run"]["mode"] & 0o111, 0o111)
        self.assertEqual(list(result), sorted(result))

    def test_same_content_in_different_location_has_same_manifest(self):
        before = inventory(self.app)
        target = self.root / "snapshot.app"
        snapshot(self.app, target)
        self.assertEqual(inventory(target), before)
        self.assertEqual(inventory(self.app), before)

    def test_detects_modified_missing_and_added_entries(self):
        before = inventory(self.app)
        (self.app / "data.bin").write_bytes(b"changed")
        (self.app / "run").unlink()
        (self.app / "new").write_text("new")
        self.assertEqual(compare_manifests(before, inventory(self.app)), {
            "added": ["new"], "removed": ["run"], "changed": ["data.bin"]})

    def test_preserves_internal_symlink_without_hashing_target_twice(self):
        (self.app / "alias").symlink_to("data.bin")
        target = self.root / "copy.app"
        snapshot(self.app, target)
        self.assertTrue((target / "alias").is_symlink())
        self.assertEqual(inventory(target)["alias"], {"type": "symlink", "target": "data.bin"})

    def test_rejects_links_outside_bundle_before_copying(self):
        secret = self.root / "outside"
        secret.write_text("outside bundle")
        (self.app / "escape").symlink_to("../outside")
        target = self.root / "copy.app"
        with self.assertRaises(ValueError):
            snapshot(self.app, target)
        self.assertFalse(target.exists())

    def test_refuses_overwriting_existing_destination(self):
        target = self.root / "existing.app"
        target.mkdir()
        (target / "keep").write_text("do not overwrite")
        with self.assertRaises(FileExistsError):
            snapshot(self.app, target)
        self.assertEqual((target / "keep").read_text(), "do not overwrite")

    def test_refuses_snapshot_inside_source(self):
        before = inventory(self.app)
        with self.assertRaises(ValueError):
            snapshot(self.app, self.app / "nested")
        self.assertEqual(inventory(self.app), before)

    def test_detects_permission_change(self):
        before = inventory(self.app)
        (self.app / "run").chmod(0o644)
        self.assertEqual(compare_manifests(before, inventory(self.app))["changed"], ["run"])


if __name__ == "__main__":
    unittest.main()
