# Factory visual correction review — 2026-09-02

Requested scope: first-person props, abnormal shading, shadows/lighting and
reproducible visual validation. Existing networking work and other workspace
changes were preserved; no commits were created.

![Before and after](before-after.png)

## Corrected causes

- Six closed authored glTF sources had twelve inward-facing triangles each.
  Their indices now face outward; the already-correct lava plane was preserved.
- Missing normals were smoothed across hard edges. They are now generated per
  face, with corner splitting before optimization.
- Nonuniformly scaled normals used the world matrix instead of its linear
  inverse transpose. Tangent frames now also handle parallel fallback tangents.
- Weapon and ability meshes followed camera position but retained world-axis
  orientation. Camera-relative translation and rotation now agree, with smaller
  silhouettes and separate depth near walls.
- Shadow filtering, coverage, cascade selection and stability were improved.
  Offscreen casters are retained; props and HUD no longer cast world shadows.
  Factory ambient fill was adjusted for shaded-face readability.

## Evidence

All nine full-resolution corrected views are saved beside this report as PNG:
Factory, forward/yaw/up/down weapon, near-wall, shadow detail, Bite and Guard.
These remain primitive assets, not final artwork.

The Debug build succeeded. The complete CTest run passed **30/30** in 45.56
seconds. The new `gloom.visual_review` gate took 4.86 seconds and matched every
reference exactly on a fresh run. No Vulkan validation failures were reported.
CPU tests cover normal direction, hard edges, nonuniform normal transforms,
and full-yaw/steep-pitch view-model placement.

For a negative control, the seven pre-fix captures were compared with the new
references. Their mean errors ranged from 14.5 to 42.8/255, exceeding the 4/255
limit; all seven failed. Near-wall and shadow-detail had no pre-fix capture, so
the corrected versions were used only as neutral placeholders in that negative
control. The comparator also rejects blank images and a missing foreground
quadrant. Tests never regenerate their own references.

Commands, thresholds and reference policy are in
[VISUAL_REVIEW.md](../../docs/VISUAL_REVIEW.md); design decisions are in
[ADR 0060](../../docs/architecture/0060-visual-correctness-and-capture-regression.md).
Raw logs and full-resolution PPM captures remain under `.cache/visual-fixes`
and `build/windows-vs/visual-review/Debug`.
