'''Compose real H05 evidence and compare the measured v15/v16 contacts.'''
import json
from pathlib import Path
import av
from PIL import Image,ImageDraw,ImageFont,ImageOps
root = Path(__file__).resolve().parents[2]
output,cache = root/'docs/art/hound/mesh-v16',root/'.cache/hound-armor-v16'
report = root/'reports/hound-armor-99'
report.mkdir(parents=True,exist_ok=True)
font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf',22)
old = json.loads((root/'.cache/hound-hood-v15/audit.json').read_text())
new = json.loads((cache/'audit.json').read_text())
verification = json.loads((cache/'verification.json').read_text())
modified = set(verification['rebuilt_parts'])
assert new['self_crossings']==old['self_crossings']
assert new['weapon_crossings']==old['weapon_crossings']
for pose,row in new['surface_crossings'].items():
    for pair,count in row.items():
        names = pair.split('/')
        if not modified.intersection(names):
            assert count==old['surface_crossings'][pose].get(pair), (pose,pair)
        if modified.intersection(names) and any(n.startswith(('Arm_surface','Head_surface','Hood_','Hand_','Neck_')) for n in names):
            assert count==0,(pose,pair)
assert all(not row.get('Back_spine_1/Rib_lamella_2_'+side,0) for row in new['surface_crossings'].values() for side in ('L','R'))
lines = ['# Contactos y cobertura de armadura v16','','33 poses globales × 5.565 pares = 183.645 evaluaciones por versión.',
         'Sin autointersecciones nuevas; se conservan los 37 pares por brazo del codo a 85°.',
         'Cero cruces de las 11 placas reconstruidas contra brazos, cabeza, cuello, capucha y manos.',
         'Contactos ajenos y apoyo de arma idénticos a v15. BVH de superficies: no certifica profundidad ni holgura continua.','',
         '## Pares afectados','','| Piezas | Reposo v15 | Reposo v16 | Máximo v15 | Máximo v16 |',
         '| --- | ---: | ---: | ---: | ---: |']
pairs = sorted({p for audit in (old,new) for row in audit['surface_crossings'].values() for p in row if modified.intersection(p.split('/'))})
for pair in pairs:
    values = [audit['surface_crossings']['rest'].get(pair,0) for audit in (old,new)]
    maxima = [max(row.get(pair,0) for row in audit['surface_crossings'].values()) for audit in (old,new)]
    lines.append('| '+pair+' | '+' | '.join(str(n) for n in values+maxima)+' |')
lines += ['','## Direcciones con armadura exterior','','Rayos radiales del torso cada 5° (72 por altura), sobre piezas de coraza en reposo.',
          'Muestreo de secciones, no porcentaje de superficie ni prueba de protección continua.',
          'Las cotas altas incluyen aberturas de axila deliberadas.','',
          '| Altura (m) | v15 / 72 | v16 / 72 | Frente / derecha / espalda / izquierda en v16 |',
          '| --- | ---: | ---: | --- |']
for z,row in verification['torso_radial_coverage']['v16'].items():
    previous = verification['torso_radial_coverage']['v15'][z]
    lines.append(f"| {z} | {previous['armor_directions']} | {row['armor_directions']} | "+
                 ' / '.join('sí' if row[k] else 'abertura' for k in ('front','right','back','left'))+' |')
(report/'contacts.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
# Retain the full decoded diagnostic video, with representative front/back movement samples.
selected = {0:'Reposo',15:'Alcance',45:'Giro delantero',75:'Giro posterior',105:'Paso izquierdo',135:'Paso derecho',
            165:'Garras',195:'Muñeca',225:'Cuello izquierdo',255:'Cuello derecho',285:'Mirar abajo',315:'Paso + giro'}
board = Image.new('RGB',(1440,1320),(32,32,35))
draw = ImageDraw.Draw(board)
video = av.open(str(output/'global-check.mp4'))
stream = video.streams.video[0]
count = 0
for i,frame in enumerate(video.decode(video=0)):
    assert frame.width==900 and frame.height==1000
    count += 1
    if i in selected:
        j = list(selected).index(i)
        board.paste(frame.to_image().resize((360,400),Image.Resampling.LANCZOS),((j%4)*360,(j//4)*440+40))
        draw.text(((j%4)*360+12,(j//4)*440+12),selected[i],font=font,fill='white')
assert count==331 and stream.average_rate==30
metadata = {'frames':count,'fps':30,'duration_s':float(stream.duration*stream.time_base),'width':900,'height':1000}
video.close()
board.save(output/'video-keyframes.png')
(cache/'video-metadata.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
Image.open(cache/'gloom-preview.ppm').save(output/'gloom-preview.png')
board = Image.new('RGB',(1260,315),(235,235,235))
draw = ImageDraw.Draw(board)
for col,direction in enumerate(('front','profile','back')):
    source = Image.open(output/('silhouette-'+direction+'.png')).convert('RGBA')
    for j,size in enumerate((200,100,50)):
        panel = ImageOps.contain(source,(size,size),Image.Resampling.LANCZOS)
        board.paste(panel,(col*420+(0,175,285)[j],35+(200-panel.height)),panel)
    draw.text((col*420+15,260),('Frente','Perfil','Espalda')[col]+' / 200, 100, 50 px',font=font,fill=(20,20,20))
board.save(output/'combat-silhouette.png')
print('H16_EVIDENCE',json.dumps(metadata),'affected_pairs',len(pairs))
