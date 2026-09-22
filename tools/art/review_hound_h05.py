'''H05: freeze the unchanged sculpture, inspect global contacts, and render review evidence.
Python --freeze; Blender on the frozen .blend: --audit, --stills or --video.
Review poses/cameras/weapon are temporary and never saved into the master.
'''
import sys
import json
import math
import hashlib
import shutil
from pathlib import Path

root = Path(__file__).resolve().parents[2]
version = next((v for v in ('v16','v15','v14') if '--'+v in sys.argv),'v13')
v14 = version!='v13'
output = root/('docs/art/hound/mesh-'+version if v14 else 'docs/art/hound/review-h05')
cache = root/({'v16':'.cache/hound-armor-v16','v15':'.cache/hound-hood-v15','v14':'.cache/hound-mesh-v14'}.get(version,'.cache/hound-h05'))
output.mkdir(parents=True,exist_ok=True)
cache.mkdir(parents=True,exist_ok=True)
manifest_path = root/'art/characters/hound/v13/sculpture-reference.json'

if '--freeze' in sys.argv:
    assert not v14, 'v14 is modeled, never frozen from v12'
    pairs = [
        ('art/characters/hound/v12/hound-mesh-v12.blend','art/characters/hound/v13/hound-sculpture-v13.blend'),
        ('assets/characters/hound_rig/v12/hound-rig.gltf','assets/characters/hound_rig/v13/hound-rig.gltf'),
        ('assets/characters/hound_rig/v12/hound-rig.bin','assets/characters/hound_rig/v13/hound-rig.bin')]
    assert not manifest_path.exists() and all(not (root/b).exists() for a,b in pairs), 'Never overwrite a reference candidate'
    files = []
    for source,destination in pairs:
        (root/destination).parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(root/source,root/destination)
        digest = hashlib.sha256((root/source).read_bytes()).hexdigest()
        assert hashlib.sha256((root/destination).read_bytes()).hexdigest()==digest
        files.append({'source':source,'path':destination,'sha256':digest,'bytes':(root/destination).stat().st_size})
    manifest_path.write_text(json.dumps({'task':'H05','milestone':95,'candidate':'v13','source_commit':'0643723',
        'geometry_changes':False,'internal_scene':'Hound_Mesh_v12','mesh':'H12_DeformMesh','rig':'Hound12_Rig',
        'action':'Hound12_joint_check','collection':'HOUND_v12_EXPORT',
        'policy':'Immutable candidate; approval is recorded in the task index, never inferred from this snapshot.',
        'files':files},indent=2)+'\n',encoding='utf-8')
    print('H05_FROZEN',json.dumps(files))
    raise SystemExit

import bpy
import bmesh
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h01_review as hands
import hound_h03_review as anatomy
import hound_h04_review as assembly

manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
for item in manifest['files']:
    assert hashlib.sha256((root/item['path']).read_bytes()).hexdigest()==item['sha256'],item['path']
internal = version[1:] if v14 else '12'
scene = bpy.data.scenes['Hound_Mesh_v'+internal]
model,rig = scene.objects['H'+internal+'_DeformMesh'],scene.objects['Hound'+internal+'_Rig']
bpy.context.window.scene = scene
scene.frame_set(1)
rig.animation_data.action = None
parts = assembly.parts(model)
owner = {i:name for name,ids in parts.items() for i in ids}
assert len(owner)==len(model.data.vertices)
faces = {name:[] for name in parts}
for polygon in model.data.polygons:
    assert len({owner[i] for i in polygon.vertices})==1
    faces[owner[polygon.vertices[0]]].append(list(polygon.vertices))

CASES = ('rest','reach','twist-left','twist-right','step-left','step-right','hip-left','hip-right',
         'head-left','head-right','fist','flex','extend','look-up','look-down','elbow','combined')

def pose(kind,amount=1):
    assembly.pose(scene,rig,'rest')
    assert all(b.matrix_basis==Matrix.Identity(4) for b in rig.pose.bones), 'Reset every pose independently'
    if kind in ('fist','flex','extend','grip'):
        hands.pose(scene,rig,kind,amount)
    elif kind in ('look-up','look-down','elbow'):
        anatomy.pose(scene,rig,kind,amount)
    elif kind=='combined':
        assembly.pose(scene,rig,'twist-left',amount)
        saved = {b.name:b.rotation_quaternion.copy() for b in rig.pose.bones if b.name in ('Bip001 Spine','Bip001 Spine1')}
        assembly.pose(scene,rig,'step-left',amount)
        for name,rotation in saved.items():
            rig.pose.bones[name].rotation_quaternion = rotation
        bpy.context.view_layer.update()
    else:
        assembly.pose(scene,rig,kind,amount)

