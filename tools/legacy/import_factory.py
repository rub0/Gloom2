"""Offline, reproducible Ogre/RepX/Lua-data import. Never executes legacy scripts.

Run with --legacy-root ... --output-root assets/legacy --work-root .cache/legacy-import.
Ogre is used only offline to normalize old binaries. Skinned assets are refused.
"""
import argparse
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
import numpy as np
from PIL import Image
from probe_factory import material_blocks

SCALE = 0.15  # Legacy capsule: cylinder 6 + two radii of 3 = 12; modern height 1.8.


from lua_data import DataParser


class Importer:
    def __init__(self, legacy, output, work):
        self.legacy, self.output, self.work = legacy, output, work
        output.mkdir(parents=True, exist_ok=True)
        work.mkdir(parents=True, exist_ok=True)
        self.files, self.materials, self.sources, self.diagnostics = {}, {}, {}, []
        self.mesh_audit = []
        config = legacy / "Exes/resources.cfg"
        self.source(config)
        group = ""
        for line in config.read_text().splitlines():
            if line.startswith("["):
                group = line
            if group != "[General]" or not line.startswith("FileSystem="):
                continue
            directory = (legacy / "Exes" / line.split("=", 1)[1]).resolve()
            if not directory.is_dir():
                continue
            for p in sorted(directory.iterdir()):
                if p.is_file():
                    self.files.setdefault(p.name.lower(), []).append(p)
                    if p.suffix == ".material":
                        for name, block in material_blocks(p).items():
                            self.materials.setdefault(name, (p, block))
        import Ogre
        self.ogre = Ogre
        self.root = Ogre.Root("", "", str(work / "ogre.log"))
        self.buffers = Ogre.DefaultHardwareBufferManager()
        self.mesh_number = 0

    def source(self, path):
        key = path.relative_to(self.legacy).as_posix()
        self.sources[key] = hashlib.sha256(path.read_bytes()).hexdigest()

    def resolve(self, name):
        candidates = self.files.get(name.lower(), [])
        if not candidates:
            raise ValueError(f"Missing resource {name}")
        if len({hashlib.sha256(p.read_bytes()).digest() for p in candidates}) > 1:
            raise ValueError(f"Ambiguous resource {name}: {candidates}")
        self.source(candidates[0])
        return candidates[0]

    def data(self, name):
        path = self.legacy / "Exes/media/maps" / name
        self.source(path)
        return DataParser(path.read_text(encoding="latin1")).document()

    def mesh(self, source, allow_bind_pose=False):
        self.source(source)
        ogre = self.ogre
        number = self.mesh_number
        self.mesh_number += 1
        group = f"import{number}"
        resources = ogre.ResourceGroupManager.getSingleton()
        resources.createResourceGroup(group)
        resources.addResourceLocation(str(source.parent), "FileSystem", group)
        # Ogre 14 stores material handles; unresolved names would be lost on export.
        for name in self.materials:
            ogre.MaterialManager.getSingleton().create(name, group)
        mesh = ogre.MeshManager.getSingleton().createManual(f"mesh{number}", group)
        serializer = ogre.MeshSerializer()
        serializer.importMesh(resources.openResource(source.name, group), mesh.__deref__())
        if mesh.hasSkeleton() or mesh.getNumAnimations():
            if source.name.lower() != "minigun.mesh" or not allow_bind_pose:
                ogre.MeshManager.getSingleton().remove(mesh.getHandle())
                del mesh
                raise ValueError(f"Skinned/animated resource needs explicit rig import: {source.name}")
            # The original MiniGun rig only spins presentation parts. Gameplay
            # and both weapon anchors use its bind mesh, so export that pose.
            mesh.setSkeletonName("")
        names = [mesh.getSubMesh(i).getMaterialName() for i in range(mesh.getNumSubMeshes())]
        normalized = self.work / f"mesh{number}.mesh"
        serializer.exportMesh(mesh.__deref__(), str(normalized), ogre.MESH_VERSION_1_8)
        ogre.MeshManager.getSingleton().remove(mesh.getHandle())
        del mesh
        normalized.with_suffix(".material").write_text("\n".join(
            f"material {name}\n{{\n technique\n {{\n pass\n {{\n }}\n }}\n}}" for name in dict.fromkeys(names)))
        scene = assimp_py.import_file(str(normalized), assimp_py.Process_Triangulate)
        visited=[]
        def visit(node):
            if not np.allclose(np.asarray(node.transformation),np.eye(4),atol=1e-6):
                raise ValueError("Unsupported nonidentity normalized node: " + source.name)
            visited.extend(node.mesh_indices)
            for child in node.children: visit(child)
        visit(scene.root_node)
        if sorted(visited)!=list(range(len(scene.meshes))):
            raise ValueError("Normalized node/mesh bindings are not one-to-one")
        vertices=np.concatenate([np.asarray(m.vertices).reshape(-1,3) for m in scene.meshes])
        self.mesh_audit.append({"source":source.relative_to(self.legacy).as_posix(),
            "serializer":source.read_bytes()[2:].split(b"\n")[0].decode("ascii"),
            "normalized_serializer":"[MeshSerializer_v1.8]","submeshes":len(scene.meshes),
            "triangles":sum(m.num_indices//3 for m in scene.meshes),
            "bounds":[vertices.min(axis=0).tolist(),vertices.max(axis=0).tolist()],
            "tangent_policy":"preserve when present; otherwise MikkTSpace in cooker"})
        return scene


class Scene:
    def __init__(self, importer, name):
        self.imp, self.name = importer, name
        self.binary = bytearray()
        self.doc = {"asset": {"version": "2.0", "generator": "Gloom legacy importer 1"},
                    "scene": 0, "scenes": [{"nodes": []}], "nodes": [], "meshes": [],
                    "materials": [], "textures": [], "images": [], "samplers": [{"wrapS": 10497, "wrapT": 10497}],
                    "bufferViews": [], "accessors": [],
                    "extensionsUsed": ["KHR_materials_specular", "KHR_materials_emissive_strength"]}
        self.material_ids, self.texture_ids = {}, {}

    def accessor(self, values, width, kind, component=5126, bounds=False):
        if not values or len(values) % width or not all(math.isfinite(v) for v in values):
            raise ValueError("Invalid mesh attribute")
        while len(self.binary) % 4:
            self.binary.append(0)
        offset = len(self.binary)
        self.binary.extend(struct.pack("<" + ("f" if component == 5126 else "I") * len(values), *values))
        view = len(self.doc["bufferViews"])
        self.doc["bufferViews"].append({"buffer": 0, "byteOffset": offset, "byteLength": len(self.binary) - offset})
        record = {"bufferView": view, "componentType": component, "count": len(values) // width, "type": kind}
        if bounds:
            record.update(min=[min(values[i::width]) for i in range(width)], max=[max(values[i::width]) for i in range(width)])
        self.doc["accessors"].append(record)
        return len(self.doc["accessors"]) - 1

    def texture(self, name):
        path = self.imp.resolve(name)
        key = hashlib.sha256(path.read_bytes()).hexdigest()[:16] + ".png"
        if key not in self.texture_ids:
            target = self.imp.output / "textures" / key
            target.parent.mkdir(exist_ok=True)
            with Image.open(path) as image:
                image.convert("RGBA").save(target)
            index = len(self.doc["images"])
            self.doc["images"].append({"uri": "textures/" + key})
            self.doc["textures"].append({"source": index, "sampler": 0})
            self.texture_ids[key] = index
        return {"index": self.texture_ids[key]}

    def material(self, name):
        if name in self.material_ids:
            return self.material_ids[name]
        if name not in self.imp.materials:
            raise ValueError(f"No material definition for {name}")
        source, block = self.imp.materials[name]
        self.imp.source(source)
        variables = dict(re.findall(r"\bset\s+(\$\w+)\s+(\S+)", block))
        for key, value in variables.items():
            block = block.replace(key, value)
        units = dict(re.findall(r"texture_unit\s+(DiffMap|NormalMap|SpecMap|GlowMap)\s*\{\s*texture\s+(\S+)", block))
        references = list(dict.fromkeys(re.findall(r"\btexture\s+(\S+)", block)))
        if "$glow_tex" in variables:
            units["GlowMap"] = variables["$glow_tex"]
        # This one glow image is absent from the original repository. Retain the
        # authored diffuse geometry and report the absent optional emission.
        missing_glow = "ironhellgoat_glow.png"
        if missing_glow in references and missing_glow not in self.imp.files:
            self.imp.diagnostics.append({"material": name, "reason": "Missing optional emission: " + missing_glow})
            references.remove(missing_glow)
            units = {k:v for k,v in units.items() if v != missing_glow}
        for ref in references: self.imp.resolve(ref)
        shininess = re.search(r"param_named\s+shininess\s+float\s+([\d.]+)", block)
        exponent = float(shininess[1]) if shininess else 10
        # Blinn exponent to perceptual microfacet roughness, then artistic review.
        roughness = (2 / (exponent + 2)) ** 0.25
        record = {"name": name, "pbrMetallicRoughness": {"metallicFactor": 0, "roughnessFactor": roughness},
                  "extras": {"gloom": {"sourceMaterial": name}}, "extensions": {}}
        if references:
            record["pbrMetallicRoughness"]["baseColorTexture"] = self.texture(units.get("DiffMap", references[0]))
        if "NormalMap" in units:
            record["normalTexture"] = self.texture(units["NormalMap"])
        if "SpecMap" in units:
            record["extensions"]["KHR_materials_specular"] = {"specularFactor": 1, "specularColorTexture": self.texture(units["SpecMap"])}
        if "GlowMap" in units or name in ("mapaAlberto_luces", "lavaFondo"):
            record["emissiveTexture"] = self.texture(units.get("GlowMap", units.get("DiffMap", references[0])))
            if "GlowMap" in units and "GlowMappingFragment" in block:
                # Original glowMapping.hlsl multiplies diffuse by the glow mask.
                # Bake that operation, preserving colored emission instead of
                # treating the mostly white mask as emissive RGB.
                self.imp.source(self.imp.legacy/"Exes/media/materials/programs/glowMapping.hlsl")
                diffuse=self.imp.resolve(units.get("DiffMap",references[0]));glow=self.imp.resolve(units["GlowMap"])
                with Image.open(diffuse) as base, Image.open(glow) as mask:
                    rgb=np.asarray(base.convert("RGB"),dtype=float)/255
                    mask_rgb=np.asarray(mask.convert("RGB").resize(base.size),dtype=float)/255
                linear=np.where(rgb<=.04045,rgb/12.92,((rgb+.055)/1.055)**2.4)*mask_rgb
                encoded=np.where(linear<=.0031308,linear*12.92,1.055*linear**(1/2.4)-.055)
                key=hashlib.sha256(diffuse.read_bytes()+glow.read_bytes()+b"linear-glow-v1").hexdigest()[:16]+".png"
                Image.fromarray(np.uint8(np.clip(encoded*255,0,255))).convert("RGBA").save(self.imp.output/"textures"/key)
                index=len(self.doc["images"])
                self.doc["images"].append({"uri":"textures/"+key})
                self.doc["textures"].append({"source":index,"sampler":0})
                record["emissiveTexture"]={"index":index}
            record["emissiveFactor"] = [1, 1, 1]
            record["extensions"]["KHR_materials_emissive_strength"] = {"emissiveStrength": 2.0 if name == "lavaFondo" else 1.0}
        if name == "lavaFondo":
            record["extras"]["gloom"].update(uvScroll=[0.025, 0.015], lavaWave=0.025)
        self.material_ids[name] = len(self.doc["materials"])
        self.doc["materials"].append(record)
        return self.material_ids[name]

    def mesh(self, scene):
        primitives = []
        for mesh in scene.meshes:
            if not mesh.normals or not mesh.texcoords:
                raise ValueError("Original normals/UVs missing")
            indices = list(mesh.indices)
            if len(indices) % 3 or max(indices) >= mesh.num_vertices:
                raise ValueError("Invalid triangle indices")
            attrs = {"POSITION": self.accessor(list(mesh.vertices), 3, "VEC3", bounds=True),
                     "NORMAL": self.accessor(list(mesh.normals), 3, "VEC3"),
                     "TEXCOORD_0": self.accessor(list(mesh.texcoords[0]), 2, "VEC2")}
            if len(mesh.texcoords) > 1:
                attrs["TEXCOORD_1"] = self.accessor(list(mesh.texcoords[1]), 2, "VEC2")
            if mesh.tangents is not None and mesh.bitangents is not None:
                tangent=np.asarray(mesh.tangents).reshape(-1,3)
                normal=np.asarray(mesh.normals).reshape(-1,3)
                bitangent=np.asarray(mesh.bitangents).reshape(-1,3)
                signs=np.where(np.sum(np.cross(normal,tangent)*bitangent,axis=1)<0,-1,1)
                attrs["TANGENT"]=self.accessor(np.column_stack((tangent,signs)).ravel().tolist(),4,"VEC4")
            primitives.append({"attributes": attrs, "indices": self.accessor(indices, 1, "SCALAR", 5125),
                               "material": self.material(scene.materials[mesh.material_index]["NAME"])})
        self.doc["meshes"].append({"primitives": primitives})
        return len(self.doc["meshes"]) - 1

    def instance(self, name, mesh, position=(0, 0, 0), scale=SCALE, yaw=0):
        index = len(self.doc["nodes"])
        self.doc["nodes"].append({"name": name, "mesh": mesh, "translation": [x * SCALE for x in position],
            "scale": [scale]*3 if isinstance(scale, (int,float)) else scale,
            "rotation": [0, math.sin(math.radians(yaw)/2), 0, math.cos(math.radians(yaw)/2)]})
        self.doc["scenes"][0]["nodes"].append(index)

    def save(self):
        self.doc["buffers"] = [{"uri": self.name + ".bin", "byteLength": len(self.binary)}]
        (self.imp.output / (self.name + ".bin")).write_bytes(self.binary)
        (self.imp.output / (self.name + ".gltf")).write_text(json.dumps(self.doc, indent=2), encoding="utf-8")


def run(args):
    legacy, output, work = (p.resolve() for p in (args.legacy_root, args.output_root, args.work_root))
    for target in (output, work):
        if target == legacy or legacy in target.parents or target in legacy.parents:
            raise ValueError("Outputs must be separate from the legacy tree")
    imp = Importer(legacy, output, work)
    client, server, standalone, archetypes = [imp.data(n) for n in
        ("Factory_client.txt", "Factory_server.txt", "Factory.map", "archetypes.txt")]
    for name, data in client.items():
        if name not in server or data != server[name]:
            raise ValueError(f"Unresolved multiplayer map divergence: {name}")
    factory = Scene(imp, "factory")
    factory.instance("World", factory.mesh(imp.mesh(legacy / "Exes/media/models/mapaAlberto/mapaAlberto.mesh")))
    lava = client["lavaFondo"]
    half = lava["plane_width"] / 2
    tiling = lava["plane_uTiling"]
    factory.doc["meshes"].append({"primitives": [{"attributes": {
        "POSITION": factory.accessor([-half,0,-half, -half,0,half, half,0,half, half,0,-half],3,"VEC3",bounds=True),
        "NORMAL": factory.accessor([0,1,0]*4,3,"VEC3"),
        "TEXCOORD_0": factory.accessor([0,0, 0,tiling, tiling,tiling, tiling,0],2,"VEC2")},
        "indices": factory.accessor([0,1,2,0,2,3],1,"SCALAR",5125), "material": factory.material("lavaFondo")}]})
    factory.instance("lavaFondo",len(factory.doc["meshes"])-1,lava["position"])
    records, cached_meshes = [], {}
    for name, data in server.items():
        resolved = dict(archetypes.get(data["type"], {}))
        resolved.update(data)
        status = "metadata"
        if name not in ("World", "lavaFondo") and "model" in resolved and data["type"] not in ("Light", "Camera"):
            model = resolved["model"]
            try:
                if model not in cached_meshes:
                    cached_meshes[model] = factory.mesh(imp.mesh(imp.resolve(model)))
                scale = resolved.get("scale", 1)
                scale = [v*SCALE for v in scale] if isinstance(scale, list) else scale*SCALE
                factory.instance(name, cached_meshes[model],resolved.get("position",[0,0,0]),scale,resolved.get("yaw",0))
                status = "visual-only; gameplay deferred"
            except ValueError as error:
                status = "deferred: " + str(error)
                imp.diagnostics.append({"entity": name,"reason":str(error)})
        if data["type"] in ("World", "Lava", "SpawnPoint"):
            status = "authoritative"
        records.append({"id":name,"type":data["type"],"status":status,"source":data,"resolved":resolved})
    factory.save()
    for name, path in (("soul_reaper", "weapons/soulReaper.mesh"),
                       ("sniper", "weapons/sniper.mesh"),
                       ("shotgun", "weapons/shotGun.mesh"),
                       ("minigun", "weapons/miniGun.mesh"),
                       ("iron_hell_goat", "weapons/ironHellGoat.mesh"),
                       ("armour_small","armourSmall.mesh")):
        scene = Scene(imp,name)
        scene.instance(name,scene.mesh(imp.mesh(legacy/"Exes/media/models"/path, name == "minigun")))
        scene.save()
    repx_path = legacy / "Exes/media/models/mapaAlberto/mapaAlberto.RepX"
    imp.source(repx_path)
    repx = ET.parse(repx_path).getroot()
    mesh = repx.find("PxTriangleMesh")
    points, indices = list(map(float,mesh.findtext("Points").split())), list(map(int,mesh.findtext("Triangles").split()))
    # The checked source has one actor, +90 degrees X, identity local/mesh poses.
    pose = list(map(float,repx.findtext("PxRigidStatic/GlobalPose").split()))
    if len(pose)!=7 or any(abs(a-b)>1e-5 for a,b in zip(pose,[2**-.5,0,0,2**-.5,0,0,0])):
        raise ValueError("RepX actor pose changed; update audited conversion")
    shape = repx.find("PxRigidStatic/Shapes/PxShape")
    if len(repx.findall("PxRigidStatic"))!=1 or len(repx.findall("PxTriangleMesh"))!=1 or len(repx.findall("PxRigidStatic/Shapes/PxShape"))!=1:
        raise ValueError("RepX topology changed; unsupported additional actors/shapes")
    for path, expected in (("LocalPose",[0,0,0,1,0,0,0]),
            ("Geometry/PxTriangleMeshGeometry/Scale/Scale",[1,1,1]),
            ("Geometry/PxTriangleMeshGeometry/Scale/Rotation",[0,0,0,1])):
        actual=list(map(float,shape.findtext(path).split()))
        if actual!=expected: raise ValueError("Unsupported RepX local pose/scale: " + path)
    if len(points)%3 or len(indices)%3 or not all(math.isfinite(v) for v in points) or min(indices)<0 or max(indices)>=len(points)//3:
        raise ValueError("Invalid RepX triangles")
    transformed = [[points[i]*SCALE,-points[i+2]*SCALE,points[i+1]*SCALE] for i in range(0,len(points),3)]
    manifest = {"version":1,"unit_scale":SCALE,"entities":records,
                "standalone_differences":{k:{"multiplayer":server.get(k),"standalone":standalone.get(k)}
                                          for k in sorted(set(server)|set(standalone)) if server.get(k)!=standalone.get(k)},
                "collision":{"vertices":transformed,"indices":indices},
                "diagnostics":imp.diagnostics,"meshes":imp.mesh_audit,
                "tool_versions":{p:importlib.metadata.version(p) for p in ("assimp-py","ogre-python","Pillow","numpy")},
                "sources":dict(sorted(imp.sources.items()))}
    (output/"factory_scene.json").write_text(json.dumps(manifest,indent=2),encoding="utf-8")
    from bake_factory_probe import bake
    bake(output)
    print(f"Imported {len(factory.doc['meshes'])} meshes, {len(records)} unique records; {len(imp.diagnostics)} explicit conversion diagnostics")
    imp.ogre.MeshManager.getSingleton().removeAll()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("legacy-root", "output-root", "work-root"):
        parser.add_argument("--"+name,type=Path,required=True)
    run(parser.parse_args())
