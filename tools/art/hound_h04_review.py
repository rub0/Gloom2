'''H04 local assembly poses and shared evidence utilities; never replace the diagnostic action.'''
import bpy
import math
from mathutils import Matrix, Quaternion
from hound_h01_review import setup, camera, snapshot, label, render

CASES = ('rest','twist-left','twist-right','reach','step-left','step-right','hip-left','hip-right','head-left','head-right')


def pose(scene, rig, kind, amount=1):
    bpy.context.window.scene = scene
    scene.frame_set(1)
    for bone in rig.pose.bones:
        bone.matrix_basis = Matrix.Identity(4)
    rotations = []
    if kind.startswith('twist-'):
        sign = 1 if kind=='twist-left' else -1
        rotations = [('Bip001 Spine',(0,0,1),10*sign),('Bip001 Spine1',(0,0,1),10*sign)]
    if kind=='reach':
        for side in ('L','R'):
            rotations += [('Bip001 '+side+' UpperArm',(1,0,0),-50),('Bip001 '+side+' Forearm',(1,0,0),-20)]
    if kind.startswith(('step-','hip-')):
        side = 'L' if kind.endswith('left') else 'R'
        other = 'R' if side=='L' else 'L'
        hip = kind.startswith('hip-')
        rotations = [('Bip001 '+side+' Thigh',(1,0,0),-40 if hip else -25),
                     ('Bip001 '+side+' Calf',(1,0,0),50 if hip else 30),
                     ('Hound '+side+' HipGuard',(1,0,0),-40 if hip else -25)]
        if not hip:
            rotations += [('Bip001 '+other+' Thigh',(1,0,0),15),('Bip001 '+other+' Calf',(1,0,0),15),
                          ('Hound '+other+' HipGuard',(1,0,0),15)]
    if kind.startswith('head-'):
        rotations = [('Bip001 Head',(0,0,1),35 if kind=='head-left' else -35)]
    for name, axis, degrees in rotations:
        rest = rig.data.bones[name].matrix_local.to_quaternion()
        rig.pose.bones[name].rotation_quaternion = rest.inverted()@Quaternion(axis,math.radians(degrees*amount))@rest
    bpy.context.view_layer.update()


def parts(model):
    result = {g.name[5:]: [] for g in model.vertex_groups if g.name.startswith('PART_')}
    for vertex in model.data.vertices:
        for weight in vertex.groups:
            name = model.vertex_groups[weight.group].name
            if name.startswith('PART_'):
                result[name[5:]].append(vertex.index)
    return result


def contact_pairs(names):
    # Every pair within these local assemblies, including cloth/metal and different rigid plates.
    zones = [
        {'Trousers_continuous','Waist_wrap','Sash_tail_L','Sash_tail_R','Torso_underlayer',
         'Hip_guard_L','Hip_guard_R','Thigh_outer_plate_L','Thigh_outer_plate_R','Abdominal_plate_3','Back_spine_2'},
        {n for n in names if n.startswith(('Breastplate_','Abdominal_','Back_spine_','Scapula_','Rib_flank_','Collar_','Yoke_'))}
        | {'Torso_underlayer','Sternum','Hood_continuous','Neck_surface','Arm_surface_L','Arm_surface_R'},
        {'Hood_continuous','Neck_surface','Head_surface','Eye_L','Eye_R','Mouth_shadow'}]
    pairs = set()
    for zone in zones:
        ordered = sorted(zone & names)
        for i,a in enumerate(ordered):
            for b in ordered[i+1:]:
                pairs.add((a,b))
    return sorted(pairs)
