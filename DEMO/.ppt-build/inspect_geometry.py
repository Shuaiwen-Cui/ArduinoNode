import json
from pathlib import Path
import win32com.client

p=Path(r'E:/PROJ/ArduinoNode/DEMO/ArduinoNode.pptx')
app=win32com.client.DispatchEx('PowerPoint.Application')
pres=app.Presentations.Open(str(p), WithWindow=False, ReadOnly=True)
out=[]
try:
 for si,slide in enumerate(pres.Slides,1):
  shapes=[]
  for sh in slide.Shapes:
   try: text=sh.TextFrame.TextRange.Text.strip()
   except: text=''
   shapes.append({'id':sh.Id,'name':sh.Name,'type':sh.Type,'x':round(sh.Left,1),'y':round(sh.Top,1),'w':round(sh.Width,1),'h':round(sh.Height,1),'text':text[:180]})
  out.append({'slide':si,'shapes':shapes})
finally:
 pres.Close();app.Quit()
Path(r'E:/PROJ/ArduinoNode/DEMO/.ppt-build/geometry.json').write_text(json.dumps(out,indent=2,ensure_ascii=False),encoding='utf-8')
