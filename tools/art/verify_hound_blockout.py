'''Validate the static blockout glTF, dimensions and its original-character scale reference.'''
import json
import math
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]

def require(condition, message):
    if not condition:
        raise ValueError(message)

def multiply(a, b):
    return [[sum(a[r][k] * b[k][c] for k in range(4)) for c in range(4)] for r in range(4)]

def transform(node):
    if 'matrix' in node:
        return [[node['matrix'][c*4+r] for c in range(4)] for r in range(4)]
    x,y,z,w = node.get('rotation', [0,0,0,1])
    matrix = [[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),0],
              [2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),0],
              [2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y),0],[0,0,0,1]]
    for r in range(3):
        for c in range(3):
            matrix[r][c] *= node.get('scale',[1,1,1])[c]
        matrix[r][3] = node.get('translation',[0,0,0])[r]
    return matrix

def inspect(path):
    scene = json.loads(path.read_text(encoding='utf-8'))
    buffers = [(path.parent / b['uri']).read_bytes() for b in scene['buffers']]
    for info, data in zip(scene['buffers'], buffers):
        require(len(data) == info['byteLength'], 'Buffer size mismatch')
    transforms = {}
    def visit(index, parent):
        world = multiply(parent, transform(scene['nodes'][index]))
        transforms[index] = world
        for child in scene['nodes'][index].get('children',[]):
            visit(child,world)
    for node in scene['scenes'][scene.get('scene',0)]['nodes']:
        visit(node, [[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]])
    lower, upper = [math.inf]*3, [-math.inf]*3
    triangles = 0
    for index,node in enumerate(scene['nodes']):
        if 'mesh' not in node or index not in transforms:
            continue
        world = transforms[index]
        for primitive in scene['meshes'][node['mesh']]['primitives']:
            require(primitive.get('mode',4) == 4, 'Non-triangle primitive')
            require('NORMAL' in primitive['attributes'], 'Missing normals')
            triangles += scene['accessors'][primitive['indices']]['count']//3
            accessor = scene['accessors'][primitive['attributes']['POSITION']]
            require(accessor['componentType'] == 5126 and accessor['type'] == 'VEC3', 'Expected float positions')
            view = scene['bufferViews'][accessor['bufferView']]
            data = buffers[view['buffer']]
            offset = view.get('byteOffset',0)+accessor.get('byteOffset',0)
            stride = view.get('byteStride',12)
            require(offset+(accessor['count']-1)*stride+12 <= len(data), 'Position accessor out of bounds')
            for vertex in range(accessor['count']):
                local = (*struct.unpack_from('<fff',data,offset+vertex*stride),1)
                position = [sum(world[r][c]*local[c] for c in range(4)) for r in range(3)]
                require(all(math.isfinite(v) for v in position), 'Non-finite geometry')
                for axis in range(3):
                    lower[axis] = min(lower[axis],position[axis])
                    upper[axis] = max(upper[axis],position[axis])
    return scene, lower, upper, triangles

version = sys.argv[1] if len(sys.argv) == 2 else 'v01'
require(len(sys.argv) <= 2 and version in ('v01','v02'), 'Usage: verify_hound_blockout.py [v01|v02]')
expected_parts, expected_triangles = (105,6974) if version == 'v01' else (113,7626)
prefix = 'H'+version[1:]+'_'
scene, lower, upper, triangles = inspect(ROOT / ('assets/characters/hound_blockout/'+version+'/hound-blockout.gltf'))
require(len(scene['meshes']) == expected_parts and len(scene['materials']) == 7, 'Unexpected model composition')
require(not scene.get('skins') and not scene.get('animations'), 'Blockout must not claim a rig or clips')
require(not scene.get('images') and not scene.get('textures'), 'Blockout must use untextured material swatches')
require(triangles == expected_triangles, 'Unexpected triangulation')
require(abs(lower[1]) < 0.001 and abs(upper[1]-1.8) < 0.001, 'Expected grounded 1.8m Y-up character')
names = {node.get('name') for node in scene['nodes']}
require(all(name.startswith(prefix) for name in names), 'Review or reference objects leaked into the export')
require({prefix+name for name in ('Hood_shell','Eye_L','Eye_R','Head_planes')} <= names, 'Identity components missing')
_, reference_lower, reference_upper, _ = inspect(ROOT / 'assets/characters/original/archangel.gltf')
reference_height = reference_upper[1]-reference_lower[1]
require(abs(reference_height-1.8) < 0.001, 'Original-character reference scale changed')
print(json.dumps({'meshes':len(scene['meshes']), 'triangles':triangles, 'materials':len(scene['materials']),
                  'gltf_bounds':[lower,upper], 'dimensions_m':[upper[i]-lower[i] for i in range(3)],
                  'original_archangel_height_m':reference_height, 'rig':False, 'textures':False},indent=2))
