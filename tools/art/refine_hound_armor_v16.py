'''H05: wrap the torso cuirass around the flanks and back while preserving the accepted front and hood.'''
import bpy
import bmesh
import json
import hashlib
import sys
import math
from mathutils import Vector
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from hound_h04_review import parts
from hound_h01_review import setup,camera

root = Path(__file__).resolve().parents[2]
source = bpy.data.scenes['Hound_Mesh_v15']
for item in json.loads((root/'art/characters/hound/v15/sculpture-reference.json').read_text())['files']:
    assert hashlib.sha256((root/item['path']).read_bytes()).hexdigest()==item['sha256']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original,old_rig = source.objects['H15_DeformMesh'],source.objects['Hound15_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v16')
for key in source.keys():
    scene[key] = source[key]
scene.unit_settings.system,scene.unit_settings.scale_length = 'METRIC',1
scene.render.fps,scene.frame_start,scene.frame_end = 30,1,181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name,frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v16_EXPORT')
scene.collection.children.link(collection)
rig = old_rig.copy()
rig.data = old_rig.data.copy()
rig.name,rig.data.name = 'Hound16_Rig','Hound16_Skeleton'
rig.animation_data.action = old_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound16_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name,model.data.name = 'H16_DeformMesh','H16_DeformTopology'
model.parent,model.modifiers[0].object = rig,rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound15_','Hound16_')
bpy.context.window.scene = scene


parts_map = parts(model)
rigid = json.loads(source['gloom_rigid_parts'])
# The new middle dorsal plate follows the same diagnostic bone as its lateral band.
rigid['Back_spine_1'] = 'Bip001 Spine1'
rebuilt,generated = set(),[]
iron = next(i for i,m in enumerate(model.data.materials) if 'Iron' in m.name)
edge = next(i for i,m in enumerate(model.data.materials) if 'EdgePlanes' in m.name)

def surface(name,vertices,faces,accents):
    data = bpy.data.meshes.new('H16_'+name)
    data.from_pydata(vertices,[],faces)
    for mat in model.data.materials:
        data.materials.append(mat)
    for face,accent in zip(data.polygons,accents):
        face.material_index,face.use_smooth = edge if accent else iron,True
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    assert all(e.is_manifold and e.is_contiguous for e in bm.edges),name
    if bm.calc_volume(signed=True)<0:
        bmesh.ops.reverse_faces(bm,faces=list(bm.faces))
    assert bm.calc_volume(signed=True)>0,name
    bm.to_mesh(data)
    bm.free()
    data.set_sharp_from_angle(angle=math.radians(46))
    obj = bpy.data.objects.new(data.name,data)
    collection.objects.link(obj)
    obj.vertex_groups.new(name='PART_'+name).add(list(range(len(vertices))),1,'REPLACE')
    obj.vertex_groups.new(name=rigid[name]).add(list(range(len(vertices))),1,'REPLACE')
    generated.append(obj)
    rebuilt.add(name)

def interpolate(values,t):
    for a,b in zip(values,values[1:]):
        if t<=b[0]:
            f = max(0,min(1,(t-a[0])/(b[0]-a[0])))
            return a[1]+(b[1]-a[1])*f
    return values[-1][1]

def radius(z):
    return interpolate(((1.06,.181),(1.13,.193),(1.187,.211),(1.243,.229),
                        (1.30,.246),(1.343,.252),(1.387,.257),(1.45,.263)),z)

def panel(name,outline):
    # Thin curved rear plate, retaining the existing names/bone assignments.
    points = [Vector((x,0,z)) for x,z in outline]
    border = []
    for i,p in enumerate(points):
        a,b,c = points[i-1],points[(i+1)%len(points)],points[(i+2)%len(points)]
        for j in range(3):
            t = j/3
            border.append(.5*(2*p+(-a+b)*t+(2*a-5*p+4*b-c)*t*t+(-a+3*p-3*b+c)*t*t*t))
    center = sum(border,Vector())/len(border)
    vertices,faces,accents = [],[],[]
    for r in (1,.976,.91,.64,.30):
        for p in border:
            q = center+(p-center)*r
            q.y = .167*math.sqrt(max(.05,1-(q.x/(radius(q.z)+.006))**2))+.002*(1-r*r)+.003*math.exp(-(q.x/.035)**2)
            vertices.append(q)
    q = center.copy()
    q.y = .169*math.sqrt(max(.05,1-(q.x/(radius(q.z)+.006))**2))+.003*math.exp(-(q.x/.035)**2)
    vertices.append(q)
    n = len(border)
    for row in range(4):
        for i in range(n):
            faces.append((row*n+i,row*n+(i+1)%n,(row+1)*n+(i+1)%n,(row+1)*n+i))
            accents.append(row==0)
    for i in range(n):
        faces.append((4*n+i,4*n+(i+1)%n,len(vertices)-1))
        accents.append(False)
    layer = len(vertices)
    vertices += [p-Vector((0,.004,0)) for p in list(vertices)]
    faces += [tuple(i+layer for i in f) for f in list(faces)]
    accents += list(accents)
    for i in range(n):
        faces.append((i,(i+1)%n,layer+(i+1)%n,layer+i))
        accents.append(False)
    surface(name,vertices,faces,accents)

