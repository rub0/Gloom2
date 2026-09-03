# Factory visual review

This is the retained milestone 58 **blockout** regression gate. Milestones
59–61 additionally test the original map with `gloom.factory_visual_review`;
see [FACTORY_RESTORATION.md](FACTORY_RESTORATION.md). Both suites must pass.

Milestone 58 fixes first-person orientation/framing, inward project triangles,
missing-normal smoothing, nonuniform normal transforms and shadow sampling.
It adds an image-based acceptance gate alongside the existing behavioral tests.
See [ADR 0060](architecture/0060-visual-correctness-and-capture-regression.md).

## Capture and compare

```powershell
& 'D:\Dev\CMake\bin\cmake.exe' --build --preset windows-debug
& 'D:\Dev\CMake\bin\ctest.exe' --preset windows-debug -R '^gloom.visual_review$' --output-on-failure
```

CTest writes full-resolution P6 PPM images, `render.log` and `comparison.txt` to
`build/windows-vs/visual-review/Debug`. It fails on capture errors, Vulkan
validation errors, missing references or excessive image differences.

For a separate diagnostic run:

```powershell
.\build\windows-vs\Debug\gloom.exe --visual-review .cache\visual-review
.\build\windows-vs\Debug\gloom_visual_capture_tests.exe .cache\visual-review tests\visual-references
```

The views cover Factory overview, forward/right-axis/up/down weapon placement,
contact with the central wall, shadow detail, active Bite and active Guard.
They share normal asset residency, renderer, visibility, lights and first-person
presentation. Only the camera and presentation snapshot are controlled; the
test does not simulate authoritative damage or replace gameplay tests.

Simulation/input remain frozen, all six authored scenes must load, render scale
is 100%, and each pose gets 32 frames of temporal settling. The current window
is 1280x720; the comparator accepts dimensions that are exact multiples of
160x90. Keep the test window open until the executable closes itself. A high-DPI
environment with different dimensions reports an explicit error rather than
silently comparing misaligned images.

## What to inspect

1. Weapon and ability props keep the same camera-relative position and shape
   when looking around, with clear central visibility.
2. The weapon remains visible next to the wall. This uses separate visual depth;
   shots and collisions continue to originate from the authoritative player.
3. Top/side faces have consistent lighting and intentional hard edges, without
   the previous inward-face gradients and black bands. Genuine shaded faces
   remain darker than illuminated faces.
4. Shadows remain attached to objects, are filtered at their edges, and do not
   disappear solely because the caster leaves the camera view. HUD and weapon
   props must not cast world shadows.
5. Lava remains one-sided and upward-facing. Existing gameplay hazard tests
   continue to validate its independent collision/damage behavior.

## Reference policy

References in `tests/visual-references` are 160x90 box-filtered RGB images from
manually inspected full-resolution captures. The comparator reports mean
absolute channel error, changed-pixel percentage and worst regional error.
All three thresholds must pass: 4/255 globally, 1.5% above 32/255 per pixel,
and 12/255 in any 20x15 tile. This tolerates small edge/driver differences while
detecting missing foreground geometry and the original rendering faults.

After an intentional visual change, inspect every full-resolution image and
compare it with the prior result before explicitly regenerating references:

```powershell
.\build\windows-vs\Debug\gloom_visual_capture_tests.exe .cache\visual-review tests\visual-references --write-reference
```

Normal tests never update references. Do not approve a reference solely because
the executable exited successfully. Do not increase tolerance to hide an
unexplained difference. The comparator includes blank-image and missing-
foreground negative controls; CPU tests separately verify winding, normals and
camera transforms so a mistaken reference cannot waive those contracts.

The models remain blockouts. Passing this gate establishes the inspected visual
baseline; it does not complete textures, character art, animation, sound or UI.
