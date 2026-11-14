"""Read-only source/runtime/banner/manual/release version gate."""
from pathlib import Path
import re
from generate_version import check_runtime
from verify_package import png_banner_version
ROOT=Path(__file__).resolve().parents[1]
def check_distribution(root=ROOT):
 version=(root/'VERSION').read_text(encoding='utf-8').strip();check_runtime(root)
 if png_banner_version(root/'docs/assets/download-button.png')!=version:raise ValueError('Stale source PNG banner')
 svg=(root/'docs/assets/download-button.svg').read_text(encoding='utf-8')
 if f'{version} · Unofficial SDK' not in svg or f'/v{version}/AI-Image-Bridge-{version}-Windows-x64.zip' not in svg:raise ValueError('Stale source SVG banner')
 readme=(root/'README.md').read_text(encoding='utf-8')
 if not readme.startswith('# AI Image Bridge '+version+'\n'):raise ValueError('Stale README title')
 urls=re.findall(r'https://github.com/FURUYAN1234/ai-image-bridge/releases/(?:download|tag)/v[^)\s]+',readme)
 expected={f'https://github.com/FURUYAN1234/ai-image-bridge/releases/download/v{version}/AI-Image-Bridge-{version}-Windows-x64.zip',f'https://github.com/FURUYAN1234/ai-image-bridge/releases/tag/v{version}'}
 if set(urls)!=expected:raise ValueError('Stale README release link')
 if not (root/f'docs/releases/v{version}.md').read_text(encoding='utf-8').startswith('# AI Image Bridge '+version):raise ValueError('Stale release notes')
 for name in ['manual-plugin.md','manual-openai.md','manual-gemini.md']:
  if not (root/'docs'/name).read_text(encoding='utf-8').startswith('# AI Image Bridge '+version+':'):raise ValueError('Stale manual source: '+name)
 from pypdf import PdfReader
 for name in ['AI-Image-Bridge-Manual.pdf','OpenAI-API-Setup.pdf','Gemini-API-Setup.pdf']:
  pages=PdfReader(root/'docs/manuals'/name).pages
  for page in pages:
   if 'AI Image Bridge '+version not in page.extract_text():raise ValueError('Stale PDF version: '+name)
 return version
if __name__=='__main__':print('DISTRIBUTION_VERSION_VERIFIED '+check_distribution())
