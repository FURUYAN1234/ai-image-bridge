"""Generate only the download banner from VERSION; no README injection or publication."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, PngImagePlugin
from html import escape
import argparse,re
ROOT=Path(__file__).resolve().parents[1]
VERSION=(ROOT/'VERSION').read_text(encoding='utf-8').strip()
if not re.fullmatch(r'\d+\.\d+\.\d+',VERSION): raise ValueError('Invalid VERSION')
a=argparse.ArgumentParser();a.add_argument('--output',type=Path,default=ROOT/'docs/assets');args=a.parse_args()
ASSETS=args.output;ASSETS.mkdir(parents=True,exist_ok=True)
S=2
BASE='https://github.com/FURUYAN1234/ai-image-bridge'
specs=[('download-button','#0e1d30','#ffffff','#65e3ee','ZIP','ダウンロード','Download Windows x64 ZIP',f'{VERSION} · Unofficial SDK / 非公式SDK版',f'{BASE}/releases/download/v{VERSION}/AI-Image-Bridge-{VERSION}-Windows-x64.zip')]
fontdir=Path('C:/Windows/Fonts') if Path('C:/Windows/Fonts').exists() else Path('/mnt/c/Windows/Fonts')
font=lambda n,b=False:ImageFont.truetype(str(fontdir/('meiryob.ttc' if b else 'meiryo.ttc')),n*S)
metadata=PngImagePlugin.PngInfo();metadata.add_text('Version',VERSION)
for name,bg,ink,accent,icon,jp,en,sub,url in specs:
 im=Image.new('RGB',(1200*S,240*S),bg); d=ImageDraw.Draw(im)
 d.rounded_rectangle((3*S,3*S,1197*S,237*S),radius=25*S,fill=bg,outline=accent,width=4*S)
 d.rounded_rectangle((30*S,40*S,156*S,200*S),radius=16*S,fill='#f1f5f9')
 f=font(26,True); box=d.textbbox((0,0),icon,font=f); d.text(((93*S-(box[2]-box[0])/2),83*S),icon,font=f,fill='#172a43')
 d.line((60*S,153*S,127*S,153*S),fill='#172a43',width=4*S)
 texts=[(188,18,'AI Image Bridge',24,accent,False),(188,58,jp,39,ink,True),(188,117,en,30,ink,False),(188,177,sub,23,accent,False)]
 for x,y,t,size,color,bold in texts:
  f=font(size,bold); bound=d.textbbox((0,0),t,font=f)
  assert x*S+bound[2]<1056*S,(name,t,'text would overlap arrow')
  assert y*S+bound[3]<234*S,(name,t,'text clipped')
  d.text((x*S,y*S),t,font=f,fill=color)
 d.ellipse((1084*S,79*S,1170*S,165*S),fill=accent)
 d.line((1105*S,122*S,1147*S,122*S),fill=bg,width=5*S)
 d.line((1132*S,106*S,1148*S,122*S,1132*S,138*S),fill=bg,width=5*S)
 im=im.resize((1200,240),Image.Resampling.LANCZOS); im.save(ASSETS/(name+'.png'),optimize=True, pnginfo=metadata)
 title=f'{jp} / {en}'; desc=f'{sub}. Link / リンク先: {url}'
 svg=[f'<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="240" viewBox="0 0 1200 240" role="img" aria-labelledby="{name}-title {name}-desc">',f'<title id="{name}-title">{escape(title)}</title><desc id="{name}-desc">{escape(desc)}</desc>',f'<rect x="3" y="3" width="1194" height="234" rx="25" fill="{bg}" stroke="{accent}" stroke-width="4"/>','<rect x="30" y="40" width="126" height="160" rx="16" fill="#f1f5f9"/>',f'<text x="93" y="116" text-anchor="middle" fill="#172a43" font-size="26" font-weight="700" font-family="Meiryo,sans-serif">{icon}</text>','<path d="M60 153H127" stroke="#172a43" stroke-width="4"/>']
 for x,y,t,size,color,bold in texts:
  svg.append(f'<text x="{x}" y="{y+size}" fill="{color}" font-family="Meiryo,sans-serif" font-size="{size}" font-weight="{700 if bold else 400}">{escape(t)}</text>')
 svg += [f'<circle cx="1127" cy="122" r="43" fill="{accent}"/>',f'<path d="M1105 122H1147M1132 106L1148 122L1132 138" fill="none" stroke="{bg}" stroke-width="5"/>','</svg>']
 (ASSETS/(name+'.svg')).write_bytes(('\n'.join(svg)+'\n').encode('utf-8'))
