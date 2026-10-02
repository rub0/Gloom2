# Medidas H07 / v17

Release, Ryzen 7 3700X/GTX 1070, 1920×1080 nativo, IMMEDIATE, VSync off; 120 frames de calentamiento y 360 medidos por proceso.
Siete TPS con cinco armas y FPS local; 32 mapas 2K/115.116 claves diagnósticos de H06. Ruta game:/h07-v17/hound.gltf, TPS LOD0.
Serie completa en orden, sin builds/tests simultáneos. CPU incluye HUD/audio/disparo y Present; GPU excluye Present.

| Pasada | FPS | CPU media | p50 | p95 | p99 | Máximo ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Previa, lector interrumpido | 277.04 | 3.610 | 3.591 | 4.055 | 4.242 | 4.580 |
| v17-lod0-null-a | 309.88 | 3.227 | 3.248 | 3.491 | 3.528 | 3.540 |
| v17-lod0-null-b | 306.53 | 3.262 | 3.252 | 3.495 | 3.511 | 3.526 |
| v17-lod0-audio-a | 305.90 | 3.269 | 3.261 | 3.497 | 3.512 | 3.534 |
| v17-lod0-audio-b | 306.42 | 3.263 | 3.260 | 3.497 | 3.528 | 3.554 |

La pasada previa sí superó el margen de p99 ≤4 ms, sin superar 5 ms. No se borra ni se atribuye a ruido o al lector:
el lector añadió por error cero aplazamientos acumulados de uploads durante el arranque; el 106 también registra 31.
Se corrigió únicamente esa comprobación nueva: siguen exigidos cero evicciones totales, recursos ausentes y aplazamientos durante la medida.
Tras corregirla se ejecutó la serie completa de cuatro, todas dentro de p99 ≤4/max ≤5 ms. La causa del tiempo mayor previo no está establecida.
Cinco observaciones conservadas: 0/1.800 frames >5 ms; margen p99 de 4 ms cumplido en 4/5, objetivo de 5 ms en 5/5.
No extrapola una partida humana/autoritativa de ocho ni mapas/clips finales. Repetir al cambiar rig, UV/bakes o clips.

## Distribuciones y contadores de la serie completa

### v17-lod0-null-a

```text
Frame budget: over_5ms=0/360
Swapchain: effective_mode=0 images=2; CPU mean flush=0.043 queue_present=0.024 acquire=0.002 fence_wait=1.417 ms
Factory benchmark: samples=360 mean=3.227 ms fps=309.88 p50=3.248 p95=3.491 p99=3.528 max=3.540 ms
CPU animation ms: poses=0.516 skin_bounds=0.104
GPU full render: samples=359 mean=2.943 p50=2.966 p95=3.206 p99=3.229 max=3.231 ms
Process memory: private=1791.69 MiB working_set=993.06 MiB peak_working_set=1327.63 MiB
Audio output: null; drawable=1920x1080; windowed
Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464
Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0
Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.434 ms
GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30
Texture compression: BC5/BC7; measured missing meshes=0 textures=0
H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31
```

### v17-lod0-null-b

```text
Frame budget: over_5ms=0/360
Swapchain: effective_mode=0 images=2; CPU mean flush=0.047 queue_present=0.026 acquire=0.003 fence_wait=1.343 ms
Factory benchmark: samples=360 mean=3.262 ms fps=306.53 p50=3.252 p95=3.495 p99=3.511 max=3.526 ms
CPU animation ms: poses=0.532 skin_bounds=0.117
GPU full render: samples=359 mean=2.974 p50=2.961 p95=3.199 p99=3.207 max=3.212 ms
Process memory: private=1768.29 MiB working_set=990.71 MiB peak_working_set=1324.32 MiB
Audio output: null; drawable=1920x1080; windowed
Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464
Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0
Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.436 ms
GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30
Texture compression: BC5/BC7; measured missing meshes=0 textures=0
H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31
```

### v17-lod0-audio-a

```text
Frame budget: over_5ms=0/360
Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.023 acquire=0.002 fence_wait=1.505 ms
Factory benchmark: samples=360 mean=3.269 ms fps=305.90 p50=3.261 p95=3.497 p99=3.512 max=3.534 ms
CPU animation ms: poses=0.521 skin_bounds=0.093
GPU full render: samples=359 mean=2.979 p50=2.967 p95=3.202 p99=3.214 max=3.217 ms
Process memory: private=1769.99 MiB working_set=993.27 MiB peak_working_set=1325.75 MiB
Audio output: SDL device; drawable=1920x1080; windowed
Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464
Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0
Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.429 ms
GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30
Texture compression: BC5/BC7; measured missing meshes=0 textures=0
H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31
```

### v17-lod0-audio-b

```text
Frame budget: over_5ms=0/360
Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.493 ms
Factory benchmark: samples=360 mean=3.263 ms fps=306.42 p50=3.260 p95=3.497 p99=3.528 max=3.554 ms
CPU animation ms: poses=0.517 skin_bounds=0.092
GPU full render: samples=359 mean=2.973 p50=2.968 p95=3.202 p99=3.214 max=3.220 ms
Process memory: private=1771.07 MiB working_set=994.54 MiB peak_working_set=1327.95 MiB
Audio output: SDL device; drawable=1920x1080; windowed
Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464
Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0
Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.437 ms
GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30
Texture compression: BC5/BC7; measured missing meshes=0 textures=0
H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31
```
