'''H05 F01-F06: revise the sculpture from the immutable v13 source, keeping the diagnostic skeleton and keys.'''
import bpy
import bmesh
import json
import math
import sys
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).resolve().parent))
from hound_h04_review import parts as collect_parts

root = Path(__file__).resolve().parents[2]
assert bpy.data.filepath.replace('\\','/').endswith('/v13/hound-sculpture-v13.blend'), 'Start from the exact v13 source'
source = bpy.data.scenes['Hound_Mesh_v12']
bpy.context.window.scene = source
source.frame_set(1)
bpy.context.view_layer.update()
original,old_rig = source.objects['H12_DeformMesh'],source.objects['Hound12_Rig']
scene = bpy.data.scenes.new('Hound_Mesh_v14')
for key in source.keys():
    scene[key] = source[key]
scene.unit_settings.system,scene.unit_settings.scale_length = 'METRIC',1
scene.render.fps,scene.frame_start,scene.frame_end = 30,1,181
for marker in source.timeline_markers:
    scene.timeline_markers.new(marker.name,frame=marker.frame)
collection = bpy.data.collections.new('HOUND_v14_EXPORT')
scene.collection.children.link(collection)
rig = old_rig.copy()
rig.data = old_rig.data.copy()
rig.name,rig.data.name = 'Hound14_Rig','Hound14_Skeleton'
rig.animation_data.action = old_rig.animation_data.action.copy()
rig.animation_data.action.name = 'Hound14_joint_check'
collection.objects.link(rig)
model = original.copy()
model.data = original.data.copy()
model.name,model.data.name = 'H14_DeformMesh','H14_DeformTopology'
model.parent,model.modifiers[0].object = rig,rig
collection.objects.link(model)
for slot in model.material_slots:
    slot.material = slot.material.copy()
    slot.material.name = slot.material.name.replace('Hound12_','Hound14_')
bpy.context.window.scene = scene
parts = collect_parts(model)
rigid = json.loads(source['gloom_rigid_parts'])
rebuilt,morphed,added,generated = set(),set(),set(),[]
materials = {n:next(i for i,m in enumerate(model.data.materials) if n in m.name) for n in ('Iron','EdgePlanes','Undersuit')}


def smooth(value):
    t = max(0,min(1,value))
    return t*t*(3-2*t)


def surface(name,vertices,faces,accents=None,bone=None,smooth_faces=True):
    data = bpy.data.meshes.new('H14_'+name)
    data.from_pydata(vertices,[],faces)
    for mat in model.data.materials:
        data.materials.append(mat)
    for polygon in data.polygons:
        polygon.material_index = materials['EdgePlanes' if accents and accents[polygon.index] else 'Iron']
        polygon.use_smooth = smooth_faces
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    assert all(e.is_manifold and e.is_contiguous for e in bm.edges),name
    if bm.calc_volume(signed=True)<0:
        bmesh.ops.reverse_faces(bm,faces=list(bm.faces))
    assert bm.calc_volume(signed=True)>0,name
    bm.to_mesh(data)
    bm.free()
    data.set_sharp_from_angle(angle=math.radians(46))
    obj = bpy.data.objects.new(data.name,data)
    collection.objects.link(obj)
    obj.vertex_groups.new(name='PART_'+name).add(list(range(len(vertices))),1,'REPLACE')
    rigid[name] = bone or rigid[name]
    obj.vertex_groups.new(name=rigid[name]).add(list(range(len(vertices))),1,'REPLACE')
    generated.append(obj)
    (rebuilt if name in parts else added).add(name)
    return obj


def bridge(faces,a,b,n):
    faces.extend((a+i,a+(i+1)%n,b+(i+1)%n,b+i) for i in range(n))


def curved_border(outline,steps=4):
    points = [Vector(p) for p in outline]
    border = []
    for i,p1 in enumerate(points):
        p0,p2,p3 = points[i-1],points[(i+1)%len(points)],points[(i+2)%len(points)]
        for j in range(steps):
            t = j/steps
            border.append(.5*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t*t+(-p0+3*p1-3*p2+p3)*t*t*t))
    return border


