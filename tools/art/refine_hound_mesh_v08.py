'''Art pass: shaped plate fields/rims, arm and facial forms, draped cloth. Preserve the diagnostic skeleton and approved landmarks.'''
import bpy
import bmesh
import json
import math
from mathutils import Vector
from mathutils.bvhtree import BVHTree

if bpy.data.scenes.get('Hound_Mesh_v08'):
    raise RuntimeError('v08 exists; preserve artist changes and choose another version')
source = bpy.data.scenes['Hound_Mesh_v07']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original, original_rig = source.objects['H07_DeformMesh'], source.objects['Hound07_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v08')
for key in source.keys():
    scene[key] = source[key]
scene['gloom_authoring_stage'] = 'v08-artistic-form-pass-awaiting-review-not-final-sculpt'
scene['gloom_limitations'] = 'Secondary forms only; no final facial topology, textures, microdetail, collision certification or production rig'
scene.unit_settings.system, scene.unit_settings.scale_length = 'METRIC', 1
scene.render.fps = 30
scene.frame_start, scene.frame_end = 1, 181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name, frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v08_EXPORT')
scene.collection.children.link(collection)
rig = original_rig.copy()
rig.data = original_rig.data.copy()
rig.name, rig.data.name = 'Hound08_Rig', 'Hound08_Skeleton'
rig.animation_data.action = original_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound08_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name, model.data.name = 'H08_DeformMesh', 'H08_DeformTopology'
model.parent = rig
model.modifiers[0].object = rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound07_', 'Hound08_')
materials = {name: next(m for m in model.data.materials if name in m.name)
             for name in ('Iron', 'EdgePlanes', 'BurgundyCloth', 'Undersuit', 'EyeAccent')}
rigid_parts = json.loads(scene['gloom_rigid_parts'])
bpy.context.window.scene = scene
parts = {g.name[5:]: [] for g in model.vertex_groups if g.name.startswith('PART_')}
for vertex in model.data.vertices:
    for weight in vertex.groups:
        name = model.vertex_groups[weight.group].name
        if name.startswith('PART_'):
            parts[name[5:]].append(vertex.index)


def bell(value, center, width):
    return math.exp(-((value-center)/width)**2)


def smooth(value, low, high):
    t = max(0, min(1, (value-low)/(high-low)))
    return t*t*(3-2*t)


# Broad muscular forms, with restrained separation rather than veins or uniform surface noise.
stations = [(1.523,.244,.008),(1.469,.27,.007),(1.405,.286,.007),(1.36,.307,.001),(1.209,.372,-.008),(.961,.454,-.036)]
for side, suffix in ((1,'L'),(-1,'R')):
    for index in parts['Arm_surface_'+suffix]:
        point = model.data.vertices[index].co
        for (a, ax, ay), (b, bx, by) in zip(stations, stations[1:]):
            if point.z >= b:
                t = (a-point.z)/(a-b)
                center = Vector((side*(ax+(bx-ax)*t), ay+(by-ay)*t, point.z))
                break
        direction = (point-center).normalized()
        front, back, outer = max(0,-direction.y), max(0,direction.y), max(0,side*direction.x)
        relief = .008*bell(point.z,1.325,.065)*front**3+.006*bell(point.z,1.335,.076)*back**3
        relief += .004*bell(point.z,1.45,.055)*outer**2-.004*bell(point.z,1.382,.018)*front**2
        relief -= .003*bell(point.z,1.252,.018)*front**2
        point += direction*relief*smooth(point.z,1.22,1.255)

# Carved cheek planes, orbit and brow/glabella; retain the closed neutral head and its existing envelope.
for index in parts['Head_surface']:
    point = model.data.vertices[index].co
    x, y, z = point
    if y >= -.030:
        continue
    front = smooth(-y,.030,.085)
    hollow = .007*bell(abs(x),.052,.015)*bell(z,1.587,.018)
    orbit = .004*bell(abs(x),.038,.020)*bell(z,1.648,.007)
    fold = .0025*bell(abs(x),.024+.18*(1.59-z),.004)*bell(z,1.582,.020)
    glabella = .004*bell(x,0,.006)*bell(z,1.685,.025)
    jaw = .004*bell(abs(x),.044,.021)*bell(z,1.536,.013)
    brow = .003*bell(abs(x),.024,.013)*bell(z,1.674,.012)
    point.y += front*(hollow+orbit+fold+glabella-jaw-brow)

