#!/usr/bin/env python3
"""Validate the exact public inventory, structure and factual contracts."""
from __future__ import annotations
import csv
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]

def read(rel: str) -> str: return (ROOT / rel).read_text(encoding="utf-8")
def values(rel: str) -> set[str]: return {line.strip() for line in read(rel).splitlines() if line.strip() and not line.lstrip().startswith("#")}

def main() -> int:
    failures: list[str] = []
    expected = values("docs/public-files.txt")
    actual = {p.relative_to(ROOT).as_posix() for p in ROOT.rglob("*") if p.is_file() and ".git" not in p.relative_to(ROOT).parts and ".pio" not in p.relative_to(ROOT).parts and "__pycache__" not in p.relative_to(ROOT).parts}
    for rel in sorted(expected - actual): failures.append(f"missing public file: {rel}")
    for rel in sorted(actual - expected): failures.append(f"unexpected public file: {rel}")
    pio = read("platformio.ini"); cfg = read("include/app_config.h"); fan = read("src/fan.c"); main_src = read("src/main.c"); dht = read("src/dht11.c"); mq = read("src/mq135.c")
    contracts = {
        "platform = ststm32@19.5.0": pio, "board = genericSTM32F103C8": pio,
        "-D ENABLE_FAN_OUTPUT=0": pio, "-D ENABLE_STARTUP_FAN_TEST=0": pio,
        "#if ENABLE_FAN_OUTPUT == 1": fan, "s_state = FAN_OFF;": fan,
        "#define DHT11_BIT_ONE_US         50U": dht, "ADC1->CR2 |= ADC_CR2_EXTTRIG;": mq,
        'uart1_write_string(" demo_index=")': main_src, 'fan_reason = "hold";': main_src,
        "当前真机复测": read("README.md"), "未执行": read("docs/PROJECT_STATUS.md"),
        "PB0/PB1 不能直接驱动电机": read("HARDWARE.md"), "不是空气质量": read("README.md"),
    }
    for needle, content in contracts.items():
        if needle not in content: failures.append(f"missing fact contract: {needle}")
    if "ENABLE_FAN_OUTPUT 1" in cfg or "ENABLE_STARTUP_FAN_TEST 1" in cfg: failures.append("header default enables hardware output")
    default_block = fan.split("#else", 1)[1].split("#endif", 1)[0]
    for token in ("gpio_config_output_pp", "gpio_write_pin"):
        if token in default_block: failures.append(f"default fan block touches output helper: {token}")
    forbidden_claims = ("production ready", "current hardware verified", "system online", "真机通过", "空气质量百分比")
    for rel in ("README.md", "docs/PROJECT_STATUS.md", "docs/HARDWARE_LAB_CARD.md"):
        lower = read(rel).lower()
        for claim in forbidden_claims:
            if claim in lower: failures.append(f"unsupported claim in {rel}: {claim}")
    try: ET.parse(ROOT / "hardware/wiring-diagram.svg")
    except (ET.ParseError, OSError) as error: failures.append(f"invalid wiring SVG: {error}")
    try:
        rows = list(csv.DictReader((ROOT / "hardware/BOM.csv").open(newline="", encoding="utf-8")))
        if len(rows) < 10: failures.append("BOM must contain at least 10 rows")
    except (OSError, csv.Error) as error: failures.append(f"invalid BOM: {error}")
    link_pattern = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
    for md in ROOT.rglob("*.md"):
        for target in link_pattern.findall(md.read_text(encoding="utf-8")):
            target = target.strip().split(" ", 1)[0]
            if target.startswith(("http://", "https://", "mailto:", "#")): continue
            clean = target.split("#", 1)[0]
            if clean and not (md.parent / clean).resolve().exists(): failures.append(f"broken local link in {md.relative_to(ROOT)}: {target}")
    if failures:
        print("repository check: FAIL", file=sys.stderr); print("\n".join(f"- {item}" for item in sorted(set(failures))), file=sys.stderr); return 1
    print(f"repository check: PASS ({len(actual)} exact public files)"); return 0
if __name__ == "__main__": raise SystemExit(main())
