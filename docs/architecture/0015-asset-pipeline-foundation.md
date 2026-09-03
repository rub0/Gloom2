# ADR 0015: Asset pipeline foundation

## Status

Accepted for cooked asset format v1.

## Context

The renderer still owns a hard-coded cube. Loading authoring files directly on
the render thread would couple import libraries, filesystem layout and GPU
resource creation, make startup nondeterministic, and prevent platform-specific
optimization. Assets also need stable references that survive moving the
project between machines and between the `C:` and `D:` drives.

glTF 2.0 is an API-neutral runtime delivery format with hierarchical scenes,
binary geometry and metallic-roughness PBR materials. It is a good interchange
format, but it is not Gloom's runtime object model or final GPU layout.

## Decision

### Virtual filesystem and stable identity

Engine code addresses files as normalized `mount:/relative/path` strings.
Backslashes, duplicate separators and `.` components are normalized. Native
drive paths, `..` traversal, unknown mounts and paths that resolve outside a
mounted root are rejected. Only the virtual filesystem knows physical roots.

`AssetId` is a deterministic 64-bit FNV-1a hash of the canonical virtual path
and asset type. It identifies the logical asset, not a particular build of its
contents. Catalog insertion detects duplicate IDs, so a hash collision fails
the build instead of aliasing data silently. Content fingerprints independently
decide whether cooked output is stale.

The catalog stores source and cooked paths, type, source fingerprint and direct
dependencies. It validates missing references and cycles and produces a
dependency-first order suitable for batch cooking and loading.

### Cooked format

Gloom `.gasset` files have a small little-endian envelope containing magic,
format version, type, logical ID, semantic source fingerprint, payload checksum,
sorted dependency IDs and payload size. Decoding is size-bounded and rejects
unknown versions, non-canonical dependencies, truncation, trailing bytes and
payload corruption.

Scene payload v1 contains indexed triangle vertices, normals, first UVs, mesh
ranges, core metallic-roughness material parameters, texture references, image
metadata, local node matrices, child relationships and scene roots. This is a
CPU/runtime representation; a later GPU cooker can generate meshlets, compressed
vertex streams and API-optimal texture formats without changing glTF ingestion.

### glTF import and offline cooking

`fastgltf` 0.9 parses and validates `.gltf` and `.glb` input and loads external
geometry buffers. Gloom copies supported data into its own structures and does
not expose fastgltf types outside the importer. glTF's right-handed, +Y-up,
meter-based coordinate convention is preserved in cooked data.

The first cooker accepts triangle meshes, POSITION, optional NORMAL and
TEXCOORD_0, node hierarchy and core PBR materials. External image files become
separate texture assets and scene dependencies. Texture bytes are preserved
verbatim in v1; decoding, KTX2/Basis transcoding, mip generation and GPU format
selection belong to the texture milestone.

Unsupported data fails explicitly instead of disappearing. The current cooker
rejects animations, skins, morph targets, Draco primitives, embedded images and
non-local image URIs. Required glTF extensions not enabled in fastgltf are also
rejected by the parser.

`gloom_asset_cooker` is a command-line offline tool:

```powershell
.\build\windows-vs\Debug\gloom_asset_cooker.exe `
  D:\ProjectAssets D:\ProjectCache `
  game:/models/scene.gltf cache:/models/scene.gasset
```

### Asynchronous runtime loading

`AsyncAssetLoader` schedules disk reads and validation on Gloom's job system,
deduplicates concurrent requests and caches shared futures. It verifies cooked
metadata against the catalog before returning data. Missing, stale or corrupt
assets return a typed error placeholder plus a diagnostic instead of leaving an
invalid handle. Queue and byte metrics make streaming behavior observable.

## Consequences and limits

Import, storage, scheduling and rendering are separate APIs. Runtime code no
longer needs fastgltf or authoring-file knowledge, and physical paths never leak
into asset references. The deterministic fixture test covers the whole route
from glTF plus external binary/image files to asynchronous cooked loading.

The loader currently returns CPU payloads and does not manage residency,
cancellation, priority or GPU upload. Asset IDs remain path-stable rather than
rename-stable; preserving identity through renames will require manifest GUIDs
or redirect records. The v1 FNV ID is not a security hash, while the payload
checksum is corruption detection rather than cryptographic authentication.

## Validation

`gloom.assets` checks path normalization and traversal rejection, deterministic
IDs, dependency order and cycle detection, real fastgltf geometry/material/node
import, external texture cooking, scene round trips, checksum corruption,
asynchronous cache deduplication and error placeholders.

## References

- [Khronos glTF 2.0 specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html)
- [fastgltf](https://github.com/spnda/fastgltf)
- [Khronos glTF Validator](https://github.khronos.org/glTF-Validator/)
