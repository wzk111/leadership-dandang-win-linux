#!/usr/bin/env python3
"""Reject development/private files and inventory a staged product, never source text."""
import argparse
import hashlib
import json
import re
from pathlib import Path

FORBIDDEN_DIRS = {".git", ".local", ".cache", ".codex", ".agents", ".ssh", ".aws", "tests", "test-results",
                  "fixtures", "diagnostics", "traces", "include", "__pycache__"}
FORBIDDEN_SUFFIXES = {".pdb", ".obj", ".o", ".lib", ".a", ".h", ".hpp", ".pem", ".key", ".pyc"}
PRIVATE_NAME = re.compile(r"(^|[-_.])(api[-_]?key|credentials?|secrets?|trace)([-_.]|$)", re.I)


def inspect(root):
    root = Path(root)
    manifest, errors = [], []
    for file in sorted(root.rglob("*")):
        relative = file.relative_to(root).as_posix()
        parts = relative.lower().split("/")
        name = parts[-1]
        if file.is_symlink():
            errors.append(f"Symlink not allowed: {relative}")
            continue
        if not file.is_file():
            continue
        forbidden = (any(p in FORBIDDEN_DIRS or p.startswith(("build", ".env"))
                         or PRIVATE_NAME.search(p) for p in parts[:-1])
                     or name.startswith(("test_", "test-", ".env"))
                     or name in {"cmakecache.txt", "cmake_install.cmake", "makefile"}
                     or file.suffix.lower() in FORBIDDEN_SUFFIXES or PRIVATE_NAME.search(name))
        if forbidden:
            errors.append(f"Forbidden package path: {relative}")
        manifest.append({"path": relative, "bytes": file.stat().st_size,
                         "sha256": _hash(file)})
    if not manifest:
        errors.append("Package is empty or missing")
    return manifest, errors


def _hash(file):
    digest = hashlib.sha256()
    with file.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("--manifest", type=Path)
    args = parser.parse_args()
    manifest, errors = inspect(args.root)
    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    for error in errors:
        print(error)
    print(f"Product manifest: {len(manifest)} files; {len(errors)} violations")
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
