'''Render the actual blockout, save its Blender source and export a static glTF.'''
import bpy
import math
from mathutils import Vector, Matrix

scene = bpy.context.scene
version = scene.get('gloom_blockout_version','v01')
if version not in ('v01','v02') or scene.name != 'Hound_Blockout_'+version:
    raise RuntimeError('Activate a supported Hound authoring scene before review/export')
prefix = 'Hound'+version[1:]
cache_name = 'hound-blockout' if version == 'v01' else 'hound-blockout-'+version
bpy.context.window.scene = scene
model = bpy.data.collections['HOUND_'+version+'_MODEL_ONLY']
scene.render.engine = 'BLENDER_WORKBENCH'
scene.render.resolution_x = 1000
scene.render.resolution_y = 1200
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.display.shading.light = 'STUDIO'
scene.display.shading.color_type = 'MATERIAL'
scene.display.shading.show_shadows = True
scene.display.shading.show_cavity = True
scene.display.shading.cavity_type = 'WORLD'
scene.display.shading.cavity_ridge_factor = 1.2
scene.display.shading.cavity_valley_factor = 1.0
scene.display.shading.show_specular_highlight = True
scene.display.shading.background_type = 'WORLD'
if not scene.world:
    scene.world = bpy.data.worlds.new(prefix+'_StudioWorld')
scene.world.color = (.105,.104,.10)
scene.view_settings.view_transform = 'Standard'
scene.view_settings.look = 'Medium High Contrast'
views = [('front',(0,-5,.91)),('profile',(5,0,.91)),('back',(0,5,.91)),('three-quarter',(2.8,-5,2.20))]
for name, position in views:
    camera_name = prefix+'_Camera_' + name
    camera = bpy.data.objects.get(camera_name)
    if not camera:
        data = bpy.data.cameras.new(camera_name)
        camera = bpy.data.objects.new(camera_name,data)
        scene.collection.objects.link(camera)
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = 2.06
    camera.location = position
    camera.rotation_euler = (Vector((0,0,.91))-camera.location).to_track_quat('-Z','Y').to_euler()
    scene.camera = camera
    scene.render.filepath = 'D:/Projects/Gloom/docs/art/hound/blockout-'+version+'/' + name + '.png'
    bpy.ops.render.render(write_still=True)
    print('HOUND_VIEW:', name)

bpy.context.view_layer.update()
depsgraph = bpy.context.evaluated_depsgraph_get()
points = []
triangles = 0
for obj in model.objects:
    evaluated = obj.evaluated_get(depsgraph)
    data = evaluated.to_mesh()
    data.calc_loop_triangles()
    triangles += len(data.loop_triangles)
    points.extend(evaluated.matrix_world @ v.co for v in data.vertices)
    evaluated.to_mesh_clear()
lower = [min(p[i] for p in points) for i in range(3)]
upper = [max(p[i] for p in points) for i in range(3)]
scene['gloom_bounds_min'] = lower
scene['gloom_bounds_max'] = upper
scene['gloom_triangle_count'] = triangles
scene['gloom_note'] = 'BLOCKOUT: approval pending. No final UVs, rig, textures, LOD tuning or gameplay replacement.'
print('HOUND_STATS:', len(model.objects), 'mesh parts;', triangles, 'triangles; bounds:', lower, upper)

# Keep only our model scene in the delivered file; the previous throwaway probe is safe on disk.
old = bpy.data.scenes.get('Scene')
if old and set(obj.name for obj in old.objects) == {'ProbePlate','ProbeTexturedPanel','ProbeEnergyCore'}:
    bpy.data.scenes.remove(old)
extra_camera = bpy.data.objects.get(prefix+'_ReviewCamera')
if extra_camera:
    bpy.data.objects.remove(extra_camera,do_unlink=True)
bpy.ops.object.select_all(action='DESELECT')
for obj in model.objects:
    obj.select_set(True)
bpy.context.view_layer.objects.active = model.objects.get('H'+version[1:]+'_Torso_underlayer')
for area in bpy.context.screen.areas:
    if area.type == 'VIEW_3D':
        area.spaces.active.region_3d.view_perspective = 'CAMERA'
        area.spaces.active.shading.color_type = 'MATERIAL'
        area.spaces.active.overlay.show_overlays = False

bpy.ops.wm.save_as_mainfile(filepath='D:/Projects/Gloom/art/characters/hound/'+version+'/hound-blockout-'+version+'.blend', check_existing=False)
bpy.ops.export_scene.gltf(filepath='D:/Projects/Gloom/assets/characters/hound_blockout/'+version+'/hound-blockout.gltf',
                         export_format='GLTF_SEPARATE', use_selection=True, use_active_scene=True, export_yup=True, export_apply=True,
                         export_animations=False, export_extras=True)
print('HOUND_SOURCE_AND_GLTF_SAVED')

# The generic Gloom viewer looks from -Z. Rotate a throwaway export only.
transforms = [(obj, obj.matrix_world.copy()) for obj in model.objects]
for obj, transform in transforms:
    obj.matrix_world = Matrix.Rotation(math.pi,4,'Z') @ transform
bpy.context.view_layer.update()
bpy.ops.export_scene.gltf(filepath='D:/Projects/Gloom/.cache/'+cache_name+'/source/hound-viewer.gltf',
                         export_format='GLTF_SEPARATE', use_selection=True, use_active_scene=True, export_yup=True, export_apply=True,
                         export_animations=False)
for obj, transform in transforms:
    obj.matrix_world = transform
bpy.context.view_layer.update()
print('HOUND_VIEWER_COPY_EXPORTED: source forward axis unchanged')