def flank(name,side,row):
    # A ruled curved plate follows the torso ellipse. The upper rim dips under the armpit.
    angles = [30,31,34]+list(range(38,150,4))+[150,153,154]
    levels = (0,.035,.11,.35,.65,.89,.965,1)
    vertices,faces,accents = [],[],[]
    for inner in (False,True):
        for v in levels:
            for degrees in angles:
                a = math.radians(degrees)
                if row==0:
                    high = interpolate(((30,1.357),(52,1.343),(62,1.308),(78,1.268),(90,1.257),
                                        (102,1.283),(124,1.405),(140,1.434),(152,1.368)),degrees)
                    low = interpolate(((42,1.267),(70,1.250),(90,1.241),(110,1.25),(152,1.275)),degrees)
                else:
                    low = (1.173 if row==1 else 1.084)+.004*math.cos(a*2)
                    high = (interpolate(((42,1.267),(70,1.250),(90,1.241),(110,1.25),(152,1.275)),degrees)-.006
                            if row==1 else 1.167+.004*math.cos(a*2))
                z = low+(high-low)*v
                inset = .004 if inner else 0
                raised = .002*math.sin(math.pi*v)**2
                rx = radius(z)-inset+raised
                ry = .153-inset+raised
                vertices.append(Vector((side*rx*math.sin(a),-ry*math.cos(a),z)))
    n,m = len(angles),len(levels)
    layer = n*m
    for offset in (0,layer):
        for j in range(m-1):
            for i in range(n-1):
                a = offset+j*n+i
                faces.append((a,a+1,a+n+1,a+n))
                accents.append(j in (0,m-2) or i in (0,n-2))
    rim = list(range(n))+[j*n+n-1 for j in range(1,m)]+list(range(layer-2,layer-n-1,-1))+[j*n for j in range(m-2,0,-1)]
    for i,a in enumerate(rim):
        b = rim[(i+1)%len(rim)]
        faces.append((a,b,b+layer,a+layer))
        accents.append(False)
    surface(name,vertices,faces,accents)

# Scapular plates cover the rear chest; preserve the old side naming rather than renaming historical groups.
for side,suffix in ((-1,'L'),(1,'R')):
    outline = [(side*x,z) for x,z in ((.008,1.440),(.089,1.465),(.183,1.437),(.224,1.364),
               (.213,1.342),(.124,1.331),(.033,1.335),(.008,1.365))]
    panel('Scapula_'+suffix,outline)
for row,(z,width,height) in enumerate(((1.2825,.193,.036),(1.205,.176,.033),(1.125,.156,.038))):
    outline = [(-width,z+.012),(-width*.82,z+height),(-.040,z+height-.007),(0,z+height-.014),
               (.040,z+height-.007),(width*.82,z+height),(width,z+.012),(width*.89,z-.016),
               (width*.47,z-height+.004),(0,z-height),(-width*.47,z-height+.004),(-width*.89,z-.016)]
    panel('Back_spine_'+str(row),outline)
for side,suffix in ((1,'L'),(-1,'R')):
    for row in range(3):
        name = 'Rib_flank_'+suffix if row==0 else 'Rib_lamella_'+str(row+1)+'_'+suffix
        flank(name,side,row)

bm = bmesh.new()
bm.from_mesh(model.data)
bm.verts.ensure_lookup_table()
removed = {i for name in rebuilt for i in parts_map[name]}
bmesh.ops.delete(bm,geom=[bm.verts[i] for i in removed],context='VERTS')
bm.to_mesh(model.data)
bm.free()
for name in rebuilt:
    model.vertex_groups.remove(model.vertex_groups['PART_'+name])
bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
for obj in generated:
    obj.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.object.join()
rig.select_set(True)
scene['gloom_v16_rebuilt_parts'] = json.dumps(sorted(rebuilt))
scene['gloom_rigid_parts'] = json.dumps(rigid,sort_keys=True)
scene['gloom_v16_weight_adjustment'] = 'Back_spine_1: Spine -> Spine1, matching Rib_lamella_2'
scene['gloom_authoring_stage'] = 'H05-wrapping-armor-candidate-awaiting-artistic-acceptance'
scene['gloom_revision'] = 'Rear cuirass, broad lumbar lamellae and wrapping flanks; accepted front and hood preserved'

scene.frame_set(1)
bpy.context.view_layer.update()
setup(scene,1000,1100)
camera(scene,(2.8,-5,2.2),(0,0,.91),2.1)
draft = root/'.cache/hound-armor-v16'
draft.mkdir(parents=True,exist_ok=True)
path = draft/'hound-draft.blend'
if '--final' in sys.argv:
    path = root/'art/characters/hound/v16/hound-mesh-v16.blend'
    assert not path.exists() and not (root/'assets/characters/hound_rig/v16').exists(), 'Never overwrite a published candidate'
    path.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(path),check_existing=False,compress=True)
model.data.calc_loop_triangles()
print('H16_ARMOR',len(rebuilt),len(model.data.vertices),len(model.data.loop_triangles))
if '--final' in sys.argv:
    export = root/'assets/characters/hound_rig/v16/hound-rig.gltf'
    export.parent.mkdir(parents=True,exist_ok=True)
    bpy.ops.export_scene.gltf(filepath=str(export),export_format='GLTF_SEPARATE',use_selection=True,use_active_scene=True,
        export_yup=True,export_apply=False,export_animations=True,export_animation_mode='ACTIVE_ACTIONS',
        export_nla_strips_merged_animation_name='Hound16_joint_check',export_anim_slide_to_zero=True,export_force_sampling=True,
        export_frame_range=True,export_skins=True,export_def_bones=True,export_rest_position_armature=True,
        export_all_influences=False,export_influence_nb=4,export_extras=True)
    manifest = {'task':'H05','milestone':99,'candidate':'v16','source':'v15','artistic_approval':False,
        'scene':scene.name,'mesh':model.name,'rig':rig.name,'action':rig.animation_data.action.name,'collection':collection.name,
        'modified_parts':sorted(rebuilt),'files':[{'path':p.relative_to(root).as_posix(),
        'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size} for p in (path,export,export.with_suffix('.bin'))]}
    (path.parent/'sculpture-reference.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print('H16_EXPORTED')
