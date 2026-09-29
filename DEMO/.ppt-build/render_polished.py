from pathlib import Path
import sys
import win32com.client
from PIL import Image,ImageOps,ImageDraw

root=Path(r'E:/PROJ/ArduinoNode/DEMO/.ppt-build')
inp=Path(sys.argv[1]) if len(sys.argv)>1 else root/'output/ArduinoNode_NTU_student_polished_v4.pptx'
out=Path(sys.argv[2]) if len(sys.argv)>2 else root/'render-polished'
out.mkdir(exist_ok=True)
a=win32com.client.DispatchEx('PowerPoint.Application')
d=a.Presentations.Open(str(inp),WithWindow=False,ReadOnly=True)
count=d.Slides.Count
try:
 for i,s in enumerate(d.Slides,1):s.Export(str(out/f'slide-{i}.png'),'PNG',1280,800)
finally:d.Close();a.Quit()
for start,end in [(i,min(i+15,count)) for i in range(1,count+1,16)]:
 contact=Image.new('RGB',(1440,((end-start)//4+1)*240),'#dde4e8')
 draw=ImageDraw.Draw(contact)
 for j,n in enumerate(range(start,end+1)):
  im=Image.open(out/f'slide-{n}.png').convert('RGB')
  im.thumbnail((354,221))
  x=(j%4)*360+3;y=(j//4)*240
  contact.paste(im,(x,y))
  draw.text((x+4,y+222),f'{n:02}',fill='#223b4a')
 contact.save(out/f'contact-{start}-{end}.png')
