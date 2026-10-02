"""H07: reduce rigid surfaces and waist circumference; retain joint rings and all 106 parts.

Run in Blender on v16. Draft is replaceable; --final refuses published paths.
No hidden faces are removed. Production UVs, bakes and rig work belong to later tasks.
"""
import bpy
import bmesh
import ctypes
import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from hound_h04_review import parts
from hound_h01_review import setup, camera

ROOT = Path(__file__).resolve().parents[2]
(ROOT/'.cache/hound-production-v17').mkdir(parents=True, exist_ok=True)
optimizer = ctypes.CDLL(str(ROOT/'build/windows-vs/vcpkg_installed/x64-windows/bin/meshoptimizer.dll'))
optimizer.meshopt_simplify.restype = ctypes.c_size_t
optimizer.meshopt_simplify.argtypes = [ctypes.POINTER(ctypes.c_uint), ctypes.POINTER(ctypes.c_uint), ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_float), ctypes.c_size_t, ctypes.c_size_t, ctypes.c_size_t, ctypes.c_float, ctypes.c_uint,
    ctypes.POINTER(ctypes.c_float)]
for item in json.loads((ROOT/'art/characters/hound/v16/sculpture-reference.json').read_text())['files']:
    assert hashlib.sha256((ROOT/item['path']).read_bytes()).hexdigest() == item['sha256']
source = bpy.data.scenes['Hound_Mesh_v16']
bpy.context.window.scene = source
source.frame_set(1)
original, old_rig = source.objects['H16_DeformMesh'], source.objects['Hound16_Rig']
groups = parts(original)
rigid = json.loads(source['gloom_rigid_parts'])
scene = bpy.data.scenes.new('Hound_Mesh_v17')
for key in source.keys():
    scene[key] = source[key]
