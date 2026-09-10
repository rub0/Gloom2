'''Check the requested v02 edits and the unchanged body against the preserved v01 scene.'''
import bpy
from mathutils import Vector

model = bpy.data.collections['HOUND_v02_MODEL_ONLY']
previous = bpy.data.collections['HOUND_v01_MODEL_ONLY']
changed = ('Pauldron_','Clavicle_','Shoulder_blade_','Hood_','Palm_','Hand_back_','Finger_','Thumb_')
unchanged = 0
for original in previous.objects:
    part = original.name[4:]
    if part.startswith(changed):
        continue
    revised = model.objects['H02_'+part]
    assert len(original.data.vertices) == len(revised.data.vertices), 'Unexpected topology change: '+part
    for a,b in zip(original.data.vertices,revised.data.vertices):
        assert (a.co-b.co).length < 1e-7, 'Unexpected geometry change: '+part
    # Compare stored local transforms: an inactive scene's evaluated matrix_world may still be identity after loading.
    assert original.parent is None and revised.parent is None, 'Unexpected parenting'
    assert (original.location-revised.location).length < 1e-6 and (original.scale-revised.scale).length < 1e-6, 'Changed transform'
    rotation_a = original.rotation_quaternion if original.rotation_mode == 'QUATERNION' else original.rotation_euler.to_quaternion()
    rotation_b = revised.rotation_quaternion if revised.rotation_mode == 'QUATERNION' else revised.rotation_euler.to_quaternion()
    assert abs(rotation_a.dot(rotation_b)) > 1-1e-6, 'Changed rotation'
    assert [tuple(p.vertices) for p in original.data.polygons] == [tuple(p.vertices) for p in revised.data.polygons], 'Changed faces'
    assert original.data.materials[0].diffuse_color[:] == revised.data.materials[0].diffuse_color[:], 'Unexpected color change'
    unchanged += 1

rim = model.objects['H02_Hood_opening_rim']
assert max(v.co.z for v in list(rim.data.vertices)[12:]) < 1.73, 'Hood inner crown must cover more forehead'
assert all(v.co.y < -.18 for v in list(rim.data.vertices)[12:]), 'Hood lip must advance over the face'
for side,suffix in [(1,'L'),(-1,'R')]:
    assert model.objects.get('H02_Pauldron_'+suffix) is None, 'Shoulder cup should be removed'
    assert model.objects.get('H02_Pauldron_front_'+suffix) is None, 'Old shoulder plate should be removed'
    collar = model.objects['H02_Collar_plate_'+suffix]
    assert max(abs(v.co.x) for v in collar.data.vertices) < .28, 'Collar must stay medial to the deltoid'
    assert 'AshSkin' in model.objects['H02_Deltoid_'+suffix].data.materials[0].name, 'Exposed shoulder needs skin'

    wrist = Vector((side*.454,-.036,.961))
    down = Vector((side*.16,-.08,-1)).normalized()
    front = Vector((0,-1,0))
    front = (front-down*front.dot(down)).normalized()
    dorsal = front.cross(down).normalized()*side
    palm = model.objects['H02_Palm_'+suffix]
    cover = model.objects['H02_Hand_back_full_point_'+suffix]
    palm_points = [palm.matrix_world @ v.co-wrist for v in palm.data.vertices]
    cover_points = [cover.matrix_world @ v.co-wrist for v in cover.data.vertices]
    assert min(p.dot(dorsal) for p in cover_points) > .02-1e-6, 'Back plate must be outside, not on the palm'
    assert max(p.dot(down) for p in cover_points) > max(p.dot(down) for p in palm_points)+.06, 'Plate must end beyond the palm'
    assert max(p.dot(front) for p in cover_points) >= max(p.dot(front) for p in palm_points), 'Dorsal plate too narrow'
    assert min(p.dot(front) for p in cover_points) <= min(p.dot(front) for p in palm_points), 'Dorsal plate too narrow'
    for digit in range(4):
        tip = model.objects['H02_Finger_'+suffix+'_'+str(digit)+'C']
        end = sum((tip.matrix_world @ v.co-wrist for v in list(tip.data.vertices)[16:]),Vector())/8
        assert end.dot(dorsal) < -.045, 'Finger must curl toward palm, away from dorsal plate'
    thumb = model.objects['H02_Thumb_'+suffix+'B']
    end = sum((thumb.matrix_world @ v.co-wrist for v in list(thumb.data.vertices)[16:]),Vector())/8
    assert end.dot(front) > .05, 'Thumb must point anteriorly'
assert unchanged == 67, 'Unexpected unchanged-part count'
print('HOUND_V02_CHANGES_VERIFIED: 67 body parts unchanged; clavicle armor, deeper hood and both hand frames/pointed covers checked')
