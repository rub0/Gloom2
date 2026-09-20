'''H04: cloth folds, thin overlapping plates and a supported sleeveless cuirass, preserving H01-H03 and the diagnostic rig.'''
import bpy
import bmesh
import json
import math
import sys
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h04_review as review

root = Path(__file__).resolve().parents[2]
assert not (root/'art/characters/hound/v12/hound-mesh-v12.blend').exists(), 'Preserve existing v12 source'
source = bpy.data.scenes['Hound_Mesh_v11']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original,old_rig = source.objects['H11_DeformMesh'],source.objects['Hound11_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v12')
for key in source.keys():
    scene[key] = source[key]
scene['gloom_authoring_stage'] = 'H04-cloth-and-assembly-not-H05-approval'
scene['gloom_limitations'] = 'Diagnostic FK only; no cloth simulation, production rig, final textures or global artistic approval'
scene.unit_settings.system,scene.unit_settings.scale_length = 'METRIC',1
scene.render.fps,scene.frame_start,scene.frame_end = 30,1,181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name,frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v12_EXPORT')
scene.collection.children.link(collection)
rig = old_rig.copy()
rig.data = old_rig.data.copy()
rig.name,rig.data.name = 'Hound12_Rig','Hound12_Skeleton'
rig.animation_data.action = old_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound12_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name,model.data.name = 'H12_DeformMesh','H12_DeformTopology'
model.parent,model.modifiers[0].object = rig,rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound11_','Hound12_')
bpy.context.window.scene = scene
parts = review.parts(model)
rebuilt = {'Sash_tail_L','Sash_tail_R'}
morphed = {'Trousers_continuous','Waist_wrap','Torso_underlayer'}
plate_names = {n for n in parts if n.startswith(('Breastplate_','Abdominal_','Back_spine_','Scapula_','Rib_flank_'))}
morphed |= plate_names
added = {'Yoke_L','Yoke_R'}
rigid = json.loads(scene['gloom_rigid_parts'])
generated = []


def smooth(value,low,high):
    t = max(0,min(1,(value-low)/(high-low)))
    return t*t*(3-2*t)


def bell(value,center,width):
    return math.exp(-((value-center)/width)**2)


# Cloth tension converges toward the crotch; broad compression folds sit above each knee.
# Keep the H02 lower lining (Z <= .61) and the upper waist attachment exact.
for index in parts['Trousers_continuous']:
    p = model.data.vertices[index].co
    if not .61 < p.z < .98:
        continue
    envelope = smooth(p.z,.61,.67)*(1-smooth(p.z,.94,.98))
    center = .122+(.946-p.z)*.051/.383
    side = 1 if p.y<0 else -1
    relief = .008*bell(p.z,.895-.38*abs(p.x),.019)-.005*bell(p.z,.873-.38*abs(p.x),.015)
    relief += .009*bell(p.z,.668+.22*abs(abs(p.x)-center),.022)
    relief -= .004*bell(p.z,.708+.22*abs(abs(p.x)-center),.019)
    relief += .004*bell(abs(p.x),center-.026,.022)*bell(p.z,.78,.11)
    p.y -= side*relief*envelope*smooth(abs(p.y),.025,.072)

# One hollow wrap, 3 mm radial wall, with two diagonal folds instead of stacked horizontal bands.
ids = parts['Waist_wrap']
assert len(ids)==2*13*48
for inner in (0,1):
    for row in range(13):
        t = row/12
        for i in range(48):
            angle = math.tau*i/48
            width = .196+(.181-.196)*min(t/.69,1)+(.175-.181)*max(0,(t-.69)/.31)
            depth = .121+(.126-.121)*min(t/.69,1)+(.113-.126)*max(0,(t-.69)/.31)
            phase = .22*math.cos(angle)+.055*math.sin(2*angle)
            fold = (.0055*bell(t,.34+phase,.15)+.004*bell(t,.70+phase,.13)-.003*bell(t,.52+phase,.10))*math.sin(math.pi*t)
            wall = inner*.003
            z = .991+.081*t-.0045*math.sin(math.pi*t)*max(0,-math.sin(angle))
            model.data.vertices[ids[inner*13*48+row*48+i]].co = ((width+fold-wall)*math.cos(angle),(depth+fold-wall)*math.sin(angle),z)

