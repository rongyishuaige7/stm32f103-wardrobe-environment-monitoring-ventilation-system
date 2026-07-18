#!/usr/bin/env python3
"""Fail closed unless public firmware/config files match the exact allowlist."""
from __future__ import annotations
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
ALLOW = ROOT / "docs/public-source-allowlist.txt"
SOURCE_ROOTS = (ROOT / "src", ROOT / "include")

def values(path: Path) -> set[str]:
    return {line.strip() for line in path.read_text(encoding="utf-8").splitlines() if line.strip() and not line.lstrip().startswith("#")}

def main() -> int:
    expected = values(ALLOW)
    actual = {"platformio.ini"}
    for base in SOURCE_ROOTS:
        for path in base.rglob("*"):
            if path.is_file(): actual.add(path.relative_to(ROOT).as_posix())
    missing = sorted(expected - actual)
    extra = sorted(actual - expected)
    if missing or extra:
        print("source manifest: FAIL", file=sys.stderr)
        if missing: print("- missing: " + ", ".join(missing), file=sys.stderr)
        if extra: print("- unexpected: " + ", ".join(extra), file=sys.stderr)
        return 1
    print(f"source manifest: PASS ({len(actual)} files)")
    return 0
if __name__ == "__main__": raise SystemExit(main())
