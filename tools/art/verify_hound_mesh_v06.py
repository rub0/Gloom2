'''Validate v06-v08 surfaces, preserved components/bones/clip, and sampled diagnostic deformation.'''
import bpy
import bmesh
import json
import math
import sys
from mathutils import Matrix, Quaternion, Vector
from mathutils.bvhtree import BVHTree

version = sys.argv[sys.argv.index('--')+1] if '--' in sys.argv else 'v06'
assert version in ('v06', 'v07', 'v08')
suffix, prior = version[1:], {'v06':'05','v07':'06','v08':'07'}[version]
scene = bpy.data.scenes['Hound_Mesh_'+version]
bpy.context.window.scene = scene
model = scene.objects['H'+suffix+'_DeformMesh']
rig = scene.objects['Hound'+suffix+'_Rig']
source = bpy.data.scenes['Hound_Mesh_v'+prior]
original = source.objects['H'+prior+'_DeformMesh']
original_rig = source.objects['Hound'+prior+'_Rig']
model.data.calc_loop_triangles()
assert (len(model.data.vertices), len(model.data.loop_triangles)) == {'v06':(13752,27124),'v07':(14026,27672),'v08':(20130,39880)}[version]
assert len(model.data.materials) == 7 and len(rig.data.bones) == 53
assert len([o for o in scene.objects if o.type == 'MESH']) == 1
assert len(model.modifiers) == 1 and model.modifiers[0].type == 'ARMATURE' and model.modifiers[0].object == rig
assert not model.modifiers[0].use_deform_preserve_volume
assert model.matrix_world == original.matrix_world and rig.matrix_world == original_rig.matrix_world
assert set(rig.data.bones.keys()) == set(original_rig.data.bones.keys())
for bone in rig.data.bones:
    reference = original_rig.data.bones[bone.name]
    assert bone.matrix_local == reference.matrix_local and abs(bone.length-reference.length) < 1e-7
    assert bone.use_deform == reference.use_deform
    assert (bone.parent.name if bone.parent else None) == (reference.parent.name if reference.parent else None)
action, reference_action = rig.animation_data.action, original_rig.animation_data.action
assert len(action.fcurves) == len(reference_action.fcurves)
for curve, reference in zip(action.fcurves, reference_action.fcurves):
    assert curve.data_path == reference.data_path and curve.array_index == reference.array_index
    assert len(curve.keyframe_points) == len(reference.keyframe_points)
    for point, other in zip(curve.keyframe_points, reference.keyframe_points):
        assert point.co == other.co and point.interpolation == other.interpolation


def parts(obj):
    result = {g.name: [] for g in obj.vertex_groups if g.name.startswith('PART_')}
    for vertex in obj.data.vertices:
        for group in vertex.groups:
            name = obj.vertex_groups[group.group].name
            if name in result:
                result[name].append(vertex.index)
    return result


def bone_weights(obj, index):
    return {obj.vertex_groups[g.group].name: g.weight for g in obj.data.vertices[index].groups
            if obj.vertex_groups[g.group].name in rig.data.bones}


source_parts, target_parts = parts(original), parts(model)
replaced = {'PART_'+name for name in ('Head_planes', 'Jaw_plane', 'Nose_plane', 'Cheek_L', 'Cheek_R', 'Brow_L', 'Brow_R',
                                     'Pelvis_cloth', 'Waist_sash', 'Leg_continuous_L', 'Leg_continuous_R')}
added = {'PART_Trousers_continuous', 'PART_Waist_wrap', 'PART_Head_surface'}
modified = {'PART_Collar_plate_L', 'PART_Collar_plate_R'} if version == 'v07' else set()
if version == 'v07':
    replaced = {'PART_Hood_shell', 'PART_Hood_opening_rim', 'PART_Neck'}
    added = {'PART_Hood_continuous', 'PART_Neck_surface'}
if version == 'v08':
    prefixes = ('Breastplate_', 'Rib_flank_', 'Hip_guard_', 'Abdominal_plate_', 'Sternum', 'Scapula_', 'Back_spine_',
                'Bracer_face_', 'Elbow_cuff_', 'Knee_shield_', 'Greave_front_', 'Shin_lower_', 'Thigh_outer_plate_',
                'Boot_toe_', 'Ankle_guard_', 'Hand_back_full_point_', 'Sash_tail_', 'Mouth_shadow', 'Eye_')
    replaced = {name for name in source_parts if name[5:].startswith(prefixes)}
    added = replaced
    modified = {'PART_'+name for name in ('Arm_surface_L','Arm_surface_R','Head_surface','Trousers_continuous','Waist_wrap')}
    assert len(replaced) == 38 and len(source_parts) == len(target_parts) == 96
    # Catch lost transforms in the inactive v02 reference scene, especially the rotated back plates.
    for name in replaced-{'PART_Mouth_shadow','PART_Eye_L','PART_Eye_R'}:
        before = [original.data.vertices[i].co for i in source_parts[name]]
        after = [model.data.vertices[i].co for i in target_parts[name]]
        for axis in range(3):
            # Cloth loses the old 18 mm pyramid ridge; retain the stricter envelope on metal and other axes.
            limit = .018 if name.startswith('PART_Sash_tail_') and axis == 1 else .016
            assert abs(min(p[axis] for p in before)-min(p[axis] for p in after)) < limit, name+' minimum moved'
            assert abs(max(p[axis] for p in before)-max(p[axis] for p in after)) < limit, name+' maximum moved'
