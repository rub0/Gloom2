"""Recover the two original character rigs and concept-directed PBR materials.

Uses Ogre offline; sources stay read-only. Original relative animation keys
are converted to absolute glTF TRS and retain source timing.
"""
import argparse
import hashlib
import importlib.metadata
import json
import math
from pathlib import Path
import struct
import numpy as np
from PIL import Image
from import_factory import Importer, Scene


def chunks(data, start, end):
    while start < end:
        if start + 6 > end:
            raise ValueError('Truncated Ogre chunk')
        kind, size = struct.unpack_from('<HI', data, start)
        if size < 6 or start + size > end:
            raise ValueError('Invalid Ogre chunk length')
        yield kind, start + 6, start + size
        start += size


def read_mesh(path):
    """Read audited v1.8 geometry, including Shadow's packed vertex color layout."""
    data = path.read_bytes()
    if data[:2] != b'\x00\x10' or data[2:data.index(b'\n')] != b'[MeshSerializer_v1.8]':
        raise ValueError('Expected normalized little-endian Ogre v1.8')
    result = []
    for kind, p, end in chunks(data, data.index(b'\n')+1, len(data)):
        if kind != 0x3000:
            continue
        for kind, p, stop in chunks(data, p+1, end):
            if kind != 0x4000:
                continue
            name_end = data.index(b'\n', p)
            material = data[p:name_end].decode('utf8')
            shared, count, wide = struct.unpack_from('<?I?', data, name_end+1)
            if shared:
                raise ValueError('Unexpected shared character geometry')
            p = name_end+7
            indices = np.frombuffer(data, dtype='<u4' if wide else '<u2', count=count, offset=p).astype(np.uint32)
            p += count*(4 if wide else 2)
            attributes, assignments = {}, []
            for kind, p, end in chunks(data, p, stop):
                if kind == 0x4100:
                    assignments.append(struct.unpack_from('<IHf', data, p))
                elif kind == 0x5000:
                    vertex_count = struct.unpack_from('<I', data, p)[0]
                    declarations, buffers = [], {}
                    for kind, p, end in chunks(data, p+4, end):
                        if kind == 0x5100:
                            for kind, q, e in chunks(data, p, end):
                                if kind != 0x5110: raise ValueError('Unknown vertex declaration')
                                declarations.append(struct.unpack_from('<5H', data, q))
                        elif kind == 0x5200:
                            binding, stride = struct.unpack_from('<HH', data, p)
                            raw = list(chunks(data, p+4, end))
                            if len(raw)!=1 or raw[0][0]!=0x5210 or raw[0][2]-raw[0][1]!=stride*vertex_count:
                                raise ValueError('Invalid vertex buffer')
                            buffers[binding]=(stride, raw[0][1])
                        else: raise ValueError('Unsupported geometry chunk')
                    for binding, typ, semantic, offset, index in declarations:
                        stride, start = buffers[binding]
                        if semantic==5:
                            # Original shaders do not consume the packed vertex color.
                            if typ!=30: raise ValueError('Unexpected vertex color encoding')
                            continue
                        if semantic not in (1,4,7,8,9) or typ not in (1,2,3):
                            raise ValueError('Unsupported character vertex attribute')
                        width=typ+1
                        if offset+width*4>stride: raise ValueError('Attribute exceeds stride')
                        attributes[(semantic,index)] = np.ndarray((vertex_count,width),dtype='<f4',buffer=data,
                            offset=start+offset,strides=(stride,4)).copy()
            if not all(np.all(np.isfinite(a)) for a in attributes.values()) or max(indices)>=vertex_count:
                raise ValueError('Invalid character geometry')
            result.append((material, attributes, indices, assignments))
    if not result: raise ValueError('No character submesh')
    return result


def matrix(position, rotation, scale):
    x,y,z,w=rotation
    value=np.eye(4)
    value[:3,:3]=np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],
        [2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],
        [2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])@np.diag(scale)
    value[:3,3]=position
    return value


def quaternion_product(a, b):
    x,y,z,w=a; X,Y,Z,W=b
    q=np.array([w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z])
    return (q/np.linalg.norm(q)).tolist()


