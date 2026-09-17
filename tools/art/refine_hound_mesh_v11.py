'''H03: a severe neutral human face and integrated arm anatomy; preserve all unrelated geometry and diagnostic keys.'''
import bpy
import bmesh
import json
import math
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree

root = Path(__file__).resolve().parents[2]
assert not (root/'art/characters/hound/v11/hound-mesh-v11.blend').exists(), 'Preserve existing v11 edits'
source = bpy.data.scenes['Hound_Mesh_v10']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original, original_rig = source.objects['H10_DeformMesh'], source.objects['Hound10_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v11')
for key in source.keys():
    scene[key] = source[key]
scene['gloom_authoring_stage'] = 'H03-face-visible-anatomy-not-final-sculpt'
scene['gloom_limitations'] = 'Neutral closed human face; no facial rig, jaw motion, production topology or global H05 approval'
scene.unit_settings.system, scene.unit_settings.scale_length = 'METRIC', 1
scene.render.fps, scene.frame_start, scene.frame_end = 30, 1, 181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name, frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v11_EXPORT')
scene.collection.children.link(collection)
rig = original_rig.copy()
rig.data = original_rig.data.copy()
rig.name, rig.data.name = 'Hound11_Rig', 'Hound11_Skeleton'
rig.animation_data.action = original_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound11_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name, model.data.name = 'H11_DeformMesh', 'H11_DeformTopology'
model.parent, model.modifiers[0].object = rig, rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound10_', 'Hound11_')
bpy.context.window.scene = scene
parts = {g.name[5:]: [] for g in model.vertex_groups if g.name.startswith('PART_')}
for vertex in model.data.vertices:
    for weight in vertex.groups:
        name = model.vertex_groups[weight.group].name
        if name.startswith('PART_'):
            parts[name[5:]].append(vertex.index)
rebuilt = {'Head_surface', 'Eye_L', 'Eye_R', 'Mouth_shadow'}
morphed = {'Arm_surface_L', 'Arm_surface_R'}
rigid = json.loads(scene['gloom_rigid_parts'])
generated = []


def bell(value, center, width):
    return math.exp(-((value-center)/width)**2)


def smooth(value, low, high):
    t = max(0, min(1, (value-low)/(high-low)))
    return t*t*(3-2*t)


def surface(name, vertices, faces, material):
    data = bpy.data.meshes.new('H11_'+name)
    data.from_pydata(vertices, [], faces)
    for mat in model.data.materials:
        data.materials.append(mat)
    index = next(i for i,m in enumerate(data.materials) if material in m.name)
    for polygon in data.polygons:
        polygon.material_index = index
        polygon.use_smooth = len(polygon.vertices)<=4
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    assert all(e.is_manifold for e in bm.edges), name
    if name=='Head_surface':
        bmesh.ops.triangulate(bm,faces=list(bm.faces))
    bm.to_mesh(data)
    bm.free()
    obj = bpy.data.objects.new(data.name, data)
    collection.objects.link(obj)
    obj.vertex_groups.new(name='PART_'+name).add(list(range(len(vertices))), 1, 'REPLACE')
    obj.vertex_groups.new(name=rigid[name]).add(list(range(len(vertices))), 1, 'REPLACE')
    generated.append(obj)
    return data


# Preserve the v06 cranial envelope and height. Concentrate authoring samples on the visible face.
# ponytail: neutral ring topology only; H06/H08 must decide facial/jaw requirements before production retopology.
profile = [(1.505,.014,.028,-.008),(1.514,.038,.040,-.014),(1.533,.060,.055,-.019),
           (1.561,.067,.061,-.020),(1.587,.071,.069,-.017),(1.624,.079,.076,-.015),
           (1.654,.084,.079,-.013),(1.683,.083,.077,-.006),(1.706,.078,.067,.001),
           (1.730,.060,.052,.004),(1.740,.044,.042,.004)]
# Monotone Hermite interpolation avoids the horizontal shelves of per-segment smoothstep.
slopes = [[0.0]*4 for _ in profile]
for j in (1,2,3):
    for i in range(len(profile)):
        left = (profile[i][j]-profile[i-1][j])/(profile[i][0]-profile[i-1][0]) if i else 0
        right = (profile[i+1][j]-profile[i][j])/(profile[i+1][0]-profile[i][0]) if i+1<len(profile) else 0
        slopes[i][j] = 2*left*right/(left+right) if left*right>0 else 0
