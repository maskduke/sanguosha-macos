"""Compare packaged resources with a pinned Git tree without executing Lua."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path


def parse_git_tree(data):
    if data and not data.endswith(b"\0"):
        raise ValueError("Expected complete NUL-terminated git ls-tree records")
    result = {}
    for record in data.split(b"\0"):
        if not record:
            continue
        header, path = record.split(b"\t", 1)
        mode, kind, oid = header.decode("ascii").split()
        if kind != "blob" or mode not in ("100644", "100755"):
            raise ValueError("Only regular Git blob entries are supported")
        name = path.decode("utf-8")
        if not re.fullmatch(r"[0-9a-f]{40}", oid) or name in result:
            raise ValueError("Malformed or duplicate Git tree entry")
        result[name] = oid
    return result


def compare_resources(root, repository_entries):
    root = Path(root)
    if root.is_symlink() or not root.is_dir():
        raise ValueError("Expected a real resource directory")
    local = {}
    for path in sorted(root.rglob("*")):
        if path.is_symlink():
            raise ValueError("Symlink resource requires explicit review: " + str(path))
        if path.is_file():
            data = path.read_bytes()
            local[path.relative_to(root).as_posix()] = hashlib.sha1(
                b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
    common = local.keys() & repository_entries.keys()
    return {"matched": sorted(p for p in common if local[p] == repository_entries[p]),
            "changed": sorted(p for p in common if local[p] != repository_entries[p]),
            "local_only": sorted(local.keys() - repository_entries.keys()),
            "repository_only": sorted(repository_entries.keys() - local.keys())}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", required=True, type=Path)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--resources", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args()
    if not re.fullmatch(r"[0-9a-f]{40}", args.commit):
        parser.error("Use a full pinned commit SHA")
    if args.report.exists():
        parser.error("Choose a new evidence report; previous results are immutable")
    for protected in (args.repository, args.resources):
        if args.report.resolve().is_relative_to(protected.resolve()):
            parser.error("Report must not modify the source under examination")
    raw = subprocess.check_output(["git", "-C", str(args.repository), "ls-tree", "-rz", args.commit])
    result = compare_resources(args.resources, parse_git_tree(raw))
    report = {"schema": 1, "commit": args.commit,
              "resources": str(args.resources.resolve()), "comparison": result,
              "scope": "file content only; matching resources do not prove compiled engine equivalence"}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    with args.report.open("x") as stream:
        json.dump(report, stream, ensure_ascii=False, indent=2)
        stream.write("\n")
    print(json.dumps({"commit": args.commit, "counts": {k: len(v) for k, v in result.items()}}, ensure_ascii=False))


if __name__ == "__main__":
    main()
