#!/usr/bin/env python3
"""Fail closed on public-release leftovers, local paths and likely secrets."""
from __future__ import annotations
import argparse
import re
import sys
from pathlib import Path
from PIL import Image
DEFAULT_ROOT = Path(__file__).resolve().parents[1]
MAX_BYTES = 5 * 1024 * 1024
BANNED_PARTS = {".git", ".pio", ".venv", "venv", "__pycache__", ".vscode", ".idea", ".vs", "Objects", "Listings", "DebugConfig", "build", "dist", "release"}
BANNED_SUFFIXES = {".elf", ".bin", ".hex", ".map", ".o", ".a", ".pyc", ".db", ".sqlite", ".pcap", ".jpg", ".jpeg", ".png", ".bmp", ".gif", ".webp", ".mp4", ".mov", ".avi", ".mkv", ".zip", ".7z", ".tar", ".gz", ".xz", ".pem", ".key", ".p12", ".jks", ".exe", ".dll", ".so", ".axf", ".crf", ".lst", ".dep", ".scvd", ".uvoptx", ".uvprojx"}
FORBIDDEN_NAMES = {".env", "local.properties", "credentials.h", "credentials.json", "secrets.h", "secrets.json", "id_rsa", "id_ed25519"}
TEXT_SUFFIXES = {".c", ".h", ".py", ".ini", ".yml", ".yaml", ".json", ".jsonc", ".sh", ".txt", ".csv", ".md", ".svg"}
EXTENSIONLESS = {"LICENSE", "NOTICE", "Makefile", ".gitignore", ".gitattributes"}
SECRET_PATTERNS = (
    r"ghp_[A-Za-z0-9]{20,}", r"github_pat_[A-Za-z0-9_]{20,}", r"AKIA[0-9A-Z]{16}",
    r"sk-[A-Za-z0-9_-]{16,}", r"-----BEGIN [A-Z ]*PRIVATE KEY-----",
    r"(?ix)\b(?:api[_-]?key|access[_-]?token|auth[_-]?token|secret|password|passwd|pwd)\b\s*[:=]\s*[\"']?(?!YOUR_|EXAMPLE|REPLACE|CHANGEME|REDACTED|\[REDACTED\]|<YOUR_)[A-Za-z0-9+/=_!@#$%^&*.-]{8,}",
)
PATH_PATTERNS = (r"/" + "home/", r"/" + "Users/", r"[A-Za-z]:" + r"\\Users\\")
CONTENT_ALLOWLIST = {"scripts/secret_scan.py"}
BANNED_CONTENT = (r"wardrobe_project" + r"\.uvguix", r"\.build_log" + r"\.htm", r"Product:" + r"\s*MDK", "License " + "Information")
PUBLIC_IMAGES = {"assets/photos/historical-prototype.jpg"}

def allowed_text(path: Path) -> bool: return path.suffix.lower() in TEXT_SUFFIXES or path.name in EXTENSIONLESS

def scan(root: Path) -> list[str]:
    failures: list[str] = []
    for path in sorted(root.rglob("*")):
        rel = path.relative_to(root)
        if rel.parts and rel.parts[0] == ".git": continue
        if path.is_symlink(): failures.append(f"symbolic link is not allowed: {rel}"); continue
        if set(rel.parts) & BANNED_PARTS: failures.append(f"forbidden path: {rel}"); continue
        if not path.is_file(): continue
        if path.name in FORBIDDEN_NAMES or path.name.startswith(".env.") or path.name.startswith("config.local.") or path.name.startswith("wardrobe_project.uvguix."):
            failures.append(f"forbidden local/config filename: {rel}"); continue
        if rel.as_posix() in PUBLIC_IMAGES:
            try:
                with Image.open(path) as image: image.verify()
                with Image.open(path) as image:
                    allowed_info = {"jfif", "jfif_version", "jfif_unit", "jfif_density", "progressive", "progression", "transparency"}
                    if len(image.getexif()) != 0: failures.append(f"public image contains EXIF metadata: {rel}")
                    if set(image.info) - allowed_info: failures.append(f"public image contains unexpected metadata: {rel}")
            except OSError as error: failures.append(f"invalid public image {rel}: {error}")
            continue
        if path.suffix.lower() in BANNED_SUFFIXES: failures.append(f"forbidden artifact: {rel}"); continue
        if not allowed_text(path): failures.append(f"unallowlisted file type: {rel}"); continue
        if path.stat().st_size > MAX_BYTES: failures.append(f"file exceeds publication limit: {rel}"); continue
        raw = path.read_bytes()
        if b"\x00" in raw: failures.append(f"NUL byte in public text: {rel}"); continue
        try: content = raw.decode("utf-8")
        except UnicodeDecodeError: failures.append(f"non-UTF-8 public text: {rel}"); continue
        if rel.as_posix() not in CONTENT_ALLOWLIST:
            for pattern in PATH_PATTERNS:
                if re.search(pattern, content): failures.append(f"local absolute path: {rel}")
        if rel.as_posix() not in CONTENT_ALLOWLIST:
            for pattern in SECRET_PATTERNS:
                if re.search(pattern, content, flags=re.IGNORECASE): failures.append(f"credential pattern: {rel}")
            for pattern in BANNED_CONTENT:
                if re.search(pattern, content, flags=re.IGNORECASE): failures.append(f"private build-log content: {rel}")
    return sorted(set(failures))

def main() -> int:
    parser = argparse.ArgumentParser(); parser.add_argument("--root", type=Path, default=DEFAULT_ROOT)
    failures = scan(parser.parse_args().root.resolve())
    if failures:
        print("secret/path scan: FAIL", file=sys.stderr); print("\n".join(f"- {item}" for item in failures), file=sys.stderr); return 1
    print("secret/path scan: PASS"); return 0
if __name__ == "__main__": raise SystemExit(main())