def panel(name,outline,depth_function,bulge=.004,bone=None):
    # A thin curved plate: the return duplicates its field, instead of leaving a deep solid wedge.
    border = curved_border([(x,0,z) for x,z in outline])
    center = sum(border,Vector())/len(border)
    vertices,faces,accents = [],[],[]
    radii = (1,.975,.91,.68,.39,.13)
    for r in radii:
        for point in border:
            p = center+(point-center)*r
            p.y = depth_function(p.x,p.z)-bulge*(1-r*r)
            vertices.append(p)
    p = center.copy()
    p.y = depth_function(p.x,p.z)-bulge
    vertices.append(p)
    n = len(border)
    for row in range(len(radii)-1):
        bridge(faces,row*n,(row+1)*n,n)
        accents.extend([row==0]*n)
    for i in range(n):
        faces.append(((len(radii)-1)*n+i,(len(radii)-1)*n+(i+1)%n,len(vertices)-1))
        accents.append(False)
    layer = len(vertices)
    vertices += [p+Vector((0,.004,0)) for p in list(vertices)]
    faces += [tuple(i+layer for i in f) for f in list(faces)]
    accents += list(accents)
    bridge(faces,0,layer,n)
    accents.extend([False]*n)
    return surface(name,vertices,faces,accents,bone)


def blade(name,outline,normal,thickness,bone=None):
    normal = Vector(normal).normalized()
    points = [Vector(p) for p in outline]
    n = len(points)
    center = sum(points,Vector())/n
    vertices = [p-normal*thickness*.25 for p in points]+[center-normal*thickness]
    vertices += [p+normal*thickness*.25 for p in points]+[center+normal*thickness]
    faces = []
    for i in range(n):
        faces += [(i,(i+1)%n,n),(i+n+1,2*n+1,(i+1)%n+n+1)]
        faces.append((i,i+n+1,(i+1)%n+n+1,(i+1)%n))
    return surface(name,vertices,faces,bone=bone,smooth_faces=False)


# F05: lower, fitted collar bands and flatter breastplates; leave the deltoid contour and hood untouched.
for side,suffix in ((1,'L'),(-1,'R')):
    outline = [(side*x,z) for x,z in ((.014,1.468),(.080,1.462),(.150,1.478),(.215,1.459),
        (.236,1.421),(.222,1.381),(.157,1.351),(.086,1.365),(.018,1.345))]
    def chest_depth(x,z):
        return -.162+.055*(abs(x)/.244)**2+.006*((z-1.41)/.10)**2
    panel('Breastplate_'+suffix,outline,chest_depth,.0035)
    vertices,faces = [],[]
    profile = [(.172,-.151,1.443),(.182,-.120,1.470),(.191,-.076,1.494),
               (.190,-.014,1.509),(.182,.047,1.493),(.169,.112,1.460)]
    for cx,y,z in profile:
        for dx,dz in ((-.023,0),(-.019,.004),(.019,.004),(.024,0),(.019,-.008),(-.019,-.008)):
            vertices.append((side*(cx+dx-.025),y,z+dz))
    for i in range(len(profile)-1):
        bridge(faces,i*6,(i+1)*6,6)
    faces += [tuple(range(6)),tuple(range(len(vertices)-6,len(vertices)))]
    surface('Collar_plate_'+suffix,vertices,faces)
    name = 'Collar_blade_middle_'+suffix
    for index in parts[name]:
        p = model.data.vertices[index].co
        t = 1-smooth((p.z-1.495)/.10)
        p.x -= side*.040*t
        p.y -= .020*t
        p.z -= .007*t
    morphed.add(name)
    # One independent rear blade per side, rooted on the upper back behind the clavicular fan.
    outline = [(side*x,y,z) for x,y,z in ((.158,.136,1.449),(.214,.136,1.466),(.243,.156,1.572),
        (.282,.194,1.746),(.223,.167,1.640),(.199,.141,1.564),(.173,.127,1.524))]
    blade('Back_blade_'+suffix,outline,(0,1,0),.009,'Bip001 Spine2')


def sternum_depth(x,z):
    return -.165-.005*(1-abs(x)/.030)
panel('Sternum',[(-.026,1.497),(.026,1.497),(.027,1.391),(0,1.359),(-.027,1.391)],sternum_depth,.002)

