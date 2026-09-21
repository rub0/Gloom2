'''Save/export the reviewed H05 draft once; no changes to earlier sources.'''
import bpy
import sys
import json
import hashlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import hound_h01_review as review
root = Path(__file__).resolve().parents[2]
scene = bpy.data.scenes['Hound_Mesh_v14']
model,rig = scene.objects['H14_DeformMesh'],scene.objects['Hound14_Rig']
bpy.context.window.scene = scene
scene.frame_set(1)
review.setup(scene,1000,1100)
review.camera(scene,(2.8,-5,2.2),(0,0,.91),2.1)
paths = [root/'art/characters/hound/v14/hound-mesh-v14.blend',root/'assets/characters/hound_rig/v14/hound-rig.gltf',
         root/'assets/characters/hound_rig/v14/hound-rig.bin']
assert all(not p.exists() for p in paths), 'Preserve published candidate'
for path in paths:
    path.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
rig.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.wm.save_as_mainfile(filepath=str(paths[0]),check_existing=False,compress=True)
bpy.ops.export_scene.gltf(filepath=str(paths[1]),export_format='GLTF_SEPARATE',use_selection=True,use_active_scene=True,
    export_yup=True,export_apply=False,export_animations=True,export_animation_mode='ACTIVE_ACTIONS',
    export_nla_strips_merged_animation_name='Hound14_joint_check',export_anim_slide_to_zero=True,export_force_sampling=True,
    export_frame_range=True,export_skins=True,export_def_bones=True,export_rest_position_armature=True,
    export_all_influences=False,export_influence_nb=4,export_extras=True)
manifest = {'task':'H05','milestone':97,'candidate':'v14','source':'v13','artistic_approval':False,
            'scene':scene.name,'mesh':model.name,'rig':rig.name,'action':rig.animation_data.action.name,
            'collection':'HOUND_v14_EXPORT','feedback':['F01','F02','F03','F04','F05','F06'],
            'files':[{'path':p.relative_to(root).as_posix(),'bytes':p.stat().st_size,
                      'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in paths]}
(paths[0].parent/'sculpture-reference.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print('H14_SAVED_EXPORTED',json.dumps(manifest))
