'''Editable Hound blockout v01. Run in Blender via MCP; no final topology or rig.'''
import bpy
import bmesh
import math
from mathutils import Matrix, Vector

scene_name = 'Hound_Blockout_v01'
existing = bpy.data.scenes.get(scene_name)
if existing:
    raise RuntimeError('Hound v01 already exists; revise the existing scene rather than overwrite it')
scene = bpy.data.scenes.new(scene_name)
scene['gloom_authoring_stage'] = 'blockout-v01-unapproved'
scene['gloom_reference'] = 'docs/art/hound/hound-concept-v01.png'
scene['gloom_forward'] = '-Y Blender / +Z glTF'
bpy.context.window.scene = scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
model = bpy.data.collections.new('HOUND_v01_MODEL_ONLY')
scene.collection.children.link(model)

def material(name, color, metal=0, rough=0.7):
    mat = bpy.data.materials.new('Hound01_' + name)
    mat.diffuse_color = (*color, 1)
    mat.use_nodes = True
    node = mat.node_tree.nodes.get('Principled BSDF')
    node.inputs['Base Color'].default_value = (*color, 1)
    node.inputs['Metallic'].default_value = metal
    node.inputs['Roughness'].default_value = rough
    mat.use_backface_culling = True
    return mat

iron = material('Iron', (0.115, 0.125, 0.135), 0.65, 0.44)
edge = material('EdgePlanes', (0.22, 0.225, 0.225), 0.7, 0.4)
cloth = material('Hood', (0.035, 0.032, 0.033))
under = material('Undersuit', (0.055, 0.047, 0.044))
skin = material('AshSkin', (0.46, 0.405, 0.34), 0, 0.78)
trouser = material('BurgundyCloth', (0.145, 0.072, 0.052), 0, 0.88)
eye = material('EyeAccent', (1.0, 0.22, 0.015), 0, 0.35)
eye.node_tree.nodes.get('Principled BSDF').inputs['Emission Color'].default_value = (1.0, 0.16, 0.01, 1)
eye.node_tree.nodes.get('Principled BSDF').inputs['Emission Strength'].default_value = 1.5

def mesh(name, vertices, faces, mat, bevel=0):
    data = bpy.data.meshes.new('H01_' + name)
    data.from_pydata(vertices, [], faces)
    data.update()
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(data)
    bm.free()
    obj = bpy.data.objects.new('H01_' + name, data)
    model.objects.link(obj)
    obj.data.materials.append(mat)
    if bevel:
        mod = obj.modifiers.new('Editable edge chamfer', 'BEVEL')
        mod.width = bevel
        mod.segments = 1
    return obj

def loft(name, rings, mat, sides=12):
    # Each ring is (x, y, z, radius_x, radius_y). Deliberately faceted volume.
    vertices = []
    for x, y, z, rx, ry in rings:
        for i in range(sides):
            angle = math.tau * i / sides
            vertices.append((x + rx * math.cos(angle), y + ry * math.sin(angle), z))
    faces = [tuple(reversed(range(sides)))]
    for j in range(len(rings) - 1):
        for i in range(sides):
            faces.append((j*sides+i, j*sides+(i+1)%sides, (j+1)*sides+(i+1)%sides, (j+1)*sides+i))
    faces.append(tuple((len(rings)-1)*sides+i for i in range(sides)))
    return mesh(name, vertices, faces, mat)

def limb(name, start, end, width, depth, mat, profile=None):
    a = Vector(start)
    direction = Vector(end) - a
    rotation = direction.to_track_quat('Z', 'Y')
    if profile is None:
        profile = [(0, 0.62), (0.18, 0.98), (0.5, 1), (0.82, 0.8), (1, 0.56)]
    obj = loft(name, [(0, 0, t * direction.length, width*r, depth*r) for t, r in profile], mat)
    obj.rotation_mode = 'QUATERNION'
    obj.rotation_quaternion = rotation
    obj.location = a
    return obj

def plate(name, polygon, front_y, thickness, mat=iron, bevel=0.004):
    # Solid X/Z silhouette with a shallow center ridge, not a decal.
    count = len(polygon)
    vertices = [(x, front_y, z) for x, z in polygon]
    vertices += [(x, front_y+thickness, z) for x, z in polygon]
    vertices.append((sum(p[0] for p in polygon)/count, front_y-0.018, sum(p[1] for p in polygon)/count))
    faces = [(i, (i+1)%count, count*2) for i in range(count)]
    faces += [tuple(range(count, count*2))]
    faces += [(i, count+i, count+(i+1)%count, (i+1)%count) for i in range(count)]
    return mesh(name, vertices, faces, mat, bevel)

