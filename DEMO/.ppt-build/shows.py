from zipfile import ZipFile, ZIP_DEFLATED
from lxml import etree as E
from pathlib import Path
p=Path(r'E:/PROJ/ArduinoNode/DEMO/.ppt-build/candidate.pptx')
with ZipFile(p) as z:
    data={n:z.read(n) for n in z.namelist()}
ns='http://schemas.openxmlformats.org/presentationml/2006/main'
rn='http://schemas.openxmlformats.org/officeDocument/2006/relationships'
r=E.fromstring(data['ppt/presentation.xml'])
ids=[s.get('{'+rn+'}id') for s in r.find('{'+ns+'}sldIdLst')]
old=r.find('{'+ns+'}custShowLst')
if old is not None:r.remove(old)
lst=E.Element('{'+ns+'}custShowLst')
shows={'Leadership overview':[1,2,3,5,8,11,14,17,38,41,42,44,45], '3-hour student laboratory':list(range(1,42))+[43,44,45], 'Complete student deck':list(range(1,46))}
for j,(name,nums) in enumerate(shows.items()):
    show=E.SubElement(lst,'{'+ns+'}custShow',name=name,id=str(j))
    sl=E.SubElement(show,'{'+ns+'}sldLst')
    for n in nums:E.SubElement(sl,'{'+ns+'}sld',{'{'+rn+'}id':ids[n-1]})
anchor=r.find('{'+ns+'}defaultTextStyle')
r.insert(r.index(anchor) if anchor is not None else len(r),lst)
data['ppt/presentation.xml']=E.tostring(r,xml_declaration=True,encoding='UTF-8',standalone=True)
with ZipFile(p,'w',ZIP_DEFLATED) as z:
    for n,b in data.items():z.writestr(n,b)
print('Three custom slide shows added')
