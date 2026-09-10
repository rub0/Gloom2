'''Run Blender in background on the delivered .blend to verify editable source.'''
import bpy
import math
from mathutils import Vector
scene = bpy.data.scenes.get('Hound_Blockout_v01')
assert scene is not None, 'Missing Hound authoring scene'
bpy.context.window.scene = scene
collection = bpy.data.collections.get('HOUND_v01_MODEL_ONLY')
assert collection is not None and len(collection.objects) == 105, 'Expected 105 editable parts'
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
assert triangles == 6974, 'Source and export triangulations must agree'
assert abs(min(p.z for p in points)) < .001 and abs(max(p.z for p in points)-1.8) < .001, 'Incorrect height or ground origin'
assert all(math.isfinite(v) for p in points for v in p), 'Non-finite geometry'
assert len([obj for obj in scene.objects if obj.type == 'CAMERA']) == 4, 'Four review cameras expected'
print('HOUND_BLEND_VERIFIED: 105 editable meshes; 6974 triangles; 1.80m; 4 review cameras; no rig')
