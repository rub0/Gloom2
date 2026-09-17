'''Verify H02 geometry, exact unaffected regions, skinning and explicit local contacts.'''
import bpy
import bmesh
import json
import math
import sys
from pathlib import Path
from mathutils.bvhtree import BVHTree
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h02_review as review

root = Path(__file__).resolve().parents[2]
scene,source = bpy.data.scenes['Hound_Mesh_v10'],bpy.data.scenes['Hound_Mesh_v09']
bpy.context.window.scene = scene
model,rig = scene.objects['H10_DeformMesh'],scene.objects['Hound10_Rig']
original,old_rig = source.objects['H09_DeformMesh'],source.objects['Hound09_Rig']
assert len(rig.data.bones)==53 and len(model.data.materials)==7
assert len([o for o in scene.objects if o.type=='MESH'])==1
assert len(model.modifiers)==1 and model.modifiers[0].object==rig
assert model.matrix_world==original.matrix_world and rig.matrix_world==old_rig.matrix_world
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


def parts(obj):
    result = {g.name[5:]: [] for g in obj.vertex_groups if g.name.startswith('PART_')}
    for vertex in obj.data.vertices:
        for group in vertex.groups:
            name = obj.vertex_groups[group.group].name
            if name.startswith('PART_'):
                result[name[5:]].append(vertex.index)
    return result


def weights(obj,index):
    return {obj.vertex_groups[w.group].name:w.weight for w in obj.data.vertices[index].groups
            if obj.vertex_groups[w.group].name in rig.data.bones}


before,after = parts(original),parts(model)
assert before.keys()==after.keys() and len(after)==96
rebuilt = set(json.loads(scene['gloom_v10_rebuilt_parts']))
morphed = set(json.loads(scene['gloom_v10_morphed_parts']))
assert rebuilt=={p+'_'+side for p in ('Greave_mass','Boot','Knee_shield','Greave_front','Shin_lower','Ankle_guard','Boot_toe') for side in ('L','R')}
assert morphed=={'Trousers_continuous'}
preserved = 0
for name,indices in before.items():
    if name in rebuilt:
        continue
    other = after[name]
    assert len(indices)==len(other)
    for a,b in zip(indices,other):
        distance = (original.data.vertices[a].co-model.data.vertices[b].co).length
        assert distance < (.04 if name in morphed else 1e-7),name
        assert weights(original,a)==weights(model,b)
        if name=='Trousers_continuous' and original.data.vertices[a].co.z>=.61:
            assert distance==0, 'Upper trousers outside H02'
    a_map,b_map = {v:i for i,v in enumerate(indices)},{v:i for i,v in enumerate(other)}
    a_faces = [(tuple(a_map[i] for i in p.vertices),p.material_index,p.use_smooth)
               for p in original.data.polygons if all(i in a_map for i in p.vertices)]
    b_faces = [(tuple(b_map[i] for i in p.vertices),p.material_index,p.use_smooth)
               for p in model.data.polygons if all(i in b_map for i in p.vertices)]
    assert a_faces==b_faces,name
    preserved += name not in morphed
assert preserved==81
assert all(tuple(a.diffuse_color)==tuple(b.diffuse_color) for a,b in zip(original.data.materials,model.data.materials))
faces_by_part = {}
for name,indices in after.items():
    ids = set(indices)
    faces_by_part[name] = [list(p.vertices) for p in model.data.polygons if all(i in ids for i in p.vertices)]
topology = {}
for name in sorted(rebuilt|morphed):
    bm = bmesh.new()
    verts = {i:bm.verts.new(model.data.vertices[i].co) for i in after[name]}
    for face in faces_by_part[name]:
        bm.faces.new([verts[i] for i in face])
    assert all(e.is_manifold and e.is_contiguous for e in bm.edges),name
    assert all(v.is_manifold for v in bm.verts) and all(f.calc_area()>1e-10 for f in bm.faces),name
    volume = bm.calc_volume(signed=True)
    assert volume>0,name
    euler = len(bm.verts)-len(bm.edges)+len(bm.faces)
    assert euler==(0 if name.startswith('Greave_mass_') else 2),name
    pending,visited = [next(iter(bm.verts))],set()
    while pending:
        v = pending.pop()
        if v in visited:
            continue
        visited.add(v)
        pending.extend(e.other_vert(v) for e in v.link_edges if e.other_vert(v) not in visited)
    assert len(visited)==len(bm.verts),name
    faces = faces_by_part[name]
    tree = BVHTree.FromPolygons([v.co for v in model.data.vertices],faces)
    assert not [(a,b) for a,b in tree.overlap(tree) if a<b and not set(faces[a]).intersection(faces[b])],name
    topology[name] = {'vertices':len(bm.verts),'euler':euler,'volume_m3':volume}
    bm.free()
