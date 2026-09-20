'''H04 evidence from real mesh geometry. --final saves/exports once; --video renders temporary review animation.'''
import bpy
import sys
import math
from pathlib import Path
from mathutils import Matrix, Vector
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h04_review as review

root = Path(__file__).resolve().parents[2]
output = root/'docs/art/hound/mesh-v12'
output.mkdir(parents=True,exist_ok=True)
scene = bpy.data.scenes['Hound_Mesh_v12']
model,rig = scene.objects['H12_DeformMesh'],scene.objects['Hound12_Rig']
review.setup(scene,1000,1100)
cam = review.camera(scene,(2.8,-5,2.2),(0,0,.91),2.1)
scene.frame_set(1)
if '--final' in sys.argv:
    for directory in ('art/characters/hound/v12','assets/characters/hound_rig/v12'):
        (root/directory).mkdir(parents=True,exist_ok=True)
    bpy.context.window.scene = scene
    bpy.ops.object.select_all(action='DESELECT')
    model.select_set(True)
    rig.select_set(True)
    bpy.context.view_layer.objects.active = model
    path = root/'art/characters/hound/v12/hound-mesh-v12.blend'
    assert not path.exists(), 'Preserve existing v12 edits'
    bpy.ops.wm.save_as_mainfile(filepath=str(path),check_existing=False,compress=True)
    bpy.ops.export_scene.gltf(filepath=str(root/'assets/characters/hound_rig/v12/hound-rig.gltf'),
        export_format='GLTF_SEPARATE',use_selection=True,use_active_scene=True,export_yup=True,export_apply=False,
        export_animations=True,export_animation_mode='ACTIVE_ACTIONS',export_nla_strips_merged_animation_name='Hound12_joint_check',
        export_anim_slide_to_zero=True,export_force_sampling=True,export_frame_range=True,export_skins=True,export_def_bones=True,
        export_rest_position_armature=True,export_all_influences=False,export_influence_nb=4,export_extras=True)
if '--export-only' in sys.argv:
    print('H04_SAVED_EXPORTED')
    raise SystemExit

if '--video' in sys.argv:
    temporary = bpy.data.scenes.new('H04_TemporaryVideo')
    bpy.context.window.scene = temporary
    arm = rig.copy()
    arm.data = rig.data.copy()
    arm.animation_data_clear()
    temporary.collection.objects.link(arm)
    obj = model.copy()
    obj.data = model.data.copy()
    obj.parent,obj.modifiers[0].object = arm,arm
    temporary.collection.objects.link(obj)
    review.setup(temporary,1000,1000)
    cam = review.camera(temporary,(2,-4,1.8),(0,0,1),2.1)
    temporary.render.fps,temporary.frame_start,temporary.frame_end = 30,1,361
    poses = []
    for frame in range(1,362):
        segment = min(8,(frame-1)//40)
        t = ((frame-1)-segment*40)/40
        review.pose(temporary,arm,review.CASES[segment+1],math.sin(math.pi*t))
        poses.append({bone.name:bone.rotation_quaternion.copy() for bone in arm.pose.bones})
    for frame,values in enumerate(poses,1):
        for name,rotation in values.items():
            bone = arm.pose.bones[name]
            bone.rotation_quaternion = rotation
            bone.keyframe_insert(data_path='rotation_quaternion',frame=frame)
    for curve in arm.animation_data.action.fcurves:
        for key in curve.keyframe_points:
            key.interpolation = 'LINEAR'

    for frame,position,target,scale in ((1,(2,-4,1.8),(0,0,1),2.1),(41,(2,4,1.8),(0,0,1.30),1.05),
                                       (81,(2,-4,1.8),(0,0,1),2.1),(121,(4,-2,1.5),(0,0,.88),1.9),
                                       (201,(1,-4,1.3),(0,-.08,.91),.9),(281,(1,-4,1.8),(0,0,1.48),.72)):
        cam.location = position
        cam.rotation_euler = (Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
        cam.data.ortho_scale = scale
        cam.keyframe_insert(data_path='location',frame=frame)
        cam.keyframe_insert(data_path='rotation_euler',frame=frame)
        cam.data.keyframe_insert(data_path='ortho_scale',frame=frame)
    for owner in (cam,cam.data):
        for curve in owner.animation_data.action.fcurves:
            for key in curve.keyframe_points:
                key.interpolation = 'CONSTANT'
    temporary.render.image_settings.file_format = 'FFMPEG'
    temporary.render.ffmpeg.format,temporary.render.ffmpeg.codec = 'MPEG4','H264'
    temporary.render.ffmpeg.constant_rate_factor = 'MEDIUM'
    temporary.render.ffmpeg.ffmpeg_preset = 'GOOD'
    temporary.render.filepath = str(output/'assembly-check.mp4')
    bpy.ops.render.render(animation=True)
    print('H04_VIDEO',361,30)
    raise SystemExit

views = [('rest',(2.8,-5,2.2),(0,0,.91),2.1),('front',(0,-5,1),(0,0,.91),2.1),
         ('profile',(5,0,1),(0,0,.91),2.1),('back',(0,5,1),(0,0,.91),2.1),
         ('waist',(1,-4,1.2),(0,0,.91),.67),('crotch',(0,-4,.72),(0,0,.78),.64),
         ('axilla',(3,-3,1.5),(.19,0,1.42),.56),('neck',(.8,-4,1.7),(0,0,1.49),.56),
         ('back-assembly',(1,4,1.6),(0,0,1.34),.85)]
for name,position,target,scale in views:
    cam.location = position
    cam.rotation_euler = (Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale = scale
    review.render(scene,output/(name+'.png'))

for name,angle,target,scale in [('cloth-comparison',0,.89,1.18),('back-comparison',math.pi,1.28,1.18)]:
    board = bpy.data.scenes.new('H04_'+name)
    review.setup(board,1800,1100)
    for n,version in enumerate(('11','12')):
        src = bpy.data.scenes['Hound_Mesh_v'+version]
        mesh = src.objects['H'+version+'_DeformMesh']
        src.frame_set(1)
        x = (n-.5)*.56
        prefixes = (('Trousers','Waist','Sash','Hip_guard','Thigh_outer','Knee','Abdominal') if name=='cloth-comparison'
                    else ('Torso','Scapula','Back_spine','Yoke','Collar_plate','Waist'))
        names = {n for n in review.parts(mesh) if n.startswith(prefixes)}
        review.snapshot(src,mesh,board,Matrix.Translation((x,0,0))@Matrix.Rotation(angle,4,'Z'),names)
        review.label(board,'V'+version,x,target-.32,.025)
    review.camera(board,(0,-5,target),(0,0,target),scale)
    review.render(board,output/(name+'.png'))

action = rig.animation_data.action
rig.animation_data.action = None
for kind in review.CASES[1:]:
    review.pose(scene,rig,kind)
    cam.location = (2,-4,1.8)
    target,scale = ((0,0,1.48),.80) if kind.startswith('head-') else ((0,0,.95),2.12)
    cam.rotation_euler = (Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale = scale
    review.render(scene,output/('pose-'+kind+'.png'))

for name,kind,position,target,scale in [('pose-twist-back','twist-left',(1,4,1.7),(0,0,1.31),.88),
                                       ('pose-hip-detail','hip-left',(1,-4,1.1),(.06,-.1,.95),.64)]:
    review.pose(scene,rig,kind)
    cam.location = position
    cam.rotation_euler = (Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale = scale
    review.render(scene,output/(name+'.png'))

rig.animation_data.action = action
scene.frame_set(1)
bpy.context.window.scene = scene
print('H04_REVIEWED')
