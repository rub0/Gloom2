'''Create a turnaround from three copies of the actual mesh, without editing rendered images.'''
import bpy
import math
from mathutils import Matrix, Vector

source = bpy.context.scene
version = source.get('gloom_blockout_version','v01')
if version not in ('v01','v02') or source.name != 'Hound_Blockout_'+version:
    raise RuntimeError('Activate a supported Hound authoring scene before rendering the board')
board_name = 'Hound'+version[1:]+'_Turnaround'
if bpy.data.scenes.get(board_name):
    raise RuntimeError('Turnaround scene already exists')
board = bpy.data.scenes.new(board_name)
bpy.context.window.scene = board
board.render.engine = 'BLENDER_WORKBENCH'
board.render.resolution_x = 2000
board.render.resolution_y = 1160
board.render.resolution_percentage = 100
board.render.image_settings.file_format = 'PNG'
board.display.shading.light = 'STUDIO'
board.display.shading.color_type = 'MATERIAL'
board.display.shading.show_shadows = True
board.display.shading.show_cavity = True
board.display.shading.cavity_type = 'WORLD'
board.display.shading.cavity_ridge_factor = 1.2
board.display.shading.cavity_valley_factor = 1.0
board.display.shading.background_type = 'WORLD'
board.world = source.world
board.view_settings.view_transform = 'Standard'
board.view_settings.look = 'Medium High Contrast'

for name,x,angle in [('FRONT',-1.4,0),('PROFILE',0,math.pi/2),('BACK',1.4,math.pi)]:
    collection = bpy.data.collections.new('ReviewOnly_'+name)
    board.collection.children.link(collection)
    transform = Matrix.Translation(Vector((x,0,0))) @ Matrix.Rotation(angle,4,'Z')
    for original in bpy.data.collections['HOUND_'+version+'_MODEL_ONLY'].objects:
        obj = original.copy()
        obj.name = 'ReviewOnly_'+name+'_'+original.name
        collection.objects.link(obj)
        obj.matrix_world = transform @ original.matrix_world

ink = bpy.data.materials.new('Hound'+version[1:]+'_Label')
ink.diffuse_color = (.8,.76,.69,1)
def label(name,text,x,z,size,align='CENTER'):
    data = bpy.data.curves.new(name,'FONT')
    data.body = text
    data.size = size
    data.align_x = align
    obj = bpy.data.objects.new(name,data)
    board.collection.objects.link(obj)
    obj.location = (x,-.45,z)
    obj.rotation_euler = (math.pi/2,0,0)
    obj.data.materials.append(ink)

label('BoardTitle','GLOOM / HOUND',-2.04,2.035,.10,'LEFT')
label('BoardVersion','VOLUMEN '+version[1:]+'  /  1,80 m',2.04,2.05,.05,'RIGHT')
label('FrontLabel','FRENTE',-1.4,-.14,.063)
label('ProfileLabel','PERFIL',0,-.14,.063)
label('BackLabel','ESPALDA',1.4,-.14,.063)
label('BoardDisclaimer','MALLA REAL DE BLENDER  /  MATERIALES PLANOS  /  PENDIENTE DE VALIDAR',0,-.28,.038)
data = bpy.data.cameras.new('TurnaroundCamera')
camera = bpy.data.objects.new('TurnaroundCamera',data)
board.collection.objects.link(camera)
camera.location = (0,-10,.945)
camera.rotation_euler = (math.pi/2,0,0)
data.type = 'ORTHO'
data.ortho_scale = 4.52
board.camera = camera
board.render.filepath = 'D:/Projects/Gloom/docs/art/hound/blockout-'+version+'/turnaround.png'
bpy.ops.render.render(write_still=True)
bpy.context.window.scene = source
print('HOUND_TURNAROUND_RENDERED: three linked geometry copies, no paintover')