maximum = 0
for vertex in model.data.vertices:
    values = list(weights(model,vertex.index).values())
    assert values and abs(sum(values)-1)<2e-6 and all(0<w<=1 for w in values)
    maximum = max(maximum,len(values))
assert maximum<=2
rigid = json.loads(scene['gloom_rigid_parts'])
assert rigid==json.loads(source['gloom_rigid_parts'])
for name,bone in rigid.items():
    assert all(weights(model,i)=={bone:1.0} for i in after[name])
rest = [v.co.copy() for v in model.data.vertices]
rigid_error = 0
minimum_area = 1
for frame in range(1,182,3):
    scene.frame_set(frame)
    bpy.context.view_layer.update()
    ev = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = ev.to_mesh()
    assert all(all(math.isfinite(c) for c in v.co) for v in mesh.vertices)
    minimum_area = min(minimum_area,min(p.area for p in mesh.polygons))
    assert minimum_area>1e-12
    if frame in (1,181):
        assert max((v.co-rest[v.index]).length for v in mesh.vertices)<1e-5
    for name in rigid:
        indices = after[name]
        origin = indices[0]
        for i in indices[1:]:
            error = abs((mesh.vertices[i].co-mesh.vertices[origin].co).length-(rest[i]-rest[origin]).length)
            rigid_error = max(rigid_error,error)
    ev.to_mesh_clear()
assert rigid_error<2e-6

rig.animation_data.action = None
contact_counts = {}
crossings = []
support = {}
cases = [('rest',1)]+[(kind,amount) for kind in ('knee','step','toe','heel') for amount in (.25,.5,.75,1)]
for kind,amount in cases:
    lift = review.pose(scene,rig,model,kind,amount)
    kind = kind if amount==1 else kind+'_'+str(amount)
    ev = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = ev.to_mesh()
    assert min(p.area for p in mesh.polygons)>1e-12
    points = [v.co.copy() for v in mesh.vertices]
    names = review.leg_parts(model)
    trees = {n:BVHTree.FromPolygons(points,faces_by_part[n]) for n in names}
    pairs = []
    for suffix in ('L','R'):
        names = sorted(review.leg_parts(model,suffix))
        pairs += [(a,b) for j,a in enumerate(names) for b in names[j+1:]]
    pairs += [(a,b) for a in review.leg_parts(model,'L')-{'Trousers_continuous'}
              for b in review.leg_parts(model,'R')-{'Trousers_continuous'}]
    for a,b in pairs:
        hits = trees[a].overlap(trees[b])
        if hits:
            crossings.append((kind,a,b,len(hits)))
    contact_counts[kind] = {'pairs':len(pairs),'crossing_face_pairs':sum(c[3] for c in crossings if c[0]==kind)}
    faces = faces_by_part['Trousers_continuous']
    tree = trees['Trousers_continuous']
    self_hits = [(a,b) for a,b in tree.overlap(tree) if a<b and not set(faces[a]).intersection(faces[b])]
    if self_hits:
        crossings.append((kind,'trousers self','trousers self',len(self_hits)))
    support[kind] = {'root_lift_m':lift}
    for suffix in ('L','R'):
        ids = after['Boot_'+suffix]
        low = min(points[i].z for i in ids)
        assert low>=-1e-6,(kind,suffix,low)
        support[kind][suffix+'_min_z_m'] = low
        # New boot indices 0..31 are the plantar rim, followed later by its bottom center.
        plantar = [points[i] for i in ids[:32]]
        normal = (plantar[8]-plantar[0]).cross(plantar[16]-plantar[0])
        assert (1 if suffix=='L' else -1)*normal.normalized().z>.70, (kind,suffix,'sole inversion')
    ev.to_mesh_clear()
rig.animation_data.action = action
scene.frame_set(1)
model.data.calc_loop_triangles()
result = {'vertices':len(rest),'triangles':len(model.data.loop_triangles),'preserved_components':preserved,
          'rebuilt_components':len(rebuilt),'morphed_components':len(morphed),'bones':53,'unchanged_diagnostic_keys':True,
          'rigid_parts':len(rigid),'max_influences':maximum,'sampled_frames':61,'minimum_area_m2':minimum_area,
          'max_rigid_error_m':rigid_error,'topology':topology,'local_contacts':contact_counts,'crossings':crossings,'support':support}
output = root/'reports/hound-legs-92'
output.mkdir(parents=True,exist_ok=True)
(output/'validation.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k!='topology'},indent=2))
assert not crossings, 'H02 contact crossings'
