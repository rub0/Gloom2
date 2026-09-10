'''Run Blender in background on the delivered .blend to verify editable source.'''
import bpy
import math
from mathutils import Vector
scene = bpy.context.scene
version = scene.get('gloom_blockout_version','v01')
assert version in ('v01','v02') and scene.name == 'Hound_Blockout_'+version, 'Missing Hound authoring scene'
expected_parts, expected_triangles = (105,6974) if version == 'v01' else (113,7626)
bpy.context.window.scene = scene
collection = bpy.data.collections.get('HOUND_'+version+'_MODEL_ONLY')
assert collection is not None and len(collection.objects) == expected_parts, 'Unexpected number of editable parts'
assert all(obj.type == 'MESH' for obj in collection.objects), 'Only geometry belongs in the model collection'
assert not any(obj.type == 'ARMATURE' for obj in scene.objects), 'No rig expected at blockout stage'
assert scene.unit_settings.scale_length == 1, 'Source must use meters'
depsgraph = bpy.context.evaluated_depsgraph_get()
triangles = 0
points = []
for obj in collection.objects:
    assert obj.data.materials, 'Unassigned material'
    assert obj.animation_data is None, 'Unexpected animation'
    evaluated = obj.evaluated_get(depsgraph)
    data = evaluated.to_mesh()
    data.calc_loop_triangles()
    triangles += len(data.loop_triangles)
    points.extend(evaluated.matrix_world @ v.co for v in data.vertices)
    assert all(poly.area > 0 for poly in data.polygons), 'Degenerate face'
    evaluated.to_mesh_clear()
assert triangles == expected_triangles, 'Source and export triangulations must agree'
assert abs(min(p.z for p in points)) < .001 and abs(max(p.z for p in points)-1.8) < .001, 'Incorrect height or ground origin'
assert all(math.isfinite(v) for p in points for v in p), 'Non-finite geometry'
assert len([obj for obj in scene.objects if obj.type == 'CAMERA']) == 4, 'Four review cameras expected'
print('HOUND_BLEND_VERIFIED:',version,expected_parts,'editable meshes;',triangles,'triangles; 1.80m; 4 review cameras; no rig')
