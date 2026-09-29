from __future__ import annotations

from pathlib import Path
import json
import win32com.client

ROOT=Path(r'E:/PROJ/ArduinoNode/DEMO')
SOURCE=ROOT/'ArduinoNode.pptx'
OUT=ROOT/'.ppt-build/output/ArduinoNode_NTU_teaching_v5.pptx'
ASSETS=ROOT/'.ppt-build/assets'

def rgb(s):
 s=s.lstrip('#');return int(s[:2],16)+int(s[2:4],16)*256+int(s[4:6],16)*65536
NAVY=rgb('#244B63');INK=rgb('#355464');TEAL=rgb('#2E8D96');WARM=rgb('#B75241')
MUTED=rgb('#6D8290');PAPER=rgb('#FBFCFC');WHITE=rgb('#FFFFFF');PALE=rgb('#A8DCE0')

def txt(sh):
 try:return sh.TextFrame.TextRange.Text.strip()
 except:return ''

def first(slide,prefix):return next(s for s in slide.Shapes if txt(s).startswith(prefix))
def typ(sh,size,color,bold=False):
 r=sh.TextFrame.TextRange;r.Font.Name='Aptos';r.Font.Size=size;r.Font.Color.RGB=color;r.Font.Bold=-1 if bold else 0
def put(slide,value,x,y,w,h,size,color,bold=False):
 sh=slide.Shapes.AddTextbox(1,x,y,w,h)
 tf=sh.TextFrame;tf.MarginLeft=tf.MarginRight=tf.MarginTop=tf.MarginBottom=0
 tf.WordWrap=-1;tf.TextRange.Text=value;typ(sh,size,color,bold)
 return sh
def set_body(slide,value,prefix):
 sh=first(slide,prefix);sh.TextFrame.TextRange.Text=value;return sh