scene.unit_settings.system, scene.unit_settings.scale_length = 'METRIC', 1
scene.render.fps, scene.frame_start, scene.frame_end = 30, 1, 181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name, frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v17_EXPORT')
scene.collection.children.link(collection)
rig = old_rig.copy()
rig.data = old_rig.data.copy()
rig.name, rig.data.name = 'Hound17_Rig', 'Hound17_Skeleton'
rig.animation_data.action = old_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound17_joint_check'
collection.objects.link(rig)
bpy.context.window.scene = scene
generated, inventory = [], []
for name, ids in groups.items():
    obj = original.copy()
    obj.data = original.data.copy()
    obj.modifiers.clear()
    obj.parent = None
    collection.objects.link(obj)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.verts.ensure_lookup_table()
    keep = set(ids)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.index not in keep], context='VERTS')
    bm.to_mesh(obj.data)
    bm.free()
    before = sum(len(p.vertices)-2 for p in obj.data.polygons)
    # Keep small silhouette blades and the complete deformable quad cages.
    ratio = 1 if name not in rigid or before < 170 else .10 if name == 'Head_surface' else .50 if name.startswith(('Finger', 'Thumb', 'Hand_')) else .20
    if name.startswith('Collar_blade_middle_'):
        ratio = .99  # Triangulate/clean the five zero-area triangles inherited from the v16 polygons.
    tolerance = (.001 if name.startswith(('Back_spine_', 'Greave_front_')) else .003 if name.startswith('Greave_mass_')
                 else .02 if name.startswith(('Finger', 'Thumb', 'Eye_')) else .006)
    if name == 'Waist_wrap':
        # Retain all 13 height/weight rings and the 3 mm lining; sample 24 of 48 angular columns.
        assert len(ids) == 2*13*48
        retained = [i for i in range(len(ids)) if i%48%2 == 0]
        remap = {i: j for j,i in enumerate(retained)}
        faces = []
        for face in obj.data.polygons:
            vertices = list(dict.fromkeys(remap[i-i%2] for i in face.vertices))
            if len(vertices) >= 3:
                faces.append(vertices)
        data = bpy.data.meshes.new('H17_Waist_wrap')
        data.from_pydata([obj.data.vertices[i].co for i in retained], [], faces)
        for material in original.data.materials:
            data.materials.append(material)
        material = obj.data.polygons[0].material_index
        for face in data.polygons:
            face.material_index, face.use_smooth = material, True
        data.normals_split_custom_set_from_vertices([obj.data.vertices[i].normal for i in retained])
        obj.data = data
        for group in original.vertex_groups:
            obj.vertex_groups.new(name=group.name)
        for i, old_index in enumerate(retained):
            for weight in original.data.vertices[ids[old_index]].groups:
                obj.vertex_groups[weight.group].add([i], weight.weight, 'REPLACE')
        ratio = .5
    elif ratio < 1:
        obj.data.calc_loop_triangles()
        faces, normals, materials, used = [], [], [], {}
        coordinates = []
        packed, corners, indices = {}, [], []
        for triangle in obj.data.loop_triangles:
            for loop in triangle.loops:
                key = (obj.data.loops[loop].vertex_index, tuple(obj.data.corner_normals[loop].vector), triangle.material_index)
                if key not in packed:
                    packed[key] = len(corners)
                    corners.append(key)
                indices.append(packed[key])
        positions = (ctypes.c_float*(3*len(corners)))(*(c for i,n,m in corners for c in obj.data.vertices[i].co))
        source_indices = (ctypes.c_uint*len(indices))(*indices)
        reduced = (ctypes.c_uint*len(indices))()
        error = ctypes.c_float()
        # Same installed simplifier as the cooker; material/normal splits remain paired seams.
        count = optimizer.meshopt_simplify(reduced, source_indices, len(indices), positions, len(corners), 12,
            max(3, int(len(indices)*ratio)//3*3), tolerance, 0, ctypes.byref(error))
        for start in range(0, count, 3):
            face = []
            for j in range(start, start+3):
                i, normal, material = corners[reduced[j]]
                if i not in used:
                    used[i] = len(coordinates)
                    coordinates.append(obj.data.vertices[i].co.copy())
                face.append(used[i])
                normals.append(normal)
            faces.append(face)
            assert len({corners[reduced[j]][2] for j in range(start, start+3)}) == 1
            materials.append(material)
        data = bpy.data.meshes.new('H17_'+name)
        data.from_pydata(coordinates, [], faces)
        for material in original.data.materials:
            data.materials.append(material)
        for face, material in zip(data.polygons, materials):
            face.material_index, face.use_smooth = material, True
        bm = bmesh.new()
        bm.from_mesh(data)
        layer = bm.loops.layers.float_vector.new('H07_reference_normal')
        cursor = 0
        for face in bm.faces:
            for loop in face.loops:
                loop[layer] = normals[cursor]
                cursor += 1
        for cleanup in range(3):
            bmesh.ops.dissolve_degenerate(bm, dist=1e-6, edges=list(bm.edges))
            bmesh.ops.triangulate(bm, faces=list(bm.faces), quad_method='BEAUTY', ngon_method='BEAUTY')
            if min(face.calc_area() for face in bm.faces) > 1e-12:
                break
        assert min(face.calc_area() for face in bm.faces) > 1e-12, name
        normals = [loop[layer].normalized() for face in bm.faces for loop in face.loops]
        bm.to_mesh(data)
        bm.free()
        data.normals_split_custom_set(normals)
        obj.data = data
        obj.vertex_groups.new(name='PART_'+name).add(list(range(len(data.vertices))), 1, 'REPLACE')
        obj.vertex_groups.new(name=rigid[name]).add(list(range(len(data.vertices))), 1, 'REPLACE')
    obj.data.calc_loop_triangles()
    inventory.append({'part': name, 'bone': rigid.get(name), 'ratio': ratio,
                      'method': 'preserved' if ratio == 1 else '24-angular-columns-13-height-rings' if name == 'Waist_wrap'
                      else 'meshoptimizer-material-normal-seams', 'relative_error': tolerance,
                      'source_vertices': len(ids), 'source_triangles': before,
                      'vertices': len(obj.data.vertices), 'triangles': len(obj.data.loop_triangles)})
    generated.append(obj)
bpy.ops.object.select_all(action='DESELECT')
for obj in generated:
    obj.select_set(True)
bpy.context.view_layer.objects.active = generated[0]
bpy.ops.object.join()
model = generated[0]
model.name, model.data.name = 'H17_DeformMesh', 'H17_ProductionTopology'
model.parent = rig
model.modifiers.new('DiagnosticSkin', 'ARMATURE').object = rig
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound16_', 'Hound17_')
if not model.data.uv_layers:
    uv = model.data.uv_layers.new(name='H07_ProvisionalUV')
    for loop in model.data.loops:
        p = model.data.vertices[loop.vertex_index].co
        uv.data[loop.index].uv = (p.x*.5+.5, p.z*.5)
scene['gloom_authoring_stage'] = 'H07-production-mesh-provisional-weights-and-UVs'
scene['gloom_revision'] = 'Rigid reduction; 10 deformable cages exact; waist retains 13 height rings with 24 angular columns'
scene['gloom_h07_inventory'] = json.dumps(inventory)
model.data.calc_loop_triangles()
(ROOT/'.cache/hound-production-v17/inventory-draft.json').write_text(json.dumps(inventory, indent=2))
print('H07_COUNT', len(model.data.loop_triangles), flush=True)
assert len(model.data.loop_triangles) <= 40000
assert len(parts(model)) == 106
scene.frame_set(1)
setup(scene, 1000, 1100)
camera(scene, (2.8, -5, 2.2), (0, 0, .91), 2.1)
folder = ROOT/('.cache/hound-production-v17' if '--final' not in sys.argv else 'art/characters/hound/v17')
export = ROOT/('.cache/hound-production-v17/export/hound-rig.gltf' if '--final' not in sys.argv else 'assets/characters/hound_rig/v17/hound-rig.gltf')
path = folder/'hound-production-v17.blend'
if '--final' in sys.argv:
    assert not folder.exists() and not export.parent.exists(), 'Never overwrite a published production mesh'
folder.mkdir(parents=True, exist_ok=True)
export.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
rig.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.wm.save_as_mainfile(filepath=str(path), check_existing=False, compress=True)
bpy.ops.export_scene.gltf(filepath=str(export), export_format='GLTF_SEPARATE', use_selection=True, use_active_scene=True,
    export_yup=True, export_apply=False, export_animations=True, export_animation_mode='ACTIVE_ACTIONS',
    export_nla_strips_merged_animation_name='Hound17_joint_check', export_anim_slide_to_zero=True, export_force_sampling=True,
    export_frame_range=True, export_skins=True, export_def_bones=True, export_rest_position_armature=True,
    export_all_influences=False, export_influence_nb=4, export_extras=True)
manifest = {'task': 'H07', 'milestone': 107, 'source': 'v16', 'version': 'v17', 'scene': scene.name,
    'mesh': model.name, 'rig': rig.name, 'action': rig.animation_data.action.name, 'collection': collection.name,
    'provisional': ['weights', 'diagnostic_action', 'UV0', 'material_factors'], 'parts': inventory,
    'files': [{'path': p.relative_to(ROOT).as_posix(), 'sha256': hashlib.sha256(p.read_bytes()).hexdigest(),
               'bytes': p.stat().st_size} for p in (path, export, export.with_suffix('.bin'))]}
(folder/'production-reference.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
print('H07_BUILT', len(model.data.vertices), len(model.data.loop_triangles), len(inventory))
