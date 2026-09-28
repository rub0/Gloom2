"""Check KTX normal XY/mips and runtime mirrored tangent frames with the H06 Blender sphere.

The KTX C API exposes RRRG literally. The C++ gloom.assets test covers the
runtime RGBA8 correction; these captures exercise it on the selected GPU path.
"""
import ctypes as C
import json
import re
import struct
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / ".cache/hound-normals-103"
SOURCE = WORK / "source"
COOKED = WORK / "cooked"
REPORT = ROOT / "reports/normal-maps-103"
for directory in (SOURCE, COOKED, REPORT):
    directory.mkdir(parents=True, exist_ok=True)
BASE = ROOT / ".cache/hound-h06/source"
assert (BASE / "normal_x.gltf").exists(), "Run create_hound_h06_probe.py in Blender first"


def run(executable, name, capture=False):
    args = [SOURCE, COOKED, "game:/" + name + ".gltf", "cache:/" + name + ".gasset"]
    if capture:
        args += [1, WORK / (name + ".ppm")]
    result = subprocess.run(["rtk", "proxy", str(ROOT / "build/windows-vs/Release" / executable), *map(str, args)],
                            cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=90)
    log = re.sub(r"\x1b\[[0-9;]*m", "", result.stdout + result.stderr)
    (WORK / (name + ("-gpu.log" if capture else "-cook.log"))).write_text(log, encoding="utf-8")
    assert result.returncode == 0 and not any(s in log for s in ("VUID-", "Diligent Engine: ERROR", "Diligent Engine: Error")), log[-2000:]
    return log


ktx = C.CDLL(str(ROOT / "build/windows-vs/vcpkg_installed/x64-windows/bin/ktx.dll"))
ktx.ktxTexture2_CreateFromMemory.argtypes = [C.c_void_p, C.c_size_t, C.c_uint, C.POINTER(C.c_void_p)]
ktx.ktxTexture2_TranscodeBasis.argtypes = [C.c_void_p, C.c_int, C.c_uint]
ktx.ktxTexture2_GetImageOffset.argtypes = [C.c_void_p, C.c_uint, C.c_uint, C.c_uint, C.POINTER(C.c_size_t)]
ktx.ktxTexture_GetData.argtypes = [C.c_void_p]
ktx.ktxTexture_GetData.restype = C.c_void_p
ktx.ktxTexture2_Destroy.argtypes = [C.c_void_p]


