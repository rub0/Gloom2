'''Temporary H02 leg poses and views; never save these transforms over the source diagnostic action.'''
import bpy
import math
from mathutils import Quaternion, Vector
from hound_h01_review import setup, camera, snapshot, label, render


def leg_parts(model,suffix=None):
    return {g.name[5:] for g in model.vertex_groups if g.name.startswith('PART_')
            and (g.name[5:].startswith(('Greave_','Knee_','Shin_','Boot_','Ankle_')) or g.name=='PART_Trousers_continuous')
            and (suffix is None or g.name.endswith('_'+suffix) or g.name=='PART_Trousers_continuous')}


def pose(scene,rig,model,kind,amount=1):
    bpy.context.window.scene = scene
    scene.frame_set(1)
    for bone in rig.pose.bones:
        bone.matrix_basis.identity()
    for suffix in ('L','R'):
        angles = {'rest':(0,0,0),'knee':(-35,70,-35),'toe':(5,12,8),'heel':(-5,8,-23),
                  'step':(-38,62,-24) if suffix=='L' else (0,0,0)}[kind]
        for ending,angle in zip(('Thigh','Calf','Foot'),angles):
            name = 'Bip001 '+suffix+' '+ending
            rest = rig.data.bones[name].matrix_local.to_quaternion()
            rig.pose.bones[name].rotation_quaternion = rest.inverted()@Quaternion((1,0,0),math.radians(angle*amount))@rest
        name = 'Hound '+suffix+' HipGuard'
        rest = rig.data.bones[name].matrix_local.to_quaternion()
        rig.pose.bones[name].rotation_quaternion = rest.inverted()@Quaternion((1,0,0),math.radians(angles[0]*amount))@rest
    bpy.context.view_layer.update()
    groups = {g.index for g in model.vertex_groups if g.name.startswith('PART_Boot_')}
    ids = [v.index for v in model.data.vertices if any(w.group in groups for w in v.groups)]
    ev = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = ev.to_mesh()
    low = min(mesh.vertices[i].co.z for i in ids)
    ev.to_mesh_clear()
    root = rig.pose.bones['Bip001']
    root.location = rig.data.bones['Bip001'].matrix_local.to_3x3().inverted()@Vector((0,0,-low))
    bpy.context.view_layer.update()
    return -low


def floor(scene):
    data = bpy.data.meshes.new('ReviewGround')
    data.from_pydata([(-2,-2,-.001),(2,-2,-.001),(2,2,-.001),(-2,2,-.001)],[],[(0,1,2,3)])
    obj = bpy.data.objects.new('ReviewGround',data)
    scene.collection.objects.link(obj)
    material = bpy.data.materials.new('ReviewGround')
    material.diffuse_color = (.14,.14,.135,1)
    data.materials.append(material)
