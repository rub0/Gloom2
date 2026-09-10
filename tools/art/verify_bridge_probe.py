'''Validate the Blender fixture and its external dependencies without Blender.'''

import json
import math
from pathlib import Path
import struct


root = Path(__file__).resolve().parents[2]
folder = root / 'assets/tests/blender_bridge'
scene = json.loads((folder / 'bridge-probe.gltf').read_text(encoding='utf-8'))


def require(condition, message):
    if not condition:
        raise ValueError(message)


require(scene['asset']['version'] == '2.0', 'Expected glTF 2.0')
require(len(scene['meshes']) == 3 and len(scene['nodes']) == 3, 'Expected three meshes and nodes')
require(len(scene['materials']) == 3 and len(scene['images']) == 1, 'Expected three materials and one image')
require(not scene.get('skins') and not scene.get('animations'), 'This fixture is static')
require({node['name'] for node in scene['nodes']} == {'ProbePlate', 'ProbeTexturedPanel', 'ProbeEnergyCore'}, 'Unexpected scene objects')

for buffer in scene['buffers']:
    path = (folder / buffer['uri']).resolve()
    require(path.parent == folder.resolve(), 'Buffer must be a local sibling')
    require(path.stat().st_size == buffer['byteLength'], 'Truncated or oversized buffer')

for view in scene['bufferViews']:
    require(view.get('byteOffset', 0) + view['byteLength'] <= scene['buffers'][view['buffer']]['byteLength'], 'Buffer view out of bounds')

components = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}
sizes = {5121: 1, 5123: 2, 5125: 4, 5126: 4}
for accessor in scene['accessors']:
    view = scene['bufferViews'][accessor['bufferView']]
    element_size = components[accessor['type']] * sizes[accessor['componentType']]
    extent = accessor.get('byteOffset', 0) + (accessor['count'] - 1) * view.get('byteStride', element_size) + element_size
    require(accessor['count'] > 0 and extent <= view['byteLength'], 'Accessor out of bounds')

triangles = 0
for mesh in scene['meshes']:
    for primitive in mesh['primitives']:
        require(primitive.get('mode', 4) == 4, 'Expected triangle geometry')
        require({'POSITION', 'NORMAL', 'TEXCOORD_0'} <= primitive['attributes'].keys(), 'Positions, normals or UVs missing')
        count = scene['accessors'][primitive['indices']]['count']
        require(count % 3 == 0, 'Incomplete triangle')
        triangles += count // 3

plate = next(node for node in scene['nodes'] if node['name'] == 'ProbePlate')
require(plate['translation'] == [0, 1, 0], 'Expected Blender Z-up to glTF Y-up conversion in meters')
position = scene['accessors'][scene['meshes'][plate['mesh']]['primitives'][0]['attributes']['POSITION']]
require(math.isclose(position['max'][1] - position['min'][1], 1.8, abs_tol=0.001), 'Plate height must remain 1.8 meters')
require(any(material.get('emissiveFactor', [0, 0, 0])[0] > 0.7 for material in scene['materials']), 'Emission missing')
require(any(material['pbrMetallicRoughness'].get('metallicFactor', 1) > 0.8 for material in scene['materials']), 'Metallic material missing')
require(any('baseColorTexture' in material['pbrMetallicRoughness'] for material in scene['materials']), 'Texture binding missing')

image = scene['images'][0]
require('uri' in image and 'bufferView' not in image, 'Cooker requires an external texture')
image_path = (folder / image['uri']).resolve()
require(image_path.parent == folder.resolve(), 'Image must be a local sibling')
png = image_path.read_bytes()
require(png[:8] == bytes([137, 80, 78, 71, 13, 10, 26, 10]), 'Expected PNG texture')
require(struct.unpack('>II', png[16:24]) == (8, 8), 'Expected 8x8 checker texture')
print(f'Bridge fixture verified: 3 meshes, {triangles} triangles, 3 materials, external 8x8 PNG, meters/Y-up.')
