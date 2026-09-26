"""Measure unchanged v16 from the cooked scene v5; no performance claims from static counts."""
import hashlib
import json
import math
import struct
from pathlib import Path
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
REPORT = ROOT / "reports/hound-contract-101"
SOURCE = ROOT / "assets/characters/hound_rig/v16/hound-rig.gltf"
scene = json.loads(SOURCE.read_text(encoding="utf-8"))
manifest = json.loads((ROOT / "art/characters/hound/v16/sculpture-reference.json").read_text(encoding="utf-8"))
for entry in manifest["files"]:
    assert hashlib.sha256((ROOT / entry["path"]).read_bytes()).hexdigest() == entry["sha256"], entry["path"]
raw = (ROOT / ".cache/hound-h06/cooked/hound-v16.gasset").read_bytes()
assert struct.unpack_from("<I", raw, 40)[0] == 0, "Expected no image dependencies"
data = raw[52:]
assert data[:8] == b"GLOOMSCN"
version, default, *counts = struct.unpack_from("<11I", data, 8)
assert version == 5 and counts[0] == 7 and counts[7] == 1
offset = 52
vertex = np.dtype([("position", "<f4", 3), ("normal", "<f4", 3), ("uv", "<f4", 2),
                   ("tangent", "<f4", 4), ("uv1", "<f4", 2), ("joints", "<u2", 8), ("weights", "<f4", 8)])
assert vertex.itemsize == 104
joints = [scene["nodes"][index]["name"] for index in scene["skins"][0]["joints"]]
arm = np.array([any(part in name for part in ("UpperArm", "Forearm", "Hand", "Finger")) for name in joints])
rows, positions = [], []
for _ in range(counts[0]):
    material, nv, ni = struct.unpack_from("<3I", data, offset)
    offset += 12
    vertices = np.frombuffer(data, vertex, nv, offset)
    offset += nv * vertex.itemsize
    indices = np.frombuffer(data, "<u4", ni, offset).reshape(-1, 3)
    offset += ni * 4
    center_x, center_y, center_z, radius, nlods = struct.unpack_from("<4fI", data, offset)
    offset += 20
    lods = []
    for lod in range(nlods):
        count = struct.unpack_from("<I", data, offset)[0]
        offset += 4 + count * 4
        lods.append(count // 3)
    weights = vertices["weights"]
    assert np.allclose(weights.sum(axis=1), 1, atol=1e-5)
    arms_weight = (arm[vertices["joints"]] * weights).sum(axis=1)
    arms_triangles = int((arms_weight[indices] > .95).all(axis=1).sum())
    vbo_copies = 1 + nlods + bool(arms_triangles)
    rows.append({"material": scene["materials"][material]["name"], "vertices": nv, "triangles": ni // 3,
                 "auto_lod_triangles": lods, "fps_arms_triangles": arms_triangles,
                 "max_influences": int((weights > 0).sum(axis=1).max()),
                 "mesh_upload_bytes": int(nv * 104 * vbo_copies + 12 * (ni // 3 + sum(lods) + arms_triangles))})
    positions.append(vertices["position"])
points = np.concatenate(positions)
# The v16 mesh node is identity; the viewer places eight shared copies on a 4 m grid.
mesh_node = next(node for node in scene["nodes"] if "mesh" in node)
assert all(key not in mesh_node for key in ("matrix", "translation", "rotation", "scale"))
eye, target = np.array([7., 5., -9.]), np.array([0., 1., 0.])
forward = target - eye
forward /= np.linalg.norm(forward)
right = np.cross([0., 1., 0.], forward)
right /= np.linalg.norm(right)
up = np.cross(forward, right)
screens = []
for copy in range(8):
    world = points + [(copy % 3 - 1.5) * 4, 0, (copy // 3 - 1.5) * 4]
    relative = world - eye
    depth = relative @ forward
    assert np.all(depth > 0)
    # Camera default is checked against include/gloom/render/scene.hpp before reporting.
    pixels = 720 / (2 * math.tan(0.785398163 / 2))
    yy = (relative @ up) / depth * pixels
    xx = (relative @ right) / depth * pixels
    screens.append({"copy": copy, "height_px_720p": float(yy.max() - yy.min()),
                    "width_px_720p": float(xx.max() - xx.min())})
report = {"source": SOURCE.relative_to(ROOT).as_posix(), "immutable_sha256_verified": True,
          "cooked_bytes": len(raw), "materials": counts[2], "mesh_primitives": counts[0],
          "bones": len(joints), "clips": [clip["name"] for clip in scene.get("animations", [])],
          "images": len(scene.get("images", [])), "vertices_cooked": sum(r["vertices"] for r in rows),
          "triangles_lod0": sum(r["triangles"] for r in rows),
          "triangles_auto_lod1": sum(r["auto_lod_triangles"][0] for r in rows),
          "triangles_auto_lod2": sum(r["auto_lod_triangles"][1] for r in rows),
          "fps_arms_triangles": sum(r["fps_arms_triangles"] for r in rows),
          "geometry_upload_bytes_including_lods_and_arms": sum(r["mesh_upload_bytes"] for r in rows),
          "eight_tps_triangles_lod0": 8 * sum(r["triangles"] for r in rows),
          "seven_tps_plus_fps_triangles_lod0": 7 * sum(r["triangles"] for r in rows) + sum(r["fps_arms_triangles"] for r in rows),
          "per_material": rows, "viewer_static_projection": screens,
          "projection_note": "1280x720 default viewer, FOV 45 degrees, no animation or production camera claim"}
REPORT.mkdir(parents=True, exist_ok=True)
(REPORT / "asset-measurements.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(json.dumps({key: value for key, value in report.items() if key not in ("per_material", "viewer_static_projection")}, indent=2))

# Preserve compact benchmark observations, not timing claims from the asset inventory.
import re
benchmarks = []
for name in ("factory-1080p", "factory-1080p-repeat", "factory-720p"):
    raw_log = (ROOT / ".cache/hound-h06" / (name + ".log")).read_bytes()
    log = raw_log.decode("utf-16" if raw_log.startswith(b"\xff\xfe") else "utf-8", errors="replace")
    log = re.sub(r"\x1b\[[0-9;]*m", "", log)
    prefixes = ("Factory benchmark:", "CPU stages ms:", "Process memory:", "Factory render:")
    lines = [line.strip() for line in log.splitlines() if line.strip().startswith(prefixes)]
    assert len(lines) == 4 and "samples=360" in lines[0]
    assert not any(word in log for word in ("VUID-", "Diligent Engine: ERROR", "Diligent Engine: Error"))
    benchmarks.append({"run": name, "observations": lines})
(REPORT / "benchmark.json").write_text(json.dumps({
    "cpu": "AMD Ryzen 7 3700X", "gpu": "NVIDIA GeForce GTX 1070", "driver": "32.0.15.8129",
    "system_memory_bytes": 17082138624, "configuration": "Release Vulkan",
    "target_user_confirmed": {"fps": 200, "frame_ms": 5, "resolution": [1920, 1080], "combatants": 8},
    "measured_content": "Current Factory and two combatants with original presentation; not Hound v16 or eight animated combatants",
    "runs": benchmarks}, indent=2) + "\n", encoding="utf-8")
