from __future__ import annotations

from pathlib import Path
import json
import win32com.client

ROOT=Path(r'E:/PROJ/ArduinoNode/DEMO')
SOURCE=ROOT/'ArduinoNode.pptx'
OUT=ROOT/'.ppt-build/output/ArduinoNode_NTU_teaching_v6.pptx'
ASSETS=ROOT/'.ppt-build/assets'
ATTACHED=Path(r'C:/Users/cswof/AppData/Local/Temp/codex-clipboard-1debe3fe-6889-4fea-bc56-e405056906ce.png')
assert ATTACHED.exists()

def rgb(s):
 s=s.lstrip('#');return int(s[:2],16)+int(s[2:4],16)*256+int(s[4:6],16)*65536
NAVY=rgb('#244B63');INK=rgb('#355464');TEAL=rgb('#2E8D96')
MUTED=rgb('#6D8290');PAPER=rgb('#FBFCFC')

def txt(s):
 try:return s.TextFrame.TextRange.Text.strip()
 except:return ''
def typ(s,size,color,bold=False):
 r=s.TextFrame.TextRange;r.Font.Name='Aptos';r.Font.Size=size;r.Font.Color.RGB=color;r.Font.Bold=-1 if bold else 0
def put(slide,value,x,y,w,h,size,color,bold=False):
 s=slide.Shapes.AddTextbox(1,x,y,w,h)
 tf=s.TextFrame;tf.MarginLeft=tf.MarginRight=tf.MarginTop=tf.MarginBottom=0
 tf.WordWrap=-1;tf.TextRange.Text=value;typ(s,size,color,bold)
 return s
def alter(slide,start,new):
 s=next(sh for sh in slide.Shapes if txt(sh).startswith(start))
 old=txt(s);s.TextFrame.TextRange.Text=new;return old,new

app=win32com.client.DispatchEx('PowerPoint.Application');app.Visible=1
d=None
try:
 d=app.Presentations.Open(str(SOURCE),WithWindow=False,ReadOnly=False)
 assert d.Slides.Count==48
 # Align the two older chapter openers with the newer, clearer Section III.
 for n,lead in [(5,'The hardware and software behind one wireless node.'),
                (19,'Assemble, program and verify the node before testing.')]:
  chapter=d.Slides(n)
  head=next(sh for sh in chapter.Shapes if txt(sh).startswith('Section'))
  head.Left=54;head.Top=151;head.Width=612;head.Height=71
  typ(head,35,rgb('#FFFFFF'),True)
  head.TextFrame.TextRange.ParagraphFormat.Alignment=1
  put(chapter,lead,57,229,593,61,18,rgb('#A8DCE0'))
 # Use the original, higher-resolution copy of the exact lecture photograph
 # supplied by the user. It comes from the same PLOTS figure as the clipboard crop.
 s=d.Slides(43)
 for sh in list(s.Shapes):
  if not (txt(sh).isdigit() and sh.Top>410):sh.Delete()
 s.Background.Fill.Solid();s.Background.Fill.ForeColor.RGB=PAPER
 put(s,'III.1  A previous course in action',36,20,650,40,25,NAVY,True)
 put(s,'APESS2025 at Hong Kong Polytechnic University',36,62,640,26,15,MUTED)
 s.Shapes.AddPicture(str(ASSETS/'PLOTS-8-1.jpeg'),False,True,36,111,306,229)
 put(s,'Teaching the sensing chain',36,351,306,31,17,INK,True)
 s.Shapes.AddPicture(str(ASSETS/'PLOTS-8-3.jpeg'),False,True,372,111,145,109)
 put(s,'Assembly',372,228,145,28,16,INK,True)
 s.Shapes.AddPicture(str(ASSETS/'PLOTS-10-13.jpg'),False,True,536,111,145,109)
 put(s,'Field deployment',536,228,145,35,16,INK,True)
 put(s,'The node built in class becomes a measurement tool on a real structure.',
     372,292,309,79,18,NAVY)
 put(s,'Earlier APESS2025 example; today’s NTU activities depend on available apparatus and site.',
     36,421,640,19,10,MUTED)

 # Before interpreting spectra, show how the earlier class placed sensors.
 s=d.Slides.Add(46,12)
 s.Background.Fill.Solid();s.Background.Fill.ForeColor.RGB=PAPER
 put(s,'III.4  Reading a sensor layout',36,20,648,40,25,NAVY,True)
 put(s,'APESS2025 footbridge example',36,62,638,25,15,MUTED)
 s.Shapes.AddPicture(str(ASSETS/'PLOTS-9-1.png'),False,True,56,101,252,100)
 put(s,'Eight student groups each operated one node. Placing them along the span lets the class compare responses at different positions.',
     336,106,331,100,17,INK)
 put(s,'Bridge and monitoring points',75,214,560,24,14,INK,True)
 s.Shapes.AddPicture(str(ASSETS/'PLOTS-9-2.png'),False,True,75,239,560,208)

 # Keep section numbering sequential after the new explanatory page.
 alter(d.Slides(47),'III.4  Reading','III.5  Reading vibration spectra')
 alter(d.Slides(48),'III.5  What','III.6  What did the earlier cohort report?')
 for n in range(1,d.Slides.Count+1):
  slide=d.Slides(n)
  markers=[sh for sh in slide.Shapes if txt(sh).isdigit() and sh.Top>410]
  if markers:
   for sh in markers:sh.TextFrame.TextRange.Text=str(n)
  else:put(slide,str(n),650,420,36,16,9,MUTED)
 assert d.Slides.Count==49
 OUT.parent.mkdir(parents=True,exist_ok=True);d.SaveAs(str(OUT),24)
 print(json.dumps({'slides':d.Slides.Count,'output':str(OUT),'lecture_photo':'PLOTS-8-1.jpeg (higher-resolution source of attached crop)'},ensure_ascii=False))
finally:
 if d:d.Close()
 app.Quit()