# Turn the faceted torso backing into a tailored sleeveless support. Its envelope and weights remain.
for index in parts['Torso_underlayer']:
    p = model.data.vertices[index].co
    envelope = smooth(p.z,1.055,1.10)*(1-smooth(p.z,1.42,1.48))
    angle = math.atan2(p.y,p.x)
    p.y += math.sin(angle)*.0028*bell(abs(p.x),.145,.036)*math.sin((p.z-1.08)*18)*envelope
    # Raised backing beneath the overlapping lamellae joins their returns to the same padded vest.
    facing = 1-smooth(abs(p.x),.025,.095)
    height = smooth(p.z,1.045,1.09)*(1-smooth(p.z,1.32,1.36))
    if p.y<0:
        p.y += min(0,-.129-p.y)*facing*height
    else:
        p.y += max(0,.150-p.y)*facing*height
torso_ids = set(parts['Torso_underlayer'])
for polygon in model.data.polygons:
    if all(i in torso_ids for i in polygon.vertices):
        polygon.use_smooth = True

# V08 panels had deep solid backs. Keep their entire outer sculpt, replace only the back perimeter with a 4 mm return.
# Neighboring lamellae keep their designed overlap; the concealed back is not an extra visible armor block.
for name in sorted(plate_names):
    ids = parts[name]
    count = (len(ids)-1)//9
    assert len(ids)==9*count+1
    front = [model.data.vertices[i].co.copy() for i in ids[:count]]
    back = [model.data.vertices[i].co.copy() for i in ids[-count:]]
    normal = (sum(front,Vector())-sum(back,Vector())).normalized()
    for j,index in enumerate(ids[-count:]):
        model.data.vertices[index].co = front[j]-normal*.004
model.data.update()


def weights(index):
    return {model.vertex_groups[w.group].name:w.weight for w in model.data.vertices[index].groups
            if model.vertex_groups[w.group].name in rig.data.bones}


def surface(name,vertices,faces,values,material):
    data = bpy.data.meshes.new('H12_'+name)
    data.from_pydata(vertices,[],faces)
    for mat in model.data.materials:
        data.materials.append(mat)
    material_index = next(i for i,m in enumerate(data.materials) if material in m.name)
    for polygon in data.polygons:
        polygon.material_index,polygon.use_smooth = material_index,True
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    assert all(e.is_manifold for e in bm.edges),name
    bm.to_mesh(data)
    bm.free()
    obj = bpy.data.objects.new(data.name,data)
    collection.objects.link(obj)
    obj.vertex_groups.new(name='PART_'+name).add(list(range(len(vertices))),1,'REPLACE')
    for i,row in enumerate(values):
        for bone,value in row.items():
            if value>1e-6:
                group = obj.vertex_groups.get(bone) or obj.vertex_groups.new(name=bone)
                group.add([i],value,'REPLACE')
    generated.append(obj)