app=win32com.client.DispatchEx('PowerPoint.Application');app.Visible=1
deck=None
try:
 deck=app.Presentations.Open(str(SOURCE),WithWindow=False,ReadOnly=False)
 assert deck.Slides.Count==46
 changes=[]
 # Objectives and the lesson path now describe the feasible NTU core while
 # retaining the source course's hardware/software/measurement progression.
 s=deck.Slides(3);old=txt(first(s,'🎯Hardware'))
 new=('01  Assemble and check an ArduinoNode wireless sensor\r'
      '02  Configure, compile and upload its firmware\r'
      '03  Capture and interpret a vibration spectrum')
 body=set_body(s,new,'🎯Hardware');body.Left=88;body.Top=126;body.Width=555;body.Height=225
 typ(body,21,INK)
 body.TextFrame.TextRange.ParagraphFormat.SpaceAfter=22
 changes.append((3,old,new))
 s=deck.Slides(4);old=txt(first(s,'🎯Introduction'))
 new=('I  Introduction: how the node senses and records\r'
      'II  Hands-on: assemble, program and check\r'
      'III  Vibration test and data reading\r'
      'Outdoor extension, if site and time permit')
 body=set_body(s,new,'🎯Introduction');body.Left=86;body.Top=116;body.Width=565;body.Height=270
 typ(body,20,INK);body.TextFrame.TextRange.ParagraphFormat.SpaceAfter=17
 changes.append((4,old,new))
 # Student-friendly transition: concrete checks before assembly.
 s=deck.Slides(20);tip=first(s,'TIP:')
 tip.Left=55;tip.Top=371;tip.Width=610;tip.Height=45;typ(tip,15,MUTED)
 put(s,'Hands-on checkpoints',55,43,610,45,29,NAVY,True)
 put(s,'01',55,133,55,35,22,TEAL,True)
 put(s,'Inspect the parts and power connections',121,133,535,35,20,INK)
 put(s,'02',55,200,55,35,22,TEAL,True)
 put(s,'Use a unique node ID when uploading firmware',121,200,535,35,20,INK)
 put(s,'03',55,267,55,35,22,TEAL,True)
 put(s,'Check the signal and save one usable record',121,267,535,35,20,INK)
 # The kit photo is historical; today's supply may differ.
 s=deck.Slides(21);note=first(s,'Contact the helper')
 old=txt(note);new='Reference kit (APESS2025). Ask the instructor if a component is missing.'
 note.TextFrame.TextRange.Text=new;note.Left=228;note.Top=55;note.Width=470;note.Height=32;typ(note,13,WARM)
 changes.append((21,old,new))
 # Rewrite the transition that had previously looked like a disclaimer.
 s=deck.Slides(42)
 title=first(s,'Section III');old=txt(title);new='Section III — Vibration and data'
 title.TextFrame.TextRange.Text=new;title.Left=53;title.Top=145;title.Width=610;title.Height=70;typ(title,34,WHITE,True)
 changes.append((42,old,new))
 note=next(sh for sh in s.Shapes if txt(sh).startswith('The following indoor'))
 old=txt(note);new=('The next examples come from the earlier APESS2025 course.\r'
                   'The instructor will adapt today’s test to the available NTU apparatus and site.')
 note.TextFrame.TextRange.Text=new;note.Left=56;note.Top=225;note.Width=608;note.Height=80;typ(note,17,PALE)
 changes.append((42,old,new))

 # A photographic teaching bridge, using three source photos at equal size.
 s=deck.Slides.Add(43,12)
 s.Background.Fill.Solid();s.Background.Fill.ForeColor.RGB=PAPER
 put(s,'III.1  A previous course in action',36,20,645,38,25,NAVY,True)
 put(s,'APESS2025 at Hong Kong Polytechnic University',36,62,640,25,15,MUTED)
 images=[('PLOTS-8-2.jpeg','Classroom'),('PLOTS-8-3.jpeg','Hands-on assembly'),('PLOTS-10-13.jpg','Field setting')]
 for i,(name,caption) in enumerate(images):
  x=36+i*224
  s.Shapes.AddPicture(str(ASSETS/name),False,True,x,116,203,152)
  put(s,caption,x,279,203,27,17,INK,True)
 put(s,'Build the node, then test whether its measurements remain interpretable as conditions become less controlled.',
     36,348,640,52,18,NAVY)
 put(s,'Earlier APESS2025 examples; the NTU session may use a different structure or remain indoors.',
     36,420,632,20,10,MUTED)

 # Original apparatus pages remain in place, but are identified as examples.
 indoor=deck.Slides(44);sh=first(indoor,'III.1');old=txt(sh)
 new='III.2  Indoor vibration test (ZS1107 example)'
 sh.TextFrame.TextRange.Text=new;typ(sh,24,NAVY,True);changes.append((44,old,new))
 outdoor=deck.Slides(45);sh=first(outdoor,'III.2');old=txt(sh)
 new='III.3  Outdoor vibration test (APESS2025 example)'
 sh.TextFrame.TextRange.Text=new;typ(sh,24,NAVY,True);changes.append((45,old,new))
 task=next((x for x in outdoor.Shapes if txt(x)=='Task:'),None)
 if task:
  task.Left=280;task.Top=83;task.Width=140;task.Height=32
  typ(task,17,NAVY,True)
 result=deck.Slides(46);sh=first(result,'Previous APESS2025');old=txt(sh)
 new='III.4  Reading vibration spectra'
 sh.TextFrame.TextRange.Text=new;typ(sh,25,NAVY,True);changes.append((46,old,new))
 sub=first(result,'A reference case');old=txt(sub)
 new='Earlier free-vibration test: compare wireless and reference traces'
 sub.TextFrame.TextRange.Text=new;typ(sub,15,MUTED);changes.append((46,old,new))

 # Student reflection carries prior reception data without overstating what
 # a short summer course proves.
 s=deck.Slides.Add(47,12)
 s.Background.Fill.Solid();s.Background.Fill.ForeColor.RGB=PAPER
 put(s,'III.5  What did the earlier cohort report?',36,20,650,40,25,NAVY,True)
 put(s,'APESS2025 post-course questionnaire · 56 respondents',36,63,642,25,15,MUTED)
 put(s,'93.3%',50,140,270,74,45,TEAL,True)
 put(s,'overall satisfaction',52,217,280,36,19,INK)
 put(s,'100%',390,140,270,74,45,TEAL,True)
 put(s,'would recommend the experience',392,217,286,55,19,INK)
 put(s,'For this lab, judge your own result: is the signal usable, and which peak can you explain?',
     49,320,625,58,19,NAVY)
 put(s,'These figures describe self-reported feedback from APESS2025; they are not a measured learning gain or NTU outcome.',
     49,408,622,29,11,MUTED)
 # Slide 48 is the preserved original closing page.
 for n in range(1,deck.Slides.Count+1):
  sl=deck.Slides(n)
  markers=[sh for sh in sl.Shapes if txt(sh).isdigit() and sh.Top>410]
  if markers:
   for sh in markers:sh.TextFrame.TextRange.Text=str(n)
  else:put(sl,str(n),650,420,36,16,9,MUTED)
 assert deck.Slides.Count==48
 OUT.parent.mkdir(parents=True,exist_ok=True)
 deck.SaveAs(str(OUT),24)
 (ROOT/'.ppt-build/content_changes_v5.json').write_text(json.dumps(changes,ensure_ascii=False,indent=2),encoding='utf-8')
 print(json.dumps({'slides':deck.Slides.Count,'output':str(OUT),'intentional_text_edits':len(changes)},ensure_ascii=False))
finally:
 if deck:deck.Close()
 app.Quit()
