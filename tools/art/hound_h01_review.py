'''H01 review helpers. Poses are temporary and never change diagnostic keys.'''
import bpy
import math
from mathutils import Matrix, Quaternion, Vector


def setup(scene, width, height):
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.render.resolution_x, scene.render.resolution_y, scene.render.resolution_percentage = width, height, 100
    scene.render.image_settings.file_format = 'PNG'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'MATERIAL'
    scene.display.shading.show_shadows = False
    scene.display.shading.show_cavity = True
    scene.display.shading.cavity_type = 'WORLD'
    scene.display.shading.background_type = 'WORLD'
    if scene.world is None:
        scene.world = bpy.data.worlds.new(scene.name+'_World')
    scene.world.color = (.105,.104,.10)
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'Medium High Contrast'


def camera(scene, position, target, scale):
    data = bpy.data.cameras.new(scene.name+'_Camera')
    obj = bpy.data.objects.new(data.name,data)
    scene.collection.objects.link(obj)
    obj.location = position
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
    data.type, data.ortho_scale = 'ORTHO', scale
    scene.camera = obj
    return obj


def pose(scene, rig, kind, amount=1):
    bpy.context.window.scene = scene
    scene.frame_set(1)
    for bone in rig.pose.bones:
        bone.matrix_basis.identity()
    for suffix,side in (('L',1),('R',-1)):
        if kind in ('fist','grip'):
            for digit in range(1,5):
                for ending,degrees in zip(('','1','2'),(32,38,20) if kind=='fist' else (12,18,8)):
                    name = 'Bip001 '+suffix+' Finger'+str(digit)+ending
                    rest = rig.data.bones[name].matrix_local.to_quaternion()
                    rig.pose.bones[name].rotation_quaternion = rest.inverted()@Quaternion((0,1,0),math.radians(side*degrees*amount))@rest
            for ending,degrees in (('0',16),('01',22)):
                name = 'Bip001 '+suffix+' Finger'+ending
                rest = rig.data.bones[name].matrix_local.to_quaternion()
                rig.pose.bones[name].rotation_quaternion = rest.inverted()@Quaternion((0,1,0),math.radians(side*degrees*amount))@rest
        if kind in ('flex','extend'):
            name = 'Bip001 '+suffix+' Hand'
            rest = rig.data.bones[name].matrix_local.to_quaternion()
            rig.pose.bones[name].rotation_quaternion = rest.inverted()@Quaternion((0,1,0),math.radians(side*25*amount*(1 if kind=='flex' else -1)))@rest
    bpy.context.view_layer.update()


def snapshot(scene, model, target, transform, names=None):
    bpy.context.window.scene = scene
    bpy.context.view_layer.update()
    groups = {g.index for g in model.vertex_groups if g.name.startswith('PART_') and (names is None or g.name[5:] in names)}
    indices = [v.index for v in model.data.vertices if any(w.group in groups for w in v.groups)]
    remap = {old:i for i,old in enumerate(indices)}
    evaluated = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    faces = [p for p in mesh.polygons if all(i in remap for i in p.vertices)]
    data = bpy.data.meshes.new('ReviewSnapshot')
    data.from_pydata([transform@mesh.vertices[i].co for i in indices],[],[[remap[i] for i in p.vertices] for p in faces])
    for material in model.data.materials:
        data.materials.append(material)
    for polygon,reference in zip(data.polygons,faces):
        polygon.material_index,polygon.use_smooth = reference.material_index,reference.use_smooth
    evaluated.to_mesh_clear()
    obj = bpy.data.objects.new(data.name,data)
    target.collection.objects.link(obj)
    return obj


def label(scene, text, x, z, size=.021):
    data = bpy.data.curves.new('ReviewLabel','FONT')
    data.body,data.size,data.align_x = text,size,'CENTER'
    obj = bpy.data.objects.new(data.name,data)
    scene.collection.objects.link(obj)
    obj.location,obj.rotation_euler = (x,-1,z),(math.pi/2,0,0)


def hand_parts(model, suffix, bracer=True):
    prefixes = ('Hand_','Wrist_','Finger_','Thumb_')+ (('Bracer_','Elbow_cuff_') if bracer else ())
    return {g.name[5:] for g in model.vertex_groups if g.name.startswith('PART_') and g.name[5:].startswith(prefixes)
            and (g.name.endswith('_'+suffix) or g.name.startswith(('PART_Finger_'+suffix+'_','PART_Thumb_'+suffix)))}


def weapon(scene, rig):
    # Original mesh and .43 presentation scale; rear-shell support socket is diagnostic, not the runtime anchor.
    bpy.context.window.scene = scene
    before = set(scene.objects)
    bpy.ops.import_scene.gltf(filepath='D:/Projects/Gloom/assets/characters/original/soul_reaper.gltf')
    obj = next(o for o in set(scene.objects)-before if o.type=='MESH')
    side = -1
    down = Vector((side*.16,-.08,-1)).normalized()
    front = Vector((0,-1,0))
    front = (front-down*front.dot(down)).normalized()
    dorsal = front.cross(down).normalized()*side
    rotation = Matrix(((-down).cross(front),-down,front)).transposed().to_4x4()
    wrist = rig.data.bones['Bip001 R Hand'].head_local
    target = wrist+down*.096-dorsal*.045
    socket = Vector((.10,-.75,.28))*.43
    bpy.context.view_layer.update()
    obj.matrix_world = Matrix.Translation(target)@rotation@Matrix.Translation(-socket)@Matrix.Scale(.43,4)@obj.matrix_world
    obj.name = 'ReviewOnly_SoulReaper'
    return obj


def render(scene,path):
    bpy.context.window.scene = scene
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