levels = [1.505+(1.740-1.505)*i/94 for i in range(95)]
sides = 96
vertices, faces = [], []
for z in levels:
    for row,(a,b) in enumerate(zip(profile, profile[1:])):
        if z<=b[0]+1e-8:
            t = (z-a[0])/(b[0]-a[0])
            width,depth,cy = [(2*t**3-3*t*t+1)*a[j]+(t**3-2*t*t+t)*(b[0]-a[0])*slopes[row][j]
                              +(-2*t**3+3*t*t)*b[j]+(t**3-t*t)*(b[0]-a[0])*slopes[row+1][j] for j in (1,2,3)]
            break
    for i in range(sides):
        angle = math.tau*i/sides
        x, y = width*math.sin(angle), cy-depth*math.cos(angle)
        front = max(0,math.cos(angle))
        if front>0:
            ax = abs(x)
            # A broad, flat chin and angular jaw; cheek ridge above a restrained buccal hollow.
            relief = .017*math.exp(-(x/.038)**4)*bell(z,1.535,.011)
            relief += .005*bell(ax,.048,.017)*bell(z,1.542,.014)
            relief += .018*bell(ax,.048,.018)*bell(z,1.619+.12*(ax-.04),.012)
            relief -= .009*bell(ax,.050,.018)*bell(z,1.587,.019)
            # Continuous nasal bridge, distinct alae, septum and small nostril depressions.
            relief += .025*bell(x,0,.010)*bell(z,1.642,.030)
            relief += .035*bell(x,0,.012)*bell(z,1.608,.011)
            relief += .009*bell(ax,.015,.0055)*bell(z,1.601,.005)
            relief -= .007*bell(ax,.011,.0038)*bell(z,1.597,.0028)
            relief += .006*bell(x,0,.0035)*bell(z,1.599,.004)
            # Orbital hollow, slanted brow and lids surround the small, fitted orange eye.
            eye_line = 1.648+.24*(ax-.021)
            relief -= .007*bell(ax,.036,.017)*bell(z,eye_line,.007)
            relief += .028*bell(ax,.037,.026)*bell(z,1.657+.26*ax,.0055)
            relief += .006*bell(ax,.036,.014)*bell(z,eye_line+.0032,.0024)
            relief += .005*bell(ax,.036,.014)*bell(z,eye_line-.0032,.0028)
            relief -= .004*bell(x,0,.004)*bell(z,1.686,.020)
            # Compressed lips with philtrum, shallow nasolabial transitions and a labiomental fold.
            mouth = 1.562-.003*(min(1,ax/.029)**2)
            relief += .013*math.exp(-(x/.027)**4)*bell(z,mouth+.004,.0031)
            relief += .016*math.exp(-(x/.025)**4)*bell(z,mouth-.004,.004)
            relief -= .003*bell(x,0,.004)*bell(z,1.579,.008)
            relief -= .004*bell(ax,.024+.28*(1.590-z),.005)*bell(z,1.582,.017)
            relief -= .003*bell(x,0,.026)*bell(z,1.548,.0035)
            y -= relief*front**1.4
        vertices.append(Vector((x,y,z)))
faces.append(tuple(range(sides)))
for row in range(len(levels)-1):
    for i in range(sides):
        faces.append((row*sides+i,row*sides+(i+1)%sides,(row+1)*sides+(i+1)%sides,(row+1)*sides+i))
faces.append(tuple(range((len(levels)-1)*sides,len(levels)*sides)))
head_data = surface('Head_surface',vertices,faces,'AshSkin')
head_data.calc_loop_triangles()
head_tree = BVHTree.FromPolygons(vertices,[list(t.vertices) for t in head_data.loop_triangles],all_triangles=True)


def fitted(x,z,offset):
    hit = head_tree.ray_cast(Vector((x,-.3,z)),Vector((0,1,0)),.4)[0]
    assert hit is not None, (x,z)
    return hit+Vector((0,-offset,0))


