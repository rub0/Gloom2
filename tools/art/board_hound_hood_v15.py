'''Compose evidence from unretouched renders and decode the full diagnostic video.'''
import json
from pathlib import Path
import av
from PIL import Image,ImageDraw,ImageFont,ImageOps

root = Path(__file__).resolve().parents[2]
output = root/'docs/art/hound/mesh-v15'
cache = root/'.cache/hound-hood-v15'
report = root/'reports/hound-hood-98'
report.mkdir(parents=True,exist_ok=True)
font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf',22)
small = ImageFont.truetype('C:/Windows/Fonts/arial.ttf',17)
concept = Image.open(root/'docs/art/characters/hound-original-concept.jpg').convert('RGB')
w,h = concept.size
concept = concept.crop((int(w*.34),int(h*.09),int(w*.60),int(h*.34)))
comparison = Image.open(output/'hood-before-after.png').convert('RGB')
panels = (concept,comparison.crop((0,0,800,600)),comparison.crop((0,710,800,1310)))
board = Image.new('RGB',(1800,690),(32,32,35))
draw = ImageDraw.Draw(board)
for i,(panel,label) in enumerate(zip(panels,('CONCEPT / PINTURA RECORTADA','V14 / RENDER ANTERIOR','V15 / CAPUCHA ABIERTA'))):
    panel = ImageOps.contain(panel,(580,570),Image.Resampling.LANCZOS)
    board.paste(panel,(i*600+(600-panel.width)//2,55+(570-panel.height)//2))
    draw.text((i*600+20,20),label,font=font,fill='white')
draw.text((20,650),'Referencia pintada y renders reales. V14/V15: misma camara y escala; piezas del entorno aisladas para ver la caida.',font=small,fill='white')
board.save(output/'concept-hood-comparison.png')
Image.open(cache/'gloom-preview.ppm').save(output/'gloom-preview.png')
selected = {0:'Reposo',15:'Cabeza izquierda',45:'Cabeza derecha',75:'Mirar arriba',
            105:'Mirar abajo',135:'Alcance',165:'Giro izquierdo',195:'Giro derecho'}
board = Image.new('RGB',(1440,880),(32,32,35))
draw = ImageDraw.Draw(board)
video = av.open(str(output/'hood-check.mp4'))
stream = video.streams.video[0]
count = 0
for i,frame in enumerate(video.decode(video=0)):
    assert frame.width==900 and frame.height==1000
    count += 1
    if i in selected:
        j = list(selected).index(i)
        panel = frame.to_image().resize((360,400),Image.Resampling.LANCZOS)
        board.paste(panel,((j%4)*360,(j//4)*440+40))
        draw.text(((j%4)*360+12,(j//4)*440+12),selected[i],font=font,fill='white')
assert count==211 and stream.average_rate==30
metadata = {'frames':count,'fps':float(stream.average_rate),'duration_s':float(stream.duration*stream.time_base),
            'width':900,'height':1000,'codec':stream.codec_context.name,'selected_frames_0based':list(selected)}
video.close()
board.save(output/'video-keyframes.png')
(cache/'video-metadata.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
verification = json.loads((cache/'verification.json').read_text())
rows = verification['contacts']
lines = ['# Contactos de la capucha v14 / v15','','33 poses, capucha contra las otras 105 piezas: 3.465 pares por version.',
         '6.930 comparaciones entre ambas. BVH de superficies; no mide profundidad, contencion ni holgura continua.',
         'Recuentos de parejas de caras, dependientes de la teselacion. Cero autointersecciones de capucha en v15.',
         'No aparecen contactos con una pieza que estuviera libre en v14 en la misma pose.',
         'Las inserciones heredadas del forro en cuello, torso y peto permanecen; requieren ajuste de produccion.',
         '','','| Pieza | Reposo v14 | Reposo v15 | Maximo v14 | Maximo v15 |',
         '| --- | ---: | ---: | ---: | ---: |']
for name in rows['v14']['rest']:
    old,new = max(row[name] for row in rows['v14'].values()),max(row[name] for row in rows['v15'].values())
    if old or new:
        lines.append(f"| {name} | {rows['v14']['rest'][name]} | {rows['v15']['rest'][name]} | {old} | {new} |")
lines += ['','## Detalle por pose','','Solo se listan piezas con algun cruce en una de las dos versiones.','',
          '| Pose | Pieza | v14 | v15 |','| --- | --- | ---: | ---: |']
for pose,row in rows['v14'].items():
    for name,old in row.items():
        new = rows['v15'][pose][name]
        if old or new:
            lines.append(f'| {pose} | {name} | {old} | {new} |')
(report/'contacts.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps(metadata))
print('HOOD_MAXIMA',json.dumps({v:{n:max(r[n] for r in poses.values()) for n in poses['rest'] if any(r[n] for r in poses.values())} for v,poses in rows.items()}))
print('BOARDS_AND_CONTACTS_OK')
