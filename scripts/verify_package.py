"""Strict package verifier: missing, changed, extra files and unsafe names fail."""
import argparse, hashlib, json, re, struct
from pathlib import Path
VERSION=(Path(__file__).resolve().parents[1]/'VERSION').read_text(encoding='utf-8').strip()

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def png_banner_version(path):
    data=Path(path).read_bytes()
    if data[:8]!=b'\x89PNG\r\n\x1a\n': raise ValueError('Invalid banner PNG')
    offset=8
    while offset+12<=len(data):
        size=struct.unpack('>I',data[offset:offset+4])[0]
        kind=data[offset+4:offset+8];chunk=data[offset+8:offset+8+size]
        if kind==b'tEXt' and chunk.startswith(b'Version\0'): return chunk.split(b'\0',1)[1].decode('ascii')
        offset+=12+size
    raise ValueError('Missing download-banner version metadata; regenerate from VERSION')

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
    if manifest['name']!='AI Image Bridge' or manifest['version']!=VERSION or manifest['variant'] not in ('unofficial','official'): raise ValueError('Invalid identity')
    if png_banner_version(root/'docs/assets/download-button.png')!=manifest['version']: raise ValueError('Stale download-banner version')
    mf=manifest['files']
    if set(mf)!=set(actual)-{'manifest.json','SHA256SUMS.json'}: raise ValueError('Plugin manifest inventory mismatch')
    if any(digest(actual[n])!=v for n,v in mf.items()): raise ValueError('Plugin manifest checksum mismatch')
    if manifest['pluginSha256']!=mf['AIImageBridge.cpm']: raise ValueError('Plugin declarations disagree')
    readme=(root/'README.md').read_text(encoding='utf-8-sig')
    for image in re.findall(r'!\[[^\]]*\]\(([^)]+)\)',readme):
        if not image.startswith(('https://','http://')) and not (root/image).is_file():
            raise ValueError(f'Missing README image: {image}')
    return {'variant':manifest['variant'],'files':len(actual),'pluginSha256':manifest['pluginSha256']}

if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('root');args=a.parse_args();print(json.dumps(verify(args.root),indent=2))
