"""Synthetic H06 load, never a production Hound: v16 geometry + UVs/maps/clip load.

Writes only .cache and cooked build content. Re-run with --size 1024/2048 to
compare texture envelopes; all source hashes must remain unchanged.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--size', type=int, choices=(1024, 2048), default=2048)
    args = parser.parse_args()
    manifest = json.loads((ROOT/'art/characters/hound/v16/sculpture-reference.json').read_text(encoding='utf-8'))
    for entry in manifest['files']:
        assert hashlib.sha256((ROOT/entry['path']).read_bytes()).hexdigest() == entry['sha256']
    folder = ROOT/'.cache/hound-budget-106/source/h06-budget'
    folder.mkdir(parents=True, exist_ok=True)
    scene = json.loads((ROOT/'assets/characters/hound_rig/v16/hound-rig.gltf').read_text(encoding='utf-8'))
    binary = bytearray((ROOT/'assets/characters/hound_rig/v16/hound-rig.bin').read_bytes())

    def accessor(index):
        a = scene['accessors'][index]
        view = scene['bufferViews'][a['bufferView']]
        width = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
        assert a['componentType'] == 5126 and 'byteStride' not in view
        return np.frombuffer(binary, '<f4', a['count']*width,
                             view.get('byteOffset', 0)+a.get('byteOffset', 0)).reshape(-1, width).copy()

    def append(values, kind):
        values = np.asarray(values, dtype='<f4')
        binary.extend(b'\0' * (-len(binary) % 4))
        scene['bufferViews'].append({'buffer': 0, 'byteOffset': len(binary), 'byteLength': values.nbytes})
        binary.extend(values.tobytes())
        scene['accessors'].append({'bufferView': len(scene['bufferViews'])-1, 'componentType': 5126,
                                  'count': len(values), 'type': kind})
        return len(scene['accessors'])-1

    # Deliberately diagnostic planar UVs, not authored UVs or an accepted bake.
    for primitive in scene['meshes'][0]['primitives']:
        position = accessor(primitive['attributes']['POSITION'])
        primitive['attributes']['TEXCOORD_0'] = append(position[:, [0, 1]]*.5 + .5, 'VEC2')
    template = copy.deepcopy(scene['animations'][0])
    times = np.linspace(0, 6, 181, dtype='<f4')
    time_index = append(times, 'SCALAR')
    for sampler in template['samplers']:
        old_times = accessor(sampler['input'])[:, 0]
        old_values = accessor(sampler['output'])
        values = np.column_stack([np.interp(times, old_times, old_values[:, c]) for c in range(old_values.shape[1])])
        if values.shape[1] == 4:
            values /= np.linalg.norm(values, axis=1)[:, None]
        sampler['input'] = time_index
        sampler['output'] = append(values, 'VEC4' if values.shape[1] == 4 else 'VEC3')
        sampler['interpolation'] = 'LINEAR'
    scene['animations'] = []
    for name in ('idle', 'forward', 'strafe_right', 'jump'):
        clip = copy.deepcopy(template)
        clip['name'] = name
        scene['animations'].append(clip)
    scene['images'], scene['textures'] = [], []
    scene['samplers'] = [{'magFilter': 9729, 'minFilter': 9987, 'wrapS': 10497, 'wrapT': 10497}]
    # Eight distinct atlas sets reserve the bytes of eight skins. Seven sets are
    # sampled across every TPS; the eighth is memory reserve, not extra geometry.
    scene['materials'].append(copy.deepcopy(scene['materials'][0]))
    yy, xx = np.indices((args.size, args.size))
    for atlas, material in enumerate(scene['materials']):
        base = len(scene['textures'])
        for channel in ('color', 'normal', 'orm', 'emission'):
            pixels = np.full((args.size, args.size, 4), 255, dtype=np.uint8)
            wave = ((xx//16 + yy//16 + atlas) % 2).astype(np.uint8)
            if channel == 'color':
                pixels[:, :, :3] = (96 + wave*48)[:, :, None]
            elif channel == 'normal':
                pixels[:, :, 0] = 112 + wave*32
                pixels[:, :, 1] = 144 - wave*32
                pixels[:, :, 2] = 253
            elif channel == 'orm':
                pixels[:, :, 0], pixels[:, :, 1], pixels[:, :, 2] = 224, 144 + wave*32, 64 + wave*64
            else:
                pixels[:, :, :3] = (wave*12)[:, :, None]
            name = f'atlas-{atlas}-{channel}.png'
            Image.fromarray(pixels).save(folder/name)
            scene['images'].append({'uri': name})
            scene['textures'].append({'sampler': 0, 'source': len(scene['images'])-1})
        material['pbrMetallicRoughness']['baseColorTexture'] = {'index': base}
        material['pbrMetallicRoughness']['metallicRoughnessTexture'] = {'index': base+2}
        material['normalTexture'] = {'index': base+1}
        material['occlusionTexture'] = {'index': base+2}
        material['emissiveTexture'] = {'index': base+3}
        material['emissiveFactor'] = [.1, .1, .1]
    scene['buffers'] = [{'uri': 'hound.bin', 'byteLength': len(binary)}]
    (folder/'hound.bin').write_bytes(binary)
    (folder/'hound.gltf').write_text(json.dumps(scene), encoding='utf-8')
    print(f'Probe: {args.size}px, 32 images, 4 diagnostic clips / 115116 channel keys; v16 preserved', flush=True)
    subprocess.run(['rtk', 'proxy', str(ROOT/'build/windows-vs/Release/gloom_asset_cooker.exe'), str(folder.parent),
                    str(ROOT/'build/windows-vs/content'), 'game:/h06-budget/hound.gltf', 'cache:/h06-budget/hound.gasset'], check=True)


if __name__ == '__main__':
    main()
