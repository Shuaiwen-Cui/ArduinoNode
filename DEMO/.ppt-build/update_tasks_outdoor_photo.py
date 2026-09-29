from pathlib import Path
import json
import win32com.client
R=Path(r'E:/PROJ/ArduinoNode/DEMO')
B=R/'.ppt-build'
OUT=B/'output/ArduinoNode_NTU_teaching_v8.pptx'
def rgb(h):return int(h[:2],16)+256*int(h[2:4],16)+65536*int(h[4:],16)
NAVY=rgb('244B63');INK=rgb('355464');TEAL=rgb('2E8D96');MUTED=rgb('6D8290');WHITE=rgb('FFFFFF');PALE=rgb('A8DCE0')
def txt(s):
 try:return s.TextFrame.TextRange.Text.strip()
 except:return ''
def by(sl,i):return next(s for s in sl.Shapes if s.Id==i)
def put(sl,t,x,y,w,h,size,color=INK,bold=False):
 s=sl.Shapes.AddTextbox(1,x,y,w,h);f=s.TextFrame;f.AutoSize=0;f.WordWrap=-1
 f.MarginLeft=f.MarginRight=f.MarginTop=f.MarginBottom=0
 r=f.TextRange;r.Text=t;r.Font.Name='Aptos';r.Font.Size=size;r.Font.Color.RGB=color;r.Font.Bold=-1 if bold else 0
 r.ParagraphFormat.SpaceWithin=1.05;r.ParagraphFormat.SpaceAfter=0
 return s
def fit(s,x,y,w,h):
 k=min(w/s.Width,h/s.Height);sw=s.Width*k;sh=s.Height*k
 s.LockAspectRatio=0;s.Width=sw;s.Height=sh;s.Left=x+(w-sw)/2;s.Top=y+(h-sh)/2
def walk(s):
 if s.Type==6:
  for c in s.GroupItems:yield from walk(c)
 else:yield s
def task(sl,n,copy):
 s=put(sl,f'TASK {n}   {copy}',36,384,648,41,21,WHITE,True)
 s.Fill.Solid();s.Fill.ForeColor.RGB=NAVY;s.Line.Visible=0
 f=s.TextFrame;f.MarginLeft=16;f.MarginRight=12;f.VerticalAnchor=3
 f.TextRange.Characters(1,6).Font.Color.RGB=PALE
 s.Name=f'Hands-on Task {n}'
 return s
a=win32com.client.DispatchEx('PowerPoint.Application');a.Visible=1
d=a.Presentations.Open(str(R/'ArduinoNode.pptx'),WithWindow=False)
try:
 assert d.Slides.Count==48
 sl=d.Slides(13)
 by(sl,5).Delete()
 for i in [16,20]:by(sl,i).Top=110
 fit(by(sl,8),52,143,264,228)
 fit(by(sl,22),382,143,264,228)
 task(sl,1,'Configure the node identity')
 sl=d.Slides(18)
 group=by(sl,9)
 # Ungroup once to remove the former arrow/label without touching the code image.
 items=group.Ungroup()
 for s in list(items):
  if s.Type==13:
   fit(s,95,145,530,225)
  else:s.Delete()
 task(sl,2,'Complete sensing_sample_once()')
 sl=d.Slides(45)
 by(sl,8).TextFrame.TextRange.Text='Example photo: APESS2025, Hong Kong Polytechnic University'
 by(sl,5).TextFrame.TextRange.Text=('1  Deploy wireless sensors at selected points on the target structure and record vibration acceleration.\r\r'
                                    '2  Use the measured responses to identify modal parameters.')
 by(sl,30).TextFrame.TextRange.Text=('Use ambient vibration or an excitation method selected by the instructor for the target structure.\r'
                                     'Collect 3 records for each test condition.')
 for s in walk(by(sl,2)):
  if txt(s)=='Footbridge':
   s.TextFrame.TextRange.Text='Target structure'
   s.TextFrame.TextRange.Font.Size=12
   s.TextFrame.WordWrap=0
 sl=d.Slides(46)
 by(sl,4).TextFrame.TextRange.Text='Sensor placement example from APESS2025, Hong Kong Polytechnic University'
 by(sl,7).TextFrame.TextRange.Text=('Select points on the target structure to compare responses at different locations. '
                                    'In this example, eight student groups each operated one node.')
 by(sl,8).TextFrame.TextRange.Text='Example sensor positions'
 # A restrained photographic ending, before the retained Q&A slide.
 sl=d.Slides.Add(48,12)
 for s in list(sl.Shapes):s.Delete()
 sl.FollowMasterBackground=0;sl.Background.Fill.Solid();sl.Background.Fill.ForeColor.RGB=WHITE
 put(sl,'APESS2025 course participants',36,24,648,39,26,NAVY,True)
 put(sl,'Earlier course at Hong Kong Polytechnic University',36,73,648,28,16,MUTED)
 p=sl.Shapes.AddPicture(str(B/'assets/PLOTS-11-1.png'),False,True,36,113,-1,-1)
 c=p.PictureFormat.Crop
 c.ShapeWidth=648;c.ShapeHeight=269;c.ShapeLeft=36;c.ShapeTop=113
 c.PictureWidth=648;c.PictureHeight=648*1279/1706
 c.PictureOffsetX=0;c.PictureOffsetY=-24
 put(sl,'Participants with their ArduinoNode sensors after the field session',36,402,612,26,16,INK)
 sl.NotesPage.Shapes.Placeholders(2).TextFrame.TextRange.Text='Photo source: PLOTS.pptx, slide 11. Earlier APESS2025 course at Hong Kong Polytechnic University.'
 for n in [48,49]:
  sl=d.Slides(n)
  for s in list(sl.Shapes):
   if txt(s).isdigit() and s.Top>410:s.Delete()
  put(sl,str(n),652,428,32,13,9,MUTED)
 assert d.Slides.Count==49
 d.SaveAs(str(OUT),24)
 out=B/'render-v8';out.mkdir(exist_ok=True)
 for n in [13,18,45,46,48,49]:d.Slides(n).Export(str(out/f'slide-{n}.png'),'PNG',1280,800)
 alerts=[]
 for n in [13,18,45,46,48,49]:
  for s in d.Slides(n).Shapes:
   if txt(s) and s.Type!=6 and s.TextFrame.TextRange.BoundHeight>s.Height+3:alerts.append((n,txt(s)))
 print(json.dumps({'slides':49,'output':str(OUT),'fit_alerts':alerts}))
finally:d.Close();a.Quit()
