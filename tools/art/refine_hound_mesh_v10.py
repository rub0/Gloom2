'''H02: hollow greaves, fitted knee/ankle armor and boot soles. Preserve the v09 source and diagnostic rig.'''
import bpy
import bmesh
import json
import math
from pathlib import Path
from mathutils import Vector

root = Path(__file__).resolve().parents[2]
assert not (root/'art/characters/hound/v10/hound-mesh-v10.blend').exists(), 'Preserve existing v10 edits'
source = bpy.data.scenes['Hound_Mesh_v09']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original, original_rig = source.objects['H09_DeformMesh'], source.objects['Hound09_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v10')
for key in source.keys():
    scene[key] = source[key]
scene['gloom_authoring_stage'] = 'H02-greaves-boots-not-final-sculpt'
scene['gloom_limitations'] = 'Diagnostic leg poses only; global art approval H05 and production locomotion H11 pending'
scene.unit_settings.system, scene.unit_settings.scale_length = 'METRIC', 1
scene.render.fps, scene.frame_start, scene.frame_end = 30, 1, 181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name, frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v10_EXPORT')
scene.collection.children.link(collection)
rig = original_rig.copy()
rig.data = original_rig.data.copy()
rig.name, rig.data.name = 'Hound10_Rig', 'Hound10_Skeleton'
rig.animation_data.action = original_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound10_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name, model.data.name = 'H10_DeformMesh', 'H10_DeformTopology'
model.parent, model.modifiers[0].object = rig, rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound09_', 'Hound10_')
bpy.context.window.scene = scene
parts = {g.name[5:]: [] for g in model.vertex_groups if g.name.startswith('PART_')}
for vertex in model.data.vertices:
    for weight in vertex.groups:
        name = model.vertex_groups[weight.group].name
        if name.startswith('PART_'):
            parts[name[5:]].append(vertex.index)
rigid = json.loads(scene['gloom_rigid_parts'])
rebuilt = {name for name in parts if name.startswith(('Greave_mass_','Boot_','Knee_shield_','Greave_front_','Shin_lower_','Ankle_guard_'))}
morphed = {'Trousers_continuous'}
materials = {name: next(i for i,m in enumerate(model.data.materials) if name in m.name) for name in ('Iron','EdgePlanes','Undersuit')}
generated = []


def surface(name, vertices, faces, accents):
    data = bpy.data.meshes.new('H10_'+name)
    data.from_pydata(vertices, [], faces)
    for material in model.data.materials:
        data.materials.append(material)
    for polygon, material in zip(data.polygons, accents):
        polygon.material_index = materials[material]
        polygon.use_smooth = True
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    assert all(e.is_manifold for e in bm.edges), name
    bm.to_mesh(data)
    bm.free()
    data.set_sharp_from_angle(angle=math.radians(40))
    obj = bpy.data.objects.new(data.name, data)
    collection.objects.link(obj)
    obj.vertex_groups.new(name='PART_'+name).add(list(range(len(vertices))), 1, 'REPLACE')
    obj.vertex_groups.new(name=rigid[name]).add(list(range(len(vertices))), 1, 'REPLACE')
    generated.append(obj)


def bridge(faces, accents, a, b, n, material):
    for i in range(n):
        faces.append((a+i,a+(i+1)%n,b+(i+1)%n,b+i))
        accents.append(material)


def smooth(t):
    t = max(0,min(1,t))
    return t*t*(3-2*t)


