from pathlib import Path
import json
import win32com.client
from PIL import Image,ImageDraw

root=Path(r'E:/PROJ/ArduinoNode/DEMO/.ppt-build')
inp=Path(r'E:/PROJ/ArduinoNode/DEMO/PLOTS.pptx')
out=root/'render-plots';out.mkdir(exist_ok=True)
a=win32com.client.DispatchEx('PowerPoint.Application')
d=a.Presentations.Open(str(inp),WithWindow=False,ReadOnly=True)
data=[]
try:
 for i,s in enumerate(d.Slides,1):
  s.Export(str(out/f'slide-{i}.png'),'PNG',1280,800)
  tx=[]
  for sh in s.Shapes:
   try:
    t=sh.TextFrame.TextRange.Text.strip()
    if t:tx.append(t)
   except:pass
  data.append({'slide':i,'text':tx})
finally:d.Close();a.Quit()
(root/'plots-text.json').write_text(json.dumps(data,indent=2,ensure_ascii=False),encoding='utf-8')
for start in range(1,len(data)+1,12):
 end=min(len(data),start+11)
 ct=Image.new('RGB',(1440,((end-start)//4+1)*240),'#dde4e8');draw=ImageDraw.Draw(ct)
 for j,n in enumerate(range(start,end+1)):
  im=Image.open(out/f'slide-{n}.png').convert('RGB');im.thumbnail((354,221))
  x=(j%4)*360+3;y=(j//4)*240;ct.paste(im,(x,y));draw.text((x+4,y+222),f'{n:02}',fill='#223b4a')
 ct.save(out/f'contact-{start}-{end}.png')
print(len(data))