assert set(source_parts)-replaced == set(target_parts)-added
preserved = 0
morph_displacements = {}
for name, indices in source_parts.items():
    if name in replaced:
        continue
    other = target_parts[name]
    assert len(indices) == len(other)
    for a, b in zip(indices, other):
        expected = original.data.vertices[a].co.copy()
        if version == 'v07' and name in modified and abs(expected.x) < .185:
            expected.x = (1 if expected.x >= 0 else -1)*(.146+(abs(expected.x)-.073)*(.185-.146)/(.185-.073))
        distance = (expected-model.data.vertices[b].co).length
        if version == 'v08' and name in modified:
            assert distance < .013, 'Art relief exceeds the approved form envelope'
            morph_displacements[name] = max(morph_displacements.get(name,0),distance)
        else:
            assert distance < 1e-7
        assert bone_weights(original, a) == bone_weights(model, b)
    source_remap, target_remap = {v: i for i, v in enumerate(indices)}, {v: i for i, v in enumerate(other)}
    before = [(tuple(source_remap[i] for i in p.vertices), p.material_index, p.use_smooth)
              for p in original.data.polygons if all(i in source_remap for i in p.vertices)]
    after = [(tuple(target_remap[i] for i in p.vertices), p.material_index, p.use_smooth)
             for p in model.data.polygons if all(i in target_remap for i in p.vertices)]
    assert before == after, 'Changed preserved faces: '+name
    preserved += name not in modified
assert preserved == {'v06':94,'v07':92,'v08':53}[version]
assert all(distance > .0005 for distance in morph_displacements.values())
assert all(tuple(a.diffuse_color) == tuple(b.diffuse_color) for a, b in zip(original.data.materials, model.data.materials))
surface_metrics = {}
surfaces = [('PART_Trousers_continuous', 1163, 2), ('PART_Waist_wrap', 1248, 0), ('PART_Head_surface', 2368, 2)] if version == 'v06' else [
    ('PART_Hood_continuous', 1106, 2), ('PART_Neck_surface', 320, 2)]
if version == 'v08':
    surfaces += [(name,len(target_parts[name]),0 if name == 'PART_Waist_wrap' else 2) for name in sorted(replaced|modified)]
for name, count, euler in surfaces:
    indices = set(target_parts[name])
    assert len(indices) == count
    faces = [p for p in model.data.polygons if all(i in indices for i in p.vertices)]
    assert all(p.area > 1e-10 for p in faces)
    bm = bmesh.new()
    vertices = {i: bm.verts.new(model.data.vertices[i].co) for i in indices}
    for polygon in faces:
        bm.faces.new([vertices[i] for i in polygon.vertices])
    assert all(e.is_manifold and e.is_contiguous for e in bm.edges)
    assert all(v.is_manifold for v in bm.verts)
    assert len(bm.verts)-len(bm.edges)+len(bm.faces) == euler
    volume = bm.calc_volume(signed=True)
    assert volume > 0, name+' has inverted normals'
    visited, pending = set(), [next(iter(bm.verts))]
    while pending:
        vertex = pending.pop()
        if vertex in visited:
            continue
        visited.add(vertex)
        pending.extend(e.other_vert(vertex) for e in vertex.link_edges if e.other_vert(vertex) not in visited)
    assert len(visited) == len(indices), 'Disconnected surface: '+name
    bm.free()
    if version in ('v07','v08'):
        tree = BVHTree.FromPolygons([v.co for v in model.data.vertices], [list(p.vertices) for p in faces])
        assert not [(a, b) for a, b in tree.overlap(tree) if a < b and not set(faces[a].vertices).intersection(faces[b].vertices)], name
    surface_metrics[name[5:]] = {'vertices': len(indices), 'faces': len(faces), 'euler': euler, 'volume_m3': volume}