def export_animations(scene, skeleton, bones):
    audits=[]
    scene.doc['animations']=[]
    for index in range(skeleton.getNumAnimations()):
        source=skeleton.getAnimation(index)
        clip={'name':source.getName(),'samplers':[],'channels':[]}
        audit={'name':source.getName(),'seconds':source.getLength(),'tracks':[],
               'translation':'bind + Ogre parent-space delta','rotation':'bind * Ogre local delta',
               'scale':'bind * Ogre scale','root_motion':'source preserved; suppressed by runtime locomotion'}
        for joint,bone in enumerate(bones):
            if not source.hasNodeTrack(joint): continue
            track=source.getNodeTrack(joint)
            times=[];values={'translation':[],'rotation':[],'scale':[]}
            for k in range(track.getNumKeyFrames()):
                key=track.getNodeKeyFrame(k);p,q,s=key.getTranslate(),key.getRotation(),key.getScale()
                times.append(float(key.getTime()))
                values['translation'].append((np.asarray(bone['translation'])+[p.x,p.y,p.z]).tolist())
                values['rotation'].append(quaternion_product(bone['rotation'],[q.x,q.y,q.z,q.w]))
                values['scale'].append((np.asarray(bone['scale'])*[s.x,s.y,s.z]).tolist())
            if not times or any(b<=a for a,b in zip(times,times[1:])): raise ValueError('Invalid original key times')
            if times[0]<0 or times[-1]>source.getLength()+1e-6: raise ValueError('Original key outside clip')
            # Ogre serializes endpoint times with tiny rounding beyond duration.
            times[-1]=min(times[-1],float(source.getLength()))
            inp=scene.accessor(times,1,'SCALAR',bounds=True)
            for path,width in (('translation',3),('rotation',4),('scale',3)):
                vals=values[path]
                clip['channels'].append({'sampler':len(clip['samplers']),'target':{'node':joint+2,'path':path}})
                clip['samplers'].append({'input':inp,'output':scene.accessor(np.asarray(vals).ravel().tolist(),width,'VEC'+str(width)),
                                         'interpolation':'LINEAR'})
            audit['tracks'].append({'bone':bone['name'],'keys':len(times),'start':times[0],'end':times[-1],
                'translation_loop_error':float(np.linalg.norm(np.array(values['translation'][0])-values['translation'][-1])),
                'rotation_loop_error':float(1-abs(np.dot(values['rotation'][0],values['rotation'][-1])))})
        scene.doc['animations'].append(clip);audits.append(audit)
    if not scene.doc['animations']: del scene.doc['animations']
    return audits


def generated_texture(scene, label, pixels):
    target=scene.imp.output/'textures'/(scene.name+'-'+label+'.png')
    target.parent.mkdir(exist_ok=True)
    Image.fromarray(np.uint8(np.clip(pixels*255,0,255))).save(target)
    index=len(scene.doc['images'])
    scene.doc['images'].append({'uri':'textures/'+target.name})
    scene.doc['textures'].append({'source':index,'sampler':0})
    return {'index':index}


def character_material(scene, name):
    arch=name=='archangel'
    diffuse='map_arcangel_DIF.jpg' if arch else 'shadow_DIF.jpg'
    packed='map_arcangel_NRM_SPEC.tga' if arch else 'shadow_NRM_SPEC.tga'
    rgb=np.asarray(Image.open(scene.imp.resolve(diffuse)).convert('RGB'),dtype=float)/255
    spec=np.asarray(Image.open(scene.imp.resolve(packed)).convert('RGBA'),dtype=float)[:,:,3]/255
    r,g,b=np.moveaxis(rgb,2,0)
    if arch:
        mask=np.clip(np.minimum(g-r,b-r)*10,0,1)
        emission=mask[:,:,None]*np.array([0.015,.7,1])
        metal=np.where((r>b*1.1)&(g>b*1.1)&(g>.08),.9,.05)
    else:
        # The source glow mask marks the six eyes; keep rust and cloth non-emissive.
        glow=np.asarray(Image.open(scene.imp.resolve('shadow_GLOW.tga')).convert('RGB'),dtype=float)/255
        mask=np.max(glow,axis=2)
        emission=mask[:,:,None]*np.array([1,.006,.003])
        yy,xx=np.indices(r.shape)
        armour=((xx/r.shape[1]>.18)&(yy/r.shape[0]<.52))
        metal=np.where(armour & (g>.065),.85,.02)
    mr=np.zeros_like(rgb);mr[:,:,1]=np.clip(.26+.32*(1-spec),.26,.58);mr[:,:,2]=metal
    material={'name':name+' concept metal',
        'pbrMetallicRoughness':{'baseColorTexture':scene.texture(diffuse),'baseColorFactor':[1,.85,.6,1] if arch else [1,1,1,1],'metallicFactor':1,'roughnessFactor':1,
            'metallicRoughnessTexture':generated_texture(scene,'metal-rough',mr)},
        'normalTexture':scene.texture(packed),'emissiveFactor':[1,1,1],
        'emissiveTexture':generated_texture(scene,'emission',emission),
        'extensions':{'KHR_materials_emissive_strength':{'emissiveStrength':4}},
        'extras':{'gloom':{'sourceMaterial':name}}}
    scene.doc['materials'].append(material)
    return {'emissive_pixels':int(np.count_nonzero(mask>.1)), 'metal_pixels':int(np.count_nonzero(metal>.5)),
        'emission_color':'cyan' if arch else 'red','roughness_range':[.26,.58]}


