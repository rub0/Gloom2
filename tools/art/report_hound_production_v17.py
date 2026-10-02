"""Rebuild compact H07 tables and contact/video/silhouette evidence from measured artifacts."""
import hashlib
import json
import re
import subprocess
from pathlib import Path
import av
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT/'.cache/hound-production-v17'
REPORT = ROOT/'reports/hound-production-107'
REPORT.mkdir(parents=True,exist_ok=True)
manifest = json.loads((ROOT/'art/characters/hound/v17/production-reference.json').read_text())
source = json.loads((WORK/'verification.json').read_text())
cooked = json.loads((WORK/'cooked.json').read_text())
lods = json.loads((WORK/'lod-verification.json').read_text())
old = json.loads((ROOT/'.cache/hound-armor-v16/audit.json').read_text())
audit = json.loads((WORK/'audit.json').read_text())
assert audit['self_crossings']==old['self_crossings']
assert all(set(v)==set(old['surface_crossings'][k]) for k,v in audit['surface_crossings'].items())
assert all({n for n,c in v.items() if c}=={n for n,c in old['weapon_crossings'][k].items() if c}
           for k,v in audit['weapon_crossings'].items())
for item in manifest['files']:
    assert hashlib.sha256((ROOT/item['path']).read_bytes()).hexdigest()==item['sha256']
tracked = subprocess.check_output(['rtk','proxy','git','ls-files','-s','--','art/characters/hound','assets/characters/hound_rig'],cwd=ROOT,text=True)
historical = []
for line in tracked.splitlines():
    metadata,path = line.split('\t',1)
    if '/v17/' in path or Path(path).suffix not in ('.blend','.gltf','.bin'):
        continue
    data = (ROOT/path).read_bytes()
    assert hashlib.sha1(f'blob {len(data)}\0'.encode()+data).hexdigest()==metadata.split()[1],path
    historical.append(path)
(WORK/'integrity.json').write_text(json.dumps({'previous_git_files_exact':historical,'v17_manifest_exact':True},indent=2)+'\n')
rows = ['# Inventario H07 / v17','',
    'Fuente: 106 componentes; 53 huesos, máximo dos influencias. Siete primitivas TPS y cuatro FPS.',
    'Los LODs son los índices reales del cooker; comparten vértices de autoría. Cada LOD/FPS tiene una copia GPU del buffer de su material.',
    'V cocinados por pieza incluye splits de material/normal/UV; bytes incluyen copias e índices, sin alineación del driver.','',
    '| Material | V cocinados | T LOD0 | T LOD1 | T LOD2 | T FPS | Bytes geométricos | D LOD1/2 (m) |',
    '| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |']
for row in cooked['materials']:
    rows.append('| '+row['material']+' | '+' | '.join(str(n) for n in [row['vertices_cooked'],*row['triangles'],row['geometry_bytes']])+
                ' | '+' / '.join(f'{n:.3f}' for n in row['transition_distances_m'])+' |')
rows += ['','Distancias nominales distancia/radio 30/80 con los bounds de reposo. La selección real usa los bounds de la instancia.',
         'Siete materiales OPAQUE; UV0 y factores de diagnóstico, sin imágenes finales. Geometría total: '+str(cooked['totals']['geometry_bytes'])+' bytes.',
         '', '| Componente | V v16→v17 | T v16→v17 | V cocinados | T LOD0/1/2/FPS | Bytes GPU | Frontera / destino del detalle |',
         '| --- | ---: | ---: | ---: | --- | ---: | --- |']
for part in source['components']:
    row = lods['components'][part['part']]
    role = ('Rígido: '+part['bone']) if part['bone'] else 'Deformable; loops conservados'
    if part['part']=='Waist_wrap':
        role = 'Deformable; 13 anillos ×24 columnas; pesos de vértices retenidos'
    role += '; geometría de silueta'
    if part['ratio']<1:
        role += '; relieve restante en v16 para H09'
    rows.append(f"| {part['part']} | {part['source_vertices']}→{part['vertices']} | {part['source_triangles']}→{part['triangles']} | "+
                f"{row['cooked_vertices']} | "+'/'.join(str(n) for n in row['triangles'])+f" | {row['geometry_bytes']} | {role} |")
rows += ['','## Conservación y límites','',
    f"{len(historical)} archivos anteriores (.blend/glTF/BIN) comparados byte a byte con los blobs Git; manifiestos v13–v16 y v17 comprobados.",
    '10 superficies deformables exactas, incluida capucha, cuello, brazos, guantes, pantalón, paños y soporte del torso.',
    'La faja conserva todas sus filas de altura, doble pared y pesos en los vértices retenidos. No se elimina ninguna pieza o superficie oculta.',
    'Cabeza, ojos, manos/garras, placas, carcasa, grebas y filos conservan geometría de silueta; biseles/relieve descartado siguen en v16 para H09.',
    'Las 95 piezas rígidas siguen separadas y con un solo hueso; no hay soldaduras entre piezas ni con la piel.',
    'Los 106 componentes TPS son cerrados/conexos/orientados y tienen volumen positivo. FPS corta por pesos >95 % según el contrato:',
    'bordes abiertos en el recorte proximal del brazo son intencionales; no se tapan ni se exporta un segundo cuerpo superpuesto.',
    'La limpieza retira degenerados heredados de polígonos de dedos/filos. LOD0/1/2/FPS cocinados: cero triángulos de área nula.',
    f"Máximo superficial bidireccional muestreado v16/v17: {max(p['surface_sample_error_m'] for p in source['components'])*1000:.6f} mm.",
    'Se muestrean vértices y centros de triángulos; no es una cota matemática continua ni comparación solo de recuentos.',
    '33 poses ×5.565 pares: mismos conjuntos de contactos y autointersecciones que v16; cambian los recuentos de caras tras la reducción.',
    'Persisten codos a 85°, cabeza/torso al mirar abajo, flexión fuerte de capucha, inserciones de placas/guantes y apoyo Soul Reaper inválido.',
    'H08 debe resolver pesos/holguras y agarres de las cinco armas; no se ha iniciado. H09 conserva la maestra para UV/bake.']
