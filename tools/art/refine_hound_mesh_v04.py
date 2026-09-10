'''Unify shoulder/biceps/arm envelopes into editable quad surfaces; preserve the v03 diagnostic skeleton.'''
import bpy
import bmesh
import json
import math
from mathutils import Vector
from mathutils.bvhtree import BVHTree

if bpy.data.scenes.get('Hound_Mesh_v04'):
    raise RuntimeError('v04 exists; preserve edits and choose a new version')
source = bpy.data.scenes['Hound_Rig_v03']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original = source.objects['H03_DeformMesh']
original_rig = source.objects['Hound03_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v04')
for key in source.keys():
    scene[key] = source[key]
scene['gloom_authoring_stage'] = 'v04-arm-topology-pass-not-final'
scene['gloom_limitations'] = 'Continuous shoulder-to-wrist surfaces; provisional anatomy, hand/cloth seams and rig weights remain under review'
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1
scene.render.fps = 30
scene.frame_start, scene.frame_end = 1, 181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name, frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v04_EXPORT')
scene.collection.children.link(collection)
rig = original_rig.copy()
rig.data = original_rig.data.copy()
rig.name, rig.data.name = 'Hound04_Rig', 'Hound04_Skeleton'
rig.animation_data.action = original_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound04_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name, model.data.name = 'H04_DeformMesh', 'H04_DeformTopology'
model.parent = rig
for modifier in model.modifiers:
    if modifier.type == 'ARMATURE':
        modifier.object = rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound03_', 'Hound04_')
skin = next(m for m in model.data.materials if 'AshSkin' in m.name)
bpy.context.window.scene = scene
targets = {'PART_'+part+'_'+suffix for suffix in ('L', 'R') for part in ('Arm_continuous', 'Biceps', 'Deltoid')}
target_groups = {original.vertex_groups[name].index for name in targets}
removed = {v.index for v in original.data.vertices if any(g.group in target_groups for g in v.groups)}
bm = bmesh.new()
bm.from_mesh(model.data)
bm.verts.ensure_lookup_table()
bmesh.ops.delete(bm, geom=[bm.verts[i] for i in removed], context='VERTS')
bm.to_mesh(model.data)
bm.free()
model.data.update()
for name in targets:
    model.vertex_groups.remove(model.vertex_groups[name])


def smooth(value, low, high):
    t = max(0, min(1, (value-low)/(high-low)))
    return t*t*(3-2*t)


def center_at(z, side):
    stations = [(1.523, .244, .008), (1.469, .27, .007), (1.405, .286, .007),
                (1.36, .307, .001), (1.209, .372, -.008), (.961, .454, -.036)]
    for (a, ax, ay), (b, bx, by) in zip(stations, stations[1:]):
        if z >= b:
            t = (a-z)/(a-b)
            return Vector((side*(ax+(bx-ax)*t), ay+(by-ay)*t, z))
    return Vector((side*.454, -.036, z))


def weights_at(z, suffix):
    prefix = 'Bip001 '+suffix+' '
    if z > 1.37:
        amount = smooth(z, 1.37, 1.515)
        return {prefix+'UpperArm': 1-amount, prefix+'Clavicle': amount}
    if z > 1.17:
        amount = smooth(z, 1.175, 1.25)
        return {prefix+'Forearm': 1-amount, prefix+'UpperArm': amount}
    amount = smooth(z, .95, 1.0)
    return {prefix+'Hand': 1-amount, prefix+'Forearm': amount}


levels = [1.5229, 1.518, 1.51, 1.50, 1.49, 1.48, 1.47, 1.455, 1.44, 1.425, 1.41, 1.395, 1.38, 1.36,
          1.34, 1.32, 1.30, 1.28, 1.26, 1.245, 1.23, 1.22, 1.21, 1.20, 1.19, 1.175, 1.16,
          1.14, 1.12, 1.10, 1.08, 1.06, 1.04, 1.02, 1.0, .985, .974]