# Closed almond inlays, sampled along both axes. Back faces deliberately embedded; no free-standing wedges.
for side,suffix in ((1,'L'),(-1,'R')):
    vertices,faces = [],[]
    columns,rows = 24,6
    for back in (False,True):
        for i in range(columns+1):
            t = -1+2*i/columns
            x = side*(.036+.015*t)
            center = 1.648+.24*(abs(x)-.021)
            height = .00012+.0030*(1-t*t)
            for j in range(rows+1):
                v = -1+2*j/rows
                vertices.append(fitted(x,center+v*height,-.001 if back else .0011+.00025*(1-v*v)*(1-t*t)))
    layer = (columns+1)*(rows+1)
    for k in (0,layer):
        for i in range(columns):
            for j in range(rows):
                a = k+i*(rows+1)+j
                faces.append((a,a+rows+1,a+rows+2,a+1))
    border = list(range(rows+1))
    border += [i*(rows+1)+rows for i in range(1,columns+1)]
    border += [columns*(rows+1)+j for j in range(rows-1,-1,-1)]
    border += [i*(rows+1) for i in range(columns-1,0,-1)]
    for a,b in zip(border,border[1:]+border[:1]):
        faces.append((a,b,b+layer,a+layer))
    surface('Eye_'+suffix,vertices,faces,'EyeAccent')
vertices,faces = [],[]
for i in range(49):
    t = -1+2*i/48
    x,z = .028*t,1.562-.003*t*t
    h = .00012+.00035*(1-t*t)
    vertices.extend((fitted(x,z+h,.0008),fitted(x,z-h,.0008),fitted(x,z+h,-.001),fitted(x,z-h,-.001)))
for i in range(48):
    a,b = i*4,(i+1)*4
    faces.extend(((a,b,b+1,a+1),(a+2,a+3,b+3,b+2),(a,a+2,b+2,b),(a+1,b+1,b+3,a+3)))
faces.extend(((0,1,3,2),(192,194,195,193)))
surface('Mouth_shadow',vertices,faces,'Undersuit')

# Reshape existing arm vertices and keep every weight/topological connection. Ends under armor stay exact.
stations = [(1.523,.244,.008),(1.469,.270,.007),(1.405,.286,.007),(1.360,.307,.001),(1.209,.372,-.008),(.961,.454,-.036)]
for side,suffix in ((1,'L'),(-1,'R')):
    for index in parts['Arm_surface_'+suffix]:
        p = model.data.vertices[index].co
        for (a,ax,ay),(b,bx,by) in zip(stations,stations[1:]):
            if p.z>=b:
                t = (a-p.z)/(a-b)
                center = Vector((side*(ax+(bx-ax)*t),ay+(by-ay)*t,p.z))
                break
        direction = (p-center).normalized()
        front,back,outer = max(0,-direction.y),max(0,direction.y),max(0,side*direction.x)
        relief = .004*bell(p.z,1.450,.042)*outer**3
        relief -= .005*bell(p.z,1.395+.045*front,.018)*outer**2
        relief += .006*bell(p.z,1.327,.053)*front**4
        relief -= .0035*bell(p.z,1.270,.014)*front**3
        relief += .005*bell(p.z,1.335,.065)*back**3*(.35+.65*outer)
        relief -= .003*bell(p.z,1.302,.055)*max(0,1-abs(direction.y)*4)*outer**2
        relief += .003*bell(p.z,1.222,.028)*outer**3
        relief += .003*bell(p.z,1.155,.040)*front**2*outer
        envelope = smooth(p.z,1.085,1.125)*(1-smooth(p.z,1.495,1.523))
        p += direction*relief*envelope
model.data.update()
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
scene['gloom_v11_rebuilt_parts'] = json.dumps(sorted(rebuilt))
scene['gloom_v11_morphed_parts'] = json.dumps(sorted(morphed))
scene['gloom_revision'] = 'H03: integrated human orbital/cheek/nasal/lip/jaw forms, fitted eyes, deltoid/biceps/triceps/forearm relief'
scene.frame_set(1)
bpy.context.view_layer.update()
model.data.calc_loop_triangles()
draft = root/'.cache/hound-mesh-v11'
draft.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(draft/'hound-draft.blend'),check_existing=False,compress=True)
print('HOUND_V11',len(model.data.vertices),len(model.data.loop_triangles),'rebuilt',len(rebuilt),'morphed',len(morphed))
