"""Reimport actual cooked LODs, compare animated surfaces and render matched review cameras."""
import bpy
import json
import math
import sys
from pathlib import Path
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import hound_h01_review as hands
import hound_h03_review as anatomy
import hound_h04_review as assembly

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT/'.cache/hound-production-v17'
OUT = WORK/'lod-review'
OUT.mkdir(exist_ok=True)
source = bpy.data.scenes['Hound_Mesh_v17']
model = source.objects['H17_DeformMesh']
parts = assembly.parts(model)
owners = {i:n for n,ids in parts.items() for i in ids}
tree = KDTree(len(model.data.vertices))
for v in model.data.vertices:
    tree.insert(v.co, v.index)
tree.balance()
stats = {n:{'cooked_vertices':0, 'referenced_vertices':[0]*4, 'triangles':[0]*4, 'geometry_bytes':0} for n in parts}
cooked = json.loads((WORK/'cooked.json').read_text())
scenes, rigs = [], []
for lod, name in enumerate(('lod0','lod1','lod2','fps')):
    gltf = json.loads((WORK/(name+'.gltf')).read_text())
    binary = (WORK/(name+'.bin')).read_bytes()
    for primitive in gltf['meshes'][0]['primitives']:
        def accessor(index, dtype, width):
            a = gltf['accessors'][index]
            view = gltf['bufferViews'][a['bufferView']]
            return np.frombuffer(binary, dtype, a['count']*width, view.get('byteOffset',0)+a.get('byteOffset',0)).reshape(-1,width)
        positions = accessor(primitive['attributes']['POSITION'], '<f4', 3)
        indices = accessor(primitive['indices'], '<u4', 1).reshape(-1,3)
        names = []
        for p in positions:
            point, index, distance = tree.find(Vector((float(p[0]),-float(p[2]),float(p[1]))))
            assert distance<1e-5, distance
            names.append(owners[index])
        for triangle in indices:
            assert len({names[i] for i in triangle}) == 1, 'LOD joined distinct parts'
            stats[names[triangle[0]]]['triangles'][lod] += 1
            stats[names[triangle[0]]]['geometry_bytes'] += 12
        for i in set(indices.ravel()):
            stats[names[i]]['referenced_vertices'][lod] += 1
        if lod == 0:
            fps = bool(cooked['materials'][primitive['material']]['triangles'][3])
            for n in names:
                stats[n]['cooked_vertices'] += 1
                stats[n]['geometry_bytes'] += 104*(3+fps)
    scene = bpy.data.scenes.new('H07_Cooked_'+name)
    scene.render.fps = 30
    bpy.context.window.scene = scene
    bpy.ops.import_scene.gltf(filepath=str(WORK/(name+'.gltf')))
    scenes.append(scene)
    rigs.append(next(o for o in scene.objects if o.type=='ARMATURE'))
assert sum(r['geometry_bytes'] for r in stats.values()) == cooked['totals']['geometry_bytes']


def surface(scene, material=None):
    bpy.context.window.scene = scene
    bpy.context.view_layer.update()
    points, faces = [], []
    for obj in scene.objects:
        if obj.type != 'MESH':
            continue
        evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = evaluated.to_mesh()
        mesh.calc_loop_triangles()
        offset = len(points)
        points.extend(evaluated.matrix_world@v.co for v in mesh.vertices)
        faces.extend([offset+i for i in t.vertices] for t in mesh.loop_triangles if material is None or t.material_index==material)
        evaluated.to_mesh_clear()
    used = sorted({i for f in faces for i in f})
    return BVHTree.FromPolygons(points, faces), [points[i] for i in used]+[sum((points[i] for i in f),Vector())/3 for f in faces[::8]]


def pose(scene, rig, kind, amount=1):
    assembly.pose(scene, rig, 'rest')
    if kind in ('fist','flex','extend'):
        hands.pose(scene, rig, kind, amount)
    elif kind in ('elbow','look-up','look-down'):
        anatomy.pose(scene, rig, kind, amount)
    else:
        assembly.pose(scene, rig, kind, amount)


