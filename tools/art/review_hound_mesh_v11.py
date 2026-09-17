'''Render H03 draft evidence; --final saves/exports once and never overwrites an existing authoring source.'''
import bpy
import sys
import math
from pathlib import Path
from mathutils import Matrix, Vector
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h03_review as review

root = Path(__file__).resolve().parents[2]
output = root/'docs/art/hound/mesh-v11'
output.mkdir(parents=True,exist_ok=True)
scene = bpy.data.scenes['Hound_Mesh_v11']
model,rig = scene.objects['H11_DeformMesh'],scene.objects['Hound11_Rig']
review.setup(scene,850,1100)
cam = review.camera(scene,(2.8,-5,2.2),(0,0,.91),2.08)
scene.frame_set(1)
if '--final' in sys.argv:
    for directory in ('art/characters/hound/v11','assets/characters/hound_rig/v11'):
        (root/directory).mkdir(parents=True,exist_ok=True)
    bpy.context.window.scene = scene
    bpy.ops.object.select_all(action='DESELECT')
    model.select_set(True)
    rig.select_set(True)
    bpy.context.view_layer.objects.active = model
    path = root/'art/characters/hound/v11/hound-mesh-v11.blend'
    assert not path.exists(), 'Preserve existing v11 source'
    bpy.ops.wm.save_as_mainfile(filepath=str(path),check_existing=False,compress=True)
    bpy.ops.export_scene.gltf(filepath=str(root/'assets/characters/hound_rig/v11/hound-rig.gltf'),
        export_format='GLTF_SEPARATE',use_selection=True,use_active_scene=True,export_yup=True,export_apply=False,
        export_animations=True,export_animation_mode='ACTIVE_ACTIONS',export_nla_strips_merged_animation_name='Hound11_joint_check',
        export_anim_slide_to_zero=True,export_force_sampling=True,export_frame_range=True,export_skins=True,export_def_bones=True,
        export_rest_position_armature=True,export_all_influences=False,export_influence_nb=4,export_extras=True)
if '--export-only' in sys.argv:
    print('H03_SAVED_EXPORTED')
    raise SystemExit
for name,position in [('rest',(2.8,-5,2.2)),('front',(0,-5,1)),('profile',(5,0,1)),('back',(0,5,1))]:
    cam.location = position
    cam.rotation_euler = (Vector((0,0,.91))-cam.location).to_track_quat('-Z','Y').to_euler()
    review.render(scene,output/(name+'.png'))

for hood in (False,True):
    board = bpy.data.scenes.new('H03_HeadComparison'+str(hood))
    review.setup(board,2400,1000)
    for col,(angle,title) in enumerate(((0,'FRENTE'),(-.65,'TRES CUARTOS'),(-math.pi/2,'PERFIL'))):
        for n,version in enumerate(('10','11')):
            src = bpy.data.scenes['Hound_Mesh_v'+version]
            mesh = src.objects['H'+version+'_DeformMesh']
            src.frame_set(1)
            x = (col*2+n-2.5)*.30
            names = review.HEAD_PARTS | ({'Hood_continuous'} if hood else set())
            transform = Matrix.Translation((x,0,0))@Matrix.Rotation(angle,4,'Z')
            review.snapshot(src,mesh,board,transform,names)
            review.label(board,'V'+version+' / '+title,x,1.43,.014)
    review.label(board,'H03 / '+('ABERTURA DE CAPUCHA CONSERVADA' if hood else 'ROSTRO AISLADO / CAPUCHA OCULTA SOLO EN ESTA VISTA'),0,1.88,.023)
    review.camera(board,(0,-4,1.65),(0,0,1.65),1.85)
    review.render(board,output/('hood-comparison.png' if hood else 'head-comparison.png'))

board = bpy.data.scenes.new('H03_AnatomyComparison')
review.setup(board,2200,1200)
for col,(angle,title) in enumerate(((0,'FRENTE'),(-math.pi/2,'LATERAL'),(math.pi,'POSTERIOR'))):
    for n,version in enumerate(('10','11')):
        src = bpy.data.scenes['Hound_Mesh_v'+version]
        mesh = src.objects['H'+version+'_DeformMesh']
        x = (col*2+n-2.5)*.31
        transform = Matrix.Translation((x,0,0))@Matrix.Rotation(angle,4,'Z')@Matrix.Translation((-.34,0,0))
        review.snapshot(src,mesh,board,transform,{'Arm_surface_L'})
        review.label(board,'V'+version+' / '+title,x,.93,.016)
review.label(board,'H03 / ANATOMIA VISIBLE / MISMA ESCALA Y LUZ',0,1.63,.026)
review.camera(board,(0,-4,1.27),(0,0,1.27),2.04)
review.render(board,output/'arm-comparison.png')

for scale,pixels,name in ((16/3,432,'combat-size'),(8,288,'combat-far')):
    board = bpy.data.scenes.new('H03_CombatSize'+str(pixels))
    review.setup(board,1280,720)
    review.snapshot(scene,model,board,Matrix.Identity(4))
    review.camera(board,(0,-5,1.02),(0,0,1.02),scale)
    review.label(board,'H03 / '+str(pixels)+' PX / SIN EMISION NI BLOOM',0,-.27,.055)
    review.render(board,output/(name+'.png'))

action = rig.animation_data.action
rig.animation_data.action = None
for kind in review.CASES:
    review.pose(scene,rig,kind)
    board = bpy.data.scenes.new('H03_Pose_'+kind)
    head = kind.startswith(('turn-','look-'))
    review.setup(board,1200,1000)
    review.snapshot(scene,model,board,Matrix.Identity(4))
    target,scale = ((0,0,1.58),.57) if head else ((0,-.10,1.28),1.42)
    review.camera(board,(1.1,-4,1.77),target,scale)
    review.render(board,output/('pose-'+kind+'.png'))
rig.animation_data.action = action
scene.frame_set(1)
bpy.context.window.scene = scene
print('H03_REVIEWED')
