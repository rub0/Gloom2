'''Small disposable Blender-to-Gloom fixture, executed through Blender MCP.'''

import bpy


if bpy.data.filepath and not bpy.data.filepath.endswith('bridge-probe.blend'):
    raise RuntimeError('Open a fresh Blender scene before running the bridge probe')
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1.0

iron = bpy.data.materials.new('ProbeIron')
iron.use_nodes = True
shader = iron.node_tree.nodes.get('Principled BSDF')
shader.inputs['Base Color'].default_value = (0.12, 0.14, 0.16, 1)
shader.inputs['Metallic'].default_value = 0.9
shader.inputs['Roughness'].default_value = 0.38

cloth = bpy.data.materials.new('ProbeCloth')
cloth.use_nodes = True
shader = cloth.node_tree.nodes.get('Principled BSDF')
shader.inputs['Roughness'].default_value = 0.85
texture = bpy.data.images.new('ProbeChecker', width=8, height=8)
pixels = []
for row in range(8):
    for column in range(8):
        pixels.extend((0.25, 0.12, 0.08, 1) if (row + column) % 2 else (0.07, 0.04, 0.03, 1))
texture.pixels = pixels
texture.pack()
texture_node = cloth.node_tree.nodes.new('ShaderNodeTexImage')
texture_node.image = texture
cloth.node_tree.links.new(texture_node.outputs['Color'], shader.inputs['Base Color'])

energy = bpy.data.materials.new('ProbeEnergy')
energy.use_nodes = True
shader = energy.node_tree.nodes.get('Principled BSDF')
shader.inputs['Base Color'].default_value = (0.8, 0.12, 0.01, 1)
shader.inputs['Emission Color'].default_value = (0.8, 0.12, 0.01, 1)
shader.inputs['Emission Strength'].default_value = 1.0
shader.inputs['Roughness'].default_value = 0.4

bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, 1))
plate = bpy.context.object
plate.name = 'ProbePlate'
plate.scale = (1.4, 0.24, 1.8)
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
plate.data.materials.append(iron)
bevel = plate.modifiers.new('RoundedEdges', 'BEVEL')
bevel.width = 0.06
bevel.segments = 3
bpy.ops.object.modifier_apply(modifier=bevel.name)

bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.2, 1))
panel = bpy.context.object
panel.name = 'ProbeTexturedPanel'
panel.scale = (0.9, 0.15, 1.1)
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
panel.data.materials.append(cloth)

bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8, radius=0.14, location=(0, 0.38, 1.4))
core = bpy.context.object
core.name = 'ProbeEnergyCore'
core.data.materials.append(energy)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.wm.save_as_mainfile(filepath='D:/Projects/Gloom/.cache/blender-bridge/bridge-probe.blend', check_existing=False)
bpy.ops.export_scene.gltf(filepath='D:/Projects/Gloom/assets/tests/blender_bridge/bridge-probe.gltf',
                          export_format='GLTF_SEPARATE', use_selection=True, export_yup=True, export_animations=False)
print('GLOOM_BRIDGE_EXPORTED: 3 meshes, 3 materials, external color texture, Y-up, meters')
