from pathlib import Path
import json, re
import win32com.client

ROOT=Path(r'E:/PROJ/ArduinoNode/DEMO')
BUILD=ROOT/'.ppt-build'
SOURCE=ROOT/'ArduinoNode_before_layout_rework_20260929_185050.pptx'
OUT=BUILD/'output/ArduinoNode_NTU_teaching_v7.pptx'
ASSETS=BUILD/'assets'
def rgb(h):
 h=h.lstrip('#');return int(h[:2],16)+256*int(h[2:4],16)+65536*int(h[4:],16)
NAVY=rgb('244B63');INK=rgb('355464');TEAL=rgb('2E8D96');MUTED=rgb('6D8290');WARM=rgb('B75241');WHITE=rgb('FFFFFF')
def txt(s):
 try:return s.TextFrame.TextRange.Text.strip()
 except:return ''
def by(sl,i):return next(s for s in sl.Shapes if s.Id==i)
def fmt(s,x,y,w,h,size=17,color=INK,bold=False,value=None):
 tf=s.TextFrame;tf.AutoSize=0;tf.WordWrap=-1
 tf.MarginLeft=tf.MarginRight=tf.MarginTop=tf.MarginBottom=0
 if value is not None:tf.TextRange.Text=value
 r=tf.TextRange;r.Font.Name='Aptos';r.Font.Size=size;r.Font.Color.RGB=color;r.Font.Bold=-1 if bold else 0
 r.ParagraphFormat.Alignment=1;r.ParagraphFormat.Bullet.Visible=0
 r.ParagraphFormat.SpaceBefore=0;r.ParagraphFormat.SpaceAfter=0
 r.ParagraphFormat.SpaceWithin=1.08
 s.Left=x;s.Top=y;s.Width=w;s.Height=h
 return s
def put(sl,t,x,y,w,h,size=17,color=INK,bold=False):
 return fmt(sl.Shapes.AddTextbox(1,x,y,w,h),x,y,w,h,size,color,bold,t)
def drop(sl,ids):
 for i in ids:by(sl,i).Delete()
def clear(sl):
 for s in list(sl.Shapes):s.Delete()
def fit(s,x,y,w,h):
 ratio=min(w/s.Width,h/s.Height);sw=s.Width*ratio;sh=s.Height*ratio
 s.LockAspectRatio=0;s.Width=sw;s.Height=sh;s.Left=x+(w-sw)/2;s.Top=y+(h-sh)/2
 return s
def bundle(sl,ids,x,y,w,h):
 shapes=[by(sl,i) for i in ids]
 if len(shapes)==1:return fit(shapes[0],x,y,w,h)
 g=sl.Shapes.Range([s.Name for s in shapes]).Group()
 return fit(g,x,y,w,h)
def header(sl,t):
 s=put(sl,t,36,24,648,39,26,NAVY,True)
 m=re.match(r'([IV]+\.\d+)',t)
 if m:s.TextFrame.TextRange.Characters(1,len(m.group())).Font.Color.RGB=TEAL
 return s
def pic(sl,name,x,y,w,h):
 s=sl.Shapes.AddPicture(str(ASSETS/name),False,True,x,y,-1,-1)
 return fit(s,x,y,w,h)
def body(sl,i,x,y,w,h,size=17,value=None,color=INK,bold=False):
 return fmt(by(sl,i),x,y,w,h,size,color,bold,value)
def heading(sl,i,x,y,w,t=None):return body(sl,i,x,y,w,30,18,t,NAVY,True)

