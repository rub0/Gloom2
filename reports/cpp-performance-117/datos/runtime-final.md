# Runtime final

Datos de diagnóstico del hito 117. Capturas/builds/blobs permanecen en caché.

```json
{
  "commit": "54a51b7525112c11b8b84479ff8d347c490d9124",
  "exe_sha256": "b5260790409ffc5a0c64cfdcb7c66abc5de0813b766cd8eaf69f7821f14d1eb4",
  "profile": false,
  "finished_utc": "2026-10-07T09:07:14.243905+00:00",
  "source_diff_sha256": "03772b254c316f1c8f5dc94f2f36a94952ef4da61645ab4c6085c8cc91bce8d5",
  "runs": [
    {
      "case": "factory-1",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.025 acquire=0.002 fence_wait=1.487 ms",
        "Factory benchmark: samples=360 mean=2.342 ms fps=427.00 p50=2.342 p95=2.361 p99=2.375 max=2.403 ms",
        "CPU stages ms: update=0.141 begin=0.019 presentation=0.106 visibility=0.025 lighting=0.018 draw=0.458 end_present=1.574",
        "CPU animation ms: poses=0.077 skin_bounds=0.004",
        "GPU full render: samples=359 mean=2.047 p50=2.047 p95=2.056 p99=2.062 max=2.068 ms",
        "Process memory: private=1072.86 MiB working_set=571.34 MiB peak_working_set=702.40 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.044 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1238 completed, 665 helped by the waiting thread, peak queue 107, 2433160 us executing and 780134 us waiting."
      ]
    },
    {
      "case": "factory-2",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.505 ms",
        "Factory benchmark: samples=360 mean=2.342 ms fps=426.97 p50=2.341 p95=2.360 p99=2.379 max=2.497 ms",
        "CPU stages ms: update=0.136 begin=0.018 presentation=0.104 visibility=0.025 lighting=0.018 draw=0.451 end_present=1.589",
        "CPU animation ms: poses=0.075 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.049 p50=2.050 p95=2.058 p99=2.061 max=2.067 ms",
        "Process memory: private=1076.51 MiB working_set=570.59 MiB peak_working_set=701.52 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.039 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1286 completed, 721 helped by the waiting thread, peak queue 107, 2402855 us executing and 765684 us waiting."
      ]
    },
    {
      "case": "factory-3",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=1/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.061 queue_present=0.024 acquire=0.002 fence_wait=1.497 ms",
        "Factory benchmark: samples=360 mean=2.358 ms fps=424.16 p50=2.343 p95=2.363 p99=2.380 max=7.870 ms",
        "CPU stages ms: update=0.137 begin=0.019 presentation=0.103 visibility=0.025 lighting=0.018 draw=0.454 end_present=1.601",
        "CPU animation ms: poses=0.074 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.050 p50=2.050 p95=2.058 p99=2.068 max=2.224 ms",
        "Process memory: private=1072.29 MiB working_set=569.45 MiB peak_working_set=704.28 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.043 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1285 completed, 718 helped by the waiting thread, peak queue 107, 2411668 us executing and 771524 us waiting."
      ]
    },
    {
      "case": "hound-null-1",
      "options": [
        "--vertical-slice-performance-hound-eight-1080p",
        "--hound-production-v17",
        "--hound-budget-lod=0"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.043 queue_present=0.024 acquire=0.002 fence_wait=1.612 ms",
        "Factory benchmark: samples=360 mean=3.281 ms fps=304.80 p50=3.272 p95=3.509 p99=3.526 max=3.530 ms",
        "CPU stages ms: update=0.227 begin=0.020 presentation=0.573 visibility=0.031 lighting=0.018 draw=0.704 end_present=1.707",
        "CPU animation ms: poses=0.492 skin_bounds=0.052",
        "GPU full render: samples=359 mean=2.989 p50=2.978 p95=3.213 p99=3.226 max=3.233 ms",
        "Process memory: private=1775.82 MiB working_set=994.39 MiB peak_working_set=1327.07 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.444 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 570 helped by the waiting thread, peak queue 134, 3859090 us executing and 1267885 us waiting."
      ]
    },
    {
      "case": "hound-null-2",
      "options": [
        "--vertical-slice-performance-hound-eight-1080p",
        "--hound-production-v17",
        "--hound-budget-lod=0"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.045 queue_present=0.025 acquire=0.002 fence_wait=1.553 ms",
        "Factory benchmark: samples=360 mean=3.284 ms fps=304.55 p50=3.274 p95=3.508 p99=3.518 max=3.531 ms",
        "CPU stages ms: update=0.236 begin=0.020 presentation=0.600 visibility=0.036 lighting=0.019 draw=0.720 end_present=1.653",
        "CPU animation ms: poses=0.512 skin_bounds=0.053",
        "GPU full render: samples=359 mean=2.991 p50=2.980 p95=3.209 p99=3.220 max=3.233 ms",
        "Process memory: private=1792.95 MiB working_set=994.89 MiB peak_working_set=1315.34 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.442 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=31",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1611 completed, 561 helped by the waiting thread, peak queue 134, 3883520 us executing and 1258061 us waiting."
      ]
    },
    {
      "case": "hound-audio-1",
      "options": [
        "--vertical-slice-performance-hound-eight-1080p",
        "--hound-production-v17",
        "--hound-budget-lod=0",
        "--audio-device"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.645 ms",
        "Factory benchmark: samples=360 mean=3.280 ms fps=304.86 p50=3.278 p95=3.510 p99=3.524 max=3.543 ms",
        "CPU stages ms: update=0.228 begin=0.018 presentation=0.561 visibility=0.029 lighting=0.017 draw=0.691 end_present=1.736",
        "CPU animation ms: poses=0.485 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.986 p50=2.979 p95=3.211 p99=3.220 max=3.241 ms",
        "Process memory: private=1777.59 MiB working_set=996.62 MiB peak_working_set=1330.03 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.890 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 561 helped by the waiting thread, peak queue 134, 3785080 us executing and 1259113 us waiting."
      ]
    },
    {
      "case": "hound-audio-2",
      "options": [
        "--vertical-slice-performance-hound-eight-1080p",
        "--hound-production-v17",
        "--hound-budget-lod=0",
        "--audio-device"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.649 ms",
        "Factory benchmark: samples=360 mean=3.280 ms fps=304.87 p50=3.279 p95=3.515 p99=3.527 max=3.534 ms",
        "CPU stages ms: update=0.228 begin=0.018 presentation=0.559 visibility=0.029 lighting=0.017 draw=0.689 end_present=1.741",
        "CPU animation ms: poses=0.483 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.984 p50=2.979 p95=3.210 p99=3.218 max=3.232 ms",
        "Process memory: private=1769.98 MiB working_set=993.89 MiB peak_working_set=1327.41 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.441 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 557 helped by the waiting thread, peak queue 134, 3769108 us executing and 1250700 us waiting."
      ]
    }
  ]
}
```
