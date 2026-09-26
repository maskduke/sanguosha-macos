"""Read-only bundle inventories and verified, non-overwriting local snapshots.

Only content, entry types, symlink text and POSIX modes are compared. Extended
attributes, timestamps and Finder metadata are not asserted by this tool.
"""
import argparse
import hashlib
import json
import os
import shutil
import stat
import tempfile
from pathlib import Path


def inventory(root):
    root = Path(root)
    if root.is_symlink() or not root.is_dir():
        raise ValueError("Expected a real bundle directory")
    root = root.resolve()
    entries = {}
    for path in sorted(root.rglob("*")):
        name = path.relative_to(root).as_posix()
        info = path.lstat()
        if stat.S_ISLNK(info.st_mode):
            target = os.readlink(path)
            # Relative links within the bundle survive relocation; absolute
            # links would keep pointing at the original after copying.
            if Path(target).is_absolute() or not path.resolve().is_relative_to(root):
                raise ValueError("Non-relocatable symlink: " + name)
            entries[name] = {"type": "symlink", "target": target}
        elif stat.S_ISDIR(info.st_mode):
            entries[name] = {"type": "directory", "mode": stat.S_IMODE(info.st_mode)}
        elif stat.S_ISREG(info.st_mode):
            digest = hashlib.sha256()
            with path.open("rb") as stream:
                for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                    digest.update(chunk)
            after = path.stat()
            if (info.st_size, info.st_mtime_ns, info.st_ino) != (after.st_size, after.st_mtime_ns, after.st_ino):
                raise RuntimeError("File changed during inventory: " + name)
            entries[name] = {"type": "file", "size": info.st_size,
                             "mode": stat.S_IMODE(info.st_mode), "sha256": digest.hexdigest()}
        else:
            raise ValueError("Unsupported filesystem entry: " + name)
    return entries


def compare_manifests(expected, actual):
    return {"added": sorted(actual.keys() - expected.keys()),
            "removed": sorted(expected.keys() - actual.keys()),
            "changed": sorted(k for k in expected.keys() & actual.keys() if expected[k] != actual[k])}


def snapshot(source, destination):
    source, destination = Path(source), Path(destination)
    if destination.exists() or destination.is_symlink():
        raise FileExistsError(str(destination))
    if destination.resolve().is_relative_to(source.resolve()):
        raise ValueError("Snapshot destination must be outside source")
    before = inventory(source)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".qs-snapshot-", dir=destination.parent) as temp:
        staged = Path(temp) / "bundle"
        shutil.copytree(source, staged, symlinks=True)
        if inventory(staged) != before or inventory(source) != before:
            raise RuntimeError("Snapshot verification failed; original or copy changed")
        # Reserve the destination atomically. A competing writer must not be
        # overwritten. Replacing our empty reserved directory is atomic.
        destination.mkdir()
        staged.rename(destination)
    return before


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args()
    if args.report.exists():
        parser.error("Report already exists; choose a new evidence path")
    for protected in (args.source, args.destination):
        if args.report.resolve().is_relative_to(protected.resolve()):
            parser.error("Report must be outside source and snapshot")
    entries = snapshot(args.source, args.destination)
    canonical = json.dumps(entries, sort_keys=True, separators=(",", ":")).encode()
    report = {"schema": 1, "source": str(args.source.resolve()),
              "snapshot": str(args.destination.resolve()), "manifest_sha256": hashlib.sha256(canonical).hexdigest(),
              "source_unchanged": True, "snapshot_matches": True,
              "verification_scope": "content, paths, entry types, relative symlinks, POSIX entry modes; excludes xattrs/timestamps",
              "entries": entries}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    with args.report.open("x") as stream:
        json.dump(report, stream, ensure_ascii=False, indent=2)
        stream.write("\n")
    print(json.dumps({k: v for k, v in report.items() if k != "entries"}, ensure_ascii=False))


if __name__ == "__main__":
    main()
