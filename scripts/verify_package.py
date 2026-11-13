"""Strict package verifier: missing, changed, extra files and unsafe names fail."""
import argparse, hashlib, json
from pathlib import Path

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def verify(root):
    root=Path(root)
    sums=json.loads((root/'SHA256SUMS.json').read_text(encoding='utf-8-sig'))
    actual={str(p.relative_to(root)).replace('\\','/'):p for p in root.rglob('*') if p.is_file()}
    if any(p.is_symlink() for p in root.rglob('*')): raise ValueError('Symlinks are forbidden')
    expected=set(sums)|{'SHA256SUMS.json'}
    if set(actual)!=expected: raise ValueError(f'File inventory mismatch: missing={sorted(expected-set(actual))}; extra={sorted(set(actual)-expected)}')
    for name,want in sums.items():
        if Path(name).is_absolute() or '..' in Path(name).parts or '\\' in name: raise ValueError('Unsafe manifest path')
        if digest(actual[name])!=want: raise ValueError(f'Checksum mismatch: {name}')
    manifest=json.loads((root/'manifest.json').read_text(encoding='utf-8-sig'))
    if manifest['name']!='AI Image Bridge' or manifest['version']!='0.4.0' or manifest['variant'] not in ('unofficial','official'): raise ValueError('Invalid identity')
    mf=manifest['files']
    if set(mf)!=set(actual)-{'manifest.json','SHA256SUMS.json'}: raise ValueError('Plugin manifest inventory mismatch')
    if any(digest(actual[n])!=v for n,v in mf.items()): raise ValueError('Plugin manifest checksum mismatch')
    if manifest['pluginSha256']!=mf['AIImageBridge.cpm']: raise ValueError('Plugin declarations disagree')
    return {'variant':manifest['variant'],'files':len(actual),'pluginSha256':manifest['pluginSha256']}

if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('root');args=a.parse_args();print(json.dumps(verify(args.root),indent=2))
