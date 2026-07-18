from __future__ import annotations
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
def text(rel: str) -> str: return (ROOT / rel).read_text(encoding="utf-8")

class PublicSourceContracts(unittest.TestCase):
    def test_pinned_stm32cube_build(self) -> None:
        ini = text("platformio.ini")
        for value in ("platform = ststm32@19.5.0", "board = genericSTM32F103C8", "framework = stm32cube", "-D STM32F103xB"):
            self.assertIn(value, ini)

    def test_fan_output_and_startup_test_are_default_off(self) -> None:
        ini = text("platformio.ini"); cfg = text("include/app_config.h"); fan = text("src/fan.c"); main = text("src/main.c")
        self.assertIn("-D ENABLE_FAN_OUTPUT=0", ini); self.assertIn("-D ENABLE_STARTUP_FAN_TEST=0", ini)
        self.assertNotIn("#define ENABLE_FAN_OUTPUT 1", cfg); self.assertNotIn("#define ENABLE_STARTUP_FAN_TEST 1", cfg)
        self.assertIn("#if ENABLE_FAN_OUTPUT == 1", fan)
        default = fan.split("#else", 1)[1].split("#endif", 1)[0]
        self.assertNotIn("gpio_write_pin", default); self.assertNotIn("gpio_config_output_pp", default)
        self.assertIn("#if ENABLE_STARTUP_FAN_TEST == 1", main)

    def test_pin_and_threshold_contracts(self) -> None:
        cfg = text("include/app_config.h")
        for value in ("HUMIDITY_ON_THRESHOLD      75U", "HUMIDITY_OFF_THRESHOLD     70U", "MQ_RAW_ON_THRESHOLD        2300U", "MQ_RAW_OFF_THRESHOLD       2000U", "DHT11_PIN    12U", "MQ135_PIN    0U", "OLED_SCL_PIN  6U", "OLED_SDA_PIN  7U", "OLED_I2C_ADDRESS 0x78U", "MQ135_SAMPLE_COUNT          16U"):
            self.assertIn(value, cfg)

    def test_mq_semantics_are_narrow(self) -> None:
        source = text("src/main.c")
        self.assertIn('uart1_write_string(" demo_index=")', source)
        self.assertIn('make_line(line, "Idx:", demo_index, "")', source)
        self.assertIn("fan_state = fan_get();", source)
        for stale in ("air_pct", '"Air:"', "AIR_RAW_ON_THRESHOLD", "AIR_RAW_OFF_THRESHOLD", 'fan_reason = "air"'):
            self.assertNotIn(stale, source)

    def test_dht_timing_and_adc_software_trigger_fix(self) -> None:
        self.assertIn("DHT11_BIT_ONE_US         50U", text("src/dht11.c"))
        mq = text("src/mq135.c")
        self.assertIn("ADC1->CR2 |= ADC_CR2_EXTTRIG;", mq)
        self.assertIn("ADC1->CR2 |= ADC_CR2_SWSTART;", mq)

    def test_hold_reason_is_not_shadowed(self) -> None:
        source = text("src/main.c")
        hold = source.index("(reason[1] == 'o')")
        humidity = source.index("if (reason[0] == 'h')", hold)
        self.assertLess(hold, humidity)
        self.assertIn('return "HOLD";', source[hold:humidity])

    def test_boot_logs_do_not_claim_sensor_success(self) -> None:
        source = text("src/main.c")
        self.assertIn("DHT11 interface initialized", source); self.assertIn("MQ ADC interface initialized", source)
        self.assertNotIn("BOOT: DHT11 OK", source); self.assertNotIn("BOOT: MQ135 OK", source)

    def test_historical_toolchain_and_font_are_not_copied(self) -> None:
        all_names = {p.name for p in ROOT.rglob("*") if p.is_file()}
        for name in ("stm32f10x.h", "core_cm3.h", "startup_stm32f10x_md.s", "system_stm32f10x.c"):
            self.assertNotIn(name, all_names)
        self.assertNotIn("font6x8[][6]", text("src/oled.c"))
        self.assertIn("Minimal glyph set authored", text("src/oled.c"))
        self.assertIn("Glyph4x5", text("src/oled.c"))

    def test_public_source_is_exactly_allowlisted(self) -> None:
        expected = {line.strip() for line in text("docs/public-source-allowlist.txt").splitlines() if line.strip() and not line.lstrip().startswith("#")}
        actual = {"platformio.ini"} | {p.relative_to(ROOT).as_posix() for base in (ROOT / "src", ROOT / "include") for p in base.rglob("*") if p.is_file()}
        self.assertEqual(expected, actual)

    def test_hardware_safety_boundaries_are_explicit(self) -> None:
        self.assertIn("不是空气质量", text("README.md")); self.assertIn("PB0/PB1 不能直接驱动电机", text("HARDWARE.md"))

if __name__ == "__main__": unittest.main()
