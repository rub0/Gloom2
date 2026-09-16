'''H01: articulated metal shells over the connected glove; preserve v08 and the diagnostic rig.'''
import bpy
import bmesh
import json
import math
from pathlib import Path
from mathutils import Vector

root = Path(__file__).resolve().parents[2]
assert not (root/'art/characters/hound/v09/hound-mesh-v09.blend').exists(), 'Preserve existing v09 edits'
source = bpy.data.scenes['Hound_Mesh_v08']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original, original_rig = source.objects['H08_DeformMesh'], source.objects['Hound08_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v09')
for key in source.keys():
    scene[key] = source[key]
scene['gloom_authoring_stage'] = 'H01-hands-gauntlets-not-final-sculpt'
scene['gloom_limitations'] = 'Diagnostic hands only; global art approval H05 and production grips H08 pending'
scene.unit_settings.system, scene.unit_settings.scale_length = 'METRIC', 1
scene.render.fps, scene.frame_start, scene.frame_end = 30, 1, 181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name, frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v09_EXPORT')
scene.collection.children.link(collection)
rig = original_rig.copy()
rig.data = original_rig.data.copy()
rig.name, rig.data.name = 'Hound09_Rig', 'Hound09_Skeleton'
rig.animation_data.action = original_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound09_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name, model.data.name = 'H09_DeformMesh', 'H09_DeformTopology'
model.parent, model.modifiers[0].object = rig, rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound08_', 'Hound09_')
bpy.context.window.scene = scene
parts = {g.name[5:]: [] for g in model.vertex_groups if g.name.startswith('PART_')}
for vertex in model.data.vertices:
    for weight in vertex.groups:
        name = model.vertex_groups[weight.group].name
        if name.startswith('PART_'):
            parts[name[5:]].append(vertex.index)
rigid = json.loads(scene['gloom_rigid_parts'])
rebuilt = {name for name in parts if name.startswith(('Bracer_mass_', 'Wrist_transition_', 'Finger_', 'Thumb_'))}
materials = {name: next(i for i,m in enumerate(model.data.materials) if name in m.name) for name in ('Iron','EdgePlanes')}
generated = []


def surface(name, vertices, faces, accents):
    data = bpy.data.meshes.new('H09_'+name)
    data.from_pydata(vertices, [], faces)
    for material in model.data.materials:
        data.materials.append(material)
    for polygon, accent in zip(data.polygons, accents):
        polygon.material_index = materials['EdgePlanes' if accent else 'Iron']
        polygon.use_smooth = True
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    assert all(e.is_manifold for e in bm.edges), name
    bm.to_mesh(data)
    bm.free()
    data.set_sharp_from_angle(angle=math.radians(45))
    obj = bpy.data.objects.new(data.name, data)
    collection.objects.link(obj)
    obj.vertex_groups.new(name='PART_'+name).add(list(range(len(vertices))), 1, 'REPLACE')
    obj.vertex_groups.new(name=rigid[name]).add(list(range(len(vertices))), 1, 'REPLACE')
    generated.append(obj)


def sleeve(name, rings, wall):
    vertices, faces, accents = [], [], []
    n = len(rings[0])
    for inner in (False, True):
        for ring in rings:
            center = sum(ring, Vector())/n
            vertices.extend(p-(p-center).normalized()*wall if inner else p for p in ring)
    rows = len(rings)
    for layer in (0,1):
        for row in range(rows-1):
            for i in range(n):
                a, b = layer*rows*n+row*n+i, layer*rows*n+row*n+(i+1)%n
                faces.append((a,b,b+n,a+n))
                accents.append(False)
    for row in (0,rows-1):
        for i in range(n):
            a,b = row*n+i,row*n+(i+1)%n
            faces.append((a,b,b+rows*n,a+rows*n))
            accents.append(True)
    surface(name,vertices,faces,accents)


