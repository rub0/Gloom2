'''Actual geometry comparison: rest, wire topology/skeleton, and elbow test.'''
import bpy
from mathutils import Vector

source = bpy.data.scenes['Hound_Rig_v03']
if bpy.data.scenes.get('Hound03_TechnicalBoard'):
    raise RuntimeError('Review board already exists; do not duplicate artist work')
scene = bpy.data.scenes.new('Hound03_TechnicalBoard')
scene.render.engine = 'BLENDER_WORKBENCH'
scene.render.resolution_x, scene.render.resolution_y = 1800, 1150
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.display.shading.light = 'STUDIO'
scene.display.shading.color_type = 'MATERIAL'
scene.display.shading.show_shadows = False
scene.display.shading.show_cavity = True
scene.display.shading.background_type = 'WORLD'
scene.world = bpy.data.worlds.new('Hound03_BoardWorld')
scene.world.color = (.09, .09, .095)
scene.view_settings.view_transform = 'Standard'
scene.view_settings.look = 'Medium High Contrast'


def material(name, color):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1)
    return mat


wire = material('Hound03_BoardWire', (.38, .41, .44))
cyan = material('Hound03_BoardBones', (.18, .77, .81))
white = material('Hound03_BoardLabels', (.76, .78, .78))


def text(name, content, x, z, size):
    data = bpy.data.curves.new(name, 'FONT')
    data.body, data.size, data.align_x = content, size, 'CENTER'
    obj = bpy.data.objects.new(name, data)
    scene.collection.objects.link(obj)
    obj.location = (x, -.4, z)
    obj.rotation_euler = (1.57079632679, 0, 0)
    data.materials.append(white)


for x, frame, label in [(-1.2, 1, 'REPOSO'), (0, 1, 'MALLA + ESQUELETO'), (1.2, 31, 'FLEXIÓN DE CODOS')]:
    bpy.context.window.scene = source
    source.frame_set(frame)
    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = source.objects['H03_DeformMesh'].evaluated_get(depsgraph)
    data = bpy.data.meshes.new_from_object(evaluated, preserve_all_data_layers=True, depsgraph=depsgraph)
    obj = bpy.data.objects.new('Board_'+label, data)
    scene.collection.objects.link(obj)
    obj.location.x = x
    if x == 0:
        data.materials.clear()
        data.materials.append(wire)
        for polygon in data.polygons:
            polygon.material_index = 0
        modifier = obj.modifiers.new('Render actual topology edges', 'WIREFRAME')
        modifier.thickness = .001
        # Octahedra use the actual rest bone endpoints. Flatten Y only for the labeled frontal overlay.
        rig = source.objects['Hound03_Rig']
        for bone in rig.data.bones:
            a, b = bone.head_local.copy(), bone.tail_local.copy()
            a.y = b.y = -.27
            axis = (b-a).normalized()
            tangent = axis.cross(Vector((0, 1, 0))).normalized()
            radius = min(.014, bone.length*.14)
            center = a.lerp(b, .27)
            points = [a, b, center+tangent*radius, center+Vector((0, radius, 0)),
                      center-tangent*radius, center-Vector((0, radius, 0))]
            faces = [(0, 2, 3), (0, 3, 4), (0, 4, 5), (0, 5, 2), (1, 3, 2), (1, 4, 3), (1, 5, 4), (1, 2, 5)]
            mesh = bpy.data.meshes.new('BoneDiagram_'+bone.name)
            mesh.from_pydata(points, [], faces)
            mesh.materials.append(cyan)
            bone_obj = bpy.data.objects.new(mesh.name, mesh)
            scene.collection.objects.link(bone_obj)
    text('Label_'+label, label, x, -.14, .075)
text('BoardTitle', 'HOUND / BASE DE DEFORMACIÓN 03', 0, 2.06, .115)
text('BoardSubtitle', '53 huesos · pesos de prueba · armadura rígida · diseño v02 conservado', 0, 1.95, .055)
text('BoardFooter', 'Prototipo técnico — sin animaciones finales, UVs ni texturas. Esqueleto superpuesto en vista frontal.', 0, -.27, .045)
bpy.context.window.scene = scene
data = bpy.data.cameras.new('Hound03_BoardCamera')
camera = bpy.data.objects.new(data.name, data)
scene.collection.objects.link(camera)
camera.location = (0, -8, .95)
camera.rotation_euler = (Vector((0, 0, .95))-camera.location).to_track_quat('-Z', 'Y').to_euler()
data.type, data.ortho_scale = 'ORTHO', 4.15
scene.camera = camera
scene.render.filepath = 'D:/Projects/Gloom/docs/art/hound/rig-v03/technical-board.png'
bpy.ops.render.render(write_still=True)
bpy.context.window.scene = source
source.frame_set(1)
print('HOUND03_TECHNICAL_BOARD_RENDERED: real mesh, true bone X/Z projection; no 2D paintover')
