'''Real H05 hood evidence: before/after views and a temporary neck/upper-body motion review.'''
import bpy
import sys
import math
from pathlib import Path
from mathutils import Matrix,Vector
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h01_review as render
import hound_h04_review as assembly
import hound_h03_review as anatomy
root = Path(__file__).resolve().parents[2]
output = root/'docs/art/hound/mesh-v15'
output.mkdir(parents=True,exist_ok=True)
scene = bpy.data.scenes['Hound_Mesh_v15']
model,rig = scene.objects['H15_DeformMesh'],scene.objects['Hound15_Rig']
rig.animation_data.action = None
def pose(kind,amount=1):
    assembly.pose(scene,rig,'rest')
    if kind in ('look-up','look-down'):
        anatomy.pose(scene,rig,kind,amount)
    else:
        assembly.pose(scene,rig,kind,amount)
pose('rest')
render.setup(scene,1000,1100)
cam = render.camera(scene,(1,-4,1.7),(0,-.02,1.58),.60)
def view(name,position,target,scale):
    cam.location = position
    cam.rotation_euler = (Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale = scale
    render.render(scene,output/(name+'.png'))
if '--video' in sys.argv:
    segments = ('head-left','head-right','look-up','look-down','reach','twist-left','twist-right')
    poses = []
    for frame in range(1,len(segments)*30+2):
        segment = min(len(segments)-1,(frame-1)//30)
        pose(segments[segment],math.sin(math.pi*((frame-1)-segment*30)/30))
        poses.append({b.name:b.rotation_quaternion.copy() for b in rig.pose.bones})
    for frame,values in enumerate(poses,1):
        for name,rotation in values.items():
            rig.pose.bones[name].rotation_quaternion = rotation
            rig.pose.bones[name].keyframe_insert(data_path='rotation_quaternion',frame=frame)
    for curve in rig.animation_data.action.fcurves:
        for key in curve.keyframe_points:
            key.interpolation = 'LINEAR'
    scene.render.resolution_x,scene.render.resolution_y = 900,1000
    scene.render.fps,scene.frame_start,scene.frame_end = 30,1,len(poses)
    cam.location = (1,-4,1.7)
    cam.rotation_euler = (Vector((0,0,1.53))-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale = .82
    scene.render.image_settings.file_format = 'FFMPEG'
    scene.render.ffmpeg.format,scene.render.ffmpeg.codec = 'MPEG4','H264'
    scene.render.ffmpeg.constant_rate_factor = 'MEDIUM'
    scene.render.filepath = str(output/'hood-check.mp4')
    bpy.ops.render.render(animation=True)
    print('H15_VIDEO',len(poses))
    raise SystemExit
for name,position in [('front',(0,-4,1.59)),('profile',(4,0,1.59)),('back',(0,4,1.59)),('three-quarter',(1,-4,1.7))]:
    view('hood-'+name,position,(0,0,1.58),.60)
view('full',(2.8,-5,2.2),(0,0,.91),2.1)
board = bpy.data.scenes.new('H15_HoodComparison')
render.setup(board,2400,1350)
for row,version in enumerate(('14','15')):
    src = bpy.data.scenes['Hound_Mesh_v'+version]
    obj,arm = src.objects['H'+version+'_DeformMesh'],src.objects['Hound'+version+'_Rig']
    arm.animation_data.action = None
    assembly.pose(src,arm,'rest')
    for col,(angle,title) in enumerate(((0,'FRENTE'),(.58,'TRES CUARTOS'),(math.pi/2,'PERFIL'))):
        x,z = (col-1)*.67,(1-row)*.63
        names = {n for n in assembly.parts(obj) if n.startswith(('Hood_','Head_','Eye_','Mouth_','Neck_',
                 'Collar_plate_','Breastplate_','Sternum'))}
        render.snapshot(src,obj,board,Matrix.Translation((x,0,z))@Matrix.Rotation(angle,4,'Z'),names)
        render.label(board,'V'+version+' / '+title,x,z+1.25,.023)
render.camera(board,(0,-6,1.85),(0,0,1.85),2.15)
render.render(board,output/'hood-before-after.png')
board = bpy.data.scenes.new('H15_Turnaround')
render.setup(board,2600,1200)
pose('rest')
for i,(angle,title) in enumerate(((0,'FRENTE'),(math.pi/2,'PERFIL'),(math.pi,'ESPALDA'),(.65,'TRES CUARTOS'))):
    x = (i-1.5)*1.23
    render.snapshot(scene,model,board,Matrix.Translation((x,0,0))@Matrix.Rotation(angle,4,'Z'))
    render.label(board,title,x,-.12,.047)
render.label(board,'H05 / V15 / CAIDA ABIERTA DE CAPUCHA',0,2.00,.063)
render.camera(board,(0,-6,.96),(0,0,.96),5.15)
render.render(board,output/'turnaround.png')
for kind in ('head-left','head-right','look-up','look-down','reach','twist-left'):
    pose(kind)
    view(kind,(1,-4,1.7),(0,0,1.52),.77)
print('H15_STILLS')