if '--transitions-only' not in sys.argv:
    errors = []
    for frame in range(0,181,3):
        surfaces = []
        for scene in scenes[:3]:
            scene.frame_set(frame)
            surfaces.append(surface(scene))
        row = []
        for lod in (1,2):
            row.append(max(
                max(surfaces[lod][0].find_nearest(p)[3] for p in surfaces[0][1]),
                max(surfaces[0][0].find_nearest(p)[3] for p in surfaces[lod][1])))
        errors.append({'frame':frame,'surface_errors_m':row})
        if frame%30 == 0:
            print('H07_LOD_FRAME',frame,row,flush=True)


    for rig in rigs:
        rig.animation_data.action = None
    extreme = []
    for kind in ('reach','twist-left','twist-right','step-left','step-right','hip-left','hip-right','head-left','head-right',
                 'fist','flex','extend','elbow','look-up','look-down'):
        for amount in (.5,1):
            for scene,rig in zip(scenes,rigs):
                pose(scene,rig,kind,amount)
            surfaces = [surface(s) for s in scenes[:3]]
            extreme.append({'pose':kind,'amount':amount,'surface_errors_m':[
                max(max(surfaces[lod][0].find_nearest(p)[3] for p in surfaces[0][1]),
                    max(surfaces[0][0].find_nearest(p)[3] for p in surfaces[lod][1])) for lod in (1,2)]})
    transitions = []
    for kind in ('rest','reach','elbow','look-down','twist-left','fist','step-left'):
        for scene,rig in zip(scenes,rigs):
            pose(scene,rig,kind)
        for material,row in enumerate(cooked['materials']):
            surfaces = [surface(s,material) for s in scenes[:3]]
            pixels = []
            for lod,threshold in ((1,30),(2,80)):
                error = max(max(surfaces[lod][0].find_nearest(p)[3] for p in surfaces[lod-1][1]),
                            max(surfaces[lod-1][0].find_nearest(p)[3] for p in surfaces[lod][1]))
                pixels.append(error*1080/(2*math.tan(math.pi/8)*row['radius_m']*(threshold-1)))
            transitions.append({'pose':kind,'material':row['material'],'projected_error_px_1080p':pixels})
    print('H07_TRANSITION_MAX', max(max(r['projected_error_px_1080p']) for r in transitions), flush=True)
    assert max(max(r['projected_error_px_1080p']) for r in transitions)<2, 'Re-review transitions above two sampled pixels'
    (WORK/'lod-verification.json').write_text(json.dumps({'components':stats,'clip_samples':errors,'extreme_samples':extreme,
        'transitions':transitions},indent=2)+'\n')


    def snapshot(scene, board, transform):
        bpy.context.window.scene = scene
        bpy.context.view_layer.update()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        for obj in scene.objects:
            if obj.type == 'MESH':
                evaluated = obj.evaluated_get(depsgraph)
                copy = bpy.data.objects.new('H07_Snapshot', bpy.data.meshes.new_from_object(evaluated, depsgraph=depsgraph))
                copy.matrix_world = transform@obj.matrix_world
                board.collection.objects.link(copy)


    board = bpy.data.scenes.new('H07_CookedTurnaround')
    hands.setup(board,2400,2600)
    for row,scene in enumerate(scenes[:3]):
        pose(scene,rigs[row],'rest')
        for col,angle in enumerate((0,math.pi/2,math.pi,.65)):
            x,z = (col-1.5)*1.25,(2-row)*2.05
            snapshot(scene,board,Matrix.Translation((x,0,z))@Matrix.Rotation(angle,4,'Z'))
            hands.label(board,f'LOD{row}',x,z-.09,.04)
    hands.camera(board,(0,-8,2.93),(0,0,2.93),6.4)
    hands.render(board,OUT/'lod-turnaround.png')
    for kind in ('reach','elbow','look-down','fist','twist-left','step-left'):
        board = bpy.data.scenes.new('H07_'+kind)
        hands.setup(board,1800,1000)
        for i,(scene,rig) in enumerate(zip(scenes[:3],rigs[:3])):
            pose(scene,rig,kind)
            snapshot(scene,board,Matrix.Translation(((i-1)*1.4,0,0))@Matrix.Rotation(.6,4,'Z'))
            hands.label(board,'LOD'+str(i), (i-1)*1.4,-.1,.06)
        hands.camera(board,(0,-6,.91),(0,0,.91),4.5)
        hands.render(board,OUT/(kind+'.png'))
    for i,(scene,rig) in enumerate(zip(scenes[:3],rigs[:3])):
        pose(scene,rig,'rest')
        hands.setup(scene,1000,1100)
        cam = hands.camera(scene,(0,-5,.91),(0,0,.91),2.1)
        scene.render.film_transparent = True
        scene.display.shading.color_type = 'SINGLE'
        scene.display.shading.single_color = (0,0,0)
        scene.display.shading.light,scene.display.shading.show_cavity = 'FLAT',False
        for view,position in (('front',(0,-5,.91)),('side',(5,0,.91)),('back',(0,5,.91))):
            cam.location = position
            cam.rotation_euler = (Vector((0,0,.91))-cam.location).to_track_quat('-Z','Y').to_euler()
            hands.render(scene,OUT/f'lod{i}-{view}.png')
    hands.setup(scenes[3],1600,1000)
    hands.camera(scenes[3],(1,-4,1.6),(0,0,1.15),1.7)
    for kind in ('rest','reach','fist','flex'):
        pose(scenes[3],rigs[3],kind)
        hands.render(scenes[3],OUT/f'fps-{kind}.png')
    print('H07_LODS_REVIEWED',flush=True)

