'''Connect each palm to five inner digit sleeves; preserve approved armor and the diagnostic skeleton.'''
import bpy
import bmesh
import json
from mathutils import Vector

if bpy.data.scenes.get('Hound_Mesh_v05'):
    raise RuntimeError('v05 exists; preserve edits and choose a new version')
source = bpy.data.scenes['Hound_Mesh_v04']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original = source.objects['H04_DeformMesh']
original_rig = source.objects['Hound04_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v05')
for key in source.keys():
    scene[key] = source[key]
scene['gloom_authoring_stage'] = 'v05-hand-topology-pass-not-final'
scene['gloom_limitations'] = 'Connected inner gloves under unchanged armor; provisional weights, no validated weapon grips or final animation'
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1
scene.render.fps = 30
scene.frame_start, scene.frame_end = 1, 181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name, frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v05_EXPORT')
scene.collection.children.link(collection)
rig = original_rig.copy()
rig.data = original_rig.data.copy()
rig.name, rig.data.name = 'Hound05_Rig', 'Hound05_Skeleton'
rig.animation_data.action = original_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound05_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name, model.data.name = 'H05_DeformMesh', 'H05_DeformTopology'
model.parent = rig
for modifier in model.modifiers:
    if modifier.type == 'ARMATURE':
        modifier.object = rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound04_', 'Hound05_')
material = next(m for m in model.data.materials if 'Undersuit' in m.name)
bpy.context.window.scene = scene
targets = {'PART_Palm_L', 'PART_Palm_R'}
groups = {model.vertex_groups[name].index for name in targets}
removed = {v.index for v in model.data.vertices if any(g.group in groups for g in v.groups)}
bm = bmesh.new()
bm.from_mesh(model.data)
bm.verts.ensure_lookup_table()
bmesh.ops.delete(bm, geom=[bm.verts[i] for i in removed], context='VERTS')
bm.to_mesh(model.data)
bm.free()
for name in targets:
    model.vertex_groups.remove(model.vertex_groups[name])


def bridge(faces, first, second):
    for i in range(4):
        faces.append((first[i], second[i], second[(i+1)%4], first[(i+1)%4]))


def influence_weight(entry):
    return entry[1]


