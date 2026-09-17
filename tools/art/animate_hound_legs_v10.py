'''H02 support video; temporary sampled poses never replace the retained diagnostic action.'''
import bpy
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h02_review as review

root = Path(__file__).resolve().parents[2]
source = bpy.data.scenes['Hound_Mesh_v10']
original,original_rig = source.objects['H10_DeformMesh'],source.objects['Hound10_Rig']
scene = bpy.data.scenes.new('H02_TemporaryVideo')
bpy.context.window.scene = scene
rig = original_rig.copy()
rig.data = original_rig.data.copy()
rig.animation_data_clear()
scene.collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.parent,model.modifiers[0].object = rig,rig
scene.collection.objects.link(model)
review.setup(scene,900,1100)
review.floor(scene)
review.camera(scene,(2,-4,1.4),(0,0,.61),1.48)
scene.render.fps,scene.frame_start,scene.frame_end = 30,1,181
stages = [(1,'rest'),(31,'knee'),(46,'rest'),(76,'step'),(91,'rest'),(116,'toe'),(131,'rest'),(156,'heel'),(181,'rest')]
names = ['Bip001']+['Bip001 '+side+' '+ending for side in ('L','R') for ending in ('Thigh','Calf','Foot')]
names += ['Hound L HipGuard','Hound R HipGuard']
poses = []
for frame in range(1,182):
    for (start,a),(end,b) in zip(stages,stages[1:]):
        if frame<=end:
            t = (frame-start)/(end-start)
            kind,amount = (a,1-t) if b=='rest' else (b,t)
            review.pose(scene,rig,model,kind,amount)
            poses.append({name:(rig.pose.bones[name].rotation_quaternion.copy(),rig.pose.bones[name].location.copy()) for name in names})
            break
for frame,values in enumerate(poses,1):
    for name,(rotation,location) in values.items():
        bone = rig.pose.bones[name]
        bone.rotation_quaternion,bone.location = rotation,location
        bone.keyframe_insert(data_path='rotation_quaternion',frame=frame)
        if name=='Bip001':
            bone.keyframe_insert(data_path='location',frame=frame)
for curve in rig.animation_data.action.fcurves:
    for key in curve.keyframe_points:
        key.interpolation = 'LINEAR'
# Hide the upper torso from the review only; preserve the complete mesh in the source/export.
import bmesh
groups = {g.index for g in model.vertex_groups if g.name[5:] in review.leg_parts(model)}
removed = {v.index for v in model.data.vertices if not any(w.group in groups for w in v.groups)}
bm = bmesh.new()
bm.from_mesh(model.data)
bm.verts.ensure_lookup_table()
bmesh.ops.delete(bm,geom=[bm.verts[i] for i in removed],context='VERTS')
bm.to_mesh(model.data)
bm.free()
scene.render.image_settings.file_format = 'FFMPEG'
scene.render.ffmpeg.format,scene.render.ffmpeg.codec = 'MPEG4','H264'
scene.render.ffmpeg.constant_rate_factor = 'MEDIUM'
scene.render.ffmpeg.ffmpeg_preset = 'GOOD'
scene.render.filepath = str(root/'docs/art/hound/mesh-v10/leg-check.mp4')
bpy.ops.render.render(animation=True)
print('H02_LEG_VIDEO',181,'frames',30,'fps')