(REPORT/'inventario.md').write_text('\n'.join(rows)+'\n',encoding='utf-8')

measurements = json.loads((WORK/'measurements.json').read_text())
lines = ['# Medidas H07 / v17','',
    'Release, Ryzen 7 3700X/GTX 1070, 1920×1080 nativo, IMMEDIATE, VSync off; 120 frames de calentamiento y 360 medidos por proceso.',
    'Siete TPS con cinco armas y FPS local; 32 mapas 2K/115.116 claves diagnósticos de H06. Ruta game:/h07-v17/hound.gltf, TPS LOD0.',
    'Serie completa en orden, sin builds/tests simultáneos. CPU incluye HUD/audio/disparo y Present; GPU excluye Present.',
    '', '| Pasada | FPS | CPU media | p50 | p95 | p99 | Máximo ms |', '| --- | ---: | ---: | ---: | ---: | ---: | ---: |',
    # Historical observation from preflight-null.log; not regenerated by the four-run protocol.
    '| Previa, lector interrumpido | 277.04 | 3.610 | 3.591 | 4.055 | 4.242 | 4.580 |']
for row in measurements:
    text = '\n'.join(row['evidence'])
    match = re.search(r'Factory benchmark: samples=360 mean=([\d.]+) ms fps=([\d.]+) p50=([\d.]+) p95=([\d.]+) p99=([\d.]+) max=([\d.]+)',text)
    assert match
    lines.append('| '+row['name']+' | '+' | '.join(match[i] for i in (2,1,3,4,5,6))+' |')
lines += ['','La pasada previa sí superó el margen de p99 ≤4 ms, sin superar 5 ms. No se borra ni se atribuye a ruido o al lector:',
    'el lector añadió por error cero aplazamientos acumulados de uploads durante el arranque; el 106 también registra 31.',
    'Se corrigió únicamente esa comprobación nueva: siguen exigidos cero evicciones totales, recursos ausentes y aplazamientos durante la medida.',
    'Tras corregirla se ejecutó la serie completa de cuatro, todas dentro de p99 ≤4/max ≤5 ms. La causa del tiempo mayor previo no está establecida.',
    'Cinco observaciones conservadas: 0/1.800 frames >5 ms; margen p99 de 4 ms cumplido en 4/5, objetivo de 5 ms en 5/5.',
    'No extrapola una partida humana/autoritativa de ocho ni mapas/clips finales. Repetir al cambiar rig, UV/bakes o clips.',
    '', '## Distribuciones y contadores de la serie completa','']
for row in measurements:
    lines += ['### '+row['name'],'','```text',*row['evidence'],'```','']
(REPORT/'mediciones.md').write_text('\n'.join(lines).rstrip()+'\n',encoding='utf-8')

font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf',18)
board = Image.new('RGB',(1560,780),(235,235,235))
draw = ImageDraw.Draw(board)
silhouette_metrics = []
for row,view in enumerate(('front','side','back')):
    paths = [ROOT/f'docs/art/hound/mesh-v16/silhouette-{"profile" if view=="side" else view}.png']
    paths += [WORK/f'lod-review/lod{i}-{view}.png' for i in range(3)]
    original = Image.open(paths[0]).convert('RGBA')
    for column,path in enumerate(paths):
        picture = Image.open(path).convert('RGBA')
        for size,xoffset in ((200,0),(100,180),(50,285)):
            scale = size/(original.getchannel('A').getbbox()[3]-original.getchannel('A').getbbox()[1])
            resized = picture.resize((round(picture.width*scale),round(picture.height*scale)),Image.Resampling.LANCZOS)
            board.paste(resized,(column*390+xoffset,row*260+25),resized)
        draw.text((column*390+8,row*260+235),('V16' if column==0 else f'LOD{column-1}')+' / '+view,font=font,fill='black')
        if column==1:
            a,b = np.array(original.getchannel('A'))>127,np.array(picture.getchannel('A'))>127
            silhouette_metrics.append({'view':view,'iou':float((a&b).sum()/(a|b).sum())})
board.save(WORK/'silhouettes.png')
board = Image.new('RGB',(1440,1320),(32,32,35))
draw = ImageDraw.Draw(board)
selected = (0,15,45,75,105,135,165,195,225,255,285,315)
video = av.open(str(WORK/'renders/global-check.mp4'))
frames = 0
for i,frame in enumerate(video.decode(video=0)):
    assert frame.width==900 and frame.height==1000
    frames += 1
    if i in selected:
        j = selected.index(i)
        board.paste(frame.to_image().resize((360,400)),((j%4)*360,(j//4)*440+40))
        draw.text(((j%4)*360+12,(j//4)*440+12),f'Frame {i}',font=font,fill='white')
assert frames==331 and video.streams.video[0].average_rate==30
video.close()
board.save(WORK/'video-keyframes.png')
Image.open(WORK/'gloom-preview.ppm').save(WORK/'gloom-preview.png')
for row in measurements:
    for suffix in ('a','b'):
        Image.open(WORK/f"{row['name']}-{suffix}.ppm").save(WORK/f"{row['name']}-{suffix}.png")
(WORK/'visual-metrics.json').write_text(json.dumps({'silhouettes':silhouette_metrics,'decoded_video_frames':frames},indent=2)+'\n')
print('H07_REPORT',len(historical),silhouette_metrics,frames)
