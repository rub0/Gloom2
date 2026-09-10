'''Render/save/export v04 and a same-lighting before/after arm comparison.'''
import bpy
from mathutils import Vector

scene = bpy.data.scenes['Hound_Mesh_v04']
bpy.context.window.scene = scene
scene.render.engine = 'BLENDER_WORKBENCH'
scene.render.resolution_x, scene.render.resolution_y = 850, 1100
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.display.shading.light = 'STUDIO'
scene.display.shading.color_type = 'MATERIAL'
scene.display.shading.show_shadows = False
scene.display.shading.show_cavity = True
scene.display.shading.cavity_type = 'WORLD'
scene.display.shading.background_type = 'WORLD'
scene.world = bpy.data.worlds.new('Hound04_ReviewWorld')
scene.world.color = (.105, .104, .10)
scene.view_settings.view_transform = 'Standard'
scene.view_settings.look = 'Medium High Contrast'
data = bpy.data.cameras.new('Hound04_ReviewCamera')
camera = bpy.data.objects.new(data.name, data)
scene.collection.objects.link(camera)
camera.data.type, camera.data.ortho_scale = 'ORTHO', 2.08
scene.camera = camera
output = 'D:/Projects/Gloom/docs/art/hound/mesh-v04/'
for name, frame, position in [('rest', 1, (2.8, -5, 2.2)), ('elbows', 31, (2.8, -5, 2.2)),
                              ('reach', 61, (2.8, -5, 2.2)), ('front', 1, (0, -5, .91))]:
    scene.frame_set(frame)
    camera.location = position
    camera.rotation_euler = (Vector((0, 0, .91))-camera.location).to_track_quat('-Z', 'Y').to_euler()
    scene.render.filepath = output+name+'.png'
    bpy.ops.render.render(write_still=True)
    print('HOUND04_RENDER', name)
scene.frame_set(1)
camera.location = (2.8, -5, 2.2)
camera.rotation_euler = (Vector((0, 0, .91))-camera.location).to_track_quat('-Z', 'Y').to_euler()
bpy.ops.object.select_all(action='DESELECT')
for name in ('H04_DeformMesh', 'Hound04_Rig'):
    scene.objects[name].select_set(True)
bpy.context.view_layer.objects.active = scene.objects['H04_DeformMesh']
for area in bpy.context.screen.areas:
    if area.type == 'VIEW_3D':
        area.spaces.active.region_3d.view_perspective = 'CAMERA'
        area.spaces.active.shading.color_type = 'MATERIAL'
bpy.ops.wm.save_as_mainfile(filepath='D:/Projects/Gloom/art/characters/hound/v04/hound-mesh-v04.blend', check_existing=False, compress=True)
bpy.ops.export_scene.gltf(filepath='D:/Projects/Gloom/assets/characters/hound_rig/v04/hound-rig.gltf',
                         export_format='GLTF_SEPARATE', use_selection=True, use_active_scene=True, export_yup=True,
                         export_apply=False, export_animations=True, export_animation_mode='ACTIVE_ACTIONS',
                         export_nla_strips_merged_animation_name='Hound04_joint_check', export_anim_slide_to_zero=True,
                         export_force_sampling=True, export_frame_range=True, export_skins=True, export_def_bones=True,
                         export_rest_position_armature=True, export_all_influences=False, export_influence_nb=4, export_extras=True)

# Isolate the same left-arm components from both actual rest meshes, without any 2D retouching.
board = bpy.data.scenes.new('Hound04_ArmComparison')
board.render.engine = 'BLENDER_WORKBENCH'
board.render.resolution_x, board.render.resolution_y = 1400, 1100
board.render.resolution_percentage = 100
board.render.image_settings.file_format = 'PNG'
board.display.shading.light = 'STUDIO'
board.display.shading.color_type = 'MATERIAL'
board.display.shading.show_shadows = False
board.display.shading.show_cavity = True
board.display.shading.cavity_type = 'WORLD'
board.display.shading.background_type = 'WORLD'
board.world = scene.world
board.view_settings.view_transform = 'Standard'
board.view_settings.look = 'Medium High Contrast'
for source_name, model_name, x, label in [('Hound_Rig_v03', 'H03_DeformMesh', -.38, 'V03 / VOLÚMENES SUPERPUESTOS'),
                                       ('Hound_Mesh_v04', 'H04_DeformMesh', .38, 'V04 / SUPERFICIE CONTINUA')]:
    source = bpy.data.scenes[source_name]
    bpy.context.window.scene = source
    source.frame_set(1)
    bpy.context.view_layer.update()
    original = source.objects[model_name]
    groups = set()
    for group in original.vertex_groups:
        if group.name.startswith('PART_') and group.name.endswith('_L'):
            if any(part in group.name for part in ('Arm_', 'Biceps_', 'Deltoid_', 'Collar_', 'Bracer_', 'Elbow_')):
                groups.add(group.index)
    indices = [v.index for v in original.data.vertices if any(g.group in groups for g in v.groups)]
    remap = {index: i for i, index in enumerate(indices)}
    selected_faces = [p for p in original.data.polygons if all(i in remap for i in p.vertices)]
    mesh = bpy.data.meshes.new('ArmCompare_'+label)
    mesh.from_pydata([original.data.vertices[i].co for i in indices], [], [[remap[i] for i in p.vertices] for p in selected_faces])
    for material in original.data.materials:
        mesh.materials.append(material)
    for polygon, original_polygon in zip(mesh.polygons, selected_faces):
        polygon.material_index = original_polygon.material_index
        polygon.use_smooth = original_polygon.use_smooth
    obj = bpy.data.objects.new(mesh.name, mesh)
    board.collection.objects.link(obj)
    obj.location.x = x-.34
    text = bpy.data.curves.new('Label_'+label, 'FONT')
    text.body, text.size, text.align_x = label, .034, 'CENTER'
    label_obj = bpy.data.objects.new(text.name, text)
    board.collection.objects.link(label_obj)
    label_obj.location = (x, -.4, .85)
    label_obj.rotation_euler = (1.57079632679, 0, 0)
bpy.context.window.scene = board
data = bpy.data.cameras.new('Hound04_ComparisonCamera')
camera = bpy.data.objects.new(data.name, data)
board.collection.objects.link(camera)
camera.location = (0, -5, 1.34)
camera.rotation_euler = (Vector((0, 0, 1.34))-camera.location).to_track_quat('-Z', 'Y').to_euler()
data.type, data.ortho_scale = 'ORTHO', 1.65
board.camera = camera
board.render.filepath = output+'arm-comparison.png'
bpy.ops.render.render(write_still=True)
bpy.context.window.scene = scene
scene.frame_set(1)
print('HOUND04_SAVED_EXPORTED_REVIEWED')
