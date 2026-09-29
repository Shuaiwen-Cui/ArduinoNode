from pathlib import Path
import json
import win32com.client
R=Path(r'E:/PROJ/ArduinoNode/DEMO/.ppt-build')
P=R/'output/ArduinoNode_NTU_teaching_v7.pptx'
def rgb(h):return int(h[:2],16)+256*int(h[2:4],16)+65536*int(h[4:],16)
NAVY=rgb('244B63');INK=rgb('355464');TEAL=rgb('2E8D96');MUTED=rgb('6D8290')
def txt(s):
 try:return s.TextFrame.TextRange.Text.strip()
 except:return ''
def put(sl,t,x,y,w,h,size,color=INK,bold=False):
 s=sl.Shapes.AddTextbox(1,x,y,w,h);f=s.TextFrame;f.AutoSize=0;f.WordWrap=-1
 f.MarginLeft=f.MarginRight=f.MarginTop=f.MarginBottom=0
 r=f.TextRange;r.Text=t;r.Font.Name='Aptos';r.Font.Size=size;r.Font.Color.RGB=color;r.Font.Bold=-1 if bold else 0
 r.ParagraphFormat.SpaceWithin=1.08;r.ParagraphFormat.SpaceAfter=0;r.ParagraphFormat.Alignment=1
 return s
def walk(s):
 if s.Type==6:
  for c in s.GroupItems:yield from walk(c)
 else:yield s
a=win32com.client.DispatchEx('PowerPoint.Application');a.Visible=1
d=a.Presentations.Open(str(P),WithWindow=False)
src=a.Presentations.Open(r'E:\PROJ\ArduinoNode\DEMO\ArduinoNode_before_layout_rework_20260929_185050.pptx',WithWindow=False,ReadOnly=True)
try:
 # Scaling a group can reflow labels in PowerPoint. Keep single-line labels intact.
 for sl in d.Slides:
  for group in sl.Shapes:
   if group.Type!=6:continue
   for s in walk(group):
    t=txt(s)
    if t and '\r' not in t and '\n' not in t:
     s.TextFrame.WordWrap=0
 # The dense architecture diagram needs its original full-size geometry.
 sl=d.Slides(12)
 for s in list(sl.Shapes):
  if not txt(s).startswith('I.7') and txt(s)!='12':s.Delete()
 for s in src.Slides(12).Shapes:
  if s.Id in [3,4,2,21]:continue
  s.Copy();sl.Shapes.Paste()
 put(sl,'Modular organization\rHierarchical dependencies',36,75,176,48,14,MUTED)
 # Recreate the two raster text lists as editable, consistently sized text.
 sl=d.Slides(7)
 for s in list(sl.Shapes):
  if s.Id in [20,22,23,24]:s.Delete()
 put(sl,'Microcontroller',390,91,294,26,18,NAVY,True)
 put(sl,'R7FA4M1AB3CFM#AA0',390,122,294,23,14,MUTED)
 put(sl,'256 kB flash / 32 kB SRAM\r8 kB data flash (EEPROM)\rReal-time clock and 4× DMAC\r14-bit ADC / up to 12-bit DAC\rOPAMP and CAN bus',390,151,294,119,15)
 put(sl,'Wi-Fi module',390,282,294,26,18,NAVY,True)
 put(sl,'ESP32-S3-MINI-1-N8',390,313,294,23,14,MUTED)
 put(sl,'Wi-Fi 4, 2.4 GHz / Bluetooth 5 LE\r3.3 V operating voltage\r384 kB ROM / 512 kB SRAM\rUp to 150 Mbps',390,342,294,84,15)
 # Preserve the named upload operation and the FTSP meaning from the original.
 sl=d.Slides(18)
 h=next(s for s in sl.Shapes if txt(s).startswith('I.13'))
 h.TextFrame.TextRange.Text='I.13  Acceleration sensing, storage and upload'
 h.TextFrame.TextRange.Characters(1,4).Font.Color.RGB=TEAL
 sl=d.Slides(16)
 h=next(s for s in sl.Shapes if txt(s).startswith('The gateway periodically'))
 h.TextFrame.TextRange.Text='The flooding-based protocol periodically broadcasts the gateway time to all leaf nodes. Each leaf estimates clock offset and drift, then aligns its local time.'
 # Fix a long URL into a useful editable link while retaining the exact address.
 sl=d.Slides(34)
 h=next(s for s in sl.Shapes if txt(s).startswith('https://drive'))
 url=txt(h);h.TextFrame.TextRange.Text='Download the ArduinoNode project'
 h.TextFrame.TextRange.ActionSettings(1).Hyperlink.Address=url
 h.TextFrame.TextRange.Font.Size=18;h.Height=66
 put(sl,'Scan the QR code or open the link, then extract the project folder.',36,282,303,80,17)
 # Retain a copy of the precise URL in the notes as well.
 try:sl.NotesPage.Shapes.Placeholders(2).TextFrame.TextRange.Text='Project download: '+url
 except:pass
 d.Save()
 # Geometry check of top-level text after the rendered fixes.
 alerts=[]
 for i,sl in enumerate(d.Slides,1):
  for s in sl.Shapes:
   if not txt(s):continue
   r=s.TextFrame.TextRange
   if r.BoundHeight>s.Height+3 or r.BoundTop+r.BoundHeight>449:
    alerts.append({'slide':i,'text':txt(s)[:80],'box_h':s.Height,'text_h':r.BoundHeight})
 (R/'v7_text_fit_checks.json').write_text(json.dumps(alerts,indent=2),encoding='utf8')
 print(json.dumps({'fit_alerts':alerts,'slides':d.Slides.Count}))
finally:src.Close();d.Close();a.Quit()
