'''Verify a hood-only revision against immutable v14, including full-body proximity in 33 poses.'''
import bpy
import bmesh
import json
import math
import sys
import hashlib
from pathlib import Path
from mathutils import Matrix
from mathutils.bvhtree import BVHTree
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h04_review as assembly
import hound_h03_review as anatomy
import hound_h01_review as hands
root = Path(__file__).resolve().parents[2]
cache = root/'.cache/hound-hood-v15'
scene,source = bpy.data.scenes['Hound_Mesh_v15'],bpy.data.scenes['Hound_Mesh_v14']
model,rig = scene.objects['H15_DeformMesh'],scene.objects['Hound15_Rig']
original,old_rig = source.objects['H14_DeformMesh'],source.objects['Hound14_Rig']
bpy.context.window.scene = scene
assert len(rig.data.bones)==53 and len(model.data.materials)==7
assert model.matrix_world==original.matrix_world and rig.matrix_world==old_rig.matrix_world
assert len(model.modifiers)==1 and model.modifiers[0].object==rig
assert len([o for o in scene.objects if o.type=='MESH'])==1
for bone in rig.data.bones:
    old = old_rig.data.bones[bone.name]
    assert bone.matrix_local==old.matrix_local and bone.length==old.length and bone.use_deform==old.use_deform
    assert (bone.parent.name if bone.parent else None)==(old.parent.name if old.parent else None)
action,old_action = rig.animation_data.action,old_rig.animation_data.action
assert len(action.fcurves)==len(old_action.fcurves)
for curve,old in zip(action.fcurves,old_action.fcurves):
    assert curve.data_path==old.data_path and curve.array_index==old.array_index
    assert len(curve.keyframe_points)==len(old.keyframe_points)
    for key,previous in zip(curve.keyframe_points,old.keyframe_points):
        assert key.co==previous.co and key.interpolation==previous.interpolation
        assert key.handle_left==previous.handle_left and key.handle_right==previous.handle_right
def weights(obj,index):
    return {obj.vertex_groups[w.group].name:w.weight for w in obj.data.vertices[index].groups
            if obj.vertex_groups[w.group].name in rig.data.bones}
before,after = assembly.parts(original),assembly.parts(model)
assert set(before)==set(after) and len(after)==106
for name in before:
    if name=='Hood_continuous':
        continue
    a,b = before[name],after[name]
    assert len(a)==len(b)
    assert all(original.data.vertices[i].co==model.data.vertices[j].co and weights(original,i)==weights(model,j) for i,j in zip(a,b)),name
    am,bm = {v:i for i,v in enumerate(a)},{v:i for i,v in enumerate(b)}
    af = [(tuple(am[i] for i in p.vertices),p.material_index,p.use_smooth) for p in original.data.polygons if p.vertices[0] in am]
    bf = [(tuple(bm[i] for i in p.vertices),p.material_index,p.use_smooth) for p in model.data.polygons if p.vertices[0] in bm]
    assert af==bf,name
assert all(tuple(a.diffuse_color)==tuple(b.diffuse_color) for a,b in zip(original.data.materials,model.data.materials))
assert scene['gloom_rigid_parts']==source['gloom_rigid_parts']
assert len(after['Hood_continuous'])==1029
maximum = 0
for vertex in model.data.vertices:
    values = list(weights(model,vertex.index).values())
    assert values and all(0<w<=1 for w in values) and abs(sum(values)-1)<2e-6
    maximum = max(maximum,len(values))
assert maximum<=2
# Preserve the nape coverage and rear outline exactly.
rear = {tuple(model.data.vertices[i].co) for i in after['Hood_continuous']}
assert all(tuple(original.data.vertices[i].co) in rear for i in before['Hood_continuous'] if original.data.vertices[i].co.y>=.02)
# The crown/face framing above the changed lower drape stays at the original landmarks.
for group_name in ('LANDMARK_Hood_opening','LANDMARK_Hood_outer_front'):
    a_group,b_group = original.vertex_groups[group_name].index,model.vertex_groups[group_name].index
    a = [v.co for v in original.data.vertices if v.co.z>=1.57 and any(w.group==a_group for w in v.groups)]
    b = [v.co for v in model.data.vertices if v.co.z>=1.57 and any(w.group==b_group for w in v.groups)]
    assert len(a)==len(b) and all(any(p==q for q in b) for p in a)
minimum_area = 1
for frame in range(1,182,3):
    scene.frame_set(frame)
    bpy.context.view_layer.update()
    ev = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = ev.to_mesh()
    assert all(all(math.isfinite(c) for c in v.co) for v in mesh.vertices)
    minimum_area = min(minimum_area,min(p.area for p in mesh.polygons))
    assert minimum_area>1e-12
    ev.to_mesh_clear()