def decode_bc5(raw, width, height):
    pixels = np.zeros((height, width, 2), dtype=float)
    for by in range((height + 3) // 4):
        for bx in range((width + 3) // 4):
            block = ((by * ((width + 3) // 4)) + bx) * 16
            for channel in range(2):
                a, b = raw[block + channel * 8:block + channel * 8 + 2]
                values = [a, b]
                values += ([(a * (7 - i) + b * i) / 7 for i in range(1, 7)] if a > b
                           else [(a * (5 - i) + b * i) / 5 for i in range(1, 5)] + [0, 255])
                indices = int.from_bytes(raw[block + channel * 8 + 2:block + channel * 8 + 8], "little")
                for i in range(16):
                    y, x = by * 4 + i // 4, bx * 4 + i % 4
                    if x < width and y < height:
                        pixels[y, x, channel] = values[(indices >> (3 * i)) & 7]
    return pixels


results = {"channels": [], "gpu": []}
cases = {"flat": (128, 128), "x": (210, 128), "minus_x": (45, 128),
         "y": (128, 210), "minus_y": (128, 45), "diagonal": (184, 72),
         "mirror_u_x": (45, 128), "mirror_u_y": (128, 210), "mirror_v_x": (210, 128), "mirror_v_y": (128, 45),
         "seam_x": (210, 128), "seam_y": (128, 210)}
for name, xy in cases.items():
    scene = json.loads((BASE / "normal_x.gltf").read_text(encoding="utf-8"))
    binary = bytearray((BASE / scene["buffers"][0]["uri"]).read_bytes())
    if name.startswith("seam"):
        # Split every triangle and offset alternating UV islands by whole tiles.
        # Constant normals must still shade like the original welded sphere.
        primitive = scene["meshes"][0]["primitives"][0]
        index = scene["accessors"][primitive["indices"]]
        view = scene["bufferViews"][index["bufferView"]]
        indices = np.frombuffer(binary, {5123: "<u2", 5125: "<u4"}[index["componentType"]], index["count"],
                                view.get("byteOffset", 0) + index.get("byteOffset", 0))
        attributes, views, accessors, output = {}, [], [], bytearray()
        for semantic, width in (("POSITION", 3), ("NORMAL", 3), ("TEXCOORD_0", 2)):
            acc = scene["accessors"][primitive["attributes"][semantic]]
            view = scene["bufferViews"][acc["bufferView"]]
            values = np.ndarray((acc["count"], width), "<f4", binary, view.get("byteOffset", 0) + acc.get("byteOffset", 0),
                                (view.get("byteStride", width * 4), 4))[indices].copy()
            if semantic == "TEXCOORD_0":
                values.reshape(-1, 3, 2)[::2, :, 0] += 2
            attributes[semantic] = len(accessors)
            accessors.append({"bufferView": len(views), "componentType": 5126, "count": len(values), "type": "VEC" + str(width),
                              "min": values.min(axis=0).tolist(), "max": values.max(axis=0).tolist()})
            views.append({"buffer": 0, "byteOffset": len(output), "byteLength": values.nbytes})
            output.extend(values.tobytes())
        primitive["attributes"] = attributes
        primitive.pop("indices")
        scene["accessors"], scene["bufferViews"], binary = accessors, views, output
        scene["buffers"][0]["byteLength"] = len(binary)
    if name.startswith("mirror"):
        primitive = scene["meshes"][0]["primitives"][0]
        uv = scene["accessors"][primitive["attributes"]["TEXCOORD_0"]]
        view = scene["bufferViews"][uv["bufferView"]]
        axis = 0 if name.startswith("mirror_u") else 1
        for index in range(uv["count"]):
            offset = view.get("byteOffset", 0) + uv.get("byteOffset", 0) + index * view.get("byteStride", 8) + axis * 4
            struct.pack_into("<f", binary, offset, 1 - struct.unpack_from("<f", binary, offset)[0])
    scene["buffers"][0]["uri"] = name + ".bin"
    scene["images"][0]["uri"] = name + ".png"
    (SOURCE / (name + ".bin")).write_bytes(binary)
    (SOURCE / (name + ".gltf")).write_text(json.dumps(scene), encoding="utf-8")
    Image.new("RGBA", (32, 32), (*xy, 224, 255)).save(SOURCE / (name + ".png"))
    run("gloom_asset_cooker.exe", name)
    envelope = (COOKED / (name + ".gasset")).read_bytes()
    dependency = struct.unpack_from("<Q", envelope, 52)[0]
    payload = (COOKED / "dependencies" / (format(dependency, "x") + ".gasset")).read_bytes()[52:]
    assert struct.unpack_from("<I", payload, 40)[0] == 6, "Expected all six mip levels of a 32x32 image"
    for target, label in ((13, "ktx_rrrg"), (5, "bc5")):
        memory, texture = C.create_string_buffer(payload), C.c_void_p()
        assert ktx.ktxTexture2_CreateFromMemory(memory, len(payload), 1, C.byref(texture)) == 0
        try:
            assert ktx.ktxTexture2_TranscodeBasis(texture, target, 0) == 0
            for level in range(6):
                size, offset = max(32 >> level, 1), C.c_size_t()
                assert ktx.ktxTexture2_GetImageOffset(texture, level, 0, 0, C.byref(offset)) == 0
                count = size * size * 4 if target == 13 else ((size + 3) // 4) ** 2 * 16
                raw = C.string_at(ktx.ktxTexture_GetData(texture) + offset.value, count)
                pixels = np.frombuffer(raw, np.uint8).reshape(size, size, 4)[..., [0, 3]] if target == 13 else decode_bc5(raw, size, size)
                error = float(np.abs(pixels.astype(float) - xy).max())
                results["channels"].append({"case": name, "target": label, "mip": level, "max_error": error, "passed": error <= 4})
        finally:
            ktx.ktxTexture2_Destroy(texture)

if "--cpu-only" not in sys.argv:
    paths = {}
    for name in ("flat", "x", "y", "mirror_u_x", "mirror_u_y", "mirror_v_x", "mirror_v_y", "seam_x", "seam_y"):
        log = run("gloom_scene_viewer.exe", name, True)
        paths[name] = next(line for line in log.splitlines() if line.startswith("Texture path:"))
        print(name + ": " + paths[name], flush=True)
    for a, b, close in (("x", "mirror_u_x", True), ("y", "mirror_u_y", True),
                         ("x", "mirror_v_x", True), ("y", "mirror_v_y", True),
                         ("x", "seam_x", True), ("y", "seam_y", True),
                         ("flat", "x", False), ("flat", "y", False), ("x", "y", False)):
        aa = np.asarray(Image.open(WORK / (a + ".ppm"))).astype(float)[180:360, 530:710]
        bb = np.asarray(Image.open(WORK / (b + ".ppm"))).astype(float)[180:360, 530:710]
        error = float(np.abs(aa - bb).mean())
        results["gpu"].append({"a": a, "b": b, "expected": "close" if close else "different",
                               "texture_path": paths[b], "mean_error": error, "passed": error < 3 if close else error > 5})
    sheet = Image.new("RGB", (960, 420), (22, 25, 29))
    draw = ImageDraw.Draw(sheet)
    for index, name in enumerate(("x", "mirror_u_x", "mirror_v_x", "y", "mirror_u_y", "mirror_v_y")):
        im = Image.open(WORK / (name + ".ppm"))
        im.thumbnail((320, 180))
        x, y = index % 3 * 320, index // 3 * 210
        sheet.paste(im, (x, y + 24))
        draw.text((x + 8, y + 5), name, fill="white")
    sheet.save(REPORT / "mirrored-normals.png")
(REPORT / "checks.json").write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
failed = [row for rows in results.values() for row in rows if not row["passed"]]
print(json.dumps({"channel_checks": len(results["channels"]), "gpu_checks": len(results["gpu"]), "failed": failed}, indent=2))
assert not failed, "Cooked normal XY or mirrored tangent frame regression"