# Wide cloth folds taper out at joints and at the wrap edges; inner and outer wrap receive the same displacement.
for index in parts['Trousers_continuous']:
    point = model.data.vertices[index].co
    z = point.z
    if not .60 < z < .99:
        continue
    side = 1 if point.x >= 0 else -1
    center_x = side*(.122+(.946-z)*.051/.383)
    radial = Vector((point.x-center_x, point.y-.006, 0)).normalized()
    envelope = smooth(z,.60,.66)*(1-smooth(z,.95,.99))
    fold = .0045*math.sin(32*(z-.66)+7*side*point.x)*envelope
    fold += .004*bell(z,.69+.25*abs(point.x-center_x),.018)*max(0,-radial.y)
    point += radial*fold
for index in parts['Waist_wrap']:
    point = model.data.vertices[index].co
    t = (point.z-.991)/.081
    angle = math.atan2(point.y, point.x)
    fold = .0035*math.sin(math.pi*t)**2*math.sin(3*math.pi*t+1.5*math.cos(angle))
    point += Vector((math.cos(angle),math.sin(angle),0))*fold
model.data.update()
head_indices = set(parts['Head_surface'])
head_tree = BVHTree.FromPolygons([v.co for v in model.data.vertices],
                               [list(p.vertices) for p in model.data.polygons if all(i in head_indices for i in p.vertices)])


def face_point(x, z, offset):
    hit = head_tree.ray_cast(Vector((x,-.3,z)),Vector((0,1,0)),.4)[0]
    assert hit is not None, 'Missing facial surface'
    return hit+Vector((0,-offset,0))


plate_prefixes = ('Breastplate_', 'Rib_flank_', 'Hip_guard_', 'Abdominal_plate_', 'Sternum', 'Scapula_', 'Back_spine_',
                  'Bracer_face_', 'Elbow_cuff_', 'Knee_shield_', 'Greave_front_', 'Shin_lower_', 'Thigh_outer_plate_',
                  'Boot_toe_', 'Ankle_guard_', 'Hand_back_full_point_', 'Sash_tail_')
rebuilt = {name for name in parts if name.startswith(plate_prefixes)} | {'Mouth_shadow','Eye_L','Eye_R'}
removed = {index for name in rebuilt for index in parts[name]}
bm = bmesh.new()
bm.from_mesh(model.data)
bm.verts.ensure_lookup_table()
bmesh.ops.delete(bm,geom=[bm.verts[i] for i in removed],context='VERTS')
bm.to_mesh(model.data)
bm.free()
for name in rebuilt:
    model.vertex_groups.remove(model.vertex_groups['PART_'+name])
generated = []


def add_surface(name, vertices, faces, face_materials):
    data = bpy.data.meshes.new('H08_'+name)
    data.from_pydata(vertices,[],faces)
    for material in materials.values():
        data.materials.append(material)
    material_indices = {name:i for i,name in enumerate(materials)}
    for polygon, material in zip(data.polygons,face_materials):
        polygon.material_index = material_indices[material]
        polygon.use_smooth = True
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    assert all(e.is_manifold for e in bm.edges), name
    bm.to_mesh(data)
    bm.free()
    data.set_sharp_from_angle(angle=math.radians(50))
    obj = bpy.data.objects.new(data.name,data)
    collection.objects.link(obj)
    obj.vertex_groups.new(name='PART_'+name).add(list(range(len(vertices))),1,'REPLACE')
    obj.vertex_groups.new(name=rigid_parts[name]).add(list(range(len(vertices))),1,'REPLACE')
    generated.append(obj)


def bridge(faces, materials, a, b, material):
    for i in range(len(a)):
        faces.append((a[i],b[i],b[(i+1)%len(b)],a[(i+1)%len(a)]))
        materials.append(material)


