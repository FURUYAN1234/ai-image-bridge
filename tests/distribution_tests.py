"""Version-driven banner generation regression; no network or API."""
from pathlib import Path
import subprocess,sys,tempfile,unittest,shutil
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from verify_package import png_banner_version
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
if __name__=='__main__':unittest.main()
