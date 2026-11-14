"""Version-driven banner generation regression; no network or API."""
from pathlib import Path
import subprocess,sys,tempfile,unittest,shutil
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from verify_package import png_banner_version
from generate_version import check_runtime,expected_header
from check_distribution import check_distribution
class BannerTests(unittest.TestCase):
 def test_version_change_deterministic_and_links(self):
  with tempfile.TemporaryDirectory() as folder:
   root=Path(folder);(root/'scripts').mkdir();shutil.copyfile(ROOT/'scripts/build_download_banner.py',root/'scripts/build_download_banner.py')
   (root/'VERSION').write_text('0.4.2\n');subprocess.run([sys.executable,str(root/'scripts/build_download_banner.py')],check=True)
   png=root/'docs/assets/download-button.png';svg=root/'docs/assets/download-button.svg';before=png.read_bytes()
   self.assertEqual(png_banner_version(png),'0.4.2');self.assertIn('0.4.2 · Unofficial',svg.read_text(encoding='utf-8'));self.assertIn('/v0.4.2/AI-Image-Bridge-0.4.2-',svg.read_text(encoding="utf-8"))
   subprocess.run([sys.executable,str(root/'scripts/build_download_banner.py')],check=True);self.assertEqual(before,png.read_bytes())
   (root/'VERSION').write_text('0.4.3\n');subprocess.run([sys.executable,str(root/'scripts/build_download_banner.py')],check=True)
   self.assertEqual(png_banner_version(png),'0.4.3');self.assertNotEqual(before,png.read_bytes());self.assertIn('/v0.4.3/',svg.read_text(encoding="utf-8"))
 def test_missing_or_invalid_metadata_fails(self):
  with tempfile.TemporaryDirectory() as folder:
   path=Path(folder)/'bad.png';path.write_bytes(b'not png')
   with self.assertRaises(ValueError):png_banner_version(path)
   path.write_bytes(b'\x89PNG\r\n\x1a\n')
   with self.assertRaises(ValueError):png_banner_version(path)
class NativeStopIntegrationTests(unittest.TestCase):
 def test_owner_draw_and_hint_use_tested_shared_helpers(self):
  source=(ROOT/'src/ui.cpp').read_text(encoding='utf-8')
  self.assertIn('danger?emergencyStopBackground(disabled,',source)
  self.assertIn('danger?emergencyStopBorder(disabled)',source)
  self.assertIn('cloud::emergencyStopWaitHint()',source)
  self.assertNotIn('中止ボタン',source)
  self.assertNotIn('RGB(127,29,29)',source)
class RuntimeVersionTests(unittest.TestCase):
 def test_current_distribution_and_runtime_match_version(self):
  self.assertEqual(check_distribution(),(ROOT/'VERSION').read_text().strip())
 def test_stale_readme_and_pdf_are_rejected(self):
  with tempfile.TemporaryDirectory() as folder:
   root=Path(folder);shutil.copytree(ROOT/'src',root/'src');shutil.copytree(ROOT/'docs',root/'docs');shutil.copyfile(ROOT/'VERSION',root/'VERSION');shutil.copyfile(ROOT/'README.md',root/'README.md')
   version=(ROOT/'VERSION').read_text().strip();path=root/'README.md';original=path.read_bytes();path.write_bytes(original.replace(('/releases/tag/v'+version).encode(),b'/releases/tag/v0.0.1'))
   with self.assertRaisesRegex(ValueError,'Stale README release link'):check_distribution(root)
   path.write_bytes(original)
   from pypdf import PdfWriter
   writer=PdfWriter();writer.add_blank_page(width=595,height=842)
   with (root/'docs/manuals/OpenAI-API-Setup.pdf').open('wb') as out:writer.write(out)
   with self.assertRaisesRegex(ValueError,'Stale PDF version'):check_distribution(root)
 def test_stale_generated_header_and_runtime_literal_are_rejected(self):
  with tempfile.TemporaryDirectory() as folder:
   root=Path(folder);shutil.copytree(ROOT/'src',root/'src');shutil.copyfile(ROOT/'VERSION',root/'VERSION')
   (root/'src/generated_version.h').write_bytes(b'old header')
   with self.assertRaisesRegex(ValueError,'Stale generated'):check_runtime(root)
   (root/'src/generated_version.h').write_bytes(expected_header(root));check_runtime(root)
   path=root/'src/ui.cpp';path.write_bytes(path.read_bytes()+b'\\n// old runtime version 0.4.1\\n')
   with self.assertRaisesRegex(ValueError,'Hardcoded runtime'):check_runtime(root)
 def test_changed_version_requires_regeneration_and_manual_sync(self):
  with tempfile.TemporaryDirectory() as folder:
   root=Path(folder);shutil.copytree(ROOT/'src',root/'src');(root/'VERSION').write_text('0.4.4\n')
   with self.assertRaisesRegex(ValueError,'Stale generated'):check_runtime(root)
if __name__=='__main__':unittest.main()