def mirrored_plate(name, side, polygon, front_y, thickness, mat=iron, bevel=0.004):
    return plate(name, [(side*x, z) for x, z in polygon], front_y, thickness, mat, bevel)

# Torso / waist: broad chest, narrow waist, distinct cloth silhouette.
loft('Torso_underlayer', [(0,0,1.00,.162,.104), (0,0,1.13,.18,.11), (0,0,1.30,.233,.14),
                         (0,0,1.43,.25,.137), (0,0,1.49,.195,.095)], under, 16)
loft('Pelvis_cloth', [(0,0,.865,.15,.085), (0,0,.97,.197,.113), (0,0,1.03,.18,.115)], trouser)
loft('Waist_sash', [(0,0,.991,.196,.121), (0,0,1.047,.181,.126), (0,0,1.072,.175,.113)], trouser, 16)
loft('Neck', [(0,.012,1.445,.07,.067), (0,.012,1.566,.068,.068)], skin)
for s, suffix in [(1,'L'),(-1,'R')]:
    mirrored_plate('Breastplate_'+suffix, s, [(.015,1.457),(.16,1.476),(.252,1.395),(.207,1.319),(.125,1.287),(.026,1.334)],
                   -.153,.09)
    mirrored_plate('Clavicle_'+suffix, s, [(.018,1.455),(.068,1.509),(.225,1.483),(.265,1.44),(.169,1.429)],
                   -.135,.064, edge)
    mirrored_plate('Rib_flank_'+suffix, s, [(.168,1.32),(.223,1.34),(.204,1.19),(.157,1.145),(.142,1.218)],
                   -.098,.12)
    mirrored_plate('Hip_guard_'+suffix, s, [(.142,1.01),(.223,.995),(.248,.857),(.208,.758),(.173,.874)],
                   -.079,.135)
    mirrored_plate('Sash_tail_'+suffix, s, [(x,1.014+(z-1.014)*.35) for x,z in [(.012,1.014),(.143,.998),(.132,.83),(.061,.751),(.014,.823)]],
                   -.127,.014,trouser,.002)
for i, z in enumerate([1.275,1.182,1.096]):
    w = .138-i*.01
    plate('Abdominal_plate_'+str(i+1), [(-w*.72,z+.043),(w*.72,z+.043),(w,z-.005),(.077,z-.052),
                                    (0,z-.028),(-.077,z-.052),(-w,z-.005)], -.129,.052)
plate('Sternum', [(-.028,1.505),(.028,1.505),(.043,1.407),(0,1.35),(-.043,1.407)], -.192,.043,edge)
# Back interpretation: broad quiet plates and a short spine, no cape/wings.
for s, suffix in [(1,'L'),(-1,'R')]:
    obj = mirrored_plate('Scapula_'+suffix, s, [(.02,1.455),(.19,1.457),(.237,1.364),(.167,1.279),(.027,1.307)],
                         -.139,.05)
    obj.rotation_euler.z = math.pi
for i,z in enumerate([1.302,1.215,1.131]):
    obj = plate('Back_spine_'+str(i), [(-.058,z+.041),(.058,z+.041),(.071,z-.016),(0,z-.06),(-.071,z-.016)], -.151,.046)
    obj.rotation_euler.z = math.pi

# Hood: an actual open shell around the face, with depth and a thick front rim.
outline = [(0,1.80),(.066,1.765),(.127,1.671),(.14,1.57),(.128,1.476),(.071,1.427),
           (0,1.474),(-.071,1.427),(-.128,1.476),(-.14,1.57),(-.127,1.671),(-.066,1.765)]
vertices = [(x,-.119,z) for x,z in outline]
vertices += [(x*.93,.073,1.622+(z-1.622)*.94) for x,z in outline]
vertices += [(x*.65,.134,1.624+(z-1.622)*.70) for x,z in outline]
faces = []
for r in range(2):
    for i in range(12):
        faces.append((r*12+i,r*12+(i+1)%12,(r+1)*12+(i+1)%12,(r+1)*12+i))
faces.append(tuple(range(24,36)))
hood = mesh('Hood_shell', vertices, faces, cloth)
solid = hood.modifiers.new('Cloth thickness', 'SOLIDIFY')
solid.thickness = .011
rim_inner = [(x*.80,-.128,1.609+(z-1.609)*.85) for x,z in outline]
mesh('Hood_opening_rim', [(x,-.119,z) for x,z in outline]+rim_inner,
     [(i,(i+1)%12,12+(i+1)%12,12+i) for i in range(12)], cloth)

