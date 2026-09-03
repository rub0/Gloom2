"""Read-only legacy audit and static Factory conversion experiment.

Requires Python 3.12+, assimp-py==1.1.0 and Pillow. This is NOT the production
material/map/skinning importer. Originals are never modified. Output must be
outside the legacy tree; see docs/ART_RESTORATION.md for limitations.
"""

import argparse
import base64
from collections import Counter
import hashlib
import importlib.metadata
import json
import math
from pathlib import Path
import re
import shutil
import struct
import xml.etree.ElementTree as ET

import assimp_py
from PIL import Image


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def material_blocks(path):
    text = re.sub(r"//[^\n]*", "", path.read_text(encoding="latin1"))
    declarations = list(re.finditer(r"(?m)^\s*material\s+([^\s:{]+)\s*\{", text))
    return {
        match[1]: text[match.end():declarations[i + 1].start()
                       if i + 1 < len(declarations) else len(text)]
        for i, match in enumerate(declarations)
    }


def map_summary(path):
    # Inventory literal type fields only. Never execute legacy Lua.
    text = re.sub(r"--[^\n]*|//[^\n]*", "", path.read_text(encoding="latin1"))
    return {"sha256": digest(path), "types": dict(sorted(Counter(
        re.findall(r'\btype\s*=\s*"([^"]+)"', text)).items()))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--legacy-root", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, required=True)
    args = parser.parse_args()
    legacy, output = args.legacy_root.resolve(), args.output_root.resolve()
    if output == legacy or legacy in output.parents or output in legacy.parents:
        parser.error("Output must be separate from the read-only legacy tree")
    media = legacy / "Exes/media"
    source = media / "models/mapaAlberto/mapaAlberto.mesh"
    material_source = media / "materials/scripts/mapaAlberto/mapaAlberto.material"
    if not source.is_file() or not material_source.is_file():
        parser.error("Expected original Factory mesh and material script")
    output.mkdir(parents=True, exist_ok=True)
    stage = output / "staged"
    stage.mkdir(exist_ok=True)
    # Assimp resolves this by basename; original Ogre used resources.cfg.
    shutil.copyfile(source, stage / source.name)
    shutil.copyfile(material_source, stage / material_source.name)
    scene = assimp_py.import_file(str(stage / source.name), assimp_py.Process_Triangulate)
    materials = material_blocks(material_source)
    texture_root = media / "materials/textures/mapaAlberto"
    textures = {p.name.casefold(): p for p in texture_root.iterdir() if p.is_file()}
    doc = {"asset": {"version": "2.0", "generator": "Gloom static Factory probe"},
           "scene": 0, "scenes": [{"nodes": [0]}],
           "nodes": [{"name": "Factory original units", "mesh": 0}],
           "meshes": [{"name": "mapaAlberto", "primitives": []}],
           "materials": [], "textures": [], "images": [],
           "samplers": [{"wrapS": 10497, "wrapT": 10497}],
           "bufferViews": [], "accessors": []}
    binary = bytearray()
    manifest = {"source_sha256": digest(source), "material_sha256": digest(material_source),
                "assimp_py": importlib.metadata.version("assimp-py"),
                "mesh_header": source.read_bytes()[2:].split(b"\n", 1)[0].decode(),
                "scope": "Static geometry, original normals, UV0, base color and normal textures only; no specular/glow/shader translation, entities or collision in glTF",
                "materials": [], "submeshes": [], "textures": []}
    image_ids = {}

    def add_texture(name):
        if name in image_ids:
            return image_ids[name]
        original = textures.get(name.casefold())
        if original is None:
            raise ValueError(f"Missing Factory texture: {name}")
        target = "textures/" + original.name + ".png"
        (output / "textures").mkdir(exist_ok=True)
        with Image.open(original) as image:
            dimensions = image.size
            image.convert("RGBA").save(output / target)
        index = len(doc["images"])
        doc["images"].append({"uri": target})
        doc["textures"].append({"source": index, "sampler": 0})
        image_ids[name] = index
        manifest["textures"].append({"source": str(original.relative_to(legacy)),
                                     "sha256": digest(original), "size": dimensions})
        return index

    for material in scene.materials:
        name = material["NAME"]
        block = materials.get(name)
        if block is None:
            raise ValueError(f"Missing Factory material declaration: {name}")
        units = dict(re.findall(r"texture_unit\s+(DiffMap|NormalMap|SpecMap)\s*\{\s*texture\s+(\S+)", block))
        referenced = sorted(set(re.findall(r"\btexture\s+(\S+)", block)))
        missing = [t for t in referenced if t.casefold() not in textures]
        if missing:
            raise ValueError(f"Missing textures in {name}: {missing}")
        result = {"name": name, "pbrMetallicRoughness": {
            "metallicFactor": 0, "roughnessFactor": 0.55},
            "extras": {"legacyMaterial": name, "probeOnly": True,
                       "untranslatedTextureReferences": referenced}}
        # White/light fixed-function scripts have unnamed units.
        color = units.get("DiffMap") or (referenced[0] if referenced else None)
        if color:
            result["pbrMetallicRoughness"]["baseColorTexture"] = {"index": add_texture(color)}
        if "NormalMap" in units:
            result["normalTexture"] = {"index": add_texture(units["NormalMap"])}
        doc["materials"].append(result)
        manifest["materials"].append({"name": name, "units": units,
                                      "all_texture_references": referenced})

    def accessor(values, width, kind, component=5126, bounds=False):
        if len(values) % width or not all(math.isfinite(v) for v in values):
            raise ValueError("Invalid attribute data")
        while len(binary) % 4:
            binary.append(0)
        start = len(binary)
        binary.extend(struct.pack("<" + ("f" if component == 5126 else "I") * len(values), *values))
        view = len(doc["bufferViews"])
        doc["bufferViews"].append({"buffer": 0, "byteOffset": start, "byteLength": len(binary) - start})
        item = {"bufferView": view, "componentType": component,
                "count": len(values) // width, "type": kind}
        if bounds:
            item["min"] = [min(values[i::width]) for i in range(width)]
            item["max"] = [max(values[i::width]) for i in range(width)]
        doc["accessors"].append(item)
        return len(doc["accessors"]) - 1

    # This one mesh has identity nodes. Refuse to flatten transformed scenes.
    identity = ((1., 0., 0., 0.), (0., 1., 0., 0.), (0., 0., 1., 0.), (0., 0., 0., 1.))
    def check_node(node):
        if node.transformation != identity:
            raise ValueError("Probe does not flatten node transforms")
        for child in node.children:
            check_node(child)
    check_node(scene.root_node)
    for mesh in scene.meshes:
        positions, indices = list(mesh.vertices), list(mesh.indices)
        if not indices or len(indices) % 3 or max(indices) >= mesh.num_vertices:
            raise ValueError("Invalid triangle list")
        if not mesh.normals or not mesh.texcoords or mesh.num_uv_components[0] != 2:
            raise ValueError("Probe requires original normals and UV0")
        attr = {"POSITION": accessor(positions, 3, "VEC3", bounds=True),
                "NORMAL": accessor(list(mesh.normals), 3, "VEC3"),
                # Assimp keeps Ogre UVs; this experiment does not certify UV/normal handedness.
                "TEXCOORD_0": accessor(list(mesh.texcoords[0]), 2, "VEC2")}
        doc["meshes"][0]["primitives"].append({"attributes": attr,
            "indices": accessor(indices, 1, "SCALAR", 5125), "material": mesh.material_index})
        manifest["submeshes"].append({"name": mesh.name, "vertices": mesh.num_vertices,
            "triangles": len(indices) // 3, "material": scene.materials[mesh.material_index]["NAME"],
            "min": [min(positions[i::3]) for i in range(3)],
            "max": [max(positions[i::3]) for i in range(3)]})
    doc["buffers"] = [{"byteLength": len(binary), "uri":
                       "data:application/octet-stream;base64," + base64.b64encode(binary).decode()}]
    (output / "factory-probe.gltf").write_text(json.dumps(doc, indent=2), encoding="utf-8")
    manifest["vertices"] = sum(m["vertices"] for m in manifest["submeshes"])
    manifest["triangles"] = sum(m["triangles"] for m in manifest["submeshes"])
    manifest["bounds"] = {"min": [min(m["min"][i] for m in manifest["submeshes"]) for i in range(3)],
                          "max": [max(m["max"][i] for m in manifest["submeshes"]) for i in range(3)]}
    manifest["maps"] = {p.name: map_summary(p) for p in sorted((media / "maps").glob("Factory*"))}
    model_paths = list((media / "models").rglob("*"))
    manifest["model_file_counts"] = dict(sorted(Counter(p.suffix.lower() for p in model_paths if p.is_file()).items()))
    manifest["mesh_versions"] = dict(Counter(p.read_bytes()[2:].split(b"\n", 1)[0].decode("ascii", "replace")
                                              for p in model_paths if p.suffix.lower() == ".mesh"))
    manifest["skeleton_files"] = [str(p.relative_to(media)) for p in model_paths if p.suffix == ".skeleton"]
    repx = ET.parse(media / "models/mapaAlberto/mapaAlberto.RepX").getroot()
    manifest["collision"] = [{"points": len(m.findtext("Points").split()) // 3,
                               "triangles": len(m.findtext("Triangles").split()) // 3}
                              for m in repx.findall("PxTriangleMesh")]
    manifest["collision_actor_poses"] = [e.text.strip() for e in repx.iter("GlobalPose")]
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps({k: manifest[k] for k in ("vertices", "triangles", "bounds", "collision", "mesh_versions")}, indent=2))


if __name__ == "__main__":
    main()
