'''Compare the original painting and actual v13/v14 renders; summarize contact changes without hiding new pairs.'''
import json
import hashlib
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageOps
root = Path(__file__).resolve().parents[2]
output = root/'docs/art/hound/mesh-v14'
font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf',26)
# Both mesh panes are taken from the same camera/scale/light comparison, not independently fitted to their bounds.
sheet = Image.open(output/'before-after.png').convert('RGB')
# The orthographic board has a 5.15 m horizontal span and 2600 px width; target Z 1.95 m.
scale = sheet.width/5.15
def pane(z):
    cx = sheet.width/2-1.845*scale
    top = sheet.height/2-((z+1.92)-1.95)*scale
    bottom = sheet.height/2-((z-.03)-1.95)*scale
    return sheet.crop((round(cx-.61*scale),round(top),round(cx+.61*scale),round(bottom)))
board = Image.new('RGB',(1950,1120),(40,42,45))
draw = ImageDraw.Draw(board)
items = [(Image.open(root/'docs/art/characters/hound-original-concept.jpg'),'CONCEPT / PINTURA'),(pane(2.1),'V13 / ANTES / RENDER'),(pane(0),'V14 / FEEDBACK / RENDER')]
for i,(im,title) in enumerate(items):
    im = ImageOps.contain(im.convert('RGB'),(630,1040))
    board.paste(im,(i*650+(650-im.width)//2,20))
    draw.text((i*650+25,1080),title,font=font,fill='white')
board.save(output/'concept-comparison.png')
preview = root/'.cache/hound-mesh-v14/gloom-preview.ppm'
if preview.exists():
    Image.open(preview).save(output/'gloom-preview.png')
a = json.loads((root/'.cache/hound-mesh-v14/audit.json').read_text())
b = json.loads((root/'.cache/hound-h05/audit.json').read_text())
rows = ['# H05 / v14 frente a v13: cambios de contacto','',
        'Mismas 33 poses y procedimiento BVH. Los recuentos cambian con la teselación; no equivalen a penetración ni a una distancia.',
        'Solo se listan pares nuevos en alguna pose, ausentes de todas las muestras v13. El inventario completo está en contacts.md.','',
        '| Par nuevo | Reposo v14 | Máximo v14 |','| --- | ---: | ---: |']
old_pairs = {p for row in b['surface_crossings'].values() for p in row}
new_pairs = sorted({p for row in a['surface_crossings'].values() for p in row}-old_pairs)
for pair in new_pairs:
    rows.append(f"| {pair} | {a['surface_crossings']['rest'].get(pair,0)} | {max(row.get(pair,0) for row in a['surface_crossings'].values())} |")
rows += ['', '## Pares bajo seguimiento', '', '| Par | Máximo v13 | Máximo v14 |','| --- | ---: | ---: |']
for pair in ('Head_surface/Torso_underlayer','Arm_surface_L/Collar_plate_L','Arm_surface_R/Collar_plate_R',
             'Arm_surface_L/Collar_blade_middle_L','Arm_surface_R/Collar_blade_middle_R',
             'Back_blade_L/Hood_continuous','Back_blade_R/Hood_continuous',
             'Bracer_blade_distal_L/Hand_back_full_point_L','Bracer_blade_distal_R/Hand_back_full_point_R'):
    rows.append(f"| {pair} | {max(row.get(pair,0) for row in b['surface_crossings'].values())} | {max(row.get(pair,0) for row in a['surface_crossings'].values())} |")
(root/'reports/hound-feedback-97/contact-changes.md').write_text('\n'.join(rows)+'\n',encoding='utf-8')
for candidate in ('v13','v14'):
    manifest = json.loads((root/f'art/characters/hound/{candidate}/sculpture-reference.json').read_text())
    for item in manifest['files']:
        assert hashlib.sha256((root/item['path']).read_bytes()).hexdigest()==item['sha256']
print('H14_BOARDS',len(list(output.glob('*.png'))),'PNG; both manifests exact; new contact pairs:',len(new_pairs))
