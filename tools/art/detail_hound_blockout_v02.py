'''Focused v02 reviews from actual geometry, with no paintover.'''
import bpy
import math
from mathutils import Matrix, Vector

source = bpy.data.scenes['Hound_Blockout_v02']
bpy.context.window.scene = source
camera_before = source.camera
resolution_before = (source.render.resolution_x,source.render.resolution_y)
path_before = source.render.filepath
data = bpy.data.cameras.new('Hound02_TemporaryHeadCamera')
camera = bpy.data.objects.new('Hound02_TemporaryHeadCamera',data)
source.collection.objects.link(camera)
data.type = 'ORTHO'
data.ortho_scale = 1.04
camera.location = (.55,-4,1.81)
camera.rotation_euler = (Vector((0,-.015,1.46))-camera.location).to_track_quat('-Z','Y').to_euler()
source.camera = camera
source.render.resolution_x = 1200
source.render.resolution_y = 1000
source.render.filepath = 'D:/Projects/Gloom/docs/art/hound/blockout-v02/detail-hood-collar.png'
bpy.ops.render.render(write_still=True)
source.camera = camera_before
source.render.resolution_x,source.render.resolution_y = resolution_before
source.render.filepath = path_before
bpy.data.objects.remove(camera,do_unlink=True)

name = 'Hound02_HandReview'
if bpy.data.scenes.get(name):
    raise RuntimeError('Hand review scene already exists')
stage = bpy.data.scenes.new(name)
bpy.context.window.scene = stage
stage.render.engine = 'BLENDER_WORKBENCH'
stage.render.resolution_x = 1400
stage.render.resolution_y = 1100
stage.render.resolution_percentage = 100
stage.render.image_settings.file_format = 'PNG'
stage.display.shading.light = 'STUDIO'
stage.display.shading.color_type = 'MATERIAL'
stage.display.shading.show_shadows = False
stage.display.shading.show_cavity = True
stage.display.shading.cavity_type = 'WORLD'
stage.display.shading.background_type = 'WORLD'
stage.world = source.world
stage.view_settings.view_transform = 'Standard'
stage.view_settings.look = 'Medium High Contrast'
wrist = Vector((.454,-.036,.961))
names = ['Palm_L','Wrist_transition_L','Hand_back_full_point_L','Bracer_mass_L','Bracer_face_L',
         'Bracer_blade_upper_L','Bracer_blade_lower_L','Elbow_cuff_L','Forearm_skin_L','Thumb_LA','Thumb_LB']
parts = [obj for obj in bpy.data.collections['HOUND_v02_MODEL_ONLY'].objects
         if obj.name[4:] in names or obj.name.startswith('H02_Finger_L')]
for view,x,angle in [('DORSO',-.235,-math.pi/2),('PALMA',.235,math.pi/2)]:
    transform = Matrix.Translation(Vector((x,0,.38))) @ Matrix.Rotation(angle,4,'Z') @ Matrix.Translation(-wrist)
    for original in parts:
        obj = original.copy()
        obj.name = 'HandReview_'+view+'_'+original.name
        stage.collection.objects.link(obj)
        obj.matrix_world = transform @ original.matrix_world
ink = bpy.data.materials.new('Hound02_ReviewLabel')
ink.diffuse_color = (.82,.78,.72,1)

def label(text,x,z,size):
    data = bpy.data.curves.new('HandReviewLabel','FONT')
    data.body = text
    data.size = size
    data.align_x = 'CENTER'
    obj = bpy.data.objects.new('HandReviewLabel',data)
    stage.collection.objects.link(obj)
    obj.location = (x,-.3,z)
    obj.rotation_euler = (math.pi/2,0,0)
    obj.data.materials.append(ink)

label('HOUND / MANO 02',0,.735,.041)
label('DORSO',-.235,.112,.033)
label('PALMA',.235,.112,.033)
label('PLACA COMPLETA EN PICO',-.235,.075,.018)
label('FLEXION HACIA LA PALMA',.235,.075,.018)
label('DOS VISTAS DE LA MISMA MANO / GEOMETRIA REAL',0,.025,.018)
data = bpy.data.cameras.new('HandReviewCamera')
camera = bpy.data.objects.new('HandReviewCamera',data)
stage.collection.objects.link(camera)
camera.location = (0,-3,.395)
camera.rotation_euler = (math.pi/2,0,0)
data.type = 'ORTHO'
data.ortho_scale = 1.0
stage.camera = camera
stage.render.filepath = 'D:/Projects/Gloom/docs/art/hound/blockout-v02/detail-hand.png'
bpy.ops.render.render(write_still=True)
bpy.context.window.scene = source
print('HOUND_V02_DETAILS_RENDERED: collar/hood and dorsal/palmar views of the same left-hand geometry')
