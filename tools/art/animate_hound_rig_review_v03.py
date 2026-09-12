'''Render the diagnostic joint clip, not a final locomotion or attack animation.'''
import bpy
import sys
from mathutils import Vector

version = sys.argv[sys.argv.index('--')+1] if '--' in sys.argv else 'v03'
assert version in ('v03', 'v04', 'v05')
scene = bpy.data.scenes['Hound_Rig_v03' if version == 'v03' else 'Hound_Mesh_'+version]
bpy.context.window.scene = scene
scene.frame_start, scene.frame_end = 1, 181
scene.render.fps = 30
scene.render.resolution_x, scene.render.resolution_y = 640, 800
scene.render.resolution_percentage = 100
scene.camera.location = (2.8, -5, 2.2)
scene.camera.rotation_euler = (Vector((0, 0, .91))-scene.camera.location).to_track_quat('-Z', 'Y').to_euler()
scene.camera.data.ortho_scale = 2.08
scene.render.image_settings.file_format = 'FFMPEG'
scene.render.ffmpeg.format = 'MPEG4'
scene.render.ffmpeg.codec = 'H264'
scene.render.ffmpeg.constant_rate_factor = 'MEDIUM'
scene.render.ffmpeg.ffmpeg_preset = 'GOOD'
folder = 'rig-v03' if version == 'v03' else 'mesh-'+version
scene.render.filepath = 'D:/Projects/Gloom/docs/art/hound/'+folder+'/joint-check.mp4'
bpy.ops.render.render(animation=True)
print('HOUND_JOINT_CHECK_VIDEO_RENDERED', version)
