from PIL import Image, ImageDraw
import numpy as np, sys
S='/private/tmp/claude-501/-Users-scottdube-code/1a1d0c10-8a25-4a4e-b325-e0e856045b0c/scratchpad/'
OUT=S+'recrop/'
# name -> (frame, box)
boxes={
 'c-dim-before':('dim-before',(20,160,1900,700)),
 'hero-4k-band':('hero-4k-clock-b',(280,470,3560,1300)),
 'c-clock-standard':('clock-standard',(0,130,1920,680)),
 'c-clock-nixie':('clock-nixie',(0,10,1920,560)),
 'c-stocks-open':('stocks-open',(np.int64(334), 185, np.int64(1557), 614)),
 'c-wide-mqtt':('wide-mqtt',(300,150,1470,560)),
 'c-wide-webdata':('wide-webdata',(300,150,1470,560)),
 'boot-close-hand':('boot-close-hand',(0,0,1920,1080)),
}
tiles=[]
for name,(frame,box) in boxes.items():
    im=Image.open(S+'grabs/'+frame+'.jpg').crop(box)
    im.save(OUT+name+'.jpg',quality=92)
    t=im.copy(); t.thumbnail((700,400)); tiles.append((name+' '+str(box)+' '+str(im.size),t))
H=sum(t.height+16 for _,t in tiles); sheet=Image.new('RGB',(700,H),'black'); d=ImageDraw.Draw(sheet); y=0
for f,t in tiles:
    sheet.paste(t,(0,y)); d.text((4,y+t.height+2),f,fill='white'); y+=t.height+16
sheet.save(OUT+'_sheet.jpg',quality=80); print(sheet.size)
