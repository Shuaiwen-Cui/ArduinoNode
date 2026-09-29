from pathlib import Path
import win32com.client
R=Path(r'E:/PROJ/ArduinoNode/DEMO/.ppt-build')
P=R/'output/ArduinoNode_NTU_teaching_v7.pptx'
def rgb(h):return int(h[:2],16)+256*int(h[2:4],16)+65536*int(h[4:],16)
def txt(s):
 try:return s.TextFrame.TextRange.Text.strip()
 except:return ''
def put(sl,t,x,y,w,h,size,color,bold=False):
 s=sl.Shapes.AddTextbox(1,x,y,w,h);f=s.TextFrame;f.AutoSize=0;f.WordWrap=-1
 f.MarginLeft=f.MarginRight=f.MarginTop=f.MarginBottom=0
 f.TextRange.Text=t;f.TextRange.Font.Name='Aptos';f.TextRange.Font.Size=size;f.TextRange.Font.Color.RGB=color;f.TextRange.Font.Bold=-1 if bold else 0
 return s
a=win32com.client.DispatchEx('PowerPoint.Application')
d=a.Presentations.Open(str(P),WithWindow=False)
try:
 # Whole-slide import avoids PowerPoint clipboard timing and preserves diagram layers.
 d.Slides.InsertFromFile(r'E:\PROJ\ArduinoNode\DEMO\ArduinoNode_before_layout_rework_20260929_185050.pptx',11,12,12)
 d.Slides(13).Delete()
 sl=d.Slides(12)
 for s in list(sl.Shapes):
  if s.Id in [3,4,2,21]:s.Delete()
 h=put(sl,'I.7  Software architecture',36,24,648,39,26,rgb('244B63'),True)
 h.TextFrame.TextRange.Characters(1,3).Font.Color.RGB=rgb('2E8D96')
 put(sl,'Modular organization\rHierarchical dependencies',36,75,176,48,14,rgb('6D8290'))
 put(sl,'12',652,428,32,13,9,rgb('6D8290'))
 sl=d.Slides(18)
 h=next(s for s in sl.Shapes if txt(s).startswith('I.13'))
 h.TextFrame.TextRange.Font.Color.RGB=rgb('244B63')
 h.TextFrame.TextRange.Characters(1,4).Font.Color.RGB=rgb('2E8D96')
 d.Save()
 for n in [12,18]:d.Slides(n).Export(str(R/f'render-v7-final/slide-{n}.png'),'PNG',1280,800)
 print(d.Slides.Count)
finally:d.Close();a.Quit()
