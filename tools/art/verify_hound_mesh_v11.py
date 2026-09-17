'''Verify H03 geometry, exact unaffected regions, skinning and explicit local contacts.'''
import bpy
import bmesh
import json
import math
import sys
from pathlib import Path
from mathutils.bvhtree import BVHTree
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h03_review as review

root = Path(__file__).resolve().parents[2]
scene,source = bpy.data.scenes['Hound_Mesh_v11'],bpy.data.scenes['Hound_Mesh_v10']
bpy.context.window.scene = scene
model,rig = scene.objects['H11_DeformMesh'],scene.objects['Hound11_Rig']
original,old_rig = source.objects['H10_DeformMesh'],source.objects['Hound10_Rig']
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
rebuilt = set(json.loads(scene['gloom_v11_rebuilt_parts']))
morphed = set(json.loads(scene['gloom_v11_morphed_parts']))
assert rebuilt=={'Head_surface','Eye_L','Eye_R','Mouth_shadow'}
assert morphed=={'Arm_surface_L','Arm_surface_R'}
preserved = 0
for name,indices in before.items():
    if name in rebuilt:
        continue
    other = after[name]
    assert len(indices)==len(other)
    for a,b in zip(indices,other):
        distance = (original.data.vertices[a].co-model.data.vertices[b].co).length
        assert distance < (.008 if name in morphed else 1e-7),name
        assert weights(original,a)==weights(model,b)
        if name in morphed and original.data.vertices[a].co.z<=1.085:
            assert distance==0, 'Wrist outside H03'
    a_map,b_map = {v:i for i,v in enumerate(indices)},{v:i for i,v in enumerate(other)}
    a_faces = [(tuple(a_map[i] for i in p.vertices),p.material_index,p.use_smooth)
               for p in original.data.polygons if all(i in a_map for i in p.vertices)]
    b_faces = [(tuple(b_map[i] for i in p.vertices),p.material_index,p.use_smooth)
               for p in model.data.polygons if all(i in b_map for i in p.vertices)]
    assert a_faces==b_faces,name
    preserved += name not in morphed
assert preserved==90
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
    assert euler==2,name
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


# Fitted details stay within 1.5 mm of the facial surface in front view; embedded back caps are intentional.
from mathutils import Vector
head_tree = BVHTree.FromPolygons(rest,faces_by_part['Head_surface'],all_triangles=True)
detail_offsets = {}
for name in ('Eye_L','Eye_R','Mouth_shadow'):
    values = []
    for i in after[name]:
        p = rest[i]
        hit = head_tree.ray_cast(Vector((p.x,-.3,p.z)),Vector((0,1,0)),.4)[0]
        assert hit is not None
        values.append(hit.y-p.y)
    detail_offsets[name] = {'minimum_m':min(values),'maximum_m':max(values)}
    assert min(values)>-.0011 and max(values)<.0015, (name,detail_offsets[name])

# Record source and H03 contacts on the same poses. Closed neck/head overlap is an attachment, not an articulated facial seam.
pairs = [('Head_surface','Hood_continuous'),('Head_surface','Neck_surface'),('Hood_continuous','Neck_surface'),
         ('Eye_L','Hood_continuous'),('Eye_R','Hood_continuous'),('Mouth_shadow','Hood_continuous')]
cases = [('rest',1)]+[(kind,amount) for kind in review.CASES[1:] for amount in (.25,.5,.75,1)]
contacts = {}
volumes = {}
self_crossings = {}
for version,sc,obj,arm in (('v10',source,original,old_rig),('v11',scene,model,rig)):
    arm.animation_data.action = None
    indices = parts(obj)
    part_faces = {}
    for name in review.HEAD_PARTS|{'Hood_continuous','Arm_surface_L','Arm_surface_R'}:
        ids = set(indices[name])
        part_faces[name] = [list(p.vertices) for p in obj.data.polygons if all(i in ids for i in p.vertices)]
    contacts[version],volumes[version],self_crossings[version] = {},{},{}
    for kind,amount in cases:
        review.pose(sc,arm,kind,amount)
        tag = kind if amount==1 else kind+'_'+str(amount)
        ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = ev.to_mesh()
        assert all(all(math.isfinite(c) for c in v.co) for v in mesh.vertices)
        assert min(p.area for p in mesh.polygons)>1e-12
        points = [v.co.copy() for v in mesh.vertices]
        trees = {name:BVHTree.FromPolygons(points,faces) for name,faces in part_faces.items()}
        contacts[version][tag] = {a+'/'+b:len(trees[a].overlap(trees[b])) for a,b in pairs}
        self_crossings[version][tag] = {}
        for name in ('Arm_surface_L','Arm_surface_R','Head_surface'):
            faces = part_faces[name]
            hits = [(a,b) for a,b in trees[name].overlap(trees[name]) if a<b and not set(faces[a]).intersection(faces[b])]
            self_crossings[version][tag][name] = len(hits)
        volumes[version][tag] = {}
        for name in ('Arm_surface_L','Arm_surface_R'):
            # Signed tetrahedron volume of the triangulated closed arm, independent of bone transforms.
            volume = 0
            for face in part_faces[name]:
                a = points[face[0]]
                for j in range(1,len(face)-1):
                    volume += a.dot(points[face[j]].cross(points[face[j+1]]))/6
            assert volume>0, (version,tag,name)
            volumes[version][tag][name] = volume
        ev.to_mesh_clear()
    arm.animation_data.action = action if version=='v11' else old_action
    sc.frame_set(1)
ratios = {kind:{name:volume/volumes['v11']['rest'][name] for name,volume in values.items()}
          for kind,values in volumes['v11'].items()}
assert all(.80<ratio<1.20 for row in ratios.values() for ratio in row.values()), ratios

bpy.context.window.scene = scene
scene.frame_set(1)
model.data.calc_loop_triangles()
result = {'vertices':len(rest),'triangles':len(model.data.loop_triangles),'preserved_components':preserved,
          'rebuilt_components':len(rebuilt),'morphed_components':len(morphed),'bones':53,'unchanged_diagnostic_keys':True,
          'rigid_parts':len(rigid),'max_influences':maximum,'sampled_frames':61,'minimum_area_m2':minimum_area,
          'max_rigid_error_m':rigid_error,'topology':topology,'detail_offsets':detail_offsets,
          'pose_cases':len(cases),'contacts':contacts,'self_crossings':self_crossings,'arm_volume_ratios':ratios}
output = root/'reports/hound-anatomy-93'
output.mkdir(parents=True,exist_ok=True)
(output/'validation.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k not in ('topology','contacts','self_crossings','arm_volume_ratios')},indent=2))
print('CONTACTS',json.dumps({v:{k:counts for k,counts in rows.items() if '_' not in k} for v,rows in contacts.items()}))
print('ARM_VOLUME_RATIO_RANGE',min(x for r in ratios.values() for x in r.values()),max(x for r in ratios.values() for x in r.values()))

# The unchanged diagnostic skinning already pinches at deep flexion/raised shoulders.
# H03 must not worsen it; H08 owns the production deformation fix.
assert all(n<=self_crossings['v10'][kind][name] for kind,row in self_crossings['v11'].items() for name,n in row.items())
assert all(row[a+'/'+b]==0 for row in contacts['v11'].values() for a,b in pairs if b=='Hood_continuous')
assert all(row['Hood_continuous/Neck_surface']==contacts['v10'][kind]['Hood_continuous/Neck_surface']
           for kind,row in contacts['v11'].items())
print('H03_VALIDATED; inherited arm pinching recorded, not certified as collision-free')