# Head: blocked facial planes, brow and small orange eyes; no sculpted microdetail.
loft('Head_planes', [(0,-.006,1.509,.039,.036),(0,-.018,1.533,.061,.052),(0,-.017,1.585,.071,.070),
                    (0,-.013,1.65,.084,.079),(0,0,1.703,.081,.071),(0,.004,1.74,.047,.045)], skin, 12)
plate('Jaw_plane', [(-.05,1.564),(.05,1.564),(.043,1.524),(0,1.504),(-.043,1.524)], -.070,.035,skin,.001)
plate('Nose_plane', [(-.015,1.66),(.015,1.66),(.026,1.602),(0,1.592),(-.026,1.602)], -.107,.04,skin,.001)
plate('Mouth_shadow', [(-.032,1.565),(.032,1.565),(.026,1.559),(-.026,1.559)], -.095,.002,under,0)
for s,suffix in [(1,'L'),(-1,'R')]:
    mirrored_plate('Cheek_'+suffix,s,[(.027,1.632),(.067,1.637),(.065,1.592),(.04,1.568),(.026,1.592)], -.087,.027,skin,.001)
    mirrored_plate('Eye_'+suffix,s,[(.018,1.65),(.058,1.66),(.054,1.648),(.025,1.642)], -.102,.008,eye,0)
    mirrored_plate('Brow_'+suffix,s,[(.011,1.661),(.061,1.683),(.072,1.67),(.059,1.658),(.023,1.654)], -.108,.028,skin,.001)

