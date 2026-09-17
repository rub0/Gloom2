'''Temporary H03 poses, sharing the existing neutral review utilities. Never save over diagnostic keys.'''
import bpy
import math
from mathutils import Quaternion
from hound_h01_review import setup, camera, snapshot, label, render

HEAD_PARTS = {'Head_surface','Eye_L','Eye_R','Mouth_shadow','Neck_surface'}
CASES = ('rest','turn-left','turn-right','look-up','look-down','elbow','reach')


def pose(scene,rig,kind,amount=1):
    bpy.context.window.scene = scene
    scene.frame_set(1)
    for bone in rig.pose.bones:
        bone.matrix_basis.identity()
    rotations = []
    if kind.startswith('turn-'):
        rotations = [('Bip001 Head',(0,0,1),35 if kind=='turn-left' else -35)]
    if kind in ('look-up','look-down'):
        sign = -1 if kind=='look-up' else 1
        rotations = [('Bip001 Neck',(1,0,0),10*sign),('Bip001 Head',(1,0,0),20*sign)]
    for side in ('L','R'):
        if kind=='elbow':
            rotations.append(('Bip001 '+side+' Forearm',(1,0,0),-85))
        if kind=='reach':
            rotations += [('Bip001 '+side+' UpperArm',(1,0,0),-70),('Bip001 '+side+' Forearm',(1,0,0),-25)]
    for name,axis,degrees in rotations:
        rest = rig.data.bones[name].matrix_local.to_quaternion()
        rig.pose.bones[name].rotation_quaternion = rest.inverted()@Quaternion(axis,math.radians(degrees*amount))@rest
    bpy.context.view_layer.update()
