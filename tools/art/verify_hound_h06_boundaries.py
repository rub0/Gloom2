"""H06 boundary probes and a read-only diagnostic of cooked normal channels."""
import base64
import copy
import ctypes as C
import json
import re
import struct
import subprocess
from pathlib import Path
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / ".cache/hound-h06"
SOURCE = WORK / "source"
REPORT = ROOT / "reports/hound-contract-101"
REPORT.mkdir(parents=True, exist_ok=True)
(WORK / "cooked").mkdir(exist_ok=True)
results = {}

def read(name):
    return json.loads((SOURCE / (name + ".gltf")).read_text(encoding="utf-8"))

def cook(name, scene=None, reject=False):
    if scene is not None:
        (SOURCE / (name + ".gltf")).write_text(json.dumps(scene), encoding="utf-8")
    result = subprocess.run(["rtk", "proxy", str(ROOT / "build/windows-vs/Release/gloom_asset_cooker.exe"),
                             str(SOURCE), str(WORK / "cooked"), "game:/" + name + ".gltf", "cache:/" + name + ".gasset"],
                            cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=30)
    log = re.sub(r"\x1b\[[0-9;]*m", "", result.stdout + result.stderr).strip()
    assert (result.returncode != 0) == reject, (name, log)
    results[name] = {"expected_rejection": reject, "exit_code": result.returncode, "message": log}

def values(scene, index):
    acc = scene["accessors"][index]
    view = scene["bufferViews"][acc["bufferView"]]
    data = (SOURCE / scene["buffers"][view["buffer"]]["uri"]).read_bytes()
    n = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}[acc["type"]]
    assert acc["componentType"] == 5126 and "byteStride" not in view
    return np.frombuffer(data, "<f4", acc["count"] * n, view.get("byteOffset", 0) + acc.get("byteOffset", 0)).reshape(-1, n)

skin = read("skin")
tip = next(i for i, n in enumerate(skin["nodes"]) if n["name"] == "Tip")
channel = next(c for c in skin["animations"][0]["channels"] if c["target"] == {"node": tip, "path": "rotation"})
sampler = skin["animations"][0]["samplers"][channel["sampler"]]
rotations = values(skin, sampler["output"])
assert len(rotations) >= 3 and np.max(np.ptp(rotations, axis=0)) > .15, "Baked animation must move"
cook("skin")
results["skin"]["rotation_keys"] = len(rotations)
results["skin"]["interpolation"] = sampler["interpolation"]
skin_weights = values(skin, skin["meshes"][0]["primitives"][0]["attributes"]["WEIGHTS_0"])
assert np.allclose(skin_weights.sum(axis=1), 1) and int((skin_weights > 0).sum(axis=1).max()) == 2
results["skin"]["two_weights_per_vertex"] = True

scene = read("factor")
scene["meshes"][0]["primitives"][0]["targets"] = [{"POSITION": 0}]
cook("reject_morph", scene, True)
scene = read("color")
scene["materials"][0]["pbrMetallicRoughness"]["metallicRoughnessTexture"] = {"index": 0}
cook("reject_mixed_semantics", scene, True)
scene = read("color")
scene["materials"][0]["pbrMetallicRoughness"]["baseColorTexture"]["texCoord"] = 2
cook("reject_uv2", scene, True)
scene = read("color")
del scene["meshes"][0]["primitives"][0]["attributes"]["TEXCOORD_0"]
cook("reject_missing_uv", scene, True)
scene = read("color")
scene["images"][0]["uri"] = "data:image/png;base64," + base64.b64encode((SOURCE / "gray.png").read_bytes()).decode()
cook("reject_embedded", scene, True)
scene = copy.deepcopy(skin)
attrs = scene["meshes"][0]["primitives"][0]["attributes"]
attrs["JOINTS_2"], attrs["WEIGHTS_2"] = attrs["JOINTS_0"], attrs["WEIGHTS_0"]
cook("reject_ninth_weight_set", scene, True)

