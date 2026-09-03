"""Deterministic static Factory IBL: texture ray sampling and GGX mip filtering.

Consumes converted glTF; never launches the legacy game. The single probe is a
global approximation of the static World and lava, not dynamic mirror capture.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
from PIL import Image

def directions(size):
    uv=(np.arange(size)+.5)/size*2-1
    u,v=np.meshgrid(uv,uv); o=np.ones_like(u)
    a=np.array([np.stack(x,-1) for x in ((o,-v,-u),(-o,-v,u),(u,o,v),(u,-o,-v),(u,-v,o),(-u,-v,-o))])
    return a/np.linalg.norm(a,axis=-1,keepdims=True)

def sample(cube,d):
    major=np.argmax(abs(d),axis=-1); face=major*2+(np.take_along_axis(d,major[...,None],-1)[...,0]<0)
    x,y,z=d[...,0],d[...,1],d[...,2]
    denom=np.max(abs(d),axis=-1)
    us=np.array([-z,z,x,x,x,-x]);vs=np.array([-y,-y,z,-z,-y,-y])
    u=np.take_along_axis(us,face[None],0)[0]/denom
    v=np.take_along_axis(vs,face[None],0)[0]/denom
    size=cube.shape[1]
    return cube[face,np.clip(((v+1)*.5*size).astype(int),0,size-1),np.clip(((u+1)*.5*size).astype(int),0,size-1)]

def bake(root):
    doc=json.loads((root/'factory.gltf').read_text()); data=(root/'factory.bin').read_bytes()
    def access(i):
        a=doc['accessors'][i];v=doc['bufferViews'][a['bufferView']]
        width={'SCALAR':1,'VEC2':2,'VEC3':3}[a['type']]
        return np.frombuffer(data,dtype='<f4' if a['componentType']==5126 else '<u4',count=a['count']*width,offset=v.get('byteOffset',0)+a.get('byteOffset',0)).reshape(-1,width)
    triangles=[];uvs=[];materials=[]
    for node in doc['nodes']:
        if node['name'] not in ('World','lavaFondo'): continue
        for primitive in doc['meshes'][node['mesh']]['primitives']:
            indices=access(primitive['indices']).ravel()
            pos=access(primitive['attributes']['POSITION'])*node['scale']+node['translation']
            triangles.extend(pos[indices].reshape(-1,3,3));uvs.extend(access(primitive['attributes']['TEXCOORD_0'])[indices].reshape(-1,3,2))
            materials.extend([primitive['material']]*(len(indices)//3))
    tri=np.asarray(triangles);uvs=np.asarray(uvs);materials=np.asarray(materials)
    textures={}
    for index,texture in enumerate(doc['textures']):
        pixels=np.array(Image.open(root/doc['images'][texture['source']]['uri']).convert('RGB'),dtype=float)/255
        textures[index]=np.where(pixels<=.04045,pixels/12.92,((pixels+.055)/1.055)**2.4)
    def tex(info,uv):
        if not info: return np.ones(3)
        image=textures[info['index']]; h,w=image.shape[:2]
        return image[int((uv[1]%1)*h)%h,int((uv[0]%1)*w)%w]
    origin=np.array([-10.,6.,-15.]); a=tri[:,0];e1=tri[:,1]-a;e2=tri[:,2]-a
    relative=origin-a;q=np.cross(relative,e1);vn=np.einsum('ij,ij->i',e2,q)
    normal=np.cross(e1,e2);normal/=np.maximum(np.linalg.norm(normal,axis=1,keepdims=True),1e-9)
    light=np.array([.4,.8,-.3]);light/=np.linalg.norm(light)
    size=32; base=[]
    for direction in directions(size).reshape(-1,3):
        h=np.cross(direction,e2);det=np.einsum('ij,ij->i',e1,h)
        inv=np.divide(1,det,out=np.zeros_like(det),where=abs(det)>1e-8)
        u=np.einsum('ij,ij->i',relative,h)*inv;v=q@direction*inv;t=vn*inv
        hits=np.where((abs(det)>1e-8)&(u>=0)&(v>=0)&(u+v<=1)&(t>.01),t,np.inf)
        i=np.argmin(hits)
        if not np.isfinite(hits[i]): base.append([.04,.05,.065]);continue
        uv=uvs[i,0]*(1-u[i]-v[i])+uvs[i,1]*u[i]+uvs[i,2]*v[i]
        material=doc['materials'][materials[i]];pbr=material['pbrMetallicRoughness']
        color=tex(pbr.get('baseColorTexture'),uv)*(.25+.75*max(0,normal[i]@light))
        strength=material.get('extensions',{}).get('KHR_materials_emissive_strength',{}).get('emissiveStrength',1)
        color+=tex(material.get('emissiveTexture'),uv)*np.array(material.get('emissiveFactor',[0,0,0]))*strength
        base.append(color)
    base=np.asarray(base).reshape(6,size,size,3);levels=[base]
    for level in range(1,6):
        n=directions(size>>level).reshape(-1,3);up=np.tile([0.,1.,0.],(len(n),1));up[abs(n[:,1])>.99]=[1,0,0]
        tangent=np.cross(up,n);tangent/=np.linalg.norm(tangent,axis=1,keepdims=True);bitangent=np.cross(n,tangent)
        total=np.zeros_like(n);weight=np.zeros((len(n),1));alpha=(level/5)**2
        for j in range(128):
            phi=2*np.pi*j/128;radical=int(f'{j:07b}'[::-1],2)/128
            cosine=np.sqrt((1-radical)/(1+(alpha*alpha-1)*radical));sine=np.sqrt(1-cosine*cosine)
            h=tangent*(sine*np.cos(phi))+bitangent*(sine*np.sin(phi))+n*cosine
            l=2*np.sum(n*h,axis=1,keepdims=True)*h-n;w=np.maximum(np.sum(n*l,axis=1,keepdims=True),0)
            total+=sample(base,l)*w;weight+=w
        levels.append((total/np.maximum(weight,1e-9)).reshape(6,size>>level,size>>level,3))
    output=bytearray(struct.pack('<4sII',b'GIBL',size,len(levels)))
    for face in range(6):
        for mip in levels:
            rgba=np.concatenate((mip[face],np.ones((*mip[face].shape[:2],1))),axis=-1)
            output.extend(rgba.astype('<f4').tobytes())
    (root/'factory.ibl').write_bytes(output)
    (root/'factory_probe.json').write_text(json.dumps({'position':origin.tolist(),'size':size,'mips':6,'filter':'GGX 128 deterministic Hammersley samples','static_geometry':['World','lavaFondo'],'scene_sha256':hashlib.sha256((root/'factory.gltf').read_bytes()+data).hexdigest()},indent=2))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('root',type=Path);bake(parser.parse_args().root)
