'''Connect the trouser crotch, refine the sash and replace overlapping facial blocks with one surface. Rig remains diagnostic.'''
import bpy
import bmesh
import json
import math
from mathutils import Vector

if bpy.data.scenes.get('Hound_Mesh_v06'):
    raise RuntimeError('v06 exists; preserve edits and choose a new version')
source = bpy.data.scenes['Hound_Mesh_v05']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original, original_rig = source.objects['H05_DeformMesh'], source.objects['Hound05_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v06')
for key in source.keys():
    scene[key] = source[key]
scene['gloom_authoring_stage'] = 'v06-cloth-face-topology-pass-not-final'
scene['gloom_limitations'] = 'Provisional cloth/face anatomy; no facial rig, cloth simulation, final weights, weapon grips or production animations'
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1
scene.render.fps = 30
scene.frame_start, scene.frame_end = 1, 181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name, frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v06_EXPORT')
scene.collection.children.link(collection)
rig = original_rig.copy()
rig.data = original_rig.data.copy()
rig.name, rig.data.name = 'Hound06_Rig', 'Hound06_Skeleton'
rig.animation_data.action = original_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound06_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name, model.data.name = 'H06_DeformMesh', 'H06_DeformTopology'
model.parent = rig
for modifier in model.modifiers:
    if modifier.type == 'ARMATURE':
        modifier.object = rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound05_', 'Hound06_')
materials = {name: next(m for m in model.data.materials if name in m.name) for name in ('BurgundyCloth', 'Undersuit', 'AshSkin')}
bpy.context.window.scene = scene
face_parts = {'Head_planes', 'Jaw_plane', 'Nose_plane', 'Cheek_L', 'Cheek_R', 'Brow_L', 'Brow_R'}
replaced = face_parts | {'Pelvis_cloth', 'Waist_sash', 'Leg_continuous_L', 'Leg_continuous_R'}
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


def mix_weights(first, second, amount):
    return {name: first.get(name, 0)*(1-amount)+second.get(name, 0)*amount for name in set(first) | set(second)}


def torso_weights(z):
    return mix_weights({'Bip001 Pelvis': 1}, {'Bip001 Spine': 1}, smooth(z, 1, 1.12))


def bridge(faces, first, second):
    for i in range(len(first)):
        faces.append((first[i], second[i], second[(i+1)%len(second)], first[(i+1)%len(first)]))


generated = []


def add_surface(name, vertices, faces, weights, material_names, face_materials=None):
    data = bpy.data.meshes.new('H06_'+name)
    data.from_pydata(vertices, [], faces)
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    assert all(e.is_manifold for e in bm.edges), 'Non-manifold surface: '+name
    bm.to_mesh(data)
    bm.free()
    for material in material_names:
        data.materials.append(materials[material])
    for polygon in data.polygons:
        polygon.use_smooth = len(polygon.vertices) == 4
        polygon.material_index = face_materials[polygon.index] if face_materials else 0
    obj = bpy.data.objects.new(data.name, data)
    collection.objects.link(obj)
    for index, values in enumerate(weights):
        filtered = {bone: weight for bone, weight in values.items() if weight > 1e-6}
        total = sum(filtered.values())
        for bone, weight in filtered.items():
            group = obj.vertex_groups.get(bone) or obj.vertex_groups.new(name=bone)
            group.add([index], weight/total, 'REPLACE')
    obj.vertex_groups.new(name='PART_'+name).add(list(range(len(vertices))), 1, 'REPLACE')
    obj.parent = rig
    modifier = obj.modifiers.new('Hound06 diagnostic skinning', 'ARMATURE')
    modifier.object = rig
    modifier.use_deform_preserve_volume = False
    generated.append(obj)


# Two D-shaped hip loops share a descending crotch seam; their outer halves form the full waist loop.
vertices, faces, weights, face_materials = [], [], [], []
root_loops, seam = {}, {}
sides = 24
for side, suffix in ((1, 'L'), (-1, 'R')):
    loop = []
    for i in range(sides):
        angle = math.tau*i/sides
        if math.cos(angle) <= 1e-8 and i in seam:
            index = seam[i]
        else:
            index = len(vertices)
            vertices.append(Vector((side*.210*max(0, math.cos(angle)), .113*math.sin(angle), .965+.11*min(0, math.cos(angle)))))
            weights.append({'Bip001 Pelvis': 1})
            if math.cos(angle) <= 1e-8:
                seam[i] = index
        loop.append(index)
    root_loops[suffix] = loop