scene = copy.deepcopy(skin)
sampler = scene["animations"][0]["samplers"][channel["sampler"]]
acc = scene["accessors"][sampler["output"]]
original = values(scene, sampler["output"])
expanded = np.zeros((len(original), 3, 4), dtype="<f4")
expanded[:, 1] = original
binary = bytearray((SOURCE / scene["buffers"][0]["uri"]).read_bytes())
while len(binary) % 4:
    binary.append(0)
scene["bufferViews"].append({"buffer": 0, "byteOffset": len(binary), "byteLength": expanded.nbytes})
binary += expanded.tobytes()
scene["accessors"].append({"bufferView": len(scene["bufferViews"]) - 1, "componentType": 5126, "count": len(original) * 3, "type": "VEC4"})
sampler["output"] = len(scene["accessors"]) - 1
sampler["interpolation"] = "CUBICSPLINE"
scene["buffers"][0] = {"uri": "cubic.bin", "byteLength": len(binary)}
(SOURCE / "cubic.bin").write_bytes(binary)
cook("reject_cubic", scene, True)

cook("normal_x")
scene = read("normal_x")
acc = scene["accessors"][scene["meshes"][0]["primitives"][0]["attributes"]["TANGENT"]]
view = scene["bufferViews"][acc["bufferView"]]
binary = bytearray((SOURCE / scene["buffers"][0]["uri"]).read_bytes())
start = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
poison = -values(scene, scene["meshes"][0]["primitives"][0]["attributes"]["TANGENT"])
binary[start:start + poison.nbytes] = poison.tobytes()
scene["buffers"][0]["uri"] = "tangent-poison.bin"
(SOURCE / "tangent-poison.bin").write_bytes(binary)
cook("tangent_poison", scene)
def payload(name):
    raw = (WORK / "cooked" / (name + ".gasset")).read_bytes()
    return raw[52 + struct.unpack_from("<I", raw, 40)[0] * 8:]
assert payload("normal_x") == payload("tangent_poison"), "Importer began honoring source tangents; review contract"
results["tangent_poison"]["cooked_payload_identical"] = True

# Existing KTX C API: inspect the RGBA expansion of the very same UASTC payload used by the viewer.
ktx = C.CDLL(str(ROOT / "build/windows-vs/vcpkg_installed/x64-windows/bin/ktx.dll"))
ktx.ktxTexture2_CreateFromMemory.argtypes = [C.c_void_p, C.c_size_t, C.c_uint, C.POINTER(C.c_void_p)]
ktx.ktxTexture2_TranscodeBasis.argtypes = [C.c_void_p, C.c_int, C.c_uint]
ktx.ktxTexture_GetData.argtypes = [C.c_void_p]
ktx.ktxTexture_GetData.restype = C.c_void_p
ktx.ktxTexture_GetDataSize.argtypes = [C.c_void_p]
ktx.ktxTexture_GetDataSize.restype = C.c_size_t
ktx.ktxTexture2_Destroy.argtypes = [C.c_void_p]
for name in ("normal_x", "normal_y"):
    cook(name)
    raw = (WORK / "cooked" / (name + ".gasset")).read_bytes()
    dependency = struct.unpack_from("<Q", raw, 52)[0]
    raw_texture = (WORK / "cooked/dependencies" / (format(dependency, "016x") + ".gasset")).read_bytes()[52:]
    texture = C.c_void_p()
    memory = C.create_string_buffer(raw_texture)
    assert ktx.ktxTexture2_CreateFromMemory(memory, len(raw_texture), 1, C.byref(texture)) == 0
    try:
        assert ktx.ktxTexture2_TranscodeBasis(texture, 13, 0) == 0  # KTX_TTF_RGBA32
        rgba = C.string_at(ktx.ktxTexture_GetData(texture), ktx.ktxTexture_GetDataSize(texture))
        unique = np.unique(np.frombuffer(rgba, np.uint8).reshape(-1, 4), axis=0)
        results[name + "_ktx_rgba"] = unique.tolist()
    finally:
        ktx.ktxTexture2_Destroy(texture)
(REPORT / "boundaries.json").write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
print(json.dumps(results, indent=2))
