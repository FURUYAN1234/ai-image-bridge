from pathlib import Path
import re, html, json, os
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, PageBreak, Table, TableStyle, Image, KeepTogether, Flowable
from reportlab.lib.pagesizes import A4
ROOT=Path(__file__).resolve().parents[1]
VERSION=(ROOT/'VERSION').read_text(encoding='utf-8').strip()
OUT=Path(os.environ.get('BRIDGE_MANUAL_OUTPUT', str(ROOT/'docs/manuals'))); OUT.mkdir(parents=True,exist_ok=True)
PRE=OUT/'renders'; PRE.mkdir(exist_ok=True)
font=str(Path('C:/Windows/Fonts/msgothic.ttc') if Path('C:/Windows/Fonts/msgothic.ttc').exists() else Path('/mnt/c/Windows/Fonts/msgothic.ttc'))
pdfmetrics.registerFont(TTFont('JP',font,subfontIndex=0))
pdfmetrics.registerFontFamily('JP',normal='JP',bold='JP',italic='JP',boldItalic='JP')
INK=colors.HexColor('#172D3C'); ACC=colors.HexColor('#087F8C'); LIGHT=colors.HexColor('#EEF5F6')
styles={
'body':ParagraphStyle('body',fontName='JP',fontSize=9,leading=14,spaceAfter=8,wordWrap='CJK',textColor=INK),
'h1':ParagraphStyle('h1',fontName='JP',fontSize=23,leading=32,spaceAfter=16,textColor=INK,wordWrap='CJK'),
'h2':ParagraphStyle('h2',fontName='JP',fontSize=14,leading=21,spaceBefore=15,spaceAfter=9,textColor=ACC,keepWithNext=True,wordWrap='CJK'),
'h3':ParagraphStyle('h3',fontName='JP',fontSize=11,leading=17,spaceBefore=10,spaceAfter=6,textColor=INK,keepWithNext=True,wordWrap='CJK'),
'cell':ParagraphStyle('cell',fontName='JP',fontSize=8,leading=12,wordWrap='CJK',textColor=INK),
'small':ParagraphStyle('small',fontName='JP',fontSize=8,leading=12,wordWrap='CJK',textColor=INK),
}
def inline(s):
    s=html.escape(s)
    s=re.sub(r'\[([^\]]+)\]\((https?://[^)]+)\)',lambda m:f'<link href="{m[2]}" color="#087F8C"><u>{m[1]}</u></link>',s)
    s=re.sub(r'`([^`]+)`',r'<font color="#087F8C">\1</font>',s)
    s=re.sub(r'\*\*([^*]+)\*\*',r'<b>\1</b>',s)
    return s
class Workflow(Flowable):
    def __init__(self,labels): Flowable.__init__(self); self.labels=labels; self.width=475; self.height=83
    def draw(self):
        c=self.canv; w=145
        for i,(a,b) in enumerate(self.labels):
            x=i*164; c.setFillColor(LIGHT); c.setStrokeColor(ACC); c.roundRect(x,12,w,61,7,fill=1,stroke=0)
            c.setFillColor(ACC); c.setFont('JP',10); c.drawString(x+10,56,f'{i+1:02d}')
            c.setFillColor(INK); c.setFont('JP',8); c.drawString(x+10,38,a); c.drawString(x+10,23,b)
            if i<2: c.setStrokeColor(ACC); c.line(x+w+3,42,x+w+15,42); c.line(x+w+11,46,x+w+15,42); c.line(x+w+11,38,x+w+15,42)
def footer(c,d):
    c.setStrokeColor(colors.HexColor('#D8E3E6')); c.line(42,39,A4[0]-42,39)
    c.setFont('JP',7); c.setFillColor(INK); c.drawString(42,27,f'AI Image Bridge {VERSION}  |  English / 日本語'); c.drawRightString(A4[0]-42,27,str(d.page))
