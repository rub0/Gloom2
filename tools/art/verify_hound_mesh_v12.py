'''Verify H04 geometry, preservation, skinning and explicit local assembly contacts.'''
import bpy
import bmesh
import json
import math
import sys
from pathlib import Path
from mathutils.bvhtree import BVHTree
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h04_review as review

root = Path(__file__).resolve().parents[2]
scene,source = bpy.data.scenes['Hound_Mesh_v12'],bpy.data.scenes['Hound_Mesh_v11']
bpy.context.window.scene = scene
model,rig = scene.objects['H12_DeformMesh'],scene.objects['Hound12_Rig']
original,old_rig = source.objects['H11_DeformMesh'],source.objects['Hound11_Rig']
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
added = {'Yoke_L','Yoke_R'}
assert set(after)==set(before)|added and len(after)==98
rebuilt = set(json.loads(scene['gloom_v12_rebuilt_parts']))
morphed = set(json.loads(scene['gloom_v12_morphed_parts']))
assert rebuilt=={'Sash_tail_L','Sash_tail_R'}|{n for n in before if n.startswith(('Collar_plate_','Collar_blade_middle_','Collar_blade_outer_'))}
plate_names = {n for n in before if n.startswith(('Breastplate_','Abdominal_','Back_spine_','Scapula_','Rib_flank_'))}
assert morphed==plate_names|{'Trousers_continuous','Waist_wrap','Torso_underlayer'}
preserved = 0
for name,indices in before.items():
    if name in rebuilt:
        continue
    other = after[name]
    assert len(indices)==len(other)
    for a,b in zip(indices,other):
        distance = (original.data.vertices[a].co-model.data.vertices[b].co).length
        assert distance < (.16 if name in plate_names else .045 if name=='Torso_underlayer' else .02 if name in morphed else 1e-7),name
        assert weights(original,a)==weights(model,b)
        if name=='Trousers_continuous' and original.data.vertices[a].co.z<=.61:
            assert distance==0, 'Preserve H02 lower lining'
    a_map,b_map = {v:i for i,v in enumerate(indices)},{v:i for i,v in enumerate(other)}
    a_faces = [(tuple(a_map[i] for i in p.vertices),p.material_index,p.use_smooth)
               for p in original.data.polygons if all(i in a_map for i in p.vertices)]
    b_faces = [(tuple(b_map[i] for i in p.vertices),p.material_index,p.use_smooth)
               for p in model.data.polygons if all(i in b_map for i in p.vertices)]
    if name=='Torso_underlayer':
        assert [(f,m) for f,m,s in a_faces]==[(f,m) for f,m,s in b_faces]
    else:
        assert a_faces==b_faces,name
    if name in plate_names:
        count = (len(indices)-1)//9
        assert all(original.data.vertices[a].co==model.data.vertices[b].co for a,b in zip(indices[:-count],other[:-count])), name
    preserved += name not in morphed
assert preserved==73
assert all(tuple(a.diffuse_color)==tuple(b.diffuse_color) for a,b in zip(original.data.materials,model.data.materials))
faces_by_part = {}
for name,indices in after.items():
    ids = set(indices)
    faces_by_part[name] = [list(p.vertices) for p in model.data.polygons if all(i in ids for i in p.vertices)]
topology = {}
for name in sorted(rebuilt|morphed|added):
    bm = bmesh.new()
    verts = {i:bm.verts.new(model.data.vertices[i].co) for i in after[name]}
    for face in faces_by_part[name]:
        bm.faces.new([verts[i] for i in face])
    assert all(e.is_manifold and e.is_contiguous for e in bm.edges),name
    assert all(v.is_manifold for v in bm.verts) and all(f.calc_area()>1e-12 for f in bm.faces),(name,min(f.calc_area() for f in bm.faces))
    volume = bm.calc_volume(signed=True)
    assert volume>0,name
    euler = len(bm.verts)-len(bm.edges)+len(bm.faces)
    assert euler==(0 if name=='Waist_wrap' else 2),(name,euler)
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

# Check thickness analytically, independently of screenshots.
from mathutils import Vector
for name in plate_names:
    ids,old_ids = after[name],before[name]
    count = (len(ids)-1)//9
    direction = sum((original.data.vertices[i].co for i in old_ids[:count]),Vector())
    direction -= sum((original.data.vertices[i].co for i in old_ids[-count:]),Vector())
    direction.normalize()
    assert all((model.data.vertices[b].co-(model.data.vertices[a].co-direction*.004)).length<1e-7
               for a,b in zip(ids[:count],ids[-count:])),name
for suffix in ('L','R'):
    ids = after['Sash_tail_'+suffix]
    assert len(ids)==442
    assert all((model.data.vertices[b].co-model.data.vertices[a].co-Vector((0,.003,0))).length<1e-7
               and weights(model,a)==weights(model,b) for a,b in zip(ids[:221],ids[221:]))
for name in rebuilt:
    if name.startswith('Collar_'):
        for i in before[name]:
            p = original.data.vertices[i].co
            if p.z>1.57:
                assert min((p-model.data.vertices[j].co).length for j in after[name])<1e-6, 'Preserve exposed blade endpoints'

