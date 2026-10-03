"""Exercise shared art helpers in Blender without saving any source or export."""
import bpy
import ast
import sys
from pathlib import Path
from mathutils import Vector
sys.path.insert(0, str(Path(__file__).resolve().parent))
from hound_h01_review import setup, camera
from hound_h04_review import parts

scene = bpy.data.scenes.new('SharedHelpersCheck')
world = bpy.data.worlds.new('SharedHelpersWorld')
scene.world = world
setup(scene, 850, 1100)
assert scene.world == world and all(abs(a - b) < 1e-6 for a, b in zip(world.color, (.105, .104, .10)))
assert scene.render.engine == 'BLENDER_WORKBENCH' and scene.render.resolution_percentage == 100
assert (scene.render.resolution_x, scene.render.resolution_y) == (850, 1100)
assert scene.render.image_settings.file_format == 'PNG' and scene.display.shading.background_type == 'WORLD'
assert scene.view_settings.view_transform == 'Standard' and scene.view_settings.look == 'Medium High Contrast'
view = camera(scene, (2.8, -5, 2.2), (0, 0, .91), 2.08)
assert scene.camera == view and view.data.type == 'ORTHO' and abs(view.data.ortho_scale - 2.08) < 1e-6
assert view.name.startswith('SharedHelpersCheck_Camera') and (view.location - Vector((2.8, -5, 2.2))).length < 1e-6
assert ((view.rotation_euler.to_quaternion() @ Vector((0, 0, -1))) - (Vector((0, 0, .91)) - view.location).normalized()).length < 1e-6
mesh = bpy.data.meshes.new('SharedHelpersMesh')
mesh.from_pydata([(0, 0, 0), (1, 0, 0), (0, 1, 0)], [], [(0, 1, 2)])
model = bpy.data.objects.new('SharedHelpersModel', mesh)
scene.collection.objects.link(model)
model.vertex_groups.new(name='PART_Arm').add([0, 2], 1, 'REPLACE')
model.vertex_groups.new(name='PART_Glove').add([2], .5, 'REPLACE')
model.vertex_groups.new(name='PART_Empty')
model.vertex_groups.new(name='Bone').add([0, 1, 2], 1, 'REPLACE')
assert parts(model) == {'Arm': [0, 2], 'Glove': [2], 'Empty': []}
assert parts(model, keep_prefix=True) == {'PART_Arm': [0, 2], 'PART_Glove': [2], 'PART_Empty': []}
for prefix, versions in (('verify', ('04', '05', '06', '09', '10', '11', '12')), ('review', ('05', '06'))):
    for version in versions:
        script = Path(__file__).with_name(prefix + '_hound_mesh_v' + version + '.py')
        tree = ast.parse(script.read_text(encoding='utf-8'))
        imports = ast.Module(body=[node for node in tree.body if isinstance(node, (ast.Import, ast.ImportFrom))], type_ignores=[])
        namespace = {}
        exec(compile(imports, str(script), 'exec'), namespace)
        if prefix == 'verify':
            assert namespace.get('parts') is parts, script
        else:
            assert namespace.get('setup') is setup and namespace.get('camera_for') is camera, script
print('SHARED_ART_HELPERS_OK')
