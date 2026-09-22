'''Verify H05 preservation, weights and diagnostic rigidity; global contacts use review_hound_h05.py --v16 --audit.'''
import bpy
import json
import math
import sys
import hashlib
from mathutils import Vector
from mathutils.bvhtree import BVHTree
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from hound_h04_review import parts

root = Path(__file__).resolve().parents[2]
scene,source = bpy.data.scenes['Hound_Mesh_v16'],bpy.data.scenes['Hound_Mesh_v15']
bpy.context.window.scene = scene
model,rig = scene.objects['H16_DeformMesh'],scene.objects['Hound16_Rig']
original,old_rig = source.objects['H15_DeformMesh'],source.objects['Hound15_Rig']
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

def weights(obj,index):
    return {obj.vertex_groups[w.group].name:w.weight for w in obj.data.vertices[index].groups
            if obj.vertex_groups[w.group].name in rig.data.bones}

before,after = parts(original),parts(model)
rebuilt = set(json.loads(scene['gloom_v16_rebuilt_parts']))
morphed = set()
added = set()
assert rebuilt=={n for n in before if n.startswith(('Scapula_','Back_spine_','Rib_flank_','Rib_lamella_'))}
assert len(rebuilt)==11 and 'Hood_continuous' not in rebuilt
assert set(after)==set(before)|added
preserved = 0
for name,indices in before.items():
    if name in rebuilt:
        continue
    other = after[name]
    assert len(indices)==len(other)
    for a,b in zip(indices,other):
        assert weights(original,a)==weights(model,b)
        if name not in morphed:
            assert original.data.vertices[a].co==model.data.vertices[b].co,name
    a_map,b_map = {v:i for i,v in enumerate(indices)},{v:i for i,v in enumerate(other)}
    a_faces = [(tuple(a_map[i] for i in p.vertices),p.material_index,p.use_smooth)
               for p in original.data.polygons if all(i in a_map for i in p.vertices)]
    b_faces = [(tuple(b_map[i] for i in p.vertices),p.material_index,p.use_smooth)
               for p in model.data.polygons if all(i in b_map for i in p.vertices)]
    assert a_faces==b_faces,name
    preserved += name not in morphed
assert all(tuple(a.diffuse_color)==tuple(b.diffuse_color) for a,b in zip(original.data.materials,model.data.materials))
maximum = 0
for vertex in model.data.vertices:
    values = list(weights(model,vertex.index).values())
    assert values and abs(sum(values)-1)<2e-6 and all(0<w<=1 for w in values)
    maximum = max(maximum,len(values))
assert maximum<=2
rigid = json.loads(scene['gloom_rigid_parts'])
old_rigid = json.loads(source['gloom_rigid_parts'])
assert all(rigid[name]==bone for name,bone in old_rigid.items() if name!='Back_spine_1')
assert rigid['Back_spine_1']=='Bip001 Spine1' and old_rigid['Back_spine_1']=='Bip001 Spine'
assert set(rigid)==set(old_rigid)|added
for name,bone in rigid.items():
    assert all(weights(model,i)=={bone:1.0} for i in after[name])
rest = [v.co.copy() for v in model.data.vertices]
rigid_error,minimum_area = 0,1
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
        ids = after[name]
        origin = ids[0]
        for i in ids[1:]:
            error = abs((mesh.vertices[i].co-mesh.vertices[origin].co).length-(rest[i]-rest[origin]).length)
            rigid_error = max(rigid_error,error)
    ev.to_mesh_clear()
assert rigid_error<2e-6
scene.frame_set(1)
model.data.calc_loop_triangles()
for version in ('v13','v14','v15'):
    for item in json.loads((root/f'art/characters/hound/{version}/sculpture-reference.json').read_text())['files']:
        assert hashlib.sha256((root/item['path']).read_bytes()).hexdigest()==item['sha256']
# Exterior radial samples document coverage of the torso bands, not a full-area safety claim.
coverage = {}
for version,obj in (('v15',original),('v16',model)):
    groups = parts(obj)
    ids = {i for name,indices in groups.items() if name.startswith(('Breastplate_','Sternum','Abdominal_','Rib_','Scapula_','Back_spine_')) for i in indices}
    tree = BVHTree.FromPolygons([v.co for v in obj.data.vertices],
                               [list(p.vertices) for p in obj.data.polygons if p.vertices[0] in ids])
    coverage[version] = {}
    for z in (1.11,1.205,1.25,1.285,1.40):
        hits = []
        for degrees in range(0,360,5):
            angle = math.radians(degrees)
            direction = Vector((math.sin(angle),-math.cos(angle),0))
            hit = tree.ray_cast(Vector((0,0,z))+direction*.75,-direction,.749)[0]
            hits.append(hit is not None)
        coverage[version][str(z)] = {'armor_directions':sum(hits),'samples':len(hits),
                                    'front':hits[0],'right':hits[18],'back':hits[36],'left':hits[54]}
assert all(coverage['v16'][str(z)][side] for z in (1.11,1.205,1.25) for side in ('front','right','back','left'))
result = {'vertices':len(model.data.vertices),'triangles':len(model.data.loop_triangles),'parts':len(after),
          'preserved_parts':preserved,'rebuilt_parts':sorted(rebuilt),'morphed_parts':sorted(morphed),'added_parts':sorted(added),
          'rigid_parts':len(rigid),'bones':len(rig.data.bones),'max_influences':maximum,'diagnostic_frames':61,
          'max_rigid_distance_error_m':rigid_error,'minimum_face_area_m2':minimum_area,'source_files_exact':9,'torso_radial_coverage':coverage,
          'bones_bind_hierarchy_keys_preserved':True}
(root/'.cache/hound-armor-v16/verification.json').write_text(json.dumps(result,indent=2)+'\n')
print('H16_VERIFIED',json.dumps(result))
