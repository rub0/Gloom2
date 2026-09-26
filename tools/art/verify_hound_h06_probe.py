"""Run H06 material fixtures through the unchanged cooker and GPU viewer."""
import json
import subprocess
import re
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / ".cache/hound-h06"
REPORT = ROOT / "reports/hound-contract-101"
REPORT.mkdir(parents=True, exist_ok=True)
(WORK / "cooked").mkdir(exist_ok=True)
variants = ("factor", "color", "normal", "normal_x", "normal_y", "orm", "orm_factor", "ao", "emission", "emission_factor", "uv0", "uv1", "mask", "blend")
results = {}

def run(executable, arguments, label):
    result = subprocess.run(["rtk", "proxy", str(ROOT / "build/windows-vs/Release" / executable), *map(str, arguments)],
                            cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=90)
    log = re.sub(r'\x1b\[[0-9;]*m', '', result.stdout + result.stderr)
    (WORK / (label + ".log")).write_text(log, encoding="utf-8")
    assert result.returncode == 0, (label, log[-1800:])
    assert not any(word in log for word in ("VUID-", "Diligent Engine: ERROR", "Diligent Engine: Error")), label
    return log

for name in variants:
    args = [WORK / "source", WORK / "cooked", "game:/" + name + ".gltf", "cache:/" + name + ".gasset"]
    run("gloom_asset_cooker.exe", args, name + "-cook")
    log = run("gloom_scene_viewer.exe", args + [1, WORK / (name + ".ppm")], name + "-gpu")
    results[name] = {"visibility": next(line for line in log.splitlines() if line.startswith("Visibility:")),
                     "size": list(Image.open(WORK / (name + ".ppm")).size)}
    print(name + ": cooker + GPU OK", flush=True)

comparisons = []
for a, b, relation in (("factor", "color", "close"), ("orm", "orm_factor", "close"),
                       ("emission", "emission_factor", "close"), ("factor", "normal", "different"),
                       ("factor", "normal_x", "different"), ("factor", "normal_y", "different"), ("factor", "orm", "different"), ("orm", "ao", "different"),
                       ("factor", "emission", "different"), ("uv0", "uv1", "different"),
                       ("uv0", "mask", "different"), ("mask", "blend", "different")):
    aa = np.asarray(Image.open(WORK / (a + ".ppm"))).astype(int)
    bb = np.asarray(Image.open(WORK / (b + ".ppm"))).astype(int)
    delta = np.abs(aa - bb)
    roi = delta[180:360, 530:710]
    record = {"a": a, "b": b, "expected": relation, "max_channel_error": int(delta.max()),
              "mean_channel_error": float(delta.mean()), "interior_mean_error": float(roi.mean()), "pixels_changed_over_3": int((delta.max(axis=2) > 3).sum())}
    # Same scene/camera/light; compression and 8-bit rounding may differ slightly.
    record["passed"] = record["interior_mean_error"] < 3 if relation == "close" else record["interior_mean_error"] > 5
    comparisons.append(record)

sheet = Image.new("RGB", (4 * 320, 4 * 210), (22, 25, 29))
draw = ImageDraw.Draw(sheet)
for index, name in enumerate(variants):
    im = Image.open(WORK / (name + ".ppm")).convert("RGB")
    im.thumbnail((320, 180))
    x, y = (index % 4) * 320, (index // 4) * 210
    sheet.paste(im, (x, y + 24))
    draw.text((x + 8, y + 5), name, fill="white")
sheet.save(REPORT / "material-probes.png")
(REPORT / "materials.json").write_text(json.dumps({"fixtures": results, "comparisons": comparisons}, indent=2) + "\n", encoding="utf-8")
assert all(row["passed"] for row in comparisons), [row["b"] for row in comparisons if not row["passed"]]
print("H06 GPU comparisons passed", flush=True)