# Fit two short hanging cloth panels to the trousers. Both surfaces share the same sampled weights.
model.data.calc_loop_triangles()
ids = set(parts['Trousers_continuous'])
triangles = [list(t.vertices) for t in model.data.loop_triangles if all(i in ids for i in t.vertices)]
tree = BVHTree.FromPolygons([v.co for v in model.data.vertices],triangles,all_triangles=True)
for side,suffix in ((1,'L'),(-1,'R')):
    vertices,faces,values = [],[],[]
    columns,rows = 16,12
    for back in (False,True):
        for row in range(rows+1):
            t = row/rows
            for col in range(columns+1):
                u = col/columns
                x = side*(.012+.131*u)
                hem = .944-.022*math.sin(math.pi*u)
                z = 1.014+(hem-1.014)*t
                hit,normal,tri,distance = tree.ray_cast(Vector((x,-.35,z)),Vector((0,1,0)),.5)
                assert hit is not None,(x,z)
                offset = (.0065+.026*t*t)*smooth(t,0,.38)-.001*(1-smooth(t,0,.38))
                offset += .002*math.sin(math.pi*u)*math.sin(math.pi*t)
                # Under the wrap at the upper edge; free hem remains outside the trousers.
                y = hit.y-offset+(.003 if back else 0)
                vertices.append(Vector((x,y,z)))
                a,b,c = [model.data.vertices[i].co for i in triangles[tri]]
                ab,ac,ap = b-a,c-a,hit-a
                d00,d01,d11,d20,d21 = ab.dot(ab),ab.dot(ac),ac.dot(ac),ap.dot(ab),ap.dot(ac)
                v = (d11*d20-d01*d21)/(d00*d11-d01*d01)
                w = (d00*d21-d01*d20)/(d00*d11-d01*d01)
                row_weights = {}
                for index,factor in zip(triangles[tri],(1-v-w,v,w)):
                    for bone,weight in weights(index).items():
                        row_weights[bone] = row_weights.get(bone,0)+max(0,factor)*weight
                selected = sorted(row_weights,key=row_weights.get,reverse=True)[:2]
                total = sum(row_weights[n] for n in selected)
                values.append({n:row_weights[n]/total for n in selected if row_weights[n]/total>1e-6})
    layer = (rows+1)*(columns+1)
    for start in (0,layer):
        for row in range(rows):
            for col in range(columns):
                a = start+row*(columns+1)+col
                faces.append((a,a+1,a+columns+2,a+columns+1))
    border = list(range(columns+1))+[r*(columns+1)+columns for r in range(1,rows+1)]
    border += [rows*(columns+1)+c for c in range(columns-1,-1,-1)]+[r*(columns+1) for r in range(rows-1,0,-1)]
    faces += [(a,b,b+layer,a+layer) for a,b in zip(border,border[1:]+border[:1])]
    surface('Sash_tail_'+suffix,vertices,faces,values,'BurgundyCloth')
    del rigid['Sash_tail_'+suffix]

# Two plain medial yokes bridge the breast/back plates underneath the clavicular bases; deltoids remain uncovered.
profile = [(-.150,1.425),(-.149,1.458),(-.102,1.484),(-.066,1.498),(-.015,1.502),(.04,1.495),(.11,1.473),(.137,1.438)]
for side,suffix in ((1,'L'),(-1,'R')):
    vertices,faces = [],[]
    for y,z in profile:
        x = side*.173
        vertices.extend((Vector((x-side*.016,y,z)),Vector((x+side*.016,y,z)),
                         Vector((x-side*.016,y,z-.004)),Vector((x+side*.016,y,z-.004))))
    for i in range(len(profile)-1):
        a,b = 4*i,4*(i+1)
        faces += [(a,b,b+1,a+1),(a+2,a+3,b+3,b+2),(a,a+2,b+2,b),(a+1,b+1,b+3,a+3)]
    faces += [(0,1,3,2),(28,30,31,29)]
    surface('Yoke_'+suffix,vertices,faces,[{'Bip001 Spine2':1}]*len(vertices),'Undersuit')
    rigid['Yoke_'+suffix] = 'Bip001 Spine2'


# The H03 arm surfaces demonstrated visible intrusions into clavicular bases.
# Trim only the hidden seating volume across the reviewed reach; preserve the blade tips and original outer planes.

