"""Original, antialiased browser symbols on a shared 36px grid."""
from pathlib import Path
from PIL import Image,ImageDraw
root=Path(__file__).parents[1];scale=6
output=['// Generated original browser icon masks.','\n#pragma once']
for name in ['back','forward','reload','home','search','close']:
    image=Image.new('L',(36*scale,36*scale));d=ImageDraw.Draw(image)
    def line(points,width=2.3):d.line([(round(x*scale),round(y*scale)) for x,y in points],fill=255,width=round(width*scale),joint='curve')
    if name in ['back','forward']:
        flip=lambda x:36-x if name=='forward' else x
        line([(flip(9),18),(flip(27),18)]);line([(flip(17),10),(flip(9),18),(flip(17),26)])
    elif name=='reload':
        d.arc(tuple(round(v*scale) for v in (8,8,28,28)),35,308,fill=255,width=round(2.3*scale));d.polygon([(round(x*scale),round(y*scale)) for x,y in [(27,6),(28,15),(20,11)]],fill=255)
    elif name=='search':
        d.ellipse(tuple(round(v*scale) for v in (7,7,24,24)),outline=255,width=round(2.3*scale));line([(22,22),(30,30)],2.6)
    elif name=='home':
        line([(6,17),(18,7),(30,17)]);line([(10,16),(10,29),(26,29),(26,16)]);line([(16,29),(16,22),(21,22),(21,29)])
    else:line([(10,10),(26,26)]);line([(26,10),(10,26)])
    image=image.resize((36,36),Image.Resampling.LANCZOS)
    output.append('static const unsigned char shell_'+name+'_pixels[]={'+','.join(str(v) for v in image.getdata())+'};')
    output.append('static const xui_mask_t shell_'+name+'={shell_'+name+'_pixels,36,36};')
(root/'src/shell_icons.h').write_text('\n'.join(output))
print('Browser icon masks generated.')
