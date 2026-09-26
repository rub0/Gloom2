"""Disposable Blender fixtures for H06. Run with Blender --background --factory-startup."""
import json
import struct
import zlib
from pathlib import Path
import bpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / ".cache/hound-h06/source"
OUT.mkdir(parents=True, exist_ok=True)
if bpy.data.filepath:
    raise RuntimeError("Use a fresh background Blender process; never an artist file")
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=2, location=(2, -2, 2))
obj = bpy.context.object
obj.name = "H06_MaterialProbe"
for face in obj.data.polygons:
    face.use_smooth = True
uv0 = obj.data.uv_layers.active
uv0.name = "UV0"
uv1 = obj.data.uv_layers.new(name="UV1")
for a, b in zip(uv0.data, uv1.data):
    b.uv = (a.uv.x * 3 + 0.17, a.uv.y * 2)
obj.data.uv_layers.active_index = 0

def png(name, pixel):
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
    data = b"".join(b"\0" + b"".join(bytes(pixel(x, y)) for x in range(32)) for y in range(32))
    path = OUT / (name + ".png")
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">2I5B", 32, 32, 8, 6, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(data)) + chunk(b"IEND", b""))
    return path

paths = {
    "gray": png("gray", lambda x, y: (128, 128, 128, 255)),
    "normal": png("normal", lambda x, y: (128, 210 if x < 16 else 46, 224, 255)),
    "normal_x": png("normal_x", lambda x, y: (210, 128, 224, 255)),
    "normal_y": png("normal_y", lambda x, y: (128, 210, 224, 255)),
    "orm": png("orm", lambda x, y: (32, 64, 255, 255)),
    "checker": png("checker", lambda x, y: (240, 70, 20, 255) if (x // 8 + y // 8) % 2 else (20, 150, 240, 0)),
}
images = {}
for name, path in paths.items():
    images[name] = bpy.data.images.load(str(path), check_existing=False)
    images[name].colorspace_settings.name = "Non-Color" if name in ("normal", "normal_x", "normal_y", "orm") else "sRGB"

linear_gray = ((128 / 255 + 0.055) / 1.055) ** 2.4
variants = ("factor", "color", "normal", "normal_x", "normal_y", "orm", "orm_factor", "ao", "emission", "emission_factor", "uv0", "uv1", "mask", "blend")
for variant in variants:
    mat = bpy.data.materials.new("H06_" + variant)
    mat.use_nodes = True
    mat.use_backface_culling = True
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    shader = nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (linear_gray, linear_gray, linear_gray, 1)
    shader.inputs["Metallic"].default_value = 0
    shader.inputs["Roughness"].default_value = 0.5

    def texture(name, uv="UV0"):
        tex = nodes.new("ShaderNodeTexImage")
        tex.image = images[name]
        coord = nodes.new("ShaderNodeUVMap")
        coord.uv_map = uv
        links.new(coord.outputs["UV"], tex.inputs["Vector"])
        return tex

    if variant == "color":
        links.new(texture("gray").outputs["Color"], shader.inputs["Base Color"])
    if variant in ("normal", "normal_x", "normal_y"):
        normal = nodes.new("ShaderNodeNormalMap")
        links.new(texture(variant).outputs["Color"], normal.inputs["Color"])
        links.new(normal.outputs["Normal"], shader.inputs["Normal"])
    if variant in ("orm", "ao"):
        separate = nodes.new("ShaderNodeSeparateColor")
        links.new(texture("orm").outputs["Color"], separate.inputs["Color"])
        links.new(separate.outputs["Green"], shader.inputs["Roughness"])
        links.new(separate.outputs["Blue"], shader.inputs["Metallic"])
        if variant == "ao":
            group = bpy.data.node_groups.new("glTF Material Output", "ShaderNodeTree")
            group.interface.new_socket(name="Occlusion", in_out="INPUT", socket_type="NodeSocketFloat")
            output = nodes.new("ShaderNodeGroup")
            output.node_tree = group
            links.new(separate.outputs["Red"], output.inputs["Occlusion"])
    if variant == "orm_factor":
        shader.inputs["Metallic"].default_value = 1
        shader.inputs["Roughness"].default_value = 64 / 255
    if variant in ("emission", "emission_factor"):
        shader.inputs["Emission Strength"].default_value = 2
        shader.inputs["Emission Color"].default_value = (linear_gray, linear_gray, linear_gray, 1)
        if variant == "emission":
            links.new(texture("gray").outputs["Color"], shader.inputs["Emission Color"])
    if variant in ("uv0", "uv1", "mask", "blend"):
        tex = texture("checker", "UV1" if variant == "uv1" else "UV0")
        links.new(tex.outputs["Color"], shader.inputs["Base Color"])
        if variant == "mask":
            clip = nodes.new("ShaderNodeMath")
            clip.operation = "GREATER_THAN"
            clip.inputs[1].default_value = 0.5
            links.new(tex.outputs["Alpha"], clip.inputs[0])
            links.new(clip.outputs[0], shader.inputs["Alpha"])
        if variant == "blend":
            links.new(tex.outputs["Alpha"], shader.inputs["Alpha"])
    bpy.ops.export_scene.gltf(filepath=str(OUT / (variant + ".gltf")), export_format="GLTF_SEPARATE",
                              use_selection=True, export_yup=True, export_animations=False, export_tangents=True)
    exported = json.loads((OUT / (variant + ".gltf")).read_text(encoding="utf-8"))
    material = exported["materials"][0]
    if variant == "ao":
        assert "occlusionTexture" in material
    if variant == "mask":
        assert material["alphaMode"] == "MASK"
    if variant == "blend":
        assert material["alphaMode"] == "BLEND"
    if variant == "uv1":
        assert material["pbrMetallicRoughness"]["baseColorTexture"]["texCoord"] == 1
print("H06: exported 14 Blender material fixtures with external PNGs; source Hound untouched")

# A separate two-bone fixture checks baked constraints without touching Hound.
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.object.armature_add()
rig = bpy.context.object
rig.name = "H06_Rig"
bpy.ops.object.mode_set(mode="EDIT")
rig.data.edit_bones[0].name = "Root"
child = rig.data.edit_bones.new("Tip")
child.head = (0, 0, 1)
child.tail = (0, 0, 2)
child.parent = rig.data.edit_bones["Root"]
bpy.ops.object.mode_set(mode="OBJECT")
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, 1))
mesh = bpy.context.object
mesh.name = "H06_Skin"
bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)
for name in ("Root", "Tip"):
    mesh.vertex_groups.new(name=name).add(list(range(len(mesh.data.vertices))), .5, "REPLACE")
