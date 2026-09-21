'''Verify H05 preservation, weights and diagnostic rigidity; global contacts use review_hound_h05.py --v14 --audit.'''
import bpy
import json
import math
import sys
import hashlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from hound_h04_review import parts

root = Path(__file__).resolve().parents[2]
scene,source = bpy.data.scenes['Hound_Mesh_v14'],bpy.data.scenes['Hound_Mesh_v12']
bpy.context.window.scene = scene
model,rig = scene.objects['H14_DeformMesh'],scene.objects['Hound14_Rig']
original,old_rig = source.objects['H12_DeformMesh'],source.objects['Hound12_Rig']
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
rebuilt = set(json.loads(scene['gloom_v14_rebuilt_parts']))
morphed = set(json.loads(scene['gloom_v14_morphed_parts']))
added = set(json.loads(scene['gloom_v14_added_parts']))
assert rebuilt=={'Sternum','Abdominal_plate_1','Abdominal_plate_2','Abdominal_plate_3'}|{
    name for name in before if name.startswith(('Breastplate_','Collar_plate_','Rib_flank_','Bracer_blade_',
                                               'Greave_front_','Shin_lower_','Knee_shield_','Ankle_guard_'))}|{
    name for name in before if name.startswith('Finger_') and name.endswith('C') or name in ('Thumb_LB','Thumb_RB')}
assert morphed=={'Collar_blade_middle_L','Collar_blade_middle_R','Greave_mass_L','Greave_mass_R'}
assert added=={prefix+side for prefix in ('Back_blade_','Bracer_blade_distal_','Rib_lamella_2_','Rib_lamella_3_') for side in ('L','R')}
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
assert all(rigid[name]==bone for name,bone in old_rigid.items())
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
for item in json.loads((root/'art/characters/hound/v13/sculpture-reference.json').read_text())['files']:
    assert hashlib.sha256((root/item['path']).read_bytes()).hexdigest()==item['sha256']
    assert (root/item['source']).read_bytes()==(root/item['path']).read_bytes()
result = {'vertices':len(model.data.vertices),'triangles':len(model.data.loop_triangles),'parts':len(after),
          'preserved_parts':preserved,'rebuilt_parts':sorted(rebuilt),'morphed_parts':sorted(morphed),'added_parts':sorted(added),
          'rigid_parts':len(rigid),'bones':len(rig.data.bones),'max_influences':maximum,'diagnostic_frames':61,
          'max_rigid_distance_error_m':rigid_error,'minimum_face_area_m2':minimum_area,'source_files_exact':3,
          'bones_bind_hierarchy_keys_preserved':True}
(root/'.cache/hound-mesh-v14/verification.json').write_text(json.dumps(result,indent=2)+'\n')
print('H14_VERIFIED',json.dumps(result))
