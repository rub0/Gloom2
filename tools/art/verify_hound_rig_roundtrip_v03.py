'''Load the exported glTF into a separate Blender scene; compare skinned poses to the source.'''
import bpy
import json
import sys
from pathlib import Path
from mathutils.kdtree import KDTree

root = Path(__file__).resolve().parents[2]
version = sys.argv[sys.argv.index('--')+1] if '--' in sys.argv else 'v03'
assert version in ('v03', 'v04', 'v05', 'v06', 'v07', 'v08', 'v09', 'v10', 'v11', 'v12', 'v14')
prefix = 'Hound'+version[1:]
mesh_prefix = 'H'+version[1:]
path = root/('assets/characters/hound_rig/'+version+'/hound-rig.gltf')
if '--gltf' in sys.argv:
    path = root/sys.argv[sys.argv.index('--gltf')+1]
gltf = json.loads(path.read_text(encoding='utf-8'))
assert len(gltf['meshes']) == 1 and len(gltf['meshes'][0]['primitives']) == 7
assert len(gltf['skins']) == 1 and len(gltf['skins'][0]['joints']) == 53
assert len(gltf['animations']) == 1
assert gltf['animations'][0]['name'] == prefix+'_joint_check'
assert not gltf.get('images') and not gltf.get('textures')
assert all(s.get('interpolation', 'LINEAR') in ('LINEAR', 'STEP') for s in gltf['animations'][0]['samplers'])
assert not any('matrix' in gltf['nodes'][c['target']['node']] for c in gltf['animations'][0]['channels'])
assert all('JOINTS_0' in p['attributes'] and 'WEIGHTS_0' in p['attributes'] for p in gltf['meshes'][0]['primitives'])
assert all('JOINTS_1' not in p['attributes'] for p in gltf['meshes'][0]['primitives'])
legacy = json.loads((root/'assets/characters/original/archangel.gltf').read_text(encoding='utf-8'))
legacy_names = {legacy['nodes'][i]['name'] for i in legacy['skins'][0]['joints']}
export_names = {gltf['nodes'][i]['name'] for i in gltf['skins'][0]['joints']}
assert len(legacy_names) == 43 and legacy_names <= export_names
assert all(node.get('name', '').startswith(('Bip001', 'Hound ', prefix+'_', mesh_prefix+'_')) for node in gltf['nodes'])
for buffer in gltf['buffers']:
    assert (path.parent/buffer['uri']).stat().st_size == buffer['byteLength']
source = bpy.data.scenes['Hound_Rig_v03' if version == 'v03' else 'Hound_Mesh_'+version]
source_rig = source.objects[prefix+'_Rig']
source_mesh = source.objects[mesh_prefix+'_DeformMesh']
assert source_rig.animation_data.action.name == prefix+'_joint_check'
roundtrip = bpy.data.scenes.new(prefix+'_RoundtripValidation')
roundtrip.render.fps = 30
bpy.context.window.scene = roundtrip
bpy.ops.import_scene.gltf(filepath=str(path))
imported_rig = next(o for o in roundtrip.objects if o.type == 'ARMATURE')
assert len(imported_rig.data.bones) == 53


def evaluated_points(scene, frame):
    bpy.context.window.scene = scene
    scene.frame_set(frame)
    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    points = []
    for obj in scene.objects:
        if obj.type != 'MESH':
            continue
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        points.extend(evaluated.matrix_world@v.co for v in mesh.vertices)
        evaluated.to_mesh_clear()
    return points


worst = 0
frame_errors = {}
for frame in (1, 16, 31, 46, 61, 76, 91, 106, 121, 136, 151, 166, 181):
    original = evaluated_points(source, frame)
    imported = evaluated_points(roundtrip, frame-1)
    # glTF splits vertices at normals/UV/material boundaries; compare surfaces, not vertex indices.
    error = 0
    for a, b in ((original, imported), (imported, original)):
        tree = KDTree(len(b))
        for i, point in enumerate(b):
            tree.insert(point, i)
        tree.balance()
        error = max(error, max(tree.find(point)[2] for point in a))
    frame_errors[frame] = error
    worst = max(worst, error)
assert worst < .0001, 'glTF skinned pose does not match source: '+str(frame_errors)
bpy.context.window.scene = source
source.frame_set(1)
print(json.dumps({'legacy_names_preserved': len(legacy_names), 'export_bones': len(export_names),
                  'meshes': 1, 'material_primitives': 7, 'clips': 1,
                  'roundtrip_frame_errors_m': frame_errors, 'worst_roundtrip_error_m': worst}, indent=2))