sides = 32
arms = []
projection_errors = {}
for side, suffix in ((1, 'L'), (-1, 'R')):
    groups = {original.vertex_groups['PART_'+part+'_'+suffix].index for part in ('Arm_continuous', 'Biceps', 'Deltoid')}
    selected = {v.index for v in original.data.vertices if any(g.group in groups for g in v.groups)}
    faces = [list(p.vertices) for p in original.data.polygons if all(i in selected for i in p.vertices)]
    tree = BVHTree.FromPolygons([v.co for v in original.data.vertices], faces)
    radii = []
    for z in levels:
        center = center_at(z, side)
        ring = []
        for i in range(sides):
            angle = math.tau*i/sides
            direction = Vector((math.cos(angle), math.sin(angle), 0))
            hit = tree.ray_cast(center+direction*.35, -direction, .7)[0]
            if hit is None:
                raise RuntimeError('Missing envelope intersection: '+str((suffix, z, i)))
            radius = (hit-center).dot(direction)
            if not .008 < radius < .15:
                raise RuntimeError('Invalid projected envelope radius: '+str((suffix, z, i, radius)))
            ring.append(radius)
        radii.append(ring)
    # Smooth the intersections between the old overlapping volumes without shrinking the full silhouette.
    for _ in range(2):
        before = [r[:] for r in radii]
        for r in range(2, len(levels)-2):
            for i in range(sides):
                radii[r][i] = before[r][i]*.7+(before[r-1][i]+before[r+1][i])*.15
    vertices = []
    for r, z in enumerate(levels):
        center = center_at(z, side)
        for i in range(sides):
            angle = math.tau*i/sides
            vertices.append(center+Vector((math.cos(angle), math.sin(angle), 0))*radii[r][i])
    faces = [tuple(range(sides))]
    for r in range(len(levels)-1):
        faces.extend((r*sides+i, (r+1)*sides+i, (r+1)*sides+(i+1)%sides, r*sides+(i+1)%sides) for i in range(sides))
    faces.append(tuple(reversed(range((len(levels)-1)*sides, len(levels)*sides))))
    data = bpy.data.meshes.new('H04_Arm_surface_'+suffix)
    data.from_pydata(vertices, [], faces)
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(data)
    bm.free()
    data.materials.append(skin)
    arm = bpy.data.objects.new(data.name, data)
    collection.objects.link(arm)
    for polygon in data.polygons:
        polygon.use_smooth = len(polygon.vertices) == 4
    for vertex in data.vertices:
        for name, weight in weights_at(vertex.co.z, suffix).items():
            if weight < 1e-6:
                continue
            group = arm.vertex_groups.get(name) or arm.vertex_groups.new(name=name)
            group.add([vertex.index], weight, 'REPLACE')
    arm.vertex_groups.new(name='PART_Arm_surface_'+suffix).add(list(range(len(vertices))), 1, 'REPLACE')
    arm.parent = rig
    modifier = arm.modifiers.new('Hound04 diagnostic skinning', 'ARMATURE')
    modifier.object = rig
    modifier.use_deform_preserve_volume = False
    arms.append(arm)
    projection_errors[suffix] = max(tree.find_nearest(v)[3] for v in vertices)
    if projection_errors[suffix] > .012:
        raise RuntimeError('Envelope diverged by more than 12mm: '+str(projection_errors))

bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
for arm in arms:
    arm.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.object.join()
scene['gloom_soft_parts'] = json.dumps([p for p in json.loads(source['gloom_soft_parts'])
                                       if 'PART_'+p not in targets]+['Arm_surface_L', 'Arm_surface_R'])
scene['gloom_arm_surface_rings'] = len(levels)
scene['gloom_arm_surface_sides'] = sides
scene['gloom_arm_projection_max_error_m'] = json.dumps(projection_errors)
scene['gloom_revision'] = 'Continuous shoulder-biceps-elbow-forearm surface; unchanged rigid armor, hood and 53-bone diagnostic skeleton'
scene.frame_set(1)
bpy.context.view_layer.update()
model.data.calc_loop_triangles()
print('HOUND_V04_MESH_READY', len(model.data.vertices), 'vertices', len(model.data.loop_triangles), 'triangles')
print('PROJECTION_ERROR_M', projection_errors, 'REMOVED_OLD_VERTICES', len(removed))