def convert(src,name,subtitle,steps):
    lines=(ROOT/'docs'/src).read_text(encoding='utf-8-sig').splitlines(); title=lines[0].lstrip('# ')
    headings=[x[3:] for x in lines if x.startswith('## ')]
    story=[Spacer(1,35),Paragraph('AI IMAGE BRIDGE',styles['small']),Spacer(1,15),Paragraph(title,styles['h1']),Paragraph(subtitle,styles['body']),Spacer(1,22),Workflow(steps),Spacer(1,20),Paragraph('Contents / 目次',styles['h2'])]
    for i,h in enumerate(headings,1): story.append(Paragraph(f'{i:02d}  {inline(h)}',styles['body']))
    story.append(PageBreak())
    if src=='manual-plugin.md':
        screens=[('ui-ja.png','Japanese interface / 日本語の操作画面'),('ui-en.png','English interface / 英語の操作画面')]
        for f,label in screens:
            p=ROOT/'docs/assets'/f
            if p.exists():
                story.append(Paragraph(label,styles['h2']))
                im=Image(str(p)); ratio=min(475/im.imageWidth,560/im.imageHeight); im.drawWidth=im.imageWidth*ratio; im.drawHeight=im.imageHeight*ratio
                story.extend([im,Spacer(1,12),Paragraph('Earlier AI Image Bridge 0.4.0 interface; the emergency-stop label and red styling are updated in 0.4.1. Select the provider and language, describe an edit, compare the preview, and apply or save. / 旧0.4.0の実画面です。0.4.1では緊急停止の表記と赤い配色へ更新しています。接続先と言語を選び、編集内容を入力し、候補を比較して適用または保存します。',styles['body']),PageBreak()])
    i=1
    while i<len(lines):
        s=lines[i].strip()
        if not s: i+=1; continue
        if s.startswith('|'):
            rows=[]
            while i<len(lines) and lines[i].strip().startswith('|'):
                cells=[x.strip() for x in lines[i].strip().strip('|').split('|')]
                if not all(re.fullmatch(r'[:\- ]+',x) for x in cells): rows.append([Paragraph(inline(x),styles['cell']) for x in cells])
                i+=1
            if rows:
                t=Table(rows,colWidths=[(A4[0]-84)/len(rows[0])]*len(rows[0]),repeatRows=1,hAlign='LEFT')
                t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),LIGHT),('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),('TOPPADDING',(0,0),(-1,-1),7),('BOTTOMPADDING',(0,0),(-1,-1),7),('LINEBELOW',(0,0),(-1,-1),.35,colors.HexColor('#D8E3E6'))])); story.extend([t,Spacer(1,9)])
            continue
        if s.startswith('```'):
            block=[]; i+=1
            while i<len(lines) and not lines[i].startswith('```'): block.append(lines[i]); i+=1
            story.append(Paragraph('<br/>'.join(html.escape(x) for x in block),styles['small'])); i+=1; continue
        m=re.match(r'!\[([^]]*)\]\(([^)]+)\)',s)
        if m:
            p=(ROOT/'docs'/m[2]).resolve()
            if p.exists():
                im=Image(str(p)); ratio=min(475/im.imageWidth,340/im.imageHeight); im.drawWidth=im.imageWidth*ratio; im.drawHeight=im.imageHeight*ratio; story.extend([im,Paragraph(inline(m[1]),styles['small']),Spacer(1,10)])
            i+=1; continue
        if s.startswith('### '): story.append(Paragraph(inline(s[4:]),styles['h3']))
        elif s.startswith('## '): story.append(Paragraph(inline(s[3:]),styles['h2']))
        elif s.startswith('# '): pass
        else:
            if s.startswith('- '): s='• '+s[2:]
            story.append(Paragraph(inline(s),styles['body']))
        i+=1
    if src=='manual-plugin.md':
        images=[('natural-applied.png','Add a person / 人物を追加'),('simple-replace-candidate.png','Replace with a dog / 犬へ置き換え'),('simple-remove-candidate.png','Remove a person / 人物を消去')]
        story.extend([PageBreak(),Paragraph('Editing examples / 編集例',styles['h2']),Paragraph('Accepted examples from the 0.3.x verification workflow; these illustrate editing behavior and do not substitute for current release validation. / 0.3.xの検証で採用した実例です。編集の動きを示すもので、現行配布版の検証を代用しません。',styles['body'])])
        for f,cap in images:
            p=ROOT/'docs/assets/examples'/f
            if p.exists():
                im=Image(str(p)); scale=min(350/im.imageWidth,230/im.imageHeight); im.drawWidth=im.imageWidth*scale; im.drawHeight=im.imageHeight*scale
                story.append(KeepTogether([Paragraph(cap,styles['h3']),im,Spacer(1,12)]))
    target=OUT/name
    doc=SimpleDocTemplate(str(target),pagesize=A4,rightMargin=42,leftMargin=42,topMargin=42,bottomMargin=53,title=title,author='AI Image Bridge',pageCompression=1)
    doc.build(story,onFirstPage=footer,onLaterPages=footer)
    return target
if __name__=='__main__':
    jobs=[('manual-plugin.md','AI-Image-Bridge-Manual.pdf','Operation and reference / 操作とリファレンス',[('Prepare layer','レイヤーを準備'),('Generate and compare','生成して比較'),('Apply or save','適用または保存')]),('manual-openai.md','OpenAI-API-Setup.pdf','Account, billing, key and connection / アカウント・課金・キー・接続',[('Account and billing','アカウントと課金'),('Create API key','APIキーを作成'),('Connect and test','接続して確認')]),('manual-gemini.md','Gemini-API-Setup.pdf','Project, billing, key and connection / プロジェクト・課金・キー・接続',[('Project and eligibility','プロジェクトと条件'),('Billing and API key','課金とAPIキー'),('Connect and test','接続して確認')])]
    import pymupdf as fitz
    result=[]
    for args in jobs:
        p=convert(*args); d=fitz.open(p); pages=[]
        for n,page in enumerate(d):
            png=PRE/f'{p.stem}-{n+1:02d}.png'; page.get_pixmap(matrix=fitz.Matrix(1.2,1.2)).save(png); pages.append(str(png))
        fonts={x[3] for pg in d for x in pg.get_fonts()}; txt=''.join(pg.get_text() for pg in d)
        result.append({'file':str(p),'pages':len(d),'fonts':sorted(fonts),'japanese_present':'日本語' in txt,'links':sum(len(pg.get_links()) for pg in d),'previews':pages})
    (PRE/'manifest.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
    print(json.dumps(result,ensure_ascii=False,indent=2))

