'''H03 head/arm review video. Temporary animation and cameras never replace the source diagnostic action.'''
import bpy
import sys
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h03_review as review

root = Path(__file__).resolve().parents[2]
source = bpy.data.scenes['Hound_Mesh_v11']
original,original_rig = source.objects['H11_DeformMesh'],source.objects['Hound11_Rig']
scene = bpy.data.scenes.new('H03_TemporaryVideo')
bpy.context.window.scene = scene
rig = original_rig.copy()
rig.data = original_rig.data.copy()
rig.animation_data_clear()
scene.collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.parent,model.modifiers[0].object = rig,rig
scene.collection.objects.link(model)
review.setup(scene,1000,1000)
cam = review.camera(scene,(1.1,-4,1.77),(0,0,1.60),.60)
scene.render.fps,scene.frame_start,scene.frame_end = 30,1,241
stages = [(1,'rest'),(21,'turn-left'),(41,'rest'),(61,'turn-right'),(81,'rest'),(101,'look-up'),(121,'rest'),
          (141,'look-down'),(161,'rest'),(181,'elbow'),(201,'rest'),(221,'reach'),(241,'rest')]
names = ['Bip001 Head','Bip001 Neck']+['Bip001 '+side+' '+ending for side in ('L','R') for ending in ('UpperArm','Forearm')]
poses = []
for frame in range(1,242):
    for (start,a),(end,b) in zip(stages,stages[1:]):
        if frame<=end:
            t = (frame-start)/(end-start)
            kind,amount = (a,1-t) if b=='rest' else (b,t)
            review.pose(scene,rig,kind,amount)
            poses.append({name:rig.pose.bones[name].rotation_quaternion.copy() for name in names})
            break
for frame,values in enumerate(poses,1):
    for name,rotation in values.items():
        bone = rig.pose.bones[name]
        bone.rotation_quaternion = rotation
        bone.keyframe_insert(data_path='rotation_quaternion',frame=frame)
for curve in rig.animation_data.action.fcurves:
    for key in curve.keyframe_points:
        key.interpolation = 'LINEAR'
for frame,target,scale in ((1,(0,0,1.60),.60),(161,(0,0,1.60),.60),(162,(0,-.10,1.28),1.55)):
    cam.rotation_euler = (Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale = scale
    cam.keyframe_insert(data_path='rotation_euler',frame=frame)
    cam.data.keyframe_insert(data_path='ortho_scale',frame=frame)
for owner in (cam,cam.data):
    for curve in owner.animation_data.action.fcurves:
        for key in curve.keyframe_points:
            key.interpolation = 'CONSTANT'
scene.render.image_settings.file_format = 'FFMPEG'
scene.render.ffmpeg.format,scene.render.ffmpeg.codec = 'MPEG4','H264'
scene.render.ffmpeg.constant_rate_factor = 'MEDIUM'
scene.render.ffmpeg.ffmpeg_preset = 'GOOD'
scene.render.filepath = str(root/'docs/art/hound/mesh-v11/anatomy-check.mp4')
bpy.ops.render.render(animation=True)
print('H03_ANATOMY_VIDEO',241,'frames',30,'fps')