gloves = []
for side, suffix in ((1, 'L'), (-1, 'R')):
    prefix = 'Bip001 '+suffix+' '
    wrist = rig.data.bones[prefix+'Hand'].head_local.copy()
    down = Vector((side*.16, -.08, -1)).normalized()
    front = Vector((0, -1, 0))
    front = (front-down*front.dot(down)).normalized()
    dorsal = front.cross(down).normalized()*side
    vertices, faces, weights = [], [], []
    columns = [-.045, -.025, -.001, .023, .045]
    rows = [(-.012, .70, .021), (.026, .95, .026), (.051, 1.05, .028), (.077, 1.08, .028), (.103, 1, .025)]
    grid = {}
    for layer, sign in enumerate((1, -1)):
        for row, (v, width, depth) in enumerate(rows):
            for col, u in enumerate(columns):
                grid[layer, row, col] = len(vertices)
                vertices.append(wrist+front*(u*width)+down*v+dorsal*(sign*depth))
                weights.append({prefix+'Hand': 1})
        for row in range(4):
            for col in range(4):
                faces.append(tuple(grid[layer, r, c] for r, c in ((row, col), (row, col+1), (row+1, col+1), (row+1, col))))
    for row in range(4):
        for col in (0, 4):
            if col == 4 and row == 1:
                continue  # Real side opening for the thumb web, not an overlapping cylinder.
            faces.append((grid[0, row, col], grid[1, row, col], grid[1, row+1, col], grid[0, row+1, col]))
    for col in range(4):
        faces.append((grid[0, 0, col], grid[1, 0, col], grid[1, 0, col+1], grid[0, 0, col+1]))
    branches = []
    for col, digit in enumerate((4, 3, 2, 1)):
        port = [grid[0, 4, col], grid[1, 4, col], grid[1, 4, col+1], grid[0, 4, col+1]]
        bones = [prefix+'Finger'+str(digit)+ending for ending in ('', '1', '2')]
        branches.append((port, bones, front, .008 if digit in (1, 2) else .0073))
    branches.append(([grid[0, 1, 4], grid[1, 1, 4], grid[1, 2, 4], grid[0, 2, 4]],
                     [prefix+'Finger0', prefix+'Finger01'], down, .0105))
    for port, bones, cross_axis, radius in branches:
        previous = port
        previous_tangent = None
        for segment, name in enumerate(bones):
            bone = rig.data.bones[name]
            tangent = (bone.tail_local-bone.head_local).normalized()
            if previous_tangent is None:
                across = (cross_axis-tangent*cross_axis.dot(tangent)).normalized()
                outward = across.cross(tangent).normalized()
                if outward.dot(dorsal) < 0:
                    outward = -outward
            else:
                # Parallel transport avoids a half-turn pinch at the bent thumb joint.
                rotation = previous_tangent.rotation_difference(tangent)
                across, outward = rotation@across, rotation@outward
            previous_tangent = tangent
            for t in (.24, .60, .90, 1.0):
                center = bone.head_local.lerp(bone.tail_local, t)
                r = radius*(1-.08*segment)
                if segment == len(bones)-1:
                    r *= 1-.55*t*t
                influences = {name: 1}
                if t < .40:
                    neighbor = bones[segment-1] if segment else prefix+'Hand'
                    amount = .5*(1-t/.40)
                    influences = {name: 1-amount, neighbor: amount}
                elif t > .60 and segment < len(bones)-1:
                    amount = .5*(t-.60)/.40
                    influences = {name: 1-amount, bones[segment+1]: amount}
                loop = []
                for a, b in ((-1, 1), (-1, -1), (1, -1), (1, 1)):
                    loop.append(len(vertices))
                    vertices.append(center+across*(a*r)+outward*(b*r))
                    weights.append(influences)
                bridge(faces, previous, loop)
                previous = loop
        faces.append(tuple(previous))
    data = bpy.data.meshes.new('H05_Hand_glove_'+suffix)
    data.from_pydata(vertices, [], faces)
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    assert all(edge.is_manifold for edge in bm.edges), 'Open or non-manifold glove cage'
    bm.to_mesh(data)
    bm.free()
    data.materials.append(material)
    glove = bpy.data.objects.new(data.name, data)
    collection.objects.link(glove)
    for index, influences in enumerate(weights):
        for name, weight in influences.items():
            group = glove.vertex_groups.get(name) or glove.vertex_groups.new(name=name)
            group.add([index], weight, 'REPLACE')
    glove.vertex_groups.new(name='PART_Hand_glove_'+suffix).add(list(range(len(vertices))), 1, 'REPLACE')
    bpy.ops.object.select_all(action='DESELECT')
    glove.select_set(True)
    bpy.context.view_layer.objects.active = glove
    modifier = glove.modifiers.new('One quad subdivision for rounded finger webs', 'SUBSURF')
    modifier.levels = modifier.render_levels = 1
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    bone_names = set(rig.data.bones.keys())
    for vertex in glove.data.vertices:
        influences = [(g.group, g.weight) for g in vertex.groups if glove.vertex_groups[g.group].name in bone_names]
        influences.sort(key=influence_weight, reverse=True)
        kept = [(index, weight) for index, weight in influences[:4] if weight > 1e-6]
        for index, _ in influences:
            glove.vertex_groups[index].remove([vertex.index])
        total = sum(weight for _, weight in kept)
        for index, weight in kept:
            glove.vertex_groups[index].add([vertex.index], weight/total, 'REPLACE')
    for polygon in glove.data.polygons:
        polygon.use_smooth = True
    glove.parent = rig
    modifier = glove.modifiers.new('Hound05 diagnostic skinning', 'ARMATURE')
    modifier.object = rig
    modifier.use_deform_preserve_volume = False
    gloves.append(glove)

bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
for glove in gloves:
    glove.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.object.join()
scene['gloom_soft_parts'] = json.dumps([p for p in json.loads(source['gloom_soft_parts'])
                                       if 'PART_'+p not in targets]+['Hand_glove_L', 'Hand_glove_R'])
scene['gloom_revision'] = 'Two connected inner gloves; unchanged 93 rigid armor parts, arms and 53-bone diagnostic skeleton'
scene.frame_set(1)
bpy.context.view_layer.update()
model.data.calc_loop_triangles()
print('HOUND_V05_MESH_READY', len(model.data.vertices), 'vertices', len(model.data.loop_triangles), 'triangles')
print('REMOVED_PALM_VERTICES', len(removed))
