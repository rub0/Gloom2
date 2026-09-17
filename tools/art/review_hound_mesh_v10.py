'''Save/export H02 only with --final; render the current draft without overwriting previous versions.'''
import bpy
import sys
import math
from pathlib import Path
from mathutils import Vector, Matrix
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h02_review as review

root = Path(__file__).resolve().parents[2]
output = root/'docs/art/hound/mesh-v10'
output.mkdir(parents=True,exist_ok=True)
scene = bpy.data.scenes['Hound_Mesh_v10']
model,rig = scene.objects['H10_DeformMesh'],scene.objects['Hound10_Rig']
review.setup(scene,850,1100)
cam = review.camera(scene,(2.8,-5,2.2),(0,0,.91),2.08)
if '--final' in sys.argv:
    for directory in ('art/characters/hound/v10','assets/characters/hound_rig/v10'):
        (root/directory).mkdir(parents=True,exist_ok=True)
    bpy.context.window.scene = scene
    bpy.ops.object.select_all(action='DESELECT')
    model.select_set(True)
    rig.select_set(True)
    bpy.context.view_layer.objects.active = model
    path = root/'art/characters/hound/v10/hound-mesh-v10.blend'
    assert not path.exists(), 'Preserve existing v10 source'
    bpy.ops.wm.save_as_mainfile(filepath=str(path),check_existing=False,compress=True)
    bpy.ops.export_scene.gltf(filepath=str(root/'assets/characters/hound_rig/v10/hound-rig.gltf'),
        export_format='GLTF_SEPARATE',use_selection=True,use_active_scene=True,export_yup=True,export_apply=False,
        export_animations=True,export_animation_mode='ACTIVE_ACTIONS',export_nla_strips_merged_animation_name='Hound10_joint_check',
        export_anim_slide_to_zero=True,export_force_sampling=True,export_frame_range=True,export_skins=True,export_def_bones=True,
        export_rest_position_armature=True,export_all_influences=False,export_influence_nb=4,export_extras=True)
for name,position in [('rest',(2.8,-5,2.2)),('front',(0,-5,1)),('profile',(5,0,1)),('back',(0,5,1))]:
    cam.location = position
    cam.rotation_euler = (Vector((0,0,.91))-cam.location).to_track_quat('-Z','Y').to_euler()
    review.render(scene,output/(name+'.png'))

board = bpy.data.scenes.new('H02_LegComparison')
review.setup(board,2100,1150)
for col,(angle,title) in enumerate(((0,'FRENTE'),(-math.pi/2,'PERFIL'),(math.pi,'ESPALDA'))):
    for n,version in enumerate(('09','10')):
        src = bpy.data.scenes['Hound_Mesh_v'+version]
        mesh = src.objects['H'+version+'_DeformMesh']
        src.frame_set(1)
        x = (col*2+n-2.5)*.37
        names = review.leg_parts(mesh,'L')-{'Trousers_continuous'}
        transform = Matrix.Translation((x,0,0))@Matrix.Rotation(angle,4,'Z')@Matrix.Translation((-.197,0,0))
        review.snapshot(src,mesh,board,transform,names)
        review.label(board,'V'+version+' / '+title,x,-.055,.021)
review.label(board,'H02 / GREBAS Y BOTAS / MISMA CAMARA, ESCALA Y LUZ',0,.84,.031)
review.camera(board,(0,-4,.36),(0,0,.36),2.32)
review.render(board,output/'leg-comparison.png')

board = bpy.data.scenes.new('H02_BodyComparison')
review.setup(board,1600,1100)
for n,version in enumerate(('09','10')):
    src = bpy.data.scenes['Hound_Mesh_v'+version]
    mesh = src.objects['H'+version+'_DeformMesh']
    review.snapshot(src,mesh,board,Matrix.Translation((n*1.2-.6,0,0))@Matrix.Rotation(.48,4,'Z'))
    review.label(board,'V'+version,n*1.2-.6,-.10,.045)
review.camera(board,(0,-4,.91),(0,0,.91),2.95)
review.render(board,output/'body-comparison.png')

action = rig.animation_data.action
rig.animation_data.action = None
for kind in ('rest','knee','step','toe','heel'):
    review.pose(scene,rig,model,kind)
    board = bpy.data.scenes.new('H02_'+kind)
    review.setup(board,1600,1100)
    for n,angle in enumerate((0,-math.pi/2)):
        transform = Matrix.Translation((n*.85-.42,0,0))@Matrix.Rotation(angle,4,'Z')
        review.snapshot(scene,model,board,transform,review.leg_parts(model))
    review.floor(board)
    review.label(board,'H02 / '+kind.upper()+' / APOYO DIAGNOSTICO',0,1.16,.036)
    review.camera(board,(0,-4,.58),(0,0,.58),2.04)
    review.render(board,output/('pose-'+kind+'.png'))
review.pose(scene,rig,model,'rest')
for name,position,target,scale,names in (
        ('knee',(.7,-1.8,.78),(.177,0,.54),.42,review.leg_parts(model,'L')-{'Trousers_continuous'}),
        ('ankle',(.8,-1.5,.40),(.197,-.02,.16),.43,review.leg_parts(model,'L')-{'Trousers_continuous'}),
        ('sole',(.201,-.06,-2),(.201,-.06,0),.38,{'Boot_L','Boot_toe_L'})):
    board = bpy.data.scenes.new('H02_Detail_'+name)
    review.setup(board,1000,1000)
    review.snapshot(scene,model,board,Matrix.Identity(4),names)
    review.camera(board,position,target,scale)
    review.render(board,output/('detail-'+name+'.png'))
rig.animation_data.action = action
scene.frame_set(1)
bpy.context.window.scene = scene
print('H02_REVIEWED')
