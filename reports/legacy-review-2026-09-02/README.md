# Original Gloom asset review — 2026-09-02

Scope: refine the user's points 5–8. No game source, existing glTF assets,
protocol, visual-test references or original repository files were changed.
The detailed implementation sequence is [ART_RESTORATION.md](../../docs/ART_RESTORATION.md).

## Reference inspection

Video: https://www.youtube.com/watch?v=yJPoulcfIAg, first 120 seconds only.
Public video was downloaded into the ignored cache; reviewed a 5-second frame
sequence over 0–120 and a denser 0.5-second sequence over 15–65 seconds.
`factory-00m45s.png` is an extracted original frame, not a modern-engine render.
The video alternates Factory and Dungeons; effects shown in Dungeons inform
shader requirements, not Factory layout. Menus are not sufficiently shown to
claim video-based menu fidelity.

Local reference: `D:/Projects/Gloom-Legacy`, clean working tree, commit
`fe59e723594cc13e0fc95a1f390d0f81f58dc45b`. The public franaisa/Gloom master was
checked at the same commit. Factory mesh SHA-256 matches the public binary;
Factory.map, Factory_client.txt, Factory_server.txt and mapaAlberto.material
match after line-ending normalization. The local clone uses the rub0/Gloom remote.

## Concrete checks

- `tools/legacy/probe_factory.py` ran successfully with Python 3.12,
  assimp-py 1.1.0 and Pillow 12.3.0, against the read-only original.
- Factory: Ogre v1.8; 16 submeshes with 16 material assignments; 16,351
  triangles, 49,053 expanded vertices, finite positions and valid indices.
  Original normals and UV0 retained. All 38 texture names in those material
  scripts resolve; 26 unique color/normal images converted to RGBA PNG.
- The real `gloom_asset_cooker.exe` accepted `factory-probe.gltf` and returned:
  `Cooked scene 17524678270999803876 with 26 external dependencies.` Exit 0.
  The output directory contains 27 cooked files totaling 7,412,667 bytes.
- Additional direct geometry probes succeeded for Archangel (4,936 triangles),
  Shadow (2,788), Soul Reaper (1,600), Minigun (4,734), and the Factory-folder
  lavaFondo mesh (182). Normals and UV0 were present. See `other-meshes.json`.
- Inventory: 133 mesh files; 117 v1.8, 16 v1.41. Assimp explicitly rejected the
  v1.41 armourSmall sample and requested OgreMeshUpgrader. This is a known
  conversion step, not evidence that every mesh is directly supported.
- RepX: 3,564 points, 5,856 triangles; indices 0–3563. Actor quaternion
  `(0.707107, 0, 0, 0.707107)` is approximately +90 degrees about X. Applying
  that pose yields bounds `(-417.374, -37.8477, -300.147)` to
  `(337.181, 146.538, 146.955)`, different from the visual bounds in manifest.json.
  Collision still needs contact/route inspection and conversion to Jolt.
- Archangel skeleton contains animation chunk 0x4000 entries named `forward`,
  `idle`, `jump`, `strafe_right`, with approximate durations 0.667, 0.5, 0.333,
  0.667 seconds. No complete skin/animation export or playback was tested.
- Legacy repository remained clean after the probes.

## Limits

The cooked proof is not integrated into the game or visually accepted. The
probe fixes metallic=0 and roughness=0.55 as diagnostic placeholders and does
not translate specular/glow, multipass blending, lava animation, entities,
collision, bones, clips or all UV sets. It does not validate normal-map
handedness or pixel fidelity. Assimp's default material import loses the named
shader texture units; the probe explicitly resolves their selected bindings.

No C++ runtime changed, so the full test suite was not rerun for this planning
revision. The last full result remains milestone 58's 30/30. The cooker run
above is the new validation. Milestones 59–64 remain planned.

Large reproducible outputs are in `.cache/legacy-review/probe` and `cooked`.
The manifest and this report are retained for subsequent implementation.