def convert(imp, name):
    o=imp.ogre
    source=imp.legacy/'Exes/media/models/personajes'/(name+'.mesh')
    imp.source(source)
    group='character-'+name
    r=o.ResourceGroupManager.getSingleton();r.createResourceGroup(group)
    r.addResourceLocation(str(source.parent),'FileSystem',group)
    for material in imp.materials:o.MaterialManager.getSingleton().create(material,group)
    mesh=o.MeshManager.getSingleton().createManual(name,group)
    o.MeshSerializer().importMesh(r.openResource(source.name,group),mesh.__deref__())
    skeleton=mesh.getSkeleton()
    if not skeleton: raise ValueError('Expected character skeleton')
    imp.source(imp.resolve(mesh.getSkeletonName()))
    normalized=imp.work/(name+'.mesh')
    o.MeshSerializer().exportMesh(mesh.__deref__(),str(normalized),o.MESH_VERSION_1_8)
    parts=read_mesh(normalized)
    scene=Scene(imp,name)
    material_audit=character_material(scene,name)
    scene.doc['nodes']=[{'name':name+' presentation root','children':[1]}, {'name':name,'mesh':0,'skin':0}]
    scene.doc['scenes'][0]['nodes']=[0]
    bones=[]
    handles={skeleton.getBone(i).getName():i for i in range(skeleton.getNumBones())}
    for i in range(skeleton.getNumBones()):
        bone=skeleton.getBone(i);p,q,s=bone.getPosition(),bone.getOrientation(),bone.getScale()
        bones.append({'name':bone.getName(),'translation':[p.x,p.y,p.z],
            'rotation':[q.x,q.y,q.z,q.w],'scale':[s.x,s.y,s.z],
            'parent':handles[bone.getParent().getName()] if bone.getParent() else None})
    root_rebase=None
    if name=='shadow':
        # The source skeleton is translated far away from its mesh. Recover an
        # explicit pelvis placement in mesh space; retain the source delta.
        target=np.array([-.29533267,-17.0,-101.80451965])
        root_rebase=(target-np.asarray(bones[0]['translation'])).tolist()
        bones[0]['translation']=target.tolist()
    worlds={}
    def world(i):
        if i not in worlds:
            b=bones[i];local=matrix(b['translation'],b['rotation'],b['scale'])
            worlds[i]=local if b['parent'] is None else world(b['parent'])@local
        return worlds[i]
    for i,bone in enumerate(bones):
        node={k:v for k,v in bone.items() if k!='parent'}
        node['children']=[j+2 for j,b in enumerate(bones) if b['parent']==i]
        scene.doc['nodes'].append(node)
        if bone['parent'] is None:scene.doc['nodes'][0]['children'].append(i+2)
    inverse=np.asarray([np.linalg.inv(world(i)).T.ravel() for i in range(len(bones))])
    scene.doc['skins']=[{'name':name+' original rig','joints':list(range(2,len(bones)+2)),
        'inverseBindMatrices':scene.accessor(inverse.ravel().tolist(),16,'MAT4')}]
    primitives=[];max_influences=0
    for material,attrs,indices,assignments in parts:
        positions=attrs[(1,0)];weights=[[] for _ in positions]
        for v,j,w in assignments:
            if v>=len(weights) or j>=len(bones) or not math.isfinite(w) or w<0:raise ValueError('Invalid bone assignment')
            if w>0:weights[v].append((j,w))
        joints=np.zeros((len(weights),8),dtype=int);values=np.zeros((len(weights),8))
        for v,entry in enumerate(weights):
            max_influences=max(max_influences,len(entry))
            if not entry or len(entry)>8:raise ValueError('Character requires unsupported bone influence count')
            total=sum(w for _,w in entry)
            for k,(j,w) in enumerate(entry):joints[v,k]=j;values[v,k]=w/total
        attributes={'POSITION':scene.accessor(positions.ravel().tolist(),3,'VEC3',bounds=True),
            'NORMAL':scene.accessor(attrs[(4,0)].ravel().tolist(),3,'VEC3'),
            'TEXCOORD_0':scene.accessor(attrs[(7,0)].ravel().tolist(),2,'VEC2'),
            'WEIGHTS_0':scene.accessor(values[:,:4].ravel().tolist(),4,'VEC4'),
            'WEIGHTS_1':scene.accessor(values[:,4:].ravel().tolist(),4,'VEC4')}
        # glTF joints must use unsigned bytes or shorts, never uint32.
        for channel in range(2):
            while len(scene.binary)%4:scene.binary.append(0)
            offset=len(scene.binary);scene.binary.extend(joints[:,channel*4:channel*4+4].astype('<u2').tobytes())
            scene.doc['bufferViews'].append({'buffer':0,'byteOffset':offset,'byteLength':len(scene.binary)-offset})
            attributes['JOINTS_'+str(channel)]=len(scene.doc['accessors'])
            scene.doc['accessors'].append({'bufferView':len(scene.doc['bufferViews'])-1,'componentType':5123,'count':len(joints),'type':'VEC4'})
        primitives.append({'attributes':attributes,'indices':scene.accessor(indices.tolist(),1,'SCALAR',5125),'material':0})
    scene.doc['meshes']=[{'name':name,'primitives':primitives}]
    points=np.concatenate([part[1][(1,0)] for part in parts]);lo=points.min(0);hi=points.max(0)
    # Explicit visual height, independent of authoritative capsules/hit volumes.
    height=1.8;scale=height/float(hi[1]-lo[1])
    scene.doc['nodes'][0]['scale']=[scale]*3
    # Shadow's source head points toward -Y; normalize anatomy as well as origin.
    rotation=[1,0,0,0] if name=='shadow' else [0,0,0,1]
    rotated=points@matrix([0,0,0],rotation,[1,1,1])[:3,:3].T
    lower,upper=rotated.min(0),rotated.max(0)
    scene.doc['nodes'][0]['rotation']=rotation
    scene.doc['nodes'][0]['translation']=[-float((lower[0]+upper[0])/2)*scale,-float(lower[1])*scale,-float((lower[2]+upper[2])/2)*scale]
    clips=export_animations(scene,skeleton,bones)
    scene.doc['extras']={'gloom':{'animationInventory':clips,'animationStatus':'original keys exported; Shadow movement authored procedurally',
        'anchors':{'rightHand':'Bip001 R Hand','leftHand':'Bip001 L Hand'}}}
    scene.save()
    return {'name':name,'bones':len(bones),'triangles':sum(len(p[2])//3 for p in parts),
        'bounds':[lo.tolist(),hi.tolist()],'visual_height':height,'scale':scale,'max_influences':max_influences,
        'clips':clips,'materials':material_audit,'root_rebase':root_rebase,
        'bind_error':float(max(np.max(np.abs(world(i)@np.linalg.inv(world(i))-np.eye(4))) for i in range(len(bones))))}


def convert_weapon(imp):
    source=imp.legacy/'Exes/media/models/weapons/soulReaper.mesh'
    imp.mesh(source)  # Normalize on a copy; recover UV directly, without Assimp's V flip.
    parts=read_mesh(imp.work/(f'mesh{imp.mesh_number-1}.mesh'))
    scene=Scene(imp,'soul_reaper')
    primitives=[]
    for material,attrs,indices,assignments in parts:
        if assignments:raise ValueError('Unexpected weapon rig')
        primitives.append({'attributes':{
            'POSITION':scene.accessor(attrs[(1,0)].ravel().tolist(),3,'VEC3',bounds=True),
            'NORMAL':scene.accessor(attrs[(4,0)].ravel().tolist(),3,'VEC3'),
            'TEXCOORD_0':scene.accessor(attrs[(7,0)].ravel().tolist(),2,'VEC2')},
            'indices':scene.accessor(indices.tolist(),1,'SCALAR',5125),'material':scene.material(material)})
    scene.doc['meshes']=[{'name':'Soul Reaper','primitives':primitives}]
    scene.instance('Soul Reaper',0)
    scene.save()


def run(args):
    legacy,output,work=(p.resolve() for p in (args.legacy_root,args.output_root,args.work_root))
    for target in (output,work):
        if target==legacy or legacy in target.parents or target in legacy.parents:raise ValueError('Keep legacy read-only')
    imp=Importer(legacy,output,work)
    result=[convert(imp,name) for name in ('archangel','shadow')]
    convert_weapon(imp)
    (output/'characters_manifest.json').write_text(json.dumps({'version':1,'characters':result,
        'tool_versions':{name:importlib.metadata.version(name) for name in ('ogre-python','assimp-py','Pillow','numpy')},
        'sources':dict(sorted(imp.sources.items()))},indent=2),encoding='utf8')
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('legacy-root','output-root','work-root'):parser.add_argument('--'+name,type=Path,required=True)
    run(parser.parse_args())
