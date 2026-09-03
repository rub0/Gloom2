"""Create a collision overlay and video/renderer comparison from inspected captures."""
import argparse
import json
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw

def report(assets,output,reference):
    manifest=json.loads((assets/'factory_scene.json').read_text());doc=json.loads((assets/'factory.gltf').read_text());binary=(assets/'factory.bin').read_bytes()
    def attribute(index,width):
        a=doc['accessors'][index];v=doc['bufferViews'][a['bufferView']]
        return np.frombuffer(binary,dtype='<f4' if a['componentType']==5126 else '<u4',count=a['count']*width,offset=v.get('byteOffset',0)+a.get('byteOffset',0)).reshape(-1,width)
    visual=[]
    for p in doc['meshes'][0]['primitives']:
        points=attribute(p['attributes']['POSITION'],3)*manifest['unit_scale'];indices=attribute(p['indices'],1).ravel()
        visual.extend(points[indices].reshape(-1,3,3))
    collision=np.asarray(manifest['collision']['vertices'])[manifest['collision']['indices']].reshape(-1,3,3)
    canvas=Image.new('RGB',(1400,610),'#f4f6fa');draw=ImageDraw.Draw(canvas)
    for panel,(lo,hi,title) in enumerate([(-.15,.6,'Rutas bajas: Y -0.15 a 0.60 m'),(6.8,7.8,'Rutas altas: Y 6.8 a 7.8 m')]):
        origin=panel*700
        draw.text((origin+20,15),title,fill='#18283c')
        draw.text((origin+20,36),'Azul: malla visual   Naranja: RepX   Negro: spawns',fill='#18283c')
        def xy(p): return origin+30+(p[0]+65)*5.3,80+(p[2]+47)*6.5
        for triangles,color in ((visual,'#4382bd'),(collision,'#da8d37')):
            for tri in triangles:
                if not (lo<=float(np.mean(tri[:,1]))<=hi) or np.ptp(tri[:,1])>.3: continue
                polygon=[xy(p) for p in tri];draw.line(polygon+[polygon[0]],fill=color,width=1)
        for index,entity in enumerate([e for e in manifest['entities'] if e['type']=='SpawnPoint']):
            position=np.array(entity['source']['position'])*manifest['unit_scale']
            if not lo-.2<=position[1]<=hi+.2:continue
            x,y=xy(position);draw.ellipse((x-4,y-4,x+4,y+4),fill='#111111');draw.text((x+6,y-10),str(index+1),fill='#111111')
        draw.text((origin+20,575),'Misma escala 0.15; incluye barreras originales que no son visibles.',fill='#18283c')
    canvas.save(output/'collision-overlay.png')
    left=Image.open(reference).convert('RGB').resize((640,360))
    right=Image.open(output/'captures/factory-central-walkway.ppm').convert('RGB').resize((640,360))
    pair=Image.new('RGB',(1280,412),'#151a24');draw=ImageDraw.Draw(pair)
    pair.paste(left,(0,26));pair.paste(right,(640,26))
    draw.text((12,7),'Video original 00:45',fill='white')
    draw.text((652,7),'Factory restaurada: otra posicion de camara',fill='white')
    draw.text((12,393),'Comparacion visual de materiales y estructura; el arma, personajes y HUD se completan en 62-64.',fill='white')
    pair.save(output/'video-comparison.png')
    for folder,name,columns,width in [('captures','factory-contact',2,640),('materials','material-contact',4,320)]:
        files=sorted((output/folder).glob('*.ppm'))
        if not files: continue
        height=180 if folder=='captures' else 320
        sheet=Image.new('RGB',(columns*width,((len(files)+columns-1)//columns)*(height+26)),'#151a24')
        labels=ImageDraw.Draw(sheet)
        for index,file in enumerate(files):
            x=(index%columns)*width;y=(index//columns)*(height+26)
            labels.text((x+8,y+7),file.stem,fill='white')
            sheet.paste(Image.open(file).convert('RGB').resize((width,height)),(x,y+26))
        sheet.save(output/(name+'.png'))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('assets','output','reference'): parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();report(args.assets,args.output,args.reference)