contacts = {}
if version in ('v07','v08'):
    opening = [(0,1.722),(.041,1.695),(.065,1.653),(.075,1.579),(.085,1.488),(.050,1.446),
               (0,1.49),(-.050,1.446),(-.085,1.488),(-.075,1.579),(-.065,1.653),(-.041,1.695)]
    outline = [(0,1.80),(.066,1.765),(.127,1.671),(.14,1.57),(.128,1.476),(.071,1.427),
               (0,1.474),(-.071,1.427),(-.128,1.476),(-.14,1.57),(-.127,1.671),(-.066,1.765)]
    for name, profile, y in [('opening', opening, -.194), ('outer_front', outline, -.170)]:
        group = model.vertex_groups['LANDMARK_Hood_'+name].index
        points = [v.co for v in model.data.vertices if any(g.group == group for g in v.groups)]
        expected = [Vector((x, y, z)).lerp(Vector((profile[(i+1)%12][0], y, profile[(i+1)%12][1])), step/4)
                    for i, (x, z) in enumerate(profile) for step in range(4)]
        assert len(points) == 48 and max((a-b).length for a, b in zip(points, expected)) < 1e-7
    previous_hood = ['PART_Hood_shell','PART_Hood_opening_rim'] if version == 'v07' else ['PART_Hood_continuous']
    previous_neck = 'PART_Neck' if version == 'v07' else 'PART_Neck_surface'
    for obj, components, hood_parts, neck_part in [(original, source_parts, previous_hood, previous_neck),
                                                  (model, target_parts, ['PART_Hood_continuous'], 'PART_Neck_surface')]:
        trees = {}
        for name in hood_parts+[neck_part]+[n for n in components if n.startswith('PART_Collar_')]:
            indices = set(components[name])
            trees[name] = BVHTree.FromPolygons([v.co for v in obj.data.vertices],
                                             [list(p.vertices) for p in obj.data.polygons if all(i in indices for i in p.vertices)])
        contacts[obj.name] = {name[5:]: {'hood_face_pairs': sum(len(trees[name].overlap(trees[h])) for h in hood_parts),
                                        'neck_face_pairs': len(trees[name].overlap(trees[neck_part]))}
                              for name in components if name.startswith('PART_Collar_')}
    print('REST_COLLAR_CONTACTS', json.dumps(contacts))
    assert all(v['hood_face_pairs'] == 0 and v['neck_face_pairs'] == 0 for v in contacts[model.name].values())
# Preserve waist height, keep the neutral head behind the approved hood opening and retain the body scale.
head = [model.data.vertices[i].co for i in target_parts['PART_Head_surface']]
assert max(abs(v.x) for v in head) <= .08401 and min(v.y for v in head) > -.150
assert 1.504 <= min(v.z for v in head) and max(v.z for v in head) <= 1.74001
max_influences = 0
for vertex in model.data.vertices:
    weights = list(bone_weights(model, vertex.index).values())
    assert weights and abs(sum(weights)-1) < 2e-6 and all(0 < w <= 1 for w in weights)
    max_influences = max(max_influences, len(weights))
assert max_influences <= 4
rigid_parts = json.loads(scene['gloom_rigid_parts'])
assert len(rigid_parts) == 87 and rigid_parts['Head_surface'] == 'Bip001 Head'
for part, bone in rigid_parts.items():
    for index in target_parts['PART_'+part]:
        assert bone_weights(model, index) == {bone: 1.0}
scene.frame_set(1)
bpy.context.view_layer.update()
rest = [v.co.copy() for v in model.data.vertices]
assert abs(min(v.z for v in rest)) < 1e-6 and abs(max(v.z for v in rest)-1.8) < 1e-6
max_displacement, rigid_error = 0, 0
for frame in range(1, 182, 3):
    scene.frame_set(frame)
    bpy.context.view_layer.update()
    evaluated = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    assert len(mesh.vertices) == len(rest)
    for vertex in mesh.vertices:
        assert all(math.isfinite(c) for c in vertex.co) and vertex.co.length < 3
        distance = (vertex.co-rest[vertex.index]).length
        max_displacement = max(max_displacement, distance)
        if frame in (1, 181):
            assert distance < 1e-5, 'Rest pose drift'
    assert all(p.area > 1e-12 for p in mesh.polygons), 'Collapsed deformed face'
    for part in rigid_parts:
        indices = target_parts['PART_'+part]
        origin = indices[0]
        for index in indices[1:]:
            error = abs((mesh.vertices[index].co-mesh.vertices[origin].co).length-(rest[index]-rest[origin]).length)
            rigid_error = max(rigid_error, error)
    evaluated.to_mesh_clear()
assert .25 < max_displacement < 1.2 and rigid_error < 2e-6
# Stress only in memory; do not change the diagnostic clip or claim anatomically valid gait/cloth collision.
extra_cases = [
    [('Bip001 L Thigh', (1, 0, 0), -60), ('Bip001 L Calf', (1, 0, 0), 85)],
    [('Bip001 R Thigh', (1, 0, 0), -60), ('Bip001 R Calf', (1, 0, 0), 85)],
    [('Bip001 L Thigh', (0, 1, 0), -25), ('Bip001 R Thigh', (0, 1, 0), 25)],
    [('Bip001 Spine', (1, 0, 0), 20), ('Bip001 Spine1', (0, 0, 1), 20), ('Bip001 Head', (0, 0, 1), 35)]]
