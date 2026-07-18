#!/usr/bin/env bash
# Run release gates in an isolated copy. No build output remains in this checkout.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d "${TMPDIR:-/tmp}/stm32-wardrobe-verify.XXXXXX")"
cleanup() { rm -rf "$TMP"; }
trap cleanup EXIT
COPY="$TMP/repo"
mkdir -p "$COPY"
(
  cd "$ROOT"
  tar --exclude='./.git' --exclude='./.pio' --exclude='./__pycache__' -cf - .
) | (cd "$COPY" && tar -xf -)
cd "$COPY"
python3 scripts/secret_scan.py
python3 scripts/source_manifest.py
python3 scripts/check_repo.py
PYTHONDONTWRITEBYTECODE=1 python3 -B -m unittest discover -s tests -v
pio run -e safe-default
firmware=".pio/build/safe-default/firmware.bin"
if [[ ! -f "$firmware" ]] || [[ "$(wc -c < "$firmware")" -le 1024 ]]; then echo "safe-default firmware is missing or implausibly small" >&2; exit 1; fi
# Compile-only coverage. This never uploads or executes GPIO.
pio run -e fan-output-opt-in
optin=".pio/build/fan-output-opt-in/firmware.bin"
if [[ ! -f "$optin" ]] || [[ "$(wc -c < "$optin")" -le 1024 ]]; then echo "fan opt-in firmware is missing or implausibly small" >&2; exit 1; fi
rm -rf .pio
find . -type d -name __pycache__ -prune -exec rm -rf {} +
python3 scripts/secret_scan.py
python3 scripts/source_manifest.py
python3 scripts/check_repo.py
if find "$ROOT" -type d -name .pio -o -type f \( -name '*.bin' -o -name '*.elf' -o -name '*.map' -o -name '*.o' \) | grep -q .; then echo "generated build output leaked into candidate checkout" >&2; exit 1; fi
printf 'Verification: PASS\n'
