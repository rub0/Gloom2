"""Audit legacy Particle Universe dependencies and translate bounded Gloom recipes.

Only reads legacy. Requires Pillow; no Ogre or shader interpreter at runtime.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
from PIL import Image

def run(legacy, output):
    legacy=legacy.resolve();output=output.resolve()
    if legacy==output or legacy in output.parents or output in legacy.parents: raise ValueError('Keep legacy read-only')
    media=legacy/'Exes/media';particles=media/'particles'
    output.mkdir(parents=True,exist_ok=True);(output/'textures').mkdir(exist_ok=True)
    sources={}
    def source(path):
        sources[path.relative_to(legacy).as_posix()]=hashlib.sha256(path.read_bytes()).hexdigest()
    inventory=[]
    for p in sorted(particles.rglob('*')):
        if p.suffix.lower() not in ('.pu','.material'): continue
        source(p);text=p.read_text(errors='replace')
        inventory.append({'path':p.relative_to(legacy).as_posix(),'materials':re.findall(r'^\s*material\s+(\S+)',text,re.M),
                          'textures':re.findall(r'^\s*texture\s+(\S+)',text,re.M)})
    for p in sorted(media.rglob('heatHaze.*')):
        if p.suffix in ('.hlsl','.material','.pu'): source(p)
    names=['pump_flare_05.png','pump_smoke_01.png','pump_ring_04.png',
           'mp_hit_metal_sparkle.png','mp_bloodsplat_01.png','magma_NRM.tga']
    # Named original PNGs are the artist-exported source counterparts of DDS.
    extra=sorted(particles.glob('Explosion/png/*.png'))
    if not extra: extra=sorted(particles.glob('Explosions/png/*.png'))
    if not extra: extra=sorted(particles.glob('Fire/png/*.png'))
    if not extra: raise ValueError('Missing original explosion/fire review texture')
    names.append(extra[0].name)
    textures=[]
    for name in names:
        candidates=sorted(media.rglob(name))
        if not candidates: raise ValueError('Missing texture '+name)
        hashes={hashlib.sha256(p.read_bytes()).hexdigest() for p in candidates}
        if len(hashes)!=1: raise ValueError('Ambiguous texture '+name)
        p=candidates[0];source(p)
        target=Path('textures')/(p.stem+'.png')
        Image.open(p).convert('RGBA').save(output/target)
        textures.append({'uri':target.as_posix(),'source':p.relative_to(legacy).as_posix()})
    recipes=[]
    def recipe(name,material,burst=0,rate=0,life=.5,speed=.3,spread=.2,gravity=0,size_start=.08,size_end=.2,
               color_start=(1,1,1,1),color_end=(1,1,1,0),trail_seconds=0,distortion=0,source_recipe=''):
        recipes.append(dict(name=name,material=material,burst=burst,rate=rate,life=life,speed=speed,spread=spread,
            gravity=gravity,size_start=size_start,size_end=size_end,color_start=color_start,color_end=color_end,
            trail_seconds=trail_seconds,distortion=distortion,source_recipe=source_recipe))
    recipe('archangel_energy',0,rate=18,life=.45,speed=.18,spread=.05,size_start=.06,size_end=.12,
           color_start=(.03,1.4,2,.5),color_end=(.01,.5,1,0),source_recipe='spawn.pu; authored cyan shoulder/wing adaptation')
    recipe('shadow_smoke',1,rate=28,life=1,speed=.24,spread=.12,gravity=-.16,size_start=.18,size_end=.48,
           color_start=(.015,.012,.022,.9),color_end=(.04,.035,.05,0),source_recipe='shadow.pu')
    recipe('shadow_eyes',0,rate=36,life=.30,speed=.45,spread=.02,size_start=.035,size_end=.05,trail_seconds=.18,
           color_start=(2,.004,.008,.75),color_end=(.8,0,0,0),source_recipe='authored concept red eye trails; original Flare_05')
    recipe('muzzle',0,burst=3,life=.10,speed=.3,spread=.08,size_start=.35,size_end=.5,
           color_start=(2.5,1.1,.12,.9),color_end=(1,.2,.01,0),source_recipe='fogonazo3.pu; bounded Soul Reaper adaptation')
    recipe('impact',3,burst=14,life=.35,speed=.4,spread=1.4,gravity=-2,size_start=.025,size_end=.008,trail_seconds=.08,
           color_start=(2,1.1,.2,.9),color_end=(1,.25,.02,0),source_recipe='Hit recipes; original metal sparkle')
    recipe('damage',3,burst=8,life=.25,spread=.6,size_start=.035,size_end=.01,
           color_start=(1.3,.08,.03,.7),source_recipe='Hit recipes; authored damage cue')
    recipe('shield',2,burst=4,life=.55,speed=.1,spread=.2,size_start=.25,size_end=1.1,
           color_start=(.08,.6,1.5,.5),source_recipe='groundTouch.pu ring; existing Guard event')
    recipe('landing',2,burst=2,life=.35,speed=.05,spread=.05,size_start=.12,size_end=.65,
           color_start=(.35,.28,.2,.35),source_recipe='groundTouch.pu')
    recipe('spawn',0,burst=26,life=.85,speed=.7,spread=.4,size_start=.035,size_end=.09,
           color_start=(.2,.9,1.8,.6),source_recipe='spawn.pu; flattened bounded upward/vortex burst')
    recipe('death',1,burst=12,life=.8,speed=.2,spread=.35,size_start=.08,size_end=.4,
           color_start=(.08,.04,.06,.4),source_recipe='shadow.pu; finite death release')
    recipe('lava',0,rate=5,life=1.1,speed=.6,spread=.2,size_start=.035,size_end=.015,
           color_start=(1.7,.25,.01,.6),source_recipe='Fire recipes; original flare texture')
    recipe('heat',5,rate=4,life=.9,speed=.35,spread=.15,size_start=.25,size_end=.55,distortion=.004,
           color_start=(1,1,1,.16),source_recipe='heatHaze.pu + heatHaze.hlsl; scene/depth snapshot refraction')
    recipe('blood_review',4,burst=8,life=.5,speed=.2,spread=.6,gravity=-1,size_start=.08,size_end=.16,
           color_start=(.45,.015,.01,.55),source_recipe='Blood: review only; no new gore gameplay event')
    recipe('explosion_review',6,burst=8,life=.65,speed=.4,spread=.6,size_start=.2,size_end=.6,
           color_start=(2,.7,.06,.65),source_recipe='original Fire/Explosion texture: review only; no explosive weapon')
    (output/'recipes.json').write_text(json.dumps({'version':1,'simulation':'CPU analytical ballistic; bounded 2048 particles / 64 emitters',
        'recipes':recipes},indent=2)+'\n')
    doc={'asset':{'version':'2.0','generator':'Gloom original VFX translator 1'},'scene':0,'scenes':[{'nodes':list(range(7))}],
         'nodes':[],'meshes':[],'materials':[],'images':[{'uri':t['uri']} for t in textures],
         'textures':[{'source':i} for i in range(7)],'buffers':[{'uri':'effects.bin','byteLength':0}],
         'bufferViews':[],'accessors':[]}
    binary=bytearray()
    def accessor(values,width,typ,integer=False):
        offset=len(binary);binary.extend(struct.pack('<'+('I' if integer else 'f')*len(values),*values))
        doc['bufferViews'].append({'buffer':0,'byteOffset':offset,'byteLength':len(binary)-offset})
        a={'bufferView':len(doc['bufferViews'])-1,'componentType':5125 if integer else 5126,'count':len(values)//width,'type':typ}
        if typ=='VEC3':a.update(min=[min(values[k::3]) for k in range(3)],max=[max(values[k::3]) for k in range(3)])
        doc['accessors'].append(a);return len(doc['accessors'])-1
    pos=accessor([-.5,-.5,0,.5,-.5,0,.5,.5,0,-.5,.5,0],3,'VEC3')
    normal=accessor([0,0,-1]*4,3,'VEC3');uv=accessor([0,1,1,1,1,0,0,0],2,'VEC2');indices=accessor([0,2,1,0,3,2],1,'SCALAR',True)
    for i in range(7):
        doc['nodes'].append({'name':names[i],'mesh':i})
        doc['meshes'].append({'primitives':[{'attributes':{'POSITION':pos,'NORMAL':normal,'TEXCOORD_0':uv},'indices':indices,'material':i}]})
        material={'name':names[i],'pbrMetallicRoughness':{'baseColorTexture':{'index':i},'metallicFactor':0,'roughnessFactor':1},
                  'alphaMode':'BLEND','doubleSided':True}
        if i==5:
            # Heat flow is normal data, never an sRGB color. The cooker selects
            # linear BC5 and the refraction shader consumes its RG channels.
            del material['pbrMetallicRoughness']['baseColorTexture']
            material['normalTexture']={'index':i}
        if i in (0,3,6):material['extras']={'gloom':{'additive':True}}
        doc['materials'].append(material)
    doc['buffers'][0]['byteLength']=len(binary)
    (output/'effects.bin').write_bytes(binary);(output/'effects.gltf').write_text(json.dumps(doc,indent=2)+'\n')
    (output/'manifest.json').write_text(json.dumps({'version':1,'sources':dict(sorted(sources.items())),
        'inventory':inventory,'textures':textures,'adaptation':'Original textures, bounded authored recipes; no Ogre runtime. Blood/explosion review only.'},indent=2)+'\n')
    print(f'{len(inventory)} recipes/material files audited; {len(textures)} textures recovered; {len(recipes)} modern recipes')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--legacy-root',type=Path,required=True);p.add_argument('--output-root',type=Path,required=True)
    a=p.parse_args();run(a.legacy_root,a.output_root)