for side,suffix in ((1,'L'),(-1,'R')):
    # Keep the original calf width and profile. Raised front/rear hems leave the ankle hinge clear.
    rings = []
    greave_profile = [(0,.197,.016,.064,.075),(.24,.192,.016,.074,.091),(.75,.181,.007,.086,.095),(1,.177,0,.084,.083)]
    for t in (0,.014,.055,.24,.50,.75,.95,.987,1):
        for a,b in zip(greave_profile,greave_profile[1:]):
            if t<=b[0]:
                f = (t-a[0])/(b[0]-a[0])
                cx,cy,rx,ry = [a[j]+(b[j]-a[j])*f for j in range(1,5)]
                break
        ring = []
        for i in range(32):
            angle = math.tau*i/32
            # The upper rim falls away behind the knee; broad front plates retain the approved crest.
            low = .184+.042*max(0,-math.sin(angle))**2
            high = .497-.043*max(0,math.sin(angle))**2
            z = low+(high-low)*t
            lip = .001*math.sin(math.pi*min(t,1-t)/.028) if min(t,1-t)<.028 else 0
            # A narrow integral tongue supports the knee cup throughout calf rotation.
            tongue = max(0,-math.sin(angle))**6*smooth((t-.95)/.05)
            ring.append(Vector((side*(cx+(rx+lip)*math.cos(angle)),cy+(ry+lip)*math.sin(angle)-.022*tongue,z+.080*tongue)))
        rings.append(ring)
    vertices,faces,accents = [],[],[]
    for inner in (False,True):
        for ring in rings:
            center = Vector((sum(p.x for p in ring)/32,sum(p.y for p in ring)/32,0))
            for p in ring:
                radial = Vector((p.x-center.x,p.y-center.y,0)).normalized()
                vertices.append(p-radial*.006 if inner else p)
    count = len(rings)*32
    for layer in (0,1):
        for row in range(len(rings)-1):
            bridge(faces,accents,layer*count+row*32,layer*count+(row+1)*32,32,'Iron')
    for row in (0,len(rings)-1):
        bridge(faces,accents,row*32,count+row*32,32,'EdgePlanes')
    surface('Greave_mass_'+suffix,vertices,faces,accents)

    # A connected boot: beveled outsole, broad forefoot, fitted heel and an actual collar cavity.
    vertices,faces,accents = [],[],[]
    profile = [(0,.201,-.060,.089,.143),(.007,.201,-.060,.094,.147),(.023,.201,-.060,.095,.148),
               (.027,.201,-.060,.091,.144),(.035,.201,-.059,.091,.144),(.060,.201,-.055,.090,.140),
               (.086,.201,-.041,.084,.123),(.112,.199,-.015,.068,.095),
               (.148,.197,.008,.064,.078),(.164,.197,.011,.062,.073),(.168,.197,.011,.060,.071)]
    for row,(z,cx,cy,rx,ry) in enumerate(profile):
        for i in range(32):
            angle = math.tau*i/32
            x,y = math.cos(angle),math.sin(angle)
            exponent = .76 if z<.09 else 1
            x,y = math.copysign(abs(x)**exponent,x),math.copysign(abs(y)**exponent,y)
            vertices.append((side*(cx+rx*x),cy+ry*y,z+(.003*max(0,-y)**8 if row==0 else 0)))
        if row:
            bridge(faces,accents,(row-1)*32,row*32,32,'Undersuit' if row<=4 else 'Iron')
    # Bottom fan avoids triangulation ambiguity at the small toe spring.
    vertices.append((side*.201,-.060,0))
    center = len(vertices)-1
    for i in range(32):
        faces.append((i,(i+1)%32,center))
        accents.append('Undersuit')
    previous = (len(profile)-1)*32
    for z,rx,ry in ((.168,.053,.064),(.159,.053,.064),(.100,.050,.060)):
        start = len(vertices)
        for i in range(32):
            angle = math.tau*i/32
            vertices.append((side*(.197+rx*math.cos(angle)),.011+ry*math.sin(angle),z))
        bridge(faces,accents,previous,start,32,'Undersuit')
        previous = start
    faces.append(tuple(range(previous,previous+32)))
    accents.append('Undersuit')
    surface('Boot_'+suffix,vertices,faces,accents)

    # Rebuild armor as fitted 4 mm shells over the actual calf/boot profiles.
    def front_surface(point,base):
        if base in ('Greave_mass','Greave_front'):
            t = max(0,min(1,(point.z-.21)/.287))
            for iteration in range(5):
                for a,b in zip(greave_profile,greave_profile[1:]):
                    if t<=b[0]:
                        f = (t-a[0])/(b[0]-a[0])
                        cx,cy,rx,ry = [a[j]+(b[j]-a[j])*f for j in range(1,5)]
                        break
                cosine = max(-.97,min(.97,(abs(point.x)-cx)/rx))
                sine = -math.sqrt(1-cosine*cosine)
                low = .184+.042*sine*sine
                t = max(0,min(1,(point.z-low)/(.497-low)))
            return cy+ry*sine-(.012 if base=='Greave_front' else 0)
        z = max(profile[0][0],min(profile[-1][0],point.z))
        for a,b in zip(profile,profile[1:]):
            if z<=b[0]:
                f = (z-a[0])/(b[0]-a[0])
                cx,cy,rx,ry = [a[j]+(b[j]-a[j])*f for j in range(1,5)]
                exponent = (.76 if a[0]<.09 else 1)*(1-f)+(.76 if b[0]<.09 else 1)*f
                break
        u = max(-.92,min(.92,(abs(point.x)-cx)/rx))
        return cy-ry*(1-abs(u)**(2/exponent))**(exponent/2)

    for kind,base in (('Knee_shield',None),('Greave_front','Greave_mass'),('Shin_lower','Greave_front'),
                      ('Boot_toe','Boot'),('Ankle_guard','Boot')):
        name = kind+'_'+suffix
        reference = bpy.data.scenes['Hound_Blockout_v02'].objects['H02_'+name]
        count = (len(reference.data.vertices)-1)//2
        outline = [reference.matrix_basis@v.co for v in reference.data.vertices[:count]]
        for point in outline:
            if kind=='Knee_shield':
                point.z += .020*(1-smooth((point.z-.493)/.07))
            elif kind=='Greave_front':
                point.z += .066*(1-smooth((point.z-.151)/.19))
            elif kind=='Shin_lower':
                point.z += .052*(1-smooth((point.z-.170)/.145))
            elif kind=='Ankle_guard':
                point.z = .103+(point.z-.105)*.52
        center = sum(outline,Vector())/count
        border = [point.lerp(outline[(i+1)%count],j/5) for i,point in enumerate(outline) for j in range(5)]
        vertices,faces,accents = [],[],[]
        radii = (1,.982,.956,.93,.87,.65,.35,.12)
        for r in radii:
            for p in border:
                point = center+(p-center)*r
                if base:
                    point.y = front_surface(point,base)-.008-(.0025 if .92<r<.97 else .0106*(1-r*r))
                else:
                    point.y = -.115+.034*((abs(point.x)-.177)/.079)**2+.015*((point.z-.56)/.069)**2-.005*(1-r*r)
                vertices.append(point)
        point = center.copy()
        if base:
            point.y = front_surface(point,base)-.0186
        else:
            point.y = -.120
        vertices.append(point)
        n = len(border)
        for row in range(len(radii)-1):
            bridge(faces,accents,row*n,(row+1)*n,n,'EdgePlanes' if row==1 else 'Iron')
        for i in range(n):
            faces.append(((len(radii)-1)*n+i,(len(radii)-1)*n+(i+1)%n,len(vertices)-1))
            accents.append('Iron')
        front_faces = list(faces)
        front_materials = list(accents)
        layer = len(vertices)
        vertices.extend(p+Vector((0,.004,0)) for p in list(vertices))
        faces.extend(tuple(i+layer for i in face) for face in front_faces)
        accents.extend(front_materials)
        bridge(faces,accents,0,layer,n,'Iron')
        surface(name,vertices,faces,accents)