modifier = mesh.modifiers.new("Armature", "ARMATURE")
modifier.object = rig
mesh.parent = rig
bpy.ops.object.empty_add()
control = bpy.context.object
control.name = "ConstraintControl"
control.rotation_mode = "QUATERNION"
from mathutils import Quaternion
for frame, angle in ((1, 0), (16, .6), (31, 0)):
    control.rotation_quaternion = Quaternion((0, 1, 0), angle)
    control.keyframe_insert("rotation_quaternion", frame=frame)
constraint = rig.pose.bones["Tip"].constraints.new("COPY_ROTATION")
constraint.target = control
constraint.owner_space = "WORLD"
constraint.target_space = "WORLD"
bpy.context.scene.render.fps = 30
bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 31
bpy.context.view_layer.update()
samples = []
for frame in (1, 8, 16, 24, 31):
    bpy.context.scene.frame_set(frame)
    samples.append({"frame": frame, "tip_matrix": [list(row) for row in rig.evaluated_get(bpy.context.evaluated_depsgraph_get()).pose.bones["Tip"].matrix]})
assert abs(samples[0]["tip_matrix"][0][0] - samples[2]["tip_matrix"][0][0]) > .1, "Constraint must visibly rotate the bone"
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True)
bpy.context.view_layer.objects.active = rig
bpy.ops.nla.bake(frame_start=1, frame_end=31, only_selected=False, visual_keying=True, clear_constraints=True, bake_types={"POSE"})
bpy.context.scene.frame_set(1)
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True)
mesh.select_set(True)
bpy.context.view_layer.objects.active = rig
bpy.ops.export_scene.gltf(filepath=str(OUT / "skin.gltf"), export_format="GLTF_SEPARATE", use_selection=True,
                          export_yup=True, export_animations=True, export_animation_mode="SCENE",
                          export_force_sampling=True, export_bake_animation=True, export_anim_scene_split_object=False)
exported = json.loads((OUT / "skin.gltf").read_text(encoding="utf-8"))
assert len(exported["skins"][0]["joints"]) == 2 and exported.get("animations")
assert all(s["interpolation"] in ("LINEAR", "STEP") for a in exported["animations"] for s in a["samplers"])
assert not any(n.get("name") == "ConstraintControl" for n in exported["nodes"])
(OUT / "constraint-samples.json").write_text(json.dumps(samples, indent=2), encoding="utf-8")
print("H06: baked constraint exported as sampled TRS, two bones and two weights")