# F06: unequal, scalloped abdominal lamellae wrap into an articulated lateral chain.
outlines = [
    [(-.160,1.316),(-.139,1.359),(-.066,1.374),(0,1.357),(.066,1.374),(.139,1.359),
     (.160,1.316),(.110,1.275),(.056,1.265),(0,1.274),(-.056,1.265),(-.110,1.275)],
    [(-.137,1.219),(-.113,1.283),(-.050,1.295),(0,1.280),(.050,1.295),(.113,1.283),
     (.137,1.219),(.095,1.181),(.046,1.168),(0,1.178),(-.046,1.168),(-.095,1.181)],
    [(-.113,1.122),(-.093,1.186),(-.040,1.197),(0,1.182),(.040,1.197),(.093,1.186),
     (.113,1.122),(.076,1.081),(.032,1.075),(0,1.084),(-.032,1.075),(-.076,1.081)]]
for index,outline in enumerate(outlines):
    def abdomen_depth(x,z):
        center = -.166+index*.006
        return center+.077*(x/.21)**2+.002*math.exp(-(x/.032)**2)+.008*((z-(1.31-index*.095))/.065)**2
    panel('Abdominal_plate_'+str(index+1),outline,abdomen_depth,.004 if index==0 else .003)

for side,suffix in ((1,'L'),(-1,'R')):
    for row,(z,width) in enumerate(((1.308,.216),(1.222,.202),(1.140,.186))):
        outline = [(side*x,z+dz) for x,dz in ((.100,.034),(width-.008,.059),(width,.025),
            (width-.006,-.018),(.105,-.048),(.092,-.012))]
        def rib_depth(x,z):
            return -.144+row*.005+.069*(abs(x)/.228)**2
        name = 'Rib_flank_'+suffix if row==0 else 'Rib_lamella_'+str(row+1)+'_'+suffix
        panel(name,outline,rib_depth,.0025,'Bip001 Spine2' if row==0 else 'Bip001 Spine1' if row==1 else 'Bip001 Spine')

# F01: three separated recurved fins belong to the bracer, independent of the hand point.
for side,suffix in ((1,'L'),(-1,'R')):
    for row,(x,z,length,height,y) in enumerate(((.396,1.167,.091,.158,-.010),
                                               (.443,1.080,.102,.148,-.028),(.491,1.050,.083,.100,-.045))):
        outline = [(side*(x+dx),y+dy,z+dz) for dx,dy,dz in (
            (-.023,0,-.023),(.024,0,-.006),(length*.66,0,height*.33),
            (length,.003,height),(.043,.002,height*.49),(.004,0,.049))]
        name = ('Bracer_blade_upper_' if row==0 else 'Bracer_blade_lower_' if row==1 else 'Bracer_blade_distal_')+suffix
        blade(name,outline,(0,1,0),.006,'Bip001 '+suffix+' Forearm')

# F02: replace only terminal finger armor with hooked, pointed metal claws over the unchanged articulated glove.
for side,suffix in ((1,'L'),(-1,'R')):
    down = Vector((side*.16,-.08,-1)).normalized()
    front = Vector((0,-1,0))
    front = (front-down*front.dot(down)).normalized()
    dorsal = front.cross(down).normalized()*side
    for name in sorted(n for n in parts if n.startswith('Finger_'+suffix) and n.endswith('C') or n=='Thumb_'+suffix+'B'):
        bone = rig.data.bones[rigid[name]]
        tangent = (bone.tail_local-bone.head_local).normalized()
        across = down if name.startswith('Thumb') else front
        across = (across-tangent*across.dot(tangent)).normalized()
        outward = across.cross(tangent).normalized()
        if outward.dot(dorsal)<0:
            outward = -outward
        radius = .0135 if name.startswith('Thumb') else .011
        vertices,faces = [],[]
        length = bone.length
        for t,width,thick,hook in ((.08,1,1,0),(.22,1.06,1.08,0),(.62,.92,1,.001),
                                  (.95,.69,.78,.003),(1.28,.42,.51,.006),(1.63,.14,.18,.011)):
            center = bone.head_local+tangent*(length*t)-outward*hook
            for i in range(12):
                a = math.tau*i/12
                vertices.append(center+across*(math.sin(a)*radius*width)+outward*(math.cos(a)*radius*thick))
        for row in range(5):
            bridge(faces,row*12,(row+1)*12,12)
        faces.append(tuple(range(12)))
        tip = len(vertices)
        vertices.append(bone.head_local+tangent*(length*1.82)-outward*.014)
        for i in range(12):
            faces.append((60+i,60+(i+1)%12,tip))
        surface(name,vertices,faces,smooth_faces=True)

