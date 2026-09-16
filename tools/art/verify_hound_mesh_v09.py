'''Verify H01 geometry, exact unaffected regions, skinning and explicit local contacts.'''
import bpy
import bmesh
import json
import math
import sys
from pathlib import Path
from mathutils.bvhtree import BVHTree
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h01_review as review

root = Path(__file__).resolve().parents[2]
scene,source = bpy.data.scenes['Hound_Mesh_v09'],bpy.data.scenes['Hound_Mesh_v08']
bpy.context.window.scene = scene
model,rig = scene.objects['H09_DeformMesh'],scene.objects['Hound09_Rig']
original,old_rig = source.objects['H08_DeformMesh'],source.objects['Hound08_Rig']
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
rebuilt = set(json.loads(scene['gloom_v09_rebuilt_parts']))
morphed = set(json.loads(scene['gloom_v09_morphed_parts']))
assert len(rebuilt)==32 and morphed=={'Hand_glove_L','Hand_glove_R','Bracer_face_L','Bracer_face_R'}
assert all(n.startswith(('Bracer_mass_','Wrist_transition_','Finger_','Thumb_')) for n in rebuilt)
preserved = 0
for name,indices in before.items():
    if name in rebuilt:
        continue
    other = after[name]
    assert len(indices)==len(other)
    for a,b in zip(indices,other):
        distance = (original.data.vertices[a].co-model.data.vertices[b].co).length
        assert distance < (.051 if name.startswith('Bracer_face_') else .012 if name in morphed else 1e-7),name
        assert weights(original,a)==weights(model,b)
    a_map,b_map = {v:i for i,v in enumerate(indices)},{v:i for i,v in enumerate(other)}
    a_faces = [(tuple(a_map[i] for i in p.vertices),p.material_index,p.use_smooth)
               for p in original.data.polygons if all(i in a_map for i in p.vertices)]
    b_faces = [(tuple(b_map[i] for i in p.vertices),p.material_index,p.use_smooth)
               for p in model.data.polygons if all(i in b_map for i in p.vertices)]
    assert a_faces==b_faces,name
    preserved += name not in morphed
assert preserved==60
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
    assert euler==(0 if name.startswith(('Bracer_mass_','Wrist_transition_')) else 2),name
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
self_contacts = {}
for kind in ('open','fist','flex','extend','grip'):
    review.pose(scene,rig,kind)
    ev = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = ev.to_mesh()
    assert min(p.area for p in mesh.polygons)>1e-12
    points = [v.co.copy() for v in mesh.vertices]
    counts = {}
    for suffix in ('L','R'):
        trees = {n:BVHTree.FromPolygons(points,faces_by_part[n]) for n in review.hand_parts(model,suffix)}
        digits = [n for n in trees if n.startswith(('Finger_','Thumb_'))]
        pairs = [(a,b) for i,a in enumerate(digits) for b in digits[i+1:]+['Hand_back_full_point_'+suffix]]
        pairs += [(a+'_'+suffix,b+'_'+suffix) for a,b in (
            ('Wrist_transition','Bracer_mass'),('Hand_back_full_point','Bracer_mass'),
            ('Hand_back_full_point','Wrist_transition'),('Hand_glove','Hand_back_full_point'))]
        pairs += [(a+'_'+suffix,b+'_'+suffix) for a in ('Hand_glove','Wrist_transition','Hand_back_full_point')
                  for b in ('Bracer_face','Bracer_blade_upper','Bracer_blade_lower','Elbow_cuff')]
        for a,b in pairs:
            count = len(trees[a].overlap(trees[b]))
            assert count==0,(kind,a,b,count)
        counts[suffix] = {'pairs':len(pairs),'crossing_face_pairs':0}
        faces = faces_by_part['Hand_glove_'+suffix]
        tree = trees['Hand_glove_'+suffix]
        intersections = [(a,b) for a,b in tree.overlap(tree) if a<b and not set(faces[a]).intersection(faces[b])]
        assert not intersections,(kind,suffix,'glove self crossings')
        self_contacts[kind+'_'+suffix] = len(intersections)
    if kind=='grip':
        gun = review.weapon(scene,rig)
        tree = BVHTree.FromPolygons([gun.matrix_world@v.co for v in gun.data.vertices],[list(p.vertices) for p in gun.data.polygons])
        weapon_crossings = 0
        minimum = 1
        for name in review.hand_parts(model,'R',False):
            part_tree = BVHTree.FromPolygons(points,faces_by_part[name])
            weapon_crossings += len(tree.overlap(part_tree))
            minimum = min(minimum,min(tree.find_nearest(points[i])[3] for i in after[name]))
        assert weapon_crossings==0
        counts['weapon'] = {'crossing_face_pairs':weapon_crossings,'minimum_vertex_surface_gap_m':minimum,
                            'kind':'rear-shell support; not a closed production grip','presentation_scale':.43}
        bpy.data.objects.remove(gun,do_unlink=True)
    contact_counts[kind] = counts
    ev.to_mesh_clear()
rig.animation_data.action = action
scene.frame_set(1)
model.data.calc_loop_triangles()
result = {'vertices':len(rest),'triangles':len(model.data.loop_triangles),'preserved_components':preserved,
          'rebuilt_components':len(rebuilt),'morphed_gloves':2,'relieved_front_guards':2,'bones':53,'unchanged_diagnostic_keys':True,
          'rigid_parts':len(rigid),'max_influences':maximum,'sampled_frames':61,'minimum_area_m2':minimum_area,
          'max_rigid_error_m':rigid_error,'topology':topology,'local_contacts':contact_counts,'glove_self_crossings':self_contacts}
(root/'reports/hound-hands-91/validation.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k!='topology'},indent=2))
