'''Short H01 articulation video using a temporary action; the source diagnostic clip is never saved over.'''
import bpy
import bmesh
import math
import sys
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h01_review as review

root = Path(__file__).resolve().parents[2]
source = bpy.data.scenes['Hound_Mesh_v09']
original,original_rig = source.objects['H09_DeformMesh'],source.objects['Hound09_Rig']
scene = bpy.data.scenes.new('H01_TemporaryVideo')
bpy.context.window.scene = scene
rig = original_rig.copy()
rig.data = original_rig.data.copy()
rig.animation_data_clear()
scene.collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.parent,model.modifiers[0].object = rig,rig
scene.collection.objects.link(model)
names = review.hand_parts(model,'R')
keep_groups = {g.index for g in model.vertex_groups if g.name[5:] in names}
removed = {v.index for v in model.data.vertices if not any(w.group in keep_groups for w in v.groups)}
bm = bmesh.new()
bm.from_mesh(model.data)
bm.verts.ensure_lookup_table()
bmesh.ops.delete(bm,geom=[bm.verts[i] for i in removed],context='VERTS')
bm.to_mesh(model.data)
bm.free()
review.setup(scene,800,1000)
scene.render.fps,scene.frame_start,scene.frame_end = 30,1,151
wrist = rig.data.bones['Bip001 R Hand'].head_local.copy()
target = wrist+Vector((0,0,.045))
cam = review.camera(scene,target+Vector((.9,-.55,.20)),target,.66)
# Read temporary poses before creating the temporary action so no original keys can be altered.
poses = []
for frame,kind in ((1,'open'),(31,'fist'),(51,'open'),(76,'flex'),(101,'extend'),(126,'grip'),(151,'open')):
    review.pose(scene,rig,kind)
    poses.append((frame,{p.name:p.rotation_quaternion.copy() for p in rig.pose.bones}))
for frame,rotations in poses:
    for name,rotation in rotations.items():
        if name.startswith('Bip001 R ') and ('Finger' in name or name.endswith('Hand')):
            bone = rig.pose.bones[name]
            bone.rotation_mode = 'QUATERNION'
            bone.rotation_quaternion = rotation
            bone.keyframe_insert(data_path='rotation_quaternion',frame=frame)
for curve in rig.animation_data.action.fcurves:
    for key in curve.keyframe_points:
        key.interpolation = 'LINEAR'
scene.render.image_settings.file_format = 'FFMPEG'
scene.render.ffmpeg.format,scene.render.ffmpeg.codec = 'MPEG4','H264'
scene.render.ffmpeg.constant_rate_factor = 'MEDIUM'
scene.render.ffmpeg.ffmpeg_preset = 'GOOD'
scene.render.filepath = str(root/'docs/art/hound/mesh-v09/hand-check.mp4')
bpy.ops.render.render(animation=True)
print('H01_HAND_VIDEO',151,'frames',30,'fps')
