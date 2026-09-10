'''Hound v02: clone v01, free deltoids, deepen hood and rebuild anatomically oriented gauntlets.'''
import bpy
import bmesh
import math
from mathutils import Vector

if bpy.data.scenes.get('Hound_Blockout_v02'):
    raise RuntimeError('v02 exists; do not overwrite an artist revision')
source = bpy.data.scenes['Hound_Blockout_v01']
source_model = bpy.data.collections['HOUND_v01_MODEL_ONLY']
scene = bpy.data.scenes.new('Hound_Blockout_v02')
scene['gloom_blockout_version'] = 'v02'
scene['gloom_authoring_stage'] = 'blockout-v02-unapproved'
scene['gloom_reference'] = 'docs/art/hound/hound-concept-v01.png'
scene['gloom_forward'] = '-Y Blender / +Z glTF'
scene['gloom_revision'] = 'Exposed deltoids; clavicle-mounted blades; deeper hood; palms inward and full pointed hand-back armor'
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1
bpy.context.window.scene = scene
model = bpy.data.collections.new('HOUND_v02_MODEL_ONLY')
scene.collection.children.link(model)
materials = {}
for original in source_model.objects:
    obj = original.copy()
    obj.data = original.data.copy()
    obj.name = original.name.replace('H01_','H02_',1)
    obj.data.name = obj.name
    model.objects.link(obj)
    for slot in obj.material_slots:
        name = slot.material.name
        if name not in materials:
            materials[name] = slot.material.copy()
            materials[name].name = name.replace('Hound01_','Hound02_',1)
        slot.material = materials[name]
iron = materials['Hound01_Iron']
edge = materials['Hound01_EdgePlanes']
cloth = materials['Hound01_Hood']
skin = materials['Hound01_AshSkin']
under = materials['Hound01_Undersuit']

def mesh(name, vertices, faces, material, bevel=0):
    data = bpy.data.meshes.new('H02_'+name)
    data.from_pydata(vertices,[],faces)
    data.update()
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(data)
    bm.free()
    obj = bpy.data.objects.new('H02_'+name,data)
    model.objects.link(obj)
    data.materials.append(material)
    if bevel:
        mod = obj.modifiers.new('Editable edge chamfer','BEVEL')
        mod.width = bevel
        mod.segments = 1
    return obj

def loft(name, rings, material, sides=12):
    vertices = []
    for x,y,z,rx,ry in rings:
        for i in range(sides):
            angle = math.tau*i/sides
            vertices.append((x+rx*math.cos(angle),y+ry*math.sin(angle),z))
    faces = [tuple(reversed(range(sides)))]
    for r in range(len(rings)-1):
        faces.extend((r*sides+i,r*sides+(i+1)%sides,(r+1)*sides+(i+1)%sides,(r+1)*sides+i) for i in range(sides))
    faces.append(tuple((len(rings)-1)*sides+i for i in range(sides)))
    return mesh(name,vertices,faces,material)

def plate(name, side, polygon, front_y, thickness, material=iron):
    n = len(polygon)
    vertices = [(side*x,front_y,z) for x,z in polygon]+[(side*x,front_y+thickness,z) for x,z in polygon]
    vertices.append((side*sum(p[0] for p in polygon)/n,front_y-.014,sum(p[1] for p in polygon)/n))
    faces = [(i,(i+1)%n,2*n) for i in range(n)]+[tuple(range(n,2*n))]
    faces += [(i,n+i,n+(i+1)%n,(i+1)%n) for i in range(n)]
    return mesh(name,vertices,faces,material,.003)

def remove(name):
    obj = model.objects.get('H02_'+name)
    if obj is None:
        raise RuntimeError('Missing revision target: '+name)
    bpy.data.objects.remove(obj,do_unlink=True)

# Replace shoulder cups with high collar armor. Skin now owns the shoulder silhouette.
for s,suffix in [(1,'L'),(-1,'R')]:
    for part in ['Pauldron','Pauldron_front','Clavicle','Shoulder_blade_inner','Shoulder_blade_middle','Shoulder_blade_outer']:
        remove(part+'_'+suffix)
    deltoid = loft('Deltoid_'+suffix,[(s*.303,.008,1.36,.055,.068),(s*.286,.007,1.405,.084,.093),
                                    (s*.27,.007,1.469,.086,.091),(s*.244,.008,1.523,.040,.053)],skin)
    for polygon in deltoid.data.polygons:
        polygon.use_smooth = True
    plate('Collar_plate_'+suffix,s,[(.073,1.481),(.103,1.535),(.177,1.565),(.229,1.552),
                                  (.273,1.507),(.242,1.478),(.162,1.463)],-.127,.168,edge)
    plate('Collar_blade_inner_'+suffix,s,[(.127,1.505),(.18,1.519),(.256,1.763),(.182,1.652)],-.050,.035)
    plate('Collar_blade_middle_'+suffix,s,[(.17,1.492),(.229,1.526),(.367,1.696),(.243,1.626)],-.014,.037)
    plate('Collar_blade_outer_'+suffix,s,[(.208,1.515),(.26,1.51),(.485,1.625),(.285,1.576)],.037,.032)

# Retain hood crown/scale, advance the fabric and lower its inner opening over the forehead.
hood = model.objects['H02_Hood_shell']
for vertex in list(hood.data.vertices)[:12]:
    vertex.co.y = -.17
