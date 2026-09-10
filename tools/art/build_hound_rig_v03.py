'''Build a non-final deformation prototype from the approved v02; run in Blender.'''
import bpy
import bmesh
import json
import math
from mathutils import Matrix, Quaternion, Vector

if bpy.data.scenes.get('Hound_Rig_v03'):
    raise RuntimeError('v03 already exists; preserve artist edits and use a new version')
source = bpy.data.scenes['Hound_Blockout_v02']
bpy.context.window.scene = source
bpy.context.view_layer.update()
depsgraph = bpy.context.evaluated_depsgraph_get()
scene = bpy.data.scenes.new('Hound_Rig_v03')
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1
scene['gloom_authoring_stage'] = 'v03-deformation-prototype-not-final'
scene['gloom_approved_reference'] = 'art/characters/hound/v02/hound-blockout-v02.blend @ 67cda2e; approval d7fc1f8'
scene['gloom_forward'] = '-Y Blender / +Z glTF'
scene['gloom_limitations'] = 'FK joint test only; no retargeted locomotion, weapon grips, final retopology, UVs or gameplay replacement'
model = bpy.data.collections.new('HOUND_v03_EXPORT')
scene.collection.children.link(model)
materials = {}
parts = []
replaced = ('Upper_arm_', 'Forearm_skin_', 'Thigh_cloth_', 'Calf_underlayer_')
for original in bpy.data.collections['HOUND_v02_MODEL_ONLY'].objects:
    name = original.name[4:]
    if name.startswith(replaced):
        continue
    evaluated = original.evaluated_get(depsgraph)
    data = bpy.data.meshes.new_from_object(evaluated, preserve_all_data_layers=True, depsgraph=depsgraph)
    data.transform(original.matrix_world)
    obj = bpy.data.objects.new('H03_'+name, data)
    model.objects.link(obj)
    original_material = original.data.materials[0]
    material_name = original_material.name.replace('Hound02_', '')
    if material_name not in materials:
        materials[material_name] = original_material.copy()
        materials[material_name].name = 'Hound03_'+material_name
    data.materials.clear()
    data.materials.append(materials[material_name])
    parts.append(obj)
bpy.context.window.scene = scene


def new_mesh(name, vertices, faces, material):
    data = bpy.data.meshes.new('H03_'+name)
    data.from_pydata(vertices, [], faces)
    data.update()
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(data)
    bm.free()
    data.materials.append(material)
    obj = bpy.data.objects.new('H03_'+name, data)
    model.objects.link(obj)
    parts.append(obj)
    return obj


def smooth(value, low, high):
    t = max(0, min(1, (value-low)/(high-low)))
    return t*t*(3-2*t)


def blend(a, b, t):
    return {a: 1-t, b: t}


def hand_frame(side):
    wrist = Vector((side*.454, -.036, .961))
    down = Vector((side*.16, -.08, -1)).normalized()
    front = Vector((0, -1, 0))
    front = (front-down*front.dot(down)).normalized()
    return wrist, down, front, front.cross(down).normalized()*side


def hand_point(side, u, v, w):
    wrist, down, front, dorsal = hand_frame(side)
    return wrist+front*u+down*v+dorsal*w


# Same 43 semantic names as the legacy rig, but corrected rest pose, units and hierarchy.
# Eight distal finger bones and two independently poseable hip plates are additional.
armature = bpy.data.armatures.new('Hound03_Skeleton')
rig = bpy.data.objects.new('Hound03_Rig', armature)
model.objects.link(rig)
rig.show_in_front = True
armature.display_type = 'OCTAHEDRAL'
bpy.context.view_layer.objects.active = rig
rig.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
bone_specs = {}


def bone(name, head, tail, parent=None):
    entry = armature.edit_bones.new(name)
    entry.head, entry.tail = head, tail
    if parent:
        entry.parent = armature.edit_bones[parent]
        entry.use_connect = (entry.head-entry.parent.tail).length < 1e-6
    entry.align_roll(Vector((0, -1, 0)))
    bone_specs[name] = (Vector(head), Vector(tail), parent)
    return entry


