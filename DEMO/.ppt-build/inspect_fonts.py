import win32com.client
p=r'E:\PROJ\ArduinoNode\DEMO\ArduinoNode.pptx'
a=win32com.client.DispatchEx('PowerPoint.Application'); d=a.Presentations.Open(p,WithWindow=False,ReadOnly=True)
try:
 for n in [3,7,9,20,21,22,24,25,28,29,30,36,39]:
  print('\n',n)
  for sh in d.Slides(n).Shapes:
   try:
    t=sh.TextFrame.TextRange.Text.strip()
    if t:
     r=sh.TextFrame.TextRange
     print(sh.Id,repr(t[:42]), 'size',r.Font.Size,'color',r.Font.Color.RGB,'bold',r.Font.Bold)
   except Exception as e:print('err',sh.Id,str(e)[:80])
finally:d.Close();a.Quit()
