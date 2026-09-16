'''Render versioned comparisons from actual meshes, save the source and export its unchanged diagnostic clip.'''
import bpy
import math
import sys
from mathutils import Matrix, Vector

revision = sys.argv[sys.argv.index('--')+1] if '--' in sys.argv else 'v06'
assert revision in ('v06', 'v07', 'v08')
suffix, prior = revision[1:], {'v06':'05','v07':'06','v08':'07'}[revision]
scene = bpy.data.scenes['Hound_Mesh_'+revision]
bpy.context.window.scene = scene
world = bpy.data.worlds.new('Hound'+suffix+'_ReviewWorld')
world.color = (.105, .104, .10)
output = 'D:/Projects/Gloom/docs/art/hound/mesh-'+revision+'/'


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


def label(board, body, x, z, size):
    data = bpy.data.curves.new('Label_'+body, 'FONT')
    data.body, data.size, data.align_x = body, size, 'CENTER'
    obj = bpy.data.objects.new(data.name, data)
    board.collection.objects.link(obj)
    obj.location = (x, -.35, z)
    obj.rotation_euler = (math.pi/2, 0, 0)


setup(scene, 850, 1100)
camera = camera_for(scene, (2.8, -5, 2.2), (0, 0, .91), 2.08)
for name, frame, position in [('rest', 1, (2.8, -5, 2.2)), ('step', 91, (2.8, -5, 2.2)),
                              ('torso', 121, (2.8, -5, 2.2)), ('back', 1, (0, 5, 1.05))]:
    scene.frame_set(frame)
    camera.location = position
    camera.rotation_euler = (Vector((0, 0, .91))-camera.location).to_track_quat('-Z', 'Y').to_euler()
    scene.render.filepath = output+name+'.png'
    bpy.ops.render.render(write_still=True)