for side,suffix in ((1,'L'),(-1,'R')):
    wrist = rig.data.bones['Bip001 '+suffix+' Hand'].head_local
    down = Vector((side*.16,-.08,-1)).normalized()
    front = Vector((0,-1,0))
    front = (front-down*front.dot(down)).normalized()
    dorsal = front.cross(down).normalized()*side
    name = 'Bracer_mass_'+suffix
    old = [original.data.vertices[i].co.copy() for i in parts[name]]
    rings = []
    for t in (0,.025,.20,.34,.65,.86,.975,1):
        u = t*3
        index, blend = min(2,int(u)), min(1,u-min(2,int(u)))
        ring = [old[index*12+i].lerp(old[(index+1)*12+i],blend) for i in range(12)]
        center = sum(ring,Vector())/12
        refined = []
        for i in range(24):
            a,b = ring[i//2],ring[(i//2+1)%12]
            point = a.copy() if i%2 == 0 else center+(a.lerp(b,.5)-center).normalized()*((a-center).length+(b-center).length)/2
            # Small lip relief gives the unchanged dorsal plate room above the wrist pivot.
            point -= down*(.016*t**5)
            point += (point-center).normalized()*(.0015*math.sin(math.pi*min(t,1-t)/.05) if min(t,1-t)<.05 else 0)
            refined.append(point)
        rings.append(refined)
    sleeve(name,rings,.007)
    rings = []
    for v,ru,rw in ((.007,.039,.024),(.009,.040,.025),(.021,.042,.024),(.025,.040,.023)):
        rings.append([wrist-dorsal*.008+down*v+front*(ru*math.cos(math.tau*i/24))+dorsal*(rw*math.sin(math.tau*i/24)) for i in range(24)])
    sleeve('Wrist_transition_'+suffix,rings,.004)

    # The front guard must stop above the moving thumb/wrist instead of entering the glove.
    for index in parts['Bracer_face_'+suffix]:
        point = model.data.vertices[index].co
        t = max(0,min(1,(1.055-point.z)/.121))
        point -= down*(.050*t*t*(3-2*t))

    # Preserve glove connectivity/weights; add a shallow thenar pad and palm hollow.
    for index in parts['Hand_glove_'+suffix]:
        p = model.data.vertices[index].co
        local = p-wrist
        u,v,w = local.dot(front),local.dot(down),local.dot(dorsal)
        if w > .0185:
            p -= dorsal*(w-.0185)
        if w < 0 and -.01 < v < .105:
            thenar = .0025*math.exp(-((u-.026)/.017)**2-((v-.047)/.027)**2)
            hollow = .0018*math.exp(-(u/.023)**2-((v-.065)/.024)**2)
            p += dorsal*(hollow-thenar)*min(1,-w/.018)
    for name in sorted(n for n in rebuilt if n.startswith(('Finger_'+suffix,'Thumb_'+suffix))):
        bone = rig.data.bones[rigid[name]]
        tangent = (bone.tail_local-bone.head_local).normalized()
        across = down if name.startswith('Thumb') else front
        across = (across-tangent*across.dot(tangent)).normalized()
        outward = across.cross(tangent).normalized()
        if outward.dot(dorsal)<0:
            outward = -outward
        digit = 0 if name.startswith('Thumb') else int(name[-2])+1
        segment = ord(name[-1])-ord('A')
        radius = (.017 if segment == 0 else .014) if digit == 0 else (.012 if digit<3 else .011 if digit==3 else .010)*(1-.08*segment)
        vertices,faces,accents = [],[],[]
        rows,cols = 5,11
        for inner in (False,True):
            for row,t in enumerate((.23,.29,.48,.73,.80) if digit == 0 else (.15,.22,.48,.78,.86)):
                r = radius*(1-.19*t)-(0.0022 if inner else 0)
                if row in (0,rows-1):
                    r -= .0006
                center = bone.head_local.lerp(bone.tail_local,t)
                for col in range(cols):
                    angle = math.radians(-100+200*col/(cols-1))
                    vertices.append(center+across*(math.sin(angle)*r*.94)+outward*(math.cos(angle)*r))
        for layer in (0,1):
            for row in range(rows-1):
                for col in range(cols-1):
                    a = layer*rows*cols+row*cols+col
                    faces.append((a,a+1,a+1+cols,a+cols))
                    accents.append(layer==0 and row==0)
        perimeter = list(range(cols))+[row*cols+cols-1 for row in range(1,rows)]
        perimeter += list(range(rows*cols-2,(rows-1)*cols-1,-1))+[row*cols for row in range(rows-2,0,-1)]
        for a,b in zip(perimeter,perimeter[1:]+perimeter[:1]):
            faces.append((a,b,b+rows*cols,a+rows*cols))
            accents.append(False)
        surface(name,vertices,faces,accents)

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
scene['gloom_v09_rebuilt_parts'] = json.dumps(sorted(rebuilt))
scene['gloom_v09_morphed_parts'] = json.dumps(['Hand_glove_L','Hand_glove_R','Bracer_face_L','Bracer_face_R'])
scene['gloom_revision'] = 'H01: 7mm bracer shells, 4mm wrist cuffs, 2.2mm articulated dorsal finger shells palm pads and relieved distal front guards'
scene.frame_set(1)
bpy.context.view_layer.update()
model.data.calc_loop_triangles()
print('HOUND_V09',len(model.data.vertices),len(model.data.loop_triangles),'rebuilt',len(rebuilt))
