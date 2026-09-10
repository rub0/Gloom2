'''Validate the v03 Blender source, bindings, rigid parts and all diagnostic frames.'''
import bpy
import bmesh
import json
import math
from mathutils import Vector

scene = bpy.data.scenes['Hound_Rig_v03']
bpy.context.window.scene = scene
rig = scene.objects['Hound03_Rig']
obj = scene.objects['H03_DeformMesh']
names = set(rig.data.bones.keys())
assert len(names) == 53
assert len([o for o in scene.objects if o.type == 'MESH']) == 1
assert len(obj.data.materials) == 7
assert len([m for m in obj.modifiers if m.type == 'ARMATURE' and m.object == rig]) == 1
assert obj.matrix_local == rig.matrix_local
assert max(abs(c) for c in rig.location) == 0 and min(rig.scale) == max(rig.scale) == 1
assert set(a.name for a in bpy.data.actions if a == rig.animation_data.action) == {'Hound03_joint_check'}
assert scene.render.fps == 30 and scene.frame_end == 181
for bone in rig.data.bones:
    assert bone.length > .001 and all(math.isfinite(c) for c in bone.head_local)
    assert bone.name == 'Bip001' or bone.parent is not None
    assert bone.use_deform
max_influences = 0
for vertex in obj.data.vertices:
    weights = [g.weight for g in vertex.groups if obj.vertex_groups[g.group].name in names]
    assert weights and abs(sum(weights)-1) < 2e-6 and all(0 < w <= 1 for w in weights)
    max_influences = max(max_influences, len(weights))
assert max_influences <= 4
for name in ('Arm_continuous_L', 'Arm_continuous_R', 'Leg_continuous_L', 'Leg_continuous_R'):
    group = obj.vertex_groups['PART_'+name].index
    indices = {v.index for v in obj.data.vertices if any(g.group == group for g in v.groups)}
    assert len(indices) == 23*12
    faces = [p for p in obj.data.polygons if set(p.vertices) <= indices]
    assert len(faces) == 22*12+2 and sum(len(p.vertices) == 4 for p in faces) == 22*12
    edges = {}
    for face in faces:
        for a, b in zip(face.vertices, list(face.vertices[1:])+[face.vertices[0]]):
            key = tuple(sorted((a, b)))
            edges[key] = edges.get(key, 0)+1
    assert all(count == 2 for count in edges.values()), 'Open or non-manifold limb'
scene.frame_set(1)
bpy.context.view_layer.update()
depsgraph = bpy.context.evaluated_depsgraph_get()
evaluated = obj.evaluated_get(depsgraph)
mesh = evaluated.to_mesh()
assert len(mesh.vertices) == len(obj.data.vertices)
rest = [v.co.copy() for v in mesh.vertices]
assert max((v.co-rest[v.index]).length for v in obj.data.vertices) < 1e-5, 'Bind pose changed mesh'
assert abs(min(v.z for v in rest)) < 1e-5 and abs(max(v.z for v in rest)-1.8) < 1e-5
evaluated.to_mesh_clear()
rigid_parts = json.loads(scene['gloom_rigid_parts'])
rigid_indices = {}
for part, bone in rigid_parts.items():
    group = obj.vertex_groups['PART_'+part].index
    indices = [v.index for v in obj.data.vertices if any(g.group == group for g in v.groups)]
    assert indices and bone in names
    rigid_indices[part] = indices
    for index in indices:
        weights = [(obj.vertex_groups[g.group].name, g.weight) for g in obj.data.vertices[index].groups if obj.vertex_groups[g.group].name in names]
        assert weights == [(bone, 1.0)], 'Rigid plate has blended influences'
worst_rigid_error = 0
max_displacement = 0
for frame in range(1, 182, 3):
    scene.frame_set(frame)
    bpy.context.view_layer.update()
    evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    for vertex in mesh.vertices:
        assert all(math.isfinite(c) for c in vertex.co)
        max_displacement = max(max_displacement, (vertex.co-rest[vertex.index]).length)
        assert vertex.co.length < 3, 'Exploded deformation'
    for part, indices in rigid_indices.items():
        origin = indices[0]
        for index in indices[1:]:
            error = abs((mesh.vertices[index].co-mesh.vertices[origin].co).length-(rest[index]-rest[origin]).length)
            worst_rigid_error = max(worst_rigid_error, error)
    assert all(p.area > 1e-12 for p in mesh.polygons), 'Collapsed face'
    evaluated.to_mesh_clear()
assert worst_rigid_error < 2e-6
assert .25 < max_displacement < 1.2, 'Diagnostic clip did not exercise the rig'
scene.frame_set(1)
print(json.dumps({'bones': len(names), 'vertices': len(obj.data.vertices), 'materials': len(obj.data.materials),
                  'max_influences': max_influences, 'rigid_parts': len(rigid_parts), 'sampled_frames': 61,
                  'worst_rigid_distance_error_m': worst_rigid_error, 'max_displacement_m': max_displacement,
                  'closed_continuous_limbs': 4, 'height_m': 1.8}, indent=2))
