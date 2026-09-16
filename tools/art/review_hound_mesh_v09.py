'''Save/export v09 and render matching H01 views and temporary stress poses.'''
import bpy
import sys
from pathlib import Path
from mathutils import Matrix, Vector
import math
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h01_review as review

root = Path(__file__).resolve().parents[2]
output = root/'docs/art/hound/mesh-v09'
scene = bpy.data.scenes['Hound_Mesh_v09']
model,rig = scene.objects['H09_DeformMesh'],scene.objects['Hound09_Rig']
review.setup(scene,850,1100)
cam = review.camera(scene,(2.8,-5,2.2),(0,0,.91),2.08)
for name,position in [('rest',(2.8,-5,2.2)),('front',(0,-5,1)),('profile',(5,0,1)),('back',(0,5,1))]:
    cam.location = position
    cam.rotation_euler = (Vector((0,0,.91))-cam.location).to_track_quat('-Z','Y').to_euler()
    review.render(scene,output/(name+'.png'))
cam.location = (2.8,-5,2.2)
cam.rotation_euler = (Vector((0,0,.91))-cam.location).to_track_quat('-Z','Y').to_euler()
bpy.context.window.scene = scene
bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
rig.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.wm.save_as_mainfile(filepath=str(root/'art/characters/hound/v09/hound-mesh-v09.blend'),check_existing=False,compress=True)
bpy.ops.export_scene.gltf(filepath=str(root/'assets/characters/hound_rig/v09/hound-rig.gltf'),
    export_format='GLTF_SEPARATE',use_selection=True,use_active_scene=True,export_yup=True,export_apply=False,
    export_animations=True,export_animation_mode='ACTIVE_ACTIONS',export_nla_strips_merged_animation_name='Hound09_joint_check',
    export_anim_slide_to_zero=True,export_force_sampling=True,export_frame_range=True,export_skins=True,export_def_bones=True,
    export_rest_position_armature=True,export_all_influences=False,export_influence_nb=4,export_extras=True)

board = bpy.data.scenes.new('H01_Comparison')
review.setup(board,2100,1050)
for col,(angle,title) in enumerate(((-math.pi/2,'DORSO'),(math.pi/2,'PALMA'),(math.pi,'PERFIL'))):
    for n,version in enumerate(('08','09')):
        src = bpy.data.scenes['Hound_Mesh_v'+version]
        mesh = src.objects['H'+version+'_DeformMesh']
        skeleton = src.objects['Hound'+version+'_Rig']
        src.frame_set(1)
        wrist = skeleton.data.bones['Bip001 L Hand'].head_local
        x = (col*2+n-2.5)*.26
        transform = Matrix.Translation((x,0,.28))@Matrix.Rotation(angle,4,'Z')@Matrix.Translation(-wrist)
        review.snapshot(src,mesh,board,transform,review.hand_parts(mesh,'L'))
        review.label(board,'V'+version+' / '+title,x,-.035,.017)
review.label(board,'H01 / MANOS Y GUANTELETES / MISMA CAMARA, ESCALA Y LUZ',0,.69,.025)
review.camera(board,(0,-4,.29),(0,0,.29),1.70)
review.render(board,output/'hand-comparison.png')

board = bpy.data.scenes.new('H01_BodyComparison')
review.setup(board,1600,1100)
for n,version in enumerate(('08','09')):
    src = bpy.data.scenes['Hound_Mesh_v'+version]
    mesh = src.objects['H'+version+'_DeformMesh']
    review.snapshot(src,mesh,board,Matrix.Translation((n*1.2-.6,0,0))@Matrix.Rotation(.48,4,'Z'))
    review.label(board,'V'+version,n*1.2-.6,-.10,.045)
review.camera(board,(0,-4,.91),(0,0,.91),2.95)
review.render(board,output/'body-comparison.png')

action = rig.animation_data.action
rig.animation_data.action = None
for kind in ('open','fist','flex','extend'):
    review.pose(scene,rig,kind)
    board = bpy.data.scenes.new('H01_'+kind)
    review.setup(board,1500,950)
    for n,(suffix,angle) in enumerate((('L',math.pi/2),('L',-math.pi/2),('R',-math.pi/2))):
        wrist = rig.data.bones['Bip001 '+suffix+' Hand'].head_local
        transform = Matrix.Translation(((n-1)*.30,0,.22))@Matrix.Rotation(angle,4,'Z')@Matrix.Translation(-wrist)
        review.snapshot(scene,model,board,transform,review.hand_parts(model,suffix,False))
        review.label(board,suffix+(' / PALMA' if n!=1 else ' / DORSO'),(n-1)*.30,-.045,.018)
    review.label(board,'H01 / '+kind.upper()+' / POSE DIAGNOSTICA',0,.44,.024)
    review.camera(board,(0,-4,.20),(0,0,.20),1.01)
    review.render(board,output/(kind+'.png'))
review.pose(scene,rig,'grip')
gun = review.weapon(scene,rig)
board = bpy.data.scenes.new('H01_SoulReaperSupport')
review.setup(board,1100,1100)
review.snapshot(scene,model,board,Matrix.Identity(4),review.hand_parts(model,'R',False))
copy = gun.copy()
board.collection.objects.link(copy)
target = rig.data.bones['Bip001 R Hand'].head_local+Vector((.03,0,-.09))
cam = review.camera(board,target+Vector((.1,-.8,.1)),target,.39)
review.render(board,output/'grip.png')
cam.location = target+Vector((.8,-.4,.25))
cam.rotation_euler = (target-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.ortho_scale = .48
review.render(board,output/'grip-oblique.png')
points = [copy.matrix_world@v.co for v in copy.data.vertices]
center = (Vector(tuple(min(p[a] for p in points) for a in range(3)))+Vector(tuple(max(p[a] for p in points) for a in range(3))))/2
cam.location = center+Vector((.8,-.4,.25))
cam.rotation_euler = (center-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.ortho_scale = 1.04
review.render(board,output/'grip-context.png')
bpy.data.objects.remove(gun,do_unlink=True)
rig.animation_data.action = action
scene.frame_set(1)
bpy.context.window.scene = scene
print('H01_SAVED_EXPORTED_REVIEWED')
