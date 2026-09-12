'''Render actual v04/v05 hand geometry, then save the editable v05 source and export its diagnostic glTF.'''
import bpy
import math
from mathutils import Matrix, Vector

scene = bpy.data.scenes['Hound_Mesh_v05']
bpy.context.window.scene = scene
world = bpy.data.worlds.new('Hound05_ReviewWorld')
world.color = (.105, .104, .10)
output = 'D:/Projects/Gloom/docs/art/hound/mesh-v05/'


def setup(target, width, height):
    target.render.engine = 'BLENDER_WORKBENCH'
    target.render.resolution_x, target.render.resolution_y = width, height
    target.render.resolution_percentage = 100
    target.render.image_settings.file_format = 'PNG'
    target.display.shading.light = 'STUDIO'
    target.display.shading.color_type = 'MATERIAL'
    target.display.shading.show_shadows = False
    target.display.shading.show_cavity = True
    target.display.shading.cavity_type = 'WORLD'
    target.display.shading.background_type = 'WORLD'
    target.world = world
    target.view_settings.view_transform = 'Standard'
    target.view_settings.look = 'Medium High Contrast'


def camera_for(target, position, look_at, scale):
    data = bpy.data.cameras.new(target.name+'_Camera')
    camera = bpy.data.objects.new(data.name, data)
    target.collection.objects.link(camera)
    camera.location = position
    camera.rotation_euler = (Vector(look_at)-camera.location).to_track_quat('-Z', 'Y').to_euler()
    data.type, data.ortho_scale = 'ORTHO', scale
    target.camera = camera
    return camera


def label(board, body, x, z, size=.016):
    data = bpy.data.curves.new('Label_'+body, 'FONT')
    data.body, data.size, data.align_x = body, size, 'CENTER'
    obj = bpy.data.objects.new(data.name, data)
    board.collection.objects.link(obj)
    obj.location = (x, -.14, z)
    obj.rotation_euler = (math.pi/2, 0, 0)


setup(scene, 850, 1100)
camera = camera_for(scene, (2.8, -5, 2.2), (0, 0, .91), 2.08)
for name, frame in (('rest', 1), ('fingers', 151)):
    scene.frame_set(frame)
    scene.render.filepath = output+name+'.png'
    bpy.ops.render.render(write_still=True)

for frame, filename in ((1, 'hand-comparison'), (151, 'hand-flexion')):
    board = bpy.data.scenes.new('Hound05_'+filename)
    setup(board, 1800, 850)
    for version, x, angle, only_glove, title in [
            ('04', -.36, math.pi/2, False, 'V04 / PALMA'),
            ('05', -.12, math.pi/2, False, 'V05 / PALMA'),
            ('05', .12, -math.pi/2, False, 'V05 / DORSO'),
            ('05', .36, math.pi/2, True, 'V05 / INTERIOR')]:
        source = bpy.data.scenes['Hound_Mesh_v'+version]
        bpy.context.window.scene = source
        source.frame_set(frame)
        bpy.context.view_layer.update()
        original = source.objects['H'+version+'_DeformMesh']
        rig = source.objects['Hound'+version+'_Rig']
        groups = set()
        for group in original.vertex_groups:
            if only_glove:
                include = group.name == 'PART_Hand_glove_L'
            else:
                include = group.name in ('PART_Palm_L', 'PART_Hand_glove_L', 'PART_Wrist_transition_L', 'PART_Hand_back_full_point_L')
                include = include or group.name.startswith(('PART_Finger_L_', 'PART_Thumb_L'))
            if include:
                groups.add(group.index)
        indices = [v.index for v in original.data.vertices if any(g.group in groups for g in v.groups)]
        remap = {index: i for i, index in enumerate(indices)}
        evaluated = original.evaluated_get(bpy.context.evaluated_depsgraph_get())
        evaluated_mesh = evaluated.to_mesh()
        # Undo whole-hand motion to compare local flexion at identical scale and camera angle.
        hand = rig.pose.bones['Bip001 L Hand']
        unpose = rig.data.bones[hand.name].matrix_local@hand.matrix.inverted()
        wrist = rig.data.bones[hand.name].head_local
        transform = Matrix.Translation(Vector((x, 0, .25)))@Matrix.Rotation(angle, 4, 'Z')@Matrix.Translation(-wrist)@unpose
        faces = [p for p in evaluated_mesh.polygons if all(i in remap for i in p.vertices)]
        data = bpy.data.meshes.new('HandReview_'+title)
        data.from_pydata([transform@evaluated_mesh.vertices[i].co for i in indices], [], [[remap[i] for i in p.vertices] for p in faces])
        for material in original.data.materials:
            data.materials.append(material)
        for polygon, reference in zip(data.polygons, faces):
            polygon.material_index = reference.material_index
            polygon.use_smooth = reference.use_smooth
        evaluated.to_mesh_clear()
        obj = bpy.data.objects.new(data.name, data)
        board.collection.objects.link(obj)
        if only_glove:
            clay = bpy.data.materials.new('ReviewOnly_InnerGloveClay')
            clay.diffuse_color = (.28, .32, .34, 1)
            data.materials.clear()
            data.materials.append(clay)
            for polygon in data.polygons:
                polygon.material_index = 0
        label(board, title, x, .015)
    label(board, 'HOUND / MANOS / '+('REPOSO' if frame == 1 else 'FLEXION DIAGNOSTICA'), 0, .36, .023)
    label(board, 'Misma escala. Interior aislado en gris para inspeccion; material real oscuro. Rig provisional.', 0, -.045, .012)
    camera_for(board, (0, -3, .16), (0, 0, .16), 1.02)
    bpy.context.window.scene = board
    board.render.filepath = output+filename+'.png'
    bpy.ops.render.render(write_still=True)

bpy.context.window.scene = scene
scene.frame_set(1)
bpy.ops.object.select_all(action='DESELECT')
for name in ('H05_DeformMesh', 'Hound05_Rig'):
    scene.objects[name].select_set(True)
bpy.context.view_layer.objects.active = scene.objects['H05_DeformMesh']
for area in bpy.context.screen.areas:
    if area.type == 'VIEW_3D':
        area.spaces.active.region_3d.view_perspective = 'CAMERA'
        area.spaces.active.shading.color_type = 'MATERIAL'
bpy.ops.wm.save_as_mainfile(filepath='D:/Projects/Gloom/art/characters/hound/v05/hound-mesh-v05.blend', check_existing=False, compress=True)
bpy.ops.export_scene.gltf(filepath='D:/Projects/Gloom/assets/characters/hound_rig/v05/hound-rig.gltf',
                         export_format='GLTF_SEPARATE', use_selection=True, use_active_scene=True, export_yup=True,
                         export_apply=False, export_animations=True, export_animation_mode='ACTIVE_ACTIONS',
                         export_nla_strips_merged_animation_name='Hound05_joint_check', export_anim_slide_to_zero=True,
                         export_force_sampling=True, export_frame_range=True, export_skins=True, export_def_bones=True,
                         export_rest_position_armature=True, export_all_influences=False, export_influence_nb=4, export_extras=True)
print('HOUND05_SAVED_EXPORTED_REVIEWED')
