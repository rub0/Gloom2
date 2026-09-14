'''Join hood shell/lip/lining, refine the neck, and trim only the hidden medial ends of the collar bases.'''
import bpy
import bmesh
import json
import math
from mathutils import Vector

if bpy.data.scenes.get('Hound_Mesh_v07'):
    raise RuntimeError('v07 exists; preserve edits and choose a new version')
source = bpy.data.scenes['Hound_Mesh_v06']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original, original_rig = source.objects['H06_DeformMesh'], source.objects['Hound06_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v07')
for key in source.keys():
    scene[key] = source[key]
scene['gloom_authoring_stage'] = 'v07-hood-neck-collar-pass-not-final'
scene['gloom_limitations'] = 'Preserved hood opening; rest-fit collar correction only, provisional skinning, no cloth simulation or final rig'
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1
scene.render.fps = 30
scene.frame_start, scene.frame_end = 1, 181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name, frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v07_EXPORT')
scene.collection.children.link(collection)
rig = original_rig.copy()
rig.data = original_rig.data.copy()
rig.name, rig.data.name = 'Hound07_Rig', 'Hound07_Skeleton'
rig.animation_data.action = original_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound07_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name, model.data.name = 'H07_DeformMesh', 'H07_DeformTopology'
model.parent = rig
for modifier in model.modifiers:
    if modifier.type == 'ARMATURE':
        modifier.object = rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound06_', 'Hound07_')
materials = {name: next(m for m in model.data.materials if name in m.name) for name in ('Hood', 'AshSkin')}
bpy.context.window.scene = scene
replaced = {'Hood_shell', 'Hood_opening_rim', 'Neck'}
groups = {model.vertex_groups['PART_'+name].index for name in replaced}
removed = {v.index for v in model.data.vertices if any(g.group in groups for g in v.groups)}
bm = bmesh.new()
bm.from_mesh(model.data)
bm.verts.ensure_lookup_table()
bmesh.ops.delete(bm, geom=[bm.verts[i] for i in removed], context='VERTS')
bm.to_mesh(model.data)
bm.free()
for name in replaced:
    model.vertex_groups.remove(model.vertex_groups['PART_'+name])


def smooth(value, low, high):
    t = max(0, min(1, (value-low)/(high-low)))
    return t*t*(3-2*t)


def surface_weights(name, z):
    if name == 'Hood_continuous':
        amount = smooth(z, 1.54, 1.70)
        return {'Bip001 Spine2': 1-amount, 'Bip001 Head': amount}
    if z < 1.505:
        amount = smooth(z, 1.435, 1.505)
        return {'Bip001 Spine2': 1-amount, 'Bip001 Neck': amount}
    amount = smooth(z, 1.505, 1.625)
    return {'Bip001 Neck': 1-amount, 'Bip001 Head': amount}


def bridge(faces, first, second):
    for i in range(len(first)):
        faces.append((first[i], second[i], second[(i+1)%len(second)], first[(i+1)%len(first)]))


generated = []


def add_surface(name, vertices, faces, material):
    data = bpy.data.meshes.new('H07_'+name)
    data.from_pydata(vertices, [], faces)
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    assert all(e.is_manifold for e in bm.edges), 'Non-manifold surface: '+name
    bm.to_mesh(data)
    bm.free()
    data.materials.append(materials[material])
    for polygon in data.polygons:
        polygon.use_smooth = True
    obj = bpy.data.objects.new(data.name, data)
    collection.objects.link(obj)
    for index, point in enumerate(vertices):
        for bone, weight in surface_weights(name, point.z).items():
            if weight > 1e-6:
                group = obj.vertex_groups.get(bone) or obj.vertex_groups.new(name=bone)
                group.add([index], weight, 'REPLACE')
    obj.vertex_groups.new(name='PART_'+name).add(list(range(len(vertices))), 1, 'REPLACE')
    obj.parent = rig
    modifier = obj.modifiers.new('Hound07 diagnostic skinning', 'ARMATURE')
    modifier.object = rig
    modifier.use_deform_preserve_volume = False
    generated.append(obj)
    return obj


# The approved front silhouettes remain exact polylines, subdivided into four edges per original segment.
outline = [(0,1.80),(.066,1.765),(.127,1.671),(.14,1.57),(.128,1.476),(.071,1.427),
           (0,1.474),(-.071,1.427),(-.128,1.476),(-.14,1.57),(-.127,1.671),(-.066,1.765)]
opening = [(0,1.722),(.041,1.695),(.065,1.653),(.075,1.579),(.085,1.488),(.050,1.446),
           (0,1.49),(-.050,1.446),(-.085,1.488),(-.075,1.579),(-.065,1.653),(-.041,1.695)]
outer = [Vector((x, -.170, z)).lerp(Vector((outline[(i+1)%12][0], -.170, outline[(i+1)%12][1])), step/4)
         for i, (x, z) in enumerate(outline) for step in range(4)]
inner = [Vector((x, -.194, z)).lerp(Vector((opening[(i+1)%12][0], -.194, opening[(i+1)%12][1])), step/4)
         for i, (x, z) in enumerate(opening) for step in range(4)]