bone('Bip001', (0, 0, 0), (0, 0, .22))
bone('Bip001 Pelvis', (0, .01, .946), (0, .01, 1.04), 'Bip001')
bone('Bip001 Spine', (0, .01, 1.04), (0, .005, 1.17), 'Bip001 Pelvis')
bone('Bip001 Spine1', (0, .005, 1.17), (0, .006, 1.32), 'Bip001 Spine')
bone('Bip001 Spine2', (0, .006, 1.32), (0, .012, 1.47), 'Bip001 Spine1')
bone('Bip001 Neck', (0, .012, 1.47), (0, .004, 1.56), 'Bip001 Spine2')
bone('Bip001 Head', (0, .004, 1.56), (0, .004, 1.74), 'Bip001 Neck')
for side, suffix in ((1, 'L'), (-1, 'R')):
    prefix = 'Bip001 '+suffix+' '
    shoulder = (side*.274, .004, 1.437)
    elbow = (side*.372, -.008, 1.209)
    wrist = (side*.454, -.036, .961)
    hip = (side*.122, .01, .946)
    knee = (side*.173, -.005, .563)
    ankle = (side*.197, .012, .146)
    bone(prefix+'Clavicle', (side*.075, .012, 1.477), shoulder, 'Bip001 Spine2')
    bone(prefix+'UpperArm', shoulder, elbow, prefix+'Clavicle')
    bone(prefix+'Forearm', elbow, wrist, prefix+'UpperArm')
    bone(prefix+'Hand', wrist, hand_point(side, 0, .103, 0), prefix+'Forearm')
    bone(prefix+'Thigh', hip, knee, 'Bip001 Pelvis')
    bone(prefix+'Calf', knee, ankle, prefix+'Thigh')
    bone(prefix+'Foot', ankle, (side*.201, -.13, .065), prefix+'Calf')
    bone(prefix+'Toe0', (side*.201, -.13, .065), (side*.201, -.205, .065), prefix+'Foot')
    bone('Hound '+suffix+' HipGuard', (side*.19, 0, .99), (side*.215, 0, .78), 'Bip001 Pelvis')
    for digit, (u, length) in enumerate(((.034, .078), (.011, .09), (-.013, .084), (-.035, .065)), 1):
        chain = [(u, .103, -.001), (u, .133, -.012), (u, .103+length*.73, -.034), (u, .103+length, -.057)]
        for segment in range(3):
            name = prefix+'Finger'+str(digit)+('' if segment == 0 else str(segment))
            parent = prefix+'Hand' if segment == 0 else prefix+'Finger'+str(digit)+('' if segment == 1 else '1')
            bone(name, hand_point(side, *chain[segment]), hand_point(side, *chain[segment+1]), parent)
    thumb = [(.044, .039, -.007), (.08, .073, -.018), (.063, .105, -.041)]
    bone(prefix+'Finger0', hand_point(side, *thumb[0]), hand_point(side, *thumb[1]), prefix+'Hand')
    bone(prefix+'Finger01', hand_point(side, *thumb[1]), hand_point(side, *thumb[2]), prefix+'Finger0')
bpy.ops.object.mode_set(mode='OBJECT')
for name in ('Body', 'Arms', 'Legs', 'Fingers', 'Armor'):
    armature.collections.new(name)
for entry in armature.bones:
    group = 'Body'
    if 'Finger' in entry.name:
        group = 'Fingers'
    elif 'HipGuard' in entry.name:
        group = 'Armor'
    elif any(k in entry.name for k in ('Clavicle', 'UpperArm', 'Forearm', 'Hand')):
        group = 'Arms'
    elif any(k in entry.name for k in ('Thigh', 'Calf', 'Foot', 'Toe')):
        group = 'Legs'
    armature.collections[group].assign(entry)
rig['gloom_rig_compatibility'] = '43 legacy names retained; changed rest axes, lengths and parentage require retargeting, not direct clip copying'
rig['gloom_controls'] = 'FK pose bones; bone collections Body/Arms/Legs/Fingers/Armor; root motion intentionally zero in diagnostic clip'


def assign(obj, index, weights):
    total = sum(weights.values())
    for name, weight in weights.items():
        if weight <= 1e-6:
            continue
        group = obj.vertex_groups.get(name) or obj.vertex_groups.new(name=name)
        group.add([index], weight/total, 'REPLACE')