for s,suffix in [(1,'L'),(-1,'R')]:
    shoulder = (s*.274,.004,1.437)
    elbow = (s*.372,-.008,1.209)
    wrist = (s*.454,-.036,.961)
    limb('Upper_arm_'+suffix, shoulder, elbow, .088,.087,skin)
    limb('Biceps_'+suffix,(s*.302,-.035,1.414),(s*.364,-.040,1.238),.063,.060,skin)
    limb('Forearm_skin_'+suffix, elbow, wrist,.069,.072,skin)
    limb('Bracer_mass_'+suffix,(s*.395,-.012,1.156),wrist,.088,.080,iron,
         [(0,.98),(.20,1.06),(.80,.79),(1,.71)])
    mirrored_plate('Bracer_face_'+suffix,s,[(.345,1.143),(.405,1.194),(.484,1.061),(.486,.962),(.427,.934),(.390,1.025)], -.112,.047,edge)
    mirrored_plate('Elbow_cuff_'+suffix,s,[(.333,1.215),(.414,1.245),(.443,1.192),(.398,1.148),(.363,1.164)], -.07,.13)
    mirrored_plate('Bracer_blade_upper_'+suffix,s,[(.41,1.14),(.467,1.151),(.488,1.312),(.451,1.226)], -.016,.03)
    mirrored_plate('Bracer_blade_lower_'+suffix,s,[(.453,1.01),(.510,1.04),(.547,1.166),(.496,1.10)], -.047,.03)

    # Shoulder plate volume plus three swept flat blades, not conical spikes.
    loft('Pauldron_'+suffix,[(s*.286,.004,1.408,.092,.111),(s*.292,.004,1.48,.126,.136),
                            (s*.277,.012,1.548,.093,.10)],iron,10)
    mirrored_plate('Pauldron_front_'+suffix,s,[(.191,1.501),(.3,1.565),(.402,1.49),(.366,1.419),(.265,1.423)], -.101,.055,edge)
    mirrored_plate('Shoulder_blade_inner_'+suffix,s,[(.235,1.501),(.288,1.514),(.363,1.763),(.289,1.661)], -.050,.035)
    mirrored_plate('Shoulder_blade_middle_'+suffix,s,[(.286,1.487),(.363,1.498),(.494,1.688),(.373,1.626)], -.014,.037)
    mirrored_plate('Shoulder_blade_outer_'+suffix,s,[(.342,1.467),(.401,1.464),(.575,1.601),(.427,1.552)], .047,.032)

    # Relaxed hands with separated thumb and four articulated placeholder digits.
    palm_end = (s*.477,-.055,.865)
    limb('Palm_'+suffix,wrist,palm_end,.045,.028,under,[(0,.77),(.4,1),(1,.88)])
    mirrored_plate('Hand_back_'+suffix,s,[(.428,.942),(.482,.938),(.511,.886),(.448,.856)], -.088,.03)
    for digit in range(4):
        x = s*(.449 + digit*.018)
        top = .877 + (0.005 if digit in (1,2) else .014)
        bottom = top - (.069 if digit in (1,2) else .056)
        limb('Finger_'+suffix+'_'+str(digit)+'A',(x,-.052,top),(x+s*.006,-.070,top-.032),.010,.010,iron,[(0,1),(1,.9)])
        limb('Finger_'+suffix+'_'+str(digit)+'B',(x+s*.006,-.070,top-.032),(x+s*.003,-.091,bottom),.009,.009,iron,[(0,1),(1,.65)])
    limb('Thumb_'+suffix+'A',(s*.438,-.061,.919),(s*.416,-.084,.89),.015,.015,iron,[(0,1),(1,.8)])
    limb('Thumb_'+suffix+'B',(s*.416,-.084,.89),(s*.424,-.112,.867),.013,.012,iron,[(0,1),(1,.65)])

    # Cloth legs and separately adjustable knee, greave and boot masses.
    hip = (s*.122,.01,.946)
    knee = (s*.173,-.005,.563)
    ankle = (s*.197,.012,.146)
    limb('Thigh_cloth_'+suffix,hip,knee,.108,.108,trouser,[(0,.90),(.23,1.04),(.57,1),(.84,.85),(1,.65)])
    limb('Calf_underlayer_'+suffix,knee,ankle,.078,.080,under)
    loft('Greave_mass_'+suffix,[(s*.197,.016,.131,.064,.075),(s*.192,.016,.274,.074,.091),
                              (s*.181,.007,.425,.086,.095),(s*.177,0,.497,.084,.083)],iron)
    mirrored_plate('Knee_shield_'+suffix,s,[(.101,.601),(.173,.628),(.252,.598),(.249,.537),(.187,.493),(.12,.52)], -.093,.073,edge)
    mirrored_plate('Greave_front_'+suffix,s,[(.113,.483),(.182,.455),(.261,.48),(.248,.311),(.199,.151),(.144,.253)], -.102,.058)
    mirrored_plate('Shin_lower_'+suffix,s,[(.14,.327),(.198,.301),(.26,.346),(.251,.231),(.207,.17),(.157,.223)], -.111,.023,edge)
    mirrored_plate('Thigh_outer_plate_'+suffix,s,[(.216,.881),(.266,.817),(.269,.666),(.218,.615),(.196,.725)], -.057,.098)
    # Shaped boots: broad squared toe, tapering ankle, flat sole at z=0.
    boot = loft('Boot_'+suffix,[(s*.201,-.061,0,.087,.147),(s*.201,-.061,.046,.095,.153),
                               (s*.201,-.039,.103,.088,.129),(s*.197,.008,.157,.065,.082)],iron,10)
    mirrored_plate('Boot_toe_'+suffix,s,[(.12,.074),(.207,.10),(.282,.074),(.267,.029),(.137,.029)], -.196,.048,edge)
    mirrored_plate('Ankle_guard_'+suffix,s,[(.144,.219),(.196,.198),(.252,.222),(.248,.128),(.197,.105),(.145,.136)], -.090,.06)

# All geometry remains named and separate for silhouette/fit edits.
bpy.context.view_layer.update()
for s, suffix in [(1,'L'),(-1,'R')]:
    pivot = Vector((s*.454,-.036,.961))
    transform = Matrix.Translation(pivot) @ Matrix.Scale(1.25,4) @ Matrix.Translation(-pivot)
    for obj in model.objects:
        if obj.name in ['H01_Palm_'+suffix,'H01_Hand_back_'+suffix] or obj.name.startswith('H01_Finger_'+suffix) or obj.name.startswith('H01_Thumb_'+suffix):
            obj.matrix_world = transform @ obj.matrix_world
for obj in model.objects:
    if obj.name.startswith(('H01_Upper_arm','H01_Biceps','H01_Forearm_skin')):
        for polygon in obj.data.polygons:
            polygon.use_smooth = True
bpy.context.view_layer.update()
bpy.ops.object.select_all(action='DESELECT')
for obj in model.objects:
    obj.select_set(True)
bpy.context.view_layer.objects.active = model.objects.get('H01_Torso_underlayer')
print('HOUND_BLOCKOUT_CREATED:', len(model.objects), 'editable mesh parts; intended height 1.80m')