waist = [root_loops['L'][i] if math.cos(math.tau*i/sides) >= -1e-8 else root_loops['R'][(sides//2-i)%sides] for i in range(sides)]
for z, rx, ry in ((.990, .194, .114), (1.012, .186, .115), (1.031, .180, .115)):
    loop = []
    for i in range(sides):
        angle = math.tau*i/sides
        loop.append(len(vertices))
        vertices.append(Vector((rx*math.cos(angle), ry*math.sin(angle), z)))
        weights.append(torso_weights(z))
    bridge(faces, waist, loop)
    waist = loop
faces.append(tuple(waist))
face_materials.extend([0]*len(faces))


def sample_ring(ring, angle):
    center = sum(ring, Vector())/len(ring)
    dx, dy = math.cos(angle), math.sin(angle)
    for a, b in zip(ring, ring[1:]+ring[:1]):
        edge = b-a
        denominator = edge.x*dy-edge.y*dx
        if abs(denominator) < 1e-10:
            continue
        t = (dx*(a.y-center.y)-dy*(a.x-center.x))/denominator
        if not -.00001 <= t <= 1.00001:
            continue
        point = a.lerp(b, max(0, min(1, t)))
        if (point.x-center.x)*dx+(point.y-center.y)*dy > 0:
            return point
    raise RuntimeError('Missing source-leg ring intersection')


samples = [0, .06, .12, .18, .3, .42, .5, .65, .78, .84, .9, .95, 1, 1.05, 1.1, 1.16, 1.22, 1.35, 1.5, 1.65, 1.82, 1.92, 2]
for side, suffix in ((1, 'L'), (-1, 'R')):
    group_index = original.vertex_groups['PART_Leg_continuous_'+suffix].index
    old = [v for v in original.data.vertices if any(g.group == group_index for g in v.groups)]
    assert len(old) == len(samples)*12
    knee = [v.co for v in old[12*12:13*12]]
    previous = root_loops[suffix]
    for r, t in enumerate(samples[1:], 1):
        ring = [v.co for v in old[r*12:(r+1)*12]]
        reference_weights = {original.vertex_groups[g.group].name: g.weight for g in old[r*12].groups
                             if original.vertex_groups[g.group].name in rig.data.bones}
        loop = []
        for i in range(sides):
            angle = math.tau*i/sides
            # The right D loop has reversed angular orientation; retain that ordering all the way down.
            global_angle = angle if side == 1 else math.pi-angle
            point = sample_ring(ring, global_angle)
            if t < .65:
                branch_point = vertices[root_loops[suffix][i]].lerp(sample_ring(knee, global_angle), t)
                point = branch_point.lerp(point, smooth(t, 0, .65))
            if .18 < t < .95:
                fold = .0025*math.sin(math.pi*(t-.18)/.77)**2*math.sin(5*math.pi*t+1.3*math.sin(angle))
                point += Vector((side*math.cos(angle), math.sin(angle), 0))*fold
            loop.append(len(vertices))
            vertices.append(point)
            values = mix_weights({'Bip001 Pelvis': 1}, reference_weights, smooth(t, 0, .32)) if t < .32 else reference_weights
            weights.append(values)
        bridge(faces, previous, loop)
        face_materials.extend([1 if r >= 13 else 0]*sides)
        previous = loop
    faces.append(tuple(previous))
    face_materials.append(1)
add_surface('Trousers_continuous', vertices, faces, weights, ['BurgundyCloth', 'Undersuit'], face_materials)

# A separate thin cloth wrap, not solid caps penetrating the trouser volume. Subtle folds, no new ornamentation.
vertices, faces, weights = [], [], []
profile = [(0, .196, .121), (.69, .181, .126), (1, .175, .113)]
rings, sides = 13, 48
for inner in (False, True):
    for r in range(rings):
        t = r/(rings-1)
        for (low, ax, ay), (high, bx, by) in zip(profile, profile[1:]):
            if t <= high:
                blend = (t-low)/(high-low)
                rx, ry = ax+(bx-ax)*blend, ay+(by-ay)*blend
                break
        for i in range(sides):
            angle = math.tau*i/sides
            fold = .0018*math.sin(math.pi*t)**2*math.sin(6*math.pi*t+1.2*math.sin(angle))
            thickness = .003 if inner else 0
            point = Vector(((rx+fold-thickness)*math.cos(angle), (ry+fold-thickness)*math.sin(angle), .991+.081*t))
            vertices.append(point)
            weights.append(torso_weights(point.z))
        if r:
            base = (rings if inner else 0)*sides
            bridge(faces, list(range(base+(r-1)*sides, base+r*sides)), list(range(base+r*sides, base+(r+1)*sides)))
for r in (0, rings-1):
    bridge(faces, list(range(r*sides, (r+1)*sides)), list(range((rings+r)*sides, (rings+r+1)*sides)))
add_surface('Waist_wrap', vertices, faces, weights, ['BurgundyCloth'])


def gaussian(value, center, width):
    return math.exp(-((value-center)/width)**2)


# Continuous cranial/jaw/cheek/nose/brow surface. Closed neutral mouth and unchanged eye accents remain separate.
# ponytail: neutral-head rings only; replace eye/mouth edge flow before facial animation.
profile = [(1.505, .014, .028, -.008), (1.514, .038, .040, -.014), (1.533, .060, .055, -.019),
           (1.561, .067, .061, -.020), (1.587, .071, .069, -.017), (1.624, .079, .076, -.015),
           (1.654, .084, .079, -.013), (1.683, .083, .077, -.006), (1.706, .078, .067, .001),
           (1.730, .060, .052, .004), (1.740, .044, .042, .004)]
levels = [1.505, 1.509, 1.514, 1.521, 1.529, 1.538, 1.547, 1.554, 1.559, 1.563, 1.567, 1.573, 1.580,
          1.587, 1.593, 1.598, 1.603, 1.608, 1.614, 1.621, 1.628, 1.635, 1.641, 1.646, 1.651, 1.656,
          1.661, 1.666, 1.671, 1.678, 1.686, 1.696, 1.706, 1.716, 1.726, 1.735, 1.740]
vertices, faces, weights = [], [], []
sides = 64
for z in levels:
    for a, b in zip(profile, profile[1:]):
        if z <= b[0]:
            t = smooth(z, a[0], b[0])
            width, depth, center_y = [a[i]+(b[i]-a[i])*t for i in (1, 2, 3)]
            break
    for i in range(sides):
        angle = math.tau*i/sides
        x = width*math.sin(angle)
        y = center_y-depth*math.cos(angle)
        if math.cos(angle) > 0:
            nose = .036*gaussian(x, 0, .014)*gaussian(z, 1.608, .013)
            bridge_depth = .024*gaussian(x, 0, .011)*gaussian(z, 1.642, .031)
            nostril = .010*gaussian(abs(x), .016, .006)*gaussian(z, 1.601, .006)
            cheek = .014*gaussian(abs(x), .047, .020)*gaussian(z, 1.612, .023)
            brow = .031*gaussian(abs(x), .038, .027)*gaussian(z, 1.656+.31*abs(x), .008)
            upper_lip = .020*gaussian(x, 0, .028)*gaussian(z, 1.568, .004)
            lower_lip = .020*gaussian(x, 0, .026)*gaussian(z, 1.555, .006)
            chin = .017*gaussian(x, 0, .032)*gaussian(z, 1.531, .012)
            y -= (nose+bridge_depth+nostril+cheek+brow+upper_lip+lower_lip+chin)*math.cos(angle)**1.4
        vertices.append(Vector((x, y, z)))
        weights.append({'Bip001 Head': 1})
faces.append(tuple(range(sides)))
for r in range(len(levels)-1):
    bridge(faces, list(range(r*sides, (r+1)*sides)), list(range((r+1)*sides, (r+2)*sides)))
faces.append(tuple(range((len(levels)-1)*sides, len(levels)*sides)))
add_surface('Head_surface', vertices, faces, weights, ['AshSkin'])

bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
for obj in generated:
    obj.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.object.join()
rigid_parts = {part: bone for part, bone in json.loads(source['gloom_rigid_parts']).items() if part not in replaced}
rigid_parts['Head_surface'] = 'Bip001 Head'
scene['gloom_rigid_parts'] = json.dumps(rigid_parts, sort_keys=True)
scene['gloom_soft_parts'] = json.dumps([part for part in json.loads(source['gloom_soft_parts']) if part not in replaced]
                                     +['Trousers_continuous', 'Waist_wrap'])
scene['gloom_revision'] = 'Continuous trousers and neutral head, thin cloth waist wrap; unchanged hood, eyes, armor, arms, hands and 53-bone diagnostic rig'
scene['gloom_v06_replaced_parts'] = json.dumps(sorted(replaced))
scene.frame_set(1)
bpy.context.view_layer.update()
model.data.calc_loop_triangles()
print('HOUND_V06_MESH_READY', len(model.data.vertices), 'vertices', len(model.data.loop_triangles), 'triangles')
print('REPLACED_PARTS', sorted(replaced), 'REMOVED_VERTICES', len(removed), 'RIGID_PARTS', len(rigid_parts))