if version in ('v07','v08'):
    extra_cases = [[('Bip001 Head', (0, 0, 1), 35)], [('Bip001 Head', (0, 0, 1), -35)],
                   [('Bip001 Neck', (1, 0, 0), 10), ('Bip001 Head', (1, 0, 0), 20)],
                   [('Bip001 Neck', (1, 0, 0), -10), ('Bip001 Head', (1, 0, 0), -20)]]
camera_matrix, camera_scale = scene.camera.matrix_world.copy(), scene.camera.data.ortho_scale
if version in ('v07','v08') and '--render-stress' in sys.argv:
    scene.camera.location = (1.5, -4, 1.95)
    scene.camera.rotation_euler = (Vector((0, 0, 1.50))-scene.camera.location).to_track_quat('-Z', 'Y').to_euler()
    scene.camera.data.ortho_scale = 1.22
min_area = 1
stress_contacts = []
saved_action = rig.animation_data.action
rig.animation_data.action = None
for case, pose in enumerate(extra_cases):
    for bone in rig.pose.bones:
        bone.matrix_basis = Matrix.Identity(4)
    for name, axis, degrees in pose:
        rest_rotation = rig.data.bones[name].matrix_local.to_quaternion()
        rig.pose.bones[name].rotation_quaternion = rest_rotation.inverted()@Quaternion(axis, math.radians(degrees))@rest_rotation
    bpy.context.view_layer.update()
    evaluated = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    assert all(p.area > 1e-12 for p in mesh.polygons)
    assert all(all(math.isfinite(c) for c in v.co) and v.co.length < 3 for v in mesh.vertices)
    tested = target_parts['PART_Hood_continuous' if version != 'v06' else 'PART_Trousers_continuous' if case < 3 else 'PART_Head_surface']
    assert max((mesh.vertices[i].co-rest[i]).length for i in tested) > .025, 'Stress pose did not actually deform the target'
    min_area = min(min_area, min(p.area for p in mesh.polygons))
    if version in ('v07','v08'):
        hood_indices = set(target_parts['PART_Hood_continuous'])
        collar_indices = set().union(*(set(indices) for name, indices in target_parts.items() if name.startswith('PART_Collar_')))
        hood_faces = [list(p.vertices) for p in mesh.polygons if all(i in hood_indices for i in p.vertices)]
        collar_faces = [list(p.vertices) for p in mesh.polygons if all(i in collar_indices for i in p.vertices)]
        hood_tree = BVHTree.FromPolygons([v.co for v in mesh.vertices], hood_faces)
        collar_tree = BVHTree.FromPolygons([v.co for v in mesh.vertices], collar_faces)
        # ponytail: diagnostic surface-pair counts only; production needs contact depth/clearance and the full motion set.
        stress_contacts.append({'pose': case, 'hood_collar_face_pairs': len(hood_tree.overlap(collar_tree)),
                                'hood_nonadjacent_self_pairs': sum(a < b and not set(hood_faces[a]).intersection(hood_faces[b])
                                                                 for a, b in hood_tree.overlap(hood_tree))})
        assert stress_contacts[-1]['hood_collar_face_pairs'] == 0 and stress_contacts[-1]['hood_nonadjacent_self_pairs'] == 0
    evaluated.to_mesh_clear()
    if '--render-stress' in sys.argv:
        scene.render.filepath = 'D:/Projects/Gloom/.cache/hound-mesh-'+version+'/stress-'+str(case)+'.png'
        bpy.ops.render.render(write_still=True)
rig.animation_data.action = saved_action
scene.camera.matrix_world, scene.camera.data.ortho_scale = camera_matrix, camera_scale
scene.frame_set(1)
bpy.context.view_layer.update()
print(json.dumps({'vertices': len(rest), 'triangles': len(model.data.loop_triangles), 'preserved_components': preserved,
                  'modified_collar_bases': len(modified) if version == 'v07' else 0, 'rest_collar_contacts': contacts,
                  'art_morph_displacements_m': morph_displacements,
                  'surfaces': surface_metrics, 'unchanged_bones': 53, 'unchanged_diagnostic_clip': True,
                  'max_influences': max_influences, 'rigid_components': len(rigid_parts), 'sampled_frames': 61,
                  'extra_pose_cases': len(extra_cases), 'extra_pose_min_face_area_m2': min_area,
                  'extra_pose_contact_diagnostics': stress_contacts,
                  'max_displacement_m': max_displacement, 'max_rigid_distance_error_m': rigid_error}, indent=2))