def torso_weights(z):
    stations = [(1.00, 'Bip001 Pelvis'), (1.12, 'Bip001 Spine'), (1.27, 'Bip001 Spine1'), (1.43, 'Bip001 Spine2')]
    if z <= stations[0][0]:
        return {stations[0][1]: 1}
    for (low, a), (high, b) in zip(stations, stations[1:]):
        if z <= high:
            return blend(a, b, smooth(z, low, high))
    return {stations[-1][1]: 1}


def segment_weights(t, first, second, parent, child):
    if t < .17:
        return blend(parent, first, smooth(t, -.06, .17))
    if t > 1.84:
        return blend(second, child, smooth(t, 1.84, 2.08))
    return blend(first, second, smooth(t, .84, 1.16))


def connected_limb(side, suffix, kind):
    prefix = 'Bip001 '+suffix+' '
    arm = kind == 'Arm'
    first, second = prefix+('UpperArm' if arm else 'Thigh'), prefix+('Forearm' if arm else 'Calf')
    parent, child = prefix+'Clavicle' if arm else 'Bip001 Pelvis', prefix+('Hand' if arm else 'Foot')
    start, joint, _ = bone_specs[first]
    end = bone_specs[second][1]
    rotation_a = (joint-start).to_track_quat('Z', 'Y')
    rotation_b = (end-joint).to_track_quat('Z', 'Y')
    # Explicit support loops on both sides of the joint; no overlapping closed limb caps.
    samples = [0, .06, .12, .18, .3, .42, .5, .65, .78, .84, .9, .95, 1, 1.05, 1.1, 1.16, 1.22, 1.35, 1.5, 1.65, 1.82, 1.92, 2]
    profile = [(0, .62), (.18, .98), (.5, 1), (.82, .8), (1, .56)]
    thigh_profile = [(0, .9), (.23, 1.04), (.57, 1), (.84, .85), (1, .65)]

    def radius(t, keys):
        for (low, a), (high, b) in zip(keys, keys[1:]):
            if t <= high:
                return a+(b-a)*(t-low)/(high-low)
        return keys[-1][1]

    vertices = []
    sides = 12
    for t in samples:
        center = start.lerp(joint, t) if t <= 1 else joint.lerp(end, t-1)
        rotation = rotation_a.slerp(rotation_b, smooth(t, .78, 1.22))
        upper_width, lower_width = (.088, .069) if arm else (.108, .078)
        upper_depth, lower_depth = (.087, .072) if arm else (.108, .080)
        keys = profile if arm or t > 1 else thigh_profile
        width = (upper_width if t <= 1 else lower_width)*radius(t if t <= 1 else t-1, keys)
        depth = (upper_depth if t <= 1 else lower_depth)*radius(t if t <= 1 else t-1, keys)
        joint_width = (upper_width*(.56 if arm else .65)+lower_width*.62)*.5
        joint_depth = (upper_depth*(.56 if arm else .65)+lower_depth*.62)*.5
        blend_joint = 1-min(1, abs(t-1)/.1)
        width, depth = width*(1-blend_joint)+joint_width*blend_joint, depth*(1-blend_joint)+joint_depth*blend_joint
        for i in range(sides):
            angle = math.tau*i/sides
            vertices.append(center+rotation@Vector((width*math.cos(angle), depth*math.sin(angle), 0)))
    faces = [tuple(reversed(range(sides)))]
    for ring in range(len(samples)-1):
        faces.extend((ring*sides+i, ring*sides+(i+1)%sides, (ring+1)*sides+(i+1)%sides, (ring+1)*sides+i) for i in range(sides))
    faces.append(tuple((len(samples)-1)*sides+i for i in range(sides)))
    obj = new_mesh(kind+'_continuous_'+suffix, vertices, faces, materials['AshSkin' if arm else 'BurgundyCloth'])
    if not arm:
        obj.data.materials.append(materials['Undersuit'])
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
        if not arm and polygon.index > 12*12:
            polygon.material_index = 1
    for index in range(len(vertices)):
        assign(obj, index, segment_weights(samples[index//sides], first, second, parent, child))
    obj['gloom_continuous_joint'] = 'elbow' if arm else 'knee'


rigid_parts = {}
soft_parts = []
for obj in parts:
    name = obj.name[4:]
    suffix = name[-1] if name.endswith(('_L', '_R')) else None
    prefix = 'Bip001 '+suffix+' ' if suffix else None
    rigid = None
    soft = name.startswith(('Torso_', 'Pelvis_', 'Waist_', 'Neck', 'Hood_', 'Biceps_', 'Deltoid_', 'Palm_'))
    if name.startswith('Finger_'):
        _, suffix, digit = name.split('_')
        rigid = 'Bip001 '+suffix+' Finger'+str(int(digit[0])+1)+{'A': '', 'B': '1', 'C': '2'}[digit[1]]
    elif name.startswith('Thumb_'):
        rigid = 'Bip001 '+name[-2]+' Finger0'+('' if name[-1] == 'A' else '1')
    elif name.startswith(('Hand_back_', 'Wrist_transition_')):
        rigid = prefix+'Hand'
    elif name.startswith(('Bracer_', 'Elbow_')):
        rigid = prefix+'Forearm'
    elif name.startswith(('Greave_', 'Shin_', 'Knee_')):
        rigid = prefix+'Calf'
    elif name.startswith(('Boot', 'Ankle_')):
        rigid = prefix+'Foot'
    elif name.startswith('Thigh_outer_'):
        rigid = prefix+'Thigh'
    elif name.startswith('Hip_guard_'):
        rigid = 'Hound '+suffix+' HipGuard'
    elif name.startswith('Sash_tail_'):
        rigid = 'Bip001 Pelvis'
    elif name.startswith(('Head_', 'Jaw_', 'Nose_', 'Mouth_', 'Eye_', 'Cheek_', 'Brow_')):
        rigid = 'Bip001 Head'
    elif name.startswith(('Abdominal_', 'Back_spine_')):
        z = sum(v.co.z for v in obj.data.vertices)/len(obj.data.vertices)
        rigid = 'Bip001 Spine1' if z > 1.22 else 'Bip001 Spine'
    elif not soft:
        rigid = 'Bip001 Spine2'
    if soft:
        # Linear subdivision provides weight samples without shrinking approved forms.
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.subdivide_edges(bm, edges=list(bm.edges), cuts=2, use_grid_fill=True)
        bm.to_mesh(obj.data)
        bm.free()
        obj.data.update()
        soft_parts.append(name)
    if rigid:
        rigid_parts[name] = rigid
    for vertex in obj.data.vertices:
        p = vertex.co
        if rigid:
            weights = {rigid: 1}
        elif name.startswith('Hood_'):
            weights = blend('Bip001 Spine2', 'Bip001 Head', smooth(p.z, 1.46, 1.62))
        elif name == 'Neck':
            weights = blend('Bip001 Neck', 'Bip001 Head', smooth(p.z, 1.49, 1.56))
        elif name.startswith(('Biceps_', 'Deltoid_')):
            a, b, _ = bone_specs[prefix+'UpperArm']
            t = (p-a).dot(b-a)/(b-a).length_squared
            weights = segment_weights(t, prefix+'UpperArm', prefix+'Forearm', prefix+'Clavicle', prefix+'Hand')
        elif name.startswith('Palm_'):
            side = 1 if suffix == 'L' else -1
            wrist, down, _, _ = hand_frame(side)
            weights = blend(prefix+'Forearm', prefix+'Hand', smooth((p-wrist).dot(down), -.025, .025))
        else:
            weights = torso_weights(p.z)
        assign(obj, vertex.index, weights)
for side, suffix in ((1, 'L'), (-1, 'R')):
    connected_limb(side, suffix, 'Arm')
    connected_limb(side, suffix, 'Leg')

# Provenance groups keep every rigid piece selectable after grouping by material.
for obj in parts:
    obj.vertex_groups.new(name='PART_'+obj.name[4:]).add(list(range(len(obj.data.vertices))), 1, 'REPLACE')
    obj.parent = rig
    modifier = obj.modifiers.new('Hound03 skinning', 'ARMATURE')
    modifier.object = rig
    modifier.use_deform_preserve_volume = False  # Same linear skinning as Gloom, not Blender-only dual quaternions.
bpy.context.view_layer.update()
bpy.ops.object.select_all(action='DESELECT')
for obj in parts:
    obj.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()
combined = bpy.context.object
combined.name = 'H03_DeformMesh'
combined.data.name = 'H03_DeformTopology'
# One mesh/skin, seven material primitives. No 113-object runtime organization.
scene['gloom_rigid_parts'] = json.dumps(rigid_parts, sort_keys=True)
scene['gloom_soft_parts'] = json.dumps(soft_parts+['Arm_continuous_L', 'Arm_continuous_R', 'Leg_continuous_L', 'Leg_continuous_R'])
scene['gloom_joint_loops'] = 23
scene['gloom_legacy_bone_names'] = 43
scene['gloom_total_bones'] = len(armature.bones)

# A diagnostic pose sequence, explicitly not a locomotion/Bite production animation.
scene.render.fps = 30
scene.frame_start, scene.frame_end = 1, 181
rig.animation_data_create()
action = bpy.data.actions.new('Hound03_joint_check')
rig.animation_data.action = action


def rotate_world(name, axis, degrees):
    rest = rig.data.bones[name].matrix_local.to_quaternion()
    rig.pose.bones[name].rotation_quaternion = rest.inverted()@Quaternion(axis, math.radians(degrees))@rest


def set_test_pose(stage):
    for pose_bone in rig.pose.bones:
        pose_bone.rotation_mode = 'QUATERNION'
        pose_bone.location = (0, 0, 0)
        pose_bone.rotation_quaternion = (1, 0, 0, 0)
        pose_bone.scale = (1, 1, 1)
    if stage == 'elbows':
        for suffix in ('L', 'R'):
            rotate_world('Bip001 '+suffix+' Forearm', (1, 0, 0), -85)
    elif stage == 'reach':
        for suffix in ('L', 'R'):
            rotate_world('Bip001 '+suffix+' UpperArm', (1, 0, 0), -70)
            rotate_world('Bip001 '+suffix+' Forearm', (1, 0, 0), -25)
    elif stage == 'step':
        rotate_world('Bip001 L Thigh', (1, 0, 0), -38)
        rotate_world('Bip001 L Calf', (1, 0, 0), 62)
        rotate_world('Bip001 L Foot', (1, 0, 0), -24)
        rotate_world('Hound L HipGuard', (1, 0, 0), -32)
    elif stage == 'torso':
        rotate_world('Bip001 Spine1', (0, 0, 1), 12)
        rotate_world('Bip001 Spine2', (0, 0, 1), 10)
        rotate_world('Bip001 Head', (0, 0, 1), -15)
    elif stage == 'fingers':
        for suffix in ('L', 'R'):
            for digit in range(1, 5):
                for end in ('', '1', '2'):
                    rotate_world('Bip001 '+suffix+' Finger'+str(digit)+end, (0, 1, 0), 12 if suffix == 'L' else -12)


for frame, stage in [(1, 'rest'), (31, 'elbows'), (61, 'reach'), (91, 'step'), (121, 'torso'), (151, 'fingers'), (181, 'rest')]:
    set_test_pose(stage)
    for pose_bone in rig.pose.bones:
        pose_bone.keyframe_insert(data_path='rotation_quaternion', frame=frame, group=pose_bone.name)
        pose_bone.keyframe_insert(data_path='location', frame=frame, group=pose_bone.name)
    scene.timeline_markers.new(stage.upper(), frame=frame)
for curve in action.fcurves:
    for key in curve.keyframe_points:
        key.interpolation = 'LINEAR'
scene.frame_set(1)
bpy.context.view_layer.update()
combined.data.calc_loop_triangles()
print('HOUND_V03_BUILT', len(combined.data.vertices), 'vertices;', len(combined.data.loop_triangles), 'triangles;', len(armature.bones), 'bones')
print('RIGID_PARTS', len(rigid_parts), 'SOFT_PARTS', len(soft_parts)+4)
print('NEXT: validate, inspect diagnostic poses, save/export with review_hound_rig_v03.py')
