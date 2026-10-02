"""Read actual cooker v5 buffers and reconstruct diagnostic glTFs of its active LODs/FPS."""
import argparse
import copy
import json
import struct
from pathlib import Path
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT/'.cache/hound-production-v17'
VERTEX = np.dtype([('position','<f4',3),('normal','<f4',3),('uv','<f4',2),('tangent','<f4',4),
                   ('uv1','<f4',2),('joints','<u2',8),('weights','<f4',8)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', default='assets/characters/hound_rig/v17/hound-rig.gltf')
    parser.add_argument('--cooked', default='.cache/hound-production-v17/cooked/hound.gasset')
    parser.add_argument('--probe', action='store_true')
    args = parser.parse_args()
    path = ROOT/args.source
    source = json.loads(path.read_text(encoding='utf-8'))
    raw = (ROOT/args.cooked).read_bytes()
    data = raw[52+8*struct.unpack_from('<I', raw, 40)[0]:]
    assert data[:8] == b'GLOOMSCN' and VERTEX.itemsize == 104
    version, default, *counts = struct.unpack_from('<11I', data, 8)
    assert version == 5 and counts[0] == 7 and counts[7] == 1
    joints = [source['nodes'][i]['name'] for i in source['skins'][0]['joints']]
    arm = np.array([any(s in n for s in ('UpperArm','Forearm','Hand','Finger')) for n in joints])
    rows, primitives, offset = [], [], 52
    for primitive in range(counts[0]):
        material, nv, ni = struct.unpack_from('<3I', data, offset)
        offset += 12
        vertices = np.frombuffer(data, VERTEX, nv, offset)
        offset += nv*104
        indices = [np.frombuffer(data, '<u4', ni, offset).reshape(-1,3)]
        offset += ni*4
        x, y, z, radius, nlods = struct.unpack_from('<4fI', data, offset)
        offset += 20
        assert nlods == 2
        for lod in range(nlods):
            count = struct.unpack_from('<I', data, offset)[0]
            offset += 4
            indices.append(np.frombuffer(data, '<u4', count, offset).reshape(-1,3))
            offset += count*4
        assert np.isfinite(vertices['position']).all() and np.isfinite(vertices['normal']).all()
        assert np.allclose(np.linalg.norm(vertices['normal'], axis=1), 1, atol=1e-4)
        assert np.allclose(vertices['weights'].sum(axis=1), 1, atol=2e-6)
        assert (vertices['weights']>0).sum(axis=1).max() <= 2
        arms = (arm[vertices['joints']]*vertices['weights']).sum(axis=1)
        indices.append(indices[0][(arms[indices[0]]>.95).all(axis=1)])
        degenerate = []
        for lod, triangles in enumerate(indices):
            p = vertices['position'][triangles]
            degenerate.append(int((np.linalg.norm(np.cross(p[:,1]-p[:,0], p[:,2]-p[:,0]),axis=1)<=1e-12).sum()))
        assert not any(degenerate), (material, degenerate)
        row = {'material':source['materials'][material]['name'], 'vertices_cooked':nv,
               'triangles':[len(i) for i in indices], 'zero_area_triangles':degenerate, 'radius_m':radius, 'center':[x,y,z],
               'transition_distances_m':[30*radius,80*radius],
               'geometry_bytes':nv*104*(3+bool(len(indices[3])))+sum(i.size*4 for i in indices)}
        rows.append(row)
        primitives.append((material, vertices, indices))
    totals = {'vertices_cooked':sum(r['vertices_cooked'] for r in rows),
              'triangles':[sum(r['triangles'][i] for r in rows) for i in range(4)],
              'geometry_bytes':sum(r['geometry_bytes'] for r in rows),
              'primitives':[sum(r['triangles'][i]>0 for r in rows) for i in range(4)]}
    assert totals['triangles'][0]<=40000 and totals['triangles'][3]<=18000
    assert totals['vertices_cooked']<=54000 and totals['geometry_bytes']<=24*1048576
    assert totals['primitives']==[7,7,7,4]
    result = {'source':args.source,'cooked':args.cooked,'materials':rows,'totals':totals}
    (WORK/('probe-cooked.json' if args.probe else 'cooked.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2),flush=True)
    if args.probe:
        return
    for lod in range(4):
        scene = copy.deepcopy(source)
        binary = bytearray((path.parent/source['buffers'][0]['uri']).read_bytes())
        meshes = []

        def append(array, kind, component):
            array = np.ascontiguousarray(array)
            binary.extend(b'\0'*(-len(binary)%4))
            scene['bufferViews'].append({'buffer':0,'byteOffset':len(binary),'byteLength':array.nbytes})
            binary.extend(array.tobytes())
            scene['accessors'].append({'bufferView':len(scene['bufferViews'])-1,'componentType':component,'count':len(array),'type':kind})
            if kind == 'VEC3' and component == 5126:
                scene['accessors'][-1].update(min=array.min(axis=0).tolist(),max=array.max(axis=0).tolist())
            return len(scene['accessors'])-1

        for material, vertices, indices in primitives:
            if not len(indices[lod]):
                continue
            attributes = {key:append(vertices[field], kind, component) for key,field,kind,component in (
                ('POSITION','position','VEC3',5126),('NORMAL','normal','VEC3',5126),('TEXCOORD_0','uv','VEC2',5126))}
            attributes['JOINTS_0'] = append(vertices['joints'][:,:4], 'VEC4', 5123)
            attributes['WEIGHTS_0'] = append(vertices['weights'][:,:4], 'VEC4', 5126)
            assert not vertices['weights'][:,4:].any()
            meshes.append({'attributes':attributes,'indices':append(indices[lod].ravel(),'SCALAR',5125),'material':material})
        scene['meshes'][0]['primitives'] = meshes
        name = f'lod{lod}' if lod<3 else 'fps'
        scene['buffers'] = [{'uri':name+'.bin','byteLength':len(binary)}]
        (WORK/(name+'.bin')).write_bytes(binary)
        (WORK/(name+'.gltf')).write_text(json.dumps(scene),encoding='utf-8')


if __name__ == '__main__':
    main()