# Fit only the lower trouser/undersuit branch inside the 6 mm greave and the boot collar.
# Existing topology and all weights stay intact, including the deformable knee transition.
for index in parts['Trousers_continuous']:
    p = model.data.vertices[index].co
    if p.z>=.61:
        continue
    side = 1 if p.x>0 else -1
    t = max(0,min(1,(.563-p.z)/.417))
    cx,cy = side*(.173+.024*t),-.005+.017*t
    factor = 1-.16*smooth((.61-p.z)/.10)-.16*(1-smooth((p.z-.19)/.12))
    p.x = cx+(p.x-cx)*factor
    p.y = cy+(p.y-cy)*factor

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
scene['gloom_v10_rebuilt_parts'] = json.dumps(sorted(rebuilt))
scene['gloom_v10_morphed_parts'] = json.dumps(sorted(morphed))
scene['gloom_revision'] = 'H02: hollow 6mm greaves, relieved knee and ankle guards, connected beveled boots with sole and collar cavity'
scene.frame_set(1)
bpy.context.view_layer.update()
model.data.calc_loop_triangles()
draft = root/'.cache/hound-mesh-v10'
draft.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(draft/'hound-draft.blend'),check_existing=False,compress=True)
print('HOUND_V10',len(model.data.vertices),len(model.data.loop_triangles),'rebuilt',len(rebuilt),'morphed',len(morphed))
