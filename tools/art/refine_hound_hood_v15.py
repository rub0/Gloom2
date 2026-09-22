'''H05: open the lower hood into two hanging cloth ends, preserving every other component and the diagnostic rig.'''
import bpy
import bmesh
import json
import hashlib
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from hound_h04_review import parts
from hound_h01_review import setup,camera

root = Path(__file__).resolve().parents[2]
source = bpy.data.scenes['Hound_Mesh_v14']
for item in json.loads((root/'art/characters/hound/v14/sculpture-reference.json').read_text())['files']:
    assert hashlib.sha256((root/item['path']).read_bytes()).hexdigest()==item['sha256']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original,old_rig = source.objects['H14_DeformMesh'],source.objects['Hound14_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v15')
for key in source.keys():
    scene[key] = source[key]
scene.unit_settings.system,scene.unit_settings.scale_length = 'METRIC',1
scene.render.fps,scene.frame_start,scene.frame_end = 30,1,181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name,frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v15_EXPORT')
scene.collection.children.link(collection)
rig = old_rig.copy()
rig.data = old_rig.data.copy()
rig.name,rig.data.name = 'Hound15_Rig','Hound15_Skeleton'
rig.animation_data.action = old_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound15_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name,model.data.name = 'H15_DeformMesh','H15_DeformTopology'
model.parent,model.modifiers[0].object = rig,rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound14_','Hound15_')
bpy.context.window.scene = scene

ids = parts(original)['Hood_continuous']
assert len(ids)==1106, 'The v07 continuous shell has 23 rings of 48 vertices and two rear caps'
# Remove the lower front sector only; retain the complete rear cloth over the nape.
# Each side is hemmed from outer cloth to its lining, never closed across the sternum.
columns = list(range(28,48))+list(range(21))
starts = list(range(0,624,48))+list(range(625,1105,48))
kept = [start+i for start in starts for i in (columns if start<336 or 625<=start<817 else range(48))]+[624,1105]
remap = {i:j for j,i in enumerate(kept)}
vertices = []
def smooth(t):
    t = max(0,min(1,t))
    return t*t*(3-2*t)
for i in kept:
    p = original.data.vertices[ids[i]].co.copy()
    fall = 1-smooth((p.z-1.43)/.13)
    front = 1-smooth((p.y+.16)/.18) if i<336 or 625<=i<817 else 0
    side = 1 if p.x>=0 else -1
    # Straighten and lower the ends; a broad subtle fold keeps them from reading as a rigid pointed frame.
    width = .109-.019*max(0,min(1,(-p.y-.170)/.024))
    if i>=625:
        width *= .96 # Keep the lining inside the reshaped outer cloth.
    p.x = side*(abs(p.x)*(1-fall*front)+width*fall*front)
    p.z -= .015*fall*front
    p.y -= .007*fall*front
    vertices.append(p)
faces = []
lookup = {i:n for n,i in enumerate(ids)}
for polygon in original.data.polygons:
    if polygon.vertices[0] not in lookup:
        continue
    local = [lookup[i] for i in polygon.vertices]
    if all(i in remap for i in local):
        faces.append(tuple(remap[i] for i in local))
# Hem the two hanging sides and the transition under the neck, leaving the back intact.
outer = list(range(144,337,48))
inner = list(range(625,818,48))
for i in (28,20):
    faces.append(tuple(remap[j+i] for j in (0,48,96,144,625)))
    for a,b,c,d in zip(outer,outer[1:],inner,inner[1:]):
        faces.append(tuple(remap[j] for j in (a+i,b+i,d+i,c+i)))
for i in range(20,28):
    faces.append(tuple(remap[j] for j in (336+i,336+i+1,817+i+1,817+i)))

data = bpy.data.meshes.new('H15_Hood')
data.from_pydata(vertices,[],faces)
for material in model.data.materials:
    data.materials.append(material)
hood_material = next(i for i,m in enumerate(data.materials) if 'Hood' in m.name)
for polygon in data.polygons:
    polygon.material_index,polygon.use_smooth = hood_material,True
bm = bmesh.new()
bm.from_mesh(data)
bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
assert all(e.is_manifold and e.is_contiguous for e in bm.edges)
if bm.calc_volume(signed=True)<0:
    bmesh.ops.reverse_faces(bm,faces=list(bm.faces))
assert bm.calc_volume(signed=True)>0
bm.to_mesh(data)
bm.free()
hood = bpy.data.objects.new('H15_Hood',data)
collection.objects.link(hood)
for j,i in enumerate(kept):
    for weight in original.data.vertices[ids[i]].groups:
        name = original.vertex_groups[weight.group].name
        group = hood.vertex_groups.get(name) or hood.vertex_groups.new(name=name)
        group.add([j],weight.weight,'REPLACE')
bm = bmesh.new()
bm.from_mesh(model.data)
bm.verts.ensure_lookup_table()
bmesh.ops.delete(bm,geom=[bm.verts[i] for i in ids],context='VERTS')
bm.to_mesh(model.data)
bm.free()
bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
hood.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.object.join()
scene['gloom_v15_modified_parts'] = json.dumps(['Hood_continuous'])
scene['gloom_authoring_stage'] = 'H05-hood-candidate-awaiting-artistic-acceptance'
scene['gloom_revision'] = 'Open front hood: two descending cloth ends, no W-shaped connection across the chest'
rig.select_set(True)
scene.frame_set(1)
bpy.context.view_layer.update()
setup(scene,1000,1100)
camera(scene,(2.8,-5,2.2),(0,0,.91),2.1)
draft = root/'.cache/hound-hood-v15'
draft.mkdir(parents=True,exist_ok=True)
path = draft/'hound-draft.blend'
if '--final' in sys.argv:
    path = root/'art/characters/hound/v15/hound-mesh-v15.blend'
    assert not path.exists() and not (root/'assets/characters/hound_rig/v15').exists(), 'Never overwrite a published candidate'
    path.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(path),check_existing=False,compress=True)
model.data.calc_loop_triangles()
print('H15_HOOD',len(vertices),len(model.data.vertices),len(model.data.loop_triangles))
if '--final' in sys.argv:
    export = root/'assets/characters/hound_rig/v15/hound-rig.gltf'
    export.parent.mkdir(parents=True,exist_ok=True)
    bpy.ops.export_scene.gltf(filepath=str(export),export_format='GLTF_SEPARATE',use_selection=True,use_active_scene=True,
        export_yup=True,export_apply=False,export_animations=True,export_animation_mode='ACTIVE_ACTIONS',
        export_nla_strips_merged_animation_name='Hound15_joint_check',export_anim_slide_to_zero=True,export_force_sampling=True,
        export_frame_range=True,export_skins=True,export_def_bones=True,export_rest_position_armature=True,
        export_all_influences=False,export_influence_nb=4,export_extras=True)
    manifest = {'task':'H05','milestone':98,'candidate':'v15','source':'v14','artistic_approval':False,
        'scene':scene.name,'mesh':model.name,'rig':rig.name,'action':rig.animation_data.action.name,'collection':collection.name,
        'modified_parts':['Hood_continuous'],'files':[{'path':p.relative_to(root).as_posix(),
        'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size} for p in (path,export,export.with_suffix('.bin'))]}
    (path.parent/'sculpture-reference.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print('H15_EXPORTED')