# Mixed LOD selection immediately around each authored radius threshold, at native 1080p.
for rig in rigs:
    rig.animation_data.action = None
for scene,rig in zip(scenes,rigs):
    pose(scene,rig,'rest')
for material,row in enumerate(cooked['materials']):
    center = Vector((row['center'][0],-row['center'][2],row['center'][1]))
    for kind in (('rest','look-down') if material==5 else ('rest','fist') if material==4 else ('rest',)):
        for scene,rig in zip(scenes,rigs):
            pose(scene,rig,kind)
        for threshold in (30,80):
            for sign in (-1,1):
                eye = center+Vector((0,-threshold*row['radius_m']*(1+sign*.02),0))
                board = bpy.data.scenes.new('H07_Transition')
                hands.setup(board,1920,1080)
                for index,other in enumerate(cooked['materials']):
                    other_center = Vector((other['center'][0],-other['center'][2],other['center'][1]))
                    ratio = (eye-other_center).length/other['radius_m']
                    scene = scenes[2 if ratio>80 else 1 if ratio>30 else 0]
                    bpy.context.window.scene = scene
                    for obj in scene.objects:
                        if obj.type != 'MESH':
                            continue
                        evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
                        mesh = evaluated.to_mesh()
                        faces = [f for f in mesh.polygons if f.material_index==index]
                        data = bpy.data.meshes.new('H07_TransitionPart')
                        data.from_pydata([evaluated.matrix_world@v.co for v in mesh.vertices],[],[list(f.vertices) for f in faces])
                        data.materials.append(obj.data.materials[index])
                        for f in data.polygons:
                            f.use_smooth = True
                        data.normals_split_custom_set([mesh.corner_normals[i].vector for f in faces for i in f.loop_indices])
                        board.collection.objects.link(bpy.data.objects.new(data.name,data))
                        evaluated.to_mesh_clear()
                cam = hands.camera(board,eye,center,2.1)
                cam.data.type,cam.data.sensor_fit = 'PERSP','VERTICAL'
                cam.data.lens = cam.data.sensor_height/(2*math.tan(math.pi/8))
                hands.render(board,OUT/f'transition-{material}-{threshold}-{sign}-{kind}.png')
                bpy.data.scenes.remove(board)
print('H07_TRANSITIONS_RENDERED',flush=True)