soft_outline = [v.copy() for v in outer]
for _ in range(2):
    soft_outline = [v*.6+(soft_outline[(i-1)%48]+soft_outline[(i+1)%48])*.2 for i, v in enumerate(soft_outline)]


def hood_profile(index, y):
    profile = outer[index].lerp(soft_outline[index], smooth(y, -.170, -.04))
    # Tuck the lower side behind the existing inner blade, leaving the approved front edge untouched.
    profile.x *= 1-.10*math.exp(-((profile.z-1.515)/.050)**2)*smooth(y, -.170, -.100)
    return profile


vertices, faces = [], []
previous = None
for t in (0, .30, .68, 1):
    ring = list(range(len(vertices), len(vertices)+48))
    vertices.extend(a.lerp(b, t) for a, b in zip(inner, outer))
    if previous is not None:
        bridge(faces, previous, ring)
    previous = ring
stations = [(-.170, 1, 1), (-.145, 1, 1), (-.100, .99, .99), (-.040, .98, .98), (.020, .956, .959),
            (.073, .93, .94), (.103, .80, .84), (.123, .65, .70), (.130, .43, .46), (.134, .16, .18)]
for y, sx, sz in stations[1:]:
    ring = list(range(len(vertices), len(vertices)+48))
    for i, point in enumerate(outer):
        profile = hood_profile(i, y)
        x, z = profile.x*sx, 1.622+(profile.z-1.622)*sz
        fold = .002*math.sin(5*math.pi*(z-1.427)/.373+3*y)*smooth(abs(x), .075, .125)*smooth(y, -.145, -.03)
        fold *= max(0, min(1, (.14-abs(x))/.03))
        vertices.append(Vector((x+(1 if x >= 0 else -1)*fold, y, z)))
    bridge(faces, previous, ring)
    previous = ring
cap = len(vertices)
vertices.append(Vector((0, .136, 1.622)))
faces.extend((previous[i], previous[(i+1)%48], cap) for i in range(48))
# Internal lining shares the opening ring; it is not a second overlapping lip object.
previous = list(range(48))
for station, (y, sx, sz) in enumerate(stations):
    ring = list(range(len(vertices), len(vertices)+48))
    for i, point in enumerate(outer):
        profile = hood_profile(i, y)
        vertices.append(Vector((profile.x*sx*.96, -.163 if station == 0 else y-.005, 1.622+(profile.z-1.622)*sz*.96)))
    bridge(faces, previous, ring)
    previous = ring
cap = len(vertices)
vertices.append(Vector((0, .130, 1.622)))
faces.extend((previous[i], previous[(i+1)%48], cap) for i in range(48))
hood = add_surface('Hood_continuous', vertices, faces, 'Hood')
hood.vertex_groups.new(name='LANDMARK_Hood_opening').add(list(range(48)), 1, 'REPLACE')
hood.vertex_groups.new(name='LANDMARK_Hood_outer_front').add(list(range(144, 192)), 1, 'REPLACE')

vertices, faces = [], []
neck_profile = [(1.435,.078,.061), (1.447,.079,.062), (1.461,.074,.061), (1.478,.067,.057), (1.494,.061,.053),
                (1.510,.057,.049), (1.526,.058,.050), (1.542,.061,.053), (1.554,.063,.056), (1.565,.064,.058)]
for z, rx, ry in neck_profile:
    for i in range(32):
        angle = math.tau*i/32
        x, y = rx*math.sin(angle), .012-ry*math.cos(angle)
        tendon = .0025*math.exp(-((abs(x)-(.038-.10*(z-1.47)))/.010)**2)*math.sin(math.pi*(z-1.435)/.130)**2
        y -= tendon*max(0, math.cos(angle))
        vertices.append(Vector((x, y, z)))
faces.append(tuple(range(32)))
for r in range(len(neck_profile)-1):
    bridge(faces, list(range(r*32,(r+1)*32)), list(range((r+1)*32,(r+2)*32)))
faces.append(tuple(range((len(neck_profile)-1)*32,len(neck_profile)*32)))
add_surface('Neck_surface', vertices, faces, 'AshSkin')

# Preserve every exterior collar endpoint; compress only the medial portion previously hidden inside the hood.
for side, suffix in ((1, 'L'), (-1, 'R')):
    index = model.vertex_groups['PART_Collar_plate_'+suffix].index
    for vertex in model.data.vertices:
        if any(g.group == index for g in vertex.groups) and abs(vertex.co.x) < .185:
            vertex.co.x = side*(.146+(abs(vertex.co.x)-.073)*(.185-.146)/(.185-.073))
model.data.update()
bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
for obj in generated:
    obj.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.object.join()
scene['gloom_soft_parts'] = json.dumps([part for part in json.loads(source['gloom_soft_parts']) if part not in replaced]
                                     +['Hood_continuous', 'Neck_surface'])
scene['gloom_revision'] = 'Exact front hood opening, continuous shell/lip/lining, shaped neck, medial collar-base trim; unchanged 53-bone diagnostic rig'
scene.frame_set(1)
bpy.context.view_layer.update()
model.data.calc_loop_triangles()
print('HOUND_V07_MESH_READY', len(model.data.vertices), 'vertices', len(model.data.loop_triangles), 'triangles')
