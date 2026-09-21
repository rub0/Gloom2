'''Compose H05 evidence from actual renders; decode every video frame and verify frozen files.'''
import sys
import json
import hashlib
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageOps
import av

root = Path(__file__).resolve().parents[2]
v14 = '--v14' in sys.argv
output = root/('docs/art/hound/mesh-v14' if v14 else 'docs/art/hound/review-h05')
cache = root/('.cache/hound-mesh-v14' if v14 else '.cache/hound-h05')
font_path = 'C:/Windows/Fonts/arial.ttf'

def board(items,columns,cell=(450,540)):
    rows = (len(items)+columns-1)//columns
    result = Image.new('RGB',(columns*cell[0],rows*cell[1]),(32,34,37))
    draw = ImageDraw.Draw(result)
    font = ImageFont.truetype(font_path,20)
    for i,(name,title) in enumerate(items):
        with Image.open(output/(name+'.png')) as source:
            source.load()
            pane = ImageOps.contain(source.convert('RGB'),(cell[0],cell[1]-42))
        x,y = i%columns*cell[0],i//columns*cell[1]
        result.paste(pane,(x+(cell[0]-pane.width)//2,y))
        draw.text((x+16,y+cell[1]-32),title,font=font,fill='white')
    return result

board([('face','Rostro'),('collar','Claviculas / hombros'),('hands','Mano / guantelete'),
       ('boots','Grebas / botas'),('waist','Cintura / tela'),('back-assembly','Ensamblaje posterior')],3).save(output/'details.png')
if v14:
    board([('bracer-side','F01 / tres pinchos'),('claws-palm','F02 / garra palmar'),
           ('boots','F03 / pie-rodilla'),('rear-blades','F04 / pinchos posteriores'),
           ('collar','F05 / clavicula-peto'),('torso','F06 / abdomen envolvente')],3).save(output/'feedback-details.png')
    board([('claws-dorsal','Dorsal'),('claws-palm','Palmar'),('claws-side','Lateral')],3,cell=(450,540)).save(output/'claws.png')
board([('pose-reach','Alcance'),('pose-combined','Paso + giro'),('pose-elbow','Codo 85 grados / limite'),
       ('pose-fist','Dedos'),('pose-look-up','Cabeza arriba'),('pose-look-down','Cabeza abajo')],3).save(output/'poses.png')
board([('weapon-full','Soul Reaper / apoyo diagnostico'),('weapon-contact','Apoyo local: no agarre final')],2).save(output/'weapon.png')
silhouettes = Image.new('RGB',(920,720),(232,230,222))
draw = ImageDraw.Draw(silhouettes)
font = ImageFont.truetype(font_path,21)
draw.text((24,20),'H05 / SILUETA / ALTURA VISIBLE EN PIXELES',fill=(24,27,29),font=font)
for col,name in enumerate(('front','profile','back')):
    with Image.open(output/('silhouette-'+name+'.png')) as source:
        source = source.convert('RGBA')
        assert source.getchannel('A').getextrema()==(0,255)
        source = source.crop(source.getchannel('A').getbbox())
        for row,height in enumerate((200,100,50)):
            resized = source.resize((round(source.width*height/source.height),height),Image.Resampling.LANCZOS)
            x,y = col*300+(300-resized.width)//2,(70,330,510)[row]
            silhouettes.paste(resized,(x,y),resized)
            draw.text((col*300+94,y+height+8),str(height)+' px',fill=(24,27,29),font=font)
draw.text((24,687),'Escalas ilustrativas; no es una captura del combate en el motor.',fill=(24,27,29),font=ImageFont.truetype(font_path,19))
silhouettes.save(output/'combat-silhouette.png')

video = av.open(str(output/'global-check.mp4'))
stream = video.streams.video[0]
assert (stream.width,stream.height)==(900,1000) and stream.average_rate==30
chosen = {0,15,45,75,105,135,165,195,225,255,285,315,330}
frames = []
for index,frame in enumerate(video.decode(video=0)):
    if index in chosen:
        image = frame.to_image()
        image.thumbnail((270,300))
        frames.append((index,image))
assert index+1==331 and len(frames)==len(chosen)
video.close()
sheet = Image.new('RGB',(5*290,3*338),(32,34,37))
draw = ImageDraw.Draw(sheet)
for n,(index,image) in enumerate(frames):
    x,y = n%5*290,n//5*338
    sheet.paste(image,(x,y))
    draw.text((x+10,y+305),'Frame '+str(index+1),fill='white',font=font)
sheet.save(output/'video-keyframes.png')
print('VIDEO_VERIFIED',331,'900x1000',30,'fps',331/30,'seconds')

for item in json.loads((root/'art/characters/hound/v13/sculpture-reference.json').read_text())['files']:
    assert hashlib.sha256((root/item['path']).read_bytes()).hexdigest()==item['sha256']
    assert (root/item['source']).read_bytes()==(root/item['path']).read_bytes()
print('FROZEN_FILES_EXACT',3)
audit = json.loads((cache/'audit.json').read_text())
rest = audit['surface_crossings']['rest']
introduced = {}
for pose,row in audit['surface_crossings'].items():
    for pair,count in row.items():
        if pair not in rest:
            introduced.setdefault(pair,{})[pose] = count
print('REST_CONTACTS',json.dumps(rest,sort_keys=True))
print('INTRODUCED_CONTACTS',json.dumps(introduced,sort_keys=True))
print('TOPOLOGY',len(audit['topology']),'closed connected oriented positive volumes')
print('IMAGES',len(list(output.glob('*.png'))))

report = root/('reports/hound-feedback-97' if v14 else 'reports/hound-review-95')
report.mkdir(parents=True,exist_ok=True)
rows = ['# H05 — inventario de contactos medidos','',
        f"{audit['poses']} poses independientes; {audit['parts']} componentes, {audit['pairs_per_pose']} pares por pose, {audit['pair_evaluations']} evaluaciones.",
        'Cruces de superficies BVH: el conteo depende de la teselación y no mide profundidad.',
        'No certifica contención, distancia mínima continua ni todos los movimientos.',
        'La tabla incluye cada par con algún cruce; los restantes dan cero en estas muestras.','',
        '| Par | Reposo | Máximo | Poses con cruce / 33 |',
        '| --- | ---: | ---: | ---: |']
all_pairs = sorted({pair for row in audit['surface_crossings'].values() for pair in row})
for pair in all_pairs:
    values = [row.get(pair,0) for row in audit['surface_crossings'].values()]
    rows.append('| '+pair+' | '+str(rest.get(pair,0))+' | '+str(max(values))+' | '+str(sum(v>0 for v in values))+' |')
rows += ['', '## Autointersecciones no adyacentes', '',
         '| Pose | Componente | Pares de caras |','| --- | --- | ---: |']
for pose,row in audit['self_crossings'].items():
    for name,count in row.items():
        rows.append('| '+pose+' | '+name+' | '+str(count)+' |')
rows += ['', f"Cero en los demás componentes/muestras. Los {audit['parts']} componentes son conexos, cerrados,",
         'con aristas orientadas consistentemente y volumen positivo en reposo.', '',
         '## Soul Reaper original', '',
         'Escala 0,43; apoyo diagnóstico de H01 sobre carcasa posterior. Se comprueban',
         f"los {audit['parts']} componentes en reposo con dedos curvados y durante el alcance.",
         'El arma sigue la transformación de la mano; no se modifica la malla ni el rig.', '',
         '| Componente | Apoyo | Alcance con apoyo |','| --- | ---: | ---: |']
for name in sorted(audit['weapon_crossings']['grip']):
    a,b = audit['weapon_crossings']['grip'][name],audit['weapon_crossings']['reach'][name]
    if a or b:
        rows.append('| '+name+' | '+str(a)+' | '+str(b)+' |')
rows += ['', ('Cero cruces en los demás componentes; los dedos y el pulgar afectados constan en la tabla.' if v14
         else 'Cero cruces en los demás componentes, incluidos guantes y dedos.'),
         '**El apoyo falla globalmente; no es un agarre válido ni un socket final.**', '']
(report/'contacts.md').write_text('\n'.join(rows),encoding='utf-8')
print('CONTACT_INVENTORY',len(rest),'rest pairs',len(all_pairs),'pairs hit in any pose')