app=win32com.client.DispatchEx('PowerPoint.Application');app.Visible=1
d=None
try:
 d=app.Presentations.Open(str(SOURCE),WithWindow=False,ReadOnly=False)
 assert d.Slides.Count==49
 S={i:d.Slides(i) for i in range(1,50)}
 before={i:[txt(s) for s in sl.Shapes if txt(s)] for i,sl in S.items()}
 # A visual lesson overview replaces the duplicate text-only agenda.
 sl=S[43];clear(sl)
 header(sl,'Course activities')
 put(sl,'Introduction, hands-on work and vibration measurements',36,70,648,30,17,MUTED)
 for x,name,title,desc in [
  (36,'PLOTS-8-1.jpeg','I  Understanding the node','Hardware, software and the sensing chain'),
  (258,'PLOTS-8-3.jpeg','II  Building and programming','Assembly, firmware and the first checks'),
  (480,'PLOTS-10-13.jpg','III  Testing and interpreting','Indoor vibration test, with an optional field extension')]:
  pic(sl,name,x,119,204,153)
  put(sl,title,x,290,204,54,19,NAVY,True)
  put(sl,desc,x,347,204,56,16,INK)
 put(sl,'Photos: APESS2025, Hong Kong Polytechnic University. NTU activities depend on the available apparatus and site.',36,420,610,25,10,MUTED)
 sl.MoveTo(4);S[4].Delete();S[48].Delete()
 # Main titles carry the topic directly; the old secondary line is redundant.
 titles={6:'I.1  The assembled wireless sensor',7:'I.2  Arduino Uno R4 WiFi',8:'I.3  Sensor shield',
 9:'I.4  Sensing, radio, indicators and storage',10:'I.5  Power supply and enclosure',11:'I.6  Development framework',
 12:'I.7  Software architecture',13:'I.8  Gateway and leaf-node configuration',14:'I.9  State machine and indicators',
 15:'I.10  Wireless communication',16:'I.11  Time and synchronization',17:'I.12  Commands and RGB LED feedback',18:'I.13  Acceleration sensing and storage',
 21:'II.1  Parts in the kit',22:'II.2  Battery holder installation',23:'II.3  Main controller installation',24:'II.4  Sensor shield installation',
 25:'II.5  MPU6050 installation',26:'II.6  Radio module installation',27:'II.7  RGB LED installation',28:'II.8  SD card module installation',
 29:'II.9  Enclosure assembly',30:'II.10  Connections before power-up',31:'II.11  VS Code installation',32:'II.12  PlatformIO installation',
 33:'II.13  Serial Monitor installation',34:'II.14  Source code download',35:'II.15  Opening the project',36:'II.16  Task 1: node configuration',
 37:'II.17  Task 2: sampling acceleration',38:'II.18  Task 2: the function to complete',39:'II.19  Task 2: reading and scaling',
 40:'II.20  Building and uploading firmware',41:'II.22  Task 2: suggested answer',
 44:'III.1  Indoor vibration test',45:'III.2  Outdoor vibration test',46:'III.3  Reading a sensor layout',47:'III.4  Reading vibration spectra'}
 for n,title in titles.items():
  sl=S[n]
  # Existing technical figures and annotations stay intact.
  h=next(s for s in sl.Shapes if re.match(r'^[IV]+\.\d+',txt(s)))
  h.Delete()
  if 6<=n<=41 and n not in [19,20]:
   sid=2 if n==6 else 4
   by(sl,sid).Delete()
  sl.FollowMasterBackground=0;sl.Background.Fill.Solid();sl.Background.Fill.ForeColor.RGB=WHITE
  header(sl,title)

 # Hardware overview pages use a consistent margin and photo/text columns.
 fit(by(S[6],49),36,104,648,286)
 sl=S[7]
 fit(by(sl,10),36,112,319,256)
 heading(sl,23,396,101,280,'Microcontroller')
 fit(by(sl,20),396,133,280,136)
 heading(sl,24,396,294,280,'Wi-Fi module')
 fit(by(sl,22),396,323,222,91)
 sl=S[8]
 body(sl,11,36,141,245,161,19,'The sensor shield connects sensors and other components to the Arduino board. Its interfaces make the wiring easier to follow.')
 fit(by(sl,9),318,91,366,318)
 sl=S[9]
 specs=[(10,6,20,36,106,'MPU6050','IMU with a 3-axis gyroscope and 3-axis accelerometer\rRanges: 2g / 4g / 8g / 16g\rResolution: 16 bit'),
        (15,25,18,374,106,'nRF24L01 radio','Low-power, low-cost 2.4 GHz transceiver\rShort-range wireless communication\r250 kbps / 1 Mbps / 2 Mbps'),
        (16,13,22,36,269,'RGB LED','Three independent red, green and blue LEDs\rMixing their light produces different colours'),
        (17,14,24,374,269,'SD card module','External storage for measured data\rCommunicates with the Arduino via SPI')]
 for hi,pi,bi,x,y,title,copy in specs:
  heading(sl,hi,x,y,300,title)
  fit(by(sl,pi),x,y+40,104,105)
  body(sl,bi,x+116,y+38,194,116,14,copy)
 sl=S[10]
 heading(sl,21,36,102,300,'Lithium batteries')
 fit(by(sl,5),38,143,85,136)
 body(sl,19,142,146,206,123,16,'Two rechargeable 18650 lithium batteries\rHigh energy density\rOutput: 3.7 V\rCapacity: 3400 mAh')
 heading(sl,25,36,300,300,'Battery holder')
 fit(by(sl,11),38,337,88,70)
 body(sl,26,142,336,206,74,16,'Holds two 18650 batteries and provides the connection interface')
 heading(sl,28,390,102,294,'Enclosure')
 fit(by(sl,23),390,143,294,108)
 fit(by(sl,27),425,262,224,149)

 # Three concepts, equal columns, with the original editable diagrams.
 sl=S[11]
 for x,hi,bi,title,copy in [
  (36,7,15,'Setup + Loop','Arduino follows an initialization and infinite-loop pattern, common to embedded systems.'),
  (260,2,8,'State machine','Separate the internal logic of each state from the transitions between states.'),
  (484,6,9,'Front and back end','This architecture supports event-driven programming in the embedded system.')]:
  heading(sl,hi,x,105,198,title);body(sl,bi,x,147,198,106,16,copy)
 bundle(sl,[10,12,16],65,277,139,128)
 bundle(sl,[20,22,23,26,32,36],264,277,191,128)
 bundle(sl,[39,40,42],493,277,179,128)
 sl=S[12]
 body(sl,2,36,70,648,26,15,'Modular organization and hierarchical dependencies',MUTED)
 ids=[s.Id for s in sl.Shapes if s.Id not in [2] and not txt(s).startswith('I.7') and not(txt(s).isdigit() and s.Top>410)]
 bundle(sl,ids,36,110,648,298)
 sl=S[13]
 body(sl,14,36,75,648,30,16,'Edit the configuration file to set the role and identity of each node.')
 heading(sl,16,52,123,294,'Gateway node');heading(sl,20,382,123,294,'Leaf node')
 fit(by(sl,8),52,161,264,229);fit(by(sl,22),382,161,264,229)
 body(sl,5,36,407,600,22,13,'Hands-on Task 1 uses this configuration file.',TEAL)
 drop(sl,[9,23])
 sl=S[14]
 body(sl,12,36,74,648,48,16,'A state machine defines states and transitions to manage complex logic without a real-time operating system.')
 fit(by(sl,7),36,134,311,271);fit(by(sl,11),375,173,309,177)
 body(sl,15,376,370,308,26,15,'State definitions in nodestate.hpp',MUTED)

 # Reflow explanatory paragraphs into readable, left-aligned blocks.
 sl=S[15]
 heading(sl,2,36,104,300,'Wi-Fi: Internet connection')
 body(sl,5,36,144,302,99,17,'The onboard Wi-Fi module connects the node to the Internet for NTP time synchronization and remote commands through MQTT.')
 heading(sl,7,36,266,302,'MQTT: remote commands')
 body(sl,8,36,306,302,104,16,'Message Queuing Telemetry Transport is a lightweight messaging protocol for low-bandwidth, high-latency or unreliable networks. It is widely used for IoT devices.')
 heading(sl,9,380,104,304,'RF: local communication')
 body(sl,11,380,144,304,231,17,'Radio frequency communication carries information on radio waves. A transmitter modulates the signal and a receiver demodulates it.\r\rApplications include radio, television, mobile phones and satellite links. This project uses an nRF24L01 wireless module.')
 sl=S[16];by(sl,2).Delete()
 for hi,bi,y,title,copy in [
  (5,6,103,'Calendar time','Human-readable year, month, day, hour, minute and second.'),
  (10,11,201,'Unix time','Seconds since 1 January 1970, 00:00:00 UTC. Standard in computer systems and easy to compute and compare, but less readable to people.'),
  (12,13,320,'Runtime','Elapsed time since startup, usually in milliseconds. A hardware timer tracks it for system status and performance monitoring.')]:
  heading(sl,hi,36,y,302,title);body(sl,bi,36,y+32,302,89,15,copy)
 heading(sl,15,382,103,302,'Global synchronization: NTP')
 body(sl,16,382,143,302,87,16,'Network Time Protocol aligns the device clock with an accurate time source on the Internet.')
 heading(sl,17,382,257,302,'Local synchronization: FTSP')
 body(sl,19,382,297,302,116,16,'The gateway periodically broadcasts its time to all leaf nodes. Each leaf uses the messages to estimate clock offset and drift, then aligns its local time.')
 sl=S[17];by(sl,2).Delete()
 put(sl,'MQTT commands',36,89,308,26,18,NAVY,True)
 body(sl,14,36,124,307,51,15,'MQTT callbacks let remote commands control the node over the Internet.')
 bundle(sl,[5,8],36,187,307,224)
 put(sl,'RGB LED feedback',382,89,302,26,18,NAVY,True)
 body(sl,6,382,124,302,51,15,'The LED colour indicates the current state of the node.')
 fit(by(sl,11),398,187,267,224)
 sl=S[18]
 body(sl,14,36,76,648,66,15,'The node samples and stores acceleration together. Without a real-time operating system, the sampling frequency can vary from its target. The highest sampling rate in this project is 250 Hz.')
 bundle(sl,[5,13],85,156,551,257)

 # Photo assembly steps: consistent left/right structure and preserved labels.
 sl=S[21]
 note=next(s for s in sl.Shapes if txt(s).startswith('Reference kit'))
 fmt(note,36,75,648,28,14,MUTED,value='Reference kit from APESS2025. Ask the instructor if a component is missing.')
 photos=[s for s in sl.Shapes if s.Type==13]
 for p in photos:fit(p,116,119,488,289)
 sl=S[22]
 body(sl,2,36,73,648,25,15,'Check the orientation before tightening the screws.',WARM)
 bundle(sl,[6,12,13],36,120,306,284)
 bundle(sl,[8,16,17],382,120,302,284)
 sl=S[23]
 bundle(sl,[6,9,2,7,11,12,13],36,125,302,246)
 bundle(sl,[8,10],382,125,302,246)
 body(sl,5,36,393,648,26,16,'Check the board orientation before tightening the M3 × 10 screws.',WARM)
 sl=S[24]
 fit(by(sl,2),270,105,414,296)
 body(sl,5,36,157,211,112,20,'Align the pins carefully before seating the shield.',WARM)
 sl=S[25]
 fit(by(sl,10),45,103,265,151)
 bundle(sl,[13,15,16,17,18],44,275,276,119)
 fit(by(sl,12),373,103,311,291)
 body(sl,5,36,414,610,22,13,'Check every connection. The INT pin can remain unconnected.',WARM)
 sl=S[26]
 fit(by(sl,11),50,104,264,149);fit(by(sl,2),50,273,264,129)
 bundle(sl,[8,7,10],381,104,303,294)
 body(sl,5,36,414,610,22,13,'Attach the RF module to the sensor shield as shown.',WARM)
 sl=S[27]
 fit(by(sl,12),48,105,266,148)
 bundle(sl,[6,2,7,8,10,11,13,15],45,272,278,130)
 fit(by(sl,9),394,144,277,247)
 body(sl,16,381,103,303,40,15,'Use the wires supplied with the RGB LED.')
 body(sl,5,381,401,303,27,13,'Connect to the pins for digital 7.',WARM)
 sl=S[28]
 bundle(sl,[16,1026],45,103,281,142)
 bundle(sl,[10,2,5,6,7,9,11,15],45,262,281,147)
 fit(by(sl,13),394,145,277,245)
 body(sl,8,381,103,303,42,15,'Check the pin connections and insert an SD card.')
 body(sl,12,381,401,303,25,13,'The table shows suggested wire colours.',MUTED)
 sl=S[29]
 body(sl,16,36,77,648,41,15,'Assemble the enclosure after programming. Connect the two mounting boards with standoffs and screws.')
 # The three detail photos and their callouts remain a single composition.
 bundle(sl,[10,12,15,17,18,2,6,9,11,13,22],36,132,648,219)
 body(sl,20,36,367,206,37,13,'The batteries do not sit fully inside.',INK)
 body(sl,19,36,402,205,19,13,'Wires: GND and 3.7 V',WARM)
 body(sl,21,480,367,204,37,13,'Power supply through two wires.',INK)
 body(sl,7,265,395,419,35,13,'Ask a helper to check all connections before powering the board.',WARM)
 sl=S[30]
 fit(by(sl,5),36,99,264,304)
 by(sl,7).Delete()
 for y,t,b in [(112,'Mechanical fixing','Glue is not required. Secure the components to suit the intended use.'),
               (268,'Before connecting the batteries','Ask a helper to inspect the connections before you power up the node.')]:
  put(sl,t,352,y,332,49,20,NAVY,True);put(sl,b,352,y+57,332,90,18,INK)

 # Software instructions retain their links, parameters, screenshots and tasks.
 for n,pi in [(31,9),(32,5),(33,6)]:
  sl=S[n];body(sl,2,36,83,648,46,17)
  fit(by(sl,pi),65,149,590,259)
 sl=S[34]
 heading(sl,8,36,153,308,'Project source files')
 body(sl,9,36,202,303,116,17)
 fit(by(sl,5),410,119,256,256)
 sl=S[35]
 body(sl,8,36,79,648,90,16,'1  Launch VS Code.\r2  Open the folder containing the extracted project.\r3  Open a code file to activate the PlatformIO environment.')
 bundle(sl,[13,17,19],36,183,306,230)
 bundle(sl,[16,20,21],378,183,306,230)
 sl=S[36]
 body(sl,2,36,79,648,113,16,'1  Open src/config.hpp.\r2  Set NODE_ID and MQTT_CLIENT_ID to your group number.\rFor example, group 1 uses NODE_ID 1 and MQTT_CLIENT_ID "LEAFNODE1". Disable the other definitions by commenting them out.')
 fit(by(sl,6),36,211,234,197);fit(by(sl,10),292,211,392,197)
 sl=S[37]
 by(sl,2).Delete()
 put(sl,'Function to complete',36,98,300,27,19,NAVY,True)
 put(sl,'Open src/sensing.cpp and locate sensing_sample_once. Complete the section marked in red.',36,142,312,111,19,INK)
 put(sl,'Required operations',384,98,300,27,19,NAVY,True)
 put(sl,'Read ax, ay and az using mpu6050.hpp / mpu6050.cpp.\r\rConvert the raw values to g and apply cali_scale_x, cali_scale_y and cali_scale_z from config.hpp / config.cpp.',384,142,300,202,17,INK)
 put(sl,'Configured range: 2g\rResolution: 16 bit',36,282,307,57,17,TEAL,True)
 fit(by(sl,9),78,351,224,63)
 sl=S[38];fit(by(sl,5),95,87,530,329)
 sl=S[39];by(sl,2).Delete()
 put(sl,'1  Read the sensor',36,100,302,30,20,NAVY,True)
 put(sl,'ax, ay and az already exist in sensing_sample_once. Look in mpu6050.hpp and mpu6050.cpp for the function that reads the raw MPU6050 data.',36,152,302,180,19,INK)
 put(sl,'2  Convert and calibrate',382,100,302,30,20,NAVY,True)
 put(sl,'The project uses the default 2g range and 16-bit raw values. Convert each axis to g, then apply its calibration scale factor.',382,152,302,129,19,INK)
 put(sl,'a_g = scale_factor × raw / 2¹⁴',382,302,302,41,18,TEAL,True)
 put(sl,'Scale factors are defined in config.hpp / config.cpp.',382,361,302,56,16,MUTED)
 # Split a crowded build/test page so the RGB colour key remains readable.
 sl=S[40];by(sl,13).Delete()
 put(sl,'1  Finish Tasks 1 and 2.\r2  Click Build and wait for it to succeed.\r3  Connect the node by USB, click Upload and wait for completion.',36,91,648,142,20,INK)
 bundle(sl,[10,12,20,22,23],49,262,622,153)
 test=d.Slides.Add(sl.SlideIndex+1,12);clear(test)
 test.FollowMasterBackground=0;test.Background.Fill.Solid();test.Background.Fill.ForeColor.RGB=WHITE
 header(test,'II.21  First boot and status checks')
 put(test,'Serial connection',36,102,300,30,20,NAVY,True)
 put(test,'Open Serial Monitor at 115200 baud.\r\rOnce the serial connection is established, press Reset to reboot and check the output.',36,149,290,195,20,INK)
 put(test,'RGB LED status',380,102,304,30,20,NAVY,True)
 for j,(label,meaning) in enumerate([('White, persistent','Booting error'),('Red','Error'),('Light blue','RF communication'),('Blue','Wi-Fi communication'),('Green','Idle / ready'),('Yellow','Preparing to sense'),('Purple','Sensing')]):
  y=151+j*37
  put(test,label,380,y,139,30,16,NAVY,True);put(test,meaning,527,y,162,30,16,INK)
 sl=S[41]
 fit(by(sl,5),36,87,648,163)
 bundle(sl,[7,9,11,16,17,18,19,26,27,28,29,30,31,32],73,274,574,145)

 # Original experiment instructions are clearly identified as prior examples.
 sl=S[44]
 put(sl,'APESS2025 example in ZS1107: cantilever beam',36,70,648,26,15,MUTED)
 fit(by(sl,16),36,111,196,299)
 by(sl,34).Delete()
 body(sl,5,276,119,408,176,19,'1  Mount the wireless sensor on the beam.\r2  Record acceleration at 100 Hz for 30 s.\r3  Plot the time history and PSD, then calibrate the scaling factor for each axis.')
 body(sl,29,276,325,408,91,16,'Excite the beam gently with a hammer at the end. Keep the response within the ArduinoNode 2g range.\rCollect 2–3 sets.',WARM)
 sl=S[45]
 put(sl,'Earlier APESS2025 footbridge experiment; optional NTU extension',36,70,648,26,15,MUTED)
 fit(by(sl,2),36,108,224,306)
 by(sl,34).Delete()
 body(sl,5,303,119,381,158,19,'1  Deploy the wireless sensors on the footbridge and record vibration acceleration.\r\r2  Use the signals to identify modal parameters.')
 body(sl,30,303,313,381,102,16,'Earlier test conditions: jumping near midspan and ambient excitation, with 3 sets for each.\rThe instructor will select suitable activities for the available NTU site.',MUTED)
 sl=S[46]
 body(sl,4,36,70,648,26,15,'APESS2025 footbridge example',MUTED)
 fit(by(sl,6),36,117,263,104)
 body(sl,7,340,116,344,108,17,'Eight student groups each operated one node. Measurements along the span let the class compare responses at different positions.')
 body(sl,8,66,245,586,25,15,'Bridge layout and monitoring points',NAVY,True)
 fit(by(sl,10),66,278,586,139)
 sl=S[47]
 body(sl,4,36,70,648,26,15,'Earlier free-vibration test: wireless and reference measurements',MUTED)
 fit(by(sl,6),36,110,352,297)
 heading(sl,7,423,113,261,'Reading exercise')
 body(sl,8,423,159,261,228,18,'Find peaks that recur across sensors.\r\rCompare the wireless and reference traces.\r\rConsider how noise, calibration or mounting could explain differences.')
 body(sl,9,36,421,610,17,10,'Earlier APESS2025 validation data. Today’s NTU test may use a different setup.',MUTED)

 # Harmonize opening pages and the page numbers after the reordering.
 for n,t in [(2,'Course materials'),(3,'Learning objectives')]:
  sl=S[n];h=next(s for s in sl.Shapes if s.Top<70 and txt(s));fmt(h,36,24,648,39,26,NAVY,True,t)
 sl=S[3]
 b=next(s for s in sl.Shapes if txt(s).startswith('01'))
 b.Delete()
 for i,(y,title,sub) in enumerate([(118,'Assemble and check','A wireless sensing node and its connections'),(221,'Configure and program','The node identity and acceleration sampling function'),(324,'Measure and interpret','A vibration record and its frequency spectrum')],1):
  put(sl,f'0{i}',36,y,54,42,28,TEAL,True)
  put(sl,title,111,y,570,33,23,NAVY,True)
  put(sl,sub,111,y+42,570,34,17,INK)
 for n,sl in enumerate(d.Slides,1):
  nums=[s for s in sl.Shapes if txt(s).isdigit() and s.Top>410]
  for s in nums:s.Delete()
  dark=sl.SlideID in [S[i].SlideID for i in [1,5,19,42]]
  put(sl,str(n),652,428,32,13,9,rgb('B7D6E3') if dark else MUTED)
 # Keep a private audit of the starting text for checking content preservation.
 (BUILD/'v7_source_text.json').write_text(json.dumps(before,ensure_ascii=False,indent=2),encoding='utf8')
 assert d.Slides.Count==48
 d.SaveAs(str(OUT),24)
 print(json.dumps({'output':str(OUT),'slides':d.Slides.Count}))
finally:
 if d:d.Close()
 app.Quit()
