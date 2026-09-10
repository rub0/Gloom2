'''Review/export the actual v03 mesh and its diagnostic rig; never write v01/v02.'''
import bpy
import math
from mathutils import Vector

scene = bpy.data.scenes['Hound_Rig_v03']
bpy.context.window.scene = scene
rig = scene.objects['Hound03_Rig']
model = scene.objects['H03_DeformMesh']
scene.render.engine = 'BLENDER_WORKBENCH'
scene.render.resolution_x, scene.render.resolution_y = 850, 1100
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.display.shading.light = 'STUDIO'
scene.display.shading.color_type = 'MATERIAL'
scene.display.shading.show_shadows = True
scene.display.shading.show_cavity = True
scene.display.shading.cavity_type = 'WORLD'
scene.display.shading.background_type = 'WORLD'
scene.world = bpy.data.worlds.new('Hound03_ReviewWorld')
scene.world.color = (.105, .104, .10)
scene.view_settings.view_transform = 'Standard'
scene.view_settings.look = 'Medium High Contrast'
camera_data = bpy.data.cameras.new('Hound03_ReviewCamera')
camera = bpy.data.objects.new('Hound03_ReviewCamera', camera_data)
scene.collection.objects.link(camera)
camera_data.type = 'ORTHO'
camera_data.ortho_scale = 2.08
scene.camera = camera
output = 'D:/Projects/Gloom/docs/art/hound/rig-v03/'
for name, frame, position in [('rest', 1, (2.8, -5, 2.2)), ('elbows', 31, (2.8, -5, 2.2)),
                              ('reach', 61, (2.8, -5, 2.2)), ('step', 91, (2.8, -5, 2.2)), ('front', 1, (0, -5, .91))]:
    scene.frame_set(frame)
    camera.location = position
    camera.rotation_euler = (Vector((0, 0, .91))-camera.location).to_track_quat('-Z', 'Y').to_euler()
    scene.render.filepath = output+name+'.png'
    bpy.ops.render.render(write_still=True)
    print('HOUND03_RENDER', name)

scene.frame_set(1)
camera.location = (2.8, -5, 2.2)
camera.rotation_euler = (Vector((0, 0, .91))-camera.location).to_track_quat('-Z', 'Y').to_euler()
bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
rig.select_set(True)
bpy.context.view_layer.objects.active = rig
for area in bpy.context.screen.areas:
    if area.type == 'VIEW_3D':
        area.spaces.active.region_3d.view_perspective = 'CAMERA'
        area.spaces.active.overlay.show_overlays = True
        area.spaces.active.shading.color_type = 'MATERIAL'

# Keep reference scenes in the authoring file; selection restricts glTF to v03.
bpy.ops.wm.save_as_mainfile(filepath='D:/Projects/Gloom/art/characters/hound/v03/hound-rig-v03.blend', check_existing=False, compress=True)
bpy.ops.export_scene.gltf(filepath='D:/Projects/Gloom/assets/characters/hound_rig/v03/hound-rig.gltf',
                         export_format='GLTF_SEPARATE', use_selection=True, use_active_scene=True, export_yup=True,
                         export_apply=False, export_animations=True, export_animation_mode='ACTIVE_ACTIONS',
                         export_nla_strips_merged_animation_name='Hound03_joint_check',
                         export_anim_slide_to_zero=True,
                         export_force_sampling=True, export_frame_range=True, export_skins=True, export_def_bones=True,
                         export_rest_position_armature=True, export_all_influences=False, export_influence_nb=4, export_extras=True)
print('HOUND03_SOURCE_AND_GLTF_SAVED')
