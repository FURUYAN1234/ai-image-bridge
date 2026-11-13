"""Build an offline unofficial release package from this checkout. No publishing."""
from pathlib import Path
import argparse,json,shutil,zipfile,hashlib
from verify_package import digest,verify
ROOT=Path(__file__).resolve().parent.parent
PDFS=['AI-Image-Bridge-Manual.pdf','OpenAI-API-Setup.pdf','Gemini-API-Setup.pdf']
ASSETS=['cover.png','before.png','add-openai.png','replace-gemini.png','remove-openai.png','ui-ja.png','ui-en.png','workflow.svg','workflow.png','040-add-openai.png','040-replace-gemini.png']
def package(stage):
 if stage.exists() and any(stage.iterdir()):raise ValueError('Stage must be new or empty')
 pairs=[('build/unofficial/AIImageBridge.cpm','AIImageBridge.cpm'),('LICENSE','LICENSE'),('THIRD_PARTY_NOTICES.md','THIRD_PARTY_NOTICES.md'),('vendor/compat/LICENSE','licenses/SDPlugin-Unlicense.txt'),('vendor/LICENSE.nlohmann-json.txt','licenses/nlohmann-json-MIT.txt')]
 pairs += [('scripts/'+n,n) for n in ['Install.cmd','Install.ps1','Uninstall.cmd','Uninstall.ps1']]
 pairs += [('docs/manuals/'+n,'manuals/'+n) for n in PDFS]
 pairs += [('docs/assets/'+n,'docs/assets/'+n) for n in ASSETS]
 missing=[s for s,_ in pairs if not (ROOT/s).is_file()]
 if missing:raise FileNotFoundError(', '.join(missing))
 for source,dest in pairs:
  target=stage/dest;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/source,target)
 readme=(ROOT/'README.md').read_text(encoding='utf-8-sig').replace('](docs/manuals/','](manuals/')
 (stage/'README.md').write_text(readme,encoding='utf-8')
 files={p.relative_to(stage).as_posix():digest(p) for p in sorted(stage.rglob('*')) if p.is_file()}
 (stage/'manifest.json').write_text(json.dumps({'name':'AI Image Bridge','version':'0.4.0','variant':'unofficial','pluginSha256':files['AIImageBridge.cpm'],'files':files},indent=2)+'\n',encoding='utf-8')
 sums={p.relative_to(stage).as_posix():digest(p) for p in sorted(stage.rglob('*')) if p.is_file()}
 (stage/'SHA256SUMS.json').write_text(json.dumps(sums,indent=2)+'\n',encoding='utf-8');return verify(stage)
def archive(stage,dest):
 with zipfile.ZipFile(dest,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for p in sorted(stage.rglob('*')):
   if p.is_file():
    info=zipfile.ZipInfo(p.relative_to(stage).as_posix(),(2026,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;z.writestr(info,p.read_bytes())
 return digest(dest)
if __name__=='__main__':
 a=argparse.ArgumentParser();a.add_argument('--stage',type=Path,required=True);a.add_argument('--zip',type=Path);x=a.parse_args();result=package(x.stage)
 if x.zip:result['zipSha256']=archive(x.stage,x.zip)
 print(json.dumps(result,indent=2))