for kind in (('waist', 'head') if revision == 'v06' else ('head', 'body') if revision == 'v08' else ('head', 'collar')):
    board = bpy.data.scenes.new('Hound'+suffix+'_'+kind+'Comparison')
    setup(board, 2000, 1050 if kind == 'head' else 550 if kind == 'collar' else 1200 if kind == 'body' else 850)
    spacing = .34 if kind == 'head' else 1.05 if kind == 'collar' else 1.12 if kind == 'body' else .58
    angle = math.pi if kind == 'waist' else .62
    view = 'ESPALDA' if kind == 'waist' else '3/4'
    for column, (version, angle, view) in enumerate([(prior, 0, 'FRENTE'), (suffix, 0, 'FRENTE'),
                                                    (prior, angle, view), (suffix, angle, view)]):
        source = bpy.data.scenes['Hound_Mesh_v'+version]
        bpy.context.window.scene = source
        source.frame_set(1)
        bpy.context.view_layer.update()
        original = source.objects['H'+version+'_DeformMesh']
        prefixes = ('Head_', 'Jaw_', 'Nose_', 'Cheek_', 'Brow_', 'Mouth_', 'Eye_', 'Hood_', 'Neck') if kind == 'head' else (
            'Pelvis_', 'Waist_', 'Leg_', 'Trousers_', 'Sash_tail_', 'Hip_guard_', 'Thigh_outer_')
        if kind == 'collar':
            prefixes = ('Head_', 'Mouth_', 'Eye_', 'Hood_', 'Neck', 'Collar_')
        if kind == 'body':
            prefixes = ('',)
        groups = {g.index for g in original.vertex_groups if g.name.startswith('PART_') and g.name[5:].startswith(prefixes)}
        indices = [v.index for v in original.data.vertices if any(g.group in groups for g in v.groups)]
        remap = {index: i for i, index in enumerate(indices)}
        faces = [p for p in original.data.polygons if all(i in remap for i in p.vertices)
                 and (kind != 'waist' or all(original.data.vertices[i].co.z >= .57 for i in p.vertices))]
        x = (column-1.5)*spacing
        transform = Matrix.Translation(Vector((x, 0, 0)))@Matrix.Rotation(angle, 4, 'Z')
        data = bpy.data.meshes.new('Review_'+kind+'_'+version+'_'+view)
        data.from_pydata([transform@original.data.vertices[i].co for i in indices], [], [[remap[i] for i in p.vertices] for p in faces])
        for material in original.data.materials:
            data.materials.append(material)
        for polygon, reference in zip(data.polygons, faces):
            polygon.material_index, polygon.use_smooth = reference.material_index, reference.use_smooth
        obj = bpy.data.objects.new(data.name, data)
        board.collection.objects.link(obj)
        label(board, 'V'+version+' / '+view, x, 1.395 if kind == 'head' else 1.25 if kind == 'collar' else -.10 if kind == 'body' else .50,
              .018 if kind == 'head' else .026)
    title = 'ROSTRO CONTINUO' if revision == 'v06' else 'CAPUCHA Y CUELLO CONTINUOS'
    if revision == 'v08':
        title = 'ROSTRO / FORMAS PRIMARIAS' if kind == 'head' else 'ARMADURA, ANATOMIA Y TELA / PASE ARTISTICO'
    if kind == 'body':
        label(board, 'HOUND / '+title, 0, 2.05, .041)
        label(board, 'Malla real / misma escala y luz / sin texturas finales', 0, -.24, .023)
        camera_for(board, (0,-4,.91), (0,0,.91), 4.65)
        bpy.context.window.scene = board
        board.render.filepath = output+kind+'-comparison.png'
        bpy.ops.render.render(write_still=True)
        continue
    label(board, 'HOUND / '+(title if kind == 'head' else 'ENCAJE CLAVICULAR' if kind == 'collar' else 'CINTURA Y TELA CONTINUAS'), 0,
          1.89 if kind == 'head' else 2.02 if kind == 'collar' else 1.19, .024 if kind == 'head' else .038)
    label(board, 'Malla real / misma escala y luz / sin texturas finales', 0, 1.335 if kind == 'head' else 1.15 if kind == 'collar' else .40,
          .014 if kind == 'head' else .021)
    z = 1.63 if kind == 'head' else 1.60 if kind == 'collar' else .815
    scale = (1.45 if revision == 'v06' else 1.60) if kind == 'head' else 4.35 if kind == 'collar' else 2.5
    camera_for(board, (0, -4, z), (0, 0, z), scale)
    bpy.context.window.scene = board
    board.render.filepath = output+kind+'-comparison.png'
    bpy.ops.render.render(write_still=True)

bpy.context.window.scene = scene
scene.frame_set(1)
camera.location = (2.8, -5, 2.2)
camera.rotation_euler = (Vector((0, 0, .91))-camera.location).to_track_quat('-Z', 'Y').to_euler()
bpy.ops.object.select_all(action='DESELECT')
for name in ('H'+suffix+'_DeformMesh', 'Hound'+suffix+'_Rig'):
    scene.objects[name].select_set(True)
bpy.context.view_layer.objects.active = scene.objects['H'+suffix+'_DeformMesh']
for area in bpy.context.screen.areas:
    if area.type == 'VIEW_3D':
        area.spaces.active.region_3d.view_perspective = 'CAMERA'
        area.spaces.active.shading.color_type = 'MATERIAL'
bpy.ops.wm.save_as_mainfile(filepath='D:/Projects/Gloom/art/characters/hound/'+revision+'/hound-mesh-'+revision+'.blend',
                         check_existing=False, compress=True)
bpy.ops.export_scene.gltf(filepath='D:/Projects/Gloom/assets/characters/hound_rig/'+revision+'/hound-rig.gltf',
                         export_format='GLTF_SEPARATE', use_selection=True, use_active_scene=True, export_yup=True,
                         export_apply=False, export_animations=True, export_animation_mode='ACTIVE_ACTIONS',
                         export_nla_strips_merged_animation_name='Hound'+suffix+'_joint_check', export_anim_slide_to_zero=True,
                         export_force_sampling=True, export_frame_range=True, export_skins=True, export_def_bones=True,
                         export_rest_position_armature=True, export_all_influences=False, export_influence_nb=4, export_extras=True)
print('HOUND_SAVED_EXPORTED_REVIEWED', revision)
