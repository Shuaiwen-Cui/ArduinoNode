from __future__ import annotations

from pathlib import Path
import json
import win32com.client

ROOT = Path(r'E:/PROJ/ArduinoNode/DEMO')
SOURCE = ROOT / 'ArduinoNode.pptx'
OUTPUT = ROOT / '.ppt-build/output/ArduinoNode_NTU_student_polished_v4.pptx'
PHOTO = ROOT / '.ppt-build/assets/ArduinoNode-6-1.jpeg'

def rgb(s):
    s=s.lstrip('#'); return int(s[0:2],16)+int(s[2:4],16)*256+int(s[4:6],16)*65536

BLUE=rgb('#28617D')
NAVY=rgb('#244B63')
INK=rgb('#355464')
TEAL=rgb('#2E8D96')
PALE=rgb('#A8DCE0')
MUTED=rgb('#6D8290')
WARM=rgb('#B75241')
PAPER=rgb('#FBFCFC')
WHITE=rgb('#FFFFFF')

def text(sh):
    try:return sh.TextFrame.TextRange.Text.strip()
    except:return ''

def shape_by_id(slide, sid):
    return next((sh for sh in slide.Shapes if sh.Id==sid),None)

def set_type(sh, size=None, color=None, bold=None, name='Aptos'):
    r=sh.TextFrame.TextRange
    r.Font.Name=name
    if size is not None:r.Font.Size=size
    if color is not None:r.Font.Color.RGB=color
    if bold is not None:r.Font.Bold=-1 if bold else 0

def add_text(slide,value,x,y,w,h,size,color,bold=False):
    sh=slide.Shapes.AddTextbox(1,x,y,w,h)
    tf=sh.TextFrame
    tf.MarginLeft=tf.MarginRight=tf.MarginTop=tf.MarginBottom=0
    tf.WordWrap=-1
    tf.TextRange.Text=value
    set_type(sh,size,color,bold)
    return sh

def background(slide,color):
    slide.Background.Fill.Solid()
    slide.Background.Fill.ForeColor.RGB=color
    for sh in slide.Shapes:
        if sh.Type==1 and sh.Left<=1 and sh.Top<=1 and sh.Width>=718 and sh.Height>=448:
            sh.Fill.ForeColor.RGB=color

def apply_header(slide,n):
    vals=[s for s in slide.Shapes if text(s)]
    title=next((s for s in vals if text(s).startswith(('I.','II.','III.'))),None)
    if not title:return
    title.Left=35;title.Top=14;title.Width=650;title.Height=34
    set_type(title,24,NAVY,True,'Aptos Display')
    title.TextFrame.MarginLeft=0
    title.TextFrame.MarginTop=0
    t=text(title)
    prefix=t.split()[0]
    try:title.TextFrame.TextRange.Characters(1,len(prefix)).Font.Color.RGB=TEAL
    except:pass
    subtitle=next((s for s in vals if s.Id!=title.Id and s.Top<82 and not text(s).isdigit()),None)
    if subtitle:
        subtitle.Left=36;subtitle.Top=56;subtitle.Width=650;subtitle.Height=36
        set_type(subtitle,16,INK,True)
        subtitle.TextFrame.MarginLeft=0
        subtitle.TextFrame.MarginTop=0
        # Mixed-color task labels keep their emphasis through the XML post-pass.
    return title,subtitle

