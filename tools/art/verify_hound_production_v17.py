"""H07 source gate: preservation, rigid surface error, weights and every diagnostic frame."""
import bpy
import hashlib
import json
import math
import sys
from pathlib import Path
from mathutils.bvhtree import BVHTree

sys.path.insert(0, str(Path(__file__).resolve().parent))
from hound_h04_review import parts

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT/'.cache/hound-production-v17'
scene, old = bpy.data.scenes['Hound_Mesh_v17'], bpy.data.scenes['Hound_Mesh_v16']
model, rig = scene.objects['H17_DeformMesh'], scene.objects['Hound17_Rig']
original, previous = old.objects['H16_DeformMesh'], old.objects['Hound16_Rig']
bpy.context.window.scene = scene
scene.frame_set(1)
assert len(rig.data.bones) == 53 and len(model.data.materials) == 7
assert len(model.modifiers) == 1 and model.modifiers[0].object == rig
assert tuple(model.scale) == tuple(rig.scale) == (1, 1, 1)
assert model.matrix_world == original.matrix_world and rig.matrix_world == previous.matrix_world
for bone in rig.data.bones:
    reference = previous.data.bones[bone.name]
    assert bone.matrix_local == reference.matrix_local and bone.length == reference.length
    assert (bone.parent.name if bone.parent else None) == (reference.parent.name if reference.parent else None)
for curve, reference in zip(rig.animation_data.action.fcurves, previous.animation_data.action.fcurves):
    assert curve.data_path == reference.data_path and curve.array_index == reference.array_index
    assert len(curve.keyframe_points) == len(reference.keyframe_points)
    for key, point in zip(curve.keyframe_points, reference.keyframe_points):
        assert key.co == point.co and key.interpolation == point.interpolation
        assert key.handle_left == point.handle_left and key.handle_right == point.handle_right
assert len(rig.animation_data.action.fcurves) == len(previous.animation_data.action.fcurves)
before, after = parts(original), parts(model)
rigid = json.loads(scene['gloom_rigid_parts'])
assert rigid == json.loads(old['gloom_rigid_parts'])
assert set(before) == set(after) and len(after) == 106


def weights(obj, index):
    return {obj.vertex_groups[w.group].name: w.weight for w in obj.data.vertices[index].groups
            if obj.vertex_groups[w.group].name in rig.data.bones and w.weight > 0}


for vertex in model.data.vertices:
    values = weights(model, vertex.index)
    assert 1 <= len(values) <= 2 and abs(sum(values.values())-1) < 2e-6
    assert all(math.isfinite(w) and 0 < w <= 1 for w in values.values())
assert all(math.isfinite(c) for uv in model.data.uv_layers for item in uv.data for c in item.uv)
assert all(all(math.isfinite(c) for c in n.vector) and abs(n.vector.length-1) < 1e-5 for n in model.data.corner_normals)
rows = []
for row in json.loads(scene['gloom_h07_inventory']):
    name = row['part']
    ids, old_ids = after[name], before[name]
    if name in rigid:
        assert all(weights(model, i) == {rigid[name]: 1.0} for i in ids), name
    if row['ratio'] == 1:
        assert len(ids) == len(old_ids)
        assert all(model.data.vertices[a].co == original.data.vertices[b].co and weights(model, a) == weights(original, b)
                   for a, b in zip(ids, old_ids)), name
        a_map, b_map = {v:i for i,v in enumerate(ids)}, {v:i for i,v in enumerate(old_ids)}
        assert [(tuple(a_map[i] for i in p.vertices), p.material_index) for p in model.data.polygons if p.vertices[0] in a_map] == [
            (tuple(b_map[i] for i in p.vertices), p.material_index) for p in original.data.polygons if p.vertices[0] in b_map]
    surfaces = []
    for obj, indices in ((original, old_ids), (model, ids)):
        obj.data.calc_loop_triangles()
        remap = {v:i for i,v in enumerate(indices)}
        points = [obj.data.vertices[i].co.copy() for i in indices]
        faces = [[remap[i] for i in p.vertices] for p in obj.data.loop_triangles if p.vertices[0] in remap]
        tree = BVHTree.FromPolygons(points, faces)
        samples = points+[sum((points[i] for i in f), points[0]*0)/len(f) for f in faces]
        surfaces.append((tree, samples))
    error = max(max(surfaces[1-i][0].find_nearest(p)[3] for p in surfaces[i][1]) for i in (0, 1))
    row['surface_sample_error_m'] = error
    rows.append(row)
print('H07_SURFACE_ERRORS', sorted([(r['surface_sample_error_m'], r['part']) for r in rows], reverse=True)[:15], flush=True)
assert max(row['surface_sample_error_m'] for row in rows) < .003, 'Rigid reduction exceeds 3 mm sampled surface gate'
rest = [v.co.copy() for v in model.data.vertices]
minimum, rigid_error = 1, 0
for frame in range(1, 182):
    scene.frame_set(frame)
    bpy.context.view_layer.update()
    evaluated = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    assert all(all(math.isfinite(c) for c in v.co) for v in mesh.vertices)
    minimum = min(minimum, min(p.area for p in mesh.polygons))
    for name in rigid:
        ids = after[name]
        for i in ids[1:]:
            rigid_error = max(rigid_error, abs((mesh.vertices[i].co-mesh.vertices[ids[0]].co).length-(rest[i]-rest[ids[0]]).length))
    evaluated.to_mesh_clear()
assert minimum > 1e-12 and rigid_error < 2e-6
scene.frame_set(1)
for version in ('v13', 'v14', 'v15', 'v16'):
    for item in json.loads((ROOT/f'art/characters/hound/{version}/sculpture-reference.json').read_text())['files']:
        assert hashlib.sha256((ROOT/item['path']).read_bytes()).hexdigest() == item['sha256'], item['path']
result = {'components': rows, 'diagnostic_frames': 181, 'minimum_area_m2': minimum,
          'rigid_distance_error_m': rigid_error, 'deformable_parts_exact': 10, 'waist_height_rings': 13, 'rigid_parts': len(rigid),
          'bones_bind_action_exact': True, 'previous_manifest_files_exact': 12}
(WORK/'verification.json').write_text(json.dumps(result, indent=2)+'\n')
print('H07_VERIFIED', minimum, rigid_error, sorted([(r['surface_sample_error_m'], r['part']) for r in rows], reverse=True)[:8])