hood.data.update()
remove('Hood_opening_rim')
outline = [(0,1.80),(.066,1.765),(.127,1.671),(.14,1.57),(.128,1.476),(.071,1.427),
           (0,1.474),(-.071,1.427),(-.128,1.476),(-.14,1.57),(-.127,1.671),(-.066,1.765)]
opening = [(0,1.722),(.041,1.695),(.065,1.653),(.075,1.579),(.085,1.488),(.050,1.446),
           (0,1.49),(-.050,1.446),(-.085,1.488),(-.075,1.579),(-.065,1.653),(-.041,1.695)]
rim = mesh('Hood_opening_rim',[(x,-.17,z) for x,z in outline]+[(x,-.194,z) for x,z in opening],
           [(i,(i+1)%12,12+(i+1)%12,12+i) for i in range(12)],cloth)
rim.modifiers.new('Cloth lip thickness','SOLIDIFY').thickness = .006

# Hands use a local anatomical frame: thumb/front, fingers/down, back/outside.
# Flexion always moves away from the back plate and toward the palm/thigh.
for s,suffix in [(1,'L'),(-1,'R')]:
    remove('Palm_'+suffix)
    remove('Hand_back_'+suffix)
    for i in range(4):
        for segment_name in ['A','B']:
            remove('Finger_'+suffix+'_'+str(i)+segment_name)
    for segment_name in ['A','B']:
        remove('Thumb_'+suffix+segment_name)
    wrist = Vector((s*.454,-.036,.961))
    down = Vector((s*.16,-.08,-1)).normalized()
    front = Vector((0,-1,0))
    front = (front-down*front.dot(down)).normalized()
    dorsal = front.cross(down).normalized()*s

    def point(u,v,w):
        return wrist+front*u+down*v+dorsal*w

    def hand_loft(name,rings,material):
        vertices = []
        for v,ru,rw in rings:
            for i in range(12):
                angle = math.tau*i/12
                vertices.append(point(ru*math.cos(angle),v,rw*math.sin(angle)))
        faces = [tuple(reversed(range(12)))]
        for r in range(len(rings)-1):
            faces.extend((r*12+i,r*12+(i+1)%12,(r+1)*12+(i+1)%12,(r+1)*12+i) for i in range(12))
        faces.append(tuple((len(rings)-1)*12+i for i in range(12)))
        return mesh(name,vertices,faces,material)

    def phalanx(name,a,b,radius):
        a,b = point(*a),point(*b)
        axis = b-a
        rotation = axis.to_track_quat('Z','Y')
        vertices = []
        for t,r in [(0,radius),(.35,radius),(1,radius*.76)]:
            for i in range(8):
                angle = math.tau*i/8
                vertices.append(a+rotation @ Vector((r*math.cos(angle),r*math.sin(angle),axis.length*t)))
        faces = [tuple(reversed(range(8)))]
        for ring in range(2):
            faces.extend((ring*8+i,ring*8+(i+1)%8,(ring+1)*8+(i+1)%8,(ring+1)*8+i) for i in range(8))
        faces.append(tuple(range(16,24)))
        return mesh(name,vertices,faces,iron)

    hand_loft('Palm_'+suffix,[(0,.033,.025),(.03,.047,.027),(.076,.054,.028),(.105,.048,.025)],under)
    hand_loft('Wrist_transition_'+suffix,[(-.018,.038,.03),(.012,.04,.032),(.034,.043,.029)],iron)
    profile = [(-.036,-.018),(.036,-.018),(.054,.052),(.058,.103),(.034,.13),
               (0,.174),(-.038,.132),(-.055,.10),(-.052,.052)]
    count = len(profile)
    vertices = [point(u,v,.035) for u,v in profile]+[point(u,v,.021) for u,v in profile]
    vertices.append(point(0,.073,.058))
    faces = [(i,(i+1)%count,count*2) for i in range(count)]+[tuple(range(count,count*2))]
    faces += [(i,count+i,count+(i+1)%count,(i+1)%count) for i in range(count)]
    cover = mesh('Hand_back_full_point_'+suffix,vertices,faces,edge,.002)
    cover['gloom_hand_frame'] = 'Dorsum lateral; palm medial; thumb anterior; distal pointed plate'
    for digit,(u,length,radius) in enumerate([(.034,.078,.012),(.011,.09,.012),(-.013,.084,.011),(-.035,.065,.01)]):
        chain = [(u,.103,-.001),(u,.133,-.012),(u,.103+length*.73,-.034),(u,.103+length,-.057)]
        for j in range(3):
            phalanx('Finger_'+suffix+'_'+str(digit)+chr(65+j),chain[j],chain[j+1],radius*(1-j*.08))
    phalanx('Thumb_'+suffix+'A',(.044,.039,-.007),(.08,.073,-.018),.017)
    phalanx('Thumb_'+suffix+'B',(.08,.073,-.018),(.063,.105,-.041),.014)

bpy.context.view_layer.update()
bpy.ops.object.select_all(action='DESELECT')
for obj in model.objects:
    obj.select_set(True)
bpy.context.view_layer.objects.active = model.objects['H02_Torso_underlayer']
print('HOUND_V02_CREATED:',len(model.objects),'editable parts; v01 geometry and materials remain untouched')