app=win32com.client.DispatchEx('PowerPoint.Application')
app.Visible=1
pres=None
try:
    pres=app.Presentations.Open(str(SOURCE),WithWindow=False,ReadOnly=False)
    assert pres.Slides.Count==46
    before={n:[text(s) for s in pres.Slides(n).Shapes if text(s) and not text(s).isdigit()] for n in range(1,47)}

    # A calm blue cover based on the earlier design the user preferred.
    cover=pres.Slides(1)
    background(cover,BLUE)
    for s in list(cover.Shapes):
        if s.Type==13 or (s.Type==17 and text(s) not in before[1]):s.Delete()
    original=shape_by_id(cover,4)
    original.Left=43;original.Top=153;original.Width=304;original.Height=104
    set_type(original,17,WHITE,False)
    original.TextFrame.TextRange.ParagraphFormat.Alignment=1
    lab=next(s for s in cover.Shapes if text(s)=='Lab Experiment Course')
    lab.Left=43;lab.Top=34;lab.Width=290;lab.Height=25
    set_type(lab,14,PALE,True)
    lab.TextFrame.TextRange.ParagraphFormat.Alignment=1
    author=next(s for s in cover.Shapes if 'Yuguang Fu' in text(s))
    author.Left=43;author.Top=360;author.Width=320;author.Height=55
    set_type(author,13,WHITE,False)
    author.TextFrame.TextRange.ParagraphFormat.Alignment=1
    add_text(cover,'ArduinoNode',43,82,320,60,42,WHITE,True)
    img=cover.Shapes.AddPicture(str(PHOTO),False,True,385,124,296,222)
    img.ZOrder(0)

    for n in range(2,47):
        slide=pres.Slides(n)
        if n in (5,19,42):
            background(slide,BLUE)
            title=next((s for s in slide.Shapes if text(s) and not text(s).isdigit()),None)
            if title:
                title.Left=54;title.Top=156;title.Width=610;title.Height=72
                set_type(title,35,WHITE,True,'Aptos Display')
                title.TextFrame.TextRange.ParagraphFormat.Alignment=1
            if n==42:
                others=[s for s in slide.Shapes if text(s) and s.Id!=title.Id and not text(s).isdigit()]
                for s in others:
                    s.Left=56;s.Top=236;s.Width=592;s.Height=92
                    set_type(s,16,PALE,False)
        else:
            background(slide,PAPER)
            if n in list(range(6,19))+list(range(21,42))+[43,44]:
                apply_header(slide,n)
            elif n in (2,3,4):
                title=next(s for s in slide.Shapes if text(s) and not text(s).isdigit())
                title.Left=44;title.Top=29;title.Width=620;title.Height=48
                set_type(title,30,NAVY,True,'Aptos Display')
                if n in (3,4):
                    body=next(s for s in slide.Shapes if s.Id!=title.Id and text(s) and not text(s).isdigit())
                    body.Left=83;body.Top=139;body.Width=570;body.Height=220
                    set_type(body,22,INK,False)
                    body.TextFrame.TextRange.ParagraphFormat.SpaceAfter=21
                else:
                    qr=next((s for s in slide.Shapes if s.Type==13),None)
                    if qr:qr.Left=250;qr.Top=103;qr.Width=220;qr.Height=220
                    link=next((s for s in slide.Shapes if text(s).startswith('https://')),None)
                    if link:
                        link.Left=151;link.Top=342;link.Width=420;link.Height=62
                        set_type(link,13,TEAL,False)
            elif n==20:
                tip=next(s for s in slide.Shapes if text(s) and not text(s).isdigit())
                tip.Left=89;tip.Top=151;tip.Width=545;tip.Height=135
                set_type(tip,28,NAVY,False)
            elif n==45:
                title=next(s for s in slide.Shapes if text(s).startswith('Previous APESS'))
                title.Left=35;title.Top=20;title.Width=650;title.Height=36
                set_type(title,24,NAVY,True,'Aptos Display')
            elif n==46:
                for s in slide.Shapes:
                    if text(s)=='Lab Experiment Course':set_type(s,14,TEAL,True)
                    elif text(s).startswith('Thank you'):
                        set_type(s,29,NAVY,True,'Aptos Display')

        # Harmonize body typography while retaining the original red-warning hierarchy.
        for s in slide.Shapes:
            t=text(s)
            if not t or t.isdigit() or n in (1,5,19,42,46):continue
            if s.Type not in (17,14):continue
            if n in (2,3,4,20,45):continue
            try: c=s.TextFrame.TextRange.Font.Color.RGB
            except:continue
            if c==0 or c==-2147483648:
                s.TextFrame.TextRange.Font.Color.RGB=INK
            elif c in (255,192):
                s.TextFrame.TextRange.Font.Color.RGB=WARM

        for s in slide.Shapes:
            if text(s).isdigit() and s.Top>410:
                s.Left=649;s.Top=420;s.Width=37;s.Height=14
                set_type(s,9,PALE if n in (1,5,19,42) else MUTED,False)

    # Repair the source pages where notes intrude on headers or fall off-slide.
    for sid,newtop,newheight,size in [(16,83,36,14)]:
        sh=shape_by_id(pres.Slides(27),sid)
        if sh:sh.Top=newtop;sh.Height=newheight;set_type(sh,size,WARM)
    sh=shape_by_id(pres.Slides(27),9)
    if sh:sh.Top=120;sh.Height=286
    sh=shape_by_id(pres.Slides(28),8)
    if sh:sh.Left=356;sh.Top=85;sh.Width=330;sh.Height=38;set_type(sh,14,WARM)
    sh=shape_by_id(pres.Slides(28),13)
    if sh:sh.Top=122;sh.Height=280
    for n in (25,26,27,28):
        slide=pres.Slides(n)
        for sh in slide.Shapes:
            if text(sh) and sh.Top>=408 and not text(sh).isdigit():
                sh.Top=402;sh.Height=33
                set_type(sh,13,WARM)
    def shrink_table(slide_no,picture_id,ids,factor):
        sl=pres.Slides(slide_no);pic=shape_by_id(sl,picture_id)
        x0,y0=pic.Left,pic.Top
        for sid in [picture_id]+ids:
            sh=shape_by_id(sl,sid)
            if sh:
                sh.Left=x0+(sh.Left-x0)*factor
                sh.Top=y0+(sh.Top-y0)*factor
                sh.Width*=factor;sh.Height*=factor
    shrink_table(26,2,[],0.87)
    shape_by_id(pres.Slides(26),5).Top=399
    right27=shape_by_id(pres.Slides(27),9)
    right27.Height=261
    note27=shape_by_id(pres.Slides(27),5)
    note27.Left=377;note27.Top=386;note27.Width=310;note27.Height=34
    set_type(note27,12,WARM)
    right28=shape_by_id(pres.Slides(28),13)
    right28.Height=255
    cap=shape_by_id(pres.Slides(28),12)
    cap.Left=368;cap.Top=385;cap.Width=310;cap.Height=23
    set_type(cap,12,WARM)
    # Slide II.9: align the three process photos and keep the safety notes readable.
    s29=pres.Slides(29)
    for sid,x in [(12,21),(10,250),(15,479)]:
        sh=shape_by_id(s29,sid)
        sh.Left=x;sh.Top=122;sh.Width=217;sh.Height=227
    for sid in (17,18,2,6,9,11,13,22):
        sh=shape_by_id(s29,sid)
        if sh and sh.Top>=100:
            sh.Top=122+(sh.Top-110)*0.84
            sh.Height=max(12,sh.Height*0.84)
    for sid,x in [(12,21),(10,250),(15,479)]:
        sh=shape_by_id(s29,sid)
        sh.Left=x;sh.Top=122;sh.Width=217;sh.Height=227
    for sid in (19,20,21,7):
        sh=shape_by_id(s29,sid)
        if sh:set_type(sh,12,WARM,False)
    shape_by_id(s29,20).Left=28;shape_by_id(s29,20).Top=357
    shape_by_id(s29,19).Left=28;shape_by_id(s29,19).Top=379
    shape_by_id(s29,21).Left=483;shape_by_id(s29,21).Top=357
    check=shape_by_id(s29,7);check.Left=254;check.Top=377;check.Width=425;check.Height=35
    check.Line.Visible=0
    set_type(check,11,WARM)
    desc=shape_by_id(s29,16);desc.Left=35;desc.Top=94;desc.Width=650;desc.Height=24;set_type(desc,14,WARM)
    sub=shape_by_id(s29,4);sub.Height=33;set_type(sub,15,INK,True)

    # Keep the original wording of every slide. Added cover wordmark is the only new text.
    after={n:[text(s) for s in pres.Slides(n).Shapes if text(s) and not text(s).isdigit()] for n in range(1,47)}
    missing=[]
    for n in range(1,47):
        for t in before[n]:
            if t not in after[n]:missing.append((n,t[:90]))
    if missing:raise RuntimeError(f'Original text lost: {missing[:5]}')
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    pres.SaveAs(str(OUTPUT),24)
    print(json.dumps({'output':str(OUTPUT),'slides':pres.Slides.Count,'lost_original_text':missing},ensure_ascii=False))
finally:
    if pres:pres.Close()
    app.Quit()
