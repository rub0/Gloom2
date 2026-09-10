'''Validate v04 topology, unchanged components/skeleton and diagnostic deformation.'''
import bpy
import json
import math

scene = bpy.data.scenes['Hound_Mesh_v04']
bpy.context.window.scene = scene
model = scene.objects['H04_DeformMesh']
rig = scene.objects['Hound04_Rig']
source = bpy.data.scenes['Hound_Rig_v03']
original = source.objects['H03_DeformMesh']
original_rig = source.objects['Hound03_Rig']
assert len(model.data.vertices) == 8941
model.data.calc_loop_triangles()
assert len(model.data.loop_triangles) == 17466
assert len(model.data.materials) == 7 and len(rig.data.bones) == 53
assert len([o for o in scene.objects if o.type == 'MESH']) == 1
assert len([m for m in model.modifiers if m.type == 'ARMATURE' and m.object == rig]) == 1
assert set(rig.data.bones.keys()) == set(original_rig.data.bones.keys())
for bone in rig.data.bones:
    reference = original_rig.data.bones[bone.name]
    assert bone.matrix_local == reference.matrix_local and abs(bone.length-reference.length) < 1e-7
    assert (bone.parent.name if bone.parent else None) == (reference.parent.name if reference.parent else None)


def parts(obj):
    result = {g.name: [] for g in obj.vertex_groups if g.name.startswith('PART_')}
    for vertex in obj.data.vertices:
        for group in vertex.groups:
            name = obj.vertex_groups[group.group].name
            if name in result:
                result[name].append(vertex.index)
    return result


source_parts, target_parts = parts(original), parts(model)
replaced = {'PART_'+name+'_'+suffix for name in ('Arm_continuous', 'Biceps', 'Deltoid') for suffix in ('L', 'R')}
assert set(source_parts)-replaced == set(target_parts)-{'PART_Arm_surface_L', 'PART_Arm_surface_R'}
preserved = 0
for name, indices in source_parts.items():
    if name in replaced:
        continue
    other = target_parts[name]
    assert len(indices) == len(other)
    assert all((original.data.vertices[a].co-model.data.vertices[b].co).length < 1e-7 for a, b in zip(indices, other))
    preserved += 1
assert preserved == 103
assert all(tuple(a.diffuse_color) == tuple(b.diffuse_color) for a, b in zip(original.data.materials, model.data.materials))

for name in ('PART_Arm_surface_L', 'PART_Arm_surface_R'):
    indices = set(target_parts[name])
    assert len(indices) == 37*32
    faces = [p for p in model.data.polygons if all(i in indices for i in p.vertices)]
    assert len(faces) == 36*32+2 and sum(len(p.vertices) == 4 for p in faces) == 36*32
    edges = {}
    neighbors = {i: set() for i in indices}
    for polygon in faces:
        assert polygon.area > 1e-10
        for a, b in zip(polygon.vertices, list(polygon.vertices[1:])+[polygon.vertices[0]]):
            edge = tuple(sorted((a, b)))
            edges[edge] = edges.get(edge, 0)+1
            neighbors[a].add(b)
            neighbors[b].add(a)
    assert all(count == 2 for count in edges.values())
    assert len(indices)-len(edges)+len(faces) == 2
    visited, pending = set(), [next(iter(indices))]
    while pending:
        index = pending.pop()
        if index in visited:
            continue
        visited.add(index)
        pending.extend(neighbors[index]-visited)
    assert visited == indices, 'Arm is not a single connected component'

bone_names = set(rig.data.bones.keys())
max_influences = 0
for vertex in model.data.vertices:
    weights = [g.weight for g in vertex.groups if model.vertex_groups[g.group].name in bone_names]
    assert weights and abs(sum(weights)-1) < 2e-6 and all(0 < w <= 1 for w in weights)
    max_influences = max(max_influences, len(weights))
assert max_influences == 2
rigid_parts = json.loads(scene['gloom_rigid_parts'])
for part, bone in rigid_parts.items():
    for index in target_parts['PART_'+part]:
        weights = [(model.vertex_groups[g.group].name, g.weight) for g in model.data.vertices[index].groups
                   if model.vertex_groups[g.group].name in bone_names]
        assert weights == [(bone, 1.0)]
scene.frame_set(1)
bpy.context.view_layer.update()
rest = [v.co.copy() for v in model.data.vertices]
assert abs(min(v.z for v in rest)) < 1e-6 and abs(max(v.z for v in rest)-1.8) < 1e-6
max_displacement = 0
rigid_error = 0
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
projection = json.loads(scene['gloom_arm_projection_max_error_m'])
assert max(projection.values()) < .003
scene.frame_set(1)
print(json.dumps({'vertices': len(rest), 'triangles': 17466, 'preserved_components': preserved,
                  'continuous_arm_components': 2, 'rings_per_arm': 37, 'vertices_per_ring': 32,
                  'unchanged_bones': 53, 'max_influences': max_influences, 'rigid_parts': len(rigid_parts),
                  'sampled_frames': 61, 'max_displacement_m': max_displacement,
                  'max_rigid_distance_error_m': rigid_error, 'directed_projection_error_m': projection}, indent=2))