maximum = 0
for vertex in model.data.vertices:
    values = list(weights(model,vertex.index).values())
    assert values and abs(sum(values)-1)<2e-6 and all(0<w<=1 for w in values)
    maximum = max(maximum,len(values))
assert maximum<=2
rigid = json.loads(scene['gloom_rigid_parts'])
expected_rigid = json.loads(source['gloom_rigid_parts'])
for suffix in ('L','R'):
    del expected_rigid['Sash_tail_'+suffix]
    expected_rigid['Yoke_'+suffix] = 'Bip001 Spine2'
assert rigid==expected_rigid
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



# Restoring a pose must reset every bone, including limbs touched by the preceding case.
action = rig.animation_data.action
rig.animation_data.action = None
review.pose(scene,rig,'hip-left')
review.pose(scene,rig,'rest')
from mathutils import Matrix
assert all(b.matrix_basis==Matrix.Identity(4) for b in rig.pose.bones)
rig.animation_data.action = action
scene.frame_set(1)

pairs = review.contact_pairs(set(after))
cases = [('rest',1)]+[(kind,amount) for kind in review.CASES[1:] for amount in (.25,.5,.75,1)]
contacts,self_crossings = {},{}
for version,sc,obj,arm in (('v11',source,original,old_rig),('v12',scene,model,rig)):
    saved_action = arm.animation_data.action
    arm.animation_data.action = None
    indices = parts(obj)
    owner = {i:name for name,ids in indices.items() for i in ids}
    part_faces = {name:[] for name in indices}
    for polygon in obj.data.polygons:
        part_faces[owner[polygon.vertices[0]]].append(list(polygon.vertices))
    checked = {n for p in pairs for n in p} & set(indices)
    contacts[version],self_crossings[version] = {},{}
    for kind,amount in cases:
        review.pose(sc,arm,kind,amount)
        tag = kind if amount==1 else kind+'_'+str(amount)
        ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = ev.to_mesh()
        assert all(all(math.isfinite(c) for c in v.co) for v in mesh.vertices)
        points = [v.co.copy() for v in mesh.vertices]
        trees = {name:BVHTree.FromPolygons(points,part_faces[name]) for name in checked}
        contacts[version][tag] = {a+'/'+b:len(trees[a].overlap(trees[b])) for a,b in pairs if a in checked and b in checked}
        self_crossings[version][tag] = {}
        for name in ('Trousers_continuous','Waist_wrap','Sash_tail_L','Sash_tail_R'):
            faces = part_faces[name]
            hits = [(a,b) for a,b in trees[name].overlap(trees[name]) if a<b and not set(faces[a]).intersection(faces[b])]
            self_crossings[version][tag][name] = len(hits)
        if version=='v12':
            for suffix in ('L','R'):
                free_faces = [f for f in part_faces['Sash_tail_'+suffix] if max(obj.data.vertices[i].co.z for i in f)<.991]
                assert not BVHTree.FromPolygons(points,free_faces).overlap(trees['Trousers_continuous']), (tag,suffix,'free hem')
        ev.to_mesh_clear()
    arm.animation_data.action = saved_action
    sc.frame_set(1)
bpy.context.window.scene = scene
scene.frame_set(1)
model.data.calc_loop_triangles()
result = {'vertices':len(rest),'triangles':len(model.data.loop_triangles),'preserved_components':preserved,
          'rebuilt_components':len(rebuilt),'morphed_components':len(morphed),'added_components':len(added),'bones':53,
          'unchanged_diagnostic_keys':True,'rigid_parts':len(rigid),'max_influences':maximum,'sampled_frames':61,
          'minimum_area_m2':minimum_area,'max_rigid_error_m':rigid_error,'topology':topology,
          'pose_cases':len(cases),'pairs_per_pose':len(pairs),'contacts':contacts,'self_crossings':self_crossings}
output = root/'reports/hound-cloth-94'
output.mkdir(parents=True,exist_ok=True)
(output/'validation.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k not in ('topology','contacts','self_crossings')},indent=2))
print('SELF_CROSSINGS',json.dumps({v:{k:r for k,r in rows.items() if any(r.values())} for v,rows in self_crossings.items()}))
clear_pairs = [(f'Arm_surface_{s}',f'Collar_{p}_{s}') for s in ('L','R') for p in ('plate','blade_middle','blade_outer')]
clear_pairs += [(f'Arm_surface_{s}',f'Breastplate_{s}') for s in ('L','R')]
clear_pairs += [('Hood_continuous',n) for n in after if n.startswith(('Collar_','Yoke_'))]
clear_pairs += [('Head_surface','Hood_continuous'),('Eye_L','Hood_continuous'),('Eye_R','Hood_continuous')]
assert all(row['/'.join(sorted((a,b)))]==0 for row in contacts['v12'].values() for a,b in clear_pairs)
assert all(n==0 for row in self_crossings['v12'].values() for n in row.values())
assert contacts['v12']['rest']['Back_spine_2/Torso_underlayer']>0, 'Seat the lower dorsal lamella on the backing'
print('H04_VALIDATED',len(pairs)*len(cases),'local contact evaluations; concealed attachment overlaps reported separately')