CASES = ('rest','reach','twist-left','twist-right','step-left','step-right','hip-left','hip-right',
         'head-left','head-right','fist','flex','extend','look-up','look-down','elbow','combined')
cases = [('rest',1)]+[(kind,amount) for kind in CASES[1:] for amount in (.5,1)]
def pose(sc,arm,kind,amount):
    assembly.pose(sc,arm,'rest')
    assert all(b.matrix_basis==Matrix.Identity(4) for b in arm.pose.bones)
    if kind in ('fist','flex','extend'):
        hands.pose(sc,arm,kind,amount)
    elif kind in ('look-up','look-down','elbow'):
        anatomy.pose(sc,arm,kind,amount)
    elif kind=='combined':
        assembly.pose(sc,arm,'twist-left',amount)
        rotations = {b.name:b.rotation_quaternion.copy() for b in arm.pose.bones if b.name in ('Bip001 Spine','Bip001 Spine1')}
        assembly.pose(sc,arm,'step-left',amount)
        for name,rotation in rotations.items():
            arm.pose.bones[name].rotation_quaternion = rotation
        bpy.context.view_layer.update()
    else:
        assembly.pose(sc,arm,kind,amount)
contacts,self_crossings,topology = {},{},{}
for version,sc,obj,arm in (('v14',source,original,old_rig),('v15',scene,model,rig)):
    arm.animation_data.action = None
    groups = assembly.parts(obj)
    owner = {i:n for n,ids in groups.items() for i in ids}
    faces = {n:[] for n in groups}
    for polygon in obj.data.polygons:
        faces[owner[polygon.vertices[0]]].append(list(polygon.vertices))
    bm = bmesh.new()
    hood_vertices = {i:bm.verts.new(obj.data.vertices[i].co) for i in groups['Hood_continuous']}
    for face in faces['Hood_continuous']:
        bm.faces.new([hood_vertices[i] for i in face])
    pending,visited = [next(iter(bm.verts))],set()
    while pending:
        v = pending.pop()
        if v in visited:
            continue
        visited.add(v)
        pending.extend(e.other_vert(v) for e in v.link_edges if e.other_vert(v) not in visited)
    topology[version] = {'vertices':len(bm.verts),'faces':len(bm.faces),'connected':len(visited)==len(bm.verts),
                         'volume_m3':bm.calc_volume(signed=True),'euler':len(bm.verts)-len(bm.edges)+len(bm.faces)}
    assert topology[version]['connected'] and topology[version]['volume_m3']>0
    assert all(e.is_manifold and e.is_contiguous for e in bm.edges)
    bm.free()
    contacts[version],self_crossings[version] = {},{}
    for kind,amount in cases:
        pose(sc,arm,kind,amount)
        tag = kind if amount==1 else kind+'-half'
        ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = ev.to_mesh()
        points = [v.co.copy() for v in mesh.vertices]
        trees = {n:BVHTree.FromPolygons(points,f) for n,f in faces.items()}
        hood_faces = faces['Hood_continuous']
        tree = trees['Hood_continuous']
        contacts[version][tag] = {n:len(tree.overlap(trees[n])) for n in groups if n!='Hood_continuous'}
        self_crossings[version][tag] = sum(a<b and not set(hood_faces[a]).intersection(hood_faces[b]) for a,b in tree.overlap(tree))
        ev.to_mesh_clear()
for tag,row in contacts['v15'].items():
    assert self_crossings['v15'][tag]==0,(tag,self_crossings['v15'][tag])
    # Existing nape/lining insertion is reported, not treated as a new visible collision.
    for name,count in row.items():
        if not contacts['v14'][tag][name]:
            assert count==0,(tag,'new contact',name,count)
rig.animation_data.action,old_rig.animation_data.action = action,old_action
bpy.context.window.scene = scene
scene.frame_set(1)
model.data.calc_loop_triangles()
for version in ('v13','v14'):
    for item in json.loads((root/f'art/characters/hound/{version}/sculpture-reference.json').read_text())['files']:
        assert hashlib.sha256((root/item['path']).read_bytes()).hexdigest()==item['sha256']
result = {'vertices':len(model.data.vertices),'triangles':len(model.data.loop_triangles),'preserved_parts':105,'modified_parts':['Hood_continuous'],
          'bones':53,'diagnostic_frames':61,'minimum_face_area_m2':minimum_area,'max_influences':maximum,'topology':topology,
          'poses':len(cases),'hood_pairs_per_pose':105,'contacts':contacts,'self_crossings':self_crossings,
          'scope':'Hood against every other part; other 105 components/rig/keys exact. Discrete surface crossings, not continuous clearance.'}
(cache/'verification.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print('H15_VERIFIED',json.dumps({k:v for k,v in result.items() if k not in ('contacts','self_crossings')}))
print('HOOD_REST',json.dumps({version:{n:v for n,v in rows['rest'].items() if v} for version,rows in contacts.items()}))