def points_and_trees():
    evaluated = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    points = [model.matrix_world@v.co for v in mesh.vertices]
    assert all(all(math.isfinite(c) for c in p) for p in points)
    assert min(p.area for p in mesh.polygons)>1e-12
    evaluated.to_mesh_clear()
    return points,{n:BVHTree.FromPolygons(points,f) for n,f in faces.items()}

if '--audit' in sys.argv:
    pose('rest')
    topology = {}
    for name,ids in parts.items():
        bm = bmesh.new()
        vertices = {i:bm.verts.new(model.data.vertices[i].co) for i in ids}
        for face in faces[name]:
            bm.faces.new([vertices[i] for i in face])
        pending,visited = [next(iter(bm.verts))],set()
        while pending:
            vertex = pending.pop()
            if vertex in visited:
                continue
            visited.add(vertex)
            pending.extend(e.other_vert(vertex) for e in vertex.link_edges if e.other_vert(vertex) not in visited)
        topology[name] = {'connected':len(visited)==len(bm.verts),
            'nonmanifold_edges':sum(not e.is_manifold for e in bm.edges),
            'inconsistent_edges':sum(not e.is_contiguous for e in bm.edges),
            'euler':len(bm.verts)-len(bm.edges)+len(bm.faces),'volume_m3':bm.calc_volume(signed=True)}
        assert topology[name]['connected'] and not topology[name]['nonmanifold_edges'] and not topology[name]['inconsistent_edges'],name
        assert topology[name]['volume_m3']>0,name
        bm.free()
    maximum = 0
    for vertex in model.data.vertices:
        weights = [w.weight for w in vertex.groups if model.vertex_groups[w.group].name in rig.data.bones]
        assert weights and all(0<w<=1 for w in weights) and abs(sum(weights)-1)<2e-6
        maximum = max(maximum,len(weights))
    assert maximum<=2
    names = sorted(parts)
    pairs = [(a,b) for i,a in enumerate(names) for b in names[i+1:]]
    cases = [('rest',1)]+[(kind,amount) for kind in CASES[1:] for amount in (.5,1)]
    contacts,self_crossings = {},{}
    for kind,amount in cases:
        pose(kind,amount)
        points,trees = points_and_trees()
        tag = kind if amount==1 else kind+'-half'
        hits = {}
        for a,b in pairs:
            count = len(trees[a].overlap(trees[b]))
            if count:
                hits[a+'/'+b] = count
        contacts[tag] = hits
        self_crossings[tag] = {}
        for name in names:
            count = sum(a<b and not set(faces[name][a]).intersection(faces[name][b]) for a,b in trees[name].overlap(trees[name]))
            if count:
                self_crossings[tag][name] = count
    pose('grip')
    gun = hands.weapon(scene,rig)
    gun_base = gun.matrix_world.copy()
    hand_base = rig.pose.bones['Bip001 R Hand'].matrix.copy()
    weapon = {}
    # Two body contexts, same diagnostic support attachment, never an approved grip.
    for kind in ('grip','reach'):
        if kind=='reach':
            pose('reach')
            body = {b.name:b.rotation_quaternion.copy() for b in rig.pose.bones if 'Finger' not in b.name}
            pose('grip')
            for name,rotation in body.items():
                rig.pose.bones[name].rotation_quaternion = rotation
            bpy.context.view_layer.update()
        gun.matrix_world = rig.matrix_world@rig.pose.bones['Bip001 R Hand'].matrix@hand_base.inverted()@rig.matrix_world.inverted()@gun_base
        points,trees = points_and_trees()
        tree = BVHTree.FromPolygons([gun.matrix_world@v.co for v in gun.data.vertices],[list(p.vertices) for p in gun.data.polygons])
        weapon[kind] = {n:len(tree.overlap(trees[n])) for n in names}
    result = {'poses':len(cases),'parts':len(parts),'pairs_per_pose':len(pairs),'pair_evaluations':len(cases)*len(pairs),
        'max_influences':maximum,'topology':topology,'surface_crossings':contacts,'self_crossings':self_crossings,
        'weapon_crossings':weapon,'scope':'Discrete surface crossings, not containment, clearance or continuous collision certification.'}
    (cache/'audit.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    if v14:
        for tag,row in self_crossings.items():
            assert row==({'Arm_surface_L':37,'Arm_surface_R':37} if tag=='elbow' else {}), (tag,row)
        clear_pairs = []
        for suffix in ('L','R'):
            clear_pairs += [('Arm_surface_'+suffix,'Collar_plate_'+suffix),
                            ('Arm_surface_'+suffix,'Collar_blade_middle_'+suffix),
                            ('Arm_surface_'+suffix,'Rib_flank_'+suffix),
                            ('Back_blade_'+suffix,'Hood_continuous'),
                            ('Bracer_blade_distal_'+suffix,'Hand_back_full_point_'+suffix)]
        for tag,row in contacts.items():
            for pair in clear_pairs:
                assert row.get('/'.join(sorted(pair)),0)==0, (tag,pair)
    print('H05_AUDIT',len(cases),len(pairs),len(cases)*len(pairs),maximum)
    print('SELF_CROSSINGS',json.dumps(self_crossings))
    print('WEAPON_CROSSINGS',json.dumps({k:{n:v for n,v in row.items() if v} for k,row in weapon.items()}))
    raise SystemExit

hands.setup(scene,1000,1100)
cam = hands.camera(scene,(2.8,-5,2.2),(0,0,.91),2.10)

def view(name,position,target,scale):
    cam.location = position
    cam.rotation_euler = (Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale = scale
    hands.render(scene,output/(name+'.png'))

if '--stills' in sys.argv:
    pose('rest')
    board = bpy.data.scenes.new('H05_Turnaround')
    hands.setup(board,2600,1200)
    for i,(angle,title) in enumerate(((0,'FRENTE'),(math.pi/2,'PERFIL'),(math.pi,'ESPALDA'),(.65,'TRES CUARTOS'))):
        x = (i-1.5)*1.23
        hands.snapshot(scene,model,board,Matrix.Translation((x,0,0))@Matrix.Rotation(angle,4,'Z'))
        hands.label(board,title,x,-.12,.047)
    hands.label(board,'H05 / ESCULTURA '+version.upper()+' / CANDIDATA A APROBACION',0,2.00,.063)
    hands.camera(board,(0,-6,.96),(0,0,.96),5.15)
    hands.render(board,output/'turnaround.png')
    for name,position,target,scale in [
        ('face',(.5,-4,1.7),(0,-.02,1.61),.46),('collar',(2,-4,2),(.03,0,1.47),.94),
        ('hands',(3,-3,1.15),(.47,-.025,.9),.74),('boots',(2,-4,.48),(0,-.04,.25),.79),
        ('waist',(1,-4,1.2),(0,0,.91),.78),('back-assembly',(1,4,1.6),(0,0,1.34),.98)]:
        view(name,position,target,scale)
    with bpy.data.libraries.load(str(root/'art/characters/hound/v02/hound-blockout-v02.blend'),link=False) as (src,dst):
        dst.scenes = ['Hound_Blockout_v02']
    approved = dst.scenes[0]
    board = bpy.data.scenes.new('H05_ApprovedComparison')
    hands.setup(board,1600,1200)
    bpy.context.window.scene = approved
    approved.frame_set(1)
    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    for obj in approved.objects:
        if obj.type=='MESH':
            evaluated = obj.evaluated_get(depsgraph)
            copy = bpy.data.objects.new('Approved_'+obj.name,bpy.data.meshes.new_from_object(evaluated))
            copy.matrix_world = Matrix.Translation((-.64,0,0))@evaluated.matrix_world
            board.collection.objects.link(copy)
    hands.snapshot(scene,model,board,Matrix.Translation((.64,0,0)))
    hands.label(board,'V02 / DISENO APROBADO',-.64,-.12,.044)
    hands.label(board,version.upper()+' / ESCULTURA EN REVISION',.64,-.12,.044)
    hands.camera(board,(0,-5,.93),(0,0,.93),2.9)
    hands.render(board,output/'approved-comparison.png')
    if v14:
        previous = {'v16':'15','v15':'14','v14':'12'}[version]
        old_scene = bpy.data.scenes['Hound_Mesh_v'+previous]
        old_model,old_rig = old_scene.objects['H'+previous+'_DeformMesh'],old_scene.objects['Hound'+previous+'_Rig']
        old_rig.animation_data.action = None
        assembly.pose(old_scene,old_rig,'rest')
        board = bpy.data.scenes.new('H05_BeforeAfter')
        hands.setup(board,2600,2200)
        for row,(src,obj,caption) in enumerate(((old_scene,old_model,'V13' if previous=='12' else 'V'+previous),(scene,model,version.upper()))):
            for i,(angle,title) in enumerate(((0,'FRENTE'),(math.pi/2,'PERFIL'),(math.pi,'ESPALDA'),(.65,'TRES CUARTOS'))):
                x,z = (i-1.5)*1.23,(1-row)*2.1
                hands.snapshot(src,obj,board,Matrix.Translation((x,0,z))@Matrix.Rotation(angle,4,'Z'))
                hands.label(board,caption+' / '+title,x,z-.1,.048)
        hands.camera(board,(0,-6,1.95),(0,0,1.95),5.15)
        hands.render(board,output/'before-after.png')
        for name,position,target,scale in [
            ('torso',(1,-5,1.4),(0,0,1.34),.88),('claws-palm',( -3,-2,1.08),(.455,-.03,.856),.34),
            ('claws-dorsal',(3,1,1),(.455,-.03,.856),.34),('claws-side',(0,-5,.95),(.455,-.03,.856),.34),
            ('bracer-side',(3,-3,1.3),(.445,0,1.12),.49),('rear-blades',(1,4,1.9),(0,.10,1.58),.9)]:
            view(name,position,target,scale)
    if version=='v16':
        board = bpy.data.scenes.new('H16_WrappingArmorComparison')
        hands.setup(board,2400,1350)
        for row,(src,obj,caption) in enumerate(((old_scene,old_model,'V15'),(scene,model,'V16'))):
            names = {n for n in assembly.parts(obj) if n.startswith(('Torso_','Breastplate_','Sternum','Abdominal_','Rib_','Scapula_','Back_spine_','Waist_'))}
            for col,(angle,title) in enumerate(((0,'FRENTE'),(math.pi/2,'COSTADO'),(math.pi,'ESPALDA'))):
                x,z = (col-1)*.64,(1-row)*.59
                hands.snapshot(src,obj,board,Matrix.Translation((x,0,z))@Matrix.Rotation(angle,4,'Z'),names)
                hands.label(board,caption+' / '+title,x,z+.948,.023)
        hands.camera(board,(0,-5,1.525),(0,0,1.525),2.07)
        hands.render(board,output/'armor-wrap-comparison.png')
        for name,position in (('armor-front',(0,-5,1.32)),('armor-side',(5,0,1.32)),
                              ('armor-back',(0,5,1.32)),('armor-rear-quarter',(2,4,1.4))):
            view(name,position,(0,0,1.30),.85)
    for kind in ('reach','combined','elbow','fist','look-up','look-down'):
        pose(kind)
        view('pose-'+kind,(2,-4,1.8),(0,0,.95),2.15)
    pose('look-down')
    view('limit-neck',(1,-4,1.65),(0,0,1.45),.64)
    pose('elbow')
    view('limit-elbow',(3,2,1.5),(.39,-.05,1.28),.58)
    pose('grip')
    gun = hands.weapon(scene,rig)
    view('weapon-full',(2,-4,1.8),(0,0,.91),2.15)
    view('weapon-contact',(-3,-4,1.2),(-.43,-.09,.82),.90)
    bpy.data.objects.remove(gun,do_unlink=True)
    pose('rest')
    scene.render.film_transparent = True
    scene.display.shading.color_type = 'SINGLE'
    scene.display.shading.single_color = (0,0,0)
    scene.display.shading.light = 'FLAT'
    scene.display.shading.show_cavity = False
    for name,position in (('silhouette-front',(0,-5,.91)),('silhouette-profile',(5,0,.91)),('silhouette-back',(0,5,.91))):
        view(name,position,(0,0,.91),2.10)
    print('H05_STILLS')
    raise SystemExit

if '--video' in sys.argv:
    # Explicit temporary poses at 30 fps; the immutable diagnostic action is untouched.
    segments = ('reach','twist-left','twist-right','step-left','step-right','fist','flex','head-left','head-right','look-down','combined')
    poses = []
    for frame in range(1,len(segments)*30+2):
        segment = min(len(segments)-1,(frame-1)//30)
        amount = math.sin(math.pi*((frame-1)-segment*30)/30)
        pose(segments[segment],amount)
        poses.append({b.name:b.rotation_quaternion.copy() for b in rig.pose.bones})
    for frame,values in enumerate(poses,1):
        for name,rotation in values.items():
            rig.pose.bones[name].rotation_quaternion = rotation
            rig.pose.bones[name].keyframe_insert(data_path='rotation_quaternion',frame=frame)
    for curve in rig.animation_data.action.fcurves:
        for key in curve.keyframe_points:
            key.interpolation = 'LINEAR'
    for frame,position,target,scale in [
        (1,(2,-4,1.8),(0,0,.95),2.15),(61,(2,4,1.8),(0,0,.95),2.15),
        (91,(3,-4,1.4),(0,0,.83),1.94),(151,(2,-4,1.3),(.46,-.02,.92),.86),
        (211,(.5,-4,1.8),(0,0,1.54),.73),(301,(2,-4,1.8),(0,0,.95),2.15)]:
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
    scene.render.resolution_x,scene.render.resolution_y = 900,1000
    scene.render.fps,scene.frame_start,scene.frame_end = 30,1,len(poses)
    scene.render.image_settings.file_format = 'FFMPEG'
    scene.render.ffmpeg.format,scene.render.ffmpeg.codec = 'MPEG4','H264'
    scene.render.ffmpeg.constant_rate_factor = 'MEDIUM'
    scene.render.filepath = str(output/'global-check.mp4')
    bpy.ops.render.render(animation=True)
    print('H05_VIDEO',len(poses))