collar_trim = {n for n in parts if n.startswith(('Collar_plate_','Collar_blade_middle_','Collar_blade_outer_'))}
old_action = old_rig.animation_data.action
old_rig.animation_data.action = None
for side,suffix in ((1,'L'),(-1,'R')):
    points = []
    arm_ids = [i for i in parts['Arm_surface_'+suffix] if original.data.vertices[i].co.z>1.38]
    center = Vector((side*.274,.004,1.437))
    for amount in (0,.25,.5,.75,1):
        review.pose(source,old_rig,'reach',amount)
        evaluated = original.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = evaluated.to_mesh()
        points.extend(center+(mesh.vertices[i].co-center)*1.045 for i in arm_ids)
        evaluated.to_mesh_clear()
    bm = bmesh.new()
    for point in points:
        bm.verts.new(point)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.000001)
    bmesh.ops.convex_hull(bm,input=list(bm.verts))
    bmesh.ops.delete(bm,geom=[v for v in bm.verts if not v.link_faces],context='VERTS')
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    if bm.calc_volume(signed=True)<0:
        bmesh.ops.reverse_faces(bm,faces=list(bm.faces))
    data = bpy.data.meshes.new('H04_LocalShoulderEnvelope')
    bm.to_mesh(data)
    bm.free()
    cutter = bpy.data.objects.new(data.name,data)
    collection.objects.link(cutter)
    bpy.context.window.scene = scene
    for name in sorted(n for n in collar_trim if n.endswith('_'+suffix)):
        ids = parts[name]
        remap = {old:i for i,old in enumerate(ids)}
        polygons = [p for p in model.data.polygons if all(i in remap for i in p.vertices)]
        data = bpy.data.meshes.new('H12_'+name)
        data.from_pydata([model.data.vertices[i].co for i in ids],[],[[remap[i] for i in p.vertices] for p in polygons])
        for mat in model.data.materials:
            data.materials.append(mat)
        for polygon,old in zip(data.polygons,polygons):
            polygon.material_index,polygon.use_smooth = old.material_index,old.use_smooth
        bm = bmesh.new()
        bm.from_mesh(data)
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        if bm.calc_volume(signed=True)<0:
            bmesh.ops.reverse_faces(bm,faces=list(bm.faces))
        bm.to_mesh(data)
        bm.free()
        obj = bpy.data.objects.new(data.name,data)
        collection.objects.link(obj)
        bpy.context.view_layer.objects.active = obj
        modifier = obj.modifiers.new('Local shoulder seating','BOOLEAN')
        modifier.operation,modifier.solver,modifier.object = 'DIFFERENCE','EXACT',cutter
        bpy.ops.object.modifier_apply(modifier=modifier.name)
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.000001)
        bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=.000001)
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(obj.data)
        bm.free()
        obj.vertex_groups.new(name='PART_'+name).add(list(range(len(obj.data.vertices))),1,'REPLACE')
        obj.vertex_groups.new(name=rigid[name]).add(list(range(len(obj.data.vertices))),1,'REPLACE')
        generated.append(obj)
    bpy.data.objects.remove(cutter,do_unlink=True)
old_rig.animation_data.action = old_action
source.frame_set(1)
bpy.context.window.scene = scene
rebuilt |= collar_trim

bm = bmesh.new()
bm.from_mesh(model.data)
bm.verts.ensure_lookup_table()
removed = {i for name in rebuilt for i in parts[name]}
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
scene['gloom_v12_rebuilt_parts'] = json.dumps(sorted(rebuilt))
scene['gloom_v12_morphed_parts'] = json.dumps(sorted(morphed))
scene['gloom_v12_added_parts'] = json.dumps(sorted(added))
scene['gloom_rigid_parts'] = json.dumps(rigid,sort_keys=True)
scene['gloom_soft_parts'] = json.dumps(json.loads(scene['gloom_soft_parts'])+['Sash_tail_L','Sash_tail_R'])
scene['gloom_revision'] = 'H04: broad trouser folds, diagonal hollow wrap, fitted 3 mm cloth hems, 4 mm plate returns and medial support yokes'
scene.frame_set(1)
bpy.context.view_layer.update()
model.data.calc_loop_triangles()
draft = root/'.cache/hound-mesh-v12'
draft.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(draft/'hound-draft.blend'),check_existing=False,compress=True)
print('H04_DRAFT',len(model.data.vertices),len(model.data.loop_triangles),'rebuilt',len(rebuilt),'morphed',len(morphed),'added',len(added))