# Rebuild the v02 design outlines as broad domed fields with a narrow forged rim, not a single triangular pyramid.
# ponytail: secondary forms only; hand finishing and bake/retopology remain an explicit art gate.
for name in sorted(rebuilt-{'Mouth_shadow','Eye_L','Eye_R'}):
    reference = bpy.data.scenes['Hound_Blockout_v02'].objects['H02_'+name]
    # Archived collections are excluded from evaluation; use stored local transforms, not stale matrix_world.
    assert reference.parent is None
    transform = reference.matrix_basis
    count = (len(reference.data.vertices)-1)//2
    assert len(reference.data.vertices) == count*2+1
    outline = [transform@v.co for v in reference.data.vertices[:count]]
    center = sum(outline,Vector())/count
    back_center = sum((transform@v.co for v in reference.data.vertices[count:count*2]),Vector())/count
    normal = (center-back_center).normalized()
    # Keep every outline corner, including the pointed hand end. Subdivision adds no silhouette bulge.
    border = [point.lerp(outline[(i+1)%count],t/4) for i,point in enumerate(outline) for t in range(4)]
    cloth = name.startswith('Sash_tail_')
    thickness = (center-back_center).length
    depth = .009 if cloth else .016 if name.startswith(('Breastplate_','Knee_','Hip_')) else .012
    field_material = 'BurgundyCloth' if cloth else 'Iron'
    edge_material = field_material if cloth else 'EdgePlanes'
    vertices, faces, face_materials = [], [], []
    previous = None
    profile = [(1,0),(.982,.001),(.956,.0025),(.93,.0025),(.87,.001),(.65,depth*.60),(.35,depth),(.12,depth*1.06)]
    if cloth:
        profile = [(r,.001*math.sin(math.pi*r)) for r,lift in profile]
    for r, lift in profile:
        ring = list(range(len(vertices),len(vertices)+len(border)))
        for point in border:
            vertex = center+(point-center)*r+normal*lift
            if cloth:
                vertex += normal*(.004*math.sin(38*vertex.x+24*vertex.z)*(1-r)*r)
            vertices.append(vertex)
        if previous is not None:
            material = edge_material if .95 < r < .97 else field_material
            bridge(faces,face_materials,previous,ring,material)
        previous = ring
    cap = len(vertices)
    vertices.append(center if cloth else center+normal*depth*1.06)
    for i in range(len(border)):
        faces.append((previous[i],previous[(i+1)%len(border)],cap))
        face_materials.append(field_material)
    back = list(range(len(vertices),len(vertices)+len(border)))
    vertices.extend(p-normal*thickness for p in border)
    bridge(faces,face_materials,list(range(len(border))),back,field_material)
    faces.append(tuple(back))
    face_materials.append(field_material)
    add_surface(name,vertices,faces,face_materials)

# Small fitted emissive eyes and a closed lip crease replace the protruding blockout pyramids.
for side,suffix in ((1,'L'),(-1,'R')):
    outline = [(side*.019,1.650),(side*.057,1.659),(side*.052,1.649),(side*.026,1.645)]
    front = [face_point(x,z,.0015) for x,z in outline]
    back = [point+Vector((0,.003,0)) for point in front]
    faces = [(0,1,2,3),(7,6,5,4)]+[(i,4+i,4+(i+1)%4,(i+1)%4) for i in range(4)]
    add_surface('Eye_'+suffix,front+back,faces,['EyeAccent']*len(faces))
vertices, faces = [], []
for i in range(25):
    t = -1+2*i/24
    x, z = .029*t, 1.562-.0025*t*t
    half_height = .0003+.0008*(1-t*t)
    upper, lower = face_point(x,z+half_height,.001),face_point(x,z-half_height,.001)
    vertices.extend((upper,lower,upper+Vector((0,.002,0)),lower+Vector((0,.002,0))))
for i in range(24):
    a,b = i*4,(i+1)*4
    faces.extend(((a,b,b+1,a+1),(a+2,a+3,b+3,b+2),(a,a+2,b+2,b),(a+1,b+1,b+3,a+3)))
faces.extend(((0,1,3,2),(96,98,99,97)))
add_surface('Mouth_shadow',vertices,faces,['Undersuit']*len(faces))

bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
for obj in generated:
    obj.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.object.join()
scene['gloom_v08_rebuilt_parts'] = json.dumps(sorted(rebuilt))
scene['gloom_v08_morphed_parts'] = json.dumps(['Arm_surface_L','Arm_surface_R','Head_surface','Trousers_continuous','Waist_wrap'])
scene['gloom_revision'] = 'Artistic plate fields and rims, muscular/facial relief, draped cloth; preserved hood/collar/fingers and diagnostic rig'
scene.frame_set(1)
bpy.context.view_layer.update()
model.data.calc_loop_triangles()
print('HOUND_V08_ART_PASS',len(model.data.vertices),'vertices',len(model.data.loop_triangles),'triangles',len(rebuilt),'rebuilt parts')
