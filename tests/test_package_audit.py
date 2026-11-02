"""Product manifests are an actual release boundary, independent of C++ fixtures."""
import importlib.util
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("audit", Path(__file__).parents[1] / "scripts/audit-package.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class PackageAuditTest(unittest.TestCase):
    def check_files(self, files):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            for name in files:
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"synthetic")
            return audit.inspect(root)

    def test_product_runtime_and_notices(self):
        manifest, errors = self.check_files(["worksidekick.exe", "worksidekick_uia_worker.exe",
            "platforms/qwindows.dll", "msvcp140.dll", "licenses/LGPL-3.0.txt",
            "usr/share/doc/worksidekick/privacy.md"])
        self.assertFalse(errors)
        self.assertEqual(len(manifest), 6)

    def test_forbidden_development_and_private_material(self):
        for name in ["test_uia.exe", "tests/fixture.txt", ".env", ".env.production",
            ".git/config", "build/CMakeCache.txt", "x.pdb", "key.pem", "api-key.txt",
            "credentials.json", "diagnostics/trace.txt", "include/QWidget.h", "lib/Qt6Core.lib"]:
            with self.subTest(name=name):
                _, errors = self.check_files([name])
                self.assertTrue(errors, name)

    def test_empty_package_is_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            self.assertTrue(audit.inspect(Path(folder))[1])

    def test_private_directory_names_are_rejected(self):
        for name in ["secrets/openai.json", "credentials/providers.json", ".env/production",
                     ".ssh/id_rsa", ".aws/config"]:
            with self.subTest(name=name):
                self.assertTrue(self.check_files([name])[1], name)


if __name__ == "__main__":
    unittest.main()