# F03: overlap the boot with the calf shell and use continuous shin/instep plating, still articulated at the ankle.
for side,suffix in ((1,'L'),(-1,'R')):
    name = 'Greave_mass_'+suffix
    for index in parts[name]:
        p = model.data.vertices[index].co
        t = 1-smooth((p.z-.184)/.245)
        front_factor = max(0,min(1,(-p.y+.01)/.10))
        radial = Vector((p.x-side*.197,p.y-.012,0)).normalized()
        p += radial*.009*t
        p.z -= (.064+.025*front_factor)*t
    morphed.add(name)
    def shin_depth(x,z):
        t = max(0,min(1,(z-.13)/.38))
        cx,cy,rx,ry = .197-.020*t,.012-.012*t,.074+.017*math.sin(math.pi*t*.8),.091+.018*math.sin(math.pi*t)
        u = max(-.97,min(.97,(abs(x)-cx)/rx))
        return cy-ry*math.sqrt(1-u*u)-.011-.019*smooth((z-.47)/.10)
    center = .185
    outline = [(side*(center+x),z) for x,z in (
        (-.072,.524),(-.056,.573),(0,.587),(.066,.542),(.077,.475),(.063,.349),
        (.055,.235),(.041,.143),(0,.125),(-.042,.152),(-.061,.299),(-.078,.435))]
    panel('Greave_front_'+suffix,outline,shin_depth,.004)
    outline = [(side*(.199+x),z) for x,z in ((-.051,.330),(.048,.327),(.042,.248),(.029,.157),(0,.132),(-.039,.179))]
    def lower_depth(x,z):
        return shin_depth(x,z)-.009
    panel('Shin_lower_'+suffix,outline,lower_depth,.002)
    outline = [(side*(.177+x),z) for x,z in ((-.074,.596),(-.060,.650),(-.010,.641),
        (.050,.668),(.077,.602),(.041,.558),(0,.538),(-.049,.563))]
    def knee_depth(x,z):
        return -.125+.041*((abs(x)-.177)/.090)**2+.011*((z-.59)/.075)**2
    panel('Knee_shield_'+suffix,outline,knee_depth,.004)
    outline = [(side*(.197+x),z) for x,z in ((-.047,.202),(.047,.202),(.053,.157),(.069,.082),
        (.051,.048),(-.052,.048),(-.071,.085),(-.058,.137))]
    def instep_depth(x,z):
        t = smooth((z-.055)/.153)
        return -.224+.143*t+.025*((abs(x)-.197)/.080)**2
    panel('Ankle_guard_'+suffix,outline,instep_depth,.003)

# Keep all unaffected geometry and weights exactly; replace only named components and join the new surfaces.
bm = bmesh.new()
bm.from_mesh(model.data)
bm.verts.ensure_lookup_table()
removed = {i for name in rebuilt for i in parts[name]}
bmesh.ops.delete(bm,geom=[bm.verts[i] for i in removed],context='VERTS')
bm.to_mesh(model.data)
bm.free()
for name in rebuilt:
    model.vertex_groups.remove(model.vertex_groups['PART_'+name])
bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
for obj in generated:
    obj.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.object.join()
scene['gloom_v14_rebuilt_parts'] = json.dumps(sorted(rebuilt))
scene['gloom_v14_morphed_parts'] = json.dumps(sorted(morphed))
scene['gloom_v14_added_parts'] = json.dumps(sorted(added))
scene['gloom_rigid_parts'] = json.dumps(rigid,sort_keys=True)
scene['gloom_authoring_stage'] = 'H05-F01-F06-candidate-awaiting-artistic-acceptance'
scene['gloom_revision'] = 'Three bracer fins, hooked claws, continuous boot-greave armor, two rear blades, lowered collar/chest and wrapping abdomen'
scene['gloom_limitations'] = 'Diagnostic rig unchanged; global contacts and weapon attachment must be reviewed; no final topology, textures or animation'
scene.frame_set(1)
bpy.context.view_layer.update()
model.data.calc_loop_triangles()
draft = root/'.cache/hound-mesh-v14'
draft.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(draft/'hound-draft.blend'),check_existing=False,compress=True)
print('H14_DRAFT',len(model.data.vertices),len(model.data.loop_triangles),len(rebuilt),len(morphed),len(added))
