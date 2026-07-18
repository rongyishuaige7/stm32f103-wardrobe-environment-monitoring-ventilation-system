from __future__ import annotations
import importlib.util
import tempfile
import unittest
from pathlib import Path
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("secret_scan", ROOT / "scripts/secret_scan.py")
module = importlib.util.module_from_spec(spec); assert spec.loader; spec.loader.exec_module(module)

class ScannerContracts(unittest.TestCase):
    def scan_file(self, name: str, content: str = "safe") -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / name; path.parent.mkdir(parents=True, exist_ok=True); path.write_text(content, encoding="utf-8")
            return module.scan(Path(tmp))

    def test_rejects_local_path(self) -> None: self.assertTrue(self.scan_file("note.md", "/" + "home/example/private"))
    def test_rejects_github_token(self) -> None: self.assertTrue(self.scan_file("note.txt", "ghp_" + "abcdefghijklmnopqrstuvwxyz012345"))
    def test_rejects_keil_build_artifact(self) -> None: self.assertTrue(self.scan_file("firmware.axf"))
    def test_rejects_private_build_log_content(self) -> None: self.assertTrue(self.scan_file("note.md", "Product:" + " MDK"))
    def test_accepts_plain_public_text(self) -> None: self.assertEqual([], self.scan_file("note.md", "public teaching boundary"))

    def test_rejects_unallowlisted_image(self) -> None:
        self.assertTrue(self.scan_file("assets/photos/other.jpg", "not really an image"))

    def test_accepts_exact_sanitized_public_image(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "assets/photos/historical-prototype.jpg"
            path.parent.mkdir(parents=True, exist_ok=True)
            Image.new("RGB", (64, 64), "white").save(path, "JPEG")
            self.assertEqual([], module.scan(Path(tmp)))

    def test_rejects_metadata_in_exact_public_image(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "assets/photos/historical-prototype.jpg"
            path.parent.mkdir(parents=True, exist_ok=True)
            exif = Image.Exif()
            exif[0x010E] = "private"
            Image.new("RGB", (64, 64), "white").save(path, "JPEG", exif=exif)
            self.assertTrue(module.scan(Path(tmp)))

if __name__ == "__main__": unittest.main()
