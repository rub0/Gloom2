# Runtime anterior

Datos de diagnóstico del hito 117. Capturas/builds/blobs permanecen en caché.

```json
{
  "commit": "54a51b7525112c11b8b84479ff8d347c490d9124",
  "exe_sha256": "d3209032af27ab4bd639f81b144cc2025dfaf99bc022c1089b8a28471f847351",
  "profile": false,
  "finished_utc": "2026-10-07T08:16:55.432365+00:00",
  "source_diff_sha256": "bc9bca3413f6de4493510e60a0985404fcfe7a4d9d96c6fdf34d74dd49258c84",
  "runs": [
    {
      "case": "factory-1",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.025 acquire=0.002 fence_wait=1.492 ms",
        "Factory benchmark: samples=360 mean=2.349 ms fps=425.77 p50=2.345 p95=2.373 p99=2.442 max=2.696 ms",
        "CPU stages ms: update=0.144 begin=0.019 presentation=0.106 visibility=0.026 lighting=0.019 draw=0.458 end_present=1.578",
        "CPU animation ms: poses=0.076 skin_bounds=0.004",
        "GPU full render: samples=359 mean=2.056 p50=2.055 p95=2.066 p99=2.134 max=2.189 ms",
        "Process memory: private=1088.80 MiB working_set=606.25 MiB peak_working_set=704.35 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.051 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1514 completed, 948 helped by the waiting thread, peak queue 102, 2438009 us executing and 546064 us waiting."
      ]
    },
    {
      "case": "factory-2",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.026 acquire=0.002 fence_wait=1.484 ms",
        "Factory benchmark: samples=360 mean=2.346 ms fps=426.35 p50=2.343 p95=2.364 p99=2.390 max=2.701 ms",
        "CPU stages ms: update=0.142 begin=0.018 presentation=0.105 visibility=0.026 lighting=0.018 draw=0.464 end_present=1.572",
        "CPU animation ms: poses=0.075 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.055 p50=2.055 p95=2.063 p99=2.080 max=2.337 ms",
        "Process memory: private=1093.26 MiB working_set=570.50 MiB peak_working_set=674.76 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.055 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1572 completed, 991 helped by the waiting thread, peak queue 102, 2444184 us executing and 547535 us waiting."
      ]
    },
    {
      "case": "factory-3",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.043 queue_present=0.026 acquire=0.002 fence_wait=1.476 ms",
        "Factory benchmark: samples=360 mean=2.341 ms fps=427.17 p50=2.339 p95=2.360 p99=2.377 max=3.103 ms",
        "CPU stages ms: update=0.140 begin=0.018 presentation=0.105 visibility=0.026 lighting=0.019 draw=0.467 end_present=1.565",
        "CPU animation ms: poses=0.075 skin_bounds=0.004",
        "GPU full render: samples=359 mean=2.050 p50=2.049 p95=2.060 p99=2.127 max=2.171 ms",
        "Process memory: private=1088.33 MiB working_set=570.73 MiB peak_working_set=675.05 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.045 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1536 completed, 961 helped by the waiting thread, peak queue 102, 2456268 us executing and 565581 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.047 queue_present=0.026 acquire=0.003 fence_wait=1.547 ms",
        "Factory benchmark: samples=360 mean=3.284 ms fps=304.53 p50=3.274 p95=3.509 p99=3.540 max=3.723 ms",
        "CPU stages ms: update=0.236 begin=0.020 presentation=0.595 visibility=0.035 lighting=0.019 draw=0.730 end_present=1.650",
        "CPU animation ms: poses=0.508 skin_bounds=0.052",
        "GPU full render: samples=359 mean=2.990 p50=2.980 p95=3.212 p99=3.224 max=3.424 ms",
        "Process memory: private=1794.46 MiB working_set=1024.26 MiB peak_working_set=1352.59 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.435 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 564 helped by the waiting thread, peak queue 125, 3851475 us executing and 1261836 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.047 queue_present=0.026 acquire=0.003 fence_wait=1.531 ms",
        "Factory benchmark: samples=360 mean=3.274 ms fps=305.41 p50=3.274 p95=3.512 p99=3.545 max=3.619 ms",
        "CPU stages ms: update=0.238 begin=0.020 presentation=0.599 visibility=0.035 lighting=0.018 draw=0.730 end_present=1.634",
        "CPU animation ms: poses=0.512 skin_bounds=0.053",
        "GPU full render: samples=359 mean=2.981 p50=2.978 p95=3.212 p99=3.237 max=3.325 ms",
        "Process memory: private=1771.33 MiB working_set=995.22 MiB peak_working_set=1327.65 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.890 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 572 helped by the waiting thread, peak queue 125, 3943397 us executing and 1309442 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.632 ms",
        "Factory benchmark: samples=360 mean=3.279 ms fps=304.93 p50=3.274 p95=3.513 p99=3.531 max=3.794 ms",
        "CPU stages ms: update=0.224 begin=0.019 presentation=0.567 visibility=0.030 lighting=0.018 draw=0.698 end_present=1.724",
        "CPU animation ms: poses=0.490 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.985 p50=2.978 p95=3.209 p99=3.228 max=3.442 ms",
        "Process memory: private=1779.66 MiB working_set=997.32 MiB peak_working_set=1330.36 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.430 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 562 helped by the waiting thread, peak queue 125, 3753757 us executing and 1227740 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.043 queue_present=0.025 acquire=0.002 fence_wait=1.605 ms",
        "Factory benchmark: samples=360 mean=3.291 ms fps=303.82 p50=3.284 p95=3.523 p99=3.700 max=3.873 ms",
        "CPU stages ms: update=0.240 begin=0.020 presentation=0.573 visibility=0.029 lighting=0.018 draw=0.712 end_present=1.700",
        "CPU animation ms: poses=0.495 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.993 p50=2.984 p95=3.217 p99=3.235 max=3.537 ms",
        "Process memory: private=1774.27 MiB working_set=994.57 MiB peak_working_set=1328.70 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.894 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 556 helped by the waiting thread, peak queue 125, 3811975 us executing and 1257879 us waiting."
      ]
    }
  ]
}
```
