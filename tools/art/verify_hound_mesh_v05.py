'''Validate connected hand topology, preservation of armor/bones/clip, and sampled diagnostic deformation.'''
import bpy
import bmesh
import json
import math
from mathutils import Quaternion

scene = bpy.data.scenes['Hound_Mesh_v05']
bpy.context.window.scene = scene
model = scene.objects['H05_DeformMesh']
rig = scene.objects['Hound05_Rig']
source = bpy.data.scenes['Hound_Mesh_v04']
original = source.objects['H04_DeformMesh']
original_rig = source.objects['Hound04_Rig']
model.data.calc_loop_triangles()
assert len(model.data.vertices) == 10401 and len(model.data.loop_triangles) == 20386
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
replaced, added = {'PART_Palm_L', 'PART_Palm_R'}, {'PART_Hand_glove_L', 'PART_Hand_glove_R'}
assert set(source_parts)-replaced == set(target_parts)-added
preserved = 0
for name, indices in source_parts.items():
    if name in replaced:
        continue
    other = target_parts[name]
    assert len(indices) == len(other)
    for a, b in zip(indices, other):
        assert (original.data.vertices[a].co-model.data.vertices[b].co).length < 1e-7
        assert bone_weights(original, a) == bone_weights(model, b)
    source_remap, target_remap = {v: i for i, v in enumerate(indices)}, {v: i for i, v in enumerate(other)}
    before = [(tuple(source_remap[i] for i in p.vertices), p.material_index, p.use_smooth)
              for p in original.data.polygons if all(i in source_remap for i in p.vertices)]
    after = [(tuple(target_remap[i] for i in p.vertices), p.material_index, p.use_smooth)
             for p in model.data.polygons if all(i in target_remap for i in p.vertices)]
    assert before == after, 'Changed preserved faces: '+name
    preserved += 1
assert preserved == 103
assert all(tuple(a.diffuse_color) == tuple(b.diffuse_color) for a, b in zip(original.data.materials, model.data.materials))
hand_metrics = {}
for name in sorted(added):
    indices = set(target_parts[name])
    assert len(indices) == 1090
    faces = [p for p in model.data.polygons if all(i in indices for i in p.vertices)]
    assert len(faces) == 1088 and all(len(p.vertices) == 4 and p.area > 1e-10 for p in faces)
    bm = bmesh.new()
    vertices = {i: bm.verts.new(model.data.vertices[i].co) for i in indices}
    for polygon in faces:
        bm.faces.new([vertices[i] for i in polygon.vertices])
    assert all(e.is_manifold and e.is_contiguous for e in bm.edges)
    assert all(v.is_manifold for v in bm.verts)
    assert len(bm.verts)-len(bm.edges)+len(bm.faces) == 2
    volume = bm.calc_volume(signed=True)
    assert volume > 0
    visited, pending = set(), [next(iter(bm.verts))]
    while pending:
        vertex = pending.pop()
        if vertex in visited:
            continue
        visited.add(vertex)
        pending.extend(e.other_vert(vertex) for e in vertex.link_edges if e.other_vert(vertex) not in visited)
    assert len(visited) == len(indices), 'Hand is not one connected component'
    bm.free()
    suffix = name[-1]
    weighted = {bone for index in indices for bone in bone_weights(model, index)}
    expected = {'Bip001 '+suffix+' Hand', 'Bip001 '+suffix+' Finger0', 'Bip001 '+suffix+' Finger01'}
    expected.update('Bip001 '+suffix+' Finger'+str(digit)+ending for digit in range(1, 5) for ending in ('', '1', '2'))
    assert weighted == expected
    hand_metrics[suffix] = {'vertices': len(indices), 'quads': len(faces), 'closed_components': 1, 'volume_m3': volume}

max_influences = 0
for vertex in model.data.vertices:
    weights = list(bone_weights(model, vertex.index).values())
    assert weights and abs(sum(weights)-1) < 2e-6 and all(0 < w <= 1 for w in weights)
    max_influences = max(max_influences, len(weights))
assert max_influences <= 4
rigid_parts = json.loads(scene['gloom_rigid_parts'])
assert rigid_parts == json.loads(source['gloom_rigid_parts']) and len(rigid_parts) == 93
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
scene.frame_set(1)
thumb_test_cases = 0
# The inherited clip curls the four fingers but does not animate the thumbs.
# Exercise both thumb chains separately without adding keys or changing the saved action.
for axis in ((0, 1, 0), (0, 0, 1)):
    for degrees in (-20, 20):
        scene.frame_set(1)
        for suffix, sign in (('L', 1), ('R', -1)):
            for ending in ('0', '01'):
                name = 'Bip001 '+suffix+' Finger'+ending
                rest_rotation = rig.data.bones[name].matrix_local.to_quaternion()
                rig.pose.bones[name].rotation_quaternion = rest_rotation.inverted()@Quaternion(axis, math.radians(degrees*sign))@rest_rotation
        bpy.context.view_layer.update()
        evaluated = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = evaluated.to_mesh()
        assert all(p.area > 1e-12 for p in mesh.polygons)
        assert all(all(math.isfinite(c) for c in v.co) for v in mesh.vertices)
        for suffix in ('L', 'R'):
            indices = target_parts['PART_Hand_glove_'+suffix]
            assert max((mesh.vertices[i].co-rest[i]).length for i in indices) > .005
        evaluated.to_mesh_clear()
        thumb_test_cases += 1
scene.frame_set(1)
bpy.context.view_layer.update()
print(json.dumps({'vertices': len(rest), 'triangles': len(model.data.loop_triangles), 'preserved_components': preserved,
                  'hands': hand_metrics, 'unchanged_bones': 53, 'unchanged_diagnostic_clip': True,
                  'max_influences': max_influences, 'rigid_parts': len(rigid_parts), 'sampled_frames': 61, 'extra_thumb_cases': thumb_test_cases,
                  'max_displacement_m': max_displacement, 'max_rigid_distance_error_m': rigid_error}, indent=2))
